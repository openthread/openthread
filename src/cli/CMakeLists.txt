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

add_library(openthread-cli-ftd)
add_library(openthread-cli-mtd)

set_target_properties(
    openthread-cli-ftd openthread-cli-mtd
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 98
)

target_compile_definitions(openthread-cli-ftd PRIVATE
    ${OT_PRIVATE_DEFINES}
    OPENTHREAD_FTD=1
)

target_compile_definitions(openthread-cli-mtd PRIVATE
    ${OT_PRIVATE_DEFINES}
    OPENTHREAD_MTD=1
)

target_compile_options(openthread-cli-ftd PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-cli-mtd PRIVATE
    ${OT_CFLAGS}
)

set(COMMON_INCLUDES
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_SOURCES
    cli.cpp
    cli_coap.cpp
    cli_coap_secure.cpp
    cli_commissioner.cpp
    cli_console.cpp
    cli_dataset.cpp
    cli_joiner.cpp
    cli_server.cpp
    cli_uart.cpp
    cli_udp.cpp
)

target_include_directories(openthread-cli-ftd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-cli-mtd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

target_sources(openthread-cli-ftd PRIVATE ${COMMON_SOURCES})
target_sources(openthread-cli-mtd PRIVATE ${COMMON_SOURCES})
