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
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/posix/platform
)

if(OT_DAEMON)
    add_executable(ot-daemon
        main.c
    )

    target_include_directories(ot-daemon PRIVATE ${COMMON_INCLUDES})

    target_compile_definitions(ot-daemon PRIVATE
        OPENTHREAD_POSIX_APP_TYPE=2
    )

    target_compile_options(ot-daemon PRIVATE
        ${OT_CFLAGS}
    )

    target_link_libraries(ot-daemon PRIVATE
        openthread-cli-ftd
        ${OT_PLATFORM_LIB}
        openthread-ftd
        ${OT_PLATFORM_LIB}
        openthread-ncp-ftd
        mbedcrypto
    )

    add_executable(ot-ctl
        client.cpp
    )

    target_compile_options(ot-ctl PRIVATE
        ${OT_CFLAGS}
    )

    target_include_directories(ot-ctl PRIVATE ${COMMON_INCLUDES})
else()
    add_executable(ot-cli
        main.c
    )

    target_include_directories(ot-cli PRIVATE ${COMMON_INCLUDES})

    target_compile_definitions(ot-cli PRIVATE
        OPENTHREAD_POSIX_APP_TYPE=2
    )

    target_compile_options(ot-cli PRIVATE
        ${OT_CFLAGS}
    )

    target_link_libraries(ot-cli
        openthread-cli-ftd
        ${OT_PLATFORM_LIB}
        openthread-ftd
        ${OT_PLATFORM_LIB}
        mbedcrypto
        openthread-ncp-ftd
    )

    add_executable(ot-ncp
        main.c
    )

    target_include_directories(ot-ncp PRIVATE ${COMMON_INCLUDES})

    target_compile_definitions(ot-ncp PRIVATE
        OPENTHREAD_POSIX_APP_TYPE=1
    )

    target_compile_options(ot-ncp PRIVATE
        ${OT_CFLAGS}
    )

    target_link_libraries(ot-ncp
        openthread-ncp-ftd
        ${OT_PLATFORM_LIB}
        openthread-ftd
        ${OT_PLATFORM_LIB}
        mbedcrypto
        openthread-ncp-ftd
    )
endif()
