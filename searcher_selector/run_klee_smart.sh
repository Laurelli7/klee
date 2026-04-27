#!/usr/bin/env bash
# run_klee_smart.sh <source.c> [time_limit_s] [--compare]
#
# Compiles a C source to LLVM bitcode, runs static analysis to pick a
# KLEE searcher, then runs KLEE. With --compare, also runs KLEE with
# the default searcher and prints a side-by-side coverage table.
set -euo pipefail

SRC="${1:-}"
TIME="${2:-60}"
COMPARE="${3:-}"

if [[ -z "${SRC}" ]]; then
    echo "usage: $0 <source.c> [time_limit_s] [--compare]" >&2
    exit 1
fi

if [[ ! -f "${SRC}" ]]; then
    echo "error: source not found: ${SRC}" >&2
    exit 1
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KLEE_ROOT="$(cd "${HERE}/.." && pwd)"
CLANG="${CLANG:-/usr/bin/clang-14}"
KLEE="${KLEE:-${KLEE_ROOT}/build/bin/klee}"
STEM="$(basename "${SRC}" .c)"
BC="/tmp/${STEM}.bc"
OUT_SMART="/tmp/klee_${STEM}_smart"
OUT_DEFAULT="/tmp/klee_${STEM}_default"

echo "=== [1/4] Compiling ${SRC} → LLVM bitcode ==="
"${CLANG}" -I "${KLEE_ROOT}/include" -emit-llvm -c -g -O0 \
    -Xclang -disable-O0-optnone -o "${BC}" "${SRC}"
echo "    -> ${BC}"
echo

echo "=== [2/4] Static analysis ==="
python3 "${HERE}/analyze.py" "${SRC}"
FLAGS="$(python3 "${HERE}/analyze.py" "${SRC}" --flags-only)"
echo

run_klee() {
    local tag="$1" outdir="$2" flags="$3"
    rm -rf "${outdir}"
    echo "--- Running KLEE (${tag}): ${flags} ---"
    "${KLEE}" \
        --max-time="${TIME}" \
        --output-dir="${outdir}" \
        ${flags} \
        "${BC}" 2>&1 | tail -8 || true
    echo
}

extract_coverage() {
    local outdir="$1"
    if [[ ! -f "${outdir}/info" ]]; then
        echo "n/a"; return
    fi
    grep -E "^KLEE: done: (completed paths|total instructions|covered instructions|explored paths|generated tests)" \
        "${outdir}/info" | head -5 || true
}

echo "=== [3/4] KLEE with RECOMMENDED searcher ==="
run_klee "smart" "${OUT_SMART}" "${FLAGS}"

if [[ "${COMPARE}" == "--compare" ]]; then
    echo "=== [4/4] KLEE with DEFAULT searcher (for comparison) ==="
    DEFAULT_FLAGS="--search=random-path --search=nurs:covnew"
    run_klee "default" "${OUT_DEFAULT}" "${DEFAULT_FLAGS}"

    echo "=== Coverage comparison ==="
    printf "  %-10s  %s\n" "smart"   "$(extract_coverage "${OUT_SMART}"  | tr '\n' ' ')"
    printf "  %-10s  %s\n" "default" "$(extract_coverage "${OUT_DEFAULT}" | tr '\n' ' ')"
else
    echo "=== [4/4] (skip comparison; pass --compare as 3rd arg to enable) ==="
fi
