#!/bin/bash
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

echo "-------------"
echo "Environment: "
echo "-------------"
set
echo "-------------"

die() {
        echo " *** ERROR: " $*
        exit 1
}

if [ -z "$TRAVIS_OS_NAME" ]
then
    die "TRAVIS_OS_NAME is not set"
fi

if [ -z "$BUILD_TARGET" ]
then
    die "BUILD_TARGET" is not set
fi

if [ -z "$PIP" ]
then
    PIP=`which pip`
fi

if [ -z "$PIP" ]
then
    sudo apt-get install python-pip
fi

MY_CWD=/tmp
cd $MY_CWD || die


if [ -z "$CACHED_DIR" ]
then
    die "CACHED_DIR is not set"
fi

# If requested, clear the cached dir
if [ x"$CACHED_DIR_reset" == x"yes" ]
then
    rm -rf ${CACHED_DIR}
fi

# Remember this 
ORIG_PATH=$PATH

mkdir -p $CACHED_DIR

if [ $TRAVIS_OS_NAME == linux ]
then
    sudo apt-get update || die

    sudo -H $PIP install --upgrade pip || die
    $PIP install --upgrade pip || die

    sudo -H $PIP install pexpect || die
    $PIP install pexpect || die

    # Packages used by ncp tools.
    sudo -H $PIP install ipaddress || die
    sudo -H $PIP install scapy==2.3.2 || die
    sudo -H $PIP install pyserial || die
    $PIP install ipaddress || die
    $PIP install scapy==2.3.2 || die
    $PIP install pyserial || die

    if [ $BUILD_TARGET == 'prep-tools' -o $BUILD_TARGET == pretty-check ]
    then
        export PATH=$ASTYLE_path:$PATH || die
        x=`which astyle`
        if [ -x "$x" ]
        then
            echo "Using cached: $x"
        else
            HERE=`pwd`
            cd $CACHED_DIR
            wget $ASTYLE_url || die
            tar xzvf $ASTYLE_file || die
            cd astyle/build/gcc || die
            LDFLAGS=" " make || die
            cd ${HERE}
        fi
        astyle --version || die
        export PATH=$ORIG_PATH
    fi

    if [ $BUILD_TARGET == 'prep-tools' -o $BUILD_TARGET == scan-build ]
    then
        sudo apt-get install clang || die
    fi

    if [ $BUILD_TARGET == 'prep-tools' -o $BUILD_TARGET == arm-gcc49 ]
    then
        export PATH=$ARM_GCC_49_path:$PATH
        x=`which arm-none-eabi-gcc`
        if [ -x "$x" ]
        then
            echo "Using cached: $x"
        else
            HERE=`pwd`
            sudo apt-get install lib32z1 || die
            cd ${CACHED_DIR}
            wget $ARM_GCC_49_url || die
            tar xjf $ARM_GCC_49_file || die
            cd ${HERE}
        fi
        arm-none-eabi-gcc --version || die
        export PATH=$ORIG_PATH
    fi

    if [ $BUILD_TARGET == 'prep-tools' -o $BUILD_TARGET == arm-gcc54 ]
    then
        export PATH=$ARM_GCC_54_path:$PATH || die
        x=`which arm-none-eabi-gcc`
        if [ -x "$x" ]
        then
            echo "Using cached: $x"
        else
            HERE=`pwd`
            cd ${CACHED_DIR}
            sudo apt-get install lib32z1 || die
            wget $ARM_GCC_54_url || die
            tar xjf $ARM_GCC_54_file || die
            cd ${HERE}
        fi
        arm-none-eabi-gcc --version || die
        export PATH=$ORIG_PATH
    fi

    if [ $BUILD_TARGET == 'prep-tools' -o $BUILD_TARGET == posix-32-bit ]
    then
        sudo apt-get install g++-multilib || die
    fi

    if [ $BUILD_TARGET != posix-distcheck ]
    then
        sudo apt-get install clang || die
        sudo apt-get install llvm-3.4-runtime || die
    fi

    if [ $BUILD_TARGET != posix -o $CC != clang ]
    then
        sudo apt-get install clang || die
    fi

    # Packages used by sniffer
    sudo -H pip install pycryptodome==3.4.3 || die
    sudo -H pip install enum34 || die
    pip install pycryptodome==3.4.3 || die
    pip install enum34 || die
fi
    
if [ $TRAVIS_OS_NAME == osx ]
then
    sudo easy_install pexpect || die

    if [ $BUILD_TARGET == cc2538 ]
    then
        wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    fi
fi

