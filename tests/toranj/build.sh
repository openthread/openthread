#!/bin/sh
#
#  Copyright (c) 2018, The OpenThread Authors.
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

cd $(dirname $0)
cd ../..

display_usage() {
    echo ""
    echo "Toranj Build script "
    echo ""
    echo "Usage: $(basename $0) [options] <config>"
    echo "    <config> can be:"
    echo "        ncp        : Build OpenThread NCP FTD mode with POSIX platform"
    echo "        rcp        : Build OpenThread RCP (NCP in radio mode) with POSIX platform"
    echo "        posix-app  : Build OpenThread POSIX App NCP"
    echo ""
    echo "Options:"
    echo "        -c/--enable-coverage  Enable code coverage"
    echo ""
}

die() {
    echo " *** ERROR: " $*
    exit 1
}

coverage=no

while [ $# -ge 2 ]
do
    case $1 in
        -c|--enable-coverage)
            coverage=yes
            shift
            ;;
        *)
            echo "Error: Unknown option \"$1\""
            display_usage
            exit 1
            ;;
    esac
done

if [ "$#" -ne 1 ]; then
    display_usage
    exit 1
fi

build_config=$1

configure_options="                \
    --disable-docs                 \
    --disable-tests                \
    --enable-border-router         \
    --enable-channel-manager       \
    --enable-channel-monitor       \
    --enable-child-supervision     \
    --enable-commissioner          \
    --enable-coverage=$coverage    \
    --enable-diag                  \
    --enable-ftd                   \
    --enable-jam-detection         \
    --enable-legacy                \
    --enable-mac-filter            \
    --enable-ncp                   \
    --enable-service               \
    --with-ncp-bus=uart            \
    "

cppflags_config='-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"../tests/toranj/openthread-core-toranj-config.h\"'

case ${build_config} in
    ncp)
        echo "==================================================================================================="
        echo "Building OpenThread NCP FTD mode with POSIX platform"
        echo "==================================================================================================="
        ./bootstrap || die
        ./configure                             \
            CPPFLAGS="$cppflags_config"         \
            --with-examples=posix               \
            $configure_options || die
        make -j 8 || die
        ;;

    rcp)
        echo "===================================================================================================="
        echo "Building OpenThread RCP (NCP in radio mode) with POSIX platform"
        echo "===================================================================================================="
        ./bootstrap || die
        ./configure                             \
            CPPFLAGS="$cppflags_config"         \
            --enable-coverage=${coverage}       \
            --enable-ncp                        \
            --with-ncp-bus=uart                 \
            --enable-radio-only                 \
            --with-examples=posix               \
            --disable-docs                      \
            --disable-tests || die
        make -j 8 || die
        ;;

    posix-app|posixapp)
        echo "===================================================================================================="
        echo "Building OpenThread POSIX App NCP"
        echo "===================================================================================================="
        ./bootstrap || die
        ./configure                             \
            CPPFLAGS="$cppflags_config -DOPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE=1" \
            --enable-posix-app                  \
            $configure_options || die
        make -j 8 || die
        ;;

    *)
      echo "Error: Unknown configuration \"$1\""
      display_usage
      exit 1
      ;;
esac

exit 0
