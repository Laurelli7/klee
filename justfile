default: rebuild

uclibc:
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p 3rd
    if [ ! -d "3rd/klee-uclibc" ]; then
        git clone git@github.com:klee/klee-uclibc.git 3rd/klee-uclibc
    fi
    pushd 3rd/klee-uclibc
        ./configure --make-llvm-lib
        make -j$(nproc)
    popd

libcxx:
    LLVM_VERSION=13 BASE=$PWD/3rd/libcxx ENABLE_OPTIMIZED=1 DISABLE_ASSERTIONS=1 ENABLE_DEBUG=0 REQUIRES_RTTI=1 scripts/build/build.sh libcxx


klee: uclibc
    cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_SOLVER_Z3=ON -DENABLE_SOLVER_STP=ON -DENABLE_POSIX_RUNTIME=ON -DKLEE_UCLIBC_PATH=$PWD/3rd/klee-uclibc -DLLVMCC=$(which clang) -DLLVMCXX=$(which clang++)
    cmake --build build

rebuild:
    cmake --build build

clean:
    rm -rf /tmp/klee-out/*
