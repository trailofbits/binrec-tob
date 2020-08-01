#!/bin/bash

############################################################################################

build(){
   . scripts/env.sh
   ./docker/start
   ./docker/run make build && ./docker/run make
   ./docker/runi386 linux32 make build_i386
}

replicate(){
   from=$1
   to=$1
   for ((i=1; i<=9; i++)); do
      cp -r $from $to$i
      to=$to$i
   done
}

buildAll(){
   to=$1-
   for ((i=0; i<=9; i++)); do
      cd $to$i
      build
      to=$to$i
   done
}

update(){
   git checkout -- .
   git pull
}

updateAll(){
   to=$1-
   for ((i=0; i<=9; i++)); do
      cd $to$i
      update
      to=$to$i
   done
}

download(){
   git clone -b spec2006 https://github.com/securesystemslab/BinRec.git $1
   git clone https://github.com/aaltinay/binrec-spec2006.git $1/test/spec2006
   python $1/scripts/google_drive.py 1s7AcCwj928Ag6yh9TLmCIySfao1ndBIx $1/qemu/debian.raw
}

#############################################################################################

docker network ls
docker network prune

to=$HOME/spec-workspace-0
base=$HOME/spec-workspace

#download $to
#replicate $to
#buildAll $base
#updateAll $base

