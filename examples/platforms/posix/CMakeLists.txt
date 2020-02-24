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

set(OT_PLATFORM_LIB "openthread-posix" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-posix-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "_BSD_SOURCE=1"
    "_DEFAULT_SOURCE=1"
    "OPENTHREAD_EXAMPLES_POSIX=1"
    "OPENTHREAD_POSIX=1"
    "OPENTHREAD_CONFIG_NCP_UART_ENABLE=1"
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-posix
    alarm.c
    diag.c
    entropy.c
    flash.c
    logging.c
    misc.c
    radio.c
    spi-stubs.c
    system.c
    uart-posix.c
    sim/alarm-sim.c
    sim/platform-sim.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_property(TARGET openthread-posix PROPERTY C_STANDARD 99)

find_library(LIBRT rt)
if(LIBRT)
    target_link_libraries(openthread-posix PRIVATE ${LIBRT})
endif()

target_link_libraries(openthread-posix PRIVATE openthread-platform-utils)

target_compile_definitions(openthread-posix PUBLIC ${OT_PLATFORM_DEFINES})

target_compile_options(openthread-posix PRIVATE
    ${OT_CFLAGS}
)

target_include_directories(openthread-posix PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src/core
)
