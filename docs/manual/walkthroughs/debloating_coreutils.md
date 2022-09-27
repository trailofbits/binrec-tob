# A More Realistic Debloating Example

In this short walkthrough, we tackle a more realistic debloating example. In this scenario, we wish to remove unneeded functionality from the `id` program, which is a coreutils program on linux systems that provides information on system users and their group memberships. Let's assume that system users should be able to use `id` to obtan information about themselves, but not other users on the system.

1. Let's start by examining how `id`'s functionalities are invoked.

   ``` bash
   id --help

   >>> Usage: id [OPTION]... [USER]
   Print user and group information for the specified USER,
   or (when USER omitted) for the current user.

   -a             ignore, for compatibility with other versions
   -Z, --context  print only the security context of the process
   -g, --group    print only the effective group ID
   -G, --groups   print all group IDs
   -n, --name     print a name instead of a number, for -ugG
   -r, --real     print the real ID instead of the effective ID, with -ugG
   -u, --user     print only the effective user ID
   -z, --zero     delimit entries with NUL characters, not whitespace;
                     not permitted in default format
         --help     display this help and exit
         --version  output version information and exit

   ```

   We see that running `id` without a specified `USER` results in default behavior, which is to print information for the current user. This means we can debloat the ability to print other user information by only exercising the default behavior.


2. Let's create a new analysis project to debloat the `id` program with the default inputs:

   ```bash
   # no symbolic arguments, no concrete arguments
   $ just new-project idproj test/benchmark/samples/bin/x86/coreutils/id "" ""
   ```
 
   This will create a project folder located at `s2e/projects/idproj/` where traces and output associated with this project will be kept.

3. Next, let's run the project:

   ```bash
   $ just run idproj
   ```

   This command will kick off the tracing process, which occurs in a virtual execution environment. BinRec first loads the `id` binary into the execution environment so it can be traced. Next it executes `id` in the environment with the provided arguments (none in this case). Using hooks inserted into tracing environment, BinRec records which sequence of code blocks in the program are executed and other supporting information. Finally, these code blocks are translated into LLVM to complete the tracing process.

   The tracing process will collect one or more (for symbolic arguments) traces for each provided input. Each trace individually represents on path through the program from beginning to end.  

4.  Before a new binary can be built, the individual traces need to be merged into a single, consolidated collection of traces. In our case, we only need one trace to collect the functionality we want. However, merging is still required even though we only have one trace because a number of preparation steps that must occur before lifting are performed in this stage.

   Next, we run the merging component:

   ```bash 
   $ just merge-traces idproj
   ```

   This command will pre-process each collection of LLVM code blocks and merge them together, removing duplicates (i.e., a single block in the binary that is executed in multiple traces). This process also consolidates the trace information (i.e., the sequence of blocks executed in each trace).

5. When merged, the consolidated LLVM IR code blocks are not yet ready for recompilation. Before they can be recompiled, they will need a significant amount optimization so they can perform at a level similar to the original binary. Also, the connections between the code blocks must be recovered and replaced to ensure the program will still function with code blocks resized and relocated in the new binary. This is performed by BinRec's lifting (i.e., refinement) component.

   Finally, we lift the merged trace and recompile it:

   ```bash
   $ just lift-trace idproj
   ```

   This will create a `s2e-out` subdirectory in the project folder. It contains the debloated program binary (`recovered`) as well the original binary (`binary`).

6. Now that we have a debloated binary, let's ensure that it behaves the way we want. First, let's ensure that the debloated binary behaves like the original for default behavior:

   ```bash
   $ just validate idproj
   ```

   Here, we see that the behavior matches as expected.

7. Next, let's test that the functions that we wanted to removed are no longer present:

   ```bash
   # Change to the output directory
   $ cd s2e/projects/idproj/s2e-out
   
   # Try running id with a user other than the current user
   $ ./binary root

   >>> uid=0(root) gid=0(root) groups=0(root)

   $ ./recovered root

   >>> uid=1000(antx) gid=1000(antx) groups=1000(antx),4(adm),24(cdrom),27(sudo),30(dip),46(plugdev),120(lpadmin),132(lxd),133(sambashare)
   ```

   We see here that the recovered binary now ignores the `USER` command line argument, limiting the program to just the current user.

