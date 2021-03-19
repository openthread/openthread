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

# Alias OT_PLATFORM as EFR32_PLATFORM
set(EFR32_PLATFORM ${OT_PLATFORM})

string(TOLOWER "${EFR32_PLATFORM}" PLATFORM_LOWERCASE)
string(TOUPPER "${EFR32_PLATFORM}" PLATFORM_UPPERCASE)

string(TOLOWER "${BOARD}" BOARD_LOWERCASE)
string(TOUPPER "${BOARD}" BOARD_UPPERCASE)

set(SILABS_GSDK_VERSION 3.1)
set(SILABS_GSDK_DIR ${PROJECT_SOURCE_DIR}/third_party/silabs/gecko_sdk_suite/v${SILABS_GSDK_VERSION})

# Check if GSDK exists
if(NOT EXISTS "${SILABS_GSDK_DIR}")
    message(FATAL_ERROR "Cannot find: ${SILABS_GSDK_DIR}\nCheck that Silicon Labs GSDK ${SILABS_GSDK_VERSION} was installed properly")
endif()
