#!/bin/bash
while read -r line; do 
    dir=$(echo "$line" | cut -d ' ' -f 1)
    fulldir="$S2EDIR/s2e-out-${dir}-2/"
    args=$(echo "$line" | cut -d ' ' -f 2-)
    (
    for item in $args; do
        cp $S2EDIR/test/spec2006/all-the-inputs/$item $fulldir
    done
    cd $fulldir
    ../scripts/lift.sh captured.bc lif.bc &> liflog
    ../scripts/lower.sh lif.bc low.bc &> lowlog
    ) &
done < "$1"
