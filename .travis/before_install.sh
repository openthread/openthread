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

    [ $BUILD_TARGET != posix-distcheck -a $BUILD_TARGET != posix-32-bit -a $BUILD_TARGET != posix-app-cli -a $BUILD_TARGET != posix-mtd -a $BUILD_TARGET != posix-ncp -a $BUILD_TARGET != posix-app-ncp ] || {
        pip install --user --upgrade pip || die
        pip install --user -r $TRAVIS_BUILD_DIR/tests/scripts/thread-cert/requirements.txt || die
        [ $BUILD_TARGET != posix-ncp -a $BUILD_TARGET != posix-app-ncp ] || {
            # Packages used by ncp tools.
            pip install --user git+https://github.com/openthread/pyspinel || die
        }
    }

    [ $BUILD_TARGET != android-build ] || {
        echo y | sdkmanager "ndk-bundle"
    }

    [ $BUILD_TARGET != pretty-check ] || {
        clang-format --version || die
    }

    [ $BUILD_TARGET != scan-build ] || {
        sudo apt-get install clang || die
    }

    [ $BUILD_TARGET != arm-gcc-4 ] || {
        sudo apt-get install lib32z1 || die
        wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }

    [ $BUILD_TARGET != arm-gcc-5 ] || {
        sudo apt-get install lib32z1 || die
        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }

    [ $BUILD_TARGET != arm-gcc-6 ] || {
        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-6-2017-q2-update/bin:$PATH || die
        arm-none-eabi-gcc --version || die

        wget https://github.com/foss-for-synopsys-dwc-arc-processors/toolchain/releases/download/arc-2017.03-rc2/arc_gnu_2017.03-rc2_prebuilt_elf32_le_linux_install.tar.gz || die
        tar xzf arc_gnu_2017.03-rc2_prebuilt_elf32_le_linux_install.tar.gz
        export PATH=/tmp/arc_gnu_2017.03-rc2_prebuilt_elf32_le_linux_install/bin:$PATH || die
        arc-elf32-gcc --version || die
    }

    [ $BUILD_TARGET != arm-gcc-7 ] || {
        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2017q4/gcc-arm-none-eabi-7-2017-q4-major-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-7-2017-q4-major-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-7-2017-q4-major/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }

    [ $BUILD_TARGET != posix-32-bit -a $BUILD_TARGET != posix-mtd ] || {
        sudo apt-get install g++-multilib || die
    }

    [ $BUILD_TARGET != posix-distcheck ] || {
        sudo apt-get install clang || die
        sudo apt-get install llvm-3.4-runtime || die
    }

    [ $BUILD_TARGET != posix -o "$CC" != clang ] || {
        sudo apt-get install clang || die
    }

    [ $BUILD_TARGET != toranj-test-framework ] || {
        # packages for wpantund
        sudo apt-get install dbus || die
        sudo apt-get install gcc g++ libdbus-1-dev || die
        sudo apt-get install autoconf-archive || die
        sudo apt-get install bsdtar || die
        sudo apt-get install libtool || die
        sudo apt-get install libglib2.0-dev || die
        sudo apt-get install libboost-dev || die
        sudo apt-get install libboost-signals-dev || die

        # clone and build wpantund
        git clone --depth=1 --branch=master https://github.com/openthread/wpantund.git
        cd wpantund || die
        ./bootstrap.sh || die
        ./configure || die
        sudo make -j 8 || die
        sudo make install || die
        cd .. || die
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
