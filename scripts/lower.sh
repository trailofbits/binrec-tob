#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

cd $s2e_outdir
[ $infile != $outfile ] && cp $infile $outfile

#note: error if following pass is run more than once, it is invoked from optimize.sh
#header "transform environment globals into allocas"
#run -env-to-allocas

header "check for leftover function declarations"
queue halt-on-declarations

header "remove section globals"
queue remove-sections

header "remove unused debug helpers"
queue internalize-debug-helpers,globaldce

#header "add extern function stubs that jump into the PLT"
#queue elf-plt-funs  # FIXME: should this pass be removed?

header "remove global memory array"
queue remove-mem-array

run-queue
if [ "${OPTIMIZE_STACK_BITCASTS}" = "1" ]; then
    run-queue
    header "optimize away stack bitcasts"
    run2 -O3
fi

run-queue

# this  is not really necessary
#header "internalize stubs"
#queue internalize -internalize-public-api-list main

export pgo_instr=0
if [ "${pgo_instr}" = "1" ]; then
    cp $outfile ${outfile%.bc}_pre_prof.bc # pgo must run on same bc instrumentation was on
    header "Instrument to record profile for PGO"
    queue pgo-instr-gen,instrprof
    run-queue
fi

# disassemble final bc file for manual debugging
header "disassemble generated bitcode"
mymake -s ${outfile%.bc}.ll

#must assemble the ir with the desired version of llvm to run next 
#or there is a producer/ reader mismatch, even though the llvm ir spec says
#it should be forward compatible
#$S2EDIR/obbuild/bin/llvm-as ${outfile%.bc}.ll -o ${outfile}

header "compile llvm to machine code"
mymake -sr ${outfile%.bc}.o

header "patch compiled llvm into binary"
if ! $S2EDIR/build/bin/binrec_link binary ${outfile%.bc}.o "$S2EDIR/build/lib/libbinrec_rt.a" ${outfile%.bc} $S2EDIR/binrec_link/ld/i386.ld; then
    echo "compilation failed"
    exit 1
fi

# This is the old linking script, it can do a few more things than the new linking but is kind of messy.
# Hopefully remove this one day.
#export no_link_lift=$no_link_lift
#exec bash $S2EDIR/scripts/move_sections.sh binary ${outfile%.bc}
