#!/bin/bash
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

display_usage()
{
    echo ""
    echo "Toranj Build script "
    echo ""
    echo "Usage: $(basename "$0") [options] <config>"
    echo "    <config> can be:"
    echo "        ncp        : Build OpenThread NCP FTD mode with simulation platform"
    echo "        rcp        : Build OpenThread RCP (NCP in radio mode) with simulation platform"
    echo "        posix      : Build OpenThread POSIX App NCP"
    echo "        cmake      : Configure and build OpenThread using cmake/ninja (RCP and NCP) only"
    echo "        cmake-posix: Configure and build OpenThread POSIX host using cmake/ninja"
    echo ""
    echo "Options:"
    echo "        -c/--enable-coverage  Enable code coverage"
    echo "        -t/--enable-tests     Enable tests"
    echo ""
}

die()
{
    echo " *** ERROR: " "$*"
    exit 1
}

cd "$(dirname "$0")" || die "cd failed"
cd ../.. || die "cd failed"

coverage=no
tests=no

while [ $# -ge 2 ]; do
    case $1 in
        -c | --enable-coverage)
            coverage=yes
            shift
            ;;
        -t | --enable-tests)
            tests=yes
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

configure_options=(
    "--disable-docs"
    "--enable-tests=$tests"
    "--enable-coverage=$coverage"
    "--enable-ftd"
    "--enable-ncp"
)

if [ -n "${top_builddir}" ]; then
    top_srcdir=$(pwd)
    mkdir -p "${top_builddir}"
else
    top_srcdir=.
    top_builddir=.
fi

case ${build_config} in
    ncp)
        echo "==================================================================================================="
        echo "Building OpenThread NCP FTD mode with POSIX platform"
        echo "==================================================================================================="
        ./bootstrap || die "bootstrap failed"
        cd "${top_builddir}" || die "cd failed"
        cppflags_config='-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"../tests/toranj/openthread-core-toranj-config-simulation.h\"'
        ${top_srcdir}/configure \
            CPPFLAGS="$cppflags_config" \
            --with-examples=simulation \
            "${configure_options[@]}" || die
        make -j 8 || die
        ;;

    rcp)
        echo "===================================================================================================="
        echo "Building OpenThread RCP (NCP in radio mode) with POSIX platform"
        echo "===================================================================================================="
        ./bootstrap || die "bootstrap failed"
        cd "${top_builddir}" || die "cd failed"
        cppflags_config='-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"../tests/toranj/openthread-core-toranj-config-simulation.h\"'
        ${top_srcdir}/configure \
            CPPFLAGS="$cppflags_config " \
            --enable-coverage=${coverage} \
            --enable-ncp \
            --enable-radio-only \
            --with-examples=simulation \
            --disable-docs \
            --enable-tests=$tests || die
        make -j 8 || die
        ;;

    posix | posix-app | posixapp)
        echo "===================================================================================================="
        echo "Building OpenThread POSIX App NCP"
        echo "===================================================================================================="
        ./bootstrap || die "bootstrap failed"
        cd "${top_builddir}" || die "cd failed"
        cppflags_config='-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"../tests/toranj/openthread-core-toranj-config-posix.h\"'
        ${top_srcdir}/configure \
            CPPFLAGS="$cppflags_config" \
            --with-platform=posix \
            "${configure_options[@]}" || die
        make -j 8 || die
        ;;

    cmake)
        echo "===================================================================================================="
        echo "Building OpenThread (NCP/CLI for FTD/MTD/RCP mode) with simulation platform using cmake"
        echo "===================================================================================================="
        cd "${top_builddir}" || die "cd failed"
        cmake -GNinja -DOT_PLATFORM=simulation -DOT_COMPILE_WARNING_AS_ERROR=on -DOT_APP_CLI=off \
            -DOT_CONFIG=../tests/toranj/openthread-core-toranj-config-simulation.h \
            "${top_srcdir}" || die
        ninja || die
        ;;

    cmake-posix-host | cmake-posix | cmake-p)
        echo "===================================================================================================="
        echo "Building OpenThread POSIX host platform using cmake"
        echo "===================================================================================================="
        cd "${top_builddir}" || die "cd failed"
        cmake -GNinja -DOT_PLATFORM=posix -DOT_COMPILE_WARNING_AS_ERROR=on -DOT_APP_CLI=off \
            -DOT_CONFIG=../tests/toranj/openthread-core-toranj-config-posix.h \
            "${top_srcdir}" || die
        ninja || die
        ;;

    *)
        echo "Error: Unknown configuration \"$1\""
        display_usage
        exit 1
        ;;
esac

exit 0
