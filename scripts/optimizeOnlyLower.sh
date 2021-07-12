#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

options=$(get_options)
final_file=$outfile

cd $s2e_outdir

outfile=optimized.bc
cp $infile $outfile

header "transform environment globals into allocas"
run -env-to-allocas

header "optimization"
#run -basicaa -scev-aa  -env-aa -argpromotion -licm -globalopt 
run -basicaa -scev-aa  -env-aa -argpromotion -licm -globalopt -O3
#run -basicaa -scev-aa -env-aa -ipsccp -argpromotion


#export OPTIMIZE_STACK_BITCASTS=1
exec bash $S2EDIR/scripts/lower.sh $options optimized.bc $final_file
