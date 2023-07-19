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

# This file provides an example on how to implement a RCP vendor extension.

# Set name of the cmake target to link with rcp-vendor-intf
set(OT_POSIX_CONFIG_RCP_VENDOR_TARGET "vendor-lib")

# Create library target using source file "vendor_source.cpp"
add_library(${OT_POSIX_CONFIG_RCP_VENDOR_TARGET} vendor_source.cpp)

# Include header files located at "${VENDOR_INCLUDE_PATH}"
target_include_directories(${OT_POSIX_CONFIG_RCP_VENDOR_TARGET}
    PRIVATE
        ${VENDOR_INCLUDE_PATH}
)

set(EXAMPLE_DEPS_ENABLE OFF CACHE BOOL "Include example deps library if enabled")

# Search for and include an optional library using the module file
# "FindExampleRcpDeps.cmake" at path "${VENDOR_MODULE_PATH}"
if (EXAMPLE_DEPS_ENABLE)
    list(APPEND CMAKE_MODULE_PATH ${VENDOR_MODULE_PATH})
    find_package(ExampleRcpVendorDeps)

    if(ExampleRcpVendorDeps_FOUND)
        target_link_libraries(${OT_POSIX_CONFIG_RCP_VENDOR_TARGET} 
            PRIVATE
            ${ExampleRcpVendorDeps::ExampleRcpVendorDeps}
        )
    endif()
endif()
