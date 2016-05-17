#
#  Copyright (c) 2016, Nest Labs, Inc.
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

libopenthread_a_CPPFLAGS =            \
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
    crypto/aes_ccm.cpp                \
    mac/mac.cpp                       \
    mac/mac_frame.cpp                 \
    mac/mac_whitelist.cpp             \
    net/icmp6.cpp                     \
    net/ip6.cpp                       \
    net/ip6_address.cpp               \
    net/ip6_mpl.cpp                   \
    net/ip6_routes.cpp                \
    net/netif.cpp                     \
    net/udp6.cpp                      \
    thread/address_resolver.cpp       \
    thread/key_manager.cpp            \
    thread/lowpan.cpp                 \
    thread/mesh_forwarder.cpp         \
    thread/mle.cpp                    \
    thread/mle_router.cpp             \
    thread/mle_tlvs.cpp               \
    thread/network_data.cpp           \
    thread/network_data_local.cpp     \
    thread/network_data_leader.cpp    \
    thread/thread_netif.cpp           \
    thread/thread_tlvs.cpp            \
    $(NULL)

noinst_HEADERS                      = \
    openthread-core-config.h          \
    coap/coap_header.hpp              \
    coap/coap_server.hpp              \
    common/code_utils.hpp             \
    common/debug.hpp                  \
    common/encoding.hpp               \
    common/logging.hpp                \
    common/message.hpp                \
    common/tasklet.hpp                \
    common/timer.hpp                  \
    crypto/aes_ccm.hpp                \
    mac/mac.hpp                       \
    mac/mac_frame.hpp                 \
    mac/mac_whitelist.hpp             \
    net/icmp6.hpp                     \
    net/ip6.hpp                       \
    net/ip6_address.hpp               \
    net/ip6_mpl.hpp                   \
    net/ip6_routes.hpp                \
    net/netif.hpp                     \
    net/socket.hpp                    \
    net/udp6.hpp                      \
    thread/address_resolver.hpp       \
    thread/key_manager.hpp            \
    thread/lowpan.hpp                 \
    thread/mesh_forwarder.hpp         \
    thread/mle.hpp                    \
    thread/mle_constants.hpp          \
    thread/mle_router.hpp             \
    thread/mle_tlvs.hpp               \
    thread/network_data.hpp           \
    thread/network_data_leader.hpp    \
    thread/network_data_local.hpp     \
    thread/network_data_tlvs.hpp      \
    thread/thread_netif.hpp           \
    thread/thread_tlvs.hpp            \
    thread/thread_uris.hpp            \
    thread/topology.hpp               \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                          = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
