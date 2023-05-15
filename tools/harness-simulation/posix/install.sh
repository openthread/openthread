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

set -euxo pipefail

POSIX_DIR="$(cd "$(dirname "$0")" && pwd)"
OT_DIR="${POSIX_DIR}/../../.."
ETC_DIR="${POSIX_DIR}/etc"
SNIFFER_DIR="${POSIX_DIR}/sniffer_sim"

PACKAGES=(
    "docker.io"
    "git"
    "jq"
    "socat"
    "tshark"
)

sudo apt install -y "${PACKAGES[@]}"

pip3 install -r "${POSIX_DIR}/requirements.txt"
python3 -m grpc_tools.protoc -I"${SNIFFER_DIR}" --python_out="${SNIFFER_DIR}" --grpc_python_out="${SNIFFER_DIR}" proto/sniffer.proto

CONFIG_NAME=${1:-"${POSIX_DIR}/config.yml"}
# convert YAML to JSON
CONFIG=$(python3 -c 'import json, sys, yaml; print(json.dumps(yaml.safe_load(open(sys.argv[1]))))' "$CONFIG_NAME")

MAX_NETWORK_SIZE=$(jq -r '.ot_build.max_number' <<<"$CONFIG")

build_ot()
{
    # SC2155: Declare and assign separately to avoid masking return values
    local target build_dir cflags version options
    target="ot-cli-ftd"
    build_dir=$(jq -r '.subpath' <<<"$1")
    cflags=$(jq -r '.cflags | join(" ")' <<<"$1")
    version=$(jq -r '.version' <<<"$1")
    options=$(jq -r '.options | join(" ")' <<<"$1")
    # Intended splitting of options
    read -ra options <<<"$options"

    (
        cd "$OT_DIR"

        OT_CMAKE_NINJA_TARGET="$target" \
            OT_CMAKE_BUILD_DIR="$build_dir" \
            CFLAGS="$cflags" \
            CXXFLAGS="$cflags" \
            script/cmake-build \
            simulation \
            "${options[@]}" \
            -DOT_THREAD_VERSION="$version" \
            -DOT_SIMULATION_MAX_NETWORK_SIZE="$MAX_NETWORK_SIZE"
    )
}

build_otbr()
{
    # SC2155: Declare and assign separately to avoid masking return values
    local target build_dir version rcp_options
    target="ot-rcp"
    build_dir=$(jq -r '.rcp_subpath' <<<"$1")
    version=$(jq -r '.version' <<<"$1")
    rcp_options=$(jq -r '.rcp_options | join(" ")' <<<"$1")
    # Intended splitting of rcp_options
    read -ra rcp_options <<<"$rcp_options"

    (
        cd "$OT_DIR"

        OT_CMAKE_NINJA_TARGET="$target" \
            OT_CMAKE_BUILD_DIR="$build_dir" \
            script/cmake-build \
            simulation \
            "${rcp_options[@]}" \
            -DOT_THREAD_VERSION="$version" \
            -DOT_SIMULATION_MAX_NETWORK_SIZE="$MAX_NETWORK_SIZE"
    )

    # SC2155: Declare and assign separately to avoid masking return values
    local otbr_docker_image build_args options
    otbr_docker_image=$(jq -r '.docker_image' <<<"$1")
    build_args=$(jq -r '.build_args | map("--build-arg " + .) | join(" ")' <<<"$1")
    # Intended splitting of build_args
    read -ra build_args <<<"$build_args"
    options=$(jq -r '.options | join(" ")' <<<"$1")

    local otbr_options=(
        "$options"
        "-DOT_THREAD_VERSION=$version"
        "-DOT_SIMULATION_MAX_NETWORK_SIZE=$MAX_NETWORK_SIZE"
    )

    docker build . \
        -t "${otbr_docker_image}" \
        -f "${ETC_DIR}/Dockerfile" \
        "${build_args[@]}" \
        --build-arg OTBR_OPTIONS="${otbr_options[*]}"
}

for item in $(jq -c '.ot_build.ot | .[]' <<<"$CONFIG"); do
    build_ot "$item"
done

git clone https://github.com/openthread/ot-br-posix.git --recurse-submodules --shallow-submodules --depth=1
(
    cd ot-br-posix
    # Use system V `service` command instead
    mkdir -p root/etc/init.d
    cp "${ETC_DIR}/commissionerd" root/etc/init.d/commissionerd
    sudo chown root:root root/etc/init.d/commissionerd
    sudo chmod +x root/etc/init.d/commissionerd

    cp "${ETC_DIR}/server.patch" script/server.patch
    patch script/server script/server.patch
    mkdir -p root/tmp
    cp "${ETC_DIR}/requirements.txt" root/tmp/requirements.txt

    for item in $(jq -c '.ot_build.otbr | .[]' <<<"$CONFIG"); do
        build_otbr "$item"
    done
)
rm -rf ot-br-posix
