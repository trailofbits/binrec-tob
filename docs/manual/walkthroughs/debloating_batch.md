# Using BinRec to Debloat a Program with Batch Files

In this short walkthrough, we once again take you through a BinRec debloating project. However, this time we will use batch files to automate tracing and validation. 

As before, our goal in the walkthrough is to exercise the target binary with a set of concrete and/or symbolic inputs. BinRec will trace the execution of the program on these inputs, merge the traces, lift the merged trace to LLVM IR, and then recompile the IR to a debloated program. 

1. Create a new analysis project by specifying the target binary, no symbolic arguments, and a set of placeholder concrete inputs (can be anything, they will not be used):

   ```bash
   # just new-project <project_name> <path_to_target_binary> <sym_args> <concrete_args_list>
   
   # Concrete arguments only (sym_args = "")
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq "" 0 0
   ```
 
   This will create a project folder located at `s2e/projects/eqproj/` where traces and output associated with this project will be kept.

2. Create a new text file with a distinct input to be traced on each line. Each input should consist of both the symbolic arguments (if any) and the concrete argument values. For example, the following text file:  

   ```
   "1" 3 2
   "" 2 2
   "" 0 10
   ```

   Indicates three traces of the target binary should be collected. In the first, the first argument should be treated as symbolic, and the second argument is the concrete value `2`. In the second and third traces, there are no symbolic arguments, only concrete values.

3. Next, run the project with specified batch file:

   ```bash
   # just run-batch <project_name> <batch_filename>
   $ just run-batch eqproj eq_inputs.txt
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

6. To validate the debloated program outputs match the original using the same batch file used during tracing:

   ```bash
   # just validate-batch <project_name> <batch_filename> --skip-first
   $ just validate-batch eqproj eq_inputs.txt --skip-first
   ```

   **NOTE 1:** The `--skip-first` flag is used here to tell BinRec to ignore the first entry in each line. When re-using the tracing batch file, the first entry indicates symbolic argument indices, not an actual program argument. 
   
   **NOTE 2:** At this time, validation only supports equivalence checks for `stdout`, `stderrr`, and the return code.

7. To validate using batch files that were not used for tracing, such as the following:

    ```
    1 2
    40 2
    -1 -1
    ```

    omit the `--skip-first flag:

    ```bash
   # just validate-batch <project_name> <batch_filename> 
   $ just validate-batch eqproj new_eq_inputs.txt
   ```


8. To validate program behaviors other than outputs to `stdout`, `stderr`, and the return code (e.g., performance, file outputs), manually compare the behaviors of programs in the `s2e-out` project folder:

   ```bash
      $ cd s2e/projects/eqproj/s2e-out

      # Observe original behavior
      $ ./binary 1 2

      # Manually ensure recovered binary behavior matches
      $ ./recovered 1 2
   ```

    **NOTE:** Validating behaviors other than outputs to `stdout`, `stderr`, and the return code cannot be batched.