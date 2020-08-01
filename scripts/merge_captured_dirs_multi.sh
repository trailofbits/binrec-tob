#!/bin/bash
echo "Hello from merge_captured_dirs_multi"
source $S2EDIR/scripts/outrc.sh

if [[ ! -n "$1" ]]; then
    echo "Please provide binary name"
    exit 1
fi
set -e
# this makes the script pwd-agnostic
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=`realpath $DIR/..`

merge_inner() {
echo "mif"
echo "$capdir_regex"
# remove any old output directory of the same name to avoid conflicts
rm -rf $outdir/*
mkdir -p $outdir
#capdir_regex=$@
#create link-ready captured.bc files
header "Creating link-ready captured.bc files.."
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        echo $file
        #infile=$file/captured.bc
        #outfile=$file/captured-link-ready.bc
        command="cd $file && $S2EDIR/scripts/prep_for_linkage.sh -f error captured.bc captured-link-ready.bc"
        eval $command
        #(eval $ommand) & #theoretically this should parallelize, but it makes malfomred files for some reason
    fi
done

#copy one of the captured.bc file to outdir and use it to link with others
header "Linking all the captured.bc files.."
outfile=$outdir/captured.bc
found=0
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        #echo $file
        cp $file/captured-link-ready.bc $outfile
        found=1
        break
    fi
done

[ $found -ne 1 ] && exit 1

tmp=$(mktemp --suffix .bc)
trap "rm -f $tmp" SIGINT SIGTERM EXIT
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        #echo $file
        #llvm-link-3.8 -v -print-all-options -o $tmp -only-needed -suppress-warnings -override=$outfile $file/captured.bc && mv $tmp $outfile
        command="$LLVMBIN/llvm-link -print-after-all -v -o $tmp -override=$outfile $file/captured-link-ready.bc && mv $tmp $outfile"
        eval $command
        #mylink -only-needed -v -o $tmp $outfile $file/captured-link-ready.bc && mv $tmp $outfile
        #mylink -v -o $tmp -override=$outfile $file/captured-link-ready.bc && mv $tmp $outfile
        #break
    fi
done
header "Disassembling generated bitcode.."
#mymake -s ${outfile%.bc}.ll
eval "$LLVMBIN/llvm-dis $outfile"

#copy one of the succs.dat file to outdir and use it to merge with others
header "Merging succ.dat files.."
found=0
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        cp $file/succs.dat $outdir
        cp $file/callerToFollowUp $outdir
        cp $file/entryToCaller $outdir
        cp $file/entryToReturn $outdir
        found=1
        break
    fi
done

[ $found -ne 1 ] && echo "Did not find frontend data, " && exit 1

echo "finished 1 succ"
#merge with other succs.dat
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        python $ROOT/scripts/merge_succs.py $outdir/succs.dat $file/succs.dat
        python $ROOT/scripts/merge_succs.py $outdir/callerToFollowUp $file/callerToFollowUp
        python $ROOT/scripts/merge_succs.py $outdir/entryToCaller $file/entryToCaller
        python $ROOT/scripts/merge_succs.py $outdir/entryToReturn $file/entryToReturn
    fi
done
echo "finished all succ"

#func_regex=$(find $file -maxdepth 1 -print | grep "func_*")
#echo "funcregex: $func_regex "
for file in $capdir_regex ; do
    if [[ -d $file ]] ; then
        echo $file
        #funcbase=$file/func
        func_regex=$(find $file -maxdepth 1 -print | grep "func_*")
        echo "funcregex: $func_regex "
        for func_file_p in $func_regex ; do
        #for func_file_p in `find $file -maxdepth 1 -name "func_*"` ; do
            #echo $func_file_p
            if [[ -f $func_file_p ]] ; then
                func_file_n="$(basename $func_file_p)"
                if [[ -f $outdir/$func_file_n ]] ; then
                    #if exists this file in the outdir, merge others into it
                    python $ROOT/scripts/merge_funcs.py $outdir/$func_file_n $func_file_p
                else
                    cp $func_file_p $outdir
                fi
            fi
        done
    fi
done
echo "done mi"
}

merge_all_inputs() {
    echo "mai"
    id=$1
    outdirbase=s2e-out-$id
    outdir=$ROOT/$outdirbase
    capdir_regex=$(find $ROOT -maxdepth 2 -print | grep "${outdirbase}-[0-9]\{1,\}/merged")
    merge_inner 
    header "Creating necessary symlink.."
    for file in $capdir_regex ; do
        if [[ -d $file ]] ; then
            cp -P $file/binary $outdir/binary
            cp -P $file/Makefile $outdir/Makefile
            break
        fi
    done
}

merge_one_input() {
    echo "moi"
    id=$1
    outdirbase=s2e-out-$id
    outdir=$ROOT/$outdirbase/merged
    capdir_regex=$(find $ROOT -maxdepth 2 -print | grep "${outdirbase}/[0-9]\{1,\}")
    merge_inner 
    header "Creating necessary symlink.."
    cp -P $ROOT/$outdirbase/0/binary $outdir/binary
    cp -P $ROOT/$outdirbase/Makefile $outdir/Makefile
    echo "moi-end"
}


