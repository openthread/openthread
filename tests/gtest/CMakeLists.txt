#
#  Copyright (c) 2024, The OpenThread Authors.
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

cmake_minimum_required(VERSION 3.14)
project(openthread-gtest)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_library(ot-fake-platform
    fake_platform.cpp
)
target_link_libraries(ot-fake-platform
    ot-config
)

add_library(ot-fake-ftd INTERFACE)
target_link_libraries(ot-fake-ftd INTERFACE
    openthread-ftd
    ot-fake-platform
)

add_executable(ot-ftd-gtest
    dataset_test.cpp
)
target_link_libraries(ot-ftd-gtest
    ot-fake-ftd
    GTest::gmock_main
)
gtest_discover_tests(ot-ftd-gtest)

add_library(ot-fake-rcp
    fake_coprocessor_platform.cpp
)
target_link_libraries(ot-fake-rcp
    openthread-hdlc
    openthread-rcp
    openthread-radio-spinel
    openthread-radio
    ot-fake-platform
)

add_executable(ot-radio-spinel-rcp-gtest
    radio_spinel_rcp_test.cpp
)
target_link_libraries(ot-radio-spinel-rcp-gtest
    ot-fake-rcp
    GTest::gmock_main
)

gtest_discover_tests(ot-radio-spinel-rcp-gtest)
