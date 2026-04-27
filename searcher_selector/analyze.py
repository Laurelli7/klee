#!/usr/bin/env python3
"""Static analysis tool for KLEE searcher selection.

Usage:
    analyze.py <source.c|program.bc|program.ll>  [--json]

Produces a KLEE --search flag recommendation derived from structural features
of the input program's LLVM IR. The 12 rules implemented come from
klee_export/analysis/searcher_selection_rules.md.
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


def _die(msg: str, code: int = 1):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(code)


def compile_to_ll(src: Path) -> Path:
    """Compile C/bitcode/IR to LLVM IR text (.ll). Returns path to .ll file."""
    if src.suffix == ".ll":
        return src
    if src.suffix == ".bc":
        ll = src.with_suffix(".ll")
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
    _die(f"unsupported input extension: {src.suffix}")


def analyze(src_path: Path) -> tuple[rules.ProgramFeatures, list[str], str]:
    ll_path = compile_to_ll(src_path)
    text = ll_path.read_text()
    functions = rules.parse_ir(text)
    features = rules.compute_features(functions)
    flags, explanation = rules.select_searcher(features)
    return features, flags, explanation


def print_human(src: Path, feats: rules.ProgramFeatures, flags: list[str], reason: str):
    print(f"=== KLEE Searcher Analysis: {src.name} ===")
    print("Features:")
    for line in feats.summary_lines():
        print(line)
    print()
    print(f"Rule triggered: {reason}")
    print(f"Recommended:    {' '.join(flags)}")


def main(argv: list[str]):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path, help="C/.bc/.ll input file")
    p.add_argument("--json", action="store_true", help="Emit JSON instead of human-readable")
    p.add_argument("--flags-only", action="store_true",
                   help="Print only the recommended KLEE --search flags (for scripts)")
    args = p.parse_args(argv)

    if not args.source.exists():
        _die(f"file not found: {args.source}")

    feats, flags, reason = analyze(args.source)

    if args.flags_only:
        print(" ".join(flags))
        return

    if args.json:
        print(json.dumps({
            "source": str(args.source),
            "features": feats.__dict__,
            "recommended_flags": flags,
            "explanation": reason,
        }, indent=2))
        return

    print_human(args.source, feats, flags, reason)


if __name__ == "__main__":
    main(sys.argv[1:])
