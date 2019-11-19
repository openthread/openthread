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
option(OT_BUILTIN_MBEDTLS "Enable builtin mbedTLS" ON)

include("${PROJECT_SOURCE_DIR}/etc/cmake/checks.cmake")
include("${PROJECT_SOURCE_DIR}/etc/cmake/options.cmake")

execute_process(
    COMMAND "${PROJECT_SOURCE_DIR}/third_party/nlbuild-autotools/repo/scripts/mkversion" "-b" "`cat .default-version`" "."
    OUTPUT_VARIABLE __version OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(OT_VERSION "${__version}")

list(APPEND OT_PRIVATE_DEFINES
    "PACKAGE_NAME=\"OPENTHREAD\""
    "PACKAGE_VERSION=\"${OT_VERSION}\""
)

set(OT_PLATFORM "none" CACHE STRING "Target platform chosen by the user at configure time")

function(ot_get_platforms arg_platforms)
    set(result "none")
    set(platforms_dir "${PROJECT_SOURCE_DIR}/examples/platforms")
    file(GLOB platforms RELATIVE "${platforms_dir}" "${platforms_dir}/*")
    foreach(platform IN LISTS platforms)
        if(IS_DIRECTORY "${platforms_dir}/${platform}")
            list(APPEND result "${platform}")
        endif()
    endforeach()

    set(${arg_platforms} "${result}" PARENT_SCOPE)
endfunction()

ot_get_platforms(OT_EXAMPLE_PLATFORMS)
set_property(CACHE OT_PLATFORM PROPERTY STRINGS ${OT_EXAMPLE_PLATFORMS})
if(NOT OT_PLATFORM IN_LIST OT_EXAMPLE_PLATFORMS)
    message(FATAL_ERROR "Platform unknown: ${OT_PLATFORM}")
endif()

list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/etc/cmake)
list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include)

if(NOT OT_PLATFORM MATCHES "none")
    list(APPEND OT_PRIVATE_INCLUDES ${PROJECT_SOURCE_DIR}/examples/platforms/${OT_PLATFORM})
    include("${PROJECT_SOURCE_DIR}/examples/platforms/${OT_PLATFORM}/platform.cmake")
endif()

list(APPEND OT_PRIVATE_DEFINES ${OT_PLATFORM_DEFINES})

if(OT_BUILTIN_MBEDTLS)
    list(APPEND OT_PRIVATE_INCLUDES ${PROJECT_SOURCE_DIR}/third_party/mbedtls)
    list(APPEND OT_PRIVATE_INCLUDES ${PROJECT_SOURCE_DIR}/third_party/mbedtls/repo/include)

    list(APPEND OT_PRIVATE_DEFINES
        "MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\""
    )
endif()

if(NOT OT_PLATFORM MATCHES "none")
    add_subdirectory(examples)
endif()

add_subdirectory(src)
add_subdirectory(third_party)
