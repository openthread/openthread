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
        pip install --upgrade pip || die
        pip install -r $TRAVIS_BUILD_DIR/tests/scripts/thread-cert/requirements.txt || die
        [ $BUILD_TARGET != posix-ncp -a $BUILD_TARGET != posix-app-ncp ] || {
            # Packages used by ncp tools.
            pip install git+https://github.com/openthread/pyspinel || die
        }
    }

    [ $BUILD_TARGET != android-build ] || {
        sudo apt-get install -y bison gcc-multilib g++-multilib
        (
        cd $HOME
        wget https://dl.google.com/android/repository/android-ndk-r17c-linux-x86_64.zip
        unzip android-ndk-r17c-linux-x86_64.zip > /dev/null
        mv android-ndk-r17c ndk-bundle
        ) || die
    }

    [ $BUILD_TARGET != pretty-check ] || {
        clang-format --version || die
    }

    [ $BUILD_TARGET != posix-app-pty ] || {
        sudo apt-get install socat expect libdbus-1-dev autoconf-archive || die
        JOBS=$(getconf _NPROCESSORS_ONLN)
        (
        WPANTUND_TMPDIR=/tmp/wpantund
        git clone --depth 1 https://github.com/openthread/wpantund.git $WPANTUND_TMPDIR
        cd $WPANTUND_TMPDIR
        ./bootstrap.sh
        ./configure --prefix= --exec-prefix=/usr --disable-ncp-dummy --enable-static-link-ncp-plugin=spinel
        make -j $JOBS
        sudo make install
        ) || die
        (
        LIBCOAP_TMPDIR=/tmp/libcoap
        mkdir $LIBCOAP_TMPDIR
        cd $LIBCOAP_TMPDIR
        wget https://github.com/obgm/libcoap/archive/bsd-licensed.tar.gz
        tar xvf bsd-licensed.tar.gz
        cd libcoap-bsd-licensed
        ./autogen.sh
        ./configure --prefix= --exec-prefix=/usr --with-boost=internal --disable-tests --disable-documentation
        make -j $JOBS
        sudo make install
        ) || die
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
        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-7-2018-q2-update/bin:$PATH || die
        arm-none-eabi-gcc --version || die
    }

    [ $BUILD_TARGET != arm-gcc-8 ] || {
        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2 || die
        tar xjf gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2 || die
        export PATH=/tmp/gcc-arm-none-eabi-8-2018-q4-major/bin:$PATH || die
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
