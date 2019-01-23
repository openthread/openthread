#
#  Copyright (c) 2018, The OpenThread Authors.
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

LOCAL_PATH := $(call my-dir)

OPENTHREAD_DEFAULT_VERSION := $(shell cat $(LOCAL_PATH)/.default-version)
OPENTHREAD_SOURCE_VERSION := $(shell git -C $(LOCAL_PATH) describe --always --match "[0-9].*" 2> /dev/null)

OPENTHREAD_COMMON_FLAGS                                          := \
    -DPACKAGE=\"openthread\"                                        \
    -DPACKAGE_BUGREPORT=\"openthread-devel@googlegroups.com\"       \
    -DPACKAGE_NAME=\"OPENTHREAD\"                                   \
    -DPACKAGE_STRING=\"OPENTHREAD\ $(OPENTHREAD_DEFAULT_VERSION)\"  \
    -DPACKAGE_VERSION=\"$(OPENTHREAD_SOURCE_VERSION)\"              \
    -DPACKAGE_TARNAME=\"openthread\"                                \
    -DVERSION=\"$(OPENTHREAD_DEFAULT_VERSION)\"                     \
    -DPACKAGE_URL=\"http://github.com/openthread/openthread\"       \
    $(NULL)

include $(CLEAR_VARS)

LOCAL_MODULE := spi-hdlc-adapter
LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES := tools/spi-hdlc-adapter/spi-hdlc-adapter.c

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := ot-core
LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES                                         := \
    $(LOCAL_PATH)/include                                   \
    $(LOCAL_PATH)/src                                       \
    $(LOCAL_PATH)/src/cli                                   \
    $(LOCAL_PATH)/src/core                                  \
    $(LOCAL_PATH)/src/ncp                                   \
    $(LOCAL_PATH)/src/posix/platform                        \
    $(LOCAL_PATH)/third_party                               \
    $(LOCAL_PATH)/third_party/mbedtls                       \
    $(LOCAL_PATH)/third_party/mbedtls/repo/include          \
    $(NULL)

LOCAL_CFLAGS                                                                := \
    -D_GNU_SOURCE                                                              \
    -DMBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"                                 \
    -DOPENTHREAD_CONFIG_FILE=\<openthread-config-android.h\>                   \
    $(OPENTHREAD_COMMON_FLAGS)                                                 \
    -DOPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE=1                          \
    -DOPENTHREAD_FTD=1                                                         \
    -DOPENTHREAD_POSIX=1                                                       \
    -DOPENTHREAD_POSIX_APP=1                                                   \
    -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-posix-config.h\"   \
    -DSPINEL_PLATFORM_HEADER=\"spinel_platform.h\"                             \
    -Wno-non-virtual-dtor                                                      \
    $(NULL)

LOCAL_SRC_FILES                                          := \
    src/core/api/border_router_api.cpp                      \
    src/core/api/channel_manager_api.cpp                    \
    src/core/api/channel_monitor_api.cpp                    \
    src/core/api/child_supervision_api.cpp                  \
    src/core/api/coap_api.cpp                               \
    src/core/api/commissioner_api.cpp                       \
    src/core/api/crypto_api.cpp                             \
    src/core/api/dataset_api.cpp                            \
    src/core/api/dataset_ftd_api.cpp                        \
    src/core/api/dns_api.cpp                                \
    src/core/api/icmp6_api.cpp                              \
    src/core/api/instance_api.cpp                           \
    src/core/api/ip6_api.cpp                                \
    src/core/api/jam_detection_api.cpp                      \
    src/core/api/joiner_api.cpp                             \
    src/core/api/link_api.cpp                               \
    src/core/api/link_raw_api.cpp                           \
    src/core/api/logging_api.cpp                            \
    src/core/api/message_api.cpp                            \
    src/core/api/netdata_api.cpp                            \
    src/core/api/server_api.cpp                             \
    src/core/api/tasklet_api.cpp                            \
    src/core/api/thread_api.cpp                             \
    src/core/api/thread_ftd_api.cpp                         \
    src/core/api/udp_api.cpp                                \
    src/core/coap/coap.cpp                                  \
    src/core/coap/coap_message.cpp                          \
    src/core/coap/coap_secure.cpp                           \
    src/core/common/crc16.cpp                               \
    src/core/common/instance.cpp                            \
    src/core/common/logging.cpp                             \
    src/core/common/locator.cpp                             \
    src/core/common/message.cpp                             \
    src/core/common/notifier.cpp                            \
    src/core/common/settings.cpp                            \
    src/core/common/string.cpp                              \
    src/core/common/tasklet.cpp                             \
    src/core/common/timer.cpp                               \
    src/core/common/tlvs.cpp                                \
    src/core/common/trickle_timer.cpp                       \
    src/core/crypto/aes_ccm.cpp                             \
    src/core/crypto/aes_ecb.cpp                             \
    src/core/crypto/hmac_sha256.cpp                         \
    src/core/crypto/mbedtls.cpp                             \
    src/core/crypto/pbkdf2_cmac.cpp                         \
    src/core/crypto/sha256.cpp                              \
    src/core/mac/channel_mask.cpp                           \
    src/core/mac/mac.cpp                                    \
    src/core/mac/mac_filter.cpp                             \
    src/core/mac/mac_frame.cpp                              \
    src/core/mac/sub_mac.cpp                                \
    src/core/mac/sub_mac_callbacks.cpp                      \
    src/core/meshcop/announce_begin_client.cpp              \
    src/core/meshcop/border_agent.cpp                       \
    src/core/meshcop/commissioner.cpp                       \
    src/core/meshcop/dataset.cpp                            \
    src/core/meshcop/dataset_local.cpp                      \
    src/core/meshcop/dataset_manager.cpp                    \
    src/core/meshcop/dataset_manager_ftd.cpp                \
    src/core/meshcop/dtls.cpp                               \
    src/core/meshcop/energy_scan_client.cpp                 \
    src/core/meshcop/joiner.cpp                             \
    src/core/meshcop/joiner_router.cpp                      \
    src/core/meshcop/leader.cpp                             \
    src/core/meshcop/meshcop.cpp                            \
    src/core/meshcop/meshcop_tlvs.cpp                       \
    src/core/meshcop/panid_query_client.cpp                 \
    src/core/meshcop/timestamp.cpp                          \
    src/core/net/dhcp6_client.cpp                           \
    src/core/net/dhcp6_server.cpp                           \
    src/core/net/dns_client.cpp                             \
    src/core/net/icmp6.cpp                                  \
    src/core/net/ip6.cpp                                    \
    src/core/net/ip6_address.cpp                            \
    src/core/net/ip6_filter.cpp                             \
    src/core/net/ip6_headers.cpp                            \
    src/core/net/ip6_mpl.cpp                                \
    src/core/net/ip6_routes.cpp                             \
    src/core/net/netif.cpp                                  \
    src/core/net/udp6.cpp                                   \
    src/core/thread/address_resolver.cpp                    \
    src/core/thread/announce_begin_server.cpp               \
    src/core/thread/announce_sender.cpp                     \
    src/core/thread/child_table.cpp                         \
    src/core/thread/data_poll_manager.cpp                   \
    src/core/thread/energy_scan_server.cpp                  \
    src/core/thread/key_manager.cpp                         \
    src/core/thread/link_quality.cpp                        \
    src/core/thread/lowpan.cpp                              \
    src/core/thread/mesh_forwarder.cpp                      \
    src/core/thread/mesh_forwarder_ftd.cpp                  \
    src/core/thread/mesh_forwarder_mtd.cpp                  \
    src/core/thread/mle.cpp                                 \
    src/core/thread/mle_router.cpp                          \
    src/core/thread/network_data.cpp                        \
    src/core/thread/network_data_leader.cpp                 \
    src/core/thread/network_data_leader_ftd.cpp             \
    src/core/thread/network_data_local.cpp                  \
    src/core/thread/network_diagnostic.cpp                  \
    src/core/thread/panid_query_server.cpp                  \
    src/core/thread/router_table.cpp                        \
    src/core/thread/src_match_controller.cpp                \
    src/core/thread/thread_netif.cpp                        \
    src/core/thread/topology.cpp                            \
    src/core/utils/channel_manager.cpp                      \
    src/core/utils/channel_monitor.cpp                      \
    src/core/utils/child_supervision.cpp                    \
    src/core/utils/heap.cpp                                 \
    src/core/utils/jam_detector.cpp                         \
    src/core/utils/missing_strlcpy.c                        \
    src/core/utils/missing_strlcat.c                        \
    src/core/utils/missing_strnlen.c                        \
    src/core/utils/parse_cmdline.cpp                        \
    src/core/utils/slaac_address.cpp                        \
    src/diag/diag_process.cpp                               \
    src/diag/openthread-diag.cpp                            \
    src/ncp/hdlc.cpp                                        \
    src/ncp/ncp_spi.cpp                                     \
    src/ncp/spinel.c                                        \
    src/ncp/spinel_decoder.cpp                              \
    src/ncp/spinel_encoder.cpp                              \
    src/posix/platform/alarm.c                              \
    src/posix/platform/hdlc_interface.cpp                   \
    src/posix/platform/logging.c                            \
    src/posix/platform/misc.c                               \
    src/posix/platform/radio_spinel.cpp                     \
    src/posix/platform/random.c                             \
    src/posix/platform/settings.cpp                         \
    src/posix/platform/spi-stubs.c                          \
    src/posix/platform/system.c                             \
    src/posix/platform/uart.c                               \
    third_party/mbedtls/repo/library/md.c                   \
    third_party/mbedtls/repo/library/md_wrap.c              \
    third_party/mbedtls/repo/library/memory_buffer_alloc.c  \
    third_party/mbedtls/repo/library/platform.c             \
    third_party/mbedtls/repo/library/platform_util.c        \
    third_party/mbedtls/repo/library/sha256.c               \
    third_party/mbedtls/repo/library/bignum.c               \
    third_party/mbedtls/repo/library/ccm.c                  \
    third_party/mbedtls/repo/library/cipher.c               \
    third_party/mbedtls/repo/library/cipher_wrap.c          \
    third_party/mbedtls/repo/library/cmac.c                 \
    third_party/mbedtls/repo/library/ctr_drbg.c             \
    third_party/mbedtls/repo/library/debug.c                \
    third_party/mbedtls/repo/library/ecjpake.c              \
    third_party/mbedtls/repo/library/ecp_curves.c           \
    third_party/mbedtls/repo/library/entropy.c              \
    third_party/mbedtls/repo/library/entropy_poll.c         \
    third_party/mbedtls/repo/library/ssl_cookie.c           \
    third_party/mbedtls/repo/library/ssl_ciphersuites.c     \
    third_party/mbedtls/repo/library/ssl_cli.c              \
    third_party/mbedtls/repo/library/ssl_srv.c              \
    third_party/mbedtls/repo/library/ssl_ticket.c           \
    third_party/mbedtls/repo/library/ssl_tls.c              \
    third_party/mbedtls/repo/library/aes.c                  \
    third_party/mbedtls/repo/library/ecp.c                  \
    $(NULL)

include $(OT_EXTRA_BUILD_CONFIG)

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := ot-cli
LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES                                         := \
    $(LOCAL_PATH)/include                                   \
    $(LOCAL_PATH)/src                                       \
    $(LOCAL_PATH)/src/cli                                   \
    $(LOCAL_PATH)/src/core                                  \
    $(LOCAL_PATH)/src/posix/platform                        \
    $(LOCAL_PATH)/third_party/mbedtls                       \
    $(LOCAL_PATH)/third_party/mbedtls/repo/include          \
    $(NULL)

LOCAL_CFLAGS                                                                := \
    -D_GNU_SOURCE                                                              \
    -DMBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"                                 \
    -DOPENTHREAD_CONFIG_FILE=\<openthread-config-android.h\>                   \
    $(OPENTHREAD_COMMON_FLAGS)                                                 \
    -DOPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE=1                          \
    -DOPENTHREAD_CONFIG_UART_CLI_RAW=1                                         \
    -DOPENTHREAD_FTD=1                                                         \
    -DOPENTHREAD_POSIX=1                                                       \
    -DOPENTHREAD_POSIX_APP=2                                                   \
    -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-posix-config.h\"   \
    -DSPINEL_PLATFORM_HEADER=\"spinel_platform.h\"                             \
    -Wno-non-virtual-dtor                                                      \
    $(NULL)

LOCAL_LDLIBS                               := \
    -lutil

LOCAL_SRC_FILES                            := \
    src/cli/cli.cpp                           \
    src/cli/cli_coap.cpp                      \
    src/cli/cli_console.cpp                   \
    src/cli/cli_dataset.cpp                   \
    src/cli/cli_server.cpp                    \
    src/cli/cli_uart.cpp                      \
    src/cli/cli_udp.cpp                       \
    src/posix/main.c                          \
    $(NULL)

include $(OT_EXTRA_BUILD_CONFIG)

LOCAL_STATIC_LIBRARIES = ot-core
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := ot-ncp
LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES                                         := \
    $(LOCAL_PATH)/include                                   \
    $(LOCAL_PATH)/src                                       \
    $(LOCAL_PATH)/src/core                                  \
    $(LOCAL_PATH)/src/ncp                                   \
    $(LOCAL_PATH)/src/posix/platform                        \
    $(LOCAL_PATH)/third_party/mbedtls                       \
    $(LOCAL_PATH)/third_party/mbedtls/repo/include          \
    $(NULL)

LOCAL_CFLAGS                                                                := \
    -D_GNU_SOURCE                                                              \
    -DMBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"                                 \
    -DOPENTHREAD_CONFIG_FILE=\<openthread-config-android.h\>                   \
    $(OPENTHREAD_COMMON_FLAGS)                                                 \
    -DOPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE=1                          \
    -DOPENTHREAD_FTD=1                                                         \
    -DOPENTHREAD_POSIX=1                                                       \
    -DOPENTHREAD_POSIX_APP=1                                                   \
    -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-posix-config.h\"   \
    -DSPINEL_PLATFORM_HEADER=\"spinel_platform.h\"                             \
    -Wno-non-virtual-dtor                                                      \
    $(NULL)

LOCAL_LDLIBS                               := \
    -lutil

LOCAL_SRC_FILES                            := \
    src/ncp/changed_props_set.cpp             \
    src/ncp/ncp_base.cpp                      \
    src/ncp/ncp_base_mtd.cpp                  \
    src/ncp/ncp_base_ftd.cpp                  \
    src/ncp/ncp_base_dispatcher.cpp           \
    src/ncp/ncp_buffer.cpp                    \
    src/ncp/ncp_uart.cpp                      \
    src/posix/main.c                          \
    $(NULL)

include $(OT_EXTRA_BUILD_CONFIG)

LOCAL_STATIC_LIBRARIES = ot-core
include $(BUILD_EXECUTABLE)
