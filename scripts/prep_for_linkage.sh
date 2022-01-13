#!/usr/bin/env bash

tmp=$(mktemp)
trap 'rm -f $tmp*' SIGINT SIGTERM EXIT
"$S2EDIR/../build/bin/binrec_lift" --link-prep-1 -o "$tmp" "$1"
#llvm-link -o "$2" "$tmp.bc" "$S2EDIR/../runlib/softfloat.bc"
# TODO (hbrodin)" Is this correct?? Different targets exists."
# TODO (hbrodin): Removing it for now because it results in multiply defined symbols
#llvm-link -o "$2" "$tmp.bc" "$S2EDIR/build/libs2e-release/i386-s2e-softmmu/libcpu/src/softfloat.bc"
ls -la $tmp
mv "$tmp.bc" "$2"
"$S2EDIR/../build/bin/binrec_lift" --link-prep-2 -o "$tmp" "$2"
mv "$tmp.bc" "$2"
