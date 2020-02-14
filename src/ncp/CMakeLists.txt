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

add_library(openthread-ncp-ftd)
add_library(openthread-ncp-mtd)
add_library(openthread-rcp)

# We depend on C++11 to validate that NCP handlers are sorted.
set_target_properties(
    openthread-ncp-mtd openthread-rcp
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 98
)

target_compile_definitions(openthread-ncp-ftd PRIVATE
    ${OT_PRIVATE_DEFINES}
    OPENTHREAD_FTD=1
    OPENTHREAD_CONFIG_NCP_UART_ENABLE=1
)

target_compile_definitions(openthread-ncp-mtd PRIVATE
    ${OT_PRIVATE_DEFINES}
    OPENTHREAD_MTD=1
    OPENTHREAD_CONFIG_NCP_UART_ENABLE=1
)

target_compile_definitions(openthread-rcp PRIVATE
    ${OT_PRIVATE_DEFINES}
    OPENTHREAD_RADIO=1
    OPENTHREAD_CONFIG_NCP_UART_ENABLE=1
)

target_compile_options(openthread-ncp-ftd PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-ncp-mtd PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-rcp PRIVATE
    ${OT_CFLAGS}
)

set(COMMON_INCLUDES
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_SOURCES
    changed_props_set.cpp
    hdlc.cpp
    ncp_base.cpp
    ncp_base_ftd.cpp
    ncp_base_mtd.cpp
    ncp_base_radio.cpp
    ncp_base_dispatcher.cpp
    ncp_buffer.cpp
    ncp_spi.cpp
    ncp_uart.cpp
    spinel.c
    spinel_decoder.cpp
    spinel_encoder.cpp
)

target_include_directories(openthread-ncp-ftd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-ncp-mtd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-rcp PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

target_sources(openthread-ncp-ftd PRIVATE ${COMMON_SOURCES})
target_sources(openthread-ncp-mtd PRIVATE ${COMMON_SOURCES})
target_sources(openthread-rcp PRIVATE ${COMMON_SOURCES})
