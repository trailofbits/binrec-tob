#!/bin/bash

#!/bin/bash

set -e
#cd $S2EDIR
SPECBINS=$HOME/recovered-spec-bins-newIMGnewS2E-3.8
#RECOVEREDDIRS=$HOME/recovered-spec-dirs-newIMGnewS2E
mkdir -p $SPECBINS
#mkdir -p $RECOVEREDDIRS

#bash ./docker/run 'export MEM_AMOUNT=64; echo $MEM_AMOUNT; lscpu'

makeTest(){
    echo ">> build tests"
    bash ./docker/run "cd test && make"
}
lift(){
    echo ">> Lifting $@..."
    bash ./docker/run "export MEM_AMOUNT=2048 && ./qemu/cmd-debian.sh --vnc $@"
}
lift-mt(){
    echo ">> Lifting $@..."
    #length=$(($#-1))
    #array=${@:1:$length}
    bash ./docker/run "export MEM_AMOUNT=2048 && ./qemu/cmd-debian-mt.sh $@"
}
lift-bb(){
    echo ">> Lifting BB_$2 ..."
    bash ./docker/run "export MEM_AMOUNT=2048 && ./qemu/cmd-debian-bb.sh $@"
}

recoverFBOffO0(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && ../scripts/doNotOptimize.sh -f none captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
}

recoverFBOnO0(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && ../scripts/doNotOptimize.sh captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
}

recoverFBErrorO0(){
    bin=$1-recovered-FBError-O0-$2
    command="cd $S2EDIR/s2e-out-$1 && ../scripts/doNotOptimize.sh -f error -l debug captured.bc $bin.bc"
#    command="cd $S2EDIR/s2e-out-$1 && ../scripts/lift.sh -f error -l debug captured.bc $bin.bc"
    bash ./docker/run $command
    #cp $S2EDIR/$1/$bin $SPECBINS
}
recoverFBErrorO3(){
    bin=$1-recovered-FBError-O3-$2
    command="cd $S2EDIR/s2e-out-$1 && ../scripts/optimize.sh -f none -l debug captured.bc $bin.bc"
    bash ./docker/run $command
    #cp $S2EDIR/$1/$bin $SPECBINS
}
recoverFBOffO3(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && ../scripts/optimize.sh -f none captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
}

recoverFBOnO3(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && linux32 ../scripts/optimize.sh captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
}

#_LIBQUANTUM_O0_###############################################################################
#specBin=libquantum_base.gcc4.8.4-O0-32bit
#lift $specBin 33 5
#lift $specBin 23 5
#lift $specBin 13 5
###############################################################################################
#
#_LIBQUANTUM_O3_###############################################################################
#specBin=libquantum_base.gcc4.8.4-O3-32bit
#lift $specBin 1397 8
#recoverFBErrorO3 $specBin 1397-8
###############################################################################################

#lift fibit --concolic 3
#lift sha2 anil
#lift sha2 anil 3
#lift sha2 caner 3
#lift-mt bins.conf14 3
#specBin=hmmer_base.gcc4.8.4-O3-32bit
#specBin=omnetpp_base.gcc4.8.4-O0-32bit
#lift hello
#./scripts/merge_captured_dirs.sh eq2 0
#recoverFBErrorO0 eq
#recoverFBErrorO3 hello

#lift eq2 a b
#lift eq2 a a
#lift 1 eq a b 
#lift 2 complex_double 
#lift 2 complex_long_double
#makeTest
#specBin=ArdaTest
#specBin=gcc_base.gcc4.8.4-O3-64bit
#lift $specBin 166.in -o 166.s
#lift $specBin 0 0 
#./scripts/merge_captured_dirs.sh $specBin
#recoverFBErrorO0 $specBin
#recoverFBErrorO0 sha2 anil-3

#lift sha2 ABC 4
#recoverFBOffO0 sha2 ABC-4
#recoverFBOnO0 sha2 ABC-4
#recoverFBOffO3 sha2 ABC-4
#recoverFBOnO3 sha2 ABC-4

#specBin=omnetpp_base.gcc4.8.4-O3-32bit
#input1=omnetpp.ini
#lift $specBin $input1
specBin=eq
lift-bb "804842c" 8048463 $specBin a b
./scripts/merge_captured_dirs.sh $specBin 0
recoverFBErrorO0 $specBin
#recoverFBErrorO3 $specBin

#recoverFBOffO3 $specBin $input1
#recoverFBOnO3 $specBin $input1

#lift hello
#recoverFBOffO0 hello

#_BZIP2_O0_###################################################################################
#specBin=bzip2_base.gcc4.8.4-O0-32bit
#input=input.source
#lift $specBin $input 280
#recoverFBOffO0 $specBin $input-280
#recoverFBOnO0 $specBin $input-280
#recoverFBOffO3 $specBin $input-280
#recoverFBOnO3 $specBin $input-280

#relink(){
#for dir in s2e-out-*-recovered-*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   path=$(readlink -f $dir/binary)
#   echo $path
#   newPath=$(echo $path | sed 's@spec-workspace-.*/test/@spec-workspace-0123456789-0/test/@g')
#   ln -sf $newPath $dir/binary
#   path=$(readlink -f $dir/binary)
#   echo $path
#done
#}
#
#relink
#echo "################################################"
#
#for dir in s2e-out-*-O0-*-FBOff-O0*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOffO0 $dir
#done
#
#for dir in s2e-out-*-O0-*-FBOn-O0*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOnO0 $dir
#done
#for dir in s2e-out-*-O0-*-FBOff-O3*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOffO3 $dir
#done
#for dir in s2e-out-*-O0-*-FBOn-O3*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOnO3 $dir
#done
#
#for dir in s2e-out-*-O3-*-FBOff-O3*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOffO3 $dir
#done
#
#for dir in s2e-out-*-O3-*-FBOn-O3*/;
#do 
#   dir=${dir%*/}
#   echo ${dir##*/}
#   recoverFBOnO3 $dir
#done
#
