#!/usr/bin/env python3
"""Per-function searcher planner ("anchor map") for KLEE.

For each function in the input program's bitcode, runs the static rules in
`rules.py` and classifies the function as either:

  * anchor      - a rule fires with a non-default verdict; the function
                  governs whichever searcher the rule recommends while a
                  state is executing inside it.
  * transparent - the rule selector falls through to the default fallback;
                  the function does not affect searcher choice.

Emits a JSON file consumable by ChunkGuidedSearcher in KLEE.

Output schema (version 1):
{
  "version": 1,
  "source": "<input path>",
  "default_searcher": ["--search=random-path", "--search=nurs:covnew"],
  "anchors": {
      "<func_name>": {
          "searcher": "dfs"|"bfs"|"random-path"|"nurs:qc"|"nurs:covnew"|...,
          "rule": "R9 (coverage-blind computation): ...",
          "blocks": <int>,
          "back_edges": <int>
      },
      ...
  },
  "transparent_count": <int>
}

Usage:
    chunk_planner.py <source.c|.bc|.ll> -o anchors.json
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import rules  # noqa: E402

CLANG = os.environ.get("CLANG", "/usr/bin/clang-14")
LLVM_DIS = os.environ.get("LLVM_DIS", "/usr/bin/llvm-dis-14")
KLEE_INCLUDE = os.environ.get("KLEE_INCLUDE", "/home/cc/klee/include")
SVF_EXTRACT = os.environ.get(
    "SVF_EXTRACT", str(HERE / "svf_features" / "build" / "extract_features"))


def _compile_to_bc(src: Path) -> Path:
    if src.suffix == ".bc":
        return src
    if src.suffix == ".ll":
        bc = Path("/tmp") / (src.stem + ".bc")
        subprocess.check_call(["/usr/bin/llvm-as-14", str(src), "-o", str(bc)])
        return bc
    if src.suffix == ".c":
        bc = Path("/tmp") / (src.stem + ".bc")
        subprocess.check_call([
            CLANG, "-I", KLEE_INCLUDE,
            "-emit-llvm", "-c", "-g", "-O0",
            "-Xclang", "-disable-O0-optnone",
            "-o", str(bc), str(src),
        ])
        return bc
    raise SystemExit(f"unsupported input extension: {src.suffix}")


def plan_svf(src_path: Path) -> dict:
    """Per-function planner using the SVF C++ extractor.

    Runs `extract_features` once with SVF_PER_FUNCTION=1; for each function
    in the resulting `per_function` map, builds a ProgramFeatures and
    applies `rules.select_searcher`. Same output schema as `plan()`.
    """
    if not Path(SVF_EXTRACT).exists():
        raise SystemExit(
            f"SVF extractor not built: {SVF_EXTRACT}\n"
            f"       run searcher_selector/svf_features/build.sh")
    bc = _compile_to_bc(src_path)
    env = dict(os.environ, SVF_PER_FUNCTION="1")
    out = subprocess.run(
        [SVF_EXTRACT, "-stat=false", str(bc)],
        check=True, capture_output=True, text=True, env=env,
    )
    data = json.loads(out.stdout)
    per_fn = data.get("per_function", {})
    # Whole-program globals to fold into each per-fn feature set.
    n_total_funcs = int(data.get("total_functions", len(per_fn)))
    has_klee_sym = bool(data.get("has_klee_symbolic", False))

    anchors: dict[str, dict] = {}
    transparent = 0
    by_kind: dict[str, int] = {}

    for fname, fdata in per_fn.items():
        merged = dict(fdata)
        merged.setdefault("total_functions", n_total_funcs)
        merged.setdefault("has_klee_symbolic", has_klee_sym)
        merged["dominant_function"] = fname
        feats = rules.features_from_json(merged)
        flags, reason = rules.select_searcher(feats)
        if is_default(flags):
            transparent += 1
            continue
        kind = flags_to_kind(flags)
        anchors[fname] = {
            "searcher": kind,
            "rule": reason,
            "blocks": int(fdata.get("total_blocks", 0)),
            "back_edges": int(fdata.get("nested_loop_count", 0)),
        }
        by_kind[kind] = by_kind.get(kind, 0) + 1

    return {
        "version": 1,
        "source": str(src_path),
        "backend": "svf",
        "default_searcher": _DEFAULT_FLAGS,
        "anchors": anchors,
        "transparent_count": transparent,
        "summary_by_kind": by_kind,
    }


def compile_to_ll(src: Path) -> Path:
    if src.suffix == ".ll":
        return src
    if src.suffix == ".bc":
        ll = Path("/tmp") / (src.stem + ".ll")
        subprocess.check_call([LLVM_DIS, str(src), "-o", str(ll)])
        return ll
    if src.suffix == ".c":
        bc = Path("/tmp") / (src.stem + ".bc")
        ll = Path("/tmp") / (src.stem + ".ll")
        subprocess.check_call([
            CLANG, "-I", KLEE_INCLUDE,
            "-emit-llvm", "-c", "-g", "-O0",
            "-Xclang", "-disable-O0-optnone",
            "-o", str(bc), str(src),
        ])
        subprocess.check_call([LLVM_DIS, str(bc), "-o", str(ll)])
        return ll
    raise SystemExit(f"unsupported input extension: {src.suffix}")


def features_for_function(fn: rules.Function,
                          all_functions: list[rules.Function]) -> rules.ProgramFeatures:
    """ProgramFeatures scoped to a single function.

    Uses the same per-function probes from rules.py but evaluated only on
    `fn` instead of aggregating across the whole module. Recursion detection
    needs the whole call graph to know what `fn` may call back into.
    """
    f = rules.ProgramFeatures()
    f.total_functions = len(all_functions)
    f.total_blocks = len(fn.blocks)
    f.dominant_function = fn.name

    f.total_branches = sum(
        1 for bb in fn.blocks.values()
        for l in bb.lines if rules._BR_COND.search(l)
    )
    f.has_klee_symbolic = any(
        rules._KLEE_SYM.search(l)
        for bb in fn.blocks.values() for l in bb.lines
    )

    f.bit_test_chain_len       = rules._max_bit_test_chain(fn)
    f.independent_call_regions = rules._count_independent_call_regions(fn)
    f.has_bitwise_branch_conds = rules._has_bitwise_branch_cond(fn)
    f.nested_loop_count        = len(fn.back_edges)
    f.symbolic_loop_bound      = rules._symbolic_loop_bound(fn)

    # Direct recursion: does `fn` call itself? (Mutual recursion is not
    # captured here — it would require reachability analysis on the call
    # graph; for anchor decisions, direct-self is a strong enough signal.)
    f.has_direct_recursion = fn.name in fn.calls

    f.coverage_blind_score = rules._coverage_blind_score(fn)
    f.srem_in_branch       = rules._srem_in_branch(fn)
    f.cheap_branch_count   = rules._cheap_branch_count(fn)
    f.gep_chain_depth      = rules._gep_chain_depth(fn)
    f.convergent_diverg    = rules._convergent_divergent(fn)
    return f


_DEFAULT_FLAGS = ["--search=random-path", "--search=nurs:covnew"]


def is_default(flags: list[str]) -> bool:
    return flags == _DEFAULT_FLAGS


def flags_to_kind(flags: list[str]) -> str:
    """Reduce KLEE --search flags to a single canonical kind name.

    The runtime side only needs one kind per anchor; if the rule emitted
    a multi-flag interleave (only the default does that today), pick the
    first concrete searcher.
    """
    for f in flags:
        if f.startswith("--search="):
            return f.split("=", 1)[1]
    raise ValueError(f"no --search in flags: {flags}")


def plan(src_path: Path) -> dict:
    ll_path = compile_to_ll(src_path)
    text = ll_path.read_text()
    functions = rules.parse_ir(text)

    anchors: dict[str, dict] = {}
    transparent = 0
    by_kind: dict[str, int] = {}

    for fn in functions:
        if not fn.blocks:
            continue
        feats = features_for_function(fn, functions)
        flags, reason = rules.select_searcher(feats)
        if is_default(flags):
            transparent += 1
            continue
        kind = flags_to_kind(flags)
        anchors[fn.name] = {
            "searcher": kind,
            "rule": reason,
            "blocks": len(fn.blocks),
            "back_edges": len(fn.back_edges),
        }
        by_kind[kind] = by_kind.get(kind, 0) + 1

    return {
        "version": 1,
        "source": str(src_path),
        "default_searcher": _DEFAULT_FLAGS,
        "anchors": anchors,
        "transparent_count": transparent,
        "summary_by_kind": by_kind,
    }


def main(argv: list[str]) -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path, help=".c / .bc / .ll input file")
    p.add_argument("-o", "--output", type=Path, default=None,
                   help="output JSON path (default: <source>.anchors.json)")
    p.add_argument("--print", action="store_true",
                   help="also print a human-readable summary to stdout")
    p.add_argument("--backend", choices=["svf", "regex"], default="svf",
                   help="feature extraction backend (default: svf)")
    args = p.parse_args(argv)

    if not args.source.exists():
        print(f"error: file not found: {args.source}", file=sys.stderr)
        sys.exit(1)

    result = plan_svf(args.source) if args.backend == "svf" else plan(args.source)

    out_path = args.output
    if out_path is None:
        stem = args.source.stem
        out_path = args.source.with_name(f"{stem}.anchors.json")
    out_path.write_text(json.dumps(result, indent=2))

    if args.print:
        print(f"=== Anchor plan: {args.source.name} -> {out_path} ===")
        print(f"  anchor functions:    {len(result['anchors'])}")
        print(f"  transparent funcs:   {result['transparent_count']}")
        print(f"  by searcher kind:")
        for kind, n in sorted(result["summary_by_kind"].items(),
                              key=lambda kv: -kv[1]):
            print(f"    {kind:20s} {n}")
        print()
        print("  Top 10 anchors by block count:")
        ranked = sorted(result["anchors"].items(),
                        key=lambda kv: -kv[1]["blocks"])[:10]
        for name, info in ranked:
            print(f"    {info['searcher']:18s} {info['blocks']:5d} blocks  "
                  f"{name}")
    else:
        print(out_path)


if __name__ == "__main__":
    main(sys.argv[1:])
