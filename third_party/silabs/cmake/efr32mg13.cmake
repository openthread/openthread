#
#  Copyright (c) 2021, The OpenThread Authors.
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

include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/efr32mg1x.cmake)

add_library(silabs-efr32mg13-sdk)

target_compile_definitions(silabs-efr32mg13-sdk
    PRIVATE
        ${COMMON_FLAG}
)

target_include_directories(silabs-efr32mg13-sdk
    PUBLIC
        ${SILABS_COMMON_INCLUDES}
        ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG13P/Include
        ${SILABS_EFR32MG1X_INCLUDES}
)

target_sources(silabs-efr32mg13-sdk
    PRIVATE
        ${SILABS_GSDK_COMMON_SOURCES}
        ${SILABS_EFR32MG13_SOURCES}
)

target_link_libraries(silabs-efr32mg13-sdk
    PUBLIC
        silabs-mbedtls
    PRIVATE
        ${silabs-librail}
        ot-config)
