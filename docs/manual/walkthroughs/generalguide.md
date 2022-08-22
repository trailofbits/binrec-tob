# A General Guide to Binary Recovery

While previous walkthroughs have shown how to use BinRec in some basic and advanced contexts, this walkthrough provides some non-specific guidance for approaching binary recovery with BinRec. We assume only that we wish to isolate and recover some set of functionalities within a target binary. This guide will walk you through the steps necessary to analyze the target and set up BinRec to recover the desired functionality.

1. Before getting started, it is important to determine if BinRec can recover your binary. BinRec has a number of limitations. If your binary has non-deterministic behavior (e.g., behavior that is influenced by external conditions, randomized behavior, etc.), is multi-threaded, or is not a 32-bit ELF executable, then BinRec cannot be used to recover the binary's functionality.


2. Determine what system resources beyond the target binary itself are required to execute the desired behavior. This includes:

   - External libraries (not part of the base Debian 9.2.1 installation)
   - Configuration files
   - Environment variables (can be collected and exported as a script)
   - Files provided as inputs via the binary's command line interface
   - Other programs required to interact with the binary (e.g., other binaries in distributed systems, client binary to correspond to server targets, etc.)

   These system resources should be specified as `input files` in the project's campaign file.

3. Determine what inputs will trigger the behavior you wish to recover. They can be direct inputs like CLI flags and a arguments, or indirect like contents of an input file that will be parsed and trigger behavior in the binary.

   In general, BinRec will perform best with concrete traces. Each trace can be thought of as a single path through the program from beginning to end. To fully capture a program's functionality, many concrete traces are usually required. It can be helpful to think of the situations the function handles and the inputs that correspond to them. Don't forget error or edge cases! Failing to capture these situations can lead to creating a recovered binary that is more vulnerable or error prone than the original.

   For example, consider recovering a function that counts the number of occurrences of a word in a paragraph. The program would need to handle inputs for words that contain leading or trailing whitespace, as must your collection of concrete traces that capture this function.

   However, enumerating concrete arguments to fully capture the functionality you wish to recover can be onerous. In these cases, symbolic arguments can be used to specify multiple paths through the program to trace "at once". This can be thought of more simply as providing a wild card for a particular command line input.

   For example, to recover a function that performs integer division you would need to collect two traces, one for typical division and one that captures error handling when trying to divide by zero. Alternatively, the denominator argument may be specified as symbolic to capture both conditions with a single trace specification.

   Finally, remember that any files you specify in concrete arguments must also be provided as input files. As with concrete command line inputs, it may be necessary to provide multiple traces with multiple input files. For example, if you are trying to capture a program's ability to convert OpenOffice documents into PDF format. You would need to provide at least one input file in a document format, spreadsheet format, presentation format, etc.

4. Finally, determine if scripting is needed to coordinate the execution of your target binary for tracing. This is required if you need to set environment variables or have to include other binaries to support the binary to be traced. Scripting should be specified as `setup` or `teardown` in the project's campaign file.
