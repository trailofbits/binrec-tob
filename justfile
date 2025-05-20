# List recipes in categorical order
default:
  @just --list --unsorted

########## Section: Environment and Recipe Variables ##########

# Environment Variables (Include in all justfiles that need them)
set dotenv-load := true
set positional-arguments := true

# Useful Recipe Variables
binrec_authors := "Trail of Bits, University of California, Irvine and Vrije Universiteit Amsterdam"
justdir := justfile_directory()
plugins_root := justdir + "/s2e/source/s2e/libs2eplugins"
plugins_dir := join(plugins_root, "src/s2e/Plugins")
plugins_cmake := join(plugins_root, "src", "CMakeLists.txt")
docs_md := "README.md CONTRIBUTING.md docs/**/*.md"
nvm_node_version := "v14"

# Add Clang / LLVM binaries (and other dependencies) from S2E install to PATH
# Add node.js-based markdown linters to PATH
mdlint_bin := env_var('NODE_MODULES') + "/markdownlint-cli"
mdspell_bin := env_var('NODE_MODULES') + "/markdown-spellcheck/bin"
export PATH := env_var('S2E_BIN') + ":" + env_var('PATH') + ":" + mdlint_bin + ":" + mdspell_bin

# S2E repos that need to be pinned to a specific commit (see issue #164)
repo_s2e_commit               := "5349d353f4455f5a120d0c8d9790f742c656a706"
repo_s2e_linux_kernel_commit  := "1e2dfecfc6a3e70e7dea478184aa1f13653dcbe1"
repo_decree_commit            := "a523ec2ec1ca1e1369b33db755bed135af57e09c"
repo_guest_images_commit      := "2afd9e4853936c3c38088272e90a927f62c9c58c"
repo_qemu_commit              := "638782a47ed9bb3f280b57a3627bb4e11b2a9cf1"
repo_scripts_commit           := "3e6e6cbffcfe2ea7f5b823d2d5509838a54b89c9"
########## End: Environment and Recipe Variables ##########


########## Section: Installation Recipes ##########

# Install BinRec from a clean clone of repository
install-binrec: _install-dependencies _binrec-init build-all build-s2e-image build-docs

# Install apt packages and git LFS. Required once before build. Requires super user privileges.
_install-dependencies:
    sudo apt-get update
    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install -y bison cmake flex g++ g++-multilib gcc gcc-multilib git libglib2.0-dev enchant-2 python3-enchant liblua5.1-dev \
        libsigc++-2.0-dev lua5.3 nasm nlohmann-json3-dev pkg-config subversion curl pipenv git-lfs doxygen graphviz \
        binutils libc6-dbg:i386 \
        python3.9-dev python3.9-venv # For s2e-env (and compatibility with Python 3.9 from Pipfile): http://s2e.systems/docs/s2e-env.html#id2

    git lfs install
    "{{justdir}}/get_nvm.sh"

# Initialize BinRec, S2E, LFS, pipenv, submodules. Required once before build. Requires super user privileges.
_binrec-init:
    -sudo chmod ugo+r /boot/vmlinu*
    test -f Pipfile.lock || pipenv lock --dev
    pipenv sync --dev
    git submodule update --recursive --init
    git lfs pull
    ln -sf "{{justdir}}/google.py" "{{justdir}}/s2e-env/s2e_env/utils/google.py"
    cd ./s2e-env && pipenv run pip install .
    pipenv run s2e init {{justdir}}/s2e
    @just _freeze-s2e
    sed -i 's|git://git.qemu.org|https://gitlab.com/qemu-project|g' "{{justdir}}/s2e/source/qemu/.gitmodules"
    sed -i 's|git://git.qemu-project.org|https://gitlab.com/qemu-project|g' "{{justdir}}/s2e/source/qemu/.gitmodules"
    @just s2e-insert-binrec-plugins

# Freeze all S2E repositories to commits that have been tested against
_freeze-s2e:
    git -C "{{justdir}}/s2e/source/s2e" checkout {{repo_s2e_commit}}
    git -C "{{justdir}}/s2e/source/s2e-linux-kernel" checkout {{repo_s2e_linux_kernel_commit}}
    git -C "{{justdir}}/s2e/source/decree" checkout {{repo_decree_commit}}
    git -C "{{justdir}}/s2e/source/guest-images" checkout {{repo_guest_images_commit}}
    git -C "{{justdir}}/s2e/source/qemu" checkout {{repo_qemu_commit}}
    git -C "{{justdir}}/s2e/source/qemu" submodule update
    git -C "{{justdir}}/s2e/source/scripts" checkout {{repo_scripts_commit}}



########## End: Installation Recipes ##########

########## Section: Build Recipes ##########

# Builds BinRec and S2E from scratch. Takes a long time (~1 hour).
build-all: _s2e-build build-binrec

# Cleans and re-builds BinRec from scratch. This takes a long time (~1 hour).
rebuild-all: clean-all build-all

# Build all of s2e. Takes a long time (~1 hour). Should only be needed if S2E is updated
_s2e-build:
  @just _s2e-command build

# Builds/Re-builds BinRec. Use for rebuilding after modifying merge or lift components
build-binrec:
    mkdir -p build
    cd build && cmake .. && make -j4

# Cleans out a previous BinRec and s2e build, including s2e dependencies.
clean-all: clean-binrec _clean-s2e

# Remove Binrec build directory
clean-binrec:
    rm -rf build

# Removes S2E build directory
_clean-s2e:
    rm -rf ./s2e/build

# Build an s2e image. Default is x86 Debian-9.2.1.
build-s2e-image image="debian-9.2.1-i386":
  @just _s2e-command image_build -d \"{{image}}\"

# This will trigger a rebuild of libs2e, which contains the plugins
rebuild-s2e-plugins:
  @just _s2e-command build -r libs2e-release

# Inserts a binrec plugin into the correct location within the S2E repository
_s2e-insert-binrec-plugin name:
  # If we do not drop existing links it will overwrite with default plugin content
  rm -f "{{plugins_dir}}/{{name}}.cpp"
  rm -f "{{plugins_dir}}/{{name}}.h"
  @just _s2e-command new_plugin --author-name \"{{binrec_authors}}\" --force \"{{name}}\"
  ln -f -s "{{justdir}}/{{name}}.cpp" "{{plugins_dir}}/{{name}}.cpp"
  ln -f -s "{{justdir}}/{{name}}.h" "{{plugins_dir}}/{{name}}.h"

#  Adds the binrec-plugins to the s2e-plugins structure
s2e-insert-binrec-plugins:
  # Regular plugins
  @just _s2e-insert-binrec-plugin "binrec_plugins/ELFSelector"
  @just _s2e-insert-binrec-plugin "binrec_plugins/Export"
  @just _s2e-insert-binrec-plugin "binrec_plugins/ExportELF"
  @just _s2e-insert-binrec-plugin "binrec_plugins/FunctionLog"

  # Special handling (e.g. only header, or other directory layout)

  # TODO (hbrodin): Not very proud of this structure. Any way of cleaning it?
  rm -f {{plugins_dir}}/binrec_traceinfo/src/trace_info.cpp
  @just _s2e-command new_plugin --author-name \"{{binrec_authors}}\" --force \"binrec_traceinfo/src/trace_info\"
  ln -s -f "{{justdir}}/binrec_traceinfo/src/trace_info.cpp" "{{plugins_dir}}/binrec_traceinfo/src/"
  rm -f "{{plugins_dir}}/binrec_traceinfo/src/trace_info.h"
  ln -s -f "{{justdir}}/binrec_traceinfo/include" "{{plugins_dir}}/binrec_traceinfo/"
  grep -F "s2e/Plugins/binrec_traceinfo/include/" "{{plugins_cmake}}" || \
    echo "\ntarget_include_directories (s2eplugins PUBLIC \"s2e/Plugins/binrec_traceinfo/include/\")" >> "{{plugins_cmake}}"

  ln -s -f "{{justdir}}/binrec_plugins/util.h" "{{plugins_dir}}/binrec_plugins"
  ln -s -f "{{justdir}}/binrec_plugins/ModuleSelector.h" "{{plugins_dir}}/binrec_plugins"

# Execute a s2e command with the s2e environment active
_s2e-command command *args:
  pipenv run s2e {{command}} {{args}}

# Rebuild the libc-argsizes database
rebuild-libc-argsizes:
  pipenv run python -m binrec.sigs "{{env_var('BINREC_LIBC_MODULE')}}" "{{justdir}}/runlib/libc-argsizes"

########## End: Build Recipes ##########

########## Section: pipenv Recipes ##########

# Remove pipenv virtual environment
_pipenv-clear:
  pipenv --rm || true

# Remove everything with pipenv (except Pipfile)
_pipenv-full-reset: _pipenv-clear
  rm -f Pipfile.lock

# Update the pipenv virtual environment
_pipenv-update:
  pipenv update

########## End: pipenv Recipes ##########


########## Section: End-User API Recipes ##########

# Create a new analysis project, optionally providing an existing campaign file to copy
new-project name binary template="":
  pipenv run python -m binrec.project new "{{name}}" "{{binary}}" "{{template}}"

# Add a new trace to an existing project
add-trace project-name trace-name symbolic-indexes args:
  pipenv run python -m binrec.project add-trace --name "{{trace-name}}" --symbolic-indexes "{{symbolic-indexes}}" "{{project-name}}" {{args}}

# Set the stdin content for a single trace
set-trace-stdin project-name trace-name stdin:
    pipenv run python -m binrec.project set-trace-stdin "{{project-name}}" "{{trace-name}}" "{{stdin}}"

# Add a new input file to an existing trace
add-trace-input-file project-name trace-name source destination="" permissions="":
    pipenv run python -m binrec.project add-trace-input-file "{{project-name}}" "{{trace-name}}" "{{source}}"

# Add a new setup command to an existing trace
add-trace-setup project-name trace-name script:
    pipenv run python -m binrec.project add-trace-setup "{{project-name}}" "{{trace-name}}" "{{script}}"

# Add a new teardown command to an existing trace
add-trace-teardown project-name trace-name script:
    pipenv run python -m binrec.project add-trace-teardown "{{project-name}}" "{{trace-name}}" "{{script}}"

# Remove an input file by absolute path or filename from an existing trace
remove-trace-input-file project-name trace-name source:
    pipenv run python -m binrec.project remove-trace-input-file "{{project-name}}" "{{trace-name}}" "{{source}}"


# Remove a trace by name or id from an existing project
remove-trace project-name trace-name:
  pipenv run python -m binrec.project remove-trace "{{project-name}}" "{{trace-name}}"

# Remove all traces from an existing project
remove-all-traces project-name:
  pipenv run python -m binrec.project remove-trace "{{project-name}}" --all

# Run all project traces
run project-name:
  pipenv run python -m binrec.project run "{{project-name}}"

# Run a single project trace by name or id
run-trace project-name trace-name:
  pipenv run python -m binrec.project run-trace "{{project-name}}" "{{trace-name}}"

# Validate the recovered binary against an entire project
validate project-name:
  pipenv run python -m binrec.project validate "{{project-name}}"

# Validate the recovered binary against a single trace by name or id
validate-trace project-name trace-name:
  pipenv run python -m binrec.project validate-trace "{{project-name}}" "{{trace-name}}"

validate-args project-name *args:
  pipenv run python -m binrec.project validate-args "$@"

# Describe the project and all registered traces
describe project-name:
  pipenv run python -m binrec.project describe "{{project-name}}"

# Recursively merge all captures and traces for a project
merge-traces project:
  pipenv run python -m binrec.merge "{{project}}"

# Lift a recovered binary from a project's merged traces. Add -o to perform extra optimizations.
lift-trace project *flags:
  pipenv run python -m binrec.lift  "{{project}}" {{flags}}

recover project-name:
  @just run "{{project-name}}"
  @just merge-traces "{{project-name}}"
  @just lift-trace "{{project-name}}"
  @just validate "{{project-name}}"

clear-trace-data project-name:
  pipenv run python -m binrec.project clear-trace-data "{{project-name}}"

list-projects:
  pipenv run python -m binrec.project list-projects

_launch-web-browser-with-delay delay:
    @/bin/sh -c "sleep {{delay}} && xdg-open http://localhost:8080" > /dev/null 2> /dev/null &

# Starts BinRec's web server and GUI, accessible via browser at http://localhost:8080
run-web-server:
  @just _launch-web-browser-with-delay 3
  pipenv run python -m binrec.web

# Lint a campaign file or directory and display validation errors
lint-campaign filename:
    pipenv run python -m binrec.campaign lint "{{filename}}"

########## End: End-User API Recipes ##########


########## Section: Code Formatting Recipes ##########

# Format all code
format: _format-python _format-clang _format-markdown

# Format Python Code
_format-python: _format-black _format-isort

# Format Python code with black
_format-black:
  pipenv run black binrec

# Format Python import order with isort
_format-isort:
  pipenv run isort binrec

# Runs clang-format linting on C++ code
_format-clang:
  @just _format-clang-dir binrec_lift
  @just _format-clang-dir binrec_link
  @just _format-clang-dir binrec_plugins
  @just _format-clang-dir binrec_rt
  @just _format-clang-dir binrec_traceinfo
  @just _format-clang-dir binrec_tracemerge

# Run clang-format linting recursively on a directory
_format-clang-dir dirname:
  find {{dirname}} -iname \*.cpp -or -iname \*.hpp -or -iname \*.h | xargs -n1 clang-format -Werror -i

_format-markdown:
    . "$HOME/.nvm/nvm.sh" && nvm exec {{nvm_node_version}} npx markdownlint --fix {{docs_md}}

# Runs linting checks
lint: _lint-python _lint-clang _lint-md _lint-rst

# Runs linting checks for Python code
_lint-python: _lint-mypy _lint-black _lint-flake8 _lint-isort

# Runs Python static code checking with flake8
_lint-flake8:
  # E203 conflicts with black (whitespace around slice operator)
  # W503 conflicts with black (binary operator at start of line)
  pipenv run flake8 --max-line-length 88 --ignore E203,W503 binrec

# Runs Python type checking with mypy
_lint-mypy:
  pipenv run mypy binrec

# Runs Python code format and style checks with black
_lint-black:
  pipenv run black --check binrec

# Runs Python import sort order format check with isort
_lint-isort:
  pipenv run isort --check binrec

# Runs linting checks for C++ code
_lint-clang:
  @just _lint-clang-dir binrec_lift
  @just _lint-clang-dir binrec_link
  @just _lint-clang-dir binrec_plugins
  @just _lint-clang-dir binrec_rt
  @just _lint-clang-dir binrec_traceinfo
  @just _lint-clang-dir binrec_tracemerge

# Run clang-format linting recursively on a directory
_lint-clang-dir dirname:
  find {{dirname}} -iname \*.cpp -or -iname \*.hpp -or -iname \*.h | xargs -n1 clang-format -Werror --dry-run

# Run linting checks for markdown
_lint-md: _lint-md-format _lint-md-spell

_lint-md-format:
  . "$HOME/.nvm/nvm.sh" && nvm exec {{nvm_node_version}} npx markdownlint {{docs_md}}

_lint-md-spell:
  . "$HOME/.nvm/nvm.sh" && nvm exec {{nvm_node_version}} npx mdspell -r -n -a --en-us {{docs_md}}

# Run linting checks for restructured text
_lint-rst: _lint-rst-format _lint-rst-spell

_lint-rst-format:
  pipenv run doc8 docs/source

_lint-rst-spell:
  pipenv run sphinx-build -b spelling docs/source docs/build

########## End: Code Formatting Recipes ##########


########## Section: Documentation Recipes ##########
# Build all documentation
build-docs: _build-python-docs _build-cpp-docs

# Build C++ documentation (doxygen)
_build-cpp-docs:
  cd build && cmake --build . --target doxygen

# Build Python documentation (sphinx)
_build-python-docs target="html":
  cd docs && pipenv run sphinx-build -M {{target}} source build

# Clean built Sphinx documentation
_clean-python-docs:
  @just _build-python-docs clean

########## End: Documentation Recipes ##########


########## Section: Testing Recipes ##########

# Runs all unit and integration tests, which may take several minutes to complete
run-all-tests: erase-test-coverage _unit-test-python _unit-test-cpp _integration-test-python print-coverage-report

# Runs unit tests
run-unit-tests: erase-test-coverage _unit-test-python _unit-test-cpp

# Runs Python unit tests
_unit-test-python:
  pipenv run coverage run --append --source=binrec --omit binrec/lib.py -m pytest --verbose -k "not test_integration"

# Runs C++ unit tests
_unit-test-cpp:
  cd build && ctest

# Runs integration tests, which may take several minutes to complete
run-integration-tests: erase-test-coverage _integration-test-python

_integration-test-python:
  pipenv run coverage run --append --source=binrec -m pytest --verbose -k  "test_integration" -rs

# Print the last test run code coverage report
print-coverage-report:
  pipenv run coverage report -m

# Erase the last test code coverage report
erase-test-coverage:
  pipenv run coverage erase

# Unit test command for Github CI
_ci-unit-tests: _unit-test-python print-coverage-report


########## End: Testing Recipes ##########

########## Section: Dev/Debug Recipes ##########

# Pretty print a trace info file
_pretty-print-trace-info ti-file *flags:
  pipenv run python -m binrec.trace_info pretty "{{ti-file}}" {{flags}}

# Finds the differences in two trace info files (add --hex or -x to print address in hex)
_diff-trace-info ti-file-1 ti-file-2 *flags:
  pipenv run python -m binrec.trace_info diff "{{ti-file-1}}" "{{ti-file-2}}" {{flags}}

########## Section: Dev/Debug Recipes ##########



########## Section: s2eout Recipes ##########
########## For working with s2eout outputs, as a replacement for the old s2eout_makefile

_s2eout_main_addrs binary:
  bash ${S2EDIR}/scripts/utils/main-addrs.sh "{{binary}}" > main_addrs

_s2eout_coverage:
  bash ${S2EDIR}/scripts/utils/coverage.sh .

########## End: s2eout Recipes ##########

########## Section: Deprecated Recipes ##########

# This recipe was originally used for setting up the host machine's network to allow connection to the QEMU VM.
# This facilitated VM interop that is now handled by s2e.

# Setup the initial network configuration

# Network variables
#net_iface := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 5`
#net_ipaddr := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 7`
#net_gateway := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 3`

#configure-network:
#  # create the bridge
#  sudo ip link add br0 type bridge
#  # reset the interface
#  sudo ip addr flush dev {{net_iface}}
#  # setup the bridge / tap
#  sudo ip link set {{net_iface}} master br0
#  sudo ip tuntap add dev tap0 mode tap user $USER
#  sudo ip link set tap0 master br0
#  sudo ip link set dev br0 up
#  sudo ip link set dev tap0 up
#  sudo ifconfig br0 {{net_ipaddr}}
#  sudo ifconfig {{net_iface}} 0.0.0.0 promisc
#  sudo route add default gw {{net_gateway}}
#  # copy the dns config from the original adapter to the bridge
#  sudo resolvectl dns br0 `resolvectl dns {{net_iface}} | cut -d':' -f 2`

########## End: Deprecated Recipes ##########
