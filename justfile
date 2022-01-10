# Environment Variables (Include in all justfiles that need them)
set dotenv-load := true

# Recipe Variables
net_iface := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 5`
net_ipaddr := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 7`
net_gateway := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 3`


# Binrec Build commands
# Install apt packages, RealVNCviewer. Required once before build. Requires super user privileges.
install-dependencies:
    # Packages
    sudo apt-get update
    sudo apt-get install -y bison clang-12 cmake flex g++ g++-multilib gcc gcc-multilib git libglib2.0-dev liblua5.1-dev \
        libsigc++-2.0-dev lld-12 llvm-12-dev lua5.3 nasm nlohmann-json3-dev pkg-config python2 subversion net-tools curl \
        pipenv git-lfs
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

# Merge cpatures within a single trace (ex. hello-1)
merge-captures trace_id:
  pipenv run python -m binrec.merge --trace-id {{trace_id}}

# Recursively merge all captures and traces for a binary (ex. hello)
merge-traces binary:
  pipenv run python -m binrec.merge --binary {{binary}} --verbose

# Lift a recovered binary from a trace (ex. hello)
lift-trace binary:
  pipenv run python -m binrec.lift {{binary}} --verbose
