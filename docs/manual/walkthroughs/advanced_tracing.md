# Advanced Program Tracing Features

In this short walkthrough, we demonstrate some advanced execution tracing features by way of a debloating project. Our goal in this project is to recover a simple server binary.

To successfully trace this binary during execution, we will need to specify files other than the server binary that BinRec must place in the emulated environment and provide BinRec with pre- and post-tracing scripting to orchestrate a request to the server. Specifically, we will need to include a client binary to make a request to the server and a setup script to orchestrate the tracing.

1. As with our previous examples, we start by creating a new project:

   ```bash
   # just new-project <project_name> <path_to_target_binary>
   $ just new-project serverproj test/benchmark/samples/bin/x86/binrec/server
   ```

2. Next, let's add a trace to be collected. In this case, there are no CLI arguments required to start the server:

   ```bash
   # just add-trace <project_name> <trace_name> <symbolic_args> <args>
   $ just add-trace serverproj tr1 "" ""
   ```

3. Now, we will need to add the additional input files required by `server`. We need to add two files. The first is a compatible `client` binary that can externally trigger the functionality we want to trace in the `server` binary. The second is a short shell script `client.sh` that when executed will pause to allow the `server` binary to startup and then run the `client` binary.

   Run the following just commands to add these two input files to the `tr1` trace.

   ```bash
   # just add-trace-input-file <project_name> <trace_name> <source>
   $ just add-trace-input-file serverproj tr1 ./test/benchmark/samples/bin/x86/binrec/client
   $ just add-trace-input-file serverproj tr1 ./test/benchmark/samples/binrec/test_inputs/input_files/client.sh
   ```

4. Next, we will need to instruct BinRec to execute the `client.sh` during tracing. To do this, we need to add a new setup action, that is run prior to launching the sample, which launches the `client.sh` script in the background. Use the follow just command:

   ```bash
   # just add-trace-setup <project_name> <trace_name> <bash_command>
   $ just add-trace-setup serverproj tr1 "./input_files/client.sh &"
   ```

   Note here that the entire `input_files` directory is mapped into the exectuion environemnt during tracing, so invoking the script is still relative to the project root.

5. Verify that the campaign is properly configured by running the `just describe` command:

   ```bash
   # just describe <project_name>
   $ just describe serverproj

   serverproj
   ==========
   Campaign File: /binrec/s2e/projects/serverproj/campaign.json
   Sample Binary: /binrec/test/benchmark/samples/bin/x86/binrec/server
   Traces (1):
     tr1
     ---
     Id: 0
     Command Line Arguments: []
     Symbolic Indexes:
     Input Files (2):
         /binrec/test/benchmark/samples/binrec/test_inputs/input_files/client.sh
           [Preserve source permissions]
         /binrec/test/benchmark/samples/bin/x86/binrec/client
           [Preserve source permissions]
     Setup (1):
         ./input_files/client.sh &
   ```

   Optionally, the campaign file can be viewed in an editor by opening the `s2e/projects/serverproj/campaign.json` file, which should look similar to the following:

    ```json
   {
     "traces": [
       {
         "args": [],
         "stdin": null,
         "match_stdout": true,
         "match_stderr": true,
         "input_files": [
           {
             "source": "/binrec/test/benchmark/samples/binrec/test_inputs/input_files/client.sh",
             "destination": null,
             "permissions": true
           },
           {
             "source": "/binrec/test/benchmark/samples/bin/x86/binrec/client",
             "destination": null,
             "permissions": true
           }
         ],
         "setup": [
           "./input_files/client.sh &"
         ],
         "teardown": [],
         "name": "tr1"
       }
     ],
     "setup": [],
     "teardown": []
   }
   ```

5. Finally, we can now `recover` the binary as with the previous examples:

   ```bash
   # just recover <project_name>
   $ just recover serverproj
   ```
