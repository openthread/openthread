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

namespace ot {
namespace Nexus {

void Node::Reset(void)
{
    Instance *instance = &GetInstance();
    uint32_t  id       = GetId();

    mRadio.Reset();
    mAlarmMilli.Reset();
    mAlarmMicro.Reset();
    mMdns.Reset();
    mInfraIf.mPendingTxQueue.DequeueAndFreeAll();
    mPendingTasklet = false;

    otIp6SetReceiveCallback(&GetInstance(), HandleIp6Receive, &GetInstance());
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
    uint8_t          mode = 0;

    switch (aJoinMode)
    {
    case kAsFed:
        SuccessOrQuit(Get<Mle::Mle>().SetRouterEligible(false));
        OT_FALL_THROUGH;

    case kAsFtd:
        mode = Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullThreadDevice |
               Mle::DeviceMode::kModeFullNetworkData;
        break;
    case kAsMed:
        mode = Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData;
        break;
    case kAsSedWithFullNetData:
        mode = Mle::DeviceMode::kModeFullNetworkData;
        break;
    case kAsSed:
        break;
    }

    SuccessOrQuit(Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(mode)));

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
                           uint8_t             aHopLimit,
                           const Ip6::Address *aSrcAddress)
{
    Message         *message;
    Ip6::MessageInfo messageInfo;

    message = Get<Ip6::Icmp>().NewMessage();
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->SetLength(aPayloadSize));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetHopLimit(aHopLimit);
    messageInfo.mAllowZeroHopLimit = true;

    if (aSrcAddress != nullptr)
    {
        messageInfo.SetSockAddr(*aSrcAddress);
    }

    Log("Sending Echo Request from Node %lu (%s) to %s (payload-size:%u)", ToUlong(GetId()), GetName(),
        aDestination.ToString().AsCString(), aPayloadSize);

    SuccessOrQuit(Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, aIdentifier));
}

void Node::SetName(const char *aPrefix, uint16_t aIndex) { mName.Clear().Append("%s_%u", aPrefix, aIndex); }

void Node::HandleIp6Receive(otMessage *aMessage, void *aContext)
{
    static_cast<Node *>(aContext)->HandleReceive(aMessage);
}

void Node::HandleReceive(otMessage *aMessage)
{
    uint16_t     length = otMessageGetLength(aMessage);
    uint8_t      buffer[1500];
    Ip6::Header *header;

    VerifyOrExit(length <= sizeof(buffer));
    VerifyOrQuit(otMessageRead(aMessage, 0, buffer, length) == length);

    VerifyOrExit(length >= sizeof(Ip6::Header));
    header = reinterpret_cast<Ip6::Header *>(buffer);

    // Forward packets to InfraIf if they are intended for the backbone.
    // We avoid forwarding link-local and realm-local scope packets.

    VerifyOrExit(mInfraIf.IsInitialized());
    VerifyOrExit(header->GetHopLimit() > 1);

    header->SetHopLimit(header->GetHopLimit() - 1);

    VerifyOrExit(header->GetDestination().GetScope() > Ip6::Address::kRealmLocalScope);

    if (header->GetDestination().IsMulticast())
    {
        VerifyOrExit(Get<BackboneRouter::Local>().IsPrimary());
    }

    VerifyOrExit(Get<NetworkData::Leader>().IsOnMesh(header->GetSource()));

    // Only forward if source is NOT Link-Local and NOT Mesh-Local.
    VerifyOrExit(!header->GetSource().IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(header->GetSource()));

    mInfraIf.SendIp6(header->GetSource(), header->GetDestination(), buffer, length);

exit:
    otMessageFree(aMessage);
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void Node::GetTrelSockAddr(Ip6::SockAddr &aSockAddr) const
{
    aSockAddr.SetAddress(mInfraIf.GetLinkLocalAddress());
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

const Ip6::Address &Node::FindGlobalAddress(void)
{
    const Ip6::Address *matchedAddress = nullptr;

    for (const Ip6::Netif::UnicastAddress &unicastAddress : Get<ThreadNetif>().GetUnicastAddresses())
    {
        const Ip6::Address &address = unicastAddress.GetAddress();

        if (address.GetScope() == Ip6::Address::kGlobalScope && !Get<Mle::Mle>().IsMeshLocalAddress(address))
        {
            matchedAddress = &address;
            break;
        }
    }

    VerifyOrQuit(matchedAddress != nullptr, "no global address found");

    return *matchedAddress;
}

} // namespace Nexus
} // namespace ot
