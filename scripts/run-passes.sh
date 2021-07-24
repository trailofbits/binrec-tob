#!/usr/bin/env bash
#LLVMBIN=$S2EDIR/build/llvm-debug/Debug+Asserts/bin
LLVMBIN=$S2EDIR/build/llvm-release/Release+Asserts/bin
TRANSLATOR=$S2EDIR/build/lib/libbinrec_translator.so

if [ $# -lt 3 ]
then
    echo "usage: $0 INFILE OUTFILE PASSES..."
    exit 1;
fi

infile=$1
outfile=$2
passes=${@:3}

set -e

$LLVMBIN/opt -load $TRANSLATOR -o $outfile $infile $passes
$LLVMBIN/llvm-dis $outfile
