#
#  Copyright (c) 2026, The OpenThread Authors.
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

# Spinel-only link test
#
# Verifies that the Spinel/HDLC libraries plus a minimal set of OT core
# files can link into a final executable *without* the full Thread stack.
# This mirrors the ESP-IDF OPENTHREAD_SPINEL_ONLY build mode used by the
# Zigbee gateway and catches linker errors (e.g. unwanted Instance::Get()
# references) that static-library-only builds cannot detect.
#
# The target is EXCLUDE_FROM_ALL so it is only built when explicitly
# requested via ninja target name.

add_library(openthread-spinel-only-core STATIC EXCLUDE_FROM_ALL
    ${PROJECT_SOURCE_DIR}/src/core/api/error_api.cpp
    ${PROJECT_SOURCE_DIR}/src/core/api/logging_api.cpp
    ${PROJECT_SOURCE_DIR}/src/core/common/error.cpp
    ${PROJECT_SOURCE_DIR}/src/core/common/log.cpp
    ${PROJECT_SOURCE_DIR}/src/core/common/string.cpp
)

target_include_directories(openthread-spinel-only-core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/src/include
        ${PROJECT_SOURCE_DIR}/examples/platforms/simulation
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(openthread-spinel-only-core PRIVATE
    OPENTHREAD_FTD=0
    OPENTHREAD_MTD=0
    OPENTHREAD_RADIO=0
    OPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-spinel-only-config.h"
    OPENTHREAD_PLATFORM_CORE_CONFIG_FILE="openthread-core-simulation-config.h"
    OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE=0
)

target_compile_options(openthread-spinel-only-core PRIVATE
    ${OT_CFLAGS}
)

add_executable(ot-test-spinel-only-link EXCLUDE_FROM_ALL
    platform_stubs.c
    mac_frame_stubs.cpp
)

target_include_directories(ot-test-spinel-only-link PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/simulation
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(ot-test-spinel-only-link PRIVATE
    OPENTHREAD_FTD=0
    OPENTHREAD_MTD=0
    OPENTHREAD_RADIO=0
    OPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-spinel-only-config.h"
    OPENTHREAD_PLATFORM_CORE_CONFIG_FILE="openthread-core-simulation-config.h"
    OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE=0
)

target_link_libraries(ot-test-spinel-only-link PRIVATE
    -Wl,--whole-archive
    -Wl,--allow-multiple-definition
    openthread-radio-spinel
    openthread-spinel-rcp
    openthread-hdlc
    openthread-platform
    openthread-spinel-only-core
    -Wl,--no-whole-archive
)
