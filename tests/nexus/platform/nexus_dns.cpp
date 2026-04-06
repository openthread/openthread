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

/**
 * @file
 *   This file implements the platform DNS APIs for Nexus.
 */

#include "nexus_dns.hpp"

#include <openthread/platform/dns.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"
#include "border_router/rx_ra_tracker.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/dns_types.hpp"

namespace ot {
namespace Nexus {

bool UpstreamDns::IsGua(const Ip6::Prefix &aPrefix) const { return !aPrefix.IsLinkLocal() && !aPrefix.IsUniqueLocal(); }

uint16_t UpstreamDns::CountGuaPrefixes(const Ip6::Address &aRouterAddress) const
{
    const BorderRouter::RxRaTracker  &rxRaTracker = Get<BorderRouter::RxRaTracker>();
    BorderRouter::PrefixTableIterator iter;
    BorderRouter::PrefixTableEntry    entry;
    uint16_t                          count = 0;

    rxRaTracker.InitIterator(iter);
    while (rxRaTracker.GetNextPrefixTableEntry(iter, entry) == kErrorNone)
    {
        if (entry.mValidLifetime > 0 && IsGua(AsCoreType(&entry.mPrefix)) &&
            AsCoreType(&entry.mRouter.mAddress) == aRouterAddress)
        {
            count++;
        }
    }

    return count;
}

Error UpstreamDns::SelectUpstreamDnsServer(Ip6::Address &aSelectedAddress) const
{
    const BorderRouter::RxRaTracker  &rxRaTracker = Get<BorderRouter::RxRaTracker>();
    BorderRouter::PrefixTableIterator iter;
    BorderRouter::RdnssAddrEntry      entry;
    bool                              found      = false;
    bool                              bestHasGua = false;

    rxRaTracker.InitIterator(iter);
    while (rxRaTracker.GetNextRdnssAddrEntry(iter, entry) == kErrorNone)
    {
        if (!entry.mRouter.mIsReachable)
        {
            continue;
        }

        bool hasGua = (CountGuaPrefixes(AsCoreType(&entry.mRouter.mAddress)) > 0);

        if (!found || (hasGua && !bestHasGua))
        {
            aSelectedAddress = AsCoreType(&entry.mAddress);
            bestHasGua       = hasGua;
            found            = true;
        }
    }

    return found ? kErrorNone : kErrorNotFound;
}

UpstreamDns::UpstreamDns(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void UpstreamDns::StartUpstreamQuery(UpstreamQueryTransaction &aTxn, const Message &aQuery)
{
    Error           error = kErrorNone;
    Node           &node  = AsNode(&GetInstance());
    Ip6::Address    serverAddress;
    ot::Dns::Header dnsHeader;
    PendingQuery   *pendingQuery;
    Message        *message = nullptr;
    // We use kDnsPort (53) as both source and destination port for upstream queries to simplify interception
    // and response matching in this simulation environment.

    SuccessOrExit(error = SelectUpstreamDnsServer(serverAddress));

    SuccessOrExit(error = aQuery.Read(0, dnsHeader));

    message = node.Get<Ip6::Udp>().CloneMessage(aQuery);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    pendingQuery = PendingQuery::Allocate();
    VerifyOrExit(pendingQuery != nullptr, error = kErrorNoBufs);

    pendingQuery->mTxn           = &aTxn;
    pendingQuery->mMessageId     = dnsHeader.GetMessageId();
    pendingQuery->mServerAddress = serverAddress;
    mPendingQueries.Push(*pendingQuery);

    node.mInfraIf.SendUdp(node.mInfraIf.SelectSourceAddress(serverAddress), serverAddress, kDnsPort, kDnsPort,
                          *message);
    message = nullptr;

exit:
    if (error != kErrorNone)
    {
        if (message != nullptr)
        {
            message->Free();
        }
        otPlatDnsUpstreamQueryDone(&GetInstance(), &aTxn, nullptr);
    }
}

void UpstreamDns::CancelUpstreamQuery(UpstreamQueryTransaction &aTxn)
{
    mPendingQueries.RemoveAndFreeAllMatching(&aTxn);
}

void UpstreamDns::Reset(void) { mPendingQueries.Clear(); }

bool UpstreamDns::IsUpstreamQueryAvailable(void) const
{
    Ip6::Address address;
    return SelectUpstreamDnsServer(address) == kErrorNone;
}

bool UpstreamDns::HandleUpstreamDnsResponse(const Ip6::Address &aSrcAddress, Message &aMessage)
{
    Instance              &instance = GetInstance();
    Node                  &node     = AsNode(&instance);
    ot::Dns::Header        dnsHeader;
    OwnedPtr<PendingQuery> query;
    bool                   handled = false;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), dnsHeader));

    query = mPendingQueries.RemoveMatching(dnsHeader.GetMessageId(), aSrcAddress);

    if (query != nullptr)
    {
        // Found matching query
        // We use kTypeOther as it is appropriate for a payload-only message.
        Message *response = node.Get<MessagePool>().Allocate(Message::kTypeOther);

        if (response != nullptr)
        {
            if (response->AppendBytesFromMessage(aMessage, aMessage.GetOffset(),
                                                 aMessage.GetLength() - aMessage.GetOffset()) != kErrorNone)
            {
                response->Free();
                response = nullptr;
            }
        }

        otPlatDnsUpstreamQueryDone(&instance, query->mTxn, response);
        handled = true;
    }

exit:
    return handled;
}

extern "C" bool otPlatDnsIsUpstreamQueryAvailable(otInstance *aInstance)
{
    return AsNode(aInstance).mUpstreamDns.IsUpstreamQueryAvailable();
}

extern "C" void otPlatDnsStartUpstreamQuery(otInstance             *aInstance,
                                            otPlatDnsUpstreamQuery *aTxn,
                                            const otMessage        *aQuery)
{
    AsNode(aInstance).mUpstreamDns.StartUpstreamQuery(AsCoreType(aTxn), AsCoreType(aQuery));
}

extern "C" void otPlatDnsCancelUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn)
{
    AsNode(aInstance).mUpstreamDns.CancelUpstreamQuery(AsCoreType(aTxn));
}

} // namespace Nexus
} // namespace ot
