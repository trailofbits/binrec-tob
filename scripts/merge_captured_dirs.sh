#!/bin/bash
source $S2EDIR/scripts/outrc.sh

if [[ ! -n "$1" ]]; then
    echo "Please provide binary name"
    exit 1
fi

func_data=1
if [[ -n "$2" ]]; then
    func_data=$2
fi

set -e
# this makes the script pwd-agnostic
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=`realpath $DIR/..`

id=$1
outdirbase=s2e-out-$id
outdir=$ROOT/$outdirbase

# remove any old output directory of the same name to avoid conflicts
rm -rf $outdir/*
mkdir -p $outdir
captured_dirs=$(find $ROOT -maxdepth 1 -type d -name "$outdirbase-*" | sort -n)

#create link-ready captured.bc files
header "Creating link-ready captured.bc files.."
for file in $captured_dirs ; do
    #echo $file
    if [[ -d $file ]] ; then
        #echo $file
        #infile=$file/captured.bc
        #outfile=$file/captured-link-ready.bc
        command="cd $file/0 && $S2EDIR/scripts/prep_for_linkage.sh captured.bc captured-link-ready.bc"
        bash ./docker/run $command
    fi
done

#copy one of the captured.bc file to outdir and use it to link with others
header "Linking all the captured.bc files.."
outfile=$outdir/captured.bc
found=0
for file in $captured_dirs ; do
    if [[ -d $file ]] ; then
        #echo $file
        cp $file/0/captured-link-ready.bc $outfile
        found=1
        break
    fi
done

[ $found -ne 1 ] && exit 1

tmp=$(mktemp --suffix .bc)
trap "rm -f $tmp" SIGINT SIGTERM EXIT

for file in $captured_dirs ; do
    if [[ -d $file ]] ; then
        #echo $file
        #llvm-link-3.8 -v -print-all-options -o $tmp -only-needed -suppress-warnings -override=$outfile $file/captured.bc && mv $tmp $outfile
        #bash ./docker/run "ls $S2EDIR/build/llvm/bin"
        command="$S2EDIR/build/llvm/bin/llvm-link -print-after-all -v -o $tmp -override=$outfile $file/0/captured-link-ready.bc && mv $tmp $outfile"
        bash ./docker/run $command
        #mylink -only-needed -v -o $tmp $outfile $file/captured-link-ready.bc && mv $tmp $outfile
        #mylink -v -o $tmp -override=$outfile $file/captured-link-ready.bc && mv $tmp $outfile
        #break
    fi
done
header "Disassembling generated bitcode.."
#mymake -s ${outfile%.bc}.ll
bash ./docker/run "$S2EDIR/build/llvm/bin/llvm-dis $outfile"

#copy one of the succs.dat file to outdir and use it to merge with others
header "Merging traceInfo.json files.."
found=0
traceInfoFiles=""
for file in $captured_dirs ; do
    if [[ -d $file ]] ; then
        traceInfoFiles+=" $file/0/traceInfo.json"
        found=1
    fi
done

[ $found -ne 1 ] && exit 1

$S2EDIR/build/bin/binrec_tracemerge $traceInfoFiles $outdir/traceInfo.json

if [ "$func_data" -eq "1" ] ; then
for file in $captured_dirs ; do
    if [[ -d $file ]] ; then
        echo $file
        #funcbase=$file/func
        for func_file_p in `find $file/0 -maxdepth 1 -name "func_*"` ; do
            echo $func_file_p
            if [[ -f $func_file_p ]] ; then
                func_file_n="$(basename $func_file_p)"
                if [[ -f $outdir/$func_file_n ]] ; then
                    python $ROOT/scripts/merge_funcs.py $outdir/$func_file_n $func_file_p
                else
                    cp $func_file_p $outdir
                fi
            fi
        done
    fi
done
fi

#copy other files(symlinks) necessary for lifting
header "Creating necessary symlink.."
for file in $captured_dirs ; do
    if [[ -d $file ]] ; then
        echo $file
        cp -P $file/0/binary $outdir/binary
        cp -P $file/Makefile $outdir/Makefile
        break
    fi
done
