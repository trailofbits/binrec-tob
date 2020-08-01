#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

cd $s2e_outdir

rmargs=
for f in BB_*.ll; do
    addr=$(sed 's/^BB_\(.*\)\.ll$/\1/' <<< $f)
    rmargs+=" -remove-block 0x$addr"
done

header "rename blocks and remove blocks from BB_*.ll"
myopt -s2e-out-dir . \
    -rename-block-funcs \
    -remove-blocks $rmargs \
    -S -o ${outfile%.bc}.ll $infile

header "append fixup blocks"
cat BB_*.ll >> ${outfile%.bc}.ll

header "assemble bitcode"
mymake -s $outfile
