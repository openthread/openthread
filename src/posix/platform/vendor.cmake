#
#  Copyright (c) 2023, The OpenThread Authors.
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

set(OT_POSIX_CONFIG_RCP_VENDOR_INTERFACE "vendor_interface_example.cpp"
    CACHE STRING "vendor interface implementation")

set(OT_POSIX_CONFIG_RCP_VENDOR_DEPS_PACKAGE "" CACHE STRING
    "path to CMake file to define and link posix vendor extension")

set(OT_POSIX_RCP_VENDOR_TARGET "" CACHE STRING 
    "name of vendor extension CMake target to link with posix library")

if(OT_POSIX_RCP_VENDOR_BUS)
    add_library(rcp-vendor-intf ${OT_POSIX_CONFIG_RCP_VENDOR_INTERFACE})

    target_link_libraries(rcp-vendor-intf PUBLIC ot-posix-config)

    target_include_directories(rcp-vendor-intf
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/src/core
            ${PROJECT_SOURCE_DIR}/src/posix/platform/include
    )

    target_link_libraries(openthread-posix PUBLIC rcp-vendor-intf)

    if (OT_POSIX_CONFIG_RCP_VENDOR_DEPS_PACKAGE)
        include(${OT_POSIX_CONFIG_RCP_VENDOR_DEPS_PACKAGE})
        if (OT_POSIX_CONFIG_RCP_VENDOR_TARGET)
            target_link_libraries(rcp-vendor-intf PRIVATE ${OT_POSIX_CONFIG_RCP_VENDOR_TARGET})
        endif()
    endif()
endif()
