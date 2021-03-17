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

# ==============================================================================
# Verify board is supported for platform
# ==============================================================================
if(BOARD_LOWERCASE STREQUAL "brd4180a")
    set(MCU "EFR32MG21A020F1024IM32")
elseif(BOARD_LOWERCASE STREQUAL "brd4180b")
    set(MCU "EFR32MG21A020F1024IM32")
else()
    message(FATAL_ERROR "
    BOARD=${BOARD} not supported.

    Please provide a value for BOARD variable e.g BOARD=brd4180a.
    Currently supported:
    - brd4180a
    - brd4180b
    ")
endif()

list(APPEND OT_PLATFORM_DEFINES "${MCU}")
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

# ==============================================================================
# Platform library
# ==============================================================================
set(OT_PLATFORM_LIB "openthread-efr32mg21")
set(OT_PLATFORM_LIB ${OT_PLATFORM_LIB} PARENT_SCOPE)

set(LD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/efr32mg21/efr32mg21.ld")

add_library(openthread-efr32mg21
    ${EFR32_COMMON_SOURCES}
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_target_properties(openthread-efr32mg21
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_link_libraries(openthread-efr32mg21
    PUBLIC
        ${EFR32_COMMON_3RD_LIBS}
        silabs-libnvm3_CM33_gcc
        silabs-efr32mg21-sdk
        -T${LD_FILE}
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
    PRIVATE
        ot-config
)

target_compile_definitions(openthread-efr32mg21
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-efr32mg21
    PRIVATE
        ${EFR32_CFLAGS}
)

target_include_directories(openthread-efr32mg21
    PUBLIC
        ${EFR32_INCLUDES}
    PRIVATE
        ${SILABS_EFR32MG2X_INCLUDES}
        ${OT_PUBLIC_INCLUDES}
)

