#!/usr/bin/env bash

set -e

export S2EDIR=$(pwd)/..

make -f "$S2EDIR/scripts/s2eout_makefile" symbols
"$S2EDIR/build/bin/binrec-lift" --clean -o cleaned captured.bc --no-link-lift
llvm-link -o linked.bc cleaned.bc "$S2EDIR/runlib/custom-helpers.bc"
"$S2EDIR/build/bin/binrec-lift" --lift -o lifted linked.bc --no-link-lift
"$S2EDIR/build/bin/binrec-lift" --optimize -o optimized lifted.bc --memssa-check-limit=100000
llvm-dis optimized.bc
"$S2EDIR/build/bin/binrec-lift" --compile -o recovered optimized.bc --no-link-lift
make -f "$S2EDIR/scripts/s2eout_makefile" -sr recovered.o
"$S2EDIR/build/bin/binrec-link" binary recovered.o "$S2EDIR/build/lib/libbinrec-rt.a" recovered "$S2EDIR/binrec-link/ld/i386.ld"
