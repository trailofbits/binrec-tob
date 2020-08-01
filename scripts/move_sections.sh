#!/usr/bin/env bash

# use objcopy to add sections, use python script to make loadable segments:
# - compile recovered binary with -Ttext=<fixed (page-aligned) address>
# - read section contents from recovered binary: .text/.rodata/.data
# - add zero-padding after .text equal to vaddr hole between .text and .rodata
# - add zero-padding before .data to align segment vaddr (data_vaddr % 0x1000)
# - use objcopy --add-section on all sections to get file offsets and easily
#   fix the section table + shstrtab section
# - overwrite NOTE segment with loadable RX segment containing .text and .rodata
# - overwrite GNU_EH_FRAME segment with loadable RW segment containing .data
#   and .bss


[ $# -ge 2 ] || { echo "usage: bash $0 INFILE OUTFILE"; exit 1; }
infile=$1
outfile=$2

if [[ "${no_link_lift}" -eq 0 && ! -f dylibs ]] ; then
    echo "file dylibs not found, put the linker flags for libraries you want in file dylibs, aborting"
    exit
fi

cd $(dirname $outfile)
outfile=$(basename $outfile)
rec=_${outfile}_rec

source $S2EDIR/scripts/readelf_util.sh
text_addr=0x9000000

clang=$LLVMBIN/clang
#clang=$S2EDIR/obbuild/bin/clang

# -Ttext=<fixed (page-aligned) address>, is almost an alias for section-start, but not quite
alwaysflags="-g -m32 -Wl,--section-start=.text=$text_addr  "
sometimesflags=" "
#sometimesflags=" -mllvm -fla -mllvm -sub -mllvm -bcf "
endflags=""
beginflags=""

if [ "${no_link_lift}" = "0" ]; then
    sometimesflags+="$(cat dylibs)"
else
    sometimesflags+=" -Wl,--wrap=_init"
    beginflags+="${S2EDIR}/runlib/replaceFuncs.o"
fi
if [ "${PGO_USE}" = "1" ]; then
    sometimesflags+="-fprofile-instr-use=default.profdata "
fi
if [ "${PGO_INSTR}" = "1" ]; then
    sometimesflags+="-fprofile-instr-generate "
fi

#if ! $clang $beginflags  ${outfile}.bc $alwaysflags $sometimesflags $endflags -o $rec; then
#if we need to change compilers for applying transformations, this step should be operating on 
#bc. However, there are some big elf layout problems if this changes to .bc instead of .o
if ! $clang $beginflags  ${outfile}.o $alwaysflags $sometimesflags $endflags -o $rec; then
    echo "compilation failed"
    exit 1
fi

PYTHONPATH=$S2EDIR/runlib \
    python $S2EDIR/scripts/make_loadable_segments.py $outfile $rec ${no_link_lift}

if [ "${no_link_lift}" != "0" ]; then
    patchAddr(){
        sym_name=$1 #typical: main, libc_csu_init,
        offset_from_lcsm=$2 #main=4, __libc_csu_fini=16, _libc_csu_init=11
        # patch address of main being passed to __libc_start_main
        main_addr=$(nm $rec | grep "T ${sym_name}\>" | cut -d' ' -f1)
        #main_addr=$(nm $rec | grep 'T main\>' | cut -d' ' -f1)
            #echo $main_addr
        call_vaddr=$(
            objdump -d $infile |
            grep 'call.*<__libc_start_main@plt>' |
            sed 's/^ *\([a-f0-9]\+\):.*/\1/'
            )
        main_arg_vaddr=$(python -c "print '%x' % (0x$call_vaddr - $offset_from_lcsm)")
        #echo $main_arg_vaddr
        text_vaddr=$(section_vaddr $infile .text)
        text_offset=$(section_offset $infile .text)

        # where to write
        main_arg_offset=$(perl -e "print 0x$main_arg_vaddr - 0x$text_vaddr + 0x$text_offset")
        # where to write + actually write it
        dd conv=swab <<< ${main_addr}0: 2>/dev/null | rev | xxd -r | \
        dd of=$outfile seek=$main_arg_offset bs=4 count=1 \
            conv=notrunc oflag=seek_bytes 2>/dev/null
    }
    patchAddr "main" 4
fi
# remove old mapped sections
# this appears to fuck shit up
#objcopy $remove_flags $outfile $outfile

cp $rec recf
if [ "$fallback_mode" = "unfallback"  ]; then
        #before or after other patching?
            header "patch original code to call trampolines"
            PYTHONPATH=$S2EDIR/runlib \
                python $S2EDIR/scripts/patch_calls.py ${outfile%.bc}
fi
PYTHONPATH=$S2EDIR/runlib \
    python $S2EDIR/scripts/patch_stuff.py $outfile $rec ${no_link_lift}
PYTHONPATH=$S2EDIR/runlib \
    python $S2EDIR/scripts/patch_calls.py ${outfile%.bc}
echo "run as: ./$outfile"

# cleanup
ret=$?
[ "$KEEP" = "" ] && rm _*
exit $ret
