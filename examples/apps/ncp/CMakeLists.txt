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

add_executable(ot-ncp-ftd
    main.c
)

add_executable(ot-ncp-mtd
    main.c
)

add_executable(ot-rcp
    main.c
)

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src/core
)

target_include_directories(ot-ncp-ftd PRIVATE ${COMMON_INCLUDES})
target_include_directories(ot-ncp-mtd PRIVATE ${COMMON_INCLUDES})
target_include_directories(ot-rcp PRIVATE ${COMMON_INCLUDES})

target_compile_definitions(ot-ncp-ftd PRIVATE ${OT_PRIVATE_DEFINES})
target_compile_definitions(ot-ncp-mtd PRIVATE ${OT_PRIVATE_DEFINES})
target_compile_definitions(ot-rcp PRIVATE ${OT_PRIVATE_DEFINES})

target_link_libraries(ot-ncp-ftd
    openthread-ncp-ftd
    ${OT_PLATFORM_LIB}
    openthread-ftd
    ${OT_PLATFORM_LIB}
    mbedcrypto
)

target_link_libraries(ot-ncp-mtd
    openthread-ncp-mtd
    ${OT_PLATFORM_LIB}
    openthread-mtd
    ${OT_PLATFORM_LIB}
    mbedcrypto
)

target_link_libraries(ot-rcp
    openthread-rcp
    ${OT_PLATFORM_LIB}
    openthread-radio
    ${OT_PLATFORM_LIB}
)

install(TARGETS ot-rcp ot-ncp-ftd ot-ncp-mtd
    DESTINATION bin)
