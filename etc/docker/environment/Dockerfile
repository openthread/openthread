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

