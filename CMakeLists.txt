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

cmake_minimum_required(VERSION 3.13.1)
project(openthread)

option(OT_BUILD_EXECUTABLES "Build executables" ON)
option(OT_COVERAGE "enable coverage" OFF)
set(OT_EXTERNAL_MBEDTLS "" CACHE STRING "Specify external mbedtls library")

include(TestBigEndian)
TEST_BIG_ENDIAN(OT_BIG_ENDIAN)
if(OT_BIG_ENDIAN)
    list(APPEND OT_PRIVATE_DEFINES "BYTE_ORDER_BIG_ENDIAN=1")
endif()

include("${PROJECT_SOURCE_DIR}/etc/cmake/checks.cmake")
include("${PROJECT_SOURCE_DIR}/etc/cmake/options.cmake")
include("${PROJECT_SOURCE_DIR}/etc/cmake/functions.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
    option(OT_COMPILE_WARNING_AS_ERROR "whether to include -Werror -pedantic-errors with gcc-compatible compilers")
    if (OT_COMPILE_WARNING_AS_ERROR)
        set(OT_CFLAGS -Werror -pedantic-errors)
    endif()

    if(OT_COVERAGE)
        list(APPEND OT_PRIVATE_DEFINES "OPENTHREAD_ENABLE_COVERAGE=1")
        add_compile_options(-g -O0 --coverage)
        add_link_options(--coverage)
    endif()

    set(OT_CFLAGS
        $<$<COMPILE_LANGUAGE:C>:${OT_CFLAGS} -Wall -Wextra -Wshadow>
        $<$<COMPILE_LANGUAGE:CXX>:${OT_CFLAGS} -Wall -Wextra -Wshadow -Wno-c++14-compat -fno-exceptions>
    )
endif()

file(READ .default-version OT_DEFAULT_VERSION)
execute_process(
    COMMAND bash "-c" "third_party/nlbuild-autotools/repo/scripts/mkversion -b ${OT_DEFAULT_VERSION}"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE OT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Version: ${OT_VERSION}")

list(APPEND OT_PRIVATE_DEFINES
    "PACKAGE_NAME=\"OPENTHREAD\""
    "PACKAGE_VERSION=\"${OT_VERSION}\""
)

set(OT_THREAD_VERSION "1.1" CACHE STRING "Thread version chosen by the user at configure time")
set_property(CACHE OT_THREAD_VERSION PROPERTY STRINGS "1.1" "1.2")
if(${OT_THREAD_VERSION} EQUAL "1.1")
    list(APPEND OT_PRIVATE_DEFINES "OPENTHREAD_CONFIG_THREAD_VERSION=OT_THREAD_VERSION_1_1")
elseif(${OT_THREAD_VERSION} EQUAL "1.2")
    list(APPEND OT_PRIVATE_DEFINES "OPENTHREAD_CONFIG_THREAD_VERSION=OT_THREAD_VERSION_1_2")
else()
    message(FATAL_ERROR "Thread version unknown: ${OT_THREAD_VERSION}")
endif()

set(OT_PLATFORM "none" CACHE STRING "Target platform chosen by the user at configure time")
ot_get_platforms(OT_EXAMPLE_PLATFORMS)
set_property(CACHE OT_PLATFORM PROPERTY STRINGS ${OT_EXAMPLE_PLATFORMS})
if(NOT OT_PLATFORM IN_LIST OT_EXAMPLE_PLATFORMS)
    if(NOT OT_PLATFORM STREQUAL "posix")
        message(FATAL_ERROR "Platform unknown: ${OT_PLATFORM}")
    endif()
endif()

set(OT_LOG_OUTPUT_VALUES
    "APP"
    "DEBUG_UART"
    "NCP_SPINEL"
    "NONE"
    "PLATFORM_DEFINED"
)
if(OT_REFERENCE_DEVICE AND NOT OT_PLATFORM STREQUAL "posix")
    set(OT_LOG_OUTPUT "APP" CACHE STRING "Set log output to application for reference device")
else()
    set(OT_LOG_OUTPUT "" CACHE STRING "Where log output goes to")
endif()
set_property(CACHE OT_LOG_OUTPUT PROPERTY STRINGS ${OT_LOG_OUTPUT_VALUES})
if(OT_LOG_OUTPUT)
    if(NOT OT_LOG_OUTPUT IN_LIST OT_LOG_OUTPUT_VALUES)
        message(FATAL_ERROR "Log output unknown: ${OT_LOG_OUTPUT}")
    endif()
    list(APPEND OT_PRIVATE_DEFINES "OPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_${OT_LOG_OUTPUT}")
    message(STATUS "Log output: ${OT_LOG_OUTPUT}")
endif()

# OT_CONFIG allows users to specify the path to OpenThread project core
# config header file. The default value of this parameter is empty string.
# When not specified by user (value is ""), a platform cmake file may
# choose to change this variable to provide its own core config header
# file instead.

set(OT_CONFIG "" CACHE STRING "OpenThread project-specific config header file chosen by user at configure time")

list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_BINARY_DIR}/etc/cmake)
list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/etc/cmake)
list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include)

if(OT_PLATFORM STREQUAL "posix")
    list(APPEND OT_PRIVATE_INCLUDES ${PROJECT_SOURCE_DIR}/src/posix/platform)
    add_subdirectory("${PROJECT_SOURCE_DIR}/src/posix/platform")
elseif(NOT OT_PLATFORM MATCHES "none")
    list(APPEND OT_PRIVATE_INCLUDES ${PROJECT_SOURCE_DIR}/examples/platforms/${OT_PLATFORM})
    add_subdirectory("${PROJECT_SOURCE_DIR}/examples/platforms/${OT_PLATFORM}")
endif()

if(OT_CONFIG)
    list(APPEND OT_PRIVATE_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")
    message(STATUS "Project core config: \"${OT_CONFIG}\"")
endif()

list(APPEND OT_PRIVATE_DEFINES ${OT_PLATFORM_DEFINES})

if(OT_PLATFORM STREQUAL "posix")
    if(OT_BUILD_EXECUTABLES)
        add_subdirectory(src/posix)
    endif()
elseif(NOT OT_PLATFORM MATCHES "none")
    add_subdirectory(examples)
endif()

add_subdirectory(src)
add_subdirectory(third_party)

if(OT_PLATFORM STREQUAL "simulation")
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Definitions: ${OT_PRIVATE_DEFINES}")
