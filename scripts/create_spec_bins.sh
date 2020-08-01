#!/bin/bash

set -e
cd $S2EDIR
SPECBINS=$HOME/recovered-spec-bins-newIMGnewS2E
RECOVEREDDIRS=$HOME/recovered-spec-dirs-newIMGnewS2E
mkdir -p $SPECBINS
mkdir -p $RECOVEREDDIRS

#bash ./docker/run 'export MEM_AMOUNT=64; echo $MEM_AMOUNT; lscpu'

lift(){
    echo ">> Lifting $@..."
    bash ./docker/run "./qemu/cmd-debian.sh --vnc $@"
}

recoverFBOffO0(){
    command="cd $S2EDIR/s2e-out-$1 &&  ../scripts/doNotOptimize.sh -f none captured.bc $1-recovered-FBOff-O0-$2.bc"
    bash ./docker/run $command
    cp $S2EDIR/s2e-out-$1/$1-recovered-FBOff-O0-$2 $SPECBINS    
    cp -r $S2EDIR/s2e-out-$1 $RECOVEREDDIRS/s2e-out-$1-recovered-FBOff-O0-$2
}

recoverFBOnO0(){
    command="cd $S2EDIR/s2e-out-$1 &&  ../scripts/doNotOptimize.sh captured.bc $1-recovered-FBOn-O0-$2.bc"
    bash ./docker/run $command
    cp $S2EDIR/s2e-out-$1/$1-recovered-FBOn-O0-$2 $SPECBINS
    cp -r $S2EDIR/s2e-out-$1 $RECOVEREDDIRS/s2e-out-$1-recovered-FBOn-O0-$2
}

recoverFBOffO3(){
    command="cd $S2EDIR/s2e-out-$1 &&  ../scripts/optimize.sh -f none captured.bc $1-recovered-FBOff-O3-$2.bc"
    bash ./docker/run $command
    cp $S2EDIR/s2e-out-$1/$1-recovered-FBOff-O3-$2 $SPECBINS
    cp -r $S2EDIR/s2e-out-$1 $RECOVEREDDIRS/s2e-out-$1-recovered-FBOff-O3-$2
}

recoverFBOnO3(){
    command="cd $S2EDIR/s2e-out-$1 &&  ../scripts/optimize.sh captured.bc $1-recovered-FBOn-O3-$2.bc"
    bash ./docker/run $command
    cp $S2EDIR/s2e-out-$1/$1-recovered-FBOn-O3-$2 $SPECBINS
    cp -r $S2EDIR/s2e-out-$1 $RECOVEREDDIRS/s2e-out-$1-recovered-FBOn-O3-$2
}


#lift sha2 ABC 4
#recoverFBOffO0 sha2 ABC-4
#recoverFBOnO0 sha2 ABC-4
#recoverFBOffO3 sha2 ABC-4
#recoverFBOnO3 sha2 ABC-4


#lift hello
#recoverFBOffO0 hello

#_BZIP2_O0_###################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=input.source
#lift --file $input $specBin $input 280
#recoverFBOffO0 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=chicken.jpg
#lift --file $input $specBin $input 30
#recoverFBOffO0 $specBin $input-30
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=liberty.jpg
#lift --file $input $specBin $input 30
#recoverFBOffO0 $specBin $input-30
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=input.program
#lift --file $input $specBin $input 280
#recoverFBOffO0 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=text.html
#lift --file $input $specBin $input 280
#recoverFBOffO0 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=input.combined
#lift --file $input $specBin $input 200
#recoverFBOffO0 $specBin $input-200
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
#
#_BZIP2_O3_####################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=input.source
#lift --file $input $specBin $input 280
#recoverFBOffO3 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=chicken.jpg
#lift --file $input $specBin $input 30
#recoverFBOffO3 $specBin $input-30
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=liberty.jpg
#lift --file $input $specBin $input 30
#recoverFBOffO3 $specBin $input-30
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=input.program
#lift --file $input $specBin $input 280
#recoverFBOffO3 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=text.html
#lift --file $input $specBin $input 280
#recoverFBOffO3 $specBin $input-280
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#specBin=bzip2_base.gcc4.8.4-O3-32bit
#input=input.combined
#lift --file $input $specBin $input 200
#recoverFBOffO3 $specBin $input-200
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input/* $S2EDIR/s2e-out-$specBin
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input/* $S2EDIR/s2e-out-$specBin
#
#_MCF_O0_######################################################################################
#specBin=mcf_base.gcc4.8.4-O0-32bit
#input=inp.in
#lift --file $input $specBin $input
#recoverFBOffO0 $specBin $input
#cp $S2EDIR/test/spec2006/benchspec/CPU2006/429.mcf/data/ref/input/* $S2EDIR/s2e-out-$specBin
###############################################################################################
#
#_MCF_O3_######################################################################################
#specBin=mcf_base.gcc4.8.4-O3-32bit
#input=inp.in
#lift --file $input $specBin $input
#recoverFBOffO3 $specBin $input
###############################################################################################
#
#_HMMER_O0_####################################################################################
#specBin=hmmer_base.gcc4.8.4-O0-32bit
#input1=nph3.hmm
#input2=swiss41
#lift --file1 $input1 --file2 $input2 $specBin $input1 $input2
#recoverFBOffO0 $specBin $input1-$input2
###############################################################################################
#specBin=hmmer_base.gcc4.8.4-O0-32bit
#input1=retro.hmm
#input2="--fixed 0 --mean 500 --num 500000 --sd 350 --seed 0"
#lift --file1 $input1 $specBin --fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 $input1
#recoverFBOffO0 $specBin $input2-$input1
###############################################################################################
#
#_HMMER_O3_####################################################################################
#specBin=hmmer_base.gcc4.8.4-O3-32bit
#input1=nph3.hmm
#input2=swiss41
#lift --file1 $input1 --file2 $input2 $specBin $input1 $input2
#recoverFBOffO3 $specBin $input1-$input2
###############################################################################################
#specBin=hmmer_base.gcc4.8.4-O3-32bit
#input1=retro.hmm
#input2="--fixed 0 --mean 500 --num 500000 --sd 350 --seed 0"
#lift --file1 $input1 $specBin --fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 $input1
#recoverFBOffO3 $specBin $input2-$input1
###############################################################################################
#
#_SJENG_O0_####################################################################################
#specBin=sjeng_base.gcc4.8.4-O0-32bit
#input1=ref.txt
#lift --file1 $input1 $specBin $input1
#recoverFBOffO0 $specBin $input1
###############################################################################################
#
#_SJENG_O3_####################################################################################
#specBin=sjeng_base.gcc4.8.4-O3-32bit
#input1=ref.txt
#lift --file1 $input1 $specBin $input1
#recoverFBOffO3 $specBin $input1
###############################################################################################
#
#_LIBQUANTUM_O0_###############################################################################
specBin=libquantum_base.gcc4.8.4-O0-32bit
lift $specBin 1397 8
recoverFBOffO0 $specBin 1397-8
###############################################################################################
#
#_LIBQUANTUM_O3_###############################################################################
#specBin=libquantum_base.gcc4.8.4-O3-32bit
#lift $specBin 1397 8
#recoverFBOffO3 $specBin 1397-8
###############################################################################################
#
#_464.H264REF_O0_##############################################################################
#specBin=h264ref_base.gcc4.8.4-O0-32bit
#input1=foreman_ref_encoder_baseline.cfg
#input2=foreman_qcif.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=h264ref_base.gcc4.8.4-O0-32bit
#input1=foreman_ref_encoder_main.cfg
#input2=foreman_qcif.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=h264ref_base.gcc4.8.4-O0-32bit
#input1=sss_encoder_main.cfg
#input2=sss.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO0 $specBin $input1
###############################################################################################
#
#_464.H264REF_O3_##############################################################################
#specBin=h264ref_base.gcc4.8.4-O3-32bit
#input1=foreman_ref_encoder_baseline.cfg
#input2=foreman_qcif.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO3 $specBin $input1
###############################################################################################
#specBin=h264ref_base.gcc4.8.4-O3-32bit
#input1=foreman_ref_encoder_main.cfg
#input2=foreman_qcif.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO3 $specBin $input1
###############################################################################################
#specBin=h264ref_base.gcc4.8.4-O3-32bit
#input1=sss_encoder_main.cfg
#input2=sss.yuv
#lift --file1 $input1 --file2 $input2 $specBin -d $input1
#recoverFBOffO3 $specBin $input1
###############################################################################################
#
#_473.ASTAR_O0_################################################################################
#specBin=astar_base.gcc4.8.4-O0-32bit
#input1=BigLakes2048.cfg
#input2=BigLakes2048.bin
#lift --file1 $input1 --file2 $input2 $specBin $input1
#recoverFBOffO0 $specBin $input1
#recoverFBOnO0 $specBin $input1
#recoverFBOffO3 $specBin $input1
#recoverFBOnO3 $specBin $input1
###############################################################################################
#specBin=astar_base.gcc4.8.4-O0-32bit
#input1=rivers.cfg
#input2=rivers.bin
#lift --file1 $input1 --file2 $input2 $specBin $input1
#recoverFBOffO0 $specBin $input1
###############################################################################################
#
#_473.ASTAR_O3_################################################################################
#specBin=astar_base.gcc4.8.4-O3-32bit
#input1=BigLakes2048.cfg
#input2=BigLakes2048.bin
#lift --file1 $input1 --file2 $input2 $specBin $input1
#recoverFBOffO3 $specBin $input1
#recoverFBOnO3 $specBin $input1
###############################################################################################
#specBin=astar_base.gcc4.8.4-O3-32bit
#input1=rivers.cfg
#input2=rivers.bin
#lift --file1 $input1 --file2 $input2 $specBin $input1
#recoverFBOffO3 $specBin $input1
###############################################################################################
#
#_445.GOBMK_O0_################################################################################
#specBin=gobmk_base.gcc4.8.4-O0-32bit
#input1=13x13.tst
#lift --file1 $input1 $specBin < $input1 --quiet --mode gtp
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=gobmk_base.gcc4.8.4-O0-32bit
#input1=nngs.tst
#lift --file1 $input1 $specBin < $input1 --quiet --mode gtp
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=gobmk_base.gcc4.8.4-O0-32bit
#input1=score2.tst
#lift --file1 $input1 $specBin < $input1 --quiet --mode gtp
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=gobmk_base.gcc4.8.4-O0-32bit
#input1=trevorc.tst
#lift --file1 $input1 $specBin < $input1 --quiet --mode gtp
#recoverFBOffO0 $specBin $input1
###############################################################################################
#specBin=gobmk_base.gcc4.8.4-O0-32bit
#input1=trevord.tst
#lift --file1 $input1 $specBin < $input1 --quiet --mode gtp
#recoverFBOffO0 $specBin $input1
###############################################################################################
#
#_400.PERLBENCH_O0_##############################################################################
#specBin=perlbench_base.gcc4.8.4-O0-32bit
#lift $specBin -I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1
#recoverFBOffO0 $specBin checkspam.pl
#recoverFBOnO0 $specBin checkspam.pl
#recoverFBOffO3 $specBin checkspam.pl
#recoverFBOnO3 $specBin checkspam.pl
###############################################################################################
#specBin=perlbench_base.gcc4.8.4-O0-32bit
#lift $specBin -I./lib diffmail.pl 4 800 10 17 19 300
#recoverFBOffO0 $specBin diffmail.pl
#recoverFBOnO0 $specBin diffmail.pl
#recoverFBOffO3 $specBin diffmail.pl
#recoverFBOnO3 $specBin diffmail.pl
###############################################################################################
#specBin=perlbench_base.gcc4.8.4-O0-32bit
#lift $specBin -I./lib splitmail.pl 1600 12 26 16 4500
#recoverFBOffO0 $specBin splitmail.pl
#recoverFBOnO0 $specBin splitmail.pl
#recoverFBOffO3 $specBin splitmail.pl
#recoverFBOnO3 $specBin splitmail.pl
###############################################################################################
#
#_400.PERLBENCH_O3_##############################################################################
#specBin=perlbench_base.gcc4.8.4-O3-32bit
#lift $specBin -I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1
#recoverFBOffO3 $specBin checkspam.pl
#recoverFBOnO3 $specBin checkspam.pl
###############################################################################################
#specBin=perlbench_base.gcc4.8.4-O3-32bit
#lift $specBin -I./lib diffmail.pl 4 800 10 17 19 300
#recoverFBOffO3 $specBin diffmail.pl
#recoverFBOnO3 $specBin diffmail.pl
###############################################################################################
#specBin=perlbench_base.gcc4.8.4-O3-32bit
#lift $specBin -I./lib splitmail.pl 1600 12 26 16 4500
#recoverFBOffO3 $specBin splitmail.pl
#recoverFBOnO3 $specBin splitmail.pl
###############################################################################################
#
#_471.OMNETPP_O0_##############################################################################
#specBin=omnetpp_base.gcc4.8.4-O0-32bit
#input1=omnetpp.ini
#lift $specBin $input1
#recoverFBOffO0 $specBin $input1
#recoverFBOnO0 $specBin $input1
#recoverFBOffO3 $specBin $input1
#recoverFBOnO3 $specBin $input1
###############################################################################################
#
#_471.OMNETPP_O3_##############################################################################
#specBin=omnetpp_base.gcc4.8.4-O3-32bit
#input1=omnetpp.ini
#lift $specBin $input1
#recoverFBOffO3 $specBin $input1
#recoverFBOnO3 $specBin $input1
###############################################################################################


