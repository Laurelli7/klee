#!/usr/bin/env python3
"""LLM-driven KLEE searcher selection feedback loop.

Pipeline per iteration:
  1. Pick a structural pattern from real GNU ICFG chunks (via icfg_chunks.py)
  2. Ask LLM to generate a minimal C program that exhibits that pattern
  3. Ask LLM to predict which searcher will win (given structural_rules.md)
  4. Compile the program; run KLEE with all 6 searchers for --time-budget seconds
  5. Measure branch coverage from run.stats
  6. If prediction was wrong: ask LLM to reflect, then propose a rule edit
  7. Apply the rule edit to structural_rules.md (with human confirmation)
  8. Update the seen-patterns database

Usage:
    experiment.py --rounds 10 --time-budget 30
    experiment.py --source-bc /path/to/make.bc --rounds 5
    experiment.py --program mytest.c  # skip generation, just benchmark
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import rules as rules_mod
import icfg_chunks

# Try importing anthropic SDK; fall back to claude CLI if not authenticated
try:
    import anthropic as _anthropic_mod
    _HAS_SDK = True
except ImportError:
    _HAS_SDK = False

CLANG    = os.environ.get("CLANG", "/usr/bin/clang-14")
LLVM_DIS = os.environ.get("LLVM_DIS", "/usr/bin/llvm-dis-14")
KLEE_BIN = os.environ.get("KLEE_BIN", "/home/cc/klee/build/bin/klee")
KLEE_INCLUDE = os.environ.get("KLEE_INCLUDE", "/home/cc/klee/include")
KLEE_UCLIBC  = os.environ.get("KLEE_UCLIBC",  "")

# Docker-based KLEE execution (used when KLEE_BIN is not found on host)
DOCKER_CONTAINER = os.environ.get("KLEE_DOCKER", "parasuit_run")
DOCKER_KLEE      = "/root/main/parasuit/klee/build/bin/klee"
DOCKER_KLEE_INC  = "/root/main/parasuit/klee/include"
DOCKER_CLANG_ENV = "LLVM_COMPILER=clang"

MODEL         = "claude-opus-4-7"
RULES_PATH    = HERE / "structural_rules.md"
SEEN_DB_PATH  = HERE / "seen_chunks.json"
GNU_BC_PATH   = HERE.parent / "examples" / "gnumake" / "install" / "bin" / "make.bc"

SEARCHERS = ["dfs", "bfs", "random-path", "nurs:covnew", "nurs:md2u", "nurs:qc"]


# ---------------------------------------------------------------------------
# LLM calling layer (SDK with prompt caching, or claude CLI fallback)
# ---------------------------------------------------------------------------

def _rules_text() -> str:
    return RULES_PATH.read_text()


ROLE_SUFFIX = (
    "\n\nYou are an expert in KLEE symbolic execution. "
    "The document above is the current searcher selection policy. "
    "Follow it precisely. When asked to update rules, produce a "
    "minimal, targeted edit — change only what the evidence demands."
)


def _call_llm_sdk(system_blocks: list[dict], user_content: str,
                  max_tokens: int = 4096) -> str:
    """Call via Anthropic Python SDK with prompt caching."""
    _client = _anthropic_mod.Anthropic()
    resp = _client.messages.create(
        model=MODEL,
        max_tokens=max_tokens,
        system=system_blocks,
        messages=[{"role": "user", "content": user_content}],
    )
    return resp.content[0].text


def _call_llm_cli(system_text: str, user_content: str, max_tokens: int = 4096) -> str:
    """Call via claude CLI (uses existing claude.ai auth)."""
    cmd = [
        "claude", "-p",
        "--model", MODEL,
        "--system-prompt", system_text,
        user_content,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
    if result.returncode != 0:
        raise RuntimeError(f"claude CLI failed: {result.stderr[:500]}")
    return result.stdout.strip()


def _call_llm(system_blocks: list[dict], user_content: str,
              max_tokens: int = 4096) -> str:
    """Call LLM: try SDK first (has prompt caching), fall back to CLI."""
    if _HAS_SDK and os.environ.get("ANTHROPIC_API_KEY"):
        return _call_llm_sdk(system_blocks, user_content, max_tokens)

    # Build flat system text for CLI
    system_text = "\n\n".join(b["text"] for b in system_blocks)
    return _call_llm_cli(system_text, user_content, max_tokens)


def _system_with_rules() -> list[dict]:
    """System blocks: rules doc + brief role."""
    return [
        {
            "type": "text",
            "text": _rules_text() + ROLE_SUFFIX,
        },
    ]


# ---------------------------------------------------------------------------
# Step 1: Generate a C program from a structural pattern
# ---------------------------------------------------------------------------

GENERATE_PROMPT_REAL = """\
I will give you a structural profile of a real function ({fn_name}) from GNU Make.
Generate a MINIMAL, self-contained C program that reproduces the dominant structural
pattern ({target_rule}) in a form KLEE can analyze in under 30 seconds.

TARGET RULE: {target_rule}
The program must make {target_rule} the DOMINANT and ISOLATED pattern — minimize
other structural noise so the searcher difference is maximally clear.

Requirements:
- Include `#include "klee/klee.h"` and use `klee_make_symbolic`.
- The program should have a `main()` that drives the pattern.
- Keep the program under 100 lines.
- Do NOT add explanatory comments.
- Output ONLY the C source code, nothing else.

Reference structural profile from {fn_name} ({blocks} blocks, {back_edges} back-edges):
Rules fired: {rules_fired}
Active features: {active_features}
"""

GENERATE_PROMPT_SYNTHETIC = """\
Generate a MINIMAL, self-contained C program that clearly exhibits the following
structural pattern for KLEE symbolic execution experiments.

TARGET RULE: {target_rule}
Pattern description: {description}

Requirements:
- Include `#include "klee/klee.h"` and use `klee_make_symbolic`.
- The program should have a `main()` that drives the pattern.
- Keep the program under 100 lines.
- Make {target_rule} the DOMINANT pattern — do NOT mix in other structural patterns.
- The pattern must be strong enough to discriminate between KLEE searchers.
- Do NOT add explanatory comments.
- Output ONLY the C source code, nothing else.
"""


def generate_program(chunk: dict) -> str:
    """Ask LLM to generate a C program targeting the chunk's dominant rule."""
    target_rule = chunk.get("target_rule", chunk["rules_fired"][0] if chunk["rules_fired"] else "R4")

    if chunk.get("source") == "synthetic" or not chunk.get("profile"):
        prompt = GENERATE_PROMPT_SYNTHETIC.format(
            target_rule=target_rule,
            description=chunk.get("description", ""),
        )
    else:
        active = [k for k, v in chunk["profile"].items()
                  if v and v is not False and k != "scale"]
        prompt = GENERATE_PROMPT_REAL.format(
            fn_name=chunk["fn_name"],
            target_rule=target_rule,
            blocks=chunk["blocks"],
            back_edges=chunk["back_edges"],
            rules_fired=chunk["rules_fired"],
            active_features=active,
        )
    return _call_llm(_system_with_rules(), prompt, max_tokens=3000)


# ---------------------------------------------------------------------------
# Step 2: Predict searcher from C source
# ---------------------------------------------------------------------------

PREDICT_PROMPT = """\
Here is a C program I am about to run under KLEE:

```c
{source}
```

After analyzing its structure against the rules policy, which single searcher
will achieve the highest branch coverage in a 30-second budget?

Respond in this exact JSON format (nothing else):
{{
  "predicted_searcher": "<one of: dfs|bfs|random-path|nurs:covnew|nurs:md2u|nurs:qc>",
  "rules_fired": ["R1", "R9", ...],  // which rules triggered this choice
  "reasoning": "<one sentence>"
}}
"""


def predict_searcher(source: str) -> dict:
    """Ask LLM to predict the best searcher for the given C source."""
    resp = _call_llm(_system_with_rules(), PREDICT_PROMPT.format(source=source))
    # Extract the JSON block from the response
    m = re.search(r'\{.*\}', resp, re.DOTALL)
    if not m:
        raise ValueError(f"LLM did not return JSON:\n{resp}")
    return json.loads(m.group(0))


# ---------------------------------------------------------------------------
# Step 3: Compile to bitcode
# ---------------------------------------------------------------------------

def _use_docker() -> bool:
    """Return True if we should use docker exec to compile and run KLEE."""
    return not Path(KLEE_BIN).exists()


def compile_to_bc(src_path: Path) -> Path:
    """Compile C source to KLEE-compatible bitcode. Returns .bc path on host."""
    bc = Path("/tmp") / (src_path.stem + "_exp.bc")

    if _use_docker():
        # Copy source into container, compile there with wllvm/clang-6
        container_src = f"/tmp/{src_path.name}"
        container_bc  = f"/tmp/{src_path.stem}_exp.bc"
        subprocess.check_call(
            ["sudo", "docker", "cp", str(src_path), f"{DOCKER_CONTAINER}:{container_src}"]
        )
        cmd = (
            f"{DOCKER_CLANG_ENV} wllvm "
            f"-I {DOCKER_KLEE_INC} "
            f"-emit-llvm -c -g -O1 "
            f"-Xclang -disable-llvm-passes "
            f"-o {container_bc} {container_src}"
        )
        result = subprocess.run(
            ["sudo", "docker", "exec", DOCKER_CONTAINER, "bash", "-c", cmd],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            raise RuntimeError(f"Docker compile failed:\n{result.stderr}")
        subprocess.check_call(
            ["sudo", "docker", "cp", f"{DOCKER_CONTAINER}:{container_bc}", str(bc)]
        )
    else:
        cmd = [
            CLANG, "-I", KLEE_INCLUDE,
            "-emit-llvm", "-c", "-g", "-O0",
            "-Xclang", "-disable-O0-optnone",
            "-o", str(bc), str(src_path),
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"Compile failed:\n{result.stderr}")
    return bc


# ---------------------------------------------------------------------------
# Step 4: Run KLEE with all searchers
# ---------------------------------------------------------------------------

def _parse_run_stats_text(text: str) -> dict:
    """Parse KLEE run.stats CSV text and return coverage metrics."""
    lines = [l for l in text.splitlines() if l.strip()]
    if len(lines) < 2:
        return {"covered_branches": 0, "total_queries": 0}

    header = [h.strip().strip("'") for h in lines[0].split(",")]
    values = [v.strip() for v in lines[-1].split(",")]
    row = dict(zip(header, values))

    def get(key: str, default: int = 0) -> int:
        try:
            return int(row.get(key, default))
        except (ValueError, TypeError):
            return default

    # KLEE 2.x uses CoveredBranches; 3.x uses CovI
    return {
        "covered_branches": max(get("CoveredBranches"), get("CovI")),
        "total_queries":    get("SolverQueries", 0),
        "completed_paths":  get("CompletedPaths", 0),
    }


def _parse_run_stats(stats_dir: Path) -> dict:
    """Read KLEE run.stats from a host directory."""
    stats_file = stats_dir / "run.stats"
    if not stats_file.exists():
        return {"covered_branches": 0, "total_queries": 0}
    return _parse_run_stats_text(stats_file.read_text())


def run_klee_searcher_docker(bc_path: Path, searcher: str, time_budget: int,
                              round_id: str) -> dict:
    """Run KLEE via docker exec. Parses run.stats from container."""
    safe = searcher.replace(":", "_").replace("-", "_")
    container_bc       = f"/tmp/{bc_path.stem}_exp.bc"
    container_out_dir  = f"/tmp/klee_{round_id}_{safe}"

    # Copy .bc to container (may already be there if compile_to_bc put it there)
    subprocess.run(
        ["sudo", "docker", "cp", str(bc_path), f"{DOCKER_CONTAINER}:{container_bc}"],
        capture_output=True,
    )

    cmd = (
        f"{DOCKER_KLEE} "
        f"--output-dir={container_out_dir} "
        f"--max-time={time_budget}s "
        f"--max-memory=2048 "
        f"--search={searcher} "
        f"--write-no-tests "
        f"{container_bc}"
    )
    result = subprocess.run(
        ["sudo", "docker", "exec", DOCKER_CONTAINER, "bash", "-c", cmd],
        capture_output=True, text=True,
        timeout=time_budget + 60,
    )

    # Read run.stats from container
    stats_result = subprocess.run(
        ["sudo", "docker", "exec", DOCKER_CONTAINER, "cat",
         f"{container_out_dir}/run.stats"],
        capture_output=True, text=True,
    )
    stats = _parse_run_stats_text(stats_result.stdout)
    stats["searcher"] = searcher
    stats["returncode"] = result.returncode
    return stats


def run_klee_searcher_local(bc_path: Path, searcher: str, time_budget: int,
                             work_dir: Path) -> dict:
    """Run KLEE locally. Returns stats dict."""
    output_dir = work_dir / f"klee_{searcher.replace(':', '_').replace('-', '_')}"
    output_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        KLEE_BIN,
        f"--output-dir={output_dir}",
        f"--max-time={time_budget}s",
        "--max-memory=2048",
        f"--search={searcher}",
        "--write-no-tests",
    ]
    if KLEE_UCLIBC:
        cmd += ["--libc=uclibc", f"--uclibc={KLEE_UCLIBC}"]
    cmd.append(str(bc_path))

    subprocess.run(cmd, capture_output=True, text=True, timeout=time_budget + 30)
    stats = _parse_run_stats(output_dir)
    stats["searcher"] = searcher
    return stats


def benchmark_all_searchers(bc_path: Path, time_budget: int,
                             round_id: str = "0") -> list[dict]:
    """Run all 6 KLEE searchers. Uses docker if KLEE not found locally."""
    results = []
    with tempfile.TemporaryDirectory(prefix="klee_exp_") as tmpdir:
        work_dir = Path(tmpdir)
        for searcher in SEARCHERS:
            print(f"  Running searcher: {searcher} ...", end=" ", flush=True)
            try:
                if _use_docker():
                    stats = run_klee_searcher_docker(bc_path, searcher,
                                                     time_budget, round_id)
                else:
                    stats = run_klee_searcher_local(bc_path, searcher,
                                                    time_budget, work_dir)
                print(f"coverage={stats.get('covered_branches', 0)}")
            except Exception as e:
                print(f"ERROR: {e}")
                stats = {"searcher": searcher, "covered_branches": 0,
                         "total_queries": 0, "error": str(e)}
            results.append(stats)

    results.sort(key=lambda r: -r.get("covered_branches", 0))
    return results


# ---------------------------------------------------------------------------
# Step 5 & 6: Reflect and propose rule update
# ---------------------------------------------------------------------------

REFLECT_PROMPT = """\
A KLEE searcher prediction was WRONG.

Program source:
```c
{source}
```

Structural profile from static analysis:
{features_json}

LLM prediction: {predicted} (reasoning: {reasoning})

Actual results (sorted by coverage):
{results_table}

The true winner was: {actual_winner}

Please:
1. Explain in 2–3 sentences WHY the prediction was wrong.
2. Propose a concrete, minimal update to the rules policy to fix this case.
   Format the rule update as a unified diff against `structural_rules.md`,
   OR describe the change in the form:
   CHANGE: [section name] [old text] → [new text]

Be conservative — only change what this specific evidence demands.
"""


def reflect_and_propose_update(
    source: str,
    features: rules_mod.ProgramFeatures,
    prediction: dict,
    results: list[dict],
) -> str:
    """Ask LLM to reflect on a wrong prediction and propose a rule update."""
    actual_winner = results[0]["searcher"] if results else "unknown"
    table_lines = [f"  {r['searcher']:18s}  coverage={r.get('covered_branches', 0):6d}"
                   f"  queries={r.get('total_queries', 0):8d}"
                   for r in results]
    table = "\n".join(table_lines)

    prompt = REFLECT_PROMPT.format(
        source=source,
        features_json=json.dumps(features.__dict__, indent=2),
        predicted=prediction["predicted_searcher"],
        reasoning=prediction.get("reasoning", ""),
        results_table=table,
        actual_winner=actual_winner,
    )
    return _call_llm(_system_with_rules(), prompt, max_tokens=2000)


# ---------------------------------------------------------------------------
# Step 7: Apply rule update (with human confirmation)
# ---------------------------------------------------------------------------

def apply_rule_update(proposal: str) -> bool:
    """Show the proposed rule update and ask for human confirmation."""
    print("\n" + "=" * 70)
    print("LLM PROPOSED RULE UPDATE:")
    print("=" * 70)
    print(proposal)
    print("=" * 70)
    answer = input("Apply this update to structural_rules.md? [y/N] ").strip().lower()
    if answer != "y":
        print("Update skipped.")
        return False

    # Ask LLM to emit the new full rules text
    apply_prompt = (
        "Based on the update proposal above and the current rules policy, "
        "output the COMPLETE updated structural_rules.md text. "
        "Make only the minimal changes described. "
        "Output ONLY the file content, no preamble."
    )
    new_text = _call_llm(_system_with_rules() + [
        {"type": "text", "text": f"Update proposal:\n{proposal}"}
    ], apply_prompt, max_tokens=8000)

    # Strip potential markdown code fences
    new_text = re.sub(r'^```[^\n]*\n', '', new_text, flags=re.MULTILINE)
    new_text = re.sub(r'\n```$', '', new_text, flags=re.MULTILINE)

    RULES_PATH.write_text(new_text.strip() + "\n")
    print(f"structural_rules.md updated ({len(new_text)} chars).")
    return True


# ---------------------------------------------------------------------------
# Main feedback loop
# ---------------------------------------------------------------------------

def run_experiment(
    source_bc: Path | None,
    rounds: int,
    time_budget: int,
    given_program: Path | None,
) -> None:
    seen_db = icfg_chunks.SeenDB(SEEN_DB_PATH)

    # Load GNU ICFG chunks for diversity-driven program generation
    gnu_functions: list[rules_mod.Function] = []
    if source_bc and source_bc.exists():
        print(f"Loading GNU bitcode from {source_bc} ...", flush=True)
        gnu_functions = icfg_chunks.load_functions(source_bc)
        print(f"  {len(gnu_functions)} functions loaded.")

    for round_num in range(1, rounds + 1):
        print(f"\n{'='*70}")
        print(f"ROUND {round_num}/{rounds}")
        print(f"{'='*70}")

        # ---- Choose or load the program ----
        if given_program:
            print(f"Using provided program: {given_program}")
            src_path = given_program
            source = src_path.read_text()
            chunk = None
        elif gnu_functions:
            # Pick a diverse chunk from real GNU code
            chunks = icfg_chunks.select_diverse_chunks(
                gnu_functions, str(source_bc), seen_db, top_n=3
            )
            if not chunks:
                print("No more unseen diverse chunks found.")
                break
            chunk = chunks[0]
            print(f"Selected chunk: {chunk['fn_name']}  "
                  f"rules={chunk['rules_fired']}  "
                  f"predicted={chunk['predicted_searcher']}")

            # ---- Generate C program ----
            print("Generating C program ...", flush=True)
            source = generate_program(chunk)
            # Strip any markdown fences the LLM might have added
            source = re.sub(r'^```[^\n]*\n', '', source, flags=re.MULTILINE)
            source = re.sub(r'\n```$', '', source, flags=re.MULTILINE)
            source = source.strip()

            # Write to temp file
            src_path = Path(f"/tmp/klee_exp_round{round_num}.c")
            src_path.write_text(source)
            print(f"  Written to {src_path} ({len(source)} chars)")
        else:
            print("No source bitcode and no given program. Stopping.")
            break

        # ---- Predict ----
        print("Predicting best searcher ...", flush=True)
        try:
            prediction = predict_searcher(source)
        except Exception as e:
            print(f"  Prediction failed: {e}")
            continue
        print(f"  Prediction: {prediction['predicted_searcher']}")
        print(f"  Rules fired: {prediction.get('rules_fired', [])}")
        print(f"  Reasoning: {prediction.get('reasoning', '')}")

        # ---- Compile ----
        print("Compiling ...", flush=True)
        try:
            bc_path = compile_to_bc(src_path)
        except RuntimeError as e:
            print(f"  Compile FAILED:\n{e}")
            # Save in seen_db as errored so we don't retry
            if chunk:
                profile = icfg_chunks.profile_function(
                    next(f for f in gnu_functions if f.name == chunk["fn_name"])
                )
                seen_db.add(chunk["fn_name"], str(source_bc), profile, actual="COMPILE_ERROR")
            continue

        # ---- Get static features (compile via host clang-14 for .ll) ----
        try:
            ll_path = Path("/tmp") / (src_path.stem + "_feat.ll")
            host_bc = Path("/tmp") / (src_path.stem + "_feat.bc")
            # Use host clang-14 just for feature extraction (LLVM IR is LLVM IR)
            subprocess.check_call(
                [CLANG, "-I", KLEE_INCLUDE,
                 "-emit-llvm", "-c", "-g", "-O0",
                 "-Xclang", "-disable-O0-optnone",
                 "-o", str(host_bc), str(src_path)],
                capture_output=True,
            )
            subprocess.check_call([LLVM_DIS, str(host_bc), "-o", str(ll_path)],
                                   capture_output=True)
            ll_functions = rules_mod.parse_ir(ll_path.read_text())
            features = rules_mod.compute_features(ll_functions)
        except Exception as e:
            print(f"  Feature extraction failed: {e}")
            features = rules_mod.ProgramFeatures()

        # ---- Run KLEE ----
        print(f"Running KLEE with {len(SEARCHERS)} searchers "
              f"({time_budget}s each) ...", flush=True)
        try:
            results = benchmark_all_searchers(bc_path, time_budget,
                                              round_id=f"r{round_num}")
        except Exception as e:
            print(f"  KLEE failed: {e}")
            continue

        # ---- Evaluate ----
        actual_winner = results[0]["searcher"] if results else "unknown"
        predicted = prediction["predicted_searcher"]
        correct = (actual_winner == predicted)

        print(f"\nResults (sorted by coverage):")
        for r in results:
            marker = " <-- ACTUAL WINNER" if r["searcher"] == actual_winner else ""
            pred_m = " <-- PREDICTED" if r["searcher"] == predicted else ""
            print(f"  {r['searcher']:18s}  "
                  f"coverage={r.get('covered_branches', 0):6d}  "
                  f"queries={r.get('total_queries', 0):8d}"
                  f"{pred_m}{marker}")

        print(f"\nPrediction: {predicted}  |  Actual: {actual_winner}  "
              f"|  {'CORRECT' if correct else 'WRONG'}")

        # ---- Update seen_db ----
        if chunk:
            fn = next((f for f in gnu_functions if f.name == chunk["fn_name"]), None)
            if fn:
                profile = icfg_chunks.profile_function(fn)
                seen_db.add(chunk["fn_name"], str(source_bc), profile, actual_winner)

        # ---- Reflect and update rules if wrong ----
        if not correct:
            print("\nReflecting on wrong prediction ...", flush=True)
            proposal = reflect_and_propose_update(source, features, prediction, results)
            apply_rule_update(proposal)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str]) -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source-bc", type=Path, default=GNU_BC_PATH,
                   help="GNU program .bc file for ICFG chunk extraction "
                        f"(default: {GNU_BC_PATH})")
    p.add_argument("--rounds", type=int, default=5,
                   help="Number of feedback loop iterations")
    p.add_argument("--time-budget", type=int, default=30,
                   help="Seconds per KLEE run (6 searchers × this)")
    p.add_argument("--program", type=Path, default=None,
                   help="Use a specific .c file instead of LLM-generated programs")
    p.add_argument("--no-generate", action="store_true",
                   help="Skip LLM program generation; requires --program")
    args = p.parse_args(argv)

    if args.no_generate and not args.program:
        p.error("--no-generate requires --program")

    if not KLEE_BIN or not Path(KLEE_BIN).exists():
        print(f"WARNING: KLEE not found at {KLEE_BIN}. "
              f"Set KLEE_BIN env var or build KLEE first.", file=sys.stderr)

    source_bc = args.source_bc if args.source_bc.exists() else None
    if not source_bc:
        print(f"WARNING: GNU bitcode not found at {args.source_bc}. "
              "Using LLM-only mode (no ICFG chunk diversity).", file=sys.stderr)

    run_experiment(
        source_bc=source_bc,
        rounds=args.rounds,
        time_budget=args.time_budget,
        given_program=args.program,
    )


if __name__ == "__main__":
    main(sys.argv[1:])
