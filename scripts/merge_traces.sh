#!/bin/bash
if [[ ! -n "$1" ]]; then
    echo "Please provide binary name"
    exit 1
fi
if [ -z "$S2EDIR" ]; then
    echo "S2EDIR is not set"
    exit 1
fi
source $S2EDIR/../scripts/merge_captured_dirs_multi.sh
#merge_one_input $1
merge_all_proj_inputs $1





