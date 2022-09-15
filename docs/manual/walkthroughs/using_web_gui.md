# Using the BinRec Web GUI

BinRec provides a web-based graphical interface where you can:

- Create new analysis projects.
- Add and remove trace parameters.
- Recover a project to an original binary and validate the lift results.

Projects created within the web interface are also available in the command line interface because both interfaces use the same underlying directory structure and design.

## Guide

This guide will walkthrough the process of creating a project, adding traces, and recovering the binary.

## Dashboard

Launch the web interface by running the `run-web-server` just receipe:

```bash
$ just run-web-server
```

The just receipe will start the web server and then attempt to launch a web browser. If the web browser does not launch, manually open a web browser to `http://localhost:8080`.

Once open, the dashboard will look similar to this:

**INSERT SCREENSHOT OF DASHBOARD PAGE**

The dashboard lists all projects currently registered within BinRec.

## Create a New Project

From the dashboard page, click the **New Project** button to begin creating a new project.

**INSERT SCREENSHOT OF NEW PROJECT PAGE**

The new project form accepts several inputs:

- **Project Name** (*optional*) - A unique name for the project. If a name is not provided, the filename of the uploaded binary is used.
- **Binary Filename** - Upload a sample binary to analyze.

For this example, the `eq` benchmark sample will be analyzed, so fill in the following information:

- **Project Name**: `demo_eq`
- **Binary Filename**: navigate to the `eq` binary, which is stored in the BinRec repository at `test/benchmark/samples/bin/x86/binrec/eq`

Save the project by clicking the **Save** button. If successful, the project is created and the browser will navigate to the new project's detail page.

## Add or Remove a Trace

The project details page shows information about the project including registered traces. Traces specify the command line arguments that will be used when recording the binary's behavior and also used after recovery to validate the results.

**INSERT SCREENSHOT OF PROJECT DETAILS PAGE**

There are initially no registered traces so the project cannot be recovered yet. To add a new trace, click the **Add Trace** button which loads the new trace form.

**INSERT SCREENSHOT OF NEW TRACE PAGE**

The new trace form accepts several inputs:

- **Trace Name** (*optional*) - The unique name of the trace. This is optional and should provide a very brief description of what the trace is exercising within the binary.
- **Command Line Arguments** - The command line arguments that will be passed into the binary. This will be the command line arguments used as if you were running the sample from the terminal. An argument that contains a space must be enclosed quotations and escape characters can be used. For example, a command line of the following:
  ```
  binrec says "hello \"world\""
  ```

  Would be interpreted as the following arguments (`argv`):

  1. `binrec`
  2. `says`
  3. `hello "world"`

- **Symbolic Argument Indices** (*optional*) - The list of symbolic arguments within the specified command line arguments. This field is space-seperated list of argument indices. In the previous example, if the first argument (`binrec`) and the last argument (`hello "world"`) are to be treated as symbolic, then this field would be set to:
  ```
  1 3
  ```

For the `eq` sample, first exercise the case where the two arguments are equal to eachother by providing the following inputs:

- **Trace Name**: `equal`
- **Command Line Arguments**: `a a`

Click the **Save** button to save the new trace.

## Recover a Project

After saving the new trace, the browser will navigate back to the project details page which will show that there is a single registered trace and the project can be recovered now.

**INSERT SCREENSHOT OF PROJECT DETAILS PAGE**

To recover a project, click the **Recover Project** button. A single project can be recovered at once. After a minute or two continue to reload the page until the recovery process finishes.

## Viewing Log Data and Verifying Results

A recovery operation could fail in which case the recovery log can be downloaded and reviewed to see what went wrong. Click the **Download Previous Recovery Log** button to download and view the entire log of the previous recovery attempt.

If a recovery is successful, the project details page will show that the previous recovery result was **Successful**, which means that:

- All registered traces were executed and recorded.
- The trace results were merged, lifted, and the binary was recovered.
- The recovered binary was run again with each trace parameter and the outputs were validated against the original binary (stdout, stderr, and exit code).

## Add a New Trace and Recover the Project

At this point, the project should be successfully recovered. However, for the `eq` sample, there needs to be another trace that exercises the case where the two arguments are not equal. Therefore, the existing trace data must be cleared, a new trace added, and then the project must be recovered again. To accomplish this, perform the following:

1. Clear all collected trace data by clicking the **Clear Collected Trace Data** button. This will delete all collected data and the recovered binary.
2. Click the **Add Trace** button to begin adding a new trace.
3. In the new trace form, specify a new trace with the following information:
   - **Trace Name**: `not-equal`
   - **Command Line Arguments**: `a b`
4. Save the new trace.
5. Verify that two traces are registered that exercise the equal and not equal cases. Then recover the project.

**INSERT SCREENSHOT OF PROJECT DETAILS PAGE WITH 2 TRACES**

After the recovery completes and is successful, you can verify the results manually by running the recovered binary from the command line. This is the same whether the project was created and recovered from the command line and web interfaces.

```bash
$ cd path/to/binrec-prerelease

$ ./s2e/projects/demo_eq/s2e-out/recovered z z
$ ./s2e/projects/demo_eq/s2e-out/recovered z a
```

## Closing the Web Interface

In the terminal where the `just run-web-server` command was run, terminate the web server via `ctrl+c`. This will also automatically close the web browser.
