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

file(GLOB TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/cli/test-*.py")

foreach(SCRIPT ${TEST_SCRIPTS})
    # Extract the base name of the script for the test name
    get_filename_component(TEST_NAME "${SCRIPT}" NAME_WE)

    if(TEST_NAME MATCHES "-multi-radio-")
        message(STATUS "skipping non multi radio test ${TEST_NAME}")
        continue()
    endif()

    add_test(
        NAME "${TEST_NAME}"
        COMMAND "${SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    if(OT_LOG_OUTPUT STREQUAL "PLATFORM_DEFINED")
        set(TORANJ_SAVE_LOGS "1")
    else()
        set(TORANJ_SAVE_LOGS "0")
    endif()
    set_property(TEST "${TEST_NAME}" PROPERTY ENVIRONMENT
        top_builddir="${PROJECT_BINARY_DIR}"
        TORANJ_VERBOSE=1
        TORANJ_SAVE_LOGS="${TORANJ_SAVE_LOGS}")
endforeach()
