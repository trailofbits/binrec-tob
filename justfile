# Environment Variables (Include in all justfiles that need them)
export S2EDIR := `pwd`
export user := `echo $USER`
export COMPOSE_PROJECT_NAME := user + "_" + S2EDIR

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
    mkdir build
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

# Format code
format:
  pipenv run black .
  # TODO: Eventually format C++ code

# Runs linting checks
lint:
  pipenv run black --check .
  # TODO: Add python linter, C++ formatter and linter?

# Remove pipenv virtual environment
clear:
  pipenv --rm || true

# Remove everything with pipenv (except Pipfile)
full-reset: clear
  rm -f Pipfile.lock

update:
  pipenv update

# BinRec Python API Commands
# TODO