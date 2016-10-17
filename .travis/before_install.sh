#!/bin/sh
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

die() {
	echo " *** ERROR: " $*
	exit 1
}

set -x

cd /tmp || die

[ $TRAVIS_OS_NAME != linux ] || {    
    sudo apt-get update || die

    sudo -H pip install --upgrade pip || die
    pip install --upgrade pip || die

    sudo -H pip install pexpect || die
    pip install pexpect || die

    # Packages used by ncp tools.
    sudo -H pip install ipaddress || die
    sudo -H pip install scapy || die
    sudo -H pip install pyserial || die
    pip install ipaddress || die
    pip install scapy || die
    pip install pyserial || die

    [ $BUILD_TARGET != pretty-check ] || {
        wget http://jaist.dl.sourceforge.net/project/astyle/astyle/astyle%202.05.1/astyle_2.05.1_linux.tar.gz || die
        tar xzvf astyle_2.05.1_linux.tar.gz || die
        cd astyle/build/gcc || die
        LDFLAGS=" " make || die
        cd ../../..
        export PATH=/tmp/astyle/build/gcc/bin:$PATH || die
        astyle --version || die
    }

    [ $BUILD_TARGET != scan-build ] || {
        sudo apt-get install clang || die
    }

    [ $BUILD_TARGET != cc2538 ] || {
        sudo apt-get install lib32z1 || die
        wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }

    [ $BUILD_TARGET != posix-32-bit ] || {
        sudo apt-get install g++-multilib || die
    }
}

[ $TRAVIS_OS_NAME != osx ] || {
    sudo easy_install pexpect || die

    [ $BUILD_TARGET != cc2538 ] || {
        wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }
}
