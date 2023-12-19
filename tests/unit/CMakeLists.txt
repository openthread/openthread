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
)

set(COMMON_LIBS_RCP
    ot-test-platform-rcp
    openthread-rcp
    ${OT_MBEDTLS}
    ot-config
)

add_executable(ot-test-aes
    test_aes.cpp
)

target_include_directories(ot-test-aes
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-aes
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-aes
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-aes COMMAND ot-test-aes)

add_executable(ot-test-array
    test_array.cpp
)

target_include_directories(ot-test-array
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-array
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-array
    PRIVATE
        ${COMMON_LIBS}
)

add_executable(ot-test-binary-search
    test_binary_search.cpp
)

target_include_directories(ot-test-binary-search
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-binary-search
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-binary-search
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-binary-search COMMAND ot-test-binary-search)

add_executable(ot-test-child
    test_child.cpp
)

add_executable(ot-test-checksum
    test_checksum.cpp
)

target_include_directories(ot-test-checksum
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-checksum
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-checksum
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-checksum COMMAND ot-test-checksum)


target_include_directories(ot-test-child
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-child
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-child
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-child COMMAND ot-test-child)

add_executable(ot-test-child-table
    test_child_table.cpp
)

target_include_directories(ot-test-child-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-child-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-child-table
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-child-table COMMAND ot-test-child-table)

add_executable(ot-test-cmd-line-parser
    test_cmd_line_parser.cpp
)

target_include_directories(ot-test-cmd-line-parser
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-cmd-line-parser
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-cmd-line-parser
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-cmd-line-parser COMMAND ot-test-cmd-line-parser)

add_executable(ot-test-data
    test_data.cpp
)

target_include_directories(ot-test-data
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-data
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-data
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-data COMMAND ot-test-data)

add_executable(ot-test-dns
    test_dns.cpp
)

target_include_directories(ot-test-dns
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-dns
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-dns
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-dns COMMAND ot-test-dns)

add_executable(ot-test-dns-client
    test_dns_client.cpp
)

target_include_directories(ot-test-dns-client
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-dns-client
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-dns-client
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-dns-client COMMAND ot-test-dns-client)

add_executable(ot-test-dso
    test_dso.cpp
)

target_include_directories(ot-test-dso
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-dso
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-dso
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-dso COMMAND ot-test-dso)


add_executable(ot-test-ecdsa
    test_ecdsa.cpp
)

target_include_directories(ot-test-ecdsa
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-ecdsa
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-ecdsa
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-ecdsa COMMAND ot-test-ecdsa)


add_executable(ot-test-flash
    test_flash.cpp
)

target_include_directories(ot-test-flash
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-flash
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-flash
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-flash COMMAND ot-test-flash)

add_executable(ot-test-frame-builder
    test_frame_builder.cpp
)

target_include_directories(ot-test-frame-builder
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-frame-builder
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-frame-builder
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-frame_builder COMMAND ot-test-frame-builder)

add_executable(ot-test-heap
    test_heap.cpp
)

target_include_directories(ot-test-heap
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-heap
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-heap
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-heap COMMAND ot-test-heap)

add_executable(ot-test-heap-array
    test_heap_array.cpp
)

target_include_directories(ot-test-heap-array
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-heap-array
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-heap-array
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-heap-array COMMAND ot-test-heap-array)

add_executable(ot-test-heap-string
    test_heap_string.cpp
)

target_include_directories(ot-test-heap-string
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-heap-string
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-heap-string
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-heap-string COMMAND ot-test-heap-string)

add_executable(ot-test-hkdf-sha256
    ${COMMON_SOURCES}
    test_hkdf_sha256.cpp
)

target_include_directories(ot-test-hkdf-sha256
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_definitions(ot-test-hkdf-sha256
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)

target_compile_options(ot-test-hkdf-sha256
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-hkdf-sha256
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-hkdf-sha256 COMMAND ot-test-hkdf-sha256)

add_executable(ot-test-hmac-sha256
    test_hmac_sha256.cpp
)

target_include_directories(ot-test-hmac-sha256
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-hmac-sha256
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-hmac-sha256
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-hmac-sha256 COMMAND ot-test-hmac-sha256)

add_executable(ot-test-ip4-header
    test_ip4_header.cpp
)

target_include_directories(ot-test-ip4-header
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-ip4-header
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-ip4-header
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-ip4-header COMMAND ot-test-ip4-header)

add_executable(ot-test-ip6-header
    test_ip6_header.cpp
)

target_include_directories(ot-test-ip6-header
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-ip6-header
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-ip6-header
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-ip6-header COMMAND ot-test-ip6-header)

add_executable(ot-test-ip-address
    test_ip_address.cpp
)

target_include_directories(ot-test-ip-address
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-ip-address
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-ip-address
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-ip-address COMMAND ot-test-ip-address)

add_executable(ot-test-link-metrics-manager
    test_link_metrics_manager.cpp
)

target_include_directories(ot-test-link-metrics-manager
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-link-metrics-manager
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-link-metrics-manager
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-link-metrics-manager COMMAND ot-test-link-metrics-manager)

add_executable(ot-test-link-quality
    test_link_quality.cpp
)

target_include_directories(ot-test-link-quality
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-link-quality
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-link-quality
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-link-quality COMMAND ot-test-link-quality)

add_executable(ot-test-linked-list
    test_linked_list.cpp
)

target_include_directories(ot-test-linked-list
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-linked-list
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-linked-list
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-linked-list COMMAND ot-test-linked-list)

add_executable(ot-test-lowpan
    test_lowpan.cpp
)

target_include_directories(ot-test-lowpan
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-lowpan
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-lowpan
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-lowpan COMMAND ot-test-lowpan)

add_executable(ot-test-mac-frame
    test_mac_frame.cpp
)

target_include_directories(ot-test-mac-frame
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-mac-frame
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-mac-frame
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-mac-frame COMMAND ot-test-mac-frame)

add_executable(ot-test-macros
    test_macros.cpp
)

target_include_directories(ot-test-macros
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-macros
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-macros
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-macros COMMAND ot-test-macros)

add_executable(ot-test-message
    test_message.cpp
)

target_include_directories(ot-test-message
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-message
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-message
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-message COMMAND ot-test-message)

add_executable(ot-test-message-queue
    test_message_queue.cpp
)

target_include_directories(ot-test-message-queue
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-message-queue
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-message-queue
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-message-queue COMMAND ot-test-message-queue)

add_executable(ot-test-mle
    test_mle.cpp
)

target_include_directories(ot-test-mle
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-mle
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-mle
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-mle COMMAND ot-test-mle)

add_executable(ot-test-multicast-listeners-table
    test_multicast_listeners_table.cpp
)

target_include_directories(ot-test-multicast-listeners-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-multicast-listeners-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-multicast-listeners-table
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-multicast-listeners-table COMMAND ot-test-multicast-listeners-table)

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

add_test(NAME ot-test-nat64 COMMAND ot-test-nat64)

add_executable(ot-test-nat64
    test_nat64.cpp
)

target_include_directories(ot-test-nat64
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-nat64
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-nat64
    PRIVATE
        ${COMMON_LIBS}
)

add_executable(ot-test-ndproxy-table
    test_ndproxy_table.cpp
)

add_executable(ot-test-network-name
    test_network_name.cpp
)

target_include_directories(ot-test-network-name
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-network-name
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-network-name
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-network-name COMMAND ot-test-network-name)

target_include_directories(ot-test-ndproxy-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-ndproxy-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)


target_link_libraries(ot-test-ndproxy-table
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-ndproxy-table COMMAND ot-test-ndproxy-table)


add_executable(ot-test-netif
    test_netif.cpp
)

target_include_directories(ot-test-netif
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-netif
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-netif
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-netif COMMAND ot-test-netif)

add_executable(ot-test-network-data
    test_network_data.cpp
)

target_include_directories(ot-test-network-data
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-network-data
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-network-data
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-network-data COMMAND ot-test-network-data)

add_executable(ot-test-pool
    test_pool.cpp
)

target_include_directories(ot-test-pool
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-pool
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-pool
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-pool COMMAND ot-test-pool)

add_executable(ot-test-power-calibration
    test_power_calibration.cpp
)

target_include_directories(ot-test-power-calibration
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-power-calibration
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-power-calibration
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-power-calibration COMMAND ot-test-power-calibration)

add_executable(ot-test-priority-queue
    test_priority_queue.cpp
)

target_include_directories(ot-test-priority-queue
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-priority-queue
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-priority-queue
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-priority-queue COMMAND ot-test-priority-queue)

add_executable(ot-test-pskc
    test_pskc.cpp
)

target_include_directories(ot-test-pskc
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-pskc
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-pskc
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-pskc COMMAND ot-test-pskc)

add_executable(ot-test-smart-ptrs
    test_smart_ptrs.cpp
)

target_include_directories(ot-test-smart-ptrs
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-smart-ptrs
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-smart-ptrs
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-smart-ptrs COMMAND ot-test-smart-ptrs)

add_executable(ot-test-meshcop
    test_meshcop.cpp
)

target_include_directories(ot-test-meshcop
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-meshcop
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-meshcop
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-meshcop COMMAND ot-test-meshcop)

add_executable(ot-test-serial-number
    test_serial_number.cpp
)

add_executable(ot-test-routing-manager
    test_routing_manager.cpp
)

target_include_directories(ot-test-routing-manager
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-routing-manager
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-routing-manager
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-routing-manager COMMAND ot-test-routing-manager)

target_include_directories(ot-test-serial-number
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-serial-number
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-serial-number
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-serial-number COMMAND ot-test-serial-number)

add_executable(ot-test-srp-server
    test_srp_server.cpp
)

target_include_directories(ot-test-srp-server
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-srp-server
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-srp-server
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-srp-server COMMAND ot-test-srp-server)


add_executable(ot-test-string
    test_string.cpp
)

target_include_directories(ot-test-string
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-string
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-string
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-string COMMAND ot-test-string)

add_executable(ot-test-timer
    test_timer.cpp
)

add_executable(ot-test-toolchain
    test_toolchain.cpp test_toolchain_c.c
)

target_include_directories(ot-test-toolchain
    PRIVATE
        ${COMMON_INCLUDES}
)

target_link_libraries(ot-test-toolchain
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-toolchain COMMAND ot-test-toolchain)

target_include_directories(ot-test-timer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-timer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-timer
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-timer COMMAND ot-test-timer)

add_executable(ot-test-trickle-timer
    test_trickle_timer.cpp
)

target_include_directories(ot-test-trickle-timer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-trickle-timer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-trickle-timer
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-trickle-timer COMMAND ot-test-trickle-timer)

add_executable(ot-test-tlv
    test_tlv.cpp
)

target_include_directories(ot-test-tlv
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-test-tlv
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(ot-test-tlv
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME ot-test-tlv COMMAND ot-test-tlv)

add_executable(ot-test-hdlc
    test_hdlc.cpp
)
target_include_directories(ot-test-hdlc
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-hdlc
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-hdlc
    PRIVATE
        ${COMMON_LIBS}
)
add_test(NAME ot-test-hdlc COMMAND ot-test-hdlc)

add_executable(ot-test-spinel-buffer
    test_spinel_buffer.cpp
)
target_include_directories(ot-test-spinel-buffer
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-spinel-buffer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-spinel-buffer
    PRIVATE
        ${COMMON_LIBS}
)
add_test(NAME ot-test-spinel-buffer COMMAND ot-test-spinel-buffer)

add_executable(ot-test-spinel-decoder
    test_spinel_decoder.cpp
)
target_include_directories(ot-test-spinel-decoder
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-spinel-decoder
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-spinel-decoder
    PRIVATE
        ${COMMON_LIBS}
)
add_test(NAME ot-test-spinel-decoder COMMAND ot-test-spinel-decoder)

add_executable(ot-test-spinel-encoder
    test_spinel_encoder.cpp
)
target_include_directories(ot-test-spinel-encoder
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-spinel-encoder
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-spinel-encoder
    PRIVATE
        ${COMMON_LIBS}
)
add_test(NAME ot-test-spinel-encoder COMMAND ot-test-spinel-encoder)

add_executable(ot-test-address-sanitizer
    test_address_sanitizer.cpp
)
target_include_directories(ot-test-address-sanitizer
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-address-sanitizer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-address-sanitizer
    PRIVATE
        ${COMMON_LIBS}
)
add_test(NAME ot-test-address-sanitizer COMMAND ot-test-address-sanitizer)

add_executable(ot-test-url
    test_url.cpp
)
target_include_directories(ot-test-url
    PRIVATE
        ${COMMON_INCLUDES}
)
target_compile_options(ot-test-url
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)
target_link_libraries(ot-test-url
    PRIVATE
        ${COMMON_LIBS}
        openthread-url
)
add_test(NAME ot-test-url COMMAND ot-test-url)
