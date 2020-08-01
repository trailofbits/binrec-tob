#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

options=$(get_options)
final_file=$outfile

cd $s2e_outdir

bash $S2EDIR/scripts/lift.sh $options $infile lifted.bc

outfile=optimized.bc
cp lifted.bc $outfile

mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-before.ll

#header "-alias-global-bb run"
#run -alias-global-bb -dce
#mymake -s ${outfile%.bc}.ll
#mv ${outfile%.bc}.ll ${outfile%.bc}-after.ll

header "run -env-aa and -licm"
run -basicaa -globals-aa -env-aa -licm
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-after-licm.ll

header "inline helper functions for optimization"
run -inline-wrapper -die -always-inline
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-after-alwaysinline.ll

header "run -gvn"
run -dce -gvn -dce
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-after-gvn.ll

header "transform environment globals into allocas"
run -env-to-allocas
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-after-allocas.ll

header "optimization"
run -env-aa -O3 -globalopt
#run -O3 -globalopt
#run -O3

export OPTIMIZE_STACK_BITCASTS=1
exec bash $S2EDIR/scripts/lower.sh $options optimized.bc $final_file
