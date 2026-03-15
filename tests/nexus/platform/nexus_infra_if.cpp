/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "nexus_infra_if.hpp"

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

InfraIf::InfraIf(void)
    : mNode(nullptr)
    , mNodeId(0)
    , mIfIndex(0)
{
}

void InfraIf::Init(Node &aNode)
{
    Ip6::Address                  address;
    otPlatInfraIfLinkLayerAddress mac;
    Ip6::InterfaceIdentifier      iid;

    mIfIndex = 1;
    mNode    = &aNode;
    mNodeId  = aNode.GetId();

    GetLinkLayerAddress(mac);

    iid.mFields.m8[0] = mac.mAddress[0] ^ 0x02;
    iid.mFields.m8[1] = mac.mAddress[1];
    iid.mFields.m8[2] = mac.mAddress[2];
    iid.mFields.m8[3] = 0xff;
    iid.mFields.m8[4] = 0xfe;
    iid.mFields.m8[5] = mac.mAddress[3];
    iid.mFields.m8[6] = mac.mAddress[4];
    iid.mFields.m8[7] = mac.mAddress[5];

    address.SetToLinkLocalAddress(iid);

    AddAddress(address);
}

bool InfraIf::HasAddress(const Ip6::Address &aAddress) const { return mAddresses.Contains(aAddress); }

void InfraIf::AddAddress(const Ip6::Address &aAddress)
{
    VerifyOrExit(!HasAddress(aAddress));

    SuccessOrQuit(mAddresses.PushBack(aAddress));

exit:
    return;
}

void InfraIf::RemoveAddress(const Ip6::Address &aAddress)
{
    for (uint16_t index = 0; index < mAddresses.GetLength(); index++)
    {
        if (mAddresses[index] == aAddress)
        {
            mAddresses[index] = *mAddresses.Back();
            mAddresses.PopBack();
            break;
        }
    }
}

const Ip6::Address *InfraIf::FindAddress(const char *aPrefix) const
{
    Ip6::Prefix         prefix;
    const Ip6::Address *matchedAddress = nullptr;

    SuccessOrQuit(prefix.FromString(aPrefix));

    for (const Ip6::Address &address : mAddresses)
    {
        if (address.MatchesPrefix(prefix))
        {
            matchedAddress = &address;
            break;
        }
    }

    return matchedAddress;
}

const Ip6::Address &InfraIf::FindMatchingAddress(const char *aPrefix) const
{
    const Ip6::Address *matchedAddress = FindAddress(aPrefix);

    VerifyOrQuit(matchedAddress != nullptr, "no matching address found on infrastructure interface");

    return *matchedAddress;
}

void InfraIf::SendIcmp6Nd(const Ip6::Address &aDestAddress, const uint8_t *aBuffer, uint16_t aBufferLength)
{
    Message    *message = GetNode().Get<MessagePool>().Allocate(Message::kTypeIp6);
    Ip6::Header ip6Header;

    VerifyOrQuit(message != nullptr);

    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(aBufferLength);
    ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
    ip6Header.SetHopLimit(255);
    ip6Header.SetSource(GetLinkLocalAddress());
    ip6Header.SetDestination(aDestAddress);

    SuccessOrQuit(message->Append(ip6Header));
    SuccessOrQuit(message->AppendBytes(aBuffer, aBufferLength));

    // Calculate ICMPv6 checksum
    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, ip6Header.GetSource(), ip6Header.GetDestination(), Ip6::kProtoIcmp6);

    mPendingTxQueue.Enqueue(*message);
}

void InfraIf::ProcessIcmp6Nd(const Ip6::Address &aSrcAddress, const uint8_t *aBuffer, uint16_t aBufferLength)
{
    Ip6::Nd::Icmp6Packet packet;

    OT_UNUSED_VARIABLE(aSrcAddress);

    packet.Init(aBuffer, aBufferLength);

    VerifyOrExit(packet.GetLength() >= sizeof(Ip6::Icmp::Header));

    switch (reinterpret_cast<const Ip6::Icmp::Header *>(packet.GetBytes())->GetType())
    {
    case Ip6::Icmp::Header::kTypeRouterAdvert:
    {
        Ip6::Nd::RouterAdvert::RxMessage raMessage(packet);

        VerifyOrExit(raMessage.IsValid());

        for (const Ip6::Nd::Option &option : raMessage)
        {
            if (option.GetType() == Ip6::Nd::Option::kTypePrefixInfo)
            {
                HandlePrefixInfoOption(static_cast<const Ip6::Nd::PrefixInfoOption &>(option));
            }
        }
        break;
    }

    case Ip6::Icmp::Header::kTypeRouterSolicit:
    case Ip6::Icmp::Header::kTypeNeighborAdvert:
    case Ip6::Icmp::Header::kTypeNeighborSolicit:
        // TODO: Handle other ND messages as needed for the simulation.
        break;

    default:
        break;
    }

exit:
    return;
}

void InfraIf::HandlePrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio)
{
    Ip6::Prefix                   prefix;
    Ip6::Address                  address;
    otPlatInfraIfLinkLayerAddress mac;

    VerifyOrExit(aPio.IsAutoAddrConfigFlagSet());

    aPio.GetPrefix(prefix);

    // Generate address using SLAAC (EUI-64 style for simplicity in test)

    address = AsCoreType(&prefix.mPrefix);
    GetLinkLayerAddress(mac);

    address.mFields.m8[8]  = mac.mAddress[0] ^ 0x02;
    address.mFields.m8[9]  = mac.mAddress[1];
    address.mFields.m8[10] = mac.mAddress[2];
    address.mFields.m8[11] = 0xff;
    address.mFields.m8[12] = 0xfe;
    address.mFields.m8[13] = mac.mAddress[3];
    address.mFields.m8[14] = mac.mAddress[4];
    address.mFields.m8[15] = mac.mAddress[5];

    if (aPio.GetValidLifetime() == 0)
    {
        if (HasAddress(address))
        {
            RemoveAddress(address);
            Log("Node %lu (%s) removed address %s from RA (lifetime 0)", ToUlong(GetNode().GetId()),
                GetNode().GetName(), address.ToString().AsCString());
        }
        ExitNow();
    }

    AddAddress(address);
    Log("Node %lu (%s) auto-configured address %s from RA", ToUlong(GetNode().GetId()), GetNode().GetName(),
        address.ToString().AsCString());

exit:
    return;
}

void InfraIf::SendIp6(const Ip6::Address &aSrcAddress,
                      const Ip6::Address &aDestAddress,
                      const uint8_t      *aBuffer,
                      uint16_t            aBufferLength)
{
    Message *message = GetNode().Get<MessagePool>().Allocate(Message::kTypeIp6);

    VerifyOrQuit(message != nullptr);

    Log("InfraIf::SendIp6 from %s to %s (len:%u)", aSrcAddress.ToString().AsCString(),
        aDestAddress.ToString().AsCString(), aBufferLength);

    SuccessOrQuit(message->AppendBytes(aBuffer, aBufferLength));

    mPendingTxQueue.Enqueue(*message);
}

void InfraIf::SendEchoRequest(const Ip6::Address &aSrcAddress,
                              const Ip6::Address &aDestAddress,
                              uint16_t            aIdentifier,
                              uint16_t            aPayloadSize)
{
    Message          *message;
    Ip6::Header       ip6Header;
    Ip6::Icmp::Header icmpHeader;

    message = GetNode().Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(sizeof(Ip6::Icmp::Header) + aPayloadSize);
    ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
    ip6Header.SetHopLimit(64);
    ip6Header.SetSource(aSrcAddress);
    ip6Header.SetDestination(aDestAddress);

    icmpHeader.Clear();
    icmpHeader.SetType(Ip6::Icmp::Header::kTypeEchoRequest);
    icmpHeader.SetCode(static_cast<Ip6::Icmp::Header::Code>(0));
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(0);

    SuccessOrQuit(message->SetLength(aPayloadSize));
    SuccessOrQuit(message->Prepend(icmpHeader));
    message->SetOffset(0);

    Checksum::UpdateMessageChecksum(*message, aSrcAddress, aDestAddress, Ip6::kProtoIcmp6);

    SuccessOrQuit(message->Prepend(ip6Header));

    mPendingTxQueue.Enqueue(*message);
}

void InfraIf::Receive(Node &aSrcNode, const Ip6::Header &aHeader, Message &aMessage)
{
    Node &node          = GetNode();
    bool  isIcmp6Nd     = false;
    bool  isEchoRequest = false;

    VerifyOrExit(!node.mInfraIf.HasAddress(aHeader.GetSource()));
    VerifyOrExit(!node.Get<NetworkData::Leader>().IsOnMesh(aHeader.GetSource()));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal())
    {
        VerifyOrExit(node.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrExit(node.Get<BackboneRouter::MulticastListenersTable>().Has(aHeader.GetDestination()));
    }
#endif

    Core::Get().SetActiveNode(&node);

    if (aHeader.GetNextHeader() == Ip6::kProtoIcmp6 &&
        aMessage.GetLength() >= sizeof(Ip6::Header) + sizeof(Ip6::Icmp::Header) &&
        (aHeader.GetDestination() == Ip6::Address::GetLinkLocalAllNodesMulticast() ||
         aHeader.GetDestination() == Ip6::Address::GetLinkLocalAllRoutersMulticast() ||
         node.mInfraIf.HasAddress(aHeader.GetDestination())))
    {
        Ip6::Icmp::Header icmpHeader;

        SuccessOrQuit(aMessage.Read(sizeof(Ip6::Header), icmpHeader));

        switch (icmpHeader.GetType())
        {
        case Ip6::Icmp::Header::kTypeRouterAdvert:
        case Ip6::Icmp::Header::kTypeRouterSolicit:
        case Ip6::Icmp::Header::kTypeNeighborAdvert:
        case Ip6::Icmp::Header::kTypeNeighborSolicit:
            isIcmp6Nd = true;
            break;
        case Ip6::Icmp::Header::kTypeEchoRequest:
            isEchoRequest = true;
            break;
        default:
            break;
        }
    }

    if (isIcmp6Nd)
    {
        Heap::Data payload;
        uint16_t   offset = sizeof(Ip6::Header);

        SuccessOrQuit(payload.SetFrom(aMessage, offset, aMessage.GetLength() - offset));

        otPlatInfraIfRecvIcmp6Nd(&node.GetInstance(), mIfIndex,
                                 reinterpret_cast<const otIp6Address *>(&aHeader.GetSource()), payload.GetBytes(),
                                 payload.GetLength());
        node.mInfraIf.ProcessIcmp6Nd(aHeader.GetSource(), payload.GetBytes(), payload.GetLength());
    }
    else if (isEchoRequest)
    {
        HandleEchoRequest(aHeader, aMessage);
    }
    else
    {
        // We also deliver generic IPv6 packets to the stack if they are NOT ICMPv6 ND packets.
        // (ND packets were already delivered via otPlatInfraIfRecvIcmp6Nd above).
        OwnedPtr<Message> messagePtr;

        messagePtr.Reset(node.Get<Ip6::Ip6>().NewMessage());
        VerifyOrQuit(messagePtr != nullptr);

        SuccessOrQuit(messagePtr->AppendBytesFromMessage(aMessage, 0, aMessage.GetLength()));
        messagePtr->SetOrigin(Message::kOriginHostUntrusted);
        messagePtr->SetLoopbackToHostAllowed(false);

        SuccessOrQuit(node.Get<Ip6::Ip6>().SendRaw(messagePtr.PassOwnership()));
    }

    Core::Get().SetActiveNode(&aSrcNode);

exit:
    return;
}

void InfraIf::HandleEchoRequest(const Ip6::Header &aHeader, Message &aMessage)
{
    Node             &node = GetNode();
    Message          *replyMessage;
    Ip6::Header       replyHeader;
    Ip6::Icmp::Header replyIcmp;
    uint16_t          payloadLen = aMessage.GetLength() - sizeof(Ip6::Header);

    replyMessage = node.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(replyMessage != nullptr);

    SuccessOrQuit(replyMessage->SetLength(payloadLen));
    replyMessage->WriteBytesFromMessage(0, aMessage, sizeof(Ip6::Header), payloadLen);

    SuccessOrQuit(replyMessage->Read(0, replyIcmp));

    replyHeader.InitVersionTrafficClassFlow();
    replyHeader.SetPayloadLength(payloadLen);
    replyHeader.SetNextHeader(Ip6::kProtoIcmp6);
    replyHeader.SetHopLimit(64);
    replyHeader.SetSource(aHeader.GetDestination());
    replyHeader.SetDestination(aHeader.GetSource());

    replyIcmp.SetType(Ip6::Icmp::Header::kTypeEchoReply);
    replyMessage->Write(0, replyIcmp);

    // Recalculate ICMPv6 checksum
    replyMessage->SetOffset(0);
    Checksum::UpdateMessageChecksum(*replyMessage, replyHeader.GetSource(), replyHeader.GetDestination(),
                                    Ip6::kProtoIcmp6);

    SuccessOrQuit(replyMessage->Prepend(replyHeader));

    mPendingTxQueue.Enqueue(*replyMessage);
}

void InfraIf::GetLinkLayerAddress(otPlatInfraIfLinkLayerAddress &aLinkLayerAddress) const
{
    // Use a unique MAC address based on Node ID
    ClearAllBytes(aLinkLayerAddress);
    aLinkLayerAddress.mLength     = 6;
    aLinkLayerAddress.mAddress[0] = 0x02;
    aLinkLayerAddress.mAddress[5] = static_cast<uint8_t>(mNodeId);
}

Node &InfraIf::GetNode(void)
{
    OT_ASSERT(mNode != nullptr);
    return *mNode;
}

const Node &InfraIf::GetNode(void) const
{
    OT_ASSERT(mNode != nullptr);
    return *mNode;
}

extern "C" {

bool otPlatInfraIfHasAddress(otInstance *aInstance, uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return AsNode(aInstance).mInfraIf.HasAddress(AsCoreType(aAddress));
}

otError otPlatInfraIfSendIcmp6Nd(otInstance         *aInstance,
                                 uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t      *aBuffer,
                                 uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    AsNode(aInstance).mInfraIf.SendIcmp6Nd(AsCoreType(aDestAddress), aBuffer, aBufferLength);

    return OT_ERROR_NONE;
}

otError otPlatInfraIfDiscoverNat64Prefix(otInstance *aInstance, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatGetInfraIfLinkLayerAddress(otInstance                    *aInstance,
                                         uint32_t                       aInfraIfIndex,
                                         otPlatInfraIfLinkLayerAddress *aLinkLayerAddress)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    AsNode(aInstance).mInfraIf.GetLinkLayerAddress(*aLinkLayerAddress);

    return OT_ERROR_NONE;
}

} // extern "C"

} // namespace Nexus
} // namespace ot
