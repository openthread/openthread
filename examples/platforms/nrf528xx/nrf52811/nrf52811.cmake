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

set(OT_PLATFORM_LIB "openthread-nrf52811" "openthread-nrf52811-transport" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/nrf52811/openthread-core-nrf52811-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nrf52811/nrf52811.ld")

set(COMM_FLAGS
    -DDISABLE_CC310=1
    -DCONFIG_GPIO_AS_PINRESET
    -DNRF52811_XXAA
    -DUSE_APP_CONFIG=1
    -Wno-unused-parameter
    -Wno-expansion-to-defined
)

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-nrf52811-config-check.h\""
)

list(APPEND OT_PUBLIC_INCLUDES
    "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/mbedtls_plat_config"
    "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/mdk"
    "${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/cmsis"
)

set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
target_compile_definitions(ot-config INTERFACE
    "MBEDTLS_USER_CONFIG_FILE=\"nrf52811-mbedtls-config.h\""
)
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

if(OT_CFLAGS MATCHES "-pedantic-errors")
    string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")
endif()

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-nrf52811
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

add_library(openthread-nrf52811-transport
    ${NRF_TRANSPORT_SOURCES}
)

add_library(openthread-nrf52811-sdk
    ${NRF_COMM_SOURCES}
    ${NRF_SINGLEPHY_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_target_properties(openthread-nrf52811 openthread-nrf52811-transport openthread-nrf52811-sdk
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_link_libraries(openthread-nrf52811
    PUBLIC
        ${OT_MBEDTLS}
        ${NRF52811_3RD_LIBS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_link_libraries(openthread-nrf52811-transport
    PUBLIC
        ${OT_MBEDTLS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        nordicsemi-nrf52811-sdk
        ot-config
)

target_link_libraries(openthread-nrf52811-sdk
    PUBLIC
        ${OT_MBEDTLS}
        ${NRF52811_3RD_LIBS}
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_compile_definitions(openthread-nrf52811
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52811-transport
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_definitions(openthread-nrf52811-sdk
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-nrf52811
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
        -DRAAL_SINGLE_PHY=1
)

target_compile_options(openthread-nrf52811-transport
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
)

target_compile_options(openthread-nrf52811-sdk
    PRIVATE
        ${OT_CFLAGS}
        ${COMM_FLAGS}
        -DRAAL_SINGLE_PHY=1
)

target_include_directories(openthread-nrf52811
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52811
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

target_include_directories(openthread-nrf52811-transport
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52811
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

target_include_directories(openthread-nrf52811-sdk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nrf52811
        ${NRF_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

