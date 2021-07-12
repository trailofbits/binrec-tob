#!/usr/bin/env bash
DEFAULT_MEM_AMOUNT=1024

if [ $# -lt 3 ]
then
    >&2 echo "usage: bash $0 STATE BINARY PRED_ADDRESS ADDRESS"
    exit 1
fi

state=$1
binary=$2
pred=$3
addr=$4
#targetdir=$3
outdir=$S2EDIR/s2e-out-$binary-BB_$addr

if [ "$DEBUG" = x ]; then release=debug; else release=release; fi

# produce core file when in debug mode
if [ "$DEBUG" = x ]; then
    ulimit -c unlimited
    rm -f core
fi

# remove any old output directory of the same name to avoid conflicts
rm -rf $outdir/*
mkdir -p $outdir

cat $S2EDIR/s2e-out-$binary/symbols > $outdir/symbols
readelf -S $S2EDIR/s2e-out-$binary/binary | awk '$2==".plt" {print $4, $2 >> "'${outdir}/symbols'"}'

# TODO: succs.dat was superseded by traceinfo. Update this if you want to use this.
echo "pluginsConfig.BBExport = { predecessor = '$pred', \
                                 address = '$addr', \
                                 prevSuccs = '$S2EDIR/s2e-out-$binary/succs.dat', \
                                 symbols = '$outdir/symbols' }" | \
    cat $S2EDIR/plugins/config.bbexport-base.lua - > $outdir/config.lua

# add makefile to output directory
ln -sf $S2EDIR/scripts/s2eout_makefile $outdir/Makefile

# cmd_config comes from cmd-debian-bb.sh. This is how other scripts were written. Not ideal.
printf -- "$cmd_config" > $S2EDIR/qemu/cmd-config
printf -- "$cmd_config" > $outdir/cmd-config

$S2EDIR/build/qemu-$release/i386-s2e-softmmu/qemu-system-i386 \
    -net none -loadvm $state -m ${MEM_AMOUNT-$DEFAULT_MEM_AMOUNT} \
    -s2e-config-file $outdir/config.lua -s2e-verbose \
    -s2e-output-dir $outdir $S2EDIR/qemu/debian.s2e \
    -vnc :10 -generate-llvm

if ls $outdir/captured.bc &>/dev/null; then
    echo "Success, moving captured block to $targetdir"
    #mv -t $targetdir $outdir/BB_*.ll
    #[ "$KEEP" = "" ] && rm -rf $outdir s2e-last
else
    echo "Failed to capture block $addr, inspect $outdir"
fi
