BinRec: Off-the-shelf Program Recompilation Through Binary Recovery
===================================================================

Overview
--------

BinRec dynamically lifts binary executables to LLVM IR. It is based on
[S2E][1]. For a full description of the framework, see our [paper][9].


Building BinRec
-------------
BinRec uses [just](https://github.com/casey/just#installation) to automate various tasks including building BinRec. The
first step in building BinRec is to install this tool (and curl if not already installed):

	  $ sudo apt-get install -y curl
      $ curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | sudo bash -s -- --to /usr/local/bin

Then, BinRec can be built from the root of this repository with:

       $ just install-dependencies
       $ just build-all

3. You can use the network to put stuff into your qemu vm. Lifting binaries that actually use the network is untested.
   Run the `configure-network` just recipe to configure the network. The recipe detects the active adapter and creates a a bride and tap interface.

       $ just configure-network  # enter sudo password when prompted

   **Note:** When running as a VM on VMWare Fusion, you may be prompted in the Mac host OS to allow the VM to monitor network traffic.

Running a binary in the BinRec front-end
----------------------------------------

### Preparing a VM image

You need a QEMU VM image to run the binary in the front-end. Follow [S2E's guide for building a VM image][3] to create a
Debian image, or use our pre-built image from [here][8]. Save the image to `qemu/debian.raw`.

We use S2E's [init\_env.so][4] library to detect execution of the target binary in the VM. Our pre-built image has a
built copy integrated, but for a custom image you need to build the library (along with the [`s2eget`][5] tool) in
[s2e/guest/tools](/s2e/guest/tools/) and copy them into the VM.

### Running the binary

S2E builds two QEMU binaries: one unmodified and one with S2E instrumentation to facilitate symbolic execution and
plugins. The latter is considerably slower and does not support KVM mode, so you want to do as much set-up work (like
booting the VM) in the unmodified version as possible. Another thing to take into consideration is that the target
binary needs to be in the VM, which does not support mounting host directories, so you need to either copy the files
into the VM on beforehand or load them at runtime. Here, we give an example of the `hello` program from the `test`
directory being recovered using our pre-built VM image in combination with S2E's "HostFiles" plugin.

1. Boot the VM:

       $ export MEM_AMOUNT=1024    # if you want to modify VM ram, default is 1G
       $ ./qemu/debian.sh -vnc :0  # boot the VM

   Pass `-net` if you want the qemu to access the network. Put it before vnc arg

       $ ./qemu/debian.sh -net -vnc :0   # boot the VM

2. Open RealVNC Viewer client to connect to the VM. You can also launch the GUI directly to do this. FIXME: The port reported by your environment configuration
   step is not correct.

       $ vncviewer 127.0.0.1:5900


   Log in (user "user" and password "password" for the prebuilt image) and run the command `getrun-cmd`:

       $ getrun-cmd

   This command is a wrapper that adds input arguments to `s2eget`, which blocks in regular QEMU and continues in S2E's
   modified version of QEMU, so we can now create a snapshot to continue later in S2E mode. Switch to the QEMU shell by
   pressing `ctrl+alt+2` and save a snapshot with the name "cmd". Note, the directory from which you invoke getrun-cmd
   is important is some cases. Then, exit the VM:

       # savevm cmd
       # quit

   **WARNING:** In macOS, if `ctrl+alt+2` doesn't work, try the following:
   `XQUARTS-->preferences-->Input-->Option keys send ALT L and ALT R`

3. Back in the original shell, load the snapshot in S2E mode with the custom plugin loaded, using the HostFiles
   plugin to copy the target binary to the VM:

       $ ./qemu/cmd-debian.sh --vnc 0 hello

   This loads the "cmd" snapshot and copies the "hello" binary into the VM as requested by the `getrun-cmd`
   command. `--vnc` makes the script pass `-vnc
   :0` to the qemu command, making it start a VNC server since we are running in a docker instance where there is no X
   server. Run `./qemu/cmd-debian.sh -h` to see options on how to pass input to the target binary. Note that the target
   binary should reside in directories scanned by the HostFiles plugin, configured in `plugins/config.debian.lua`. Also
   see https://github.com/S2E/s2e-old/blob/master/docs/Howtos/init_env.rst

4. Parallel lifting is supported by creating a config file which contains the binary invocation and args, then using the
   cmd-debian-mt script. Edit this script, for instance to pass `--net` to the cmd-debian invocations

        $ ./qemu/cmd-debian-mt.sh configFile

    The VM is killed automatically after the target binary has been run. Directories
    called `s2e-out-hello-1` `s2e-out-hello-2` are created, containing the captured
    LLVM bitcode in `captured.bc`.

Deinstrumentation and lowering of captured bitcode
--------------------------------------------------

Before we run the lifting script, we need to merge these multiple directories into 1 directory called `s2e-out-hello`. To
do that run following in `$S2EDIR` if you want to merge all directories \**-hello-*\*

    $ ./scripts/double_merge.sh hello

or run the following if you want to merge the numbered subdirectories of s2e-out-hello-1

    $ ./scripts/merge_traces.sh hello-1

**Note:** the scripts assume that multiple traces have been run and they will not work against a single trace.

Now go into the `s2e-out-hello` directory and run the lifting script to obtain deinstrumented code.

    $ cd s2e-out-hello
    $ ../scripts/lift2.sh

This created a recovered binary called `recovered` that contains the compiled code from `recovered.bc` (LLVM source code
is generated in `recovered.ll`).

Running Tests
-------------

Unit and integration tests are run with the `run-tests` just recipe. The tests require a working Debian qemu image with a
`cmd` snapshot already saved. Follow the instructions in this README through the `savevm` command.

With a qemu image and `cmd` snapshot ready, verify that the submodules are up to date and then run the tests with the
`run-tests` just recipe.

```
git submodule update --recursive --init
just run-tests
```

Tests are written in Python (`pytest`). Integration tests use sample code and binaries provided by the
[binrec-benchmark](https://github.com/trailofbits/binrec-benchmark) repository, which is a submodule of the binrec repo.
Each integration test sample is run multiple times, depending on the test inputs, the runtime traces are merged, the
merged trace is lifted, and then the recovered binary is compared against the original binary with the same test inputs
used during the trace operations. The process exit code, stdout, and stderr are compared for each test input.

For more information, refer to the binrec-benchmark README.


Other Notes
-----------

### Linking

In the current implementation we reuse the entrypoint of the original binary. Different compiler and operating system
combinations generate slightly different variants of this. Before `__libc_start_main` is called, the address of `main`
needs to be patched to the new entrypoint. We cannot do that automatically yet, so open up your binary in your favorite
disassembler, go to `_start` and count the bytes until you get to the address of `main`. Use this value
in `binrec_link/src/Stitch.cpp:getStartPatch`.

Development
-----------

### Python API

The BinRec Python 3 API and module is located in the `binrec` directory.

### Linting and Formatting

There are several just recipes to lint and format the Python source code for BinRec. In general, the
project uses the following tools:

- [black](https://github.com/psf/black) - Python code formatting and style
- [flake8](https://flake8.pycqa.org/en/latest/) - Python static code analysis
- [mypy](https://github.com/python/mypy) - Python type checking
- [isort](https://github.com/PyCQA/isort) - Python import order formatting

These tools can be called through just using:

- `just lint` - Run all code linters
- `just lint-<toolname>` - Run a single code linter, Ex. `just lint-flake8`
- `just format` - Run all code formatters
- `just format-<toolname>` - Run a single code formatter, ex. `just format-black`

Change Log
--------

Trace merging scripts deduplicated. Assumes you pass the s2e-max-processes flag for directory structure. See the
Deinstrumenting and lowering bitcode section.

Support for callbacks is currently work in progress. To enable it, set the lift and lower option `-f unfallback`. Note
this does not actually enable the fallback, it will use whatever fallback option you have hardcoded. Later the option
can be made more clear.

--------

[1]: https://github.com/dslab-epfl/s2e/blob/master/docs/index.rst
"S²E documentation"

[2]: https://github.com/dslab-epfl/s2e/blob/master/docs/BuildingS2E.rst
"Building S²E"

[3]: https://github.com/dslab-epfl/s2e/blob/master/docs/ImageInstallation.rst
"Preparing VM Images for S²E"

[4]: https://github.com/dslab-epfl/s2e/blob/master/docs/Howtos/init_env.rst
"init_env.so preload library"

[5]: https://github.com/dslab-epfl/s2e/blob/master/docs/UsingS2EGet.rst
"Quickly Uploading Programs to the Guest with s2eget"

[6]: https://www.docker.com/
"Docker"

[7]: https://www.docker.com/products/docker-compose
"Docker Compose"

[8]: https://www.ics.uci.edu/~fparzefa/binrec/debian10.raw.xz
"BinRec Debian 10 image"

[9]: https://dl.acm.org/doi/10.1145/3342195.3387550
"BinRec: dynamic binary lifting and recompilation"

[10]: https://www.realvnc.com/en/connect/download/viewer/linux/
"Download VNC Viewer for Linux"
