#!/usr/bin/env bash

tmp=$(mktemp)
trap 'rm -f $tmp*' SIGINT SIGTERM EXIT

"$S2EDIR/build/bin/binrec_lift" --link-prep-1 -o "$tmp" "$1"
llvm-link -o "$2" "$tmp.bc" "$S2EDIR/runlib/softfloat.bc"
"$S2EDIR/build/bin/binrec_lift" --link-prep-2 -o "$tmp" "$2"
mv "$tmp.bc" "$2"
