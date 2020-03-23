#
#  Copyright (c) 2020, The OpenThread Authors.
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

set(OT_PLATFORM_LIB "openthread-cc2650" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-cc2650-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-cc2650-config-check.h\""
    "OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE=1"
    "MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\""
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
set(CC2650_CFLAGS
    $<$<COMPILE_LANGUAGE:C>: -Wall -Wextra -Wshadow -Werror>
    $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wshadow -Wno-c++14-compat -fno-exceptions>
)

list(APPEND OT_PRIVATE_DEFINES "MBEDTLS_USER_CONFIG_FILE=\"cc2650-mbedtls-config.h\"")
set(OT_PRIVATE_DEFINES ${OT_PRIVATE_DEFINES} PARENT_SCOPE)
list(APPEND OT_PUBLIC_INCLUDES
    "${PROJECT_SOURCE_DIR}/examples/platforms/cc2650/crypto"
    "${PROJECT_SOURCE_DIR}/third_party/ti/devices/cc26x0"
)
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-cc2650
    alarm.c
    diag.c
    entropy.c
    logging.c
    misc.c
    radio.c
    system.c
    uart.c
    crypto/aes_alt.c
    crypto/sha256_alt.c
    cc2650_ccfg.c
    cc2650_startup.c
    cxx_helpers.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_property(TARGET openthread-cc2650 PROPERTY C_STANDARD 99)

target_link_libraries(openthread-cc2650 PRIVATE openthread-platform-utils)
target_link_libraries(openthread-cc2650 PRIVATE "${PROJECT_SOURCE_DIR}/third_party/ti/devices/cc26x0/driverlib/bin/gcc/driverlib.a")
target_link_options(openthread-cc2650 PUBLIC -T${PROJECT_SOURCE_DIR}/third_party/ti/devices/cc26x0/linker_files/cc26x0f128.lds)
target_link_options(openthread-cc2650 PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)

target_compile_definitions(openthread-cc2650
    PUBLIC
        ${OT_PLATFORM_DEFINES}
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)

target_compile_options(openthread-cc2650 PRIVATE ${CC2650_CFLAGS})

target_include_directories(openthread-cc2650
    PRIVATE
        ${OT_PUBLIC_INCLUDES}
        ${OT_PRIVATE_INCLUDES}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/examples/platforms
        ${PROJECT_SOURCE_DIR}/third_party/mbedtls/repo/include
        ${PROJECT_SOURCE_DIR}/third_party/mbedtls
)
