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

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_INCLUDES_RCP
    ${COMMON_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core/radio
)

set(COMMON_COMPILE_OPTIONS
    -DOPENTHREAD_FTD=1
    -DOPENTHREAD_MTD=0
    -DOPENTHREAD_RADIO=0
    -DOPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=1
)

set(COMMON_COMPILE_OPTIONS_RCP
    -DOPENTHREAD_FTD=0
    -DOPENTHREAD_MTD=0
    -DOPENTHREAD_RADIO=1
    -DOPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=1
    -DOPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE=0
)

set(MULTIPAN_RCP_COMPILE_OPTIONS
    -DOPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE=1
    -DOPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE=1
    -DOPENTHREAD_CONFIG_LOG_PREPEND_UPTIME=0
    -DOPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE=0 # used to skip backoff and request tx from platform directly.
    -DOPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1
)

add_library(ot-test-platform-ftd
    test_platform.cpp
    test_util.cpp
)
add_library(ot-test-platform-rcp
    test_platform.cpp
    test_util.cpp
)

target_include_directories(ot-test-platform-ftd
    PRIVATE
        ${COMMON_INCLUDES}
)

target_include_directories(ot-test-platform-rcp
    PRIVATE
	${COMMON_INCLUDES}
)

target_compile_options(ot-test-platform-ftd
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_compile_options(ot-test-platform-rcp
    PRIVATE
        ${COMMON_COMPILE_OPTIONS_RCP}
)

if(OT_MULTIPAN_RCP)
    target_compile_options(ot-test-platform-rcp
        PRIVATE
            "-DOPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE=1"
            "-DOPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE=1"
            "-DOPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1"
    )
endif()

target_link_libraries(ot-test-platform-ftd
    PRIVATE
        ot-config
        ${OT_MBEDTLS}
)

target_link_libraries(ot-test-platform-rcp
    PRIVATE
        ot-config
        ${OT_MBEDTLS}
)

set(COMMON_LIBS
    openthread-spinel-ncp
    openthread-hdlc
    ot-test-platform-ftd
    openthread-ftd
    ot-test-platform-ftd
    ${OT_MBEDTLS}
    ot-config
    openthread-ftd
    openthread-url
)

set(COMMON_LIBS_RCP
    ot-test-platform-rcp
    openthread-rcp
    ${OT_MBEDTLS}
    ot-config
)

#----------------------------------------------------------------------------------------------------------------------

macro(ot_unit_test name)

    # Macro to add an OpenThread unit test.
    #
    #   Unit test name will be `ot-test-{name}`. Test source file of
    #   `test_{name}.cpp` is used. Optional extra arguments can be
    #   passed to provide additional source files.

    add_executable(ot-test-${name}
        test_${name}.cpp ${ARGN}
    )

    target_include_directories(ot-test-${name}
    PRIVATE
        ${COMMON_INCLUDES}
    )

    target_link_libraries(ot-test-${name}
    PRIVATE
        ${COMMON_LIBS}
    )

    target_compile_options(ot-test-${name}
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
    )

    add_test(NAME ot-test-${name} COMMAND ot-test-${name})
endmacro()

#----------------------------------------------------------------------------------------------------------------------

macro(ot_unit_ncp_test name)

    # Macro to add an OpenThread unit test for NCP functions.
    #
    #   Unit test name will be `ot-test-ncp-{name}`. Test source file of
    #   `test_ncp_{name}.cpp` is used. Optional extra arguments can be
    #   passed to provide additional source files.

    add_executable(ot-test-ncp-${name}
        test_ncp_${name}.cpp ${ARGN}
    )

    target_include_directories(ot-test-ncp-${name}
    PRIVATE
        ${COMMON_INCLUDES}
    )

    target_link_libraries(ot-test-ncp-${name}
    PRIVATE
        openthread-ncp-ftd
        ${COMMON_LIBS}
    )

    target_compile_options(ot-test-ncp-${name}
    PRIVATE
        -DOPENTHREAD_FTD=1
    )

    add_test(NAME ot-test-ncp-${name} COMMAND ot-test-ncp-${name})
endmacro()

#----------------------------------------------------------------------------------------------------------------------
# Unit tests

ot_unit_test(address_sanitizer)
ot_unit_test(aes)
ot_unit_test(array)
ot_unit_test(binary_search)
ot_unit_test(checksum)
ot_unit_test(child)
ot_unit_test(child_table)
ot_unit_test(cmd_line_parser)
ot_unit_test(data)
ot_unit_test(dataset)
ot_unit_test(dns)
ot_unit_test(dns_client)
ot_unit_test(dnssd_discovery_proxy)
ot_unit_test(dso)
ot_unit_test(ecdsa)
ot_unit_test(flash)
ot_unit_test(frame_builder)
ot_unit_test(hdlc)
ot_unit_test(heap)
ot_unit_test(heap_array)
ot_unit_test(heap_string)
ot_unit_test(hkdf_sha256)
ot_unit_test(hmac_sha256)
ot_unit_test(ip4_header)
ot_unit_test(ip6_header)
ot_unit_test(ip_address)
ot_unit_test(link_metrics_manager)
ot_unit_test(link_quality)
ot_unit_test(linked_list)
ot_unit_test(lowpan)
ot_unit_test(mac_frame)
ot_unit_test(macros)
ot_unit_test(mdns)
ot_unit_test(meshcop)
ot_unit_test(message)
ot_unit_test(message_queue)
ot_unit_test(mle)
ot_unit_test(multicast_listeners_table)
ot_unit_test(nat64)
ot_unit_test(ndproxy_table)
ot_unit_test(netif)
ot_unit_test(network_data)
ot_unit_test(network_name)
ot_unit_test(offset_range)
ot_unit_test(pool)
ot_unit_test(power_calibration)
ot_unit_test(priority_queue)
ot_unit_test(pskc)
ot_unit_test(routing_manager)
ot_unit_test(serial_number)
ot_unit_test(smart_ptrs)
ot_unit_test(spinel_buffer)
ot_unit_test(spinel_decoder)
ot_unit_test(spinel_encoder)
ot_unit_test(srp_adv_proxy)
ot_unit_test(srp_server)
ot_unit_test(string)
ot_unit_test(tcat)
ot_unit_test(timer)
ot_unit_test(tlv)
ot_unit_test(toolchain test_toolchain_c.c)
ot_unit_test(trickle_timer)
ot_unit_test(url)

ot_unit_ncp_test(infra_if)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if(OT_MULTIPAN_RCP)
    add_executable(ot-test-multipan-rcp-instances
        test_multipan_rcp_instances.cpp
    )

    target_include_directories(ot-test-multipan-rcp-instances
        PRIVATE
            ${COMMON_INCLUDES_RCP}
    )

    target_compile_options(ot-test-multipan-rcp-instances
        PRIVATE
            ${COMMON_COMPILE_OPTIONS_RCP}
            ${MULTIPAN_RCP_COMPILE_OPTIONS}
    )

    target_compile_definitions(ot-test-multipan-rcp-instances
        PRIVATE
            "OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1"
    )

    target_compile_options(ot-config-radio
        INTERFACE
            "-DOPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE=0" # used to skip backoff and request tx from platform directly.
    )

    target_link_libraries(ot-test-multipan-rcp-instances
        PRIVATE
            ${COMMON_LIBS_RCP}
    )

    add_test(NAME ot-test-multipan-rcp-instances COMMAND ot-test-multipan-rcp-instances)
 endif()
