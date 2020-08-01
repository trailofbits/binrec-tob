#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

cd $s2e_outdir
[ $infile != $outfile ] && cp $infile $outfile

#note: error if following pass is run more than once, and it is invoked from optimize.sh
#header "transform environment globals into allocas"
#run -env-to-allocas

header "check for leftover function declarations"
queue -halt-on-declarations

header "remove section globals"
queue -remove-sections

header "remove unused debug helpers"
queue -internalize-debug-helpers -globaldce

#never enabled since Taddeus worked on it
#header "add extern function stubs that jump into the PLT"
#queue -elf-plt-funs  # FIXME: should this pass be removed?

header "remove global memory array"
queue -remove-mem-array

run-queue
if [ "${OPTIMIZE_STACK_BITCASTS}" = "1" ]; then
    run-queue
    header "optimize away stack bitcasts"
    run -O3
fi

run-queue

#never enabled since Taddeus worked on it
# this is not really necessary
#header "internalize stubs"
#queue -internalize -internalize-public-api-list main
# FIXME: funny, this appears to slow things down... (with sha2)
#header "optimize more"
#queue -O3

export pgo_instr=0
if [ "${pgo_instr}" = "1" ]; then
    cp $outfile ${outfile%.bc}_pre_prof.bc # pgo must run on same bc instrumentation was on
    header "Instrument to record profile for PGO"
    queue -pgo-instr-gen -instrprof
    run-queue
fi

#queue -mllvm -fla -mllvm -sub -mllvm -bcf
#run -fla 
 #-sub  -bcf
 queue -add-attributes
 run-queue

# disassemble final bc file for manual debugging
header "disassemble generated bitcode"
mymake -s ${outfile%.bc}.ll

#must assemble the ir with the desired version of llvm passes to run next 
#or there is a producer/ reader mismatch, even though the spec says
#it should be forward compatible
#$S2EDIR/obbuild/bin/llvm-as ${outfile%.bc}.ll -o ${outfile}

header "compile llvm to machine code"
mymake -sr ${outfile%.bc}.o

header "patch compiled llvm into binary"
export no_link_lift=$no_link_lift
exec bash $S2EDIR/scripts/move_sections.sh binary ${outfile%.bc}

