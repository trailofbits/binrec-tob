export LLVMBIN=$S2EDIR/multicompiler/bin
#LLVMBIN=$S2EDIR/build/llvm-release/Release+Asserts/bin
TRANSLATOR=$S2EDIR/build/tran-passes-release/translator/translator.so

myopt() {
    $LLVMBIN/opt \
        -load $TRANSLATOR \
        -disable-verify -debug-verbosity "$debug_level" "$@"
        #-debug-verbosity "$debug_level"  -v "$@" #note, will not be able to run the first pass with verifier on, unknown why
}

outfile=${outfile-out.bc}

run() {
    myopt -loglevel "$log_level" -s2e-out-dir . -o "$outfile" "$outfile" "$@"
}

run_alt() {
    $S2EDIR/obbuild/bin/opt -v -disable-verify -s2e-out-dir . -o "$outfile" "$outfile" "$@"
}

QUEUE=

queue() {
    if [ $# -ge 1 ]; then
        if [ $always_flush -eq 1 ]; then
            run $@
            mymake -s ${outfile%.bc}.ll
        else
            QUEUE+=" $@"
        fi
    fi
}

run-queue() {
    if [ "$QUEUE" != "" ]; then
        echo "run passes:$QUEUE" >&2
        [ $time_passes -eq 1 ] && queue -time-passes
        run $QUEUE
        QUEUE=
    fi
}


header() {
    echo -- "$@" -- >&2
}

mylink() {
    $LLVMBIN/llvm-link "$@"
}

mymake() {
    make -f $S2EDIR/scripts/s2eout_makefile "$@" # TODO: move Makefile
}
