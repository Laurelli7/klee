#!/usr/bin/env bash
# End-to-end demo: static analysis guides KLEE searcher choice on exp40.
#
# exp40_priority_inversion.c has 2^24 noise branches ("sea" of bit-tests)
# preceding 10 unique "island" functions. Default KLEE gets stuck exploring
# noise; DFS commits to one noise path and reaches all islands.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KLEE_ROOT="$(cd "${HERE}/.." && pwd)"
REPO_ROOT="$(cd "${KLEE_ROOT}/.." && pwd)"
CLANG=/usr/bin/clang-14
KLEE="${REPO_ROOT}/build/bin/klee"
KLEE_STATS="${REPO_ROOT}/build/bin/klee-stats"

PROG="${1:-${HERE}/programs/exp40_priority_inversion.c}"
TIME="${2:-8}"
STEM="$(basename "${PROG}" .c)"
BC="/tmp/${STEM}.bc"

banner() { printf "\n\033[1m%s\033[0m\n" "$*"; }
rule()   { printf "%0.s─" $(seq 1 "${1:-72}"); printf "\n"; }

banner "KLEE Searcher Selector — end-to-end demo"
rule
printf "Program:       %s\n" "${PROG}"
printf "Time budget:   %ss each\n" "${TIME}"

banner "[1/4] Compile → LLVM bitcode"
"${CLANG}" -I "${REPO_ROOT}/include" -emit-llvm -c -g -O0 \
    -Xclang -disable-O0-optnone -o "${BC}" "${PROG}"
printf "  %s\n" "${BC}"

banner "[2/4] Static analysis"
python3 "${KLEE_ROOT}/analyze.py" "${PROG}"
FLAGS="$(python3 "${KLEE_ROOT}/analyze.py" "${PROG}" --flags-only)"

run_searcher() {
    local tag="$1" flags="$2" outdir="/tmp/klee_${STEM}_$1"
    rm -rf "${outdir}"
    printf "  Running (%s): %s\n" "${tag}" "${flags}" >&2
    "${KLEE}" --max-time="${TIME}" ${flags} --output-dir="${outdir}" "${BC}" \
        >/dev/null 2>&1 || true
    echo "${outdir}"
}

banner "[3/4] Run KLEE: DEFAULT vs RECOMMENDED"
DEFAULT_OUT="/tmp/klee_${STEM}_default"
SMART_OUT="/tmp/klee_${STEM}_smart"
run_searcher default "--search=random-path --search=nurs:covnew" >/dev/null
run_searcher smart "${FLAGS}" >/dev/null

banner "[4/4] Coverage comparison"
rule

# klee-stats has a bug with multi-dir + custom columns, so query one-by-one.
fetch_row() {
    "${KLEE_STATS}" --print-columns "Instrs,ICov(%),BCov(%),Time(s)" "$1" 2>/dev/null \
        | awk -F'|' '/^\|.*[0-9]/ && !/Instrs/ {gsub(/ /,""); print $2","$3","$4","$5; exit}'
}

DEF_ROW="$(fetch_row "${DEFAULT_OUT}")"
SMART_ROW="$(fetch_row "${SMART_OUT}")"
IFS=',' read -r D_INSTRS D_ICOV D_BCOV D_TIME <<<"${DEF_ROW}"
IFS=',' read -r S_INSTRS S_ICOV S_BCOV S_TIME <<<"${SMART_ROW}"

printf "  %-12s  %10s  %10s  %10s  %10s\n" "searcher" "Instrs" "ICov(%)" "BCov(%)" "Time(s)"
printf "  %-12s  %10s  %10s  %10s  %10s\n" "default"     "${D_INSTRS}" "${D_ICOV}" "${D_BCOV}" "${D_TIME}"
printf "  %-12s  %10s  %10s  %10s  %10s\n" "recommended" "${S_INSTRS}" "${S_ICOV}" "${S_BCOV}" "${S_TIME}"
rule

DELTA_ICOV=$(python3 -c "print(f'{float(\"${S_ICOV}\") - float(\"${D_ICOV}\"):+.2f}')" 2>/dev/null || echo "?")
DELTA_BCOV=$(python3 -c "print(f'{float(\"${S_BCOV}\") - float(\"${D_BCOV}\"):+.2f}')" 2>/dev/null || echo "?")
banner "Improvement: instruction coverage ${DELTA_ICOV} pp, branch coverage ${DELTA_BCOV} pp"
echo "(default = KLEE baseline 'random-path + nurs:covnew'; recommended chosen by static analysis)"
