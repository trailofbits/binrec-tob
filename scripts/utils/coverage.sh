#!/usr/bin/env bash
if [ $# -lt 1 ]
then
    >&2 echo "usage: bash $0 S2E_OUTDIR"
    exit 1
fi
outdir=$1

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=$DIR/..

check_exists() {
    [ -e $1 ] || { echo "file $1 does not exist"; exit 1; }
}

check_exists $outdir/ExecutionTracer.dat
check_exists $outdir/binary

mod=`readlink $outdir/binary`
moddir=`dirname $mod`
basename=`basename $mod`

if [ -f ${mod}.bblist ]; then
    set -e
    make -f $ROOT/Makefile -s ${mod}.excl
    $ROOT/build/tools-release/Release+Asserts/bin/coverage \
        -trace=$outdir/ExecutionTracer.dat -outputdir=$outdir \
        -moddir=$moddir >/dev/null 2>/dev/null
    mv $outdir/${basename}.repcov $outdir/coverage-full.txt
    [ -f $outdir/${basename}.repcov-filtered ] && \
        mv $outdir/${basename}.repcov-filtered $outdir/coverage-filtered.txt
    mv $outdir/${basename}.timecov $outdir/coverage-time.txt
    mv $outdir/${basename}.bbcov $outdir/coverage-bbs.txt
else
    echo "missing ${mod}.bblist, skipping coverage report"
fi
