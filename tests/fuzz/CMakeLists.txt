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

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_COMPILE_OPTIONS
    -DOPENTHREAD_FTD=1
    -DOPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=1
)

set(COMMON_SOURCES
    fuzzer_platform.cpp
)

set(COMMON_LIBS
    openthread-ftd
    ${OT_MBEDTLS}
    $ENV{LIB_FUZZING_ENGINE}
    ot-config
)

add_executable(cli-received-fuzzer
    cli_received.cpp
    ${COMMON_SOURCES}
)

target_compile_options(cli-received-fuzzer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_include_directories(cli-received-fuzzer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_link_libraries(cli-received-fuzzer
    PRIVATE
        openthread-cli-ftd
        ${COMMON_LIBS}
)

add_executable(ip6-send-fuzzer
    ip6_send.cpp
    ${COMMON_SOURCES}
)

target_compile_options(ip6-send-fuzzer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_include_directories(ip6-send-fuzzer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_link_libraries(ip6-send-fuzzer
    PRIVATE
        ${COMMON_LIBS}
)

add_executable(radio-receive-done-fuzzer
    radio_receive_done.cpp
    ${COMMON_SOURCES}
)

target_compile_options(radio-receive-done-fuzzer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_include_directories(radio-receive-done-fuzzer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_link_libraries(radio-receive-done-fuzzer
    PRIVATE
        ${COMMON_LIBS}
)

add_executable(ncp-hdlc-received-fuzzer
    ncp_hdlc_received.cpp
    ${COMMON_SOURCES}
)

target_compile_options(ncp-hdlc-received-fuzzer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_include_directories(ncp-hdlc-received-fuzzer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_link_libraries(ncp-hdlc-received-fuzzer
    PRIVATE
        openthread-ncp-ftd
        ${COMMON_LIBS}
)
