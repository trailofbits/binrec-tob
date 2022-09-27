# Advanced Program Debloating Features

In this walkthrough, we will explore some of BinRec's other useful interface features that can be used during debloating. Each subsection describes a situation in which you would use a given feature.

## Re-using a campaign file

1. When creating a new project, you can optionally specify a campaign file to initialize the project with (as opposed to the default empty file). This is useful if you want to share campaigns between workstations or your new project closely resembles an existing one.

   ```bash
   # just new-project <project_name> <path_to_target_binary> <path_to_campaign_file>
   $ just new-project eq2proj ./test/benchmark/samples/bin/x86/binrec/eq2 ./s2e/projects/eqproj/campaign.json

   # Recover the eq2 binary with the same settings and trace arguments as used in the `eqproj` project
   $ just recover eq2proj
   ```

## Running recovery steps individually

1. The `just recover` command runs BinRec from end-to-end, however there are some situations in which running the stages individually is useful. Running `just recover` is equivalent to running the following commands in sequence:

   ```bash
   # just run <project_name>
   $ just run eq2proj

   # just merge-traces <project_name>
   $ just merge-traces eq2proj

   # just lift-trace <project_name>
   $ just lift-trace eq2proj

   # just validate <project_name> <concrete_args>
   $ just validate-args eq2proj 1 2
   $ just validate-args eq2proj 2 2

   # Note: the validation step is done for all traces consisting of concrete inputs in the campaign file.
   ```

## Adding trace arguments and re-running a project

1. When using BinRec, you may discover that your first attempt at specifying functionality to recover fell short. BinRec supports incremental tracing without re-collecting previous traces. To add a trace to a project that has already been recovered, first add the new set of trace arguments as usual:

   ```bash
   # just add-trace <project_name> <trace_name> <symbolic-indexes> <args>
   $ just add-trace eq2proj tr3 "" "a b"
   ```

2. Then use the `just run-trace` command with the `--last` flag (or the trace name) to collect a trace with the new set of arguments:

   ```bash
   # just run <project_name> <--last>
   $ just run-trace eq2proj --last

   # Or:

   # just run <project_name> <--trace> <trace_name>
   $ just run-trace eq2proj tr3
   ```
3. Then, re-run the rest of the recovery process in sequence:

   ```bash
   # just merge-traces <project_name>
   $ just merge-traces eq2proj

   # just lift-trace <project_name>
   $ just lift-trace eq2proj

   # just validate <project_name> <concrete_args>
   $ just validate eq2proj a b
   ```

## Removing trace arguments and re-running a project

1. When using BinRec, you may discover that your first attempt at specifying functionality to recover included too many functions. BinRec supports clearing the collected traces and modifying the campaign file for a second recovery attempt. This process avoids needing to set up a new project. To remove sets of trace arguments and re-run recovery, first clear out the existing trace data:

   ```bash
   # just clear-trace-data <project_name>
   $ just clear-trace-data eq2proj
   ```

2. Next, remove the desired traces using the `just remove-trace` command (or the `just remove-all-traces` command if you want to start over from scratch):

   ```bash
   # just delete-trace <project_name> <trace_name>
   $ just remove-trace eq2proj tr3

   # Or:

   # just delete-all-traces <project_name>
   $ just remove-all-traces eq2proj
   ```

3. Then add any traces as desired as shown in the previous section. Finally, re-run the recovery process:

   ```bash
   # just recover <project_name>
   $ just recover eq2proj
   ```
