#!/usr/bin/env bash
# Build the SVF-based feature extractor.
#
# Requires:
#   - SVF source tree built at $SVF_DIR/Release-build (default ~/src/SVF)
#   - llvm-14 dev headers (apt: llvm-14-dev)
#   - cmake, ninja, libz3-dev
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SVF_DIR="${SVF_DIR:-$HOME/src/SVF}"
LLVM_DIR="${LLVM_DIR:-/usr/lib/llvm-14/lib/cmake/llvm}"

if [[ ! -f "$SVF_DIR/Release-build/svf/libSvfCore.a" ]]; then
    echo "ERROR: SVF not built at $SVF_DIR/Release-build" >&2
    echo "Build SVF first:  cd $SVF_DIR && mkdir -p Release-build && cd Release-build && \\" >&2
    echo "    LLVM_DIR=$LLVM_DIR cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSVF_WARN_AS_ERROR=OFF .. && ninja" >&2
    exit 1
fi

mkdir -p "$HERE/build"
cd "$HERE/build"
LLVM_DIR="$LLVM_DIR" SVF_DIR="$SVF_DIR" \
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
          -DLLVM_DIR="$LLVM_DIR" -DSVF_DIR="$SVF_DIR" ..
ninja
echo
echo "Built: $HERE/build/extract_features"
