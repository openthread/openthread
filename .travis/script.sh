#! /bin/bash
#----------------------------------------
# NOTE: this uses and required $BASH_SOURCE
#       Hence this is a bash script, not a 'sh' script
#----------------------------------------
#
#  Copyright (c) 2016, The OpenThread Authors.
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

set -x

# See:
# http://stackoverflow.com/questions/59895/getting-the-current-present-working-directory-of-a-bash-script-from-within-the-s
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. ${DIR}/common.sh

die() {
        echo " *** ERROR: " $*
        exit 1
}

echo "-------------"
echo "Environment: "
echo "-------------"
set
echo "-------------"

if [ -z "$BUILD_TARGET" ]
then
    die "BUILD_TARGET" is not set
fi

if [ $BUILD_TARGET == 'prep-tools' ]
then
    echo "Prep Tools not actually build anything"
    exit 0
fi

[ $BUILD_TARGET != pretty-check ] || {
    git clean -dxf || die
    ./bootstrap || die
    export PATH=$ASTYLE_path:$PATH || die
    ./configure --enable-cli --enable-diag --enable-dhcp6-client --enable-dhcp6-server --enable-commissioner --enable-joiner --with-examples=posix || die
    make pretty-check || die
}

[ $BUILD_TARGET != scan-build ] || {
    git clean -dxf || die
    ./bootstrap || die
    scan-build ./configure --with-examples=posix --enable-cli --enable-ncp || die
    scan-build --status-bugs -analyze-headers -v make || die
}

[ $BUILD_TARGET != arm-gcc49 ] || {
    export PATH=$ARM_GCC_49_path:$PATH || die

    git clean -dxf || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-ftd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-mtd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-ncp || die

    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-ftd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-mtd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-ncp || die
}

[ $BUILD_TARGET != arm-gcc54 ] || {
    export PATH=$ARM_GCC_54_path:$PATH || die

    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-ftd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-mtd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-ncp || die

    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-ftd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-cli-mtd || die
    arm-none-eabi-size  output/bin/arm-none-eabi-ot-ncp || die
}

[ $BUILD_TARGET != posix ] || {
    git clean -xfd || die
    ./bootstrap || die
    if [ -z "$CC" ]
    then
        export CC=gcc
    fi
    if [ -z "$CXX" ]
    then
        export CXX=g++
    fi
    sh -c '$CC --version' || die
    sh -c '$CXX --version' || die
    ./bootstrap || die
    make -f examples/Makefile-posix || die
}

[ $BUILD_TARGET != posix-distcheck ] || {
    export ASAN_SYMBOLIZER_PATH=`which llvm-symbolizer-3.4` || die
    export ASAN_OPTIONS=symbolize=1 || die
    git clean -xfd || die
    ./bootstrap || die
    BuildJobs=10 make -f examples/Makefile-posix distcheck || die
}

[ $BUILD_TARGET != posix-32-bit ] || {
    git clean -xfd || die
    ./bootstrap || die
    COVERAGE=1 CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 BuildJobs=10 make -f examples/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-ncp-spi ] || {
    git clean -xfd || die
    ./bootstrap || die
    BuildJobs=10 make -f examples/Makefile-posix check configure_OPTIONS="--enable-ncp=spi --with-examples=posix --with-platform-info=POSIX" || die
}

[ $BUILD_TARGET != posix-ncp ] || {
    git clean -xfd || die
    ./bootstrap || die
    COVERAGE=1 NODE_TYPE=ncp-sim BuildJobs=10 make -f examples/Makefile-posix check || die
}

