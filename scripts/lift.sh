#!/bin/bash
additional_argsdesc="[PASS_OPTION...]"

source $S2EDIR/scripts/parse-opts.sh
source $S2EDIR/scripts/outrc.sh

additional_options="$@"

cd $s2e_outdir
[ $infile != $outfile ] && cp $infile $outfile

tmp=$(mktemp --suffix .bc)
trap "rm -f $tmp" SIGINT SIGTERM EXIT

#header "rename captured blocks to uniform prefix"
#queue -rename-block-funcs

#run-queue
#header "set linkage types and remove excess definitions"
#queue -externalize-functions -internalize-imports -globaldce

#run-queue

##if we remove all exceptions, it gets stuck in the loop
##header "remove calls to helper_raise_exception"
##queue -remove-exceptions
##run-queue

#header "link generated helpers (softfloat.bc)"
#mylink -o $tmp $outfile $S2EDIR/runlib/softfloat.bc && mv $tmp $outfile

#header "prune excess helper implementations"
#queue -internalize-imports -unimplement-custom-helpers -globaldce
#run-queue

#header "transform environment accesses"
##UnflattenEnv pass fails on cast operation. It looks like we need to understand what it does and change it accordingly.
#queue -unflatten-env
#run-queue 
#queue -decompose-env
#run-queue 
#queue -rename-env
#run-queue

header "remove more excess helpers"
#queue -instcombine -tag-inst-pc -remove-qemu-helpers -dce
queue -instcombine
#run-queue
queue -tag-inst-pc
#run-queue
#mymake -s ${outfile%.bc}.ll
#mv ${outfile%.bc}.ll ${outfile%.bc}-tagip.ll
run-queue
queue -remove-qemu-helpers
#run-queue
queue -dce
run-queue

header "insert variables that can be used by custom helpers"
queue -add-custom-helper-vars $additional_options
run-queue

header "link custom helpers"
mylink -o $tmp $outfile $S2EDIR/runlib/custom-helpers.bc && mv $tmp $outfile

header "set data layout to 32-bit environment"
queue -set-data-layout-32
#run-queue
#mymake -s ${outfile%.bc}.ll
#mv ${outfile%.bc}.ll ${outfile%.bc}-set-data-layout32.ll

header "zero-initialize decomposed environment variables"
queue -internalize-globals -globaldce
run-queue

header "create metadata annotations for successor lists"
queue -successor-lists
run-queue
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-succs-list.ll

header "add symbol annotations to stubs that call into the PLT"
mymake symbols
queue -elf-symbols
run-queue 
queue -prune-null-succs
#run-queue
#mymake -s ${outfile%.bc}.ll
#mv ${outfile%.bc}.ll ${outfile%.bc}-prune-null-succs.ll

header "transform Functions into BasicBlocks"
queue -merge-functions
run-queue
mymake -s ${outfile%.bc}.ll
mv ${outfile%.bc}.ll ${outfile%.bc}-merge-funcs.ll

if [ "$log_level" = "debug" ]; then
    header "plot overlapping BBs"
    queue -dot-overlaps
fi

header "merge basic blocks that cover overlapping PC ranges"
# TODO(jmnash):FixOverlaps may have transient failures after port to 3.8
queue -fix-overlaps -prune-trivially-dead-succs
run-queue

if [ "$log_level" = "debug" ]; then
    header "plot merged BBs"
    queue -dot-merged-blocks
fi

header "instrument extern function calls with register marshalling"
queue -extern-plt
run-queue

header "insert debug statements at the start of basic blocks"
queue -add-debug-output -always-inline
run-queue

header "inline extern function stubs to untangle the CFG (optimization)"
queue -inline-stubs
run-queue

header "insert control flow instructions based on successor lists"
queue -pc-jumps -fallback-mode $fallback_mode
run-queue

PRUNE_LIBARGS_PUSH=0
if [ "${PRUNE_LIBARGS_PUSH}" = "1" ]; then
  header "prune pushed return addresses at libcalls"
  queue -prune-retaddr-push
fi

queue -fix-cfg
run-queue
mymake -s ${outfile%.bc}.ll
cp ${outfile%.bc}.ll lifted-fix.ll

if [ "$fallback_mode" = "unfallback" ]; then 
    header "find functions in original binary"
    PYTHONPATH=$S2EDIR/runlib \
        python $S2EDIR/scripts/find_functions.py binary
	header "insert trampolines for orig binary to call recovered functions"
	queue -recov-func-trampolines
fi

run-queue
mymake -s ${outfile%.bc}.ll
cp ${outfile%.bc}.ll lifted-rft.ll

queue -insert-tramp-for-rec-funcs
run-queue

mymake -s ${outfile%.bc}.ll
cp ${outfile%.bc}.ll lifted-itfrf.ll

header "inline arguments to extern functions to elimitate unnecessary stack accesses"
queue -inline-lib-call-args -die -always-inline
#queue -die -always-inline

run-queue

if [ "$no_link_lift" = "0" ]; then
    header "replace raw access to dynamic symbol addresses"
    # must run this pass while __ldl_mmu type mem access still present
    python  $S2EDIR/scripts/get_dynamic_symbols.py dynsym
    queue -replace-dynamic-symbols
    run-queue
fi

if [ "${PRUNE_LIBARGS_PUSH}" = "1" ]; then
    header "prune pushed arguments at libcalls with known argument counts"
    queue -prune-libargs-push
fi

#header "Replace fn ptrs which could be passed to library calls with their lifted equivalents"
#broken pass, likely addressed by fix-cfg and recovfunctrampolines
#queue -replace-local-function-pointers
#run-queue

if [ "$no_link_lift" = "0" ]; then
    header "Replace fn ptr args to extern call helper with ptr to new PLT"
    queue -lib-call-new-plt

    header "implement stubs to call new plt"
    queue -implement-lib-calls-new-plt
else
    header "implement stubs to call old plt"
    queue -implement-lib-call-stubs
fi

header "replace memory accesses with getelementptr accesses into a global memory array"
queue -add-mem-array
run-queue

#snippet to view bitcode at a certain point
#mymake -s ${outfile%.bc}.ll
#cp ${outfile%.bc}.ll aama.ll


header "remove unused functions (all except main and debug helpers)"
queue -internalize-functions -globaldce

header "unalign stack"
queue -unalign-stack

header "remove leftover metadata"
queue -remove-metadata

queue $additional_options
run-queue

header "disassemble generated bitcode"
mymake -s ${outfile%.bc}.ll

if [ "$no_link_lift" = "0" ]; then
    touch dylibs
    echo "Before running lower.sh, edit file dylibs to contain the flags to link any libraries your binary needs in one line"
fi
