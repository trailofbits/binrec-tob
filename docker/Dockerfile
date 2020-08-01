FROM ubuntu:14.04
MAINTAINER Joe Nash <jmnash@uci.edu>

# S2E build dependencies
RUN echo "deb-src http://archive.ubuntu.com/ubuntu trusty main" >> /etc/apt/sources.list
RUN echo "deb-src http://archive.ubuntu.com/ubuntu/ trusty-updates main" >> /etc/apt/sources.list
RUN echo "deb-src http://security.ubuntu.com/ubuntu trusty-security main" >> /etc/apt/sources.list
RUN apt-get update --fix-missing 
RUN apt-get build-dep -y llvm-toolchain-3.4 qemu \    
    && apt-get install -y build-essential subversion git gettext liblua5.1-0-dev \
    libsdl1.2-dev libsigc++-2.0-dev binutils-dev python-docutils \
    python-pygments nasm libiberty-dev libc6-dev-i386 wget \
    && apt-get install -y gdb vim \
    && apt-get install -y g++-multilib graphviz kpartx mingw32 cmake3 realpath \
    time sudo libgettextpo-dev ninja-build 

# Fix locale
RUN locale-gen en_US.UTF-8

#experimental

# Expose VNC server port
EXPOSE 5900


#it would be great to do the following to support some client programs, but it breaks if you try in in the dockerfile, dont ask me why
#RUN dpkg --add-architecture i386
#RUN apt-get update 
#RUN apt-get upgrade
#RUN apt-get install -y gcc-multilib libc6-dbg:i386 libssl-dev:i386
#
