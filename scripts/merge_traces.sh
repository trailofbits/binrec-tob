#!/bin/bash
if [[ ! -n "$1" ]]; then
    echo "Please provide binary name"
    exit 1
fi
source $S2EDIR/scripts/merge_captured_dirs_multi.sh 
merge_one_input $1





