#!/usr/bin/env python3
"""ICFG chunk diversity engine for KLEE searcher selection experiments.

Extracts function-level structural profiles from real GNU program bitcode,
tracks which (structural_pattern × actual_winner) pairs have been tested,
and surfaces functions that expose structural patterns not yet covered.

Usage:
    icfg_chunks.py <program.bc|program.ll> [--top N] [--dump-all] [--seen-db FILE]
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import rules

LLVM_DIS = os.environ.get("LLVM_DIS", "/usr/bin/llvm-dis-14")

# ---------------------------------------------------------------------------
# Structural fingerprint
# ---------------------------------------------------------------------------

class ChunkProfile(NamedTuple):
    """Discrete structural fingerprint of a single function."""
    # Pattern buckets — each maps to a structural rule
    coverage_blind: bool      # R9  — arithmetic-only loop blocks ≥ 2
    has_back_edges: bool      # R9/R7/R10 — any loops at all
    symbolic_loop_bound: bool # R7  — loop header compares two %-regs
    has_recursion: bool       # R8  — direct self-call
    wide_dispatch: bool       # R4  — ≥ 4 distinct callees
    multi_dispatch: bool      # R4b/c — ≥ 6 distinct callees
    bitwise_flags: bool       # R4a — bitwise branch conditions
    bit_test_barrier: bool    # R2  — ≥ 8 consecutive bit-tests
    convergent: bool          # R6  — merge-then-re-branch blocks ≥ 2
    srem_branch: bool         # R11 — modular arith in branch
    cost_asymmetry: bool      # R11 — srem AND cheap branches ≥ 2
    pointer_chain: bool       # R12 — GEP chain depth ≥ 2
    # Scale (bucketed so fingerprint is discrete)
    scale: str                # "tiny" <20, "small" <100, "medium" <500, "large" ≥500

    def as_dict(self) -> dict:
        return self._asdict()

    @property
    def predicted_searcher(self) -> str:
        """Apply priority rules to predict the best searcher."""
        if self.coverage_blind and self.has_back_edges:
            return "dfs"
        if self.bit_test_barrier:
            return "dfs"
        if self.wide_dispatch:
            if self.bitwise_flags and not self.has_back_edges:
                return "nurs:covnew"
            if self.multi_dispatch:
                return "random-path"
            if self.has_back_edges:
                return "random-path"
            return "nurs:covnew"
        if self.srem_branch and self.cost_asymmetry:
            return "nurs:qc"
        if self.has_recursion:
            return "bfs"
        if self.symbolic_loop_bound and not self.coverage_blind:
            return "bfs"
        if self.convergent:
            return "nurs:covnew"
        if self.pointer_chain:
            return "random-path"
        return "default"

    def interesting_rules(self) -> list[str]:
        """Which rules fire for this profile."""
        fired = []
        if self.coverage_blind and self.has_back_edges:
            fired.append("R9")
        if self.bit_test_barrier:
            fired.append("R2")
        if self.wide_dispatch:
            if self.bitwise_flags:
                fired.append("R4a")
            if self.multi_dispatch:
                fired.append("R4b")
            if self.has_back_edges:
                fired.append("R4c")
            fired.append("R4")
        if self.srem_branch and self.cost_asymmetry:
            fired.append("R11")
        if self.has_recursion:
            fired.append("R8")
        if self.symbolic_loop_bound and not self.coverage_blind:
            fired.append("R7")
        if self.convergent:
            fired.append("R6")
        if self.pointer_chain:
            fired.append("R12")
        return fired or ["none"]


def _scale(blocks: int) -> str:
    if blocks < 20:
        return "tiny"
    if blocks < 100:
        return "small"
    if blocks < 500:
        return "medium"
    return "large"


def profile_function(fn: rules.Function) -> ChunkProfile:
    """Compute a ChunkProfile for a single parsed Function."""
    feats = rules.ProgramFeatures()
    feats.total_functions = 1
    feats.total_blocks = len(fn.blocks)

    feats.bit_test_chain_len       = rules._max_bit_test_chain(fn)
    feats.independent_call_regions = rules._count_independent_call_regions(fn)
    feats.has_bitwise_branch_conds = rules._has_bitwise_branch_cond(fn)
    feats.nested_loop_count        = len(fn.back_edges)
    feats.symbolic_loop_bound      = rules._symbolic_loop_bound(fn)
    feats.has_direct_recursion     = fn.name in fn.calls
    feats.coverage_blind_score     = rules._coverage_blind_score(fn)
    feats.srem_in_branch           = rules._srem_in_branch(fn)
    feats.cheap_branch_count       = rules._cheap_branch_count(fn)
    feats.gep_chain_depth          = rules._gep_chain_depth(fn)
    feats.convergent_diverg        = rules._convergent_divergent(fn)

    return ChunkProfile(
        coverage_blind     = feats.coverage_blind_score >= 2,
        has_back_edges     = feats.nested_loop_count >= 1,
        symbolic_loop_bound= feats.symbolic_loop_bound,
        has_recursion      = feats.has_direct_recursion,
        wide_dispatch      = feats.independent_call_regions >= 4,
        multi_dispatch     = feats.independent_call_regions >= 6,
        bitwise_flags      = feats.has_bitwise_branch_conds,
        bit_test_barrier   = feats.bit_test_chain_len >= 8,
        convergent         = feats.convergent_diverg >= 2,
        srem_branch        = feats.srem_in_branch,
        cost_asymmetry     = (feats.srem_in_branch and feats.cheap_branch_count >= 2),
        pointer_chain      = feats.gep_chain_depth >= 2,
        scale              = _scale(len(fn.blocks)),
    )


# ---------------------------------------------------------------------------
# Coverage tracking
# ---------------------------------------------------------------------------

class SeenDB:
    """Tracks which (fingerprint, predicted_searcher) pairs have been tested.

    File format: JSON list of {"fingerprint": {...}, "predicted": str,
    "actual": str|null, "fn_name": str, "source": str}
    """

    def __init__(self, path: Path):
        self.path = path
        self.records: list[dict] = []
        if path.exists():
            self.records = json.loads(path.read_text())

    def already_seen(self, profile: ChunkProfile) -> bool:
        fp = profile.as_dict()
        return any(r["fingerprint"] == fp for r in self.records)

    def add(self, fn_name: str, source: str, profile: ChunkProfile,
            actual: str | None = None):
        self.records.append({
            "fn_name": fn_name,
            "source": source,
            "fingerprint": profile.as_dict(),
            "predicted": profile.predicted_searcher,
            "actual": actual,
        })
        self.path.write_text(json.dumps(self.records, indent=2))

    def pattern_coverage(self) -> dict[str, int]:
        """How many times each rule has been tested."""
        counts: dict[str, int] = {}
        for r in self.records:
            fp = ChunkProfile(**r["fingerprint"])
            for rule in fp.interesting_rules():
                counts[rule] = counts.get(rule, 0) + 1
        return counts


# ---------------------------------------------------------------------------
# Bitcode loading
# ---------------------------------------------------------------------------

def load_functions(src: Path) -> list[rules.Function]:
    if src.suffix == ".ll":
        text = src.read_text()
    elif src.suffix == ".bc":
        ll_path = Path("/tmp") / (src.stem + "_chunks.ll")
        subprocess.check_call([LLVM_DIS, str(src), "-o", str(ll_path)])
        text = ll_path.read_text()
    else:
        raise SystemExit(f"unsupported: {src.suffix}")
    return rules.parse_ir(text)


# ---------------------------------------------------------------------------
# Chunk selection
# ---------------------------------------------------------------------------

# All 12 rules we care about, in priority order
ALL_RULES = ["R9", "R2", "R1", "R3", "R4a", "R4b", "R4c", "R4",
             "R10", "R8", "R7", "R11", "R6", "R12"]

# Rules that gnumake cannot supply (they need R-targeted generation)
GNUMAKE_BLIND_SPOTS = {"R9", "R2", "R1", "R3", "R10"}

# Canonical searcher for each rule (used when no ICFG chunk exists for that rule)
RULE_SEARCHER = {
    "R9": "dfs", "R2": "dfs", "R1": "dfs", "R3": "dfs",
    "R4": "nurs:covnew", "R4a": "nurs:covnew",
    "R4b": "random-path", "R4c": "random-path",
    "R8": "bfs", "R7": "bfs", "R10": "dfs",
    "R11": "nurs:qc", "R6": "nurs:covnew", "R12": "random-path",
}


def least_tested_rule(seen_db: SeenDB) -> str:
    """Return the rule with the fewest tests in the seen_db."""
    coverage = seen_db.pattern_coverage()
    return min(ALL_RULES, key=lambda r: coverage.get(r, 0))


def best_chunk_for_rule(
    functions: list[rules.Function],
    target_rule: str,
    source_name: str,
    seen_db: SeenDB,
    min_blocks: int = 8,
) -> dict | None:
    """Find the best function that fires target_rule (may also fire others).

    Returns None if no function in this source fires target_rule.
    """
    candidates = []
    for fn in functions:
        if len(fn.blocks) < min_blocks:
            continue
        profile = profile_function(fn)
        fired = profile.interesting_rules()
        if target_rule not in fired:
            continue
        # Prefer functions where target_rule is the ONLY or dominant rule
        specificity = 1.0 / len(fired)   # higher = more focused on target_rule
        # Prefer medium-sized functions
        size_score = 1.0 if profile.scale == "medium" else (0.6 if profile.scale == "small" else 0.3)
        candidates.append((specificity + size_score, fn.name, len(fn.blocks), len(fn.back_edges), profile, fired))

    if not candidates:
        return None
    candidates.sort(key=lambda t: -t[0])
    _, fn_name, blocks, back_edges, profile, fired = candidates[0]
    return {
        "fn_name": fn_name,
        "source": source_name,
        "blocks": blocks,
        "back_edges": back_edges,
        "profile": profile.as_dict(),
        "rules_fired": fired,
        "predicted_searcher": profile.predicted_searcher,
        "target_rule": target_rule,
    }


def synthetic_chunk_for_rule(target_rule: str) -> dict:
    """When no real function exhibits target_rule, return a synthetic description."""
    descriptions = {
        "R9": "coverage-blind accumulator: a loop body where every path runs the same instructions (XOR/shift/AND) but accumulates a different value. No symbolic branches inside the loop — all branching only at the final check on the accumulated value.",
        "R2": "useless fork barrier: many symbolic branches (bit-tests or byte comparisons) that produce NO new code coverage sit BEFORE the useful target code. All branches converge before reaching the interesting functions.",
        "R1": "sequential gating chain: target code is reachable ONLY by passing N>=15 sequential symbolic checks in series. Each check is a fork; the 'fail' edge goes to a dead end. You must take the 'pass' branch at every fork.",
        "R3": "symbolic setup then concrete sweep: 1-3 symbolic variables are read at the start. The rest of the program is a large concrete loop (100K+ iterations) whose behavior depends on the symbolic choice. The loop body is concrete (no symbolic branches).",
        "R10": "identical loop body with different memory targets: a loop body applies the SAME transformation every iteration (same instructions, same branch structure) but writes to different array slots. After the first iteration, all loop body branches are 'already covered' from a coverage-heuristic perspective.",
        "R7": "symbolic loop bound: a symbolic variable n controls how many times a loop executes. Different values of n cover different instructions (e.g., the base case vs deep iterations). The loop body changes meaningfully with n.",
        "R8": "recursive combinatorial explosion: a recursive function makes K>=2 recursive calls per invocation. Different branches cover different code at the base case.",
        "R11": "constraint cost asymmetry: some branches use expensive modular arithmetic (x%p==k) while other equally-novel branches use cheap equality (x==c). Both types lead to uncovered code.",
        "R6": "convergent-divergent flow: multiple code paths merge at a hub node, then branch again. All states at the merge point have identical coverage, so coverage heuristics lose their signal temporarily.",
        "R4a": "bitwise flag decoding: a non-loop function tests individual bits of a symbolic bitfield with independent AND+compare branches (flags&0x01, flags&0x02, etc.). Each bit-test is an independent coverage target.",
        "R4b": "multi-dimensional dispatch: two or more independent dispatch points (e.g., two separate switches on different symbolic variables). NURS greedily optimizes one dimension and misses the other.",
        "R4c": "nested scanner: outer loop over positions, inner loop over token types. Both loops branch on symbolic data creating a 2D search space.",
        "R4": "wide independent regions: 4+ independent code regions at the top level, each reached by a different branch arm, each covering different functions/instructions.",
        "R12": "symbolic pointer/index chain: x = a[x] pattern where the loaded value is used as the next index. Chain of 2+ dependent symbolic loads.",
    }
    return {
        "fn_name": f"<synthetic:{target_rule}>",
        "source": "synthetic",
        "blocks": 0,
        "back_edges": 0,
        "profile": {},
        "rules_fired": [target_rule],
        "predicted_searcher": RULE_SEARCHER.get(target_rule, "nurs:covnew"),
        "target_rule": target_rule,
        "description": descriptions.get(target_rule, ""),
    }


def select_diverse_chunks(
    functions: list[rules.Function],
    source_name: str,
    seen_db: SeenDB,
    top_n: int = 10,
    min_blocks: int = 5,
) -> list[dict]:
    """Return up to top_n chunks targeting the least-tested rules.

    For each rule (in least-tested order), find the best matching function.
    If gnumake has no function for a rule (its blind spots), return a synthetic
    description that the LLM will use to generate the program from scratch.
    """
    coverage = seen_db.pattern_coverage()
    rules_by_rarity = sorted(ALL_RULES, key=lambda r: coverage.get(r, 0))

    results = []
    seen_rules_this_batch: set[str] = set()

    for target_rule in rules_by_rarity:
        if len(results) >= top_n:
            break
        if target_rule in seen_rules_this_batch:
            continue

        chunk = best_chunk_for_rule(functions, target_rule, source_name,
                                    seen_db, min_blocks)
        if chunk is None:
            # gnumake has nothing for this rule → use synthetic description
            chunk = synthetic_chunk_for_rule(target_rule)

        results.append(chunk)
        seen_rules_this_batch.add(target_rule)

    return results


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str]) -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path, help=".bc or .ll program file")
    p.add_argument("--top", type=int, default=10, help="Number of chunks to return")
    p.add_argument("--dump-all", action="store_true",
                   help="Print all functions with their profiles (for inspection)")
    p.add_argument("--seen-db", type=Path, default=Path("seen_chunks.json"),
                   help="Path to JSON tracking already-tested patterns")
    p.add_argument("--min-blocks", type=int, default=5,
                   help="Ignore functions smaller than this")
    args = p.parse_args(argv)

    if not args.source.exists():
        print(f"error: {args.source} not found", file=sys.stderr)
        sys.exit(1)

    print(f"Loading {args.source}...", file=sys.stderr)
    functions = load_functions(args.source)
    print(f"  parsed {len(functions)} functions", file=sys.stderr)

    seen_db = SeenDB(args.seen_db)

    if args.dump_all:
        print(f"\n{'Function':40s}  {'Blocks':>6}  {'Rules':30s}  {'Predicted':14s}")
        print("-" * 100)
        rows = []
        for fn in functions:
            if len(fn.blocks) < args.min_blocks:
                continue
            profile = profile_function(fn)
            fired = profile.interesting_rules()
            rows.append((len(fn.blocks), fn.name, fired, profile.predicted_searcher))
        rows.sort(key=lambda r: -r[0])
        for blocks, name, fired, pred in rows:
            print(f"{name:40s}  {blocks:6d}  {','.join(fired):30s}  {pred}")
        return

    chunks = select_diverse_chunks(
        functions, str(args.source), seen_db,
        top_n=args.top, min_blocks=args.min_blocks,
    )

    print(f"\n=== Top {len(chunks)} diverse chunks from {args.source.name} ===")
    print(f"Pattern coverage so far: {seen_db.pattern_coverage()}")
    print()
    for i, chunk in enumerate(chunks, 1):
        print(f"  [{i}] {chunk['fn_name']}")
        print(f"       blocks={chunk['blocks']}  back_edges={chunk['back_edges']}")
        print(f"       rules: {chunk['rules_fired']}")
        print(f"       predicted: {chunk['predicted_searcher']}")
        print()

    print(json.dumps(chunks, indent=2))


if __name__ == "__main__":
    main(sys.argv[1:])
