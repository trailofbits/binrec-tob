#!/bin/bash
source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

final_file=$outfile
fallback_mode=none
debug_level=0
options="$(get_options)"

cd $s2e_outdir

PRUNE_LIBARGS_PUSH=1 bash $S2EDIR/scripts/lift.sh $options $infile lifted.bc

outfile=vars.bc
cp lifted.bc $outfile

header "optimizations for optimally detecting variables"
queue -simplifycfg -dce -dse -globalopt -instcombine

header "add sections and detect variables"
queue -elf-sections -constant-loads -detect-vars -simplifycfg

run-queue

header "disassemble generated bitcode"
mymake -s ${outfile%.bc}.ll

header "generate dot graph of bitcode"
myopt -dot-cfg-only -o /dev/null $outfile 2>/dev/null
[ -e cfg.wrapper.dot ] || { echo "no wrapper dotfile"; exit 1; }
dot -T svg -o ${outfile%.bc}.svg cfg.wrapper.dot
rm cfg.*.dot

echo "Follow the following steps to finalize deobfuscation:"
echo "1. Open ${outfile%.bc}.svg and find the VM entry block"
echo "   (the outer loop entry which also has a backedge)"
echo "2. Open ${outfile%.bc}.ll and find the VPC global "
echo "   (use ida on the binary to find the correct offset)"
echo "3. Run: ../scripts/instrument-vm.sh $outfile instrumented.bc <vm-entry> <vpc-global>"
echo "4. For each input of the binary, run:"
echo "   ../scripts/trace-vpc.sh ./instrumented <arg...> > <tracefile>"
echo "5. Run: ../scripts/untangle-vm.sh $outfile $final_file <vm-entry> <vpc-global> <tracefile...>"
