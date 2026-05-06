#!/usr/bin/env bash
# bc: LLM-authored per-function map vs KLEE default
set -euo pipefail
KLEE=${KLEE:-/home/cc/klee/build/bin/klee}
KSTATS=${KSTATS:-/home/cc/klee/build/bin/klee-stats}
BC=${BC:-/home/cc/klee/examples/bc/install/bin/bc.bc}
MAP=${MAP:-/home/cc/klee/examples/bc/install/bin/bc.llm.anchors.json}
TIME=${TIME:-180}
OUT=${OUT:-/tmp/klee-bc-llm-exp}
mkdir -p "$OUT"; rm -rf "$OUT/llm" "$OUT/default"

run() {
  local label=$1; shift
  local outdir="$OUT/$label"
  echo "=== $label ==="
  "$KLEE" --output-dir="$outdir" --max-time="$TIME" \
    --libc=uclibc --posix-runtime --external-calls=all \
    --only-output-states-covering-new \
    --use-forked-solver --solver-backend=z3 \
    --max-solver-time=30 --watchdog \
    "$@" "$BC" --sym-stdin 64 --sym-arg 4 \
    > "$outdir.stdout" 2> "$outdir.stderr" || true
}

run llm     --search=LLMProgramsearch --llm-anchor-map="$MAP"
run default

echo
"$KSTATS" "$OUT/llm" "$OUT/default"
echo
for label in llm default; do
  echo "== $label =="
  grep -E "^KLEE: done" "$OUT/$label.stderr"
done
