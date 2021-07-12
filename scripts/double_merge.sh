#!/bin/bash
if [[ ! -n "$1" ]]; then
    echo "Please provide binary name"
    exit 1
fi
source $S2EDIR/scripts/merge_captured_dirs_multi.sh 

set -e
name_regex=$(find $ROOT -maxdepth 1 -print | grep "s2e-out-*" | grep -o "$1-[0-9]\{1,\}")
echo $name_regex
for file in $name_regex ; do
    merge_one_input $file
done
merge_all_inputs $1
echo "Finished merging program $1"
set +e
