#
#  Copyright (c) 2025, The OpenThread Authors.
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

set(OT_PLATFORM_LIB_MDNS "openthread-posix-mdns" PARENT_SCOPE)

if(NOT OT_PLATFORM_CONFIG)
    set(OT_PLATFORM_CONFIG "openthread-core-posix-mdns-config.h" PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "_BSD_SOURCE=1"
    "_DEFAULT_SOURCE=1"
    "OPENTHREAD_EXAMPLES_POSIX_MDNS=1"
)

set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

add_library(openthread-posix-mdns
    alarm.c
    logging.c
    mdns_socket.c
    misc.c
    system.c
    uart.c
)

target_compile_definitions(openthread-posix-mdns PRIVATE OPENTHREAD_MDNS=1)

find_library(LIBRT rt)
if(LIBRT)
    target_link_libraries(openthread-posix-mdns PRIVATE ${LIBRT})
endif()

target_link_libraries(openthread-posix-mdns PRIVATE
    openthread-platform
    ot-config
)

target_compile_options(openthread-posix-mdns PRIVATE
    ${OT_CFLAGS}
)

target_include_directories(openthread-posix-mdns PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CPACK_PACKAGE_NAME "openthread-posix-mdns")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpenThread Authors (https://github.com/openthread/openthread)")
    set(CPACK_PACKAGE_CONTACT "OpenThread Authors (https://github.com/openthread/openthread)")
    include(CPack)
endif()
