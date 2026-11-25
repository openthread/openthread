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

add_library(openthread-radio-spinel)
add_library(openthread-spinel-ncp)
add_library(openthread-spinel-rcp)

target_compile_definitions(openthread-radio-spinel PRIVATE
    OPENTHREAD_FTD=0
    OPENTHREAD_MTD=0
    OPENTHREAD_RADIO=0
)

target_compile_definitions(openthread-spinel-ncp PRIVATE
    OPENTHREAD_FTD=0
    OPENTHREAD_MTD=0
    OPENTHREAD_RADIO=0
    PUBLIC OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=1
)

target_compile_definitions(openthread-spinel-rcp PRIVATE
    OPENTHREAD_FTD=0
    OPENTHREAD_MTD=0
    OPENTHREAD_RADIO=0
    PUBLIC OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=0
)

target_compile_options(openthread-radio-spinel PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-spinel-ncp PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-spinel-rcp PRIVATE
    ${OT_CFLAGS}
)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_SOURCES
    spinel.c
    spinel_buffer.cpp
    spinel_decoder.cpp
    spinel_encoder.cpp
    spinel_helper.cpp
    spinel_prop_codec.cpp
)

set(OT_SPINEL_VENDOR_HOOK_SOURCE "" CACHE STRING "set vendor hook source file for Spinel")
set(OT_SPINEL_VENDOR_HOOK_HEADER "" CACHE STRING "set vendor hook header file for Spinel")
if(OT_SPINEL_VENDOR_HOOK_SOURCE)
    target_compile_definitions(openthread-spinel-rcp PUBLIC "OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE=1")
    target_compile_definitions(openthread-spinel-ncp PUBLIC "OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE=1")
    target_compile_definitions(openthread-spinel-rcp PUBLIC "OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER=\"${OT_SPINEL_VENDOR_HOOK_HEADER}\"")
    target_compile_definitions(openthread-spinel-ncp PUBLIC "OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER=\"${OT_SPINEL_VENDOR_HOOK_HEADER}\"")
    list(APPEND COMMON_SOURCES ${OT_SPINEL_VENDOR_HOOK_SOURCE_DIR}${OT_SPINEL_VENDOR_HOOK_SOURCE})
    list(APPEND COMMON_INCLUDES ${OT_SPINEL_VENDOR_HOOK_SOURCE_DIR})
endif()

target_include_directories(openthread-radio-spinel PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-spinel-ncp PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-spinel-rcp PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

target_sources(openthread-radio-spinel
    PRIVATE
        logger.cpp
        radio_spinel.cpp
        spinel_driver.cpp
        spinel_helper.cpp
)
target_sources(openthread-spinel-ncp PRIVATE ${COMMON_SOURCES})
target_sources(openthread-spinel-rcp PRIVATE ${COMMON_SOURCES})

target_link_libraries(openthread-radio-spinel
    INTERFACE
        ot-lib-config
    PRIVATE
        openthread-platform
        ot-config
)

target_link_libraries(openthread-spinel-ncp
    INTERFACE
        ot-lib-config
    PRIVATE
        ot-config-ftd
        ot-config
)

target_link_libraries(openthread-spinel-rcp
    INTERFACE
        ot-lib-config
    PRIVATE
        ot-config-radio
        ot-config
)

# A host-side Spinel library with no OpenThread core or platform link dependency.
# Keep the existing NCP/RCP and RadioSpinel targets and their configurations intact.
add_library(openthread-spinel-standalone STATIC
    logger.cpp
    spinel_driver.cpp
    spinel.c
    spinel_buffer.cpp
    spinel_decoder.cpp
    spinel_encoder.cpp
    spinel_helper.cpp
    spinel_prop_codec.cpp
)
# Build-tree aliases; all names refer to the same archive and usage requirements.
add_library(openthread-spinel ALIAS openthread-spinel-standalone)
add_library(OpenThread::Spinel ALIAS openthread-spinel-standalone)

target_include_directories(openthread-spinel-standalone PUBLIC
    ${OT_PUBLIC_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/include
)
target_compile_definitions(openthread-spinel-standalone PUBLIC
    OPENTHREAD_SPINEL_CONFIG_STANDALONE_ENABLE=1
    OPENTHREAD_SPINEL_CONFIG_LOG_ENABLE=0
    OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=0
)
# Keep the standalone library and its consumers on the same driver layout.
if(OT_MULTIPAN_RCP)
    target_compile_definitions(openthread-spinel-standalone PUBLIC
        OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1
    )
endif()

if(BUILD_TESTING)
    add_executable(ot-test-spinel
        spinel.c
    )
    target_compile_definitions(ot-test-spinel
        PRIVATE -DSPINEL_SELF_TEST=1 -D_GNU_SOURCE
    )
    add_test(NAME ot-test-spinel COMMAND ot-test-spinel)

    # Exercise the same library target available to downstream hosts.
    add_executable(ot-test-spinel-standalone
        ${PROJECT_SOURCE_DIR}/tests/unit/test_spinel_standalone.cpp
    )
    target_include_directories(ot-test-spinel-standalone PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/unit
    )
    target_link_libraries(ot-test-spinel-standalone PRIVATE OpenThread::Spinel)
    add_test(NAME ot-test-spinel-standalone COMMAND ot-test-spinel-standalone)

    # Check the short alias from a separate consumer, using the same library.
    add_executable(ot-test-spinel-alias
        ${PROJECT_SOURCE_DIR}/tests/unit/test_spinel_standalone.cpp
    )
    target_include_directories(ot-test-spinel-alias PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/unit
    )
    target_link_libraries(ot-test-spinel-alias PRIVATE openthread-spinel)
    add_test(NAME ot-test-spinel-alias COMMAND ot-test-spinel-alias)

    # Check default logging delegation and formatting against host-provided hooks.
    add_executable(ot-test-spinel-logger
        ${PROJECT_SOURCE_DIR}/tests/unit/test_spinel_logger.cpp
        logger.cpp
        spinel.c
    )
    target_include_directories(ot-test-spinel-logger PRIVATE
        ${OT_PUBLIC_INCLUDES}
        ${COMMON_INCLUDES}
        ${PROJECT_SOURCE_DIR}/src/include
        ${PROJECT_SOURCE_DIR}/tests/unit
    )
    add_test(NAME ot-test-spinel-logger COMMAND ot-test-spinel-logger)
endif()
