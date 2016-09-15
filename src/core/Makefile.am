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

lib_LIBRARIES                       = libopenthread.a

libopenthread_a_CPPFLAGS            = \
    -I$(top_srcdir)/include           \
    $(OPENTHREAD_TARGET_DEFINES)      \
    $(NULL)

libopenthread_a_SOURCES             = \
    openthread.cpp                    \
    coap/coap_header.cpp              \
    coap/coap_server.cpp              \
    common/logging.cpp                \
    common/message.cpp                \
    common/tasklet.cpp                \
    common/timer.cpp                  \
    common/trickle_timer.cpp          \
    crypto/aes_ccm.cpp                \
    crypto/aes_ecb.cpp                \
    crypto/hmac_sha256.cpp            \
    crypto/mbedtls.cpp                \
    crypto/sha256.cpp                 \
    mac/mac.cpp                       \
    mac/mac_frame.cpp                 \
    mac/mac_whitelist.cpp             \
    mac/mac_blacklist.cpp             \
    meshcop/joiner_router.cpp         \
    meshcop/leader.cpp                \
    net/icmp6.cpp                     \
    net/ip6.cpp                       \
    net/ip6_address.cpp               \
    net/ip6_filter.cpp                \
    net/ip6_mpl.cpp                   \
    net/ip6_routes.cpp                \
    net/netif.cpp                     \
    net/udp6.cpp                      \
    thread/address_resolver.cpp       \
    thread/key_manager.cpp            \
    thread/link_quality.cpp           \
    thread/lowpan.cpp                 \
    thread/meshcop_dataset.cpp        \
    thread/meshcop_dataset_manager.cpp \
    thread/meshcop_tlvs.cpp           \
    thread/mesh_forwarder.cpp         \
    thread/mle.cpp                    \
    thread/mle_router.cpp             \
    thread/mle_tlvs.cpp               \
    thread/network_data.cpp           \
    thread/network_data_local.cpp     \
    thread/network_data_leader.cpp    \
    thread/panid_query_server.cpp     \
    thread/thread_netif.cpp           \
    thread/thread_tlvs.cpp            \
    $(NULL)

if OPENTHREAD_ENABLE_COMMISSIONER
libopenthread_a_SOURCES            += \
    meshcop/commissioner.cpp          \
    meshcop/panid_query_client.cpp    \
    $(NULL)
endif  # OPENTHREAD_ENABLE_COMMISSIONER

if OPENTHREAD_ENABLE_JOINER
libopenthread_a_SOURCES            += \
    meshcop/joiner.cpp                \
    $(NULL)
endif  # OPENTHREAD_ENABLE_JOINER

if OPENTHREAD_ENABLE_DTLS
libopenthread_a_SOURCES            += \
    meshcop/dtls.cpp                  \
    $(NULL)
endif  # OPENTHREAD_ENABLE_DTLS

noinst_HEADERS                      = \
    openthread-core-config.h          \
    openthread-core-default-config.h  \
    coap/coap_header.hpp              \
    coap/coap_server.hpp              \
    common/code_utils.hpp             \
    common/debug.hpp                  \
    common/encoding.hpp               \
    common/logging.hpp                \
    common/message.hpp                \
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
    mac/mac_frame.hpp                 \
    mac/mac_whitelist.hpp             \
    mac/mac_blacklist.hpp             \
    meshcop/commissioner.hpp          \
    meshcop/dtls.hpp                  \
    meshcop/joiner.hpp                \
    meshcop/joiner_router.hpp         \
    meshcop/leader.hpp                \
    meshcop/panid_query_client.hpp    \
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
    thread/address_resolver.hpp       \
    thread/key_manager.hpp            \
    thread/link_quality.hpp           \
    thread/lowpan.hpp                 \
    thread/meshcop_tlvs.hpp           \
    thread/mesh_forwarder.hpp         \
    thread/meshcop_dataset.hpp        \
    thread/meshcop_dataset_manager.hpp \
    thread/meshcop_tlvs.hpp           \
    thread/mle.hpp                    \
    thread/mle_constants.hpp          \
    thread/mle_router.hpp             \
    thread/mle_tlvs.hpp               \
    thread/network_data.hpp           \
    thread/network_data_leader.hpp    \
    thread/network_data_local.hpp     \
    thread/network_data_tlvs.hpp      \
    thread/panid_query_server.hpp     \
    thread/thread_netif.hpp           \
    thread/thread_tlvs.hpp            \
    thread/thread_uris.hpp            \
    thread/topology.hpp               \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
Dash                                = -
CLEANFILES                          = $(shell find $(top_builddir)/src/core $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
