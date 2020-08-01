#!/usr/bin/env bash
#LLVMBIN=$S2EDIR/build/llvm-debug/Debug+Asserts/bin
LLVMBIN=$S2EDIR/build/llvm-release/Release+Asserts/bin
TRANSLATOR=$S2EDIR/build/tran-passes-release/translator/translator.so

if [ $# -lt 3 ]
then
    echo "usage: $0 INFILE OUTFILE PASSES..."
    exit 1;
fi

infile=$1
outfile=$2
passes=${@:3}
s2e_outdir=`dirname $infile`

set -e

$LLVMBIN/opt -load $TRANSLATOR -s2e-out-dir $s2e_outdir -o $outfile $infile $passes
$LLVMBIN/llvm-dis $outfile
