#!/bin/bash
entry=BB_XXXXXXX
vpc=.bss_XXX
bash ../scripts/deobfuscate-vm.sh captured.bc deobfuscated.bc
bash ../scripts/instrument-vm.sh vars.bc instrumented.bc $entry $vpc
bash ../scripts/trace-vpc.sh ./instrumented XXX > trace
bash ../scripts/untangle-vm.sh vars.bc deobfuscated.bc $entry $vpc trace
