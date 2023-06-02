# Ubuntu image with tools required to build OpenThread
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive
ENV LANG en_US.UTF-8

RUN set -x \
    && apt-get update -y \
    && apt-get install -y locales \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8 \
    && apt-get --no-install-recommends install -fy \
        bzip2 \
        git \
        ninja-build \
        python3 \
        python3-pip \
        python3-setuptools \
        software-properties-common \
        sudo \
        netbase \
        inetutils-ping \
        ca-certificates \
    && update-ca-certificates \
    && python3 -m pip install -U pip \
    && python3 -m pip install -U cmake \
    && python3 -m pip install wheel

# set up openthread simulation cli example apps 
WORKDIR /
COPY . openthread
RUN set -x \
    && cd openthread \
    && ./script/bootstrap \
    && mkdir build \
    && cd build \
    && cmake -GNinja -DOT_COMMISSIONER=ON -DOT_JOINER=ON -DOT_PLATFORM=simulation .. \
    && ninja

# set up ot-daemon
WORKDIR /
RUN set -x \
    && cd openthread \
    && ./script/cmake-build posix -DOT_DAEMON=ON
