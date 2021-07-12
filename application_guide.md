Application Guide
===================================================================

SPEC
--------

 SPEC2006 tracing instructions. Be familiar with lifting a program first, such as test/hello
        
        $ # Install spec2006 at $S2EDIR/test/spec2006. Else, in $S2EDIR/plugins/config.debian.lua comment the paths
        which are used to find the binaries
        $ # Use the network (or some other way) to copy all the desired spec inputs into your qemu vm. I.e. inside your qemu image, scp host:/path/to/spec/inputs . 
        $ # Use  getrun-cmd to take a snapshot to begin tracing, invoke getrun-cmd in the directory where you put the spec inputs. 
        $ # Use the specConfTest or specConfRelease config file as the argument to cmd-debian-mt.sh
        

Using our prelifted spec traces
    1. Extract the folders s2e-out-\* into your S2E_DIR
    2. merge the traces \*-{1-N}. Note, these were lifted in an old folder format so do the following
        $ go to S2E_DIR on your host system **NOT in docker**
        $ source scripts/env.sh
        $ ./scripts/merge_captured_dirs_old.sh <binary name>
    3. return to your docker environment by invoking 
        $ ./docker/run bash
    4. use lifting and lowering scripts as usual
        $ cd s2e-out-<binary name>
        $ ../scripts/lift.sh captured.bc lifted.bc
        $ 
        $ ../scripts/optimizeOnlyLower.sh lifted.bc opt.bc
        $ ./opt <input>

Incremental Lifting
--------

(Current) Incremental lifting works from the address where the base binary switched to fallback till end of binary.

    $ # First argument is the address of the BB identifier of the predecessor block
    $ # Second argument is the PC value from where we would like to start tracing
    $ # Third argument - specBin has location of the binary we want to lift
    $ # Working on understanding 4th arg
    $ ./qemu/cmd-debian-bb.sh "804842c" "8048463" $specBin 0
    $ # Run script to merge required traces
    $ ./scripts/merge_captured_dirs.sh $specBin 0



De-Virtualization
----------------------------


test programs: test/vm.c, test/*-vm.c

1. Compile your test binary, for example from eq-vm.c
2. Follow the normal procedure for tracing and merging.
3. Refer to and execute scripts/deobfuscate-vm.sh. You may find scripts/deobfuscate-skeleton.sh also useful as a higher level wrapper around deobfuscate-vm.sh once you understand the workflow.

ASAN
----------------------------

test programs: test/heapover.cpp, test/uaf.cpp, test/stackoob.cpp

1. Follow the normal procedure including tracing, merging and lifting. 
2. Add the compiler flag -fsanitize=address in scripts/move_sections.sh line 38 

Safe-Stack
----------------------------

1. Follow the normal procedure including tracing, merging and lifting. 
2. Add the compiler flag -fsanitize=safe-stack in scripts/move_sections.sh line 38 

