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

add_library(openthread-native-its-file STATIC)
target_sources(openthread-native-its-file PRIVATE
    src/file/its_file.c
)
target_include_directories(openthread-native-its-file PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${OT_PUBLIC_INCLUDES}
    $<TARGET_PROPERTY:ot-config,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(openthread-native-its-file
    PRIVATE
        $<TARGET_PROPERTY:ot-config,INTERFACE_COMPILE_DEFINITIONS>
    PUBLIC
        OPENTHREAD_PSA_CRYPTO_NATIVE_ITS_FILE=1
)

add_library(openthread-native-its-ram STATIC)
target_sources(openthread-native-its-ram PRIVATE
    src/ram/its_ram.c
)
target_include_directories(openthread-native-its-ram PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${OT_PUBLIC_INCLUDES}
    $<TARGET_PROPERTY:ot-config,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(openthread-native-its-ram
    PRIVATE
        $<TARGET_PROPERTY:ot-config,INTERFACE_COMPILE_DEFINITIONS>
    PUBLIC
        OPENTHREAD_PSA_CRYPTO_NATIVE_ITS_RAM=1
)

