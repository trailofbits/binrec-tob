# Environment Variables (Include in all justfiles that need them)
export S2EDIR := `pwd`
export user := `echo $USER`
export COMPOSE_PROJECT_NAME := user + "_" + S2EDIR

# Recipe Variables
net_iface := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 5`
net_ipaddr := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 7`
net_gateway := `ip route get 8.8.8.8 | head -n1 | cut -d' ' -f 3`

# Binrec Build commands
# Install apt packages, RealVNCviewer. Required once before build. Requires super user privileges.
install-dependencies:
    # Packages
    sudo apt-get update
    sudo apt-get install -y bison clang cmake flex g++ g++-multilib gcc gcc-multilib git libglib2.0-dev liblua5.1-dev \
        libsigc++-2.0-dev lld llvm-dev lua5.3 nasm nlohmann-json3-dev pkg-config python2 subversion net-tools curl \
        pipenv

    # RealVNCviewer
    curl -sSf https://www.realvnc.com/download/file/viewer.files/VNC-Viewer-6.21.406-Linux-x64.deb --output vnc.deb
    sudo apt install ./vnc.deb
    rm vnc.deb

    # TODO: Initialize / Build submodules

    # Initialize pipenv
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
# Initialize Python dependencies
init:
    test -f Pipfile.lock || pipenv lock --dev
    pipenv sync --dev
    git submodule update --recursive --init

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
  pipenv run flake8 binrec

# Runs Python type checking with mypy
lint-mypy:
  pipenv run mypy binrec

# Runs Python code format and style checks with black
lint-black:
  pipenv run black --check binrec

# Runs Python import sort order format check with isort
lint-isort:
  pipenv run isort --check binrec

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
  sudo ip tuntap add dev tap0 mode tap user {{user}}
  sudo ip link set tap0 master br0
  sudo ip link set dev br0 up
  sudo ip link set dev tap0 up
  sudo ifconfig br0 {{net_ipaddr}}
  sudo ifconfig {{net_iface}} 0.0.0.0 promisc
  sudo route add default gw {{net_gateway}}
  # copy the dns config from the original adapter to the bridge
  sudo resolvectl dns br0 `resolvectl dns {{net_iface}} | cut -d':' -f 2`


# Runs all tests
run-tests:
  pipenv run pytest --verbose

# BinRec Python API Commands
# TODO
