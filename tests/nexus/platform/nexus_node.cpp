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

//----------------------------------------------------------------------------------------------------------------------

Node::Node(void)
    : Platform(static_cast<Instance &>(*this))
    , mX(0.0f)
    , mY(0.0f)
    , mLastParentId(0xffff)
{
}

void Node::Reset(void)
{
    Instance *instance = &GetInstance();
    uint32_t  id       = GetId();

    mRadio.Reset();
    mAlarmMilli.Reset();
    mAlarmMicro.Reset();
    mMdns.Reset();
    mUpstreamDns.Reset();
    mInfraIf.mPendingTxQueue.DequeueAndFreeAll();
    mPendingTasklet = false;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    mTrel.Reset();
#endif

    instance->~Instance();

    instance = new (instance) Instance();
    instance->SetId(id);
    instance->AfterInit();

    instance->Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, this);
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

    SuccessOrQuit(Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, aIdentifier));
}

void Node::SetName(const char *aPrefix, uint16_t aIndex) { mName.Clear().Append("%s_%u", aPrefix, aIndex); }

void Node::HandleIp6Receive(otMessage *aMessage, void *aContext)
{
    OwnedPtr<Message> messagePtr(AsCoreTypePtr(aMessage));

    static_cast<Node *>(aContext)->HandleIp6Receive(messagePtr.PassOwnership());
}

void Node::HandleIp6Receive(OwnedPtr<Message> aMessagePtr)
{
    Ip6::Header header;

    VerifyOrExit(aMessagePtr != nullptr);
    SuccessOrExit(header.ParseFrom(*aMessagePtr));

    // Forward packets to InfraIf if they are intended for the backbone.
    // We avoid forwarding link-local and realm-local scope packets.

    VerifyOrExit(mInfraIf.IsInitialized());

    VerifyOrExit(header.GetHopLimit() > 1);
    header.SetHopLimit(header.GetHopLimit() - 1);
    aMessagePtr->Write(0, header);

    VerifyOrExit(header.GetDestination().GetScope() > Ip6::Address::kRealmLocalScope);

    if (header.GetDestination().IsMulticast())
    {
        VerifyOrExit(Get<BackboneRouter::Local>().IsPrimary());
    }

    VerifyOrExit(!header.GetSource().IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(header.GetSource()));
    VerifyOrExit(Get<NetworkData::Leader>().IsOnMesh(header.GetSource()));

    mInfraIf.SendIp6(header, aMessagePtr.PassOwnership());

exit:
    return;
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

bool Node::Matches(const Ip6::Address &aAddress, AddressNetif aNetif) const
{
    bool matches = false;

    switch (aNetif)
    {
    case kThreadNetifAddress:
        matches = Get<ThreadNetif>().HasUnicastAddress(aAddress);
        break;

    case kInfraNetifAddress:
        matches = mInfraIf.HasAddress(aAddress);
        break;

    case kAnyNetifAddress:
        matches = Get<ThreadNetif>().HasUnicastAddress(aAddress) || mInfraIf.HasAddress(aAddress);
        break;
    }

    return matches;
}

const char *Node::GetExtendedRoleString(void) const
{
    const char     *roleStr;
    Mle::DeviceRole role = Get<Mle::Mle>().GetRole();

    switch (role)
    {
    case Mle::kRoleDisabled:
        roleStr = "Disabled";
        break;
    case Mle::kRoleDetached:
        roleStr = "Detached";
        break;
    case Mle::kRoleLeader:
        roleStr = "Leader";
        break;
    case Mle::kRoleRouter:
        roleStr = "Router";
        break;
    case Mle::kRoleChild:
        if (Get<Mle::Mle>().IsFullThreadDevice())
        {
            roleStr = Get<Mle::Mle>().IsRouterRoleAllowed() ? "REED" : "FED";
        }
        else
        {
            roleStr = Get<Mle::Mle>().IsRxOnWhenIdle() ? "MED" : "SED";
        }
        break;
    default:
        roleStr = "Unknown";
        break;
    }

    return roleStr;
}

//----------------------------------------------------------------------------------------------------------------------

void AllowLinkBetween(Node &aFirstNode, Node &aSecondNode)
{
    aFirstNode.AllowList(aSecondNode);
    aSecondNode.AllowList(aFirstNode);
}

void UnallowLinkBetween(Node &aFirstNode, Node &aSecondNode)
{
    aFirstNode.UnallowList(aSecondNode);
    aSecondNode.UnallowList(aFirstNode);
}

} // namespace Nexus
} // namespace ot
