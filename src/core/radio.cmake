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

add_library(openthread-radio)

set_target_properties(
    openthread-radio
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_compile_definitions(openthread-radio PRIVATE
    OPENTHREAD_RADIO=1
)

target_compile_options(openthread-radio PRIVATE
    ${OT_CFLAGS}
)

target_include_directories(openthread-radio PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

target_sources(openthread-radio PRIVATE
    api/diags_api.cpp
    api/instance_api.cpp
    api/link_raw_api.cpp
    api/logging_api.cpp
    api/random_noncrypto_api.cpp
    api/tasklet_api.cpp
    common/instance.cpp
    common/logging.cpp
    common/random_manager.cpp
    common/string.cpp
    common/tasklet.cpp
    common/timer.cpp
    crypto/aes_ccm.cpp
    crypto/aes_ecb.cpp
    diags/factory_diags.cpp
    mac/link_raw.cpp
    mac/mac_frame.cpp
    mac/mac_types.cpp
    mac/sub_mac.cpp
    mac/sub_mac_callbacks.cpp
    radio/radio.cpp
    radio/radio_callbacks.cpp
    radio/radio_platform.cpp
    thread/link_quality.cpp
    utils/lookup_table.cpp
    utils/parse_cmdline.cpp
)

if(OT_VENDOR_EXTENSION)
  target_sources(openthread-radio PRIVATE ${OT_VENDOR_EXTENSION})
endif()

target_link_libraries(openthread-radio
    PRIVATE
        ${OT_MBEDTLS}
        ot-config
)
