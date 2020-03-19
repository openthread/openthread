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

[ $BUILD_TARGET != scan-build ] || {
    ./bootstrap || die

    export CPPFLAGS="-DMBEDTLS_DEBUG_C"
    export CPPFLAGS="${CPPFLAGS} -I${TRAVIS_BUILD_DIR}/third_party/mbedtls"
    export CPPFLAGS="${CPPFLAGS} -I${TRAVIS_BUILD_DIR}/third_party/mbedtls/repo/include"
    export CPPFLAGS="${CPPFLAGS} -DMBEDTLS_CONFIG_FILE=\\\"mbedtls-config.h\\\""

    # UART transport
    export CPPFLAGS="${CPPFLAGS}                          \
        -DOPENTHREAD_CONFIG_BORDER_AGENT_ENABLE=1         \
        -DOPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1        \
        -DOPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE=1      \
        -DOPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE=1      \
        -DOPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE=1    \
        -DOPENTHREAD_CONFIG_COAP_API_ENABLE=1             \
        -DOPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE=1      \
        -DOPENTHREAD_CONFIG_COMMISSIONER_ENABLE=1         \
        -DOPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE=1         \
        -DOPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE=1         \
        -DOPENTHREAD_CONFIG_DIAG_ENABLE=1                 \
        -DOPENTHREAD_CONFIG_DNS_CLIENT_ENABLE=1           \
        -DOPENTHREAD_CONFIG_ECDSA_ENABLE=1                \
        -DOPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE=1    \
        -DOPENTHREAD_CONFIG_LEGACY_ENABLE=1               \
        -DOPENTHREAD_CONFIG_JAM_DETECTION_ENABLE=1        \
        -DOPENTHREAD_CONFIG_JOINER_ENABLE=1               \
        -DOPENTHREAD_CONFIG_LINK_RAW_ENABLE=1             \
        -DOPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1           \
        -DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1             \
        -DOPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE=1   \
        -DOPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE=1     \
        -DOPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE=1          \
        -DOPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1  \
        -DOPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE=1 \
        -DOPENTHREAD_CONFIG_UDP_FORWARD_ENABLE=1"

    scan-build ./configure                \
        --enable-builtin-mbedtls=no       \
        --enable-cli                      \
        --enable-executable=no            \
        --enable-ftd                      \
        --enable-mtd                      \
        --enable-ncp                      \
        --enable-radio-only               \
        --with-examples=simulation || die

    scan-build --status-bugs -analyze-headers -v make -j2 || die

    # SPI transport
    scan-build ./configure                \
        --enable-builtin-mbedtls=no       \
        --enable-cli                      \
        --enable-executable=no            \
        --enable-ftd                      \
        --enable-mtd                      \
        --enable-ncp                      \
        --enable-radio-only               \
        --with-examples=simulation || die

    scan-build --status-bugs -analyze-headers -v make -j2 || die
}

[ $BUILD_TARGET != android-build ] || {
    (cd .. && ${TRAVIS_BUILD_DIR}/.travis/check-android-build) || die
}

[ $BUILD_TARGET != gn-build ] || {
    (cd ${TRAVIS_BUILD_DIR} && .travis/check-gn-build) || die
}

build_cc1352() {
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 SLAAC=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc1352 || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc1352/bin/ot-ncp-mtd || die
}

build_cc2538() {
    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=examples/platforms/cc2538/arm-none-eabi.cmake -DOT_PLATFORM=cc2538 -DOT_COMPILE_WARNING_AS_ERROR=ON .. || die
    ninja || die
    cd .. || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 SLAAC=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2538 || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2538/bin/ot-ncp-mtd || die
}

build_cc2650() {
    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DOT_PLATFORM=cc2650 -DCMAKE_TOOLCHAIN_FILE=examples/platforms/cc2650/arm-none-eabi.cmake -DOT_COMPILE_WARNING_AS_ERROR=on -DCMAKE_BUILD_TYPE=Debug ..
    ninja ot-cli-mtd ot-ncp-mtd || die

    arm-none-eabi-size  examples/apps/cli/ot-cli-mtd || die
    arm-none-eabi-size  examples/apps/ncp/ot-ncp-mtd || die
}

build_cc2652() {
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 SLAAC=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-cc2652 || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/cc2652/bin/ot-ncp-mtd || die
}

build_kw41z() {
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    COMMISSIONER=1 JOINER=1 SLAAC=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 make -f examples/Makefile-kw41z || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/kw41z/bin/ot-ncp-mtd || die
}

build_nrf52811() {
    # Default OpenThread switches for nRF52811 platform
    OPENTHREAD_FLAGS="BORDER_ROUTER=1 COAP=1 DNS_CLIENT=1 LINK_RAW=1 MAC_FILTER=1 MTD_NETDIAG=1"

    # UART transport
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-nrf52811 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52811/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52811/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52811/bin/ot-rcp || die

    # SPI transport for NCP
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    NCP_SPI=1 make -f examples/Makefile-nrf52811 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52811/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52811/bin/ot-rcp || die

    # Build without transport (no CLI or NCP applications)
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    DISABLE_TRANSPORTS=1 make -f examples/Makefile-nrf52811 || die
}

build_nrf52833() {
    # Default OpenThread switches for nRF52833 platform
    OPENTHREAD_FLAGS="BORDER_AGENT=1 BORDER_ROUTER=1 COAP=1 COAPS=1 COMMISSIONER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 ECDSA=1 FULL_LOGS=1 IP6_FRAGM=1 JOINER=1 LINK_RAW=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SLAAC=1 SNTP_CLIENT=1 UDP_FORWARD=1"

    # UART transport
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-nrf52833 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52833/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-rcp || die

    # USB transport
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    USB=1 make -f examples/Makefile-nrf52833 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52833/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-rcp || die

    # SPI transport for NCP
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    NCP_SPI=1 make -f examples/Makefile-nrf52833 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52833/bin/ot-rcp || die

    # Build without transport (no CLI or NCP applications)
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    DISABLE_TRANSPORTS=1 make -f examples/Makefile-nrf52833 $OPENTHREAD_FLAGS || die
}

build_nrf52840() {
    # Default OpenThread switches for nRF52840 platform
    OPENTHREAD_FLAGS="BORDER_AGENT=1 BORDER_ROUTER=1 COAP=1 COAPS=1 COMMISSIONER=1 DEBUG=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 DNS_CLIENT=1 ECDSA=1 FULL_LOGS=1 IP6_FRAGM=1 JOINER=1 LINK_RAW=1 MAC_FILTER=1 MTD_NETDIAG=1 SERVICE=1 SLAAC=1 SNTP_CLIENT=1 UDP_FORWARD=1"

    # UART transport
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-rcp || die

    # USB transport with bootloader e.g. to support PCA10059 dongle
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    USB=1 BOOTLOADER=1 make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-rcp || die

    # SPI transport for NCP
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    NCP_SPI=1 make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-rcp || die

    # Build without transport (no CLI or NCP applications)
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    DISABLE_TRANSPORTS=1 make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die

    # Software cryptography
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    DISABLE_BUILTIN_MBEDTLS=0 make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-rcp || die

    # Software cryptography with threading support
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    DISABLE_BUILTIN_MBEDTLS=0 MBEDTLS_THREADING=1 make -f examples/Makefile-nrf52840 $OPENTHREAD_FLAGS || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-cli-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-ftd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-ncp-mtd || die
    arm-none-eabi-size  output/nrf52840/bin/ot-rcp || die
}

build_qpg6095() {
    CMAKE_FLAGS="                        \
        -DOT_COMPILE_WARNING_AS_ERROR=on \
        -DOT_COMMISSIONER=on             \
        -DOT_DHCP6_CLIENT=on             \
        -DOT_DHCP6_SERVER=on             \
        -DOT_DNS_CLIENT=on               \
        -DOT_JOINER=on                   \
        -DOT_SLAAC=on"
    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=examples/platforms/qpg6095/arm-none-eabi.cmake -DOT_PLATFORM=qpg6095 ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=Debug .. || die
    ninja || die

    arm-none-eabi-size  examples/apps/cli/ot-cli-ftd || die
    arm-none-eabi-size  examples/apps/cli/ot-cli-mtd || die
    arm-none-eabi-size  examples/apps/ncp/ot-ncp-ftd || die
    arm-none-eabi-size  examples/apps/ncp/ot-ncp-mtd || die
    cd .. || die
}

build_samr21() {
    CMAKE_FLAGS="                        \
        -DOT_COMPILE_WARNING_AS_ERROR=on \
        -DOT_COMMISSIONER=on             \
        -DOT_DHCP6_CLIENT=on             \
        -DOT_DHCP6_SERVER=on             \
        -DOT_DNS_CLIENT=on               \
        -DOT_JOINER=on                   \
        -DOT_SLAAC=on"
    git checkout -- . || die
    git clean -xfd || die
    wget http://ww1.microchip.com/downloads/en/DeviceDoc/asf-standalone-archive-3.45.0.85.zip || die
    unzip -qq asf-standalone-archive-3.45.0.85.zip || die
    mv xdk-asf-3.45.0 third_party/microchip/asf || die
    mkdir build && cd build || die
    cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=examples/platforms/samr21/arm-none-eabi.cmake -DOT_PLATFORM=samr21 ${CMAKE_FLAGS} -DCMAKE_BUILD_TYPE=Debug .. || die
    ninja || die

    arm-none-eabi-size  examples/apps/cli/ot-cli-ftd || die
    arm-none-eabi-size  examples/apps/cli/ot-cli-mtd || die
    arm-none-eabi-size  examples/apps/ncp/ot-ncp-ftd || die
    arm-none-eabi-size  examples/apps/ncp/ot-ncp-mtd || die
    cd .. || die
}

[ $BUILD_TARGET != arm-gcc-4 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-4_9-2015q3/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != arm-gcc-5 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != arm-gcc-6 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-6-2017-q2-update/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != arm-gcc-7 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-7-2018-q2-update/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != arm-gcc-8 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-8-2018-q4-major/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != arm-gcc-9 ] || {
    export PATH=/tmp/gcc-arm-none-eabi-9-2019-q4-major/bin:$PATH || die

    build_cc1352
    build_cc2538
    build_cc2650
    build_cc2652
    build_kw41z
    build_nrf52811
    build_nrf52833
    build_nrf52840
    build_qpg6095
    build_samr21
}

[ $BUILD_TARGET != simulation ] || {
    CMAKE_FLAGS="                        \
        -DOT_COMPILE_WARNING_AS_ERROR=on \
        -DOT_BORDER_AGENT=on             \
        -DOT_BORDER_ROUTER=on            \
        -DOT_COAP=on                     \
        -DOT_COAPS=on                    \
        -DOT_COMMISSIONER=on             \
        -DOT_CHANNEL_MANAGER=on          \
        -DOT_CHANNEL_MONITOR=on          \
        -DOT_CHILD_SUPERVISION=on        \
        -DOT_DHCP6_CLIENT=on             \
        -DOT_DHCP6_SERVER=on             \
        -DOT_DIAGNOSTIC=on               \
        -DOT_DNS_CLIENT=on               \
        -DOT_ECDSA=on                    \
        -DOT_IP6_FRAGM=on                \
        -DOT_JAM_DETECTION=on            \
        -DOT_JOINER=on                   \
        -DOT_LINK_RAW=on                 \
        -DOT_MAC_FILTER=on               \
        -DOT_MTD_NETDIAG=on              \
        -DOT_SERVICE=on                  \
        -DOT_SLAAC=on                    \
        -DOT_SNTP_CLIENT=on"

    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DOT_PLATFORM=simulation $CMAKE_FLAGS .. || die
    ninja || die
    cd .. || die

    git checkout -- . || die
    git clean -xfd || die
    ./tests/toranj/build.sh cmake || die

    git checkout -- . || die
    git clean -xfd || die
    ./tests/toranj/build.sh cmake-posix || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_NONE make -f examples/Makefile-simulation || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_DEBG make -f examples/Makefile-simulation || die

    export CPPFLAGS="                                             \
        -DOPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE=1              \
        -DOPENTHREAD_CONFIG_BORDER_AGENT_ENABLE=1                 \
        -DOPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1                \
        -DOPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE=1              \
        -DOPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE=1              \
        -DOPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE=1            \
        -DOPENTHREAD_CONFIG_COAP_API_ENABLE=1                     \
        -DOPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE=1              \
        -DOPENTHREAD_CONFIG_COMMISSIONER_ENABLE=1                 \
        -DOPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE=1                 \
        -DOPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE=1                 \
        -DOPENTHREAD_CONFIG_DIAG_ENABLE=1                         \
        -DOPENTHREAD_CONFIG_DNS_CLIENT_ENABLE=1                   \
        -DOPENTHREAD_CONFIG_ECDSA_ENABLE=1                        \
        -DOPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE=1            \
        -DOPENTHREAD_CONFIG_IP6_SLAAC_ENABLE=1                    \
        -DOPENTHREAD_CONFIG_LEGACY_ENABLE=1                       \
        -DOPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE=1 \
        -DOPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_ENABLE=1           \
        -DOPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE=1    \
        -DOPENTHREAD_CONFIG_MPL_DYNAMIC_INTERVAL_ENABLE           \
        -DOPENTHREAD_CONFIG_JAM_DETECTION_ENABLE=1                \
        -DOPENTHREAD_CONFIG_JOINER_ENABLE=1                       \
        -DOPENTHREAD_CONFIG_LINK_RAW_ENABLE=1                     \
        -DOPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE=1            \
        -DOPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1                   \
        -DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1                     \
        -DOPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE=1           \
        -DOPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE=1               \
        -DOPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE=1          \
        -DOPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE=1          \
        -DOPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE=1             \
        -DOPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE=1                  \
        -DOPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE=1         \
        -DOPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE=1        \
        -DOPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE=1         \
        -DOPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE=1          \
        -DOPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1          \
        -DOPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE=1         \
        -DOPENTHREAD_CONFIG_UDP_FORWARD_ENABLE=1"

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-simulation || die

    export CPPFLAGS="${CPPFLAGS}                          \
        -DOPENTHREAD_CONFIG_ASSERT_ENABLE=0"

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-simulation || die

    export CPPFLAGS="                                    \
        -DOPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1       \
        -DOPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE=1     \
        -DOPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE=1     \
        -DOPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE=1   \
        -DOPENTHREAD_CONFIG_DIAG_ENABLE=1                \
        -DOPENTHREAD_CONFIG_JAM_DETECTION_ENABLE=1       \
        -DOPENTHREAD_CONFIG_LEGACY_ENABLE=1              \
        -DOPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1          \
        -DOPENTHREAD_CONFIG_NCP_SPI_ENABLE=1             \
        -DOPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE=1  \
        -DOPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1"

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE=1 make -f examples/Makefile-simulation || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    CPPFLAGS=-DOPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE=1 make -f examples/Makefile-simulation || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-ncp                        \
        --enable-ftd                        \
        --enable-mtd                        \
        --with-examples=simulation          \
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
        --with-examples=simulation          \
        --disable-docs                      \
        --disable-tests || die
    make -j 8 || die

    export CPPFLAGS="                                   \
        -DOPENTHREAD_CONFIG_ANOUNCE_SENDER_ENABLE=1     \
        -DOPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE=1 \
        -DOPENTHREAD_CONFIG_TIME_SYNC_ENABLE=1          \
        -DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1"

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-cli                        \
        --enable-ncp                        \
        --enable-ftd                        \
        --enable-mtd                        \
        --enable-radio-only                 \
        --with-examples=simulation || die
    make -j 8 || die

    export CPPFLAGS="                                    \
        -DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1            \
        -DOPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE=1"

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    ./configure                             \
        --enable-ncp                        \
        --enable-ftd                        \
        --enable-mtd                        \
        --with-examples=simulation          \
        --disable-docs                      \
        --disable-tests                     \
        --with-ncp-vendor-hook-source=./src/ncp/example_vendor_hook.cpp || die
    make -j 8 || die
}

[ $BUILD_TARGET != simulation-distcheck ] || {
    export ASAN_SYMBOLIZER_PATH=`which llvm-symbolizer` || die
    export ASAN_OPTIONS=symbolize=1 || die
    export DISTCHECK_CONFIGURE_FLAGS= CPPFLAGS=-DOPENTHREAD_SIMULATION_VIRTUAL_TIME=1  || die
    ./bootstrap || die
    REFERENCE_DEVICE=1 make -f examples/Makefile-simulation distcheck || die
}

[ $BUILD_TARGET != simulation-32-bit ] || {
    ./bootstrap || die
    REFERENCE_DEVICE=1 COVERAGE=1 CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 make -f examples/Makefile-simulation check || die
}

[ $BUILD_TARGET != posix-cli ] || {
    ./bootstrap || die
    # enable code coverage for OpenThread transceiver only
    COVERAGE=1 VIRTUAL_TIME_UART=1 make -f examples/Makefile-simulation || die
    # readline supports pipe, editline does not
    REFERENCE_DEVICE=1 COVERAGE=1 READLINE=readline make -f src/posix/Makefile-posix || die
    REFERENCE_DEVICE=1 COVERAGE=1 PYTHONUNBUFFERED=1 OT_CLI_PATH="$(pwd)/$(ls output/posix/*/bin/ot-cli) -v" RADIO_DEVICE="$(pwd)/$(ls output/*/bin/ot-rcp)" make -f src/posix/Makefile-posix check || die
}

[ $BUILD_TARGET != v1.2 ] || {
    ./bootstrap || die
    ./script/test build || die
    ./script/test cert_suite tests/scripts/thread-cert/v1_2_* || die
}

[ $BUILD_TARGET != posix-pty ] || {
    # check daemon mode
    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DOT_PLATFORM=posix -DOT_DAEMON=ON -DCOMPILE_WARNING_AS_ERROR=ON .. || die
    ninja || die
    cd .. || die

    git checkout -- . || die
    git clean -xfd || die
    mkdir build && cd build || die
    cmake -GNinja -DOT_PLATFORM=posix -DCOMPILE_WARNING_AS_ERROR=ON .. || die
    ninja || die
    cd .. || die

    ./bootstrap
    .travis/check-posix-pty || die
}

[ $BUILD_TARGET != posix-migrate ] || {
    ./bootstrap
    .travis/check-ncp-rcp-migrate || die
}

[ $BUILD_TARGET != simulation-mtd ] || {
    ./bootstrap || die
    REFERENCE_DEVICE=1 COVERAGE=1 CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 USE_MTD=1 make -f examples/Makefile-simulation check || die
}

[ $BUILD_TARGET != simulation-ncp-spi ] || {
    CPPFLAGS="-DOPENTHREAD_CONFIG_NCP_SPI_ENABLE=1"

    ./bootstrap || die
    make -f examples/Makefile-simulation check configure_OPTIONS="--enable-ncp --enable-ftd --with-examples=simulation" || die
}

[ $BUILD_TARGET != posix-ncp ] || {
    ./bootstrap || die
    REFERENCE_DEVICE=1 COVERAGE=1 VIRTUAL_TIME_UART=1 make -f examples/Makefile-simulation || die
    REFERENCE_DEVICE=1 COVERAGE=1 READLINE=readline make -f src/posix/Makefile-posix || die
    REFERENCE_DEVICE=1 COVERAGE=1 PYTHONUNBUFFERED=1 OT_NCP_PATH="$(pwd)/$(ls output/posix/*/bin/ot-ncp)" RADIO_DEVICE="$(pwd)/$(ls output/*/bin/ot-rcp)" NODE_TYPE=ncp-sim make -f src/posix/Makefile-posix check || die
}

[ $BUILD_TARGET != posix-spi ] || {
    ./bootstrap || die
    REFERENCE_DEVICE=1 READLINE=readline RCP_SPI=1 make -f src/posix/Makefile-posix || die
}

[ $BUILD_TARGET != simulation-ncp ] || {
    ./bootstrap || die
    REFERENCE_DEVICE=1 COVERAGE=1 PYTHONUNBUFFERED=1 NODE_TYPE=ncp-simulation make -f examples/Makefile-simulation check || die
}

[ $BUILD_TARGET != toranj-test-framework ] || {
    top_builddir=$(pwd)/build/toranj ./tests/toranj/start.sh || die
}

[ $BUILD_TARGET != osx ] || {
    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f examples/Makefile-simulation || die

    git checkout -- . || die
    git clean -xfd || die
    ./bootstrap || die
    make -f src/posix/Makefile-posix || die
}
