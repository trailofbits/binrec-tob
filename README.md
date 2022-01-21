BinRec: Off-the-shelf Program Recompilation Through Binary Recovery
===================================================================

Overview
--------

BinRec dynamically lifts binary executables to LLVM IR. It is based on
[S2E][1]. For a full description of the framework, see the original research [paper][9].


Key Dependencies and Environment
--------------------------------

Binrec has been developed for and tested against the following environments and major dependencies:

- Ubuntu: 20.04.03
- LLVM (Clang): 12
- Python 3.9
- s2e-env @97727c4

The limiting factor for both Linux environment and LLVM is s2e, which supports Ubuntu 20.04 LTS at a maximum.

Building BinRec
---------------
1. BinRec uses [just](https://github.com/casey/just#installation) to automate various tasks including building BinRec. The first step in building BinRec is to install this tool (and `curl` if not already installed). We provide a simple shell script for this:

        $ ./get_just.sh

2. Next, BinRec and its dependencies can be fetched, built, and installed from the root of this repository with:

       $ just install-binrec

   NOTE: Your `git` instance requires a configured user name to install BinRec's S2E plugins.

3. (Optional) The above command will download a pre-built QEMU virtual machine image within which target binaries will run. The default pre-built image is a Linux `x86` image (currently the only supported environment). If you want to download additional images (e.g., Linux `x64`) you can use the following command:

   ```bash
   # just build-s2e-image <image_name>
   $ just build-s2e-image debian-9.2.1-x86_64
   ```


Running a binary in the BinRec front-end
----------------------------------------

S2E builds two QEMU binaries: one unmodified and one with S2E instrumentation to facilitate symbolic execution and
plugins. The latter is considerably slower and does not support KVM mode, so you want to do as much set-up work (like
booting the VM) in the unmodified version as possible. Another thing to take into consideration is that the target
binary needs to be in the VM, which does not support mounting host directories, so you need to either copy the files
into the VM on beforehand or load them at runtime. Here, we give an example of the `hello` program from the `test`
directory being recovered using our pre-built VM image in combination with S2E's "HostFiles" plugin.

<TODO: From here, the process has changed and must be re-documented.>
1. Load the snapshot in S2E mode with the custom plugin loaded, using the HostFiles
   plugin to copy the target binary to the VM:

       $ source .env
       $ ./qemu/cmd-debian.sh --vnc 0 hello

    (TODO: I think we can eliminate the `source .env` above if we wrap this in `just`.)

   This loads the "cmd" snapshot and copies the "hello" binary into the VM as requested by the `getrun-cmd`
   command. `--vnc` makes the script pass `-vnc
   :0` to the qemu command, making it start a VNC server since we are running in a docker instance where there is no X
   server. Run `./qemu/cmd-debian.sh -h` to see options on how to pass input to the target binary. Note that the target
   binary should reside in directories scanned by the HostFiles plugin, configured in `plugins/config.debian.lua`. Also
   see https://github.com/S2E/s2e-old/blob/master/docs/Howtos/init_env.rst

2. Parallel lifting is supported by creating a config file which contains the binary invocation and args, then using the
   cmd-debian-mt script. Edit this script, for instance to pass `--net` to the cmd-debian invocations

        $ ./qemu/cmd-debian-mt.sh configFile

    The VM is killed automatically after the target binary has been run. Directories
    called `s2e-out-hello-1` `s2e-out-hello-2` are created, containing the captured
    LLVM bitcode in `captured.bc`.

Deinstrumentation and lowering of captured bitcode
--------------------------------------------------

Before we run the lifting script, we need to merge these multiple directories into 1 directory called `s2e-out-hello`. To
do that run following in `$S2EDIR` if you want to merge all directories \**-hello-*\*

```bash
$ just merge-traces hello
```

or run the following if you want to merge the numbered subdirectories of s2e-out-hello-1

```bash
$ just merge-captures hello-1
```

Now go into the `s2e-out-hello` directory and run the lifting script to obtain deinstrumented code.

```bash
$ just lift-trace hello
```

This created a recovered binary called `recovered` that contains the compiled code from `recovered.bc` (LLVM source code
is generated in `recovered.ll`).

Running Tests
-------------

Unit and integration tests are written in pytest. Running tests can be done through multiple just recipes.

```
just run-tests  # run all unit and integration tests
just run-unit-tests  # run only unit tests
just run-integration-tests  # run only integration tests (this will take several minutes)
just run-test-coverage-report  # print the last pytest coverage report
```

The integration tests require a working Debian qemu image with a `cmd` snapshot already saved. Follow the instructions in this README through the `savevm` command. Integration tests use sample code and binaries provided by the
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

### Environment Variables

The BinRec toolkit depends on several environment variables. The `.env` file contains the required environment variables and is loaded by every just recipe and the Python API. The `.env` file needs to be loaded when calling BinRec tools manually within Bash:

```bash
$ source .env
```


### Documentation

The `build-docs` just recipe builds documentation for both the Python API and the C++ API.

```bash
$ just build-docs
```

Alternatively, Python and C++ API documentation can be built independently using the language-specific just recipes.

The Python API is documented in [Sphinx](https://www.sphinx-doc.org/en/master/) and the docs can be built using the `build-python-docs` just recipe. By default, the recipe will build HTML documentation, but this can be changed by specifying a `target` parameter:

```bash
# build HTML docs to docs/build/html/index.html
$ just build-python-docs

# build manpage docs to docs/build/man/binrec.1
$ just build-python-docs man
```

The C++ API is documented with [Doxygen](https://www.doxygen.nl/index.html) and the docs can be built using the `build-cpp-docs` just recipe.

```bash
$ just build-cpp-docs
```

### Continuous Integration
Commits to pull requests in progress will be automatically checked to ensure the code is properly linted and passes unit tests. Additionally, the doc build will be checked. Integration tests are not automatically checked due to BinRec's complexity. Before submitting a PR for review, the contributor shall ensure integration tests pass on their development machine. Additionally, PR contributors shall add sufficient unit tests to maintain test coverage.

Notes
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
