#!/bin/bash
if [ ! -f default.profraw ]; then
    echo "Profile not found!, please run lower.sh with pgo enabled, and run the program"
    exit 1
fi

source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh
cd $s2e_outdir

header "use pgo"
export pgo_use=1
$LLVMBIN/llvm-profdata merge -output=default.profdata default.profraw
#$LLVMBIN/opt -pgo-instr-use 
#$LLVMBIN/opt  -pgo-instr-use default.profdata ${infile%.bc}_pre_prof.bc -o "$outfile"
$LLVMBIN/opt  -pgo-instr-use -pgo-test-profile-file=default.profdata ${infile%.bc}_pre_prof.bc -o "$outfile"

run -O3

# disassemble final bc file for manual debugging
header "disassemble generated bitcode"
mymake -s ${outfile%.bc}.ll

header "compile llvm to machine code"
mymake -sr ${outfile%.bc}.o

header "patch compiled llvm into binary"
exec bash $S2EDIR/scripts/move_sections.sh binary ${outfile%.bc}
