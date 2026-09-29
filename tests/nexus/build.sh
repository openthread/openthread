#!/bin/bash
#
#  Copyright (c) 2024, The OpenThread Authors.
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

display_usage()
{
    echo ""
    echo "Nexus Build script"
    echo ""
    echo "Usage: $(basename "$0") [config]"
    echo "    <config> can be:"
    echo "        (no arg)        : Build OpenThread Nexus platform with default config"
    echo "        trel            : Build OpenThread Nexus platform with TREL radio (no 15.4)"
    echo "        wasm            : Build OpenThread Nexus platform for WebAssembly"
    echo "        long_routes     : Build OpenThread Nexus platform with LONG_ROUTES feature enabled"
    echo "        no_tests        : Build OpenThread Nexus platform without test executables"
    echo ""
}

die()
{
    echo " *** ERROR: " "$*"
    exit 1
}

cd "$(dirname "$0")" || die "cd failed"
cd ../.. || die "cd failed"

top_srcdir=$(pwd)

if [ -n "${top_builddir}" ]; then
    mkdir -p "${top_builddir}"
else
    top_builddir=.
fi

if [ "$#" -gt 1 ]; then
    display_usage
    exit 1
fi

long_routes=OFF
build_tests=ON

case $1 in
    trel)
        fifteenfour=OFF
        wasm=OFF
        ;;
    wasm)
        fifteenfour=ON
        wasm=ON
        ;;
    long_routes)
        fifteenfour=ON
        wasm=OFF
        long_routes=ON
        ;;
    no_tests)
        fifteenfour=ON
        wasm=OFF
        build_tests=OFF
        ;;
    "")
        fifteenfour=ON
        wasm=OFF
        ;;
    help | --help | -h)
        display_usage
        exit 0
        ;;
    *)
        echo "Error: Unknown configuration \"$1\""
        display_usage
        exit 1
        ;;
esac

echo "===================================================================================================="
echo "Building OpenThread Nexus test platform"
echo "===================================================================================================="
cd "${top_builddir}" || die "cd failed"

CMAKE_ARGS=(
    -GNinja
    -DOT_PLATFORM=nexus
    -DOT_COMPILE_WARNING_AS_ERROR=ON
    -DOT_THREAD_VERSION=1.4
    -DOT_APP_CLI=OFF
    -DOT_APP_NCP=OFF
    -DOT_APP_RCP=OFF
    -DOT_15_4="${fifteenfour}"
    -DOT_MLE_LONG_ROUTES="${long_routes}"
    -DOT_NEXUS_BUILD_TESTS="${build_tests}"
    -DOT_PROJECT_CONFIG="${top_srcdir}/tests/nexus/openthread-core-nexus-config.h"
)

if [ "${wasm}" = "ON" ]; then
    emcmake cmake "${CMAKE_ARGS[@]}" "${top_srcdir}" || die
else
    cmake "${CMAKE_ARGS[@]}" -DOT_NEXUS_GRPC="${OT_NEXUS_GRPC:-OFF}" "${top_srcdir}" || die
fi

ninja || die

exit 0
