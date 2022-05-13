# Using BinRec to Debloat a Target Program

In this short walkthrough, we take you through a sample BinRec project: debloating a program binary. Our goal in the walkthrough is to exercise the target binary with a set of concrete and/or symbolic inputs. BinRec will trace the execution of the program on these inputs,
merge the traces, lift the merged trace to LLVM IR, and then recompile the IR to a debloated program. 

1. Create a new analysis project by specifying the target binary, arguments to treat as symbolic, and the first set of concrete inputs:

   ```bash
   # just new-project <project_name> <path_to_target_binary> <sym_args> <concrete_args_list>
   
   # Concrete arguments only (sym_args = "")
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq "" 3 2

   # OR

   # First argument (3) is a symbolic argument
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq "1" 3 2

   # OR

   # Both arguments (3 and 2) are symbolic argument
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq "1 2" 3 2

   ```
 
   This will create a project folder located at `s2e/projects/eqproj/` where traces and output associated with this project will be kept.

2. Run the project:

   ```bash  
   # just run <project_name>
   $ just run eqproj
   ```

3. For each additional set of concrete or symbolic inputs to trace, re-run the project with new arguments:

   ```bash
   # just set-args <project_name> <concrete_args_list>
   # just run <project_name>
   $ just set-args eqproj  "" 2 2
   $ just run eqproj
   
   # OR

   # just run <project_name> --args <concrete_args_list>
   just run eqproj --args "" 0 10
   ```

4. Next, merge the traces, optionally specifying the specific trace numbers to merge:

   ```bash 
   # just merge-traces <project_name>
   $ just merge-traces eqproj
   ```


5. Finally, lift the merged trace:

   ```bash
   # just lift-tace <project_name>
   $ just lift-trace eqproj
   ```

   This will create a `s2e-out` subdirectory in the project folder. It contains the debloated program binary (`recovered`) as well the binary's associated LLVM IR (`recovered.bc` and `recovered.ll`).

6. To validate the debloated program outputs match the original:

   ```bash
   # just validate <project_name> <concrete_args>
   $ just validate eqproj 3 2

   $ just validate eqproj 2 2

   $ just validate eqproj 0 10
   ```

   **NOTE:** At this time, validation only supports equivalence checks for `stdout`, `stderrr`, and the return code.

7. To validate program behaviors other than outputs to `stdout`, `stderr`, and the return code (e.g., performance, file outputs), manually compare the behaviors of programs in the `s2e-out` project folder:

   ```bash
      $ cd s2e/projects/eqproj/s2e-out

      # Observe original behavior
      $ ./binary 1 2

      # Manually ensure recovered binary behavior matches
      $ ./recovered 1 2
   ```