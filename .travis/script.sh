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

python --version || die

[ $BUILD_TARGET != pretty-check ] || {
    ./bootstrap || die
    ./configure || die
    make pretty-check || die
}

[ $BUILD_TARGET != scan-build ] || {
    ./bootstrap || die

    export CPPFLAGS="-DMBEDTLS_DEBUG_C"

    scan-build ./configure                \
        --enable-application-coap         \
        --enable-application-coap-secure  \
        --enable-border-agent             \
        --enable-border-router            \
        --enable-cert-log                 \
        --enable-channel-manager          \
        --enable-channel-monitor          \
        --enable-child-supervision        \
        --enable-cli                      \
        --enable-commissioner             \
        --enable-dhcp6-client             \
        --enable-dhcp6-server             \
        --enable-diag                     \
        --enable-dns-client               \
        --enable-ecdsa                    \
        --enable-ftd                      \
        --enable-jam-detection            \
        --enable-joiner                   \
        --enable-legacy                   \
        --enable-mac-filter               \
        --enable-mtd                      \
        --enable-mtd-network-diagnostic   \
        --enable-ncp                      \
        --with-ncp-bus=uart               \
        --enable-radio-only               \
        --enable-raw-link-api             \
        --enable-service                  \
        --enable-sntp-client              \
        --enable-udp-forward              \
        --with-examples=posix || die

    scan-build --status-bugs -analyze-headers -v make || die
}

[ $BUILD_TARGET != android-build ] || {
    (cd .. && ${TRAVIS_BUILD_DIR}/.travis/check-android-build) || die
}

[ $BUILD_TARGET != arm-gcc-4 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    BORDER_ROUTER=1 COAP=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 ECDSA=1 FULL_LOGS=1 JOINER=1 LINK_RAW=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SNTP_CLIENT=1 UDP_FORWARD=1 make -f examples/Makefile-nrf52840 || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-radio || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-cc2650 || die
    arm-none-eabi-size  output/cc2650/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2650/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.36.0.58.zip || die
    unzip -qq asf-standalone-archive-3.36.0.58.zip || die
    mv xdk-asf-3.36.0 third_party/microchip/asf || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-samr21 || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-mtd || die
}

[ $BUILD_TARGET != arm-gcc-5 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    BORDER_ROUTER=1 COAP=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 ECDSA=1 FULL_LOGS=1 JOINER=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SNTP_CLIENT=1 UDP_FORWARD=1 make -f examples/Makefile-nrf52840 || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-radio || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-cc2650 || die
    arm-none-eabi-size  output/cc2650/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2650/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.36.0.58.zip || die
    unzip -qq asf-standalone-archive-3.36.0.58.zip || die
    mv xdk-asf-3.36.0 third_party/microchip/asf || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-samr21 || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-mtd || die
}

[ $BUILD_TARGET != arm-gcc-6 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-6-2017-q2-update/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    BORDER_ROUTER=1 COAP=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 ECDSA=1 FULL_LOGS=1 JOINER=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SNTP_CLIENT=1 UDP_FORWARD=1 make -f examples/Makefile-nrf52840 || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-radio || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-cc2650 || die
    arm-none-eabi-size  output/cc2650/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2650/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.36.0.58.zip || die
    unzip -qq asf-standalone-archive-3.36.0.58.zip || die
    mv xdk-asf-3.36.0 third_party/microchip/asf || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-samr21 || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/samr21/bin/ot-ncp-mtd || die

    export PATH=/tmp/arc_gnu_2017.03-rc2_prebuilt_elf32_le_linux_install/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-emsk || die
    arc-elf32-size  output/emsk/bin/ot-cli-ftd || die
    arc-elf32-size  output/emsk/bin/ot-cli-mtd || die
    arc-elf32-size  output/emsk/bin/ot-ncp-ftd || die
    arc-elf32-size  output/emsk/bin/ot-ncp-mtd || die
}

[ $BUILD_TARGET != arm-gcc-7 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-7-2018-q2-update/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-da15000 || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/da15000/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    BORDER_ROUTER=1 COAP=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 FULL_LOGS=1 JOINER=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SNTP_CLIENT=1 UDP_FORWARD=1 make -f examples/Makefile-nrf52840 || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-radio || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-cc2650 || die
    arm-none-eabi-size  output/cc2650/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2650/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die

    # SAMR21 build failure (#2967):
    #
    # third_party/microchip/asf/sam0/utils/compiler.h:140:0: \
    #   error: "__always_inline" redefined [-Werror] \
    #   #  define __always_inline             __attribute__((__always_inline__))
    #
    # git checkout -- . || die
    # git clean -xfd || die
    # wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.36.0.58.zip || die
    # unzip -qq asf-standalone-archive-3.36.0.58.zip || die
    # mv xdk-asf-3.36.0 third_party/microchip/asf || die
    # ./bootstrap || die
    # COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-samr21 || die
    # arm-none-eabi-size  output/samr21/bin/ot-cli-ftd || die
    # arm-none-eabi-size  output/samr21/bin/ot-cli-mtd || die
    # arm-none-eabi-size  output/samr21/bin/ot-ncp-ftd || die
    # arm-none-eabi-size  output/samr21/bin/ot-ncp-mtd || die
}

[ $BUILD_TARGET != arm-gcc-8 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-8-2018-q4-major/bin:$PATH || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die

    # DA15000 build failure:
    #
    # third_party/dialog/DialogSDK/bsp/peripherals/src/hw_aes_hash.c:399:99: \
    #    error: bitwise comparison always evaluates to false [-Werror=tautological-compare]
    #
    # git checkout -- . || die
    # git clean -xfd || die
    # ./bootstrap || die
    # COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-da15000 || die
    # arm-none-eabi-size  output/da15000/bin/ot-cli-ftd || die
    # arm-none-eabi-size  output/da15000/bin/ot-cli-mtd || die
    # arm-none-eabi-size  output/da15000/bin/ot-ncp-ftd || die
    # arm-none-eabi-size  output/da15000/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    BORDER_ROUTER=1 COAP=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 FULL_LOGS=1 JOINER=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SNTP_CLIENT=1 UDP_FORWARD=1 make -f examples/Makefile-nrf52840 || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-radio || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-cc2650 || die
    arm-none-eabi-size  output/cc2650/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2650/bin/ot-ncp-mtd || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die

    # SAMR21 build failure (#2967):
    #
    # third_party/microchip/asf/sam0/utils/compiler.h:140:0: \
    #   error: "__always_inline" redefined [-Werror] \
    #   #  define __always_inline             __attribute__((__always_inline__))
    #
    # git checkout -- . || die
    # git clean -xfd || die
    # wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.36.0.58.zip || die
    # unzip -qq asf-standalone-archive-3.36.0.58.zip || die
    # mv xdk-asf-3.36.0 third_party/microchip/asf || die
    # ./bootstrap || die
    # COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-samr21 || die
    # arm-none-eabi-size  output/samr21/bin/ot-cli-ftd || die
    # arm-none-eabi-size  output/samr21/bin/ot-cli-mtd || die
    # arm-none-eabi-size  output/samr21/bin/ot-ncp-ftd || die
    # arm-none-eabi-size  output/samr21/bin/ot-ncp-mtd || die
}

[ $BUILD_TARGET != posix ] || {
    sh -c '$CC --version' || die
    sh -c '$CXX --version' || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_NONE make -f examples/Makefile-posix || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_DEBG make -f examples/Makefile-posix || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-ncp                        \
        --enable-ftd                        \
        --enable-mtd                        \
        --with-ncp-bus=spi                  \
        --with-examples=posix               \
        --enable-border-router              \
        --enable-child-supervision          \
        --enable-diag                       \
        --enable-jam-detection              \
        --enable-legacy                     \
        --enable-mac-filter                 \
        --enable-service                    \
        --enable-channel-manager            \
        --enable-channel-monitor            \
        --disable-docs                      \
        --disable-tests                     \
        --with-vendor-extension=./src/core/common/extension_example.cpp || die
    make -j 8 || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-cli                        \
        --enable-mtd                        \
        --with-ncp-bus=spi                  \
        --with-examples=posix               \
        --enable-border-router              \
        --enable-child-supervision          \
        --enable-legacy                     \
        --enable-mac-filter                 \
        --enable-service                    \
        --disable-docs                      \
        --disable-tests || die
    make -j 8 || die

    export CPPFLAGS=-DOPENTHREAD_CONFIG_ENABLE_TIME_SYNC=1

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-cli                        \
        --enable-ncp                        \
        --enable-ftd                        \
        --enable-mtd                        \
        --enable-radio-only                 \
        --with-ncp-bus=uart                 \
        --with-examples=posix || die
    make -j 8 || die
}

[ $BUILD_TARGET != posix-distcheck ] || {
    export ASAN_SYMBOLIZER_PATH=`which llvm-symbolizer-5.0` || die
    export ASAN_OPTIONS=symbolize=1 || die
    export DISTCHECK_CONFIGURE_FLAGS= CPPFLAGS=-DOPENTHREAD_POSIX_VIRTUAL_TIME=1 || die
    ./bootstrap || die
    make -f examples/Makefile-posix distcheck || die
}

[ $BUILD_TARGET != posix-32-bit ] || {
    ./bootstrap || die
    COVERAGE=1 CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 make -f examples/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-app-cli ] || {
    ./bootstrap || die
    # enable code coverage for OpenThread transceiver only
    COVERAGE=1 VIRTUAL_TIME_UART=1 make -f examples/Makefile-posix || die
    COVERAGE=1 make -f src/posix/Makefile-posix || die
    COVERAGE=1 PYTHONUNBUFFERED=1 OT_CLI_PATH="$(pwd)/$(ls output/posix/*/bin/ot-cli)" RADIO_DEVICE="$(pwd)/$(ls output/*/bin/ot-ncp-radio)" make -f src/posix/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-app-pty ] || {
    .travis/check-posix-app-pty || die
}

[ $BUILD_TARGET != posix-mtd ] || {
    ./bootstrap || die
    COVERAGE=1 CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 USE_MTD=1 make -f examples/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-ncp-spi ] || {
    ./bootstrap || die
    make -f examples/Makefile-posix check configure_OPTIONS="--enable-ncp --enable-ftd --with-ncp-bus=spi --with-examples=posix" || die
}

[ $BUILD_TARGET != posix-app-ncp ] || {
    ./bootstrap || die
    COVERAGE=1 VIRTUAL_TIME_UART=1 make -f examples/Makefile-posix || die
    # enable code coverage for OpenThread posix radio
    COVERAGE=1 make -f src/posix/Makefile-posix || die
    COVERAGE=1 PYTHONUNBUFFERED=1 OT_NCP_PATH="$(pwd)/$(ls output/posix/*/bin/ot-ncp)" RADIO_DEVICE="$(pwd)/$(ls output/*/bin/ot-ncp-radio)" NODE_TYPE=ncp-sim make -f src/posix/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-ncp ] || {
    ./bootstrap || die
    COVERAGE=1 PYTHONUNBUFFERED=1 NODE_TYPE=ncp-sim make -f examples/Makefile-posix check || die
}

[ $BUILD_TARGET != toranj-test-framework ] || {
    ./tests/toranj/start.sh || die
}

[ $BUILD_TARGET != osx ] || {
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-posix || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f src/posix/Makefile-posix || die
}
