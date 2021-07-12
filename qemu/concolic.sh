#!/usr/bin/env bash
DEFAULT_MEM_AMOUNT=1024

if [ $# -lt 4 ]
then
    >&2 echo "usage: bash $0 IMAGE STATE CONFIG_FILE ID ARGS..."
    exit 1
fi

# this makes the script pwd-agnostic
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=`realpath $DIR/..`

rootmake() {
    make -f $ROOT/Makefile -s $*
}

image=$1
state=$2
config_file_name=$3
id=$4
args="${@:5}"
timeout=${TIMEOUT-0}
kill_after=30  # send KILL signal 30s after timeout to limit cleanup time
outdirbase=s2e-out-$id
outdir=$ROOT/$outdirbase


if [ "$DEBUG" = x ]; then release=debug; else release=release; fi
#release=debug

# produce core file when in debug mode
if [ "$DEBUG" = x ]
then
    ulimit -c unlimited
    rm -f core
fi

version=1
if [ -d "$outdir-$version" ]; then 
  version=$(( `find $ROOT -maxdepth 1 -type d -name "${outdirbase}-*" | wc -w` + 1 ))
fi

mkdir -p $outdir-$version
outdir=$outdir-$version
outdirbase=$outdirbase-$version
cp $DIR/../plugins/$config_file_name $outdir/
echo $outdir
config_file=$outdir/$config_file_name
sed -i "s/qemu/$outdirbase/g" $config_file
printf -- "$cmd_config" > $outdir/cmd-config

# add makefile to output directory
ln -s $S2EDIR/scripts/s2eout_makefile $outdir/Makefile

# run qemu with configured timeout
# FPar: when using the timeout command without --foreground, S2E can freeze when not run from a shell prompt.
`which time` -f "time elapsed: %es (%E)" \
timeout --foreground -s INT -k $kill_after $timeout \
$ROOT/build/qemu-$release/i386-s2e-softmmu/qemu-system-i386 \
    -net none -loadvm $state -m ${MEM_AMOUNT-$DEFAULT_MEM_AMOUNT} \
    -s2e-config-file $config_file -s2e-verbose \
    -s2e-max-processes 40 -nographic \
    -s2e-output-dir $outdir $image $args #-D $outdir/qemu.log -d in_asm
ret=$?

# generate coverage report if bblist exists (generate with IDA script)
[ -f $outdir/ExecutionTracer.dat ] && rootmake $outdir/coverage

# get unique testcases (more interesting than raw)
[ -f $outdir/testcases.txt ] && rootmake $outdir/testcases-uniq.txt

# notify user if timeout occurred
case $ret in
    124)     echo "qemu timed out" ;;
    12[567]) echo "timeout exited with status $ret"; exit 1 ;;
    137)     echo "qemu was killed with SIGKILL" ;;
esac
