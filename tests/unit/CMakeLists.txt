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

if(OT_FTD)
    add_library(ot-test-platform-ftd
        test_platform.cpp
        test_util.cpp
    )
    target_include_directories(ot-test-platform-ftd
        PRIVATE
            ${COMMON_INCLUDES}
    )
    target_compile_options(ot-test-platform-ftd
        PRIVATE
            ${COMMON_COMPILE_OPTIONS}
    )
    target_link_libraries(ot-test-platform-ftd
        PRIVATE
            ot-config
            ${OT_MBEDTLS}
    )
endif()

if(OT_RCP)
    add_library(ot-test-platform-rcp
        test_platform.cpp
        test_util.cpp
    )
    target_include_directories(ot-test-platform-rcp
        PRIVATE
            ${COMMON_INCLUDES}
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
    target_link_libraries(ot-test-platform-rcp
        PRIVATE
            ot-config
            ${OT_MBEDTLS}
    )
endif()

set(COMMON_LIBS
    openthread-spinel-ncp
    openthread-hdlc
    openthread-pcapng
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

macro(ot_unit_test name labels)

    # Macro to add an OpenThread unit test.
    #
    #   Unit test name will be `ot-test-{name}`. Test source file of
    #   `test_{name}.cpp` is used. The `labels` argument assigns
    #   categories to the test, allowing specific subsets to be run
    #   using `ctest -L {label}`. Optional extra arguments can be
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

    set_tests_properties(ot-test-${name} PROPERTIES LABELS "${labels}")
endmacro()

#----------------------------------------------------------------------------------------------------------------------

macro(ot_unit_ncp_test name labels)

    # Macro to add an OpenThread unit test for NCP functions.
    #
    #   Unit test name will be `ot-test-ncp-{name}`. Test source file of
    #   `test_ncp_{name}.cpp` is used. The `labels` argument assigns
    #   categories to the test, allowing specific subsets to be run
    #   using `ctest -L {label}`. Optional extra arguments can be
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

    set_tests_properties(ot-test-ncp-${name} PROPERTIES LABELS "${labels}")
endmacro()

#----------------------------------------------------------------------------------------------------------------------
# Unit tests

if(OT_FTD)
    ot_unit_test(address_sanitizer "common;unit")
    ot_unit_test(aes "crypto;unit")
    ot_unit_test(array "common;unit")
    ot_unit_test(binary_search "common;unit")
    ot_unit_test(bit_utils "common;unit")
    ot_unit_test(bit_set "common;unit")
    ot_unit_test(checksum "net;unit")
    ot_unit_test(child "mle;unit")
    ot_unit_test(child_table "mle;unit")
    ot_unit_test(cmd_line_parser "common;unit")
    ot_unit_test(coap_message "coap;unit")
    ot_unit_test(coap_overflow "coap;unit")
    ot_unit_test(crc "common;unit")
    ot_unit_test(data "common;unit")
    ot_unit_test(dataset "meshcop;unit")
    ot_unit_test(dhcp6_pd_client "border_router;net;unit")
    ot_unit_test(dns "dns;net;unit")
    ot_unit_test(dns_client "dns;net;unit")
    ot_unit_test(dnssd_discovery_proxy "dns;border_router;net;unit")
    ot_unit_test(dso "dns;net;unit")
    ot_unit_test(ecdsa "crypto;unit")
    ot_unit_test(flash "common;platform;unit")
    ot_unit_test(frame_builder "common;unit")
    ot_unit_test(hdlc "spinel;unit")
    ot_unit_test(heap "common;unit")
    ot_unit_test(heap_array "common;unit")
    ot_unit_test(heap_string "common;unit")
    ot_unit_test(hkdf_sha256 "crypto;unit")
    ot_unit_test(hmac_sha256 "crypto;unit")
    ot_unit_test(ip4_header "net;unit")
    ot_unit_test(ip6_header "net;unit")
    ot_unit_test(ip_address "net;unit")
    ot_unit_test(link_metrics_manager "mac;mle;unit")
    ot_unit_test(link_quality "mac;unit")
    ot_unit_test(linked_list "common;unit")
    ot_unit_test(lowpan "net;unit")
    ot_unit_test(ltv "common;unit")
    ot_unit_test(mac_filter "mac;unit")
    ot_unit_test(mac_frame "mac;unit")
    ot_unit_test(macros "common;unit")
    ot_unit_test(mdns "dns;net;unit")
    ot_unit_test(meshcop "meshcop;unit")
    ot_unit_test(message "common;unit")
    ot_unit_test(message_queue "common;unit")
    ot_unit_test(mle "mle;unit")
    ot_unit_test(msg_backed_array "common;unit")
    ot_unit_test(multicast_listeners_table "bbr;net;unit")
    ot_unit_test(nat64 "nat64;net;unit")
    ot_unit_test(netif "net;unit")
    ot_unit_test(network_data "netdata;unit")
    ot_unit_test(network_name "mac;meshcop;unit")
    ot_unit_test(offset_range "common;unit")
    ot_unit_test(pcapng "common;unit")
    ot_unit_test(pool "common;unit")
    ot_unit_test(plat_tcp "net;platform;unit")
    ot_unit_test(power_calibration "platform;radio;unit")
    ot_unit_test(priority_queue "common;unit")
    ot_unit_test(pskc "crypto;meshcop;unit")
    ot_unit_test(random "common;crypto;unit")
    ot_unit_test(routing_manager "border_router;net;unit")
    ot_unit_test(seeker "meshcop;unit")
    ot_unit_test(serial_number "common;unit")
    ot_unit_test(smart_ptrs "common;unit")
    ot_unit_test(spinel_buffer "spinel;unit")
    ot_unit_test(spinel_decoder "spinel;unit")
    ot_unit_test(spinel_encoder "spinel;unit")
    ot_unit_test(spinel_prop_codec "spinel;unit")
    ot_unit_test(srp_adv_proxy "srp;border_router;net;unit")
    ot_unit_test(srp_server "srp;net;unit")
    ot_unit_test(string "common;unit")
    ot_unit_test(sub_mac_recv_at "mac;radio;unit")
    ot_unit_test(tasklet "common;unit")
    ot_unit_test(tcat "meshcop;unit")
    ot_unit_test(tcp "net;unit")
    ot_unit_test(thread_direct "thread_direct;unit")
    ot_unit_test(thread_direct_wake_cmd "thread_direct;unit")
    ot_unit_test(timer "common;unit")
    ot_unit_test(tlv "common;unit")
    ot_unit_test(toolchain "common;unit" test_toolchain_c.c)
    ot_unit_test(trickle_timer "common;unit")
    ot_unit_test(url "common;unit")
    ot_unit_test(vendor_oui "common;unit")

    ot_unit_ncp_test(cli "ncp;cli;unit")
    ot_unit_ncp_test(dnssd "ncp;dns;unit")
    ot_unit_ncp_test(infra_if "ncp;border_router;unit")
    ot_unit_ncp_test(srp_server "ncp;srp;unit")
    ot_unit_ncp_test(ephemeral_key "ncp;border_router;meshcop;unit")
endif()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if(OT_RCP AND OT_MULTIPAN_RCP)
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

    set_tests_properties(ot-test-multipan-rcp-instances PROPERTIES LABELS "multipan;rcp;unit")
 endif()
