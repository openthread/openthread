#  Copyright (c) 2022, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

ARG BASE_IMAGE=ubuntu:focal
FROM ${BASE_IMAGE}

ARG INFRA_IF_NAME
ARG BORDER_ROUTING
ARG BACKBONE_ROUTER
ARG OTBR_OPTIONS
ARG EXTERNAL_COMMISSIONER
ARG DNS64
ARG NAT64
ARG NAT64_SERVICE
ARG REFERENCE_DEVICE
ARG REST_API
ARG WEB_GUI
ARG MDNS

ENV INFRA_IF_NAME=${INFRA_IF_NAME:-eth0}
ENV BORDER_ROUTING=${BORDER_ROUTING:-1}
ENV BACKBONE_ROUTER=${BACKBONE_ROUTER:-1}
ENV OTBR_MDNS=${MDNS:-mDNSResponder}
ENV OTBR_OPTIONS=${OTBR_OPTIONS}
ENV EXTERNAL_COMMISSIONER=${EXTERNAL_COMMISSIONER:-1}
ENV DEBIAN_FRONTEND noninteractive
ENV PLATFORM ubuntu
ENV REFERENCE_DEVICE=${REFERENCE_DEVICE:-0}
ENV NAT64=${NAT64:-1}
ENV NAT64_SERVICE=${NAT64_SERVICE:-tayga}
ENV DNS64=${DNS64:-0}
ENV WEB_GUI=${WEB_GUI:-1}
ENV REST_API=${REST_API:-1}
ENV DOCKER 1

RUN env

COPY . /app
WORKDIR /app

# Required during build or run
ENV OTBR_DOCKER_REQS sudo python2 python3 python-is-python2

# Required during build, could be removed
ENV OTBR_DOCKER_DEPS git ca-certificates python3-pip wget

# Required during run python scripts
ENV OTBR_PYTHON_REQS zeroconf

# Required and installed during build (script/bootstrap), could be removed
ENV OTBR_BUILD_DEPS apt-utils build-essential psmisc ninja-build cmake ca-certificates \
  libreadline-dev libncurses-dev libcpputest-dev libdbus-1-dev libavahi-common-dev \
  libavahi-client-dev libboost-dev libboost-filesystem-dev libboost-system-dev \
  libnetfilter-queue-dev

RUN apt-get update \
  && cp -r ./root/. / \
  && rm -rf ./root \
  && apt-get install --no-install-recommends -y $OTBR_DOCKER_REQS $OTBR_DOCKER_DEPS \
  && wget -P /tmp "https://bootstrap.pypa.io/pip/2.7/get-pip.py" \
  && python2 /tmp/get-pip.py \
  && pip2 install -r /tmp/requirements.txt \
  && pip3 install $OTBR_PYTHON_REQS \
  && ln -fs /usr/share/zoneinfo/UTC /etc/localtime \
  && ([ "${EXTERNAL_COMMISSIONER}" != "1" ] || ( \
    git clone https://github.com/openthread/ot-commissioner.git --recurse-submodules --shallow-submodules --depth=1 \
    && cd ot-commissioner \
    && ./script/bootstrap.sh \
    && mkdir -p build \
    && cd build \
    && cmake -GNinja -DCMAKE_INSTALL_PREFIX="/usr/local" -DOT_COMM_REFERENCE_DEVICE=ON .. \
    && ninja \
    && ninja install \
    && cd /app \
    && rm -rf ot-commissioner \
  )) \
  && ./script/bootstrap \
  && ./script/setup \
  && ([ "${DNS64}" = "0" ] || chmod 644 /etc/bind/named.conf.options) \
  && mv ./script /tmp \
  && mv ./etc /tmp \
  && find . -delete \
  && rm -rf /usr/include \
  && mv /tmp/script . \
  && mv /tmp/etc . \
  && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $OTBR_DOCKER_DEPS \
  && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $OTBR_BUILD_DEPS  \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /tmp/* \
  && sed -i "s/\/root/\/home\/pi/g" /etc/passwd
  # The command above changes root home directory to /home/pi

ENTRYPOINT ["/app/etc/docker/docker_entrypoint.sh"]

EXPOSE 80
