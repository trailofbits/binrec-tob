#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

options=$(get_options)
final_file=$outfile

cd $s2e_outdir

bash $S2EDIR/scripts/lift.sh $options $infile lifted.bc

outfile=optimized.bc
cp lifted.bc $outfile

#header "transform environment globals into allocas"
#run -env-to-allocas

header "optimization"
#run -env-aa -O3 -globalopt

#export OPTIMIZE_STACK_BITCASTS=1
exec bash $S2EDIR/scripts/lower.sh $options optimized.bc $final_file
