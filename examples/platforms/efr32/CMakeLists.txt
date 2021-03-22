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

include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/utility.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/includes.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/imported_libs.cmake)


# ==============================================================================
# Common sources and includes
# ==============================================================================
set(EFR32_COMMON_SOURCES
    src/alarm.c
    src/diag.c
    src/entropy.c
    src/fem-control.c
    src/flash.c
    src/ieee802154mac.h
    src/logging.c
    src/mbedtls_config.h
    src/memory.c
    src/misc.c
    src/openthread-core-efr32-config-check.h
    src/openthread-core-efr32-config.h
    src/platform-band.h
    src/platform-efr32.h
    src/radio.c
    src/rail_config.h
    src/startup-gcc.c
    src/system.c
    src/uart.c
)

set(EFR32_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/${EFR32_PLATFORM}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/${EFR32_PLATFORM}/${BOARD_LOWERCASE}
    ${PROJECT_SOURCE_DIR}/third_party/silabs/rail_config
    ${SILABS_GSDK_INCLUDES}
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src/core
)

set(EFR32_CFLAGS
    -Wno-sign-compare
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

# ==============================================================================
# mbedtls
# ==============================================================================
if(NOT OT_EXTERNAL_MBEDTLS)
    message(FATAL_ERROR "OT_EXTERNAL_MBEDTLS is not set. Please include the define when running cmake\n\t-DOT_EXTERNAL_MBEDTLS=silabs-mbedtls")
else()
    list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"")
endif()
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

if(PLATFORM_LOWERCASE MATCHES "^efr32mg2\\d*")
    list(APPEND OT_PUBLIC_INCLUDES
        "${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/inc"
        "${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src"
    )
    set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)
endif()

# ==============================================================================
# General variables
# ==============================================================================

# ==============================================================================
# Filter out CFLAGS
# ==============================================================================
set(ignored_flags
    "-pedantic-errors"  # Needed for RAIL lib
    "-Wshadow"          # Needed for RAIL lib
)

foreach(flag IN LISTS ignored_flags)
    string(REPLACE "${flag}" "" OT_CFLAGS "${OT_CFLAGS}")
endforeach()
list(APPEND EFR32_CFLAGS ${OT_CFLAGS})
set(OT_CFLAGS ${OT_CFLAGS} PARENT_SCOPE)

# Use default config file if one isn't specified
if(NOT OT_CONFIG)
    set(OT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/src/openthread-core-efr32-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\""
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-efr32-config-check.h\""
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

# ==============================================================================
# Platform targets
# ==============================================================================
set(EFR32_COMMON_3RD_LIBS
    jlinkrtt
    ${OT_MBEDTLS}
)

include(${PLATFORM_LOWERCASE}/${PLATFORM_LOWERCASE}.cmake)

list(APPEND OT_PUBLIC_INCLUDES ${EFR32_INCLUDES})
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

# ==============================================================================
# sleepy-demo
# ==============================================================================
option(EFR32_APP_SLEEPY_DEMO "enable sleepy-demo app" ON)

if(EFR32_APP_SLEEPY_DEMO)
    add_subdirectory(sleepy-demo)
endif()
