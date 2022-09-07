# Contributing to BinRec

Thanks for your interest in contributing to BinRec!

This document is primarily focused on detailing BinRec's high-level development
process. For information on installing and using BinRec, please refer to the
[README](README.md).

## Issue Tracking

Bugs, new features, enhancements, and general issues are all tracked in our
[GitHub repository](https://github.com/trailofbits/binrec-prerelease).
A GitHub issue should contain all relevant discussion about the topic.
If discussions about a particular issue occur in our slack channel
(#research-metis), please summarize the takeaways from the discussion
in a comment on the issue.

## Development Environment

BinRec and its dependencies are built for and run on Ubuntu 20.04.
BinRec is primarily written in Python 3.9 and LLVM-flavored C++.
While we do not prescribe or maintain an environment a specific IDE /
code editor, VScode is a good IDE to start with for newcomers
as it provides decent support for both the Python and C++ portions of BinRec.

### Repository Structure

- `binrec` - BinRec Python 3 API
- `binrec_lift` - BinRec C++ components (LLVM Passes) that perform Merging
and Lifting functions.
- `binrec_link` - BinRec C++ components that perform recompilation.
- `binrec_plugins` - BinRec C++ plugins for S2E, used to generate LLVM IR
and trace information during execution tracing
- `binrec_rt` - BinRec runtime (used for binary recompilation)
- `binrec_traceinfo` - Custom C++ library implementing the trace info data
structure and its import / export functions.
- `binrec_tracemerge` - Simple C++ tool for merging multiple trace info files
into one. User during Merging function.
- `docs` - BinRec autogenerate documentation infrastructure
- `runlib` - Supplementary build files supporting recompilation
- `scripts` - Various utilities
- `test` - Testing infrastructure, includes benchmark samples.
- `s2e-env` - Submodule for cloning, building, and interfacing with S2E.

### Environment Variables

The BinRec toolkit depends on several environment variables. The `.env` file
contains the required environment variables and is loaded by every just recipe
and the Python API. The `.env` file needs to be loaded when calling BinRec
tools manually within Bash:

   ```bash
   source .env
   ```

### Useful BinRec Development Just Commands

The BinRec `justfile` contains several commands that are useful for
development. The following list contains these commands and a short
description of their purpose:

- `just build-binrec` : rebuilds the BinRec components only. Used to recompile
after changes to the lifting, merging, or recompilation components.
- `just rebuild-s2e-plugins` : rebuilds BinRec's plugins to S2E and any
dependent code within S2E. Used to recompile after changes to the plugins.
NOTE: This process may be lengthy for the first plugin rebuild after installing
BinRec, but should be short for subsequent rebuilds.
- `just format` : formats all code (Python and C++). CI will fail if code pushed
to the remote is ill-formatted.
- `just print-coverage-report` : Prints a test coverage report for the Python
API. Our test coverage is not currently and likely will never be 100%, however
as we add new API features / fix bugs we strive to maintain our current level
of test coverage.
- `just _pretty-print-trace-info <ti-file> |--hex|` : Debug command to dump trace
info files produced during S2E tracing runs in a human readable format. Adding the
`--hex` flag prints the addresses in hex rather than decimal.
- `just _diff-trace-info <ti-file-1> <ti-file-2> |--hex|` : Debug command to
perform a diff between two trace info files. Adding the `--hex` flag prints the
addresses in hex rather than decimal.

   **NOTE:** Many BinRec commands in the `justfile` are "private" (i.e., unlisted)
   to end user. They are indicated by commands starting with an `_` character.
   In most cases these commands are private because they are subsumed by some other
   command for routine usage, but may be useful to run individually for
   development or troubleshooting.

## Code Quality

### Linting and Formatting

There are several just recipes to lint and format the source code for BinRec.
Formatting is enforced by CI. In general, the project uses the following tools:

- [black](https://github.com/psf/black) - Python code formatting and style
- [flake8](https://flake8.pycqa.org/en/latest/) - Python static code analysis
- [mypy](https://github.com/python/mypy) - Python type checking
- [isort](https://github.com/PyCQA/isort) - Python import order formatting
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) - C++ source
  code formatting and style
- [markdownlint-cli](https://github.com/igorshubovych/markdownlint-cli) -
  Markdown documentation formatting
- [mdspell](https://github.com/mtuchowski/mdspell) - Markdown spelling checker
- [doc8](https://github.com/pycqa/doc8) - rST formatting checker that accounts
  for Sphinx-specific false positives
- [sphinxcontrib.spelling](https://sphinxcontrib-spelling.readthedocs.io) -
  spelling checker for Sphinx rST docs

These tools can be called through just using:

```bash
just lint  # Run all code linters
just _lint-<toolname>  # Run a single code linter, Ex. just lint-flake8
just format # Run all code formatters
just _format-<toolname> # Run a single code formatter, ex. just format-black
```

## Testing

Unit and integration tests are written in `pytest` and `GoogleTest`.
Running tests can be done through multiple `just` recipes.

```bash
just run-all-tests  # run all unit and integration tests
just run-unit-tests  # run only unit tests
just run-integration-tests  # run only integration tests (takes a few minutes)
just print-coverage-report  # print the last pytest coverage report
```

Integration tests use sample code and binaries provided by the
[binrec-benchmark](https://github.com/trailofbits/binrec-benchmark) repository,
which is a submodule of the BinRec repo.
Each integration test sample is run multiple times, depending on the test inputs,
the runtime traces are merged, the merged trace is lifted, and then the
recovered binary is compared against the original binary with the same test inputs
used during the trace operations. The process exit code, stdout, and stderr are
compared for each test input.

For more information, refer to the binrec-benchmark `README`.

## Contributing Code

BinRec uses the pull request contribution model. When working on an issue,
please create a new branch for your contribution and submit code contributions
via pull request.

### Continuous Integration

Commits to pull requests in progress will be automatically checked to ensure
the code is properly linted and passes unit tests. Additionally, the doc build
will be checked. Integration tests are not automatically checked due to
BinRec's complexity. Before submitting a PR for review, the contributor shall
ensure integration tests pass on their development machine. Additionally,
PR contributors shall add sufficient unit tests to maintain test coverage.

## Documentation

The `build-docs` just recipe builds documentation for both the Python API
and the C++ API.

   ```bash
   just build-docs
   ```

Alternatively, Python and C++ API documentation can be built independently
using the language-specific just recipes.

The Python API is documented in [Sphinx](https://www.sphinx-doc.org/en/master/)
and the docs can be built using the `build-python-docs` just recipe. By default,
the recipe will build HTML documentation, but this can be changed by specifying
a `target` parameter:

   ```bash
   # build HTML docs to docs/build/html/index.html
   $ just build-python-docs

   # build manpage docs to docs/build/man/binrec.1
   $ just build-python-docs man
   ```

The C++ API is documented with [Doxygen](https://www.doxygen.nl/index.html)
and the docs can be built using the `build-cpp-docs` just recipe.

   ```bash
   just build-cpp-docs
   ```
