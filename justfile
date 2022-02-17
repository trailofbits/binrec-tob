# Environment Variables (Include in all justfiles that need them)
set dotenv-load := true
set positional-arguments := true

# Recipe Variables
net_iface := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 5`
net_ipaddr := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 7`
net_gateway := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 3`
binrec_authors := "Trail of Bits, University of California, Irvine and Vrije Universiteit Amsterdam"

# s2e paths
plugins_root := justfile_directory() + "/s2e/source/s2e/libs2eplugins"
plugins_dir := join(plugins_root, "src/s2e/Plugins")
plugins_cmake := join(plugins_root, "src", "CMakeLists.txt")

justdir := justfile_directory()


# BinRec one-shot install command
install-binrec:
    just install-dependencies
    just build-all
    just build-s2e-image


# Install apt packages, git Large File Storage. Initialize system. Required once before build. Requires super user privileges.
install-dependencies:
    # Packages
    sudo apt-get update

    sudo apt-get install -y bison clang-12 cmake flex g++ g++-multilib gcc gcc-multilib git libglib2.0-dev liblua5.1-dev \
        libsigc++-2.0-dev lld-12 llvm-12-dev lua5.3 nasm nlohmann-json3-dev pkg-config python2 subversion net-tools curl \
        pipenv git-lfs doxygen graphviz \
        clang-format-12 \
        python3.9-dev python3.9-venv # For s2e-env (and compatibility with Python 3.9 from Pipfile): http://s2e.systems/docs/s2e-env.html#id2

    git lfs install

    # Initialize pipenv, submodules, git-lfs
    just init

    # Make kernels readable to support image generation
    sudo chmod ugo+r /boot/vmlinu*


# Cleans out a previous BinRec and s2e build - Note this also wipes out s2e dependencies which take a long time to build.
clean-all:
    rm -rf build
    rm -rf ./s2e/build

# Builds BinRec and s2e from scratch, default number of jobs is 4.
build-all jobs="4":
  just s2e-build
  just build-binrec {{jobs}}

# Builds BinRec from scratch, default number of jobs is 4.
build-binrec jobs="4":
    mkdir -p build
    cd build && cmake .. && make -j{{jobs}}

# Cleans and re-builds BinRec from scratch. This takes a long time (for now).
rebuild-all:
     just clean-all
     just build-all

# TODO Eventually make incremental build commands for just the BinRec pieces

# Pipenv setup /teardown commands
# Initialize Python dependencies, clone submodules, pull lfs files
init:
    test -f Pipfile.lock || pipenv lock --dev
    pipenv sync --dev
    git submodule update --recursive --init
    cd ./test/benchmark && git lfs pull
    just init-s2e-env

# Init the s2e-env, will install dependencies and checkout the s2e repository
init-s2e-env:
  cd ./s2e-env && pipenv run pip install .
  pipenv run s2e init --non-interactive {{justfile_directory()}}/s2e
  just s2e-insert-binrec-plugins

# Execute a s2e command with the s2e environment active
s2e-command command *args:
  pipenv run s2e {{command}} {{args}}

# Build all of s2e takes a long time...
s2e-build:
  just s2e-command build

# Build an s2e image. Default is x86 Debian-9.2.1
# Other (eventually useful) targets: debian-9.2.1-x86_64
build-s2e-image image="debian-9.2.1-i386":
  just s2e-command image_build -d \"{{image}}\"

# This will trigger a rebuild of libs2e, which contains the plugins
s2e-rebuild-plugins:
  rm -f ${S2EDIR}/build/stamps/libs2e-release-install
  cd ${S2EDIR}/build && make -f ../source/s2e/Makefile stamps/libs2e-release-install

s2e-insert-binrec-plugin name:
  # If we don't drop existing links it will overwrite with default plugin
  # content
  rm -f "{{plugins_dir}}/{{name}}.cpp"
  rm -f "{{plugins_dir}}/{{name}}.h"
  just s2e-command new_plugin --author-name \"{{binrec_authors}}\" --force \"{{name}}\"
  ln -f -s "{{justdir}}/{{name}}.cpp" "{{plugins_dir}}/{{name}}.cpp"
  ln -f -s "{{justdir}}/{{name}}.h" "{{plugins_dir}}/{{name}}.h"


#  Adds the binrec-plugins to the s2e-plugins structure
s2e-insert-binrec-plugins:
  # "Regular plugins"
  just s2e-insert-binrec-plugin "binrec_plugins/BBExport"
  just s2e-insert-binrec-plugin "binrec_plugins/ELFSelector"
  just s2e-insert-binrec-plugin "binrec_plugins/Export"
  just s2e-insert-binrec-plugin "binrec_plugins/ExportELF"
  just s2e-insert-binrec-plugin "binrec_plugins/FunctionLog"

  # Special handling (e.g. only header, or other directory layout)

  # TODO (hbrodin): Not very proud of this structure. Any way of cleaning it?
  rm -f {{plugins_dir}}/binrec_traceinfo/src/trace_info.cpp
  just s2e-command new_plugin --author-name \"{{binrec_authors}}\" --force \"binrec_traceinfo/src/trace_info\"
  ln -s -f "{{justdir}}/binrec_traceinfo/src/trace_info.cpp" "{{plugins_dir}}/binrec_traceinfo/src/"
  rm -f "{{plugins_dir}}/binrec_traceinfo/src/trace_info.h"
  ln -s -f "{{justdir}}/binrec_traceinfo/include" "{{plugins_dir}}/binrec_traceinfo/"
  grep -F "s2e/Plugins/binrec_traceinfo/include/" "{{plugins_cmake}}" || \
    echo "\ntarget_include_directories (s2eplugins PUBLIC \"s2e/Plugins/binrec_traceinfo/include/\")" >> "{{plugins_cmake}}"

  ln -s -f "{{justdir}}/binrec_plugins/util.h" "{{plugins_dir}}/binrec_plugins"
  ln -s -f "{{justdir}}/binrec_plugins/ModuleSelector.h" "{{plugins_dir}}/binrec_plugins"


# Create a new analysis project
new-project name binary symargs *args:
  pipenv run python -m binrec.project new "{{name}}" "{{binary}}" "{{symargs}}" {{args}}


# Run an S2E analysis project. Override sample command line arguments by passing "--args ARGS ARGS"
run project-name *args:
  pipenv run python -m binrec.project run "$@"

# Set an S2E analysis project command line arguments.
set-args project-name *args:
  pipenv run python -m binrec.project --verbose set-args "$@"

# Format all code
format: format-python format-clang

# Format Python Code
format-python: format-black format-isort

# Format Python code with black
format-black:
  pipenv run black binrec

# Format Python import order with isort
format-isort:
  pipenv run isort binrec

# Runs clang-format linting on C++ code
format-clang:
  just _format-clang-dir binrec_lift
  just _format-clang-dir binrec_link
  just _format-clang-dir binrec_plugins
  just _format-clang-dir binrec_rt
  just _format-clang-dir binrec_traceinfo
  just _format-clang-dir binrec_tracemerge

# Run clang-format linting recursively on a directory
_format-clang-dir dirname:
  find {{dirname}} -iname \*.cpp -or -iname \*.hpp -or -iname \*.h | xargs -n1 clang-format-12 -Werror -i

# Runs linting checks
lint: lint-python lint-clang

# Runs linting checks for Python code
lint-python: lint-mypy lint-black lint-flake8 lint-isort

# Runs Python static code checking with flake8
lint-flake8:
  pipenv run flake8 --max-line-length 88 binrec

# Runs Python type checking with mypy
lint-mypy:
  pipenv run mypy binrec

# Runs Python code format and style checks with black
lint-black:
  pipenv run black --check binrec

# Runs Python import sort order format check with isort
lint-isort:
  pipenv run isort --check binrec

# Runs linting checks for C++ code
lint-clang:
  just _lint-clang-dir binrec_lift
  just _lint-clang-dir binrec_link
  just _lint-clang-dir binrec_plugins
  just _lint-clang-dir binrec_rt
  just _lint-clang-dir binrec_traceinfo
  just _lint-clang-dir binrec_tracemerge

# Run clang-format linting recursively on a directory
_lint-clang-dir dirname:
  find {{dirname}} -iname \*.cpp -or -iname \*.hpp -or -iname \*.h | xargs -n1 clang-format-12 -Werror --dry-run

# Build all documentation
build-docs: build-python-docs build-cpp-docs

# Build C++ documentation (doxygen)
build-cpp-docs:
  cd build && cmake --build . --target doxygen

# Build Python documentation (sphinx)
build-python-docs target="html":
  cd docs && pipenv run sphinx-build -M {{target}} source build

# Clean built Sphinx documentation
clean-python-docs:
  just build-python-docs clean

# Remove pipenv virtual environment
clear:
  pipenv --rm || true

# Remove everything with pipenv (except Pipfile)
full-reset: clear
  rm -f Pipfile.lock

update:
  pipenv update

# Runs all unit and integration tests, which may take several minutes to complete
run-tests: erase-test-coverage run-unit-tests run-gtest run-integration-tests


# Runs unit tests
run-unit-tests: && run-test-coverage-report
  pipenv run coverage run --source=binrec --omit binrec/lib.py -m pytest --verbose -k "not test_integration"


# Runs integration tests, which may take several minutes to complete
run-integration-tests:
  pipenv run coverage run --source=binrec -m pytest --verbose -k "test_integration"


# Runs C++ unit tests
run-gtest:
  cd build && ctest


# Print the last test run code coverage report
run-test-coverage-report:
  pipenv run coverage report -m


# Erase the last test code coverage report
erase-test-coverage:
  pipenv run coverage erase


# BinRec Python API Commands

# Recursively merge all captures and traces for a project (ex. hello)
merge-traces project:
  pipenv run python -m binrec.merge -vv "{{project}}"

# Lift a recovered binary from a trace (ex. hello)
lift-trace project:
  pipenv run python -m binrec.lift -vv "{{project}}"

list-projects:
  pipenv run python -m binrec.project list

list-traces project:
  pipenv run python -m binrec.project list-traces "{{project}}"

list-merged project:
  pipenv run python -m binrec.project list-merged "{{project}}"



########## DEPRECATED RECIPES ##########

# This recipe was originally used for setting up the host machine's network to allow connection to the QEMU VM.
# This facilitated VM interop that is now handled by s2e.

# Setup the initial network configuration
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
