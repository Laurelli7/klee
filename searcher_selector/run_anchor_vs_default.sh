#!/usr/bin/env bash
# End-to-end comparison on gnumake:
#   (A) anchor      : --search=anchor with the SVF per-function anchor map
#   (B) default     : KLEE default (random-path interleaved with nurs:covnew)
#
# Same time budget, same sym-args, same solver. Reports coverage stats from
# klee-stats so the only varying factor is the searcher.

set -euo pipefail

KLEE=${KLEE:-/home/cc/klee/build/bin/klee}
KLEE_STATS=${KLEE_STATS:-/home/cc/klee/build/bin/klee-stats}
PLANNER=${PLANNER:-/home/cc/klee/searcher_selector/chunk_planner.py}
BC=${BC:-/home/cc/klee/examples/gnumake/install/bin/make.bc}
TIME=${TIME:-180}
SYM_ARGS=${SYM_ARGS:-"3 3 6"}
# Concrete + symbolic args appended after the bitcode (overrides --sym-args).
# Set POSIX_ARGS to anything non-empty to disable --sym-args mode.
POSIX_ARGS=${POSIX_ARGS:-""}
OUT_DIR=${OUT_DIR:-/tmp/klee-anchor-exp}
ANCHORS=${ANCHORS:-/tmp/make.anchors.svf.json}

mkdir -p "$OUT_DIR"
rm -rf "$OUT_DIR/anchor" "$OUT_DIR/default"

echo "=== [1/3] generating SVF per-function anchor map ==="
python3 "$PLANNER" "$BC" -o "$ANCHORS" --backend svf --print | tail -20

run_klee() {
  local label=$1
  shift
  local outdir="$OUT_DIR/$label"
  echo
  echo "=== [run] $label ==="
  echo "  out:  $outdir"
  echo "  args: $*"
  "$KLEE" \
    --output-dir="$outdir" \
    --max-time="${TIME}" \
    --libc=uclibc \
    --posix-runtime \
    --external-calls=all \
    --only-output-states-covering-new \
    --use-forked-solver \
    --solver-backend=z3 \
    --max-solver-time=30 \
    --watchdog \
    "$@" \
    "$BC" \
    ${POSIX_ARGS:---sym-args $SYM_ARGS} \
    > "$outdir.stdout" 2> "$outdir.stderr" || true
  echo "  -- stats --"
  "$KLEE_STATS" --print-all "$outdir" 2>/dev/null | tail -5 || true
}

echo
echo "=== [2/3] anchor (SVF per-function) ==="
run_klee anchor --search=anchor --anchor-map="$ANCHORS"

echo
echo "=== [3/3] default ==="
run_klee default

echo
echo "=== summary ==="
for label in anchor default; do
  echo
  echo "--- $label ---"
  "$KLEE_STATS" --print-all "$OUT_DIR/$label" 2>/dev/null | tail -3 || true
done
