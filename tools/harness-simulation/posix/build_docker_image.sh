#!/bin/bash
#
# Copyright (c) 2022, The OpenThread Authors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# NOTE: This script has not been fully tested up to now

set -euxo pipefail

if [[ $OT_PATH == "" ]]; then
    OT_PATH="/home/pi/repo/openthread"
fi

if [[ $OTBR_DOCKER_IMAGE == "" ]]; then
    OTBR_DOCKER_IMAGE="openthread/otbr:reference-device-12"
fi

git clone --recursive https://github.com/openthread/ot-br-posix.git
pushd ot-br-posix

DOCKER_BUILD_OTBR_OPTIONS=(
    "-DOTBR_DUA_ROUTING=ON"
    "-DOT_DUA=ON"
    "-DOT_MLR=ON"
    "-DOT_THREAD_VERSION=1.2"
    "-DOT_SIMULATION_MAX_NETWORK_SIZE=64"
)

# Use system V `service` command instead
ETC_PATH="${OT_PATH}/tools/harness-simulation/posix/etc"
mkdir -p root/etc/init.d
cp "${ETC_PATH}/commissionerd" root/etc/init.d/commissionerd
sudo chown root:root root/etc/init.d/commissionerd
sudo chmod +x root/etc/init.d/commissionerd

cp "${ETC_PATH}/Dockerfile" etc/docker/Dockerfile
cp "${ETC_PATH}/server" script/server
mkdir -p root/tmp
cp "${ETC_PATH}/requirements.txt" root/tmp/requirements.txt

docker build . \
    -t "${OTBR_DOCKER_IMAGE}" \
    -f etc/docker/Dockerfile \
    --build-arg REFERENCE_DEVICE=1 \
    --build-arg BORDER_ROUTING=0 \
    --build-arg BACKBONE_ROUTER=1 \
    --build-arg NAT64=0 \
    --build-arg WEB_GUI=0 \
    --build-arg REST_API=0 \
    --build-arg OTBR_OPTIONS="'${DOCKER_BUILD_OTBR_OPTIONS[*]}'"

popd
rm -rf ot-br-posix
