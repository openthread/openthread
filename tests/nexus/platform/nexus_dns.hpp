/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#ifndef OT_NEXUS_PLATFORM_NEXUS_DNS_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_DNS_HPP_

#include <openthread/platform/dns.h>
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/owning_list.hpp"
#include "instance/instance.hpp"
#include "net/dnssd_server.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Nexus {

class Node;

class UpstreamDns : public InstanceLocator
{
public:
    static constexpr uint16_t kDnsPort = 53;

    typedef Dns::ServiceDiscovery::Server::UpstreamQueryTransaction UpstreamQueryTransaction;

    struct PendingQuery : public Heap::Allocatable<PendingQuery>, public LinkedListEntry<PendingQuery>
    {
        bool Matches(const UpstreamQueryTransaction *aTxn) const { return mTxn == aTxn; }
        bool Matches(uint16_t aMessageId, const Ip6::Address &aSrcAddr) const
        {
            return (mMessageId == aMessageId) && (mServerAddress == aSrcAddr);
        }

        UpstreamQueryTransaction *mTxn;
        uint16_t                  mMessageId;
        Ip6::Address              mServerAddress;
        PendingQuery             *mNext;
    };

    explicit UpstreamDns(Instance &aInstance);
    ~UpstreamDns(void) { Reset(); }

    void StartUpstreamQuery(UpstreamQueryTransaction &aTxn, const Message &aQuery);
    void CancelUpstreamQuery(UpstreamQueryTransaction &aTxn);
    bool HandleUpstreamDnsResponse(const Ip6::Address &aSrcAddress, Message &aMessage);
    bool IsUpstreamQueryAvailable(void) const;
    void Reset(void);

private:
    bool     IsGua(const Ip6::Prefix &aPrefix) const;
    uint16_t CountGuaPrefixes(const Ip6::Address &aRouterAddress) const;
    Error    SelectUpstreamDnsServer(Ip6::Address &aSelectedAddress) const;

    OwningList<PendingQuery> mPendingQueries;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_DNS_HPP_
