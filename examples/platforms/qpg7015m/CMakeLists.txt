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

set(OT_PLATFORM_LIB "openthread-qpg7015m" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-qpg7015m-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-qpg7015m-config-check.h\""
)

list(APPEND OT_PLATFORM_DEFINES
    "_BSD_SOURCE=1"
    "_DEFAULT_SOURCE=1"
)

set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

if(OT_CFLAGS MATCHES "-pedantic-errors")
    string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")
endif()

if(OT_CFLAGS MATCHES "-Wno-c\\+\\+14-compat")
    string(REPLACE "-Wno-c++14-compat" "" OT_CFLAGS "${OT_CFLAGS}")
endif()

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-qpg7015m
    alarm.c
    diag.c     
    entropy.c
    flash.c
    logging.c
    misc.c
    radio.c
    system.c
    uart-posix.c
)

set_target_properties(
    openthread-qpg7015m
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_link_libraries(openthread-qpg7015m
    PRIVATE
        qpg7015m-driver
        ${OT_MBEDTLS}
        ot-config
    PUBLIC
        -lrt
        -pthread
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_PROPERTY:NAME>.map
)

target_compile_definitions(openthread-qpg7015m
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-qpg7015m
    PRIVATE
        ${OT_CFLAGS}
)

target_include_directories(openthread-qpg7015m
    PRIVATE
        ${OT_PUBLIC_INCLUDES}
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/examples/platforms
)
