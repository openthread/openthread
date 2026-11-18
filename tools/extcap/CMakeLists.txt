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

cmake_minimum_required(VERSION 3.12)

project(ot-extcap)

get_filename_component(OPENTHREAD_DIR "${PROJECT_SOURCE_DIR}/../.." ABSOLUTE)

# Configure the OpenThread sub-project: POSIX platform, settings kept in RAM only (nothing is persisted), and no
# apps or executables. Other OpenThread features keep their defaults.
set(OT_PLATFORM "posix" CACHE STRING "Target platform")
set(OT_SETTINGS_RAM ON CACHE BOOL "Keep settings in RAM only")
set(OT_APP_CLI OFF CACHE BOOL "Disable CLI app")
set(OT_APP_NCP OFF CACHE BOOL "Disable NCP app")
set(OT_APP_RCP OFF CACHE BOOL "Disable RCP app")
set(OT_BUILD_EXECUTABLES OFF CACHE BOOL "Disable building executables")
set(OT_READLINE OFF CACHE STRING "Disable readline")

# The POSIX platform builds in all log levels. Start at debug level so that logs emitted while the RCP is being
# initialized (before ot-extcap can set the level) are available with `--debug`; without it, ot-extcap filters
# them with setlogmask() and lowers the level once initialized.
add_compile_definitions(OPENTHREAD_CONFIG_LOG_LEVEL_INIT=OT_LOG_LEVEL_DEBG)

# Add the top-level OpenThread project as a sub-project
add_subdirectory(${OPENTHREAD_DIR} ${CMAKE_CURRENT_BINARY_DIR}/openthread EXCLUDE_FROM_ALL)

add_executable(ot-extcap
    extcap.cpp
    pcapng.cpp
)

# CMAKE_CXX_STANDARD and OT_CFLAGS are scoped to the OpenThread sub-project directory, so apply them explicitly.
set_target_properties(ot-extcap PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

get_directory_property(OT_CFLAGS DIRECTORY "${OPENTHREAD_DIR}" DEFINITION OT_CFLAGS)
target_compile_options(ot-extcap PRIVATE ${OT_CFLAGS})

target_include_directories(ot-extcap PRIVATE
    ${OPENTHREAD_DIR}/include
    ${OPENTHREAD_DIR}/src
    ${OPENTHREAD_DIR}/src/core
    ${OPENTHREAD_DIR}/src/include
    ${OPENTHREAD_DIR}/src/posix/platform
    ${OPENTHREAD_DIR}/src/posix/platform/include
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
)

# Install the binary to the standard bin directory
install(TARGETS ot-extcap DESTINATION bin)
