BinRec: Off-the-shelf Program Recompilation Through Binary Recovery
===================================================================

Overview
--------

BinRec dynamically lifts binary executables to LLVM IR. It is based on
[S2E](http://s2e.systems/docs/). For a full description of the framework, see the original research [paper](https://dl.acm.org/doi/10.1145/3342195.3387550).


Key Dependencies and Environment
--------------------------------

Binrec has been developed for and tested against the following environments and major dependencies:

- Ubuntu: 20.04.03
- LLVM (Clang): 12
- Python 3.9
- s2e-env @778116b

The limiting factor for both Linux environment and LLVM is s2e, which supports Ubuntu 20.04 LTS at a maximum.

Building BinRec
---------------
1. BinRec uses [just](https://github.com/casey/just#installation) to automate various tasks including building BinRec. The first step in building BinRec is to install this tool (and `curl` if not already installed). We provide a simple shell script for this:

        $ ./get_just.sh

2. Next, BinRec and its dependencies can be fetched, built, and installed from the root of this repository with:

       $ just install-binrec

3. (Optional) The above command will download a pre-built QEMU virtual machine image within which target binaries will run. The default pre-built image is a Linux `x86` image (currently the only supported environment). If you want to download additional images (e.g., Linux `x64`) you can use the following command:

   ```bash
   # just build-s2e-image <image_name>
   $ just build-s2e-image debian-9.2.1-x86_64
   ```


Simple BinRec Project
----------------------------------------

In this short walkthrough, we take you through a sample BinRec project: debloating a program binary. Our goal in the walkthrough is to exercise the target binary with a set of concrete inputs. BinRec will trace the execution of the program on these inputs,
merge the traces, lift the merged trace to LLVM IR, and then recompile the IR to a debloated program. 

1. Create a new analysis project by specifying the target binary and the first set of concrete inputs: 

   ```bash
   # just new-project <project_name> <path_to_target_binary> <sym_args> <concrete_args_list>
   $ just new-project eqproj test/benchmark/samples/bin/x86/binrec/eq "" 1 2
   ```
   
   **NOTE:** Currently there is no support for symbolic arguments, this should be left as `""`.
 
   This will create a project folder located at `s2e/projects/eqproj/` where traces and output associated with this project will be kept.

2. Run the project:

   ```bash  
   # just run <project_name>
   $ just run eqproj
   ```

3. For each additional set of concrete inputs to trace, re-run the project with new arguments:

   ```bash
   # just set-args <project_name> <concrete_args_list>
   # just run <project_name>
   $ just set-args eqproj 2 2
   $ just run eqproj
   
   # OR 

   # just run <project_name> --args <concrete_args_list>
   just run eqproj --args 0 10
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

6. To verify the debloated program behaves as intended, it can be compared by running it with the traced inputs and verifying it produces the same output as the original binary (`binary`).

   ```bash
   $ cd s2e/projects/eqproj/s2e-out

   $ ./binary 1 2
   # arguments are NOT equal
   $ ./binary 2 2
   # arguments are equal
   $ ./binary 0 10
   # arguments are NOT equal

   $ ./recovered 1 2 
   # arguments are NOT equal 
   $ ./recovered 2 2
   # arguments are equal
   $ ./recovered 0 10
   # arguments are NOT equal
   ```

Running Tests
-------------

Unit and integration tests are written in `pytest` and `GoogleTest`. Running tests can be done through multiple `just` recipes.

   ```bash
   just run-all-tests  # run all unit and integration tests
   just run-unit-tests  # run only unit tests
   just run-integration-tests  # run only integration tests (this will take several minutes)
   just print-coverage-report  # print the last pytest coverage report
   ```

Integration tests use sample code and binaries provided by the
[binrec-benchmark](https://github.com/trailofbits/binrec-benchmark) repository, which is a submodule of the BinRec repo.
Each integration test sample is run multiple times, depending on the test inputs, the runtime traces are merged, the
merged trace is lifted, and then the recovered binary is compared against the original binary with the same test inputs
used during the trace operations. The process exit code, stdout, and stderr are compared for each test input.

For more information, refer to the binrec-benchmark `README`.

Development
-----------

### Python API

The BinRec Python 3 API and module is located in the `binrec` directory.

### Linting and Formatting

There are several just recipes to lint and format the  source code for BinRec. In general, the
project uses the following tools:

- [black](https://github.com/psf/black) - Python code formatting and style
- [flake8](https://flake8.pycqa.org/en/latest/) - Python static code analysis
- [mypy](https://github.com/python/mypy) - Python type checking
- [isort](https://github.com/PyCQA/isort) - Python import order formatting
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) - C++ source code formatting and style

These tools can be called through just using:

- `just lint` - Run all code linters
- `just _lint-<toolname>` - Run a single code linter, Ex. `just lint-flake8`
- `just format` - Run all code formatters
- `just _format-<toolname>` - Run a single code formatter, ex. `just format-black`

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

Other Notes
-----------

### Callbacks

Support for callbacks is currently work in progress. To enable it, set the lift and lower option `-f unfallback`. Note
this does not actually enable the fallback, it will use whatever fallback option you have hardcoded. Later the option
can be made more clear.

--------
