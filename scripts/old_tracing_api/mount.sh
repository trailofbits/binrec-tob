#!/bin/sh
if [ $# -lt 1 ]; then echo "usage: sh $0 IMAGE [ -w ]" 1>&2; exit 1; fi
sudo true || { echo "need root permissions"; exit 1; }
image=$1

if [ "$2" = -w ]; then
    mountflags=
else
    mountflags=-r
fi

tmpdir=/tmp/s2e-vm

loop=`sudo kpartx -av $image | grep -oP 'loop\d\w+'`
mkdir -p $tmpdir
sleep 0.2
sudo mount $mountflags /dev/mapper/$loop $tmpdir

cd $tmpdir
bash -i

sudo umount $tmpdir
rmdir $tmpdir
sudo kpartx -d $image
