#
#  Copyright (c) 2021, The OpenThread Authors.
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

set(OT_PLATFORM_LIB "openthread-nrf52833" "openthread-nrf52833-transport" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/nrf52833/openthread-core-nrf52833-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

option(OT_BOOTLOADER "OT nrf bootloader type")
if(OT_BOOTLOADER STREQUAL "USB")
    list(APPEND OT_PLATFORM_DEFINES "APP_USBD_NRF_DFU_TRIGGER_ENABLED=1")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52833/nrf52833_bootloader_usb.ld")
elseif(OT_BOOTLOADER STREQUAL "UART")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52833/nrf52833_bootloader_uart.ld")
elseif(OT_BOOTLOADER STREQUAL "BLE")
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52833/nrf52833_bootloader_ble.ld")
else()
    set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52833/nrf52833.ld")
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-nrf52833-config-check.h\""
)
list(APPEND OT_PUBLIC_INCLUDES
    "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/mbedtls_plat_config"
    "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/nrfx/mdk"
     "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/cmsis"
)

set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
target_compile_definitions(ot-config INTERFACE
    "MBEDTLS_USER_CONFIG_FILE=\"nrf52833-mbedtls-config.h\""
)
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

set(COMM_FLAGS
    -DDISABLE_CC310=1
    -DCONFIG_GPIO_AS_PINRESET
    -DNRF52833_XXAA
    -DUSE_APP_CONFIG=1
    -Wno-unused-parameter
    -Wno-expansion-to-defined
)
if(OT_CFLAGS MATCHES "-pedantic-errors")
    string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")
endif()
list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-nrf52833
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

add_library(openthread-nrf52833-transport
    ${NRF_TRANSPORT_SOURCES}
)

add_library(openthread-nrf52833-sdk
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

add_library(openthread-nrf52833-softdevice-sdk
    ${NRF_COMM_SOURCES}
    ${NRF_SOFTDEVICE_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_target_properties(openthread-nrf52833 openthread-nrf52833-transport openthread-nrf52833-sdk openthread-nrf52833-softdevice-sdk
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_link_libraries(openthread-nrf52833
    PUBLIC
        ${OT_MBEDTLS}
        ${NRF52833_3RD_LIBS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_link_libraries(openthread-nrf52833-transport
    PUBLIC
        ${OT_MBEDTLS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        nordicsemi-nrf52833-sdk
        ot-config
)

target_link_libraries(openthread-nrf52833-sdk
    PUBLIC
        ${OT_MBEDTLS}
        ${NRF52833_3RD_LIBS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_link_libraries(openthread-nrf52833-softdevice-sdk
    PUBLIC
        ${OT_MBEDTLS}
        ${NRF52833_3RD_LIBS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_compile_definitions(openthread-nrf52833
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52833-transport
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52833-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52833-softdevice-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-nrf52833
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
)

target_compile_options(openthread-nrf52833-transport
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
)

target_compile_options(openthread-nrf52833-sdk
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
)

target_compile_options(openthread-nrf52833-softdevice-sdk
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
        -DSOFTDEVICE_PRESENT
        -DS140
)

target_include_directories(openthread-nrf52833
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52833
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

target_include_directories(openthread-nrf52833-transport
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52833
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

target_include_directories(openthread-nrf52833-sdk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52833
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

target_include_directories(openthread-nrf52833-softdevice-sdk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52833
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)
