/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the Thread network interface.
 */

#include "thread_netif.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/message.hpp"
#include "instance/instance.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

ThreadNetif::ThreadNetif(Instance &aInstance)
    : Netif(aInstance)
    , mIsUp(false)
{
}

void ThreadNetif::Up(void)
{
    VerifyOrExit(!mIsUp);

    // Enable the MAC just in case it was disabled while the Interface was down.
    Get<Mac::Mac>().SetEnabled(true);
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    IgnoreError(Get<Utils::ChannelMonitor>().Start());
#endif
    Get<MeshForwarder>().Start();

    mIsUp = true;

    SubscribeAllNodesMulticast();
    IgnoreError(Get<Mle::MleRouter>().Enable());
    IgnoreError(Get<Tmf::Agent>().Start());
#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    IgnoreError(Get<Dns::ServiceDiscovery::Server>().Start());
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    IgnoreError(Get<Dns::Client>().Start());
#endif
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    IgnoreError(Get<Sntp::Client>().Start());
#endif
    Get<Notifier>().Signal(kEventThreadNetifStateChanged);

exit:
    return;
}

void ThreadNetif::Down(void)
{
    VerifyOrExit(mIsUp);

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    Get<Dns::Client>().Stop();
#endif
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    IgnoreError(Get<Sntp::Client>().Stop());
#endif
#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    Get<Dns::ServiceDiscovery::Server>().Stop();
#endif
#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
    Get<Tmf::SecureAgent>().Stop();
#endif
    IgnoreError(Get<Tmf::Agent>().Stop());
    IgnoreError(Get<Mle::MleRouter>().Disable());
    RemoveAllExternalUnicastAddresses();
    UnsubscribeAllExternalMulticastAddresses();
    UnsubscribeAllRoutersMulticast();
    UnsubscribeAllNodesMulticast();

    mIsUp = false;
    Get<MeshForwarder>().Stop();
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    IgnoreError(Get<Utils::ChannelMonitor>().Stop());
#endif
    Get<Notifier>().Signal(kEventThreadNetifStateChanged);

exit:
    return;
}

} // namespace ot
