#
#  Copyright (c) 2020, The OpenThread Authors.
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

set(OT_PLATFORM_LIB "openthread-nrf52840" "openthread-nrf52840-transport" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/nrf52840/openthread-core-nrf52840-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

set(OT_BOOTLOADER "USB" CACHE STRING "OT bootloader type")
if(${OT_BOOTLOADER} STREQUAL "USB")
    list(APPEND OT_PLATFORM_DEFINES "APP_USBD_NRF_DFU_TRIGGER_ENABLED=1")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52840/nrf52840_bootloader_usb.ld")
elseif(${OT_BOOTLOADER} STREQUAL "UART")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52840/nrf52840_bootloader_uart.ld")
elseif(${OT_BOOTLOADER} STREQUAL "BLE")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52840/nrf52840_bootloader_ble.ld")
else()
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52840/nrf52840.ld")
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-nrf52840-config-check.h\""
)
if(NOT OT_EXTERNAL_MBEDTLS)
    list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"")
    if(OT_MBEDTLS_THREADING)
        list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_THREADING_C")
        list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_THREADING_ALT")
        list(APPEND OT_PUBLIC_INCLUDES
            "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/include/software-only-threading"
        )
    endif()
else()
    list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_CONFIG_FILE=\"nrf-config.h\"")
    list(APPEND OT_PUBLIC_INCLUDES
        "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/include"
        "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/config"
        "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/nrf_cc310_plat/include"
    )
endif()

set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
target_compile_definitions(ot-config INTERFACE
    "MBEDTLS_USER_CONFIG_FILE=\"nrf52840-mbedtls-config.h\""
)
list(APPEND OT_PUBLIC_INCLUDES "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/mbedtls_plat_config")
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

if(OT_CFLAGS MATCHES "-pedantic-errors")
    string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")
endif()
list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-nrf52840
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
)

add_library(openthread-nrf52840-transport
    ${NRF_TRANSPORT_SOURCES}
)

add_library(openthread-nrf52840-sdk
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
)

add_library(openthread-nrf52840-softdevice-sdk
    ${NRF_COMM_SOURCES}
    ${NRF_SOFTDEVICE_SOURCES}
)

set_target_properties(openthread-nrf52840
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)
set_target_properties(openthread-nrf52840-transport
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)
set_target_properties(openthread-nrf52840-sdk
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)
set_target_properties(openthread-nrf52840-softdevice-sdk
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

if(OT_EXTERNAL_MBEDTLS)
    target_link_libraries(openthread-nrf52840
        PUBLIC
            ${OT_MBEDTLS}
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            ot-config
    )
    target_link_libraries(openthread-nrf52840-transport
        PUBLIC
            ${OT_MBEDTLS}
        PRIVATE
            nordicsemi-nrf52840-sdk
            ot-config
    )
    target_link_libraries(openthread-nrf52840-sdk
        PUBLIC
            ${OT_MBEDTLS}
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            ot-config
    )
    target_link_libraries(openthread-nrf52840-softdevice-sdk
        PUBLIC
            ${OT_MBEDTLS}
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            ot-config
    )
else()
    target_link_libraries(openthread-nrf52840
        PUBLIC
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            openthread-nrf52840-transport
            ${OT_MBEDTLS}
            ot-config
    )
    target_link_libraries(openthread-nrf52840-transport
        PUBLIC
            nordicsemi-nrf52840-sdk
        PRIVATE
            ${OT_MBEDTLS}
            ot-config
    )
    target_link_libraries(openthread-nrf52840-sdk
        PUBLIC
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            ${OT_MBEDTLS}
            ot-config
    )
    target_link_libraries(openthread-nrf52840-softdevice-sdk
        PUBLIC
            ${NRF52840_3RD_LIBS}
        PRIVATE
            openthread-platform-utils
            ${OT_MBEDTLS}
            ot-config
    )
endif()

target_link_options(openthread-nrf52840
    PUBLIC
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
)

target_link_options(openthread-nrf52840-transport
    PUBLIC
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
)

target_link_options(openthread-nrf52840-sdk
    PUBLIC
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
)

target_link_options(openthread-nrf52840-softdevice-sdk
    PUBLIC
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
)

target_compile_definitions(openthread-nrf52840
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52840-transport
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52840-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52840-softdevice-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-nrf52840 PRIVATE
    ${OT_CFLAGS}
    -DCONFIG_GPIO_AS_PINRESET
    -DNRF52840_XXAA
    -DUSE_APP_CONFIG=1
    -DPLATFORM_OPENTHREAD_VANILLA
    -Wno-expansion-to-defined
    -Wno-unused-parameter
)
target_compile_options(openthread-nrf52840-transport PRIVATE
    ${OT_CFLAGS}
    -DCONFIG_GPIO_AS_PINRESET
    -DNRF52840_XXAA
    -DUSE_APP_CONFIG=1
    -DPLATFORM_OPENTHREAD_VANILLA
    -Wno-expansion-to-defined
    -Wno-unused-parameter
)
target_compile_options(openthread-nrf52840-sdk
    PRIVATE
        ${OT_CFLAGS}
        -DCONFIG_GPIO_AS_PINRESET
        -DNRF52840_XXAA
        -DUSE_APP_CONFIG=1
        -Wno-unused-parameter
        -Wno-expansion-to-defined
)
target_compile_options(openthread-nrf52840-softdevice-sdk
    PRIVATE
        ${OT_CFLAGS}
        -DCONFIG_GPIO_AS_PINRESET
        -DNRF52840_XXAA
        -DUSE_APP_CONFIG=1
        -DSOFTDEVICE_PRESENT
        -DS140
        -Wno-unused-parameter
        -Wno-expansion-to-defined
)

target_include_directories(openthread-nrf52840
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52840
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)
target_include_directories(openthread-nrf52840-transport
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52840
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)
target_include_directories(openthread-nrf52840-sdk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52840
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)
target_include_directories(openthread-nrf52840-softdevice-sdk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52840
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)
