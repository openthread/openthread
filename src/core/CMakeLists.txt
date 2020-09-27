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

add_library(openthread-ftd)
add_library(openthread-mtd)
add_library(openthread-radio)

set_target_properties(
    openthread-ftd openthread-mtd openthread-radio
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_compile_definitions(openthread-ftd PRIVATE
    OPENTHREAD_FTD=1
)

target_compile_definitions(openthread-mtd PRIVATE
    OPENTHREAD_MTD=1
)

target_compile_definitions(openthread-radio PRIVATE
    OPENTHREAD_RADIO=1
)

target_compile_options(openthread-ftd PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-mtd PRIVATE
    ${OT_CFLAGS}
)

target_compile_options(openthread-radio PRIVATE
    ${OT_CFLAGS}
)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/src/core
)

set(COMMON_SOURCES
    api/backbone_router_api.cpp
    api/backbone_router_ftd_api.cpp
    api/border_router_api.cpp
    api/channel_manager_api.cpp
    api/channel_monitor_api.cpp
    api/child_supervision_api.cpp
    api/coap_api.cpp
    api/coap_secure_api.cpp
    api/commissioner_api.cpp
    api/crypto_api.cpp
    api/dataset_api.cpp
    api/dataset_ftd_api.cpp
    api/diags_api.cpp
    api/dns_api.cpp
    api/entropy_api.cpp
    api/heap_api.cpp
    api/icmp6_api.cpp
    api/instance_api.cpp
    api/ip6_api.cpp
    api/jam_detection_api.cpp
    api/joiner_api.cpp
    api/link_api.cpp
    api/link_metrics_api.cpp
    api/link_raw_api.cpp
    api/logging_api.cpp
    api/message_api.cpp
    api/netdata_api.cpp
    api/netdiag_api.cpp
    api/network_time_api.cpp
    api/random_crypto_api.cpp
    api/random_noncrypto_api.cpp
    api/server_api.cpp
    api/sntp_api.cpp
    api/tasklet_api.cpp
    api/thread_api.cpp
    api/thread_ftd_api.cpp
    api/udp_api.cpp
    backbone_router/backbone_tmf.cpp
    backbone_router/bbr_leader.cpp
    backbone_router/bbr_local.cpp
    backbone_router/bbr_manager.cpp
    backbone_router/multicast_listeners_table.cpp
    backbone_router/ndproxy_table.cpp
    coap/coap.cpp
    coap/coap_message.cpp
    coap/coap_secure.cpp
    common/crc16.cpp
    common/instance.cpp
    common/logging.cpp
    common/message.cpp
    common/notifier.cpp
    common/random_manager.cpp
    common/settings.cpp
    common/string.cpp
    common/tasklet.cpp
    common/time_ticker.cpp
    common/timer.cpp
    common/tlvs.cpp
    common/trickle_timer.cpp
    crypto/aes_ccm.cpp
    crypto/aes_ecb.cpp
    crypto/ecdsa.cpp
    crypto/hmac_sha256.cpp
    crypto/mbedtls.cpp
    crypto/pbkdf2_cmac.cpp
    crypto/sha256.cpp
    diags/factory_diags.cpp
    mac/channel_mask.cpp
    mac/data_poll_handler.cpp
    mac/data_poll_sender.cpp
    mac/link_raw.cpp
    mac/mac.cpp
    mac/mac_filter.cpp
    mac/mac_frame.cpp
    mac/mac_types.cpp
    mac/sub_mac.cpp
    mac/sub_mac_callbacks.cpp
    meshcop/announce_begin_client.cpp
    meshcop/border_agent.cpp
    meshcop/commissioner.cpp
    meshcop/dataset.cpp
    meshcop/dataset_local.cpp
    meshcop/dataset_manager.cpp
    meshcop/dataset_manager_ftd.cpp
    meshcop/dtls.cpp
    meshcop/energy_scan_client.cpp
    meshcop/joiner.cpp
    meshcop/joiner_router.cpp
    meshcop/meshcop.cpp
    meshcop/meshcop_leader.cpp
    meshcop/meshcop_tlvs.cpp
    meshcop/panid_query_client.cpp
    meshcop/timestamp.cpp
    net/checksum.cpp
    net/dhcp6_client.cpp
    net/dhcp6_server.cpp
    net/dns_client.cpp
    net/icmp6.cpp
    net/ip6.cpp
    net/ip6_address.cpp
    net/ip6_filter.cpp
    net/ip6_headers.cpp
    net/ip6_mpl.cpp
    net/netif.cpp
    net/sntp_client.cpp
    net/udp6.cpp
    radio/radio.cpp
    radio/radio_callbacks.cpp
    radio/radio_platform.cpp
    thread/address_resolver.cpp
    thread/announce_begin_server.cpp
    thread/announce_sender.cpp
    thread/child_table.cpp
    thread/csl_tx_scheduler.cpp
    thread/discover_scanner.cpp
    thread/dua_manager.cpp
    thread/energy_scan_server.cpp
    thread/indirect_sender.cpp
    thread/key_manager.cpp
    thread/link_metrics.cpp
    thread/link_quality.cpp
    thread/lowpan.cpp
    thread/mesh_forwarder.cpp
    thread/mesh_forwarder_ftd.cpp
    thread/mesh_forwarder_mtd.cpp
    thread/mle.cpp
    thread/mle_router.cpp
    thread/mle_types.cpp
    thread/mlr_manager.cpp
    thread/neighbor_table.cpp
    thread/network_data.cpp
    thread/network_data_leader.cpp
    thread/network_data_leader_ftd.cpp
    thread/network_data_local.cpp
    thread/network_data_notifier.cpp
    thread/network_diagnostic.cpp
    thread/panid_query_server.cpp
    thread/router_table.cpp
    thread/src_match_controller.cpp
    thread/thread_netif.cpp
    thread/time_sync_service.cpp
    thread/tmf.cpp
    thread/topology.cpp
    thread/uri_paths.cpp
    utils/channel_manager.cpp
    utils/channel_monitor.cpp
    utils/child_supervision.cpp
    utils/flash.cpp
    utils/heap.cpp
    utils/jam_detector.cpp
    utils/otns.cpp
    utils/parse_cmdline.cpp
    utils/slaac_address.cpp
)

target_include_directories(openthread-ftd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-mtd PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})
target_include_directories(openthread-radio PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

set(OT_VENDOR_EXTENSION "" CACHE STRING "specify a C++ source file built as part of OpenThread core library")
if(OT_VENDOR_EXTENSION)
  target_compile_definitions(ot-config INTERFACE "OPENTHREAD_ENABLE_VENDOR_EXTENSION=1")
  list(APPEND COMMON_SOURCES ${OT_VENDOR_EXTENSION})
endif()
target_sources(openthread-ftd PRIVATE ${COMMON_SOURCES})
target_sources(openthread-mtd PRIVATE ${COMMON_SOURCES})

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
    utils/parse_cmdline.cpp
)

if(OT_VENDOR_EXTENSION)
  target_sources(openthread-radio PRIVATE ${OT_VENDOR_EXTENSION})
endif()

target_link_libraries(openthread-ftd
    PRIVATE
        ${OT_MBEDTLS}
        ot-config
)

target_link_libraries(openthread-mtd
    PRIVATE
        ${OT_MBEDTLS}
        ot-config
)

target_link_libraries(openthread-radio
    PRIVATE
        ${OT_MBEDTLS}
        ot-config
)
