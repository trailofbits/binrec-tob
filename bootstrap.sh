#!/bin/sh
set -e
sudo apt-get install -y build-essential subversion git gettext liblua5.1-0-dev \
    libsdl1.2-dev libsigc++-2.0-dev binutils-dev python-docutils \
    python-pygments nasm libiberty-dev libc6-dev-i386 \
    g++-multilib graphviz kpartx mingw32 cmake realpath time wget
sudo apt-get build-dep llvm-3.3 qemu
export S2EDIR=`pwd`
