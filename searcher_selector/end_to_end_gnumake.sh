#!/usr/bin/env bash
# end_to_end_gnumake.sh
#
# End-to-end experiment: run KLEE on gnumake's make.bc with
#   (a) the searcher recommended by the SVF static analysis, and
#   (b) KLEE's default searcher (random-path interleaved with nurs:covnew).
# Then compare instruction coverage, completed paths, and generated tests.
#
# Requires:
#   - docker (klee/klee:3.1 image will be pulled if missing)
#   - sudo (for docker)
#   - searcher_selector/svf_features/build/extract_features built
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KLEE_ROOT="$(cd "${HERE}/.." && pwd)"

BC_HOST="${BC_HOST:-${KLEE_ROOT}/examples/gnumake/install/bin/make.bc}"
TIME="${TIME:-180}"            # per-run budget in seconds
SYM_ARGS="${SYM_ARGS:-3 3 6}"  # min-args max-args max-len  (default: tiny)
RESULTS_DIR="${RESULTS_DIR:-/tmp/gnumake_experiment_$(date +%Y%m%d_%H%M%S)}"
DOCKER_IMG="${DOCKER_IMG:-klee/klee:3.1}"

if [[ ! -f "${BC_HOST}" ]]; then
    echo "ERROR: ${BC_HOST} not found" >&2
    exit 1
fi

mkdir -p "${RESULTS_DIR}"
echo "Results dir: ${RESULTS_DIR}"
echo "Bitcode:     ${BC_HOST}  ($(wc -c < "${BC_HOST}") bytes)"
echo "Per-run:     ${TIME}s"
echo "sym-args:    ${SYM_ARGS}"
echo

# ---- 1. Static analysis (SVF backend) ---------------------------------
echo "=== [1/3] SVF static analysis ==="
ANALYSIS_JSON="${RESULTS_DIR}/svf_analysis.json"
python3 "${HERE}/analyze.py" --backend svf --json "${BC_HOST}" > "${ANALYSIS_JSON}"
SMART_FLAGS="$(python3 -c '
import json,sys
d=json.load(open(sys.argv[1]))
print(" ".join(d["recommended_flags"]))
' "${ANALYSIS_JSON}")"
SMART_REASON="$(python3 -c '
import json,sys
d=json.load(open(sys.argv[1]))
print(d["explanation"])
' "${ANALYSIS_JSON}")"

DEFAULT_FLAGS="--search=random-path --search=nurs:covnew"

echo "  recommended : ${SMART_FLAGS}"
echo "  rule reason : ${SMART_REASON}"
echo "  default     : ${DEFAULT_FLAGS}"
echo

# ---- 2. Two KLEE runs --------------------------------------------------
BC_DIR="$(dirname "${BC_HOST}")"
BC_NAME="$(basename "${BC_HOST}")"

run_klee () {
    local tag="$1" flags="$2"
    local outdir_host="${RESULTS_DIR}/klee_${tag}"
    local outdir_in_container="/out/klee_${tag}"
    local log="${RESULTS_DIR}/klee_${tag}.log"

    rm -rf "${outdir_host}"
    mkdir -p "${outdir_host}"

    echo "--- Running KLEE [${tag}] (${flags}) ---"
    set +e
    sudo docker run --rm \
        -v "${BC_DIR}:/in:ro" \
        -v "${RESULTS_DIR}:/out" \
        --ulimit stack=-1:-1 \
        "${DOCKER_IMG}" \
        klee \
            --output-dir="${outdir_in_container}" \
            --max-time="${TIME}" \
            --libc=uclibc \
            --posix-runtime \
            --external-calls=all \
            --only-output-states-covering-new \
            --use-forked-solver \
            --max-solver-time=30 \
            --watchdog \
            ${flags} \
            "/in/${BC_NAME}" \
            --sym-args ${SYM_ARGS} \
            > "${log}" 2>&1
    local rc=$?
    set -e
    # fix root-owned output
    sudo chown -R "$USER:$USER" "${outdir_host}" "${log}" 2>/dev/null || true
    echo "  exit=${rc}    log=${log}"
}

echo "=== [2/3] Two KLEE runs (${TIME}s each) ==="
run_klee "smart"   "${SMART_FLAGS}"
run_klee "default" "${DEFAULT_FLAGS}"
echo

# ---- 3. Compare --------------------------------------------------------
extract () {
    # extract a single 'KLEE: done: <key> = <val>' line from a klee log
    local log="$1" key="$2"
    grep -E "^KLEE: done: ${key} = " "${log}" | tail -1 | sed -E "s/.*= //"
}

echo "=== [3/3] Comparison ==="
printf "%-12s | %12s | %18s | %16s | %14s | %14s\n" \
    "Run" "Wall (s)" "Total instrs" "Covered instrs" "Compl paths" "Tests"
printf -- "-------------|--------------|--------------------|------------------|----------------|---------------\n"

for tag in smart default; do
    log="${RESULTS_DIR}/klee_${tag}.log"
    wall=$(grep -E "^KLEE: done: time elapsed = " "${log}" | tail -1 | sed -E 's/.*= //; s/s$//' || true)
    total=$(extract "${log}" "total instructions" || echo "?")
    cov=$(extract "${log}" "covered instructions" || echo "?")
    paths=$(extract "${log}" "completed paths" || echo "?")
    tests=$(extract "${log}" "generated tests" || echo "?")
    printf "%-12s | %12s | %18s | %16s | %14s | %14s\n" \
        "$tag" "${wall:-?}" "${total}" "${cov}" "${paths}" "${tests}"
done

echo
echo "Full logs and KLEE output: ${RESULTS_DIR}"
