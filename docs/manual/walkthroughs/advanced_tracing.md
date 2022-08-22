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
   # just add-trace <project_name> <trace_name> <args>
   $ just add-trace serverproj tr1
   ```

3. Now, we will need to edit the campaign file to specify additional input files required by `server`. We need to add two files. The first is a compatible `client` binary that can externally trigger the functionality we want to trace in the `server` binary. The second is a short shell script `client.sh` that when executed will pause to allow the `server` binary to startup and then run the `client` binary.

    When we open the campaign file, it will look like this:

   ```bash
   cat ./s2e/projects/serverproj/campaign.json

   {
    "setup": [],
    "teardown": [],
    "traces": [
        {
        "args": [],
        "name": "tr1"
        }
    ]
   }
   ```

   To specify input files or other files required in the tracing environment, we need to add the `"input_files"` attribute to the trace `tr1`:

   ```bash
    {
    "setup": [],
    "teardown": [],
    "traces": [
        {
        "args": [],
        "name": "tr1",
        "input_files": [
            "./input_files/client",
            "./input_files/client.sh"
        ],
        }
    ]
   }
   ```

   Note here that the input files we have specified are located relative to the root of the project folder. So, in addition to modifying the campaign file, we also need to place the `client` binary and the `client.sh` files to the `s2e/projects/serverproj/input_files` directory.

4. Next, we will need to instruct BinRec to execute the `client.sh` during tracing. To do this, we need to further edit the trace to add a `setup` attribute to the trace `tr1`:

   ```bash
   {
    "setup": [],
    "teardown": [],
    "traces": [
        {
        "args": [],
        "name": "tr1",
        "input_files": [
            "./input_files/client",
            "./input_files/client.sh"
        ],
        "setup": [
            "./input_files/client.sh &"
        ]
        }
    ]
   }
   ```

   Note here that the entire `input_files` directory is mapped into the exectuion environemnt during tracing, so invoking the script is still relative to the project root.

5. Finally, we can now `recover` the binary as with the previous examples:

   ```bash
   # just recover <project_name>
   $ just recover serverproj
   ```
