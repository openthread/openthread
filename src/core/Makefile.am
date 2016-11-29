#
#  Copyright (c) 2016, The OpenThread Authors.
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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                       = \
    libopenthread-ftd.a               \
    libopenthread-mtd.a               \
    $(NULL)

CPPFLAGS_COMMON                     = \
    -I$(top_srcdir)/include           \
    -I${abs_top_srcdir}/third_party/mbedtls/repo/include \
    $(MBEDTLS_CPPFLAGS)               \
    $(OPENTHREAD_TARGET_DEFINES)      \
    $(NULL)

SOURCES_COMMON                      = \
    openthread.cpp                    \
    coap/coap_base.cpp                \
    coap/coap_client.cpp              \
    coap/coap_header.cpp              \
    coap/coap_server.cpp              \
    common/crc16.cpp                  \
    common/logging.cpp                \
    common/message.cpp                \
    common/tasklet.cpp                \
    common/timer.cpp                  \
    common/trickle_timer.cpp          \
    crypto/aes_ccm.cpp                \
    crypto/aes_ecb.cpp                \
    crypto/hmac_sha256.cpp            \
    crypto/openthread-crypto.cpp      \
    crypto/mbedtls.cpp                \
    crypto/sha256.cpp                 \
    mac/mac.cpp                       \
    mac/mac_frame.cpp                 \
    meshcop/dataset.cpp               \
    meshcop/dataset_manager.cpp       \
    meshcop/tlvs.cpp                  \
    net/icmp6.cpp                     \
    net/ip6.cpp                       \
    net/ip6_address.cpp               \
    net/ip6_filter.cpp                \
    net/ip6_mpl.cpp                   \
    net/ip6_routes.cpp                \
    net/netif.cpp                     \
    net/udp6.cpp                      \
    thread/announce_begin_server.cpp  \
    thread/energy_scan_server.cpp     \
    thread/key_manager.cpp            \
    thread/link_quality.cpp           \
    thread/lowpan.cpp                 \
    thread/mesh_forwarder.cpp         \
    thread/mle.cpp                    \
    thread/mle_tlvs.cpp               \
    thread/network_data.cpp           \
    thread/network_data_leader.cpp    \
    thread/panid_query_server.cpp     \
    thread/network_diag.cpp           \
    thread/network_diag_tlvs.cpp      \
    thread/thread_netif.cpp           \
    thread/thread_tlvs.cpp            \
    utils/slaac_address.cpp           \
    $(NULL)

if OPENTHREAD_ENABLE_MAC_WHITELIST
SOURCES_COMMON                     += \
    mac/mac_blacklist.cpp             \
    mac/mac_whitelist.cpp             \
    $(NULL)
endif  # OPENTHREAD_ENABLE_MAC_WHITELIST

if OPENTHREAD_ENABLE_COMMISSIONER
SOURCES_COMMON                     += \
    coap/secure_coap_server.cpp       \
    meshcop/announce_begin_client.cpp \
    meshcop/commissioner.cpp          \
    meshcop/energy_scan_client.cpp    \
    meshcop/panid_query_client.cpp    \
    $(NULL)
endif  # OPENTHREAD_ENABLE_COMMISSIONER

if OPENTHREAD_ENABLE_JOINER
SOURCES_COMMON                     += \
    coap/secure_coap_client.cpp       \
    meshcop/joiner.cpp                \
    $(NULL)
endif  # OPENTHREAD_ENABLE_JOINER

if OPENTHREAD_ENABLE_DTLS
SOURCES_COMMON                     += \
    meshcop/dtls.cpp                  \
    $(NULL)
endif  # OPENTHREAD_ENABLE_DTLS

if OPENTHREAD_ENABLE_DHCP6_CLIENT
SOURCES_COMMON                     += \
    net/dhcp6_client.cpp              \
    $(NULL)
endif  # OPENTHREAD_ENABLE_DHCP6_CLIENT

if OPENTHREAD_ENABLE_DHCP6_SERVER
SOURCES_COMMON                     += \
    net/dhcp6_server.cpp              \
    $(NULL)
endif  # OPENTHREAD_ENABLE_DHCP6_SERVER

if OPENTHREAD_ENABLE_JAM_DETECTION
SOURCES_COMMON                     += \
    utils/jam_detector.cpp            \
    $(NULL)
endif  # OPENTHREAD_ENABLE_JAM_DETECTION

libopenthread_ftd_a_CPPFLAGS        = \
    $(CPPFLAGS_COMMON)                \
    $(NULL)

libopenthread_ftd_a_SOURCES         = \
    $(SOURCES_COMMON)                 \
    meshcop/dataset_manager_ftd.cpp   \
    meshcop/joiner_router.cpp         \
    meshcop/leader.cpp                \
    thread/address_resolver.cpp       \
    thread/mle_router.cpp             \
    thread/network_data_leader_ftd.cpp \
    thread/network_data_local.cpp     \
    $(NULL)

libopenthread_mtd_a_CPPFLAGS        = \
    $(CPPFLAGS_COMMON)                \
    -DOPENTHREAD_MTD                  \
    $(NULL)

libopenthread_mtd_a_SOURCES         = \
    $(SOURCES_COMMON)                 \
    $(NULL)

noinst_HEADERS                      = \
    openthread-core-config.h          \
    openthread-core-default-config.h  \
    openthread-instance.h             \
    coap/coap_base.hpp                \
    coap/coap_client.hpp              \
    coap/coap_header.hpp              \
    coap/coap_server.hpp              \
    coap/secure_coap_client.hpp       \
    coap/secure_coap_server.hpp       \
    common/code_utils.hpp             \
    common/crc16.hpp                  \
    common/debug.hpp                  \
    common/encoding.hpp               \
    common/logging.hpp                \
    common/message.hpp                \
    common/settings.hpp               \
    common/new.hpp                    \
    common/tasklet.hpp                \
    common/timer.hpp                  \
    common/trickle_timer.hpp          \
    crypto/aes_ccm.hpp                \
    crypto/aes_ecb.hpp                \
    crypto/hmac_sha256.hpp            \
    crypto/mbedtls.hpp                \
    crypto/sha256.hpp                 \
    mac/mac.hpp                       \
    mac/mac_blacklist.hpp             \
    mac/mac_blacklist_impl.hpp        \
    mac/mac_blacklist_stub.hpp        \
    mac/mac_frame.hpp                 \
    mac/mac_whitelist.hpp             \
    mac/mac_whitelist_impl.hpp        \
    mac/mac_whitelist_stub.hpp        \
    meshcop/announce_begin_client.hpp \
    meshcop/commissioner.hpp          \
    meshcop/dataset.hpp               \
    meshcop/dataset_manager.hpp       \
    meshcop/dataset_manager_ftd.hpp   \
    meshcop/dataset_manager_mtd.hpp   \
    meshcop/dtls.hpp                  \
    meshcop/energy_scan_client.hpp    \
    meshcop/joiner.hpp                \
    meshcop/joiner_router.hpp         \
    meshcop/joiner_router_ftd.hpp     \
    meshcop/joiner_router_mtd.hpp     \
    meshcop/leader.hpp                \
    meshcop/leader_ftd.hpp            \
    meshcop/leader_mtd.hpp            \
    meshcop/panid_query_client.hpp    \
    meshcop/tlvs.hpp                  \
    net/icmp6.hpp                     \
    net/ip6.hpp                       \
    net/ip6_address.hpp               \
    net/ip6_headers.hpp               \
    net/ip6_filter.hpp                \
    net/ip6_mpl.hpp                   \
    net/ip6_routes.hpp                \
    net/netif.hpp                     \
    net/socket.hpp                    \
    net/udp6.hpp                      \
    net/tcp.hpp                       \
    net/dhcp6.hpp                     \
    net/dhcp6_client.hpp              \
    net/dhcp6_server.hpp              \
    thread/address_resolver.hpp       \
    thread/address_resolver_ftd.hpp   \
    thread/address_resolver_mtd.hpp   \
    thread/announce_begin_server.hpp  \
    thread/energy_scan_server.hpp     \
    thread/key_manager.hpp            \
    thread/link_quality.hpp           \
    thread/lowpan.hpp                 \
    thread/mesh_forwarder.hpp         \
    thread/mle.hpp                    \
    thread/mle_constants.hpp          \
    thread/mle_router.hpp             \
    thread/mle_router_ftd.hpp         \
    thread/mle_router_mtd.hpp         \
    thread/mle_tlvs.hpp               \
    thread/network_data.hpp           \
    thread/network_data_leader.hpp    \
    thread/network_data_leader_ftd.hpp \
    thread/network_data_leader_mtd.hpp \
    thread/network_data_local.hpp     \
    thread/network_data_local_ftd.hpp \
    thread/network_data_local_mtd.hpp \
    thread/network_data_tlvs.hpp      \
    thread/panid_query_server.hpp     \
    thread/network_diag.hpp           \
    thread/network_diag_tlvs.hpp      \
    thread/thread_netif.hpp           \
    thread/thread_tlvs.hpp            \
    thread/thread_uris.hpp            \
    thread/topology.hpp               \
    utils/slaac_address.hpp           \
    utils/jam_detector.hpp            \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
Dash                                = -
CLEANFILES                          = $(shell find $(top_builddir)/src/core $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
