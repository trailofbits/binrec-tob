export LLVMBIN=$S2EDIR/build/llvm/bin
#LLVMBIN=$S2EDIR/build/llvm-release/Release+Asserts/bin
TRANSLATOR=$S2EDIR/build/lib/libbinrec_translator.so

myopt() {
    opt \
        -load $TRANSLATOR -load-pass-plugin $TRANSLATOR \
        -disable-verify -debug-verbosity "$debug_level" "$@"
        #-debug-verbosity "$debug_level"  -v "$@" #note, will not be able to run the first pass with verifier on, unknown why
}

outfile=${outfile-out.bc}

run() {
    myopt -loglevel "$log_level" -o "$outfile" "$outfile" -passes="$@"
}

run2() {
    myopt -loglevel "$log_level" -o "$outfile" "$outfile" $@
}

run_alt() {
    $S2EDIR/obbuild/bin/opt -v -disable-verify -o "$outfile" "$outfile" "$@"
}

QUEUE=
SEPARATOR=

queue() {
    if [ $# -ge 1 ]; then
        if [ $always_flush -eq 1 ]; then
            run $@
            mymake -s ${outfile%.bc}.ll
        else
            QUEUE+="${SEPARATOR}$@"
            SEPARATOR=","
        fi
    fi
}

run-queue() {
    if [ "$QUEUE" != "" ]; then
        echo "run passes:$QUEUE" >&2
        [ $time_passes -eq 1 ] && queue -time-passes
        run $QUEUE
        QUEUE=
        SEPARATOR=
    fi
}


header() {
    echo -- "$@" -- >&2
}

mylink() {
    llvm-link "$@"
}

mymake() {
    make -f $S2EDIR/scripts/s2eout_makefile "$@" # TODO: move Makefile
}
