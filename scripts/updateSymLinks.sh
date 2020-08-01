#!/usr/bin/env bash
DEFAULT_MEM_AMOUNT=1024

if [ $# -lt 1 ]
then
    >&2 echo "usage: bash $0 FOLDERNAME"
    exit 1
fi

outdir=$S2EDIR/$1

[ $? -ne 0 ] && exit 1

user=$(whoami)
sudo chown -R $user $outdir
#sudo chown $user $outdir/*

path=$(readlink -f $outdir/binary)
#echo $path
binaryPath=$(echo $path | cut -d/ -f-1,5-)
#echo $binaryPath

ln -sf $S2EDIR/$binaryPath $outdir/binary
ln -sf $S2EDIR/scripts/s2eout_makefile $outdir/Makefile

