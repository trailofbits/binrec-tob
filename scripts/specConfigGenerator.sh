#!/bin/bash

tail -q -n 1 $S2EDIR/test/spec2006/benchspec/CPU2006/*/run/run_base_test_pentium32-gcc10-O0.0000/speccmds.cmd | cut -d ' ' -f 1-4 --complement | cut -d '/' -f 3- > conf
