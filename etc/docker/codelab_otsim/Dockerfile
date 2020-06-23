# Ubuntu image with tools required to build OpenThread
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

# Install dependencies:
RUN apt-get update -qq

# Install packages needed for build and runtime:
RUN apt-get --no-install-recommends install -y git sudo software-properties-common \
    ca-certificates \
    && update-ca-certificates

# Install OpenThread
RUN mkdir -p ~/src && \
    cd ~/src && \
    git clone --recursive https://github.com/openthread/openthread.git && \
    cd openthread && \
    ./script/bootstrap && \
    ./bootstrap && \
    make -f examples/Makefile-simulation

# Install OpenThread Daemon and ot-ctl
RUN cd ~/src/openthread && make -f src/posix/Makefile-posix DAEMON=1
