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


libopenthread_ftd_a_CPPFLAGS        = \
    $(CPPFLAGS_COMMON)                \
    -DOPENTHREAD_FTD=1                \
    $(NULL)

libopenthread_mtd_a_CPPFLAGS        = \
    $(CPPFLAGS_COMMON)                \
    -DOPENTHREAD_MTD=1                \
    $(NULL)

#----------------------------------------
# Note to maintainer/developers about "SOURCES_COMMON"
#
# The traditional method for GNU style Makefile.am files
# is to have IF statements within the Makefile
#
# These IF statements would add (or not add) certain
# source files from the build/library being built.
#
# In general, IDEs do not easily support this.
# Some do, some do not, those that do support it
# often do so in a very different or complex way.
# Bottom line: It is a very mixed bag of results.
#
# Thus we bend to the IDEs, and impose these rules:
#
# Rule #1
#     All code(files) should be compiled
#     Even if it is not used or required
#
#     For example: In the MTD build, some features
#     are disabled and thus in the traditional scheme
#     source files could be excluded SOURCES_COMMON
#     via makefile IF/ELSE/ENDIF constructs.
#
#     This makes it impossibly hard for IDEs.
#     Thus we compile everything.
#
#     And then rely upon Rule #2.
#
# Rule #2
#     Files that are not required, in certain
#     configurations require appropriate wrappers
#     Such as #if/#else/#endif
#
#     The net result is sort of an "empty translation unit"
#

SOURCES_COMMON                      = \
    api/coap_api.cpp                  \
    api/commissioner_api.cpp          \
    api/child_supervision_api.cpp     \
    api/crypto_api.cpp                \
    api/border_router_api.cpp         \
    api/dataset_api.cpp               \
    api/dataset_ftd_api.cpp           \
    api/dhcp6_api.cpp                 \
    api/dhcp6_api.cpp                 \
    api/dns_api.cpp                   \
    api/icmp6_api.cpp                 \
    api/instance_api.cpp              \
    api/ip6_api.cpp                   \
    api/jam_detection_api.cpp         \
    api/joiner_api.cpp                \
    api/link_api.cpp                  \
    api/link_raw_api.cpp              \
    api/message_api.cpp               \
    api/netdata_api.cpp               \
    api/tasklet_api.cpp               \
    api/thread_api.cpp                \
    api/thread_ftd_api.cpp            \
    api/tmf_proxy_api.cpp             \
    api/udp_api.cpp                   \
    coap/coap.cpp                     \
    coap/coap_header.cpp              \
    coap/coap_secure.cpp              \
    common/crc16.cpp                  \
    common/logging.cpp                \
    common/message.cpp                \
    common/tasklet.cpp                \
    common/timer.cpp                  \
    common/tlvs.cpp                   \
    common/trickle_timer.cpp          \
    crypto/aes_ccm.cpp                \
    crypto/aes_ecb.cpp                \
    crypto/hmac_sha256.cpp            \
    crypto/mbedtls.cpp                \
    crypto/pbkdf2_cmac.cpp            \
    crypto/sha256.cpp                 \
    mac/mac.cpp                       \
    mac/mac_blacklist.cpp             \
    mac/mac_frame.cpp                 \
    mac/mac_whitelist.cpp             \
    meshcop/announce_begin_client.cpp \
    meshcop/commissioner.cpp          \
    meshcop/dataset.cpp               \
    meshcop/dataset_manager.cpp       \
    meshcop/dataset_manager_ftd.cpp   \
    meshcop/dtls.cpp                  \
    meshcop/energy_scan_client.cpp    \
    meshcop/joiner.cpp                \
    meshcop/joiner_router.cpp         \
    meshcop/leader.cpp                \
    meshcop/panid_query_client.cpp    \
    meshcop/timestamp.cpp             \
    meshcop/meshcop_tlvs.cpp          \
    net/dhcp6_client.cpp              \
    net/dhcp6_server.cpp              \
    net/dns_client.cpp                \
    net/icmp6.cpp                     \
    net/ip6.cpp                       \
    net/ip6_address.cpp               \
    net/ip6_filter.cpp                \
    net/ip6_mpl.cpp                   \
    net/ip6_routes.cpp                \
    net/netif.cpp                     \
    net/udp6.cpp                      \
    thread/address_resolver.cpp       \
    thread/announce_begin_server.cpp  \
    thread/data_poll_manager.cpp      \
    thread/energy_scan_server.cpp     \
    thread/key_manager.cpp            \
    thread/link_quality.cpp           \
    thread/lowpan.cpp                 \
    thread/mesh_forwarder.cpp         \
    thread/mle.cpp                    \
    thread/mle_router.cpp             \
    thread/network_data.cpp           \
    thread/network_data_leader.cpp    \
    thread/network_data_leader_ftd.cpp \
    thread/network_data_local.cpp     \
    thread/network_diagnostic.cpp     \
    thread/panid_query_server.cpp     \
    thread/src_match_controller.cpp   \
    thread/thread_netif.cpp           \
    thread/tmf_proxy.cpp              \
    thread/topology.cpp               \
    utils/child_supervision.cpp       \
    utils/jam_detector.cpp            \
    utils/missing_strlcpy.c           \
    utils/missing_strlcat.c           \
    utils/missing_strnlen.c           \
    utils/slaac_address.cpp           \
    $(NULL)


libopenthread_mtd_a_SOURCES         = \
    $(SOURCES_COMMON)                 \
    $(NULL)

libopenthread_ftd_a_SOURCES         = \
    $(SOURCES_COMMON)                 \
    $(NULL)

HEADERS_COMMON                      = \
    openthread-core-config.h          \
    openthread-core-default-config.h  \
    openthread-instance.h             \
    api/link_raw.hpp                  \
    coap/coap.hpp                     \
    coap/coap_header.hpp              \
    coap/coap_secure.hpp              \
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
    common/tlvs.hpp                   \
    common/trickle_timer.hpp          \
    crypto/aes_ccm.hpp                \
    crypto/aes_ecb.hpp                \
    crypto/hmac_sha256.hpp            \
    crypto/mbedtls.hpp                \
    crypto/pbkdf2_cmac.h              \
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
    meshcop/leader.hpp                \
    meshcop/meshcop.hpp               \
    meshcop/panid_query_client.hpp    \
    meshcop/timestamp.hpp             \
    meshcop/meshcop_tlvs.hpp          \
    net/dhcp6.hpp                     \
    net/dhcp6_client.hpp              \
    net/dhcp6_server.hpp              \
    net/dns_client.hpp                \
    net/dns_headers.hpp               \
    net/icmp6.hpp                     \
    net/ip6.hpp                       \
    net/ip6_address.hpp               \
    net/ip6_headers.hpp               \
    net/ip6_filter.hpp                \
    net/ip6_mpl.hpp                   \
    net/ip6_routes.hpp                \
    net/netif.hpp                     \
    net/socket.hpp                    \
    net/tcp.hpp                       \
    net/udp6.hpp                      \
    thread/address_resolver.hpp       \
    thread/announce_begin_server.hpp  \
    thread/data_poll_manager.hpp      \
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
    thread/network_data_tlvs.hpp      \
    thread/panid_query_server.hpp     \
    thread/network_diagnostic.hpp     \
    thread/network_diagnostic_tlvs.hpp \
    thread/src_match_controller.hpp   \
    thread/thread_netif.hpp           \
    thread/thread_tlvs.hpp            \
    thread/thread_uri_paths.hpp       \
    thread/tmf_proxy.hpp              \
    thread/topology.hpp               \
    utils/child_supervision.hpp       \
    utils/slaac_address.hpp           \
    utils/jam_detector.hpp            \
    utils/wrap_stdbool.h              \
    utils/wrap_stdint.h               \
    utils/wrap_string.h               \
    $(NULL)

noinst_HEADERS                      = \
    $(HEADERS_COMMON)                 \
    $(NULL)

PRETTY_FILES                        = \
    $(SOURCES_COMMON)                 \
    $(HEADERS_COMMON)                 \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
Dash                                = -
CLEANFILES                          = $(shell find $(top_builddir)/src/core $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
