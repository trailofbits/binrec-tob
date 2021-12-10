# Environment Variables (Include in all justfiles that need them)
set dotenv-load := true

# Recipe Variables
net_iface := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 5`
net_ipaddr := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 7`
net_gateway := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 3`

# s2e paths
plugins_root := justfile_directory() + "/s2e/source/s2e/libs2eplugins"
plugins_dir := join(plugins_root, "src/s2e/Plugins")
plugins_cmake := join(plugins_root, "src", "CMakeLists.txt")


# Binrec Build commands
# Install apt packages, RealVNCviewer. Required once before build. Requires super user privileges.
install-dependencies:
    # Packages
    sudo apt-get update
    sudo apt-get install -y bison clang cmake flex g++ g++-multilib gcc gcc-multilib git libglib2.0-dev liblua5.1-dev \
        libsigc++-2.0-dev lld llvm-dev lua5.3 nasm nlohmann-json3-dev pkg-config python2 subversion net-tools curl \
        pipenv git-lfs \
        python3.9-dev python3.9-venv # For s2e-env (and compatibility with Python 3.9 from Pipfile): http://s2e.systems/docs/s2e-env.html#id2
    git lfs install

    # RealVNCviewer
    curl -sSf https://www.realvnc.com/download/file/viewer.files/VNC-Viewer-6.21.406-Linux-x64.deb --output vnc.deb
    sudo apt install ./vnc.deb
    rm vnc.deb

    # Initialize pipenv, submodules, git-lfs
    just init


# Cleans out a previous BinRec build
clean-all:
    rm -rf build

# Builds BinRec from scratch, default number of jobs is 4.
build-all jobs="4":
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
  pipenv run s2e init {{justfile_directory()}}/s2e

# Execute a s2e command with the s2e environment active
s2e-command command *args:
  pipenv run s2e {{command}} {{args}}

# Build all of s2e takes a long time...
s2e-build:
  just s2e-command build

# Build the debian image.
# TODO (hbrodin): This could be parmeterized to allow for different image builds
s2e-image-build:
  echo "Kernels to be readable."
  sudo chmod ugo+r /boot/vmlinu*
  just s2e-command image_build debian-9.2.1-i386

# This will trigger a rebuild of libs2e, which contains the plugins
s2e-rebuild-plugins:
  rm -f s2e/build/stamps/libs2e-release-make
  just s2e-command build

s2e-insert-binrec-plugin name:
  just s2e-command new_plugin --force {{name}}
  cp {{name}}.cpp {{plugins_dir}}/binrec_plugins/
  cp {{name}}.h {{plugins_dir}}/binrec_plugins/


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
  just s2e-command new_plugin --force "binrec_traceinfo/src/trace_info"
  cp binrec_traceinfo/src/trace_info.cpp {{plugins_dir}}/binrec_traceinfo/src/
  rm {{plugins_dir}}/binrec_traceinfo/src/trace_info.h
  cp -r binrec_traceinfo/include {{plugins_dir}}/binrec_traceinfo/
  grep -F "s2e/Plugins/binrec_traceinfo/include/" {{plugins_cmake}} || \
    echo "\ntarget_include_directories (s2eplugins PUBLIC \"s2e/Plugins/binrec_traceinfo/include/\")" >> {{plugins_cmake}}

  cp binrec_plugins/util.h {{plugins_dir}}/binrec_plugins
  cp binrec_plugins/ModuleSelector.h {{plugins_dir}}/binrec_plugins


# Create a new analysis project
new-project name binary symargs *args:
  pipenv run python -m binrec.project new --bin {{binary}} --sym-args "{{symargs}}" {{name}} {{args}}

run project-name:
  pipenv run python -m binrec.project run {{project-name}}

# Format code
format: format-black format-isort
  # TODO: Eventually format C++ code: clang-format -i DIRNAME

# Format Python code with black
format-black:
  pipenv run black binrec

# Format Python import order with isort
format-isort:
  pipenv run isort binrec

# Runs linting checks
lint: lint-mypy lint-black lint-flake8 lint-isort
  # TODO: Add C++ formatter and linter: clang-format -Werror --dry-run DIRNAME

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

# Build Sphinx Documentation
build-docs target="html":
  cd docs && pipenv run sphinx-build -M {{target}} source build

# Clean built Sphinx documentation
clean-docs:
  just build-docs clean

# Remove pipenv virtual environment
clear:
  pipenv --rm || true

# Remove everything with pipenv (except Pipfile)
full-reset: clear
  rm -f Pipfile.lock

update:
  pipenv update

# Setup the initial network configuration
configure-network:
  # create the bridge
  sudo ip link add br0 type bridge
  # reset the interface
  sudo ip addr flush dev {{net_iface}}
  # setup the bridge / tap
  sudo ip link set {{net_iface}} master br0
  sudo ip tuntap add dev tap0 mode tap user $USER
  sudo ip link set tap0 master br0
  sudo ip link set dev br0 up
  sudo ip link set dev tap0 up
  sudo ifconfig br0 {{net_ipaddr}}
  sudo ifconfig {{net_iface}} 0.0.0.0 promisc
  sudo route add default gw {{net_gateway}}
  # copy the dns config from the original adapter to the bridge
  sudo resolvectl dns br0 `resolvectl dns {{net_iface}} | cut -d':' -f 2`


# Runs all unit and integration tests, which may take several minutes to complete
run-tests: erase-test-coverage run-unit-tests run-integration-tests


# Runs unit tests
run-unit-tests: && run-test-coverage-report
  pipenv run coverage run --source=binrec -m pytest --verbose -k "not test_integration"


# Runs integration tests, which may take several minutes to complete
run-integration-tests:
  pipenv run coverage run --source=binrec -m pytest --verbose -k "test_integration"


# Print the last test run code coverage report
run-test-coverage-report:
  pipenv run coverage report -m


# Erase the last test code coverage report
erase-test-coverage:
  pipenv run coverage erase


# BinRec Python API Commands

# Merge cpatures within a single trace (ex. hello-1)
merge-captures project trace_id:
  pipenv run python -m binrec.merge --trace-id {{trace_id}}

# Recursively merge all captures and traces for a project (ex. hello)
merge-traces project *trace_id:
  pipenv run python -m binrec.merge --project-name {{project}} {{trace_id}}

# Lift a recovered binary from a trace (ex. hello)
lift-trace project *index:
  pipenv run python -m binrec.lift {{project}} {{index}}

list-projects:
  pipenv run python -m binrec.project list

list-traces project:
  pipenv run python -m binrec.project list-traces {{project}}

list-merged project:
  pipenv run python -m binrec.project list-merged {{project}}
