# Ubuntu image with tools required to build OpenThread
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

# Install dependencies:
RUN apt-get update -qq

# Install packages needed for wpantund build and runtime:
RUN apt-get install -y build-essential git make autoconf \
                       autoconf-archive automake dbus libtool gcc \
                       g++ gperf flex bison texinfo ncurses-dev \
                       libexpat-dev python sed python-pip gawk \
                       libreadline6-dev libdbus-1-dev \
                       libboost-dev inetutils-ping

RUN apt-get install -y --force-yes gcc-arm-none-eabi
RUN pip install pexpect

# Install wpantund:
RUN mkdir -p ~/src && \
    cd ~/src && \
    git clone --recursive https://github.com/openthread/wpantund.git && \
    cd wpantund && \
    git checkout full/master && \
    ./configure --sysconfdir=/etc && \
    make && make install

RUN mkdir -p /dev/net && mknod /dev/net/tun c 10 200 && chmod 600 /dev/net/tun

# Restart dbus
RUN service dbus restart

# Install OpenThread
RUN cd ~/src && \
    git clone --recursive https://github.com/openthread/openthread.git && \
    cd openthread && \
    ./bootstrap && \
    make -f examples/Makefile-posix
