/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "nexus_node.hpp"
#include "nexus_utils.hpp"

namespace ot {
namespace Nexus {

void Node::Reset(void)
{
    Instance *instance = &GetInstance();
    uint32_t  id       = GetId();

    mRadio.Reset();
    mAlarm.Reset();
    mMdns.Reset();
    mPendingTasklet = false;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    mTrel.Reset();
#endif

    instance->~Instance();

    instance = new (instance) Instance();
    instance->SetId(id);
    instance->AfterInit();
}

void Node::Form(void)
{
    MeshCoP::Dataset::Info datasetInfo;

    SuccessOrQuit(datasetInfo.GenerateRandom(*this));
    Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

    Get<ThreadNetif>().Up();
    SuccessOrQuit(Get<Mle::Mle>().Start());
}

void Node::Join(Node &aNode, JoinMode aJoinMode)
{
    MeshCoP::Dataset dataset;
    Mle::DeviceMode  mode(0);

    switch (aJoinMode)
    {
    case kAsFed:
        SuccessOrQuit(Get<Mle::Mle>().SetRouterEligible(false));
        OT_FALL_THROUGH;

    case kAsFtd:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullThreadDevice |
                 Mle::DeviceMode::kModeFullNetworkData);
        break;
    case kAsMed:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);
        break;
    case kAsSed:
        mode.Set(Mle::DeviceMode::kModeFullNetworkData);
        break;
    }

    SuccessOrQuit(Get<Mle::Mle>().SetDeviceMode(mode));

    SuccessOrQuit(aNode.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
    Get<MeshCoP::ActiveDatasetManager>().SaveLocal(dataset);

    Get<ThreadNetif>().Up();
    SuccessOrQuit(Get<Mle::Mle>().Start());
}

void Node::AllowList(Node &aNode)
{
    SuccessOrQuit(Get<Mac::Filter>().AddAddress(aNode.Get<Mac::Mac>().GetExtAddress()));
    Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
}

void Node::UnallowList(Node &aNode) { Get<Mac::Filter>().RemoveAddress(aNode.Get<Mac::Mac>().GetExtAddress()); }

void Node::SendEchoRequest(const Ip6::Address &aDestination,
                           uint16_t            aIdentifier,
                           uint16_t            aPayloadSize,
                           uint8_t             aHopLimit)
{
    Message         *message;
    Ip6::MessageInfo messageInfo;

    message = Get<Ip6::Icmp>().NewMessage();
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->SetLength(aPayloadSize));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetHopLimit(aHopLimit);

    Log("Sending Echo Request from Node %lu (%s) to %s (payload-size:%u)", ToUlong(GetId()), GetName(),
        aDestination.ToString().AsCString(), aPayloadSize);

    SuccessOrQuit(Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, aIdentifier));
}

void Node::SetName(const char *aPrefix, uint16_t aIndex) { mName.Clear().Append("%s %u", aPrefix, aIndex); }

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void Node::GetTrelSockAddr(Ip6::SockAddr &aSockAddr) const
{
    aSockAddr.SetAddress(mMdns.mIfAddresses[0]);
    aSockAddr.SetPort(mTrel.mUdpPort);
}
#endif

const Ip6::Address &Node::FindMatchingAddress(const char *aPrefix)
{
    Ip6::Prefix         prefix;
    const Ip6::Address *matchedAddress = nullptr;

    SuccessOrQuit(prefix.FromString(aPrefix));

    for (const Ip6::Netif::UnicastAddress &unicastAddress : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (unicastAddress.GetAddress().MatchesPrefix(prefix))
        {
            matchedAddress = &unicastAddress.GetAddress();
            break;
        }
    }

    VerifyOrQuit(matchedAddress != nullptr, "no matching address found");

    return *matchedAddress;
}

} // namespace Nexus
} // namespace ot
