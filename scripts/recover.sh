#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

options=$(get_options)

cd $s2e_outdir

bash $S2EDIR/scripts/lift.sh $options $infile lifted.bc
exec bash $S2EDIR/scripts/lower.sh $options lifted.bc $outfile
