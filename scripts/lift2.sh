#!/usr/bin/env bash

set -e

S2EDIR=$(dirname "$(pwd)")
export S2EDIR

make -f "$S2EDIR/scripts/s2eout_makefile" symbols
"$S2EDIR/build/bin/binrec_lift" --clean -o cleaned captured.bc
llvm-link -o linked.bc cleaned.bc "$S2EDIR/runlib/custom-helpers.bc"
"$S2EDIR/build/bin/binrec_lift" --lift -o lifted linked.bc --clean-names
"$S2EDIR/build/bin/binrec_lift" --optimize -o optimized lifted.bc --memssa-check-limit=100000
llvm-dis optimized.bc
"$S2EDIR/build/bin/binrec_lift" --compile -o recovered optimized.bc
make -f "$S2EDIR/scripts/s2eout_makefile" -sr recovered.o
"$S2EDIR/build/bin/binrec_link" \
  -b binary \
  -r recovered.o \
  -l "$S2EDIR/build/lib/libbinrec_rt.a" \
  -o recovered \
  -t "$S2EDIR/binrec_link/ld/i386.ld"
