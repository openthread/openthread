#
#  Copyright (c) 2019, The OpenThread Authors.
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

set(OT_PLATFORM_LIB "openthread-posix" PARENT_SCOPE)

add_library(ot-posix-config INTERFACE)

option(OT_DAEMON "Enable daemon mode" OFF)
if(OT_DAEMON)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE=1"
    )

    # We have to add this definition to `ot-config` because openthread core
    # libraries will set config file to "openthrad-core-posix-config.h" which
    # depends on `OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE` to correctly enable
    # PLATFORM_NETIF and PLATFORM_UDP features. Otherwise, openthread core and
    # posix libraries will use different feature definitions.
    list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE=1")
    set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
endif()

if(OT_MULTIPAN_RCP)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1"
    )
endif()


option(OT_POSIX_INSTALL_EXTERNAL_ROUTES "Install External Routes as IPv6 routes" ON)
if(OT_POSIX_INSTALL_EXTERNAL_ROUTES)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE=1"
    )
else()
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE=0"
    )
endif()

option(OT_POSIX_VIRTUAL_TIME "enable virtual time" OFF)
if(OT_POSIX_VIRTUAL_TIME)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_VIRTUAL_TIME=1"
    )
endif()

option(OT_POSIX_MAX_POWER_TABLE  "enable max power table" OFF)
if(OT_POSIX_MAX_POWER_TABLE)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE=1"
    )
endif()

option(OT_POSIX_SECURE_SETTINGS "enable secure settings" OFF)
if (OT_POSIX_SECURE_SETTINGS)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE=1"
    )
endif()

option(OT_POSIX_RCP_HDLC_BUS "enable RCP HDLC bus" OFF)
if(OT_POSIX_RCP_HDLC_BUS)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE=1"
    )
endif()

option(OT_POSIX_RCP_SPI_BUS "enable RCP SPI bus" OFF)
if(OT_POSIX_RCP_SPI_BUS)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE=1"
    )
endif()

option(OT_POSIX_RCP_VENDOR_BUS "enable RCP vendor bus" OFF)
if(OT_POSIX_RCP_VENDOR_BUS)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE=1"
    )
endif()

set(OT_POSIX_NAT64_CIDR "192.168.255.0/24" CACHE STRING "NAT64 CIDR for OpenThread NAT64")
if(OT_POSIX_NAT64_CIDR)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_NAT64_CIDR=\"${OT_POSIX_NAT64_CIDR}\""
    )
endif()

if(NOT OT_PLATFORM_CONFIG)
    set(OT_PLATFORM_CONFIG "openthread-core-posix-config.h" PARENT_SCOPE)
endif()

set(CMAKE_EXE_LINKER_FLAGS "-rdynamic ${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE)

if(OT_ANDROID_NDK)
    target_compile_options(ot-posix-config INTERFACE -Wno-sign-compare)
endif()

add_library(openthread-posix
    alarm.cpp
    backtrace.cpp
    ble.cpp
    configuration.cpp
    config_file.cpp
    daemon.cpp
    entropy.cpp
    firewall.cpp
    hdlc_interface.cpp
    infra_if.cpp
    logging.cpp
    mainloop.cpp
    memory.cpp
    misc.cpp
    multicast_routing.cpp
    netif.cpp
    power.cpp
    radio.cpp
    radio_url.cpp
    resolver.cpp
    settings.cpp
    spi_interface.cpp
    system.cpp
    trel.cpp
    udp.cpp
    utils.cpp
    virtual_time.cpp
)

include(vendor.cmake)

target_link_libraries(openthread-posix
    PUBLIC
        openthread-platform
    PRIVATE
        openthread-cli-ftd
        openthread-ftd
        openthread-hdlc
        openthread-radio-spinel
        openthread-spinel-rcp
        openthread-url
        ot-config-ftd
        ot-config
        ot-posix-config
        $<$<NOT:$<BOOL:${OT_ANDROID_NDK}>>:util>
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:rt>
)

option(OT_TARGET_OPENWRT "enable openthread posix for OpenWRT" OFF)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT OT_TARGET_OPENWRT)
    target_compile_definitions(ot-posix-config
        INTERFACE "OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE=1"
    )
    target_link_libraries(openthread-posix PRIVATE anl)
endif()

target_compile_definitions(openthread-posix
    PUBLIC
        ${OT_PUBLIC_DEFINES}
    PRIVATE
        OPENTHREAD_FTD=1
        OPENTHREAD_MTD=0
        OPENTHREAD_RADIO=0
)

target_compile_options(openthread-posix PRIVATE
    ${OT_CFLAGS}
)

target_include_directories(openthread-posix PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/third_party/mbedtls/repo/include
    ${OT_SPINEL_VENDOR_HOOK_SOURCE_DIR}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/posix/platform/include
)

add_executable(ot-posix-test-settings
    settings.cpp
)
target_compile_definitions(ot-posix-test-settings
    PRIVATE -DSELF_TEST=1 -DOPENTHREAD_CONFIG_LOG_PLATFORM=0
)
target_include_directories(ot-posix-test-settings
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/src/posix/platform/include
)
add_test(NAME ot-posix-test-settings COMMAND ot-posix-test-settings)
