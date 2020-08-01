#!/bin/bash

#!/bin/bash

set -e
#cd $S2EDIR
SPECBINS=$HOME/recovered-spec-bins-llvm-3.8-spec2006Branch
#RECOVEREDDIRS=$HOME/recovered-spec-dirs-newIMGnewS2E
mkdir -p $SPECBINS
#mkdir -p $RECOVEREDDIRS

#bash ./docker/run 'export MEM_AMOUNT=64; echo $MEM_AMOUNT; lscpu'

lift(){
    echo ">> Lifting $@..."
    bash ./docker/run "export MEM_AMOUNT=3072 && ./qemu/cmd-debian.sh --vnc $@"
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

recoverFBOffO3(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && ../scripts/optimize.sh -f none captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
    cp $S2EDIR/$1/${bin}_orig $SPECBINS
}
recoverFBErrorO3(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && ../scripts/optimize.sh -f error captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
    cp $S2EDIR/$1/${bin}_orig $SPECBINS
}

recoverFBOnO3(){
    bin=${dir##*s2e-out-}
    command="cd $S2EDIR/$1 && linux32 ../scripts/optimize.sh captured.bc $bin.bc"
    bash ./docker/run $command
    cp $S2EDIR/$1/$bin $SPECBINS
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
#lift $specBin $input 280
#recoverFBOffO0 $specBin $input-280
#recoverFBOnO0 $specBin $input-280
#recoverFBOffO3 $specBin $input-280
#recoverFBOnO3 $specBin $input-280


relink(){
for dir in s2e-out-*-recovered-*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   path=$(readlink -f $dir/binary)
   echo $path
   newPath=$(echo $path | sed 's@binrec-spec2006-.*/test/@binrec-llvm-3.8-spec2006-0/test/@g')
   ln -sf $newPath $dir/binary
   path=$(readlink -f $dir/binary)
   echo $path

   path=$(readlink -f $dir/Makefile)
   echo $path
   newPath=$(echo $path | sed 's@binrec-spec2006-.*/scripts/@binrec-llvm-3.8-spec2006-0/scripts/@g')
   ln -sf $newPath $dir/Makefile
   path=$(readlink -f $dir/Makefile)
   echo $path
done
}

relink
echo "################################################"

for dir in s2e-out-*-O0-*-FBOff-O0*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOffO0 $dir
done

for dir in s2e-out-*-O0-*-FBOn-O0*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOnO0 $dir
done
for dir in s2e-out-*-O0-*-FBOff-O3*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOffO3 $dir
done
for dir in s2e-out-*-O0-*-FBOn-O3*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOnO3 $dir
done

for dir in s2e-out-*-O3-*-FBOff-O3*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOffO3 $dir
done
for dir in s2e-out-*-O3-*-FBError-O3*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBErrorO3 $dir
done

for dir in s2e-out-*-O3-*-FBOn-O3*/;
do 
   dir=${dir%*/}
   echo ${dir##*/}
   recoverFBOnO3 $dir
done

