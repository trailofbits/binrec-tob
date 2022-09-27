# Using BinRec to Debloat a Target Program

In this short walkthrough, we take you through a sample BinRec project: debloating a program binary. Our goal in the walkthrough is to exercise the target binary with a set of concrete and/or symbolic inputs. BinRec will trace the execution of the program on these inputs,
merge the traces, lift the merged trace to LLVM IR, and then recompile the IR to a debloated program.

1. Create a new analysis project by specifying the target binary

   ```bash
   # just new-project <project_name> <path_to_target_binary>
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq
   ```

   This will create a project folder located at `s2e/projects/eqproj/` where project settings, trace data, outputs, and other resources associated with this project will be kept.

2. Add one or more sets of arguments to invoke the target binary with during tracing. These are recorded in the project settings file (`campaign.json`) in the project folder. Adding a set of arguments requires three components: a name for the trace, a space-separated list of
   of argument numbers that should be considered symbolic, and a space-separated set of concrete arguments.

   Here's an example with only concrete arguments:

   ```bash
   # just add-trace <project_name> <trace_name> <symbolic-indexes> <args>
   $ just add-trace eqproj tr1 "" "1 2"
   $ just add-trace eqproj tr2 "" "2 2"
   ```

   Note in this case we leave the list of symbolic argument numbers empty (i.e., `""`).


   Here's an example using symbolic arguments:

   ```bash
   # just add-trace <project_name> <trace_name> <symbolic-indexes> <args>
   $ just add-trace eqproj tr1 "1" "3 2"
   ```

   Note in this case to mark the first argument (i.e., the value `3`) as a symbolic argument, we provide the list `"1"` for our `symbolic-indexes`. During tracing, BinRec will ignore the actual value `3` and treat this argument as a wildcard. However, the valuer `3` will be used during validation (covered in Step 4).


3. Run the recovery process on the project:

   ```bash
   # just recover <project_name>
   $ just recover eqproj
   ```

   This will execute the target binary with each set of concrete arguments and collect execution traces of each. It will then merge the two traces, lift the result, and recompile it to a recovered binary.

   The recovered binary is located in the `s2e-out` subdirectory in the project folder. In addition to the debloated program binary (`recovered`), the folder also contains the binary's associated LLVM IR (`recovered.bc` and `recovered.ll`).

   The recovery process also validates that the recovered binary's output matches the original binary's with respect to the provided concrete arguments.

   The `BINREC_DEBUG` environment variable can be used to increase the verbosity of log messages. For example, to recover the project with more logging enabled:

   ```bash
   $ BINREC_DEBUG=1 just recover eqproj
   ```

4. To validate the debloated program outputs match the original for aditional inputs:

   ```bash
   # just validate <project_name> <concrete_args>
   $ just validate-args eqproj 3 2

   $ just validate-args eqproj 2 2

   $ just validate-args eqproj 0 10
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
