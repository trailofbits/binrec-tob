#!/bin/bash
additional_nargs=2
additional_argsdesc="VM_ENTRY VPC_GLOBAL"

source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

vm_entry=$1
vpc_global=$2

cd $s2e_outdir
[ $infile != $outfile ] && cp $infile $outfile

header "instrument interpreter with printed VPC"
queue -instrument-interpreter -interpreter-entry $vm_entry \
    -vpc-global $vpc_global
run-queue

exec bash $S2EDIR/scripts/lower.sh $options $outfile $outfile
