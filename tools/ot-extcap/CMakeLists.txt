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

cmake_minimum_required(VERSION 3.10.2)
cmake_policy(SET CMP0048 NEW)

project(ot-extcap)

set(OPENTHREAD_DIR ${PROJECT_SOURCE_DIR}/../../)

# Configure OpenThread sub-project options
set(OT_PLATFORM "posix" CACHE STRING "Target platform" FORCE)
set(OT_SUDO OFF CACHE BOOL "Disable sudo" FORCE)
set(OT_DOC OFF CACHE BOOL "Disable documentation" FORCE)
set(OT_DAEMON OFF CACHE BOOL "Disable daemon" FORCE)
set(OT_BORDER_ROUTER OFF CACHE BOOL "Disable border router" FORCE)
set(OT_JOINER OFF CACHE BOOL "Disable joiner" FORCE)
set(OT_EXTERNAL_HEAP OFF CACHE BOOL "Disable external heap" FORCE)
set(OT_POSIX_MAX_POWER_TABLE OFF CACHE BOOL "Disable max power table" FORCE)
set(OT_COMMISSIONER OFF CACHE BOOL "Disable commissioner" FORCE)
set(OT_COAP OFF CACHE BOOL "Disable CoAP" FORCE)
set(OT_DNS_CLIENT OFF CACHE BOOL "Disable DNS client" FORCE)
set(OT_SRP_CLIENT OFF CACHE BOOL "Disable SRP client" FORCE)
set(OT_SRP_SERVER OFF CACHE BOOL "Disable SRP server" FORCE)
set(OT_DNSSD_SERVER OFF CACHE BOOL "Disable DNSSD server" FORCE)
set(OT_LINK_RAW OFF CACHE BOOL "Disable link raw" FORCE)
set(OT_UPTIME ON CACHE BOOL "Enable uptime" FORCE)
set(OT_APP_CLI OFF CACHE BOOL "Disable CLI app" FORCE)
set(OT_APP_NCP OFF CACHE BOOL "Disable NCP app" FORCE)
set(OT_APP_RCP OFF CACHE BOOL "Disable RCP app" FORCE)
set(OT_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(OT_BUILD_EXECUTABLES OFF CACHE BOOL "Disable building executables in core" FORCE)
set(OT_SETTINGS_RAM ON CACHE BOOL "Disable settings file" FORCE)

# Add OpenThread repository as a sub-project
add_subdirectory(${OPENTHREAD_DIR} ${CMAKE_CURRENT_BINARY_DIR}/openthread EXCLUDE_FROM_ALL)

include_directories(
    ${OPENTHREAD_DIR}/include
    ${OPENTHREAD_DIR}/src
    ${OPENTHREAD_DIR}/src/core
    ${OPENTHREAD_DIR}/src/include
    ${OPENTHREAD_DIR}/src/posix/platform
    ${OPENTHREAD_DIR}/src/posix/platform/include
)

add_executable(ot-extcap
    ot-extcap.cpp
    pcapng.cpp
)

target_compile_options(ot-extcap PRIVATE
    ${OT_CFLAGS}
)

target_link_libraries(ot-extcap PRIVATE
    openthread-ftd
    openthread-posix
    openthread-hdlc
    openthread-radio-spinel
    openthread-spinel-rcp
    openthread-url
    ot-config
    ot-posix-config
    ${OT_MBEDTLS}
    $<$<NOT:$<BOOL:${OT_ANDROID_NDK}>>:util>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:rt>
)

if(OT_LINKER_MAP)
    if(APPLE)
        target_link_libraries(ot-extcap PRIVATE -Wl,-map,ot-extcap.map)
    else()
        target_link_libraries(ot-extcap PRIVATE -Wl,-Map=ot-extcap.map)
    endif()
endif()

# Install the binary to the standard bin directory
install(TARGETS ot-extcap DESTINATION bin)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "ot-extcap")
    if(NOT OT_PACKAGE_VERSION)
        set(OT_PACKAGE_VERSION "0.01.00")
    endif()
    set(CPACK_PACKAGE_VERSION "${OT_PACKAGE_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpenThread Authors <openthread-users@googlegroups.com>")
    set(CPACK_PACKAGE_CONTACT "OpenThread Authors <openthread-users@googlegroups.com>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "wireshark-common")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    
    # Include maintainer scripts for dynamic install-time extcap path detection
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst"
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postrm"
    )
    
    include(CPack)
endif()
