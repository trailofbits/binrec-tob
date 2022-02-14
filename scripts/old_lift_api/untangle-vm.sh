#!/bin/bash
additional_nargs=3
additional_argsdesc="VM_ENTRY VPC_GLOBAL TRACEFILE [...]"

source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

vm_entry=$1
vpc_global=$2
tracefiles=(${@:3})
final_file=$outfile
fallback_mode=none
debug_level=0
log_level=debug
options="$(get_options)"

cd $s2e_outdir
outfile=untangled.bc
cp $infile $outfile

header "optimizations for optimally detecting variables"
tracefile_args=${tracefiles[@]/#/-vpc-trace }
queue -untangle-interpreter -interpreter-entry $vm_entry \
    -vpc-global $vpc_global $tracefile_args

header "transform environment globals into allocas"
queue -env-to-allocas

run-queue

header "optimize"
run -env-aa -O3 -globalopt -simplifycfg -O1

header "detect newly found variables"
run -simplify-stack-offsets -detect-vars

header "optimize again"
run -env-aa -O2

mymake -s untangled.{ll,svg}

export OPTIMIZE_STACK_BITCASTS=1
exec bash $S2EDIR/scripts/lower.sh $options untangled.bc $final_file
