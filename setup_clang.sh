#!/bin/bash

set -e

TARGETS=(
    /usr/bin/clang++
    /usr/bin/asan_symbolize
    /usr/bin/clang-cpp
    /usr/bin/clang-check
    /usr/bin/clang-cl
    /usr/bin/ld.lld
    /usr/bin/clang-format
    /usr/bin/clang-format-diff
    /usr/bin/clang-include-fixer
    /usr/bin/clang-offload-bundler
    /usr/bin/clang-query
    /usr/bin/clang-rename
    /usr/bin/clang-reorder-fields
    /usr/bin/clang-tidy
    /usr/bin/llc
    /usr/bin/lld
    /usr/bin/lld-link
    /usr/bin/lldb
    /usr/bin/lldb-server
    /usr/bin/lli
    /usr/bin/lli-child-target
    /usr/bin/llvm-addr2line
    /usr/bin/llvm-ar
    /usr/bin/llvm-as
    /usr/bin/llvm-bcanalyzer
    /usr/bin/llvm-bitcode-strip
    /usr/bin/llvm-cat
    /usr/bin/llvm-cfi-verify
    /usr/bin/llvm-config
    /usr/bin/llvm-cov
    /usr/bin/llvm-c-test
    /usr/bin/llvm-cvtres
    /usr/bin/llvm-cxxdump
    /usr/bin/llvm-cxxfilt
    /usr/bin/llvm-cxxmap
    /usr/bin/llvm-diff
    /usr/bin/llvm-dis
    /usr/bin/llvm-dlltool
    /usr/bin/llvm-dwarfdump
    /usr/bin/llvm-dwp
    /usr/bin/llvm-elfabi
    /usr/bin/llvm-exegesis
    /usr/bin/llvm-extract
    /usr/bin/llvm-gsymutil
    /usr/bin/llvm-ifs
    /usr/bin/llvm-install-name-tool
    /usr/bin/llvm-jitlink
    /usr/bin/llvm-jitlink-executor
    /usr/bin/llvm-lib
    /usr/bin/llvm-libtool-darwin
    /usr/bin/llvm-link
    /usr/bin/llvm-lipo
    /usr/bin/llvm-lto
    /usr/bin/llvm-lto2
    /usr/bin/llvm-mc
    /usr/bin/llvm-mca
    /usr/bin/llvm-ml
    /usr/bin/llvm-modextract
    /usr/bin/llvm-mt
    /usr/bin/llvm-nm
    /usr/bin/llvm-objcopy
    /usr/bin/llvm-objdump
    /usr/bin/llvm-omp-device-info
    /usr/bin/llvm-opt-report
    /usr/bin/llvm-otool
    /usr/bin/llvm-pdbutil
    /usr/bin/llvm-PerfectShuffle
    /usr/bin/llvm-profdata
    /usr/bin/llvm-profgen
    /usr/bin/llvm-ranlib
    /usr/bin/llvm-rc
    /usr/bin/llvm-readelf
    /usr/bin/llvm-readobj
    /usr/bin/llvm-reduce
    /usr/bin/llvm-rtdyld
    /usr/bin/llvm-sim
    /usr/bin/llvm-size
    /usr/bin/llvm-split
    /usr/bin/llvm-stress
    /usr/bin/llvm-strings
    /usr/bin/llvm-strip
    /usr/bin/llvm-symbolizer
    /usr/bin/llvm-tapi-diff
    /usr/bin/llvm-tblgen
    /usr/bin/llvm-undname
    /usr/bin/llvm-windres
    /usr/bin/llvm-xray
)

function register_clang_version {
    local version=$1
    local priority=$2
    local items=()

    for item in ${TARGETS[@]};
    do
        if [ -f ${item}-${version} ];
        then
            name=$(basename $item)
            items+=("--slave ${item} ${name} ${item}-${version}")
        fi
    done

    sudo update-alternatives \
        --verbose \
        --install /usr/bin/clang clang /usr/bin/clang-${version} ${priority} \
        ${items[@]}
}

if [ -f /usr/bin/clang-12 ];
then
    register_clang_version 12 100
fi

if [ -f /usr/bin/clang-13 ];
then
    register_clang_version 13 90
fi
