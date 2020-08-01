#!/usr/bin/env bash
additional_argsdesc="[PASS_OPTION...]"

source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

additional_options="$@"

cd $s2e_outdir
[ $infile != $outfile ] && cp $infile $outfile

tmp=$(mktemp --suffix .bc)
trap "rm -f $tmp" SIGINT SIGTERM EXIT

header "rename captured blocks to uniform prefix"
queue -rename-block-funcs

header "set linkage types and remove excess definitions"
queue -externalize-functions -internalize-imports -globaldce

run-queue

header "link generated helpers (softfloat.bc)"
mylink -o $tmp $outfile $S2EDIR/runlib/softfloat.bc && mv $tmp $outfile

header "prune excess helper implementations"
queue -internalize-imports -unimplement-custom-helpers -globaldce
run-queue

header "transform environment accesses"
#UnflattenEnv pass fails on cast operation. It looks like we need to understand what it does and change it accordingly.
queue -unflatten-env
run-queue 
queue -decompose-env
run-queue 
queue -rename-env
run-queue

#header "remove more excess helpers"
##queue -instcombine -tag-inst-pc -remove-qemu-helpers -dce
#queue -instcombine
#run-queue
#queue -tag-inst-pc
#run-queue
#queue -remove-qemu-helpers
#run-queue
#queue -dce
#run-queue


#header "link generated helpers (softfloat.bc)"
#mylink -o $tmp $outfile $S2EDIR/runlib/softfloat.bc && mv $tmp $outfile

#header "prune excess helper implementations"
#queue -internalize-imports -unimplement-custom-helpers -globaldce

#header "transform environment accesses"
#queue -unflatten-env -decompose-env -rename-env

#header "remove more excess helpers"
#queue -instcombine -tag-inst-pc -remove-qemu-helpers -dce

#header "insert variables that can be used by custom helpers"
#queue -add-custom-helper-vars $additional_options

#run-queue

#header "link custom helpers"
#mylink -o $tmp $outfile $S2EDIR/runlib/custom-helpers.bc && mv $tmp $outfile

