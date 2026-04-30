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

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/posix/platform
    ${PROJECT_SOURCE_DIR}/src/posix/platform/include
)

ot_option(OT_PLATFORM_POSIX_CONFIG_FILE OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE "posix config file")

set(OT_READLINE_VALUES
    "readline"
    "edit"
)

set(OT_READLINE "" CACHE STRING "set readline library name")
set_property(CACHE OT_READLINE PROPERTY STRINGS ${OT_READLINE_VALUES})

if(OT_READLINE STREQUAL "")
    foreach(X IN LISTS OT_READLINE_VALUES)
        find_library(READLINE ${X})
        if (READLINE)
            set(OT_READLINE ${X})
            break()
        endif()
    endforeach()
elseif(OT_READLINE)
    find_library(READLINE ${OT_READLINE})

    if (NOT READLINE)
        message(FATAL_ERROR "Failed to find ${OT_READLINE}")
    endif()
endif()

if (READLINE)
    message(STATUS "Readline: ${OT_READLINE}")

    find_library(NCURSES ncurses)
    if (NOT NCURSES)
        message(FATAL_ERROR "Failed to find ncurses")
    endif()

    list(APPEND READLINE_LINK_LIBRARIES "${READLINE}" "${NCURSES}")
endif()

if(OT_DAEMON)
    include(daemon.cmake)

    if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        set(CPACK_PACKAGE_NAME "openthread-daemon")
    endif()
elseif(OT_APP_CLI)
    include(cli.cmake)

    if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        set(CPACK_PACKAGE_NAME "openthread-standalone")
    endif()
endif()

set(OT_POSIX_FACTORY_CONFIG "" CACHE STRING "OpenThread factory config file")
set(OT_POSIX_PRODUCT_CONFIG "" CACHE STRING "OpenThread product config file")

if (OT_POSIX_FACTORY_CONFIG)
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE=\"${OT_POSIX_FACTORY_CONFIG}\"")
    message(STATUS "OT_POSIX_FACTORY_CONFIG=\"${OT_POSIX_FACTORY_CONFIG}\"")
endif()

if (OT_POSIX_PRODUCT_CONFIG)
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE=\"${OT_POSIX_PRODUCT_CONFIG}\"")
    message(STATUS "OT_POSIX_PRODUCT_CONFIG=\"${OT_POSIX_PRODUCT_CONFIG}\"")
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpenThread Authors (https://github.com/openthread/openthread)")
    set(CPACK_PACKAGE_CONTACT "OpenThread Authors (https://github.com/openthread/openthread)")
    include(CPack)
endif()
