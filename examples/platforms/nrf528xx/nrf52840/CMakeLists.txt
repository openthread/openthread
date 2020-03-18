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

set(OT_PLATFORM_LIB "openthread-nrf52840" "openthread-nrf52840-sdk" "openthread-nrf52840-softdevice-sdk" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-nrf52840-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-nrf52840-config-check.h\""
    "OPENTHREAD_SETTINGS_RAM=0"
    "OPENTHREAD_CONFIG_NCP_UART_ENABLE=1"
    "UART_AS_SERIAL_TRANSPORT=1"
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
set(NRF_CFLAGS
    $<$<COMPILE_LANGUAGE:C>: -Wall -Wextra -Wshadow -Werror>
    $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wshadow -Wno-c++14-compat -fno-exceptions>
)

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")
set(COMM_SOURCES
    ../src/alarm.c
    ../src/diag.c
    ../src/entropy.c
    ../src/fem.c
    ../src/flash.c
    ../src/logging.c
    ../src/misc.c
    ../src/radio.c
    ../src/spi-slave.c
    ../src/system.c
    ../src/temp.c
    ../src/uart.c
    ../src/usb-cdc-uart.c
)

set(COMM_INCLUDES
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/cmsis
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52840/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/dependencies
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/clock
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/common
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/power
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/systick
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/usbd
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/app_error
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/atfifo
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/atomic
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/block_dev
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/delay
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/class/cdc
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/utf_converter
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/hal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/drivers
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/drivers/include
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/mdk
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/soc
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/softdevice/s140/headers
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/hal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/platform/lp_timer
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/platform/temperature
)

add_library(openthread-nrf52840
    ${COMM_SOURCES}
    ../src/flash_nosd.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

add_library(openthread-nrf52840-sdk
    ${COMM_SOURCES}
    ../src/flash_nosd.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

add_library(openthread-nrf52840-softdevice-sdk
    ${COMM_SOURCES}
    ../src/flash_sd.c
    ../src/softdevice.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_property(TARGET openthread-nrf52840 PROPERTY C_STANDARD 99)
set_property(TARGET openthread-nrf52840-sdk PROPERTY C_STANDARD 99)
set_property(TARGET openthread-nrf52840-softdevice-sdk PROPERTY C_STANDARD 99)
target_link_libraries(openthread-nrf52840 PRIVATE openthread-platform-utils)
target_link_libraries(openthread-nrf52840-sdk PRIVATE openthread-platform-utils)
target_link_libraries(openthread-nrf52840-softdevice-sdk PRIVATE openthread-platform-utils)
target_link_options(openthread-nrf52840 PRIVATE -T${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/nrf52840_bootloader_uart.ld)
target_link_options(openthread-nrf52840-sdk PUBLIC -T${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/nrf52840_bootloader_uart.ld)
target_link_options(openthread-nrf52840-softdevice-sdk PUBLIC -T${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/nrf52840_bootloader_uart.ld)
target_link_options(openthread-nrf52840 PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)
target_link_options(openthread-nrf52840-sdk PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)
target_link_options(openthread-nrf52840-softdevice-sdk PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)

target_compile_definitions(openthread-nrf52840
    PUBLIC
        ${OT_PLATFORM_DEFINES}
        "MBEDTLS_CONFIG_FILE=\"nrf-config.h\""
        "MBEDTLS_USER_CONFIG_FILE=\"nrf52840-mbedtls-config.h\""
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)
target_compile_definitions(openthread-nrf52840-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
        "MBEDTLS_CONFIG_FILE=\"nrf-config.h\""
        "MBEDTLS_USER_CONFIG_FILE=\"nrf52840-mbedtls-config.h\""
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)
target_compile_definitions(openthread-nrf52840-softdevice-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
        "MBEDTLS_CONFIG_FILE=\"nrf-config.h\""
        "MBEDTLS_USER_CONFIG_FILE=\"nrf52840-mbedtls-config.h\""
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)

target_compile_options(openthread-nrf52840 PRIVATE
    ${NRF_CFLAGS}
    -DCONFIG_GPIO_AS_PINRESET
    -DNRF52840_XXAA
    -DUSE_APP_CONFIG=1
    -DPLATFORM_OPENTHREAD_VANILLA
    -Wno-expansion-to-defined
    -Wno-unused-parameter

)
target_compile_options(openthread-nrf52840-sdk PRIVATE
    ${NRF_CFLAGS}
    PUBLIC
        -DCONFIG_GPIO_AS_PINRESET
        -DNRF52840_XXAA
        -DUSE_APP_CONFIG=1
        -Wno-unused-parameter
        -Wno-expansion-to-defined
)
target_compile_options(openthread-nrf52840-softdevice-sdk PRIVATE
    ${NRF_CFLAGS}
    PUBLIC
        -DCONFIG_GPIO_AS_PINRESET
        -DNRF52840_XXAA
        -DUSE_APP_CONFIG=1
        -DSOFTDEVICE_PRESENT
        -DS140
        -Wno-unused-parameter
        -Wno-expansion-to-defined
)

target_include_directories(openthread-nrf52840 PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/src
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/include
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/nrf_cc310_plat/include
    PUBLIC
        ${COMM_INCLUDES}
)
target_include_directories(openthread-nrf52840-sdk PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/src
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/include
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/nrf_cc310_plat/include
    PUBLIC
        ${COMM_INCLUDES}
)
target_include_directories(openthread-nrf52840-softdevice-sdk PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/src
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/include
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/nrf_cc310_plat/include
    PUBLIC
        ${COMM_INCLUDES}
)
