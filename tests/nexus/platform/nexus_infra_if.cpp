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

InfraIf::InfraIf(Instance &aInstance)
    : mNode(nullptr)
    , mNodeId(0)
    , mIfIndex(0)
    , mHasRioPrefix(false)
    , mRaTimer(aInstance)
{
}

void InfraIf::Init(Node &aNode)
{
    Ip6::Address             address;
    LinkLayerAddress         mac;
    Ip6::InterfaceIdentifier iid;

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
    mNode->mMdns.HandleHostAddressEvent(aAddress, /* aAdded */ true);

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
            mNode->mMdns.HandleHostAddressEvent(aAddress, /* aAdded */ false);
            break;
        }
    }
}

void InfraIf::RemoveAllAddresses(void)
{
    mAddresses.Clear();
    mNode->mMdns.HandleHostAddressRemoveAll();
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

void InfraIf::SendRouterAdvertisement(const Ip6::Address &aDestination,
                                      const Ip6::Prefix  *aPioPrefix,
                                      const Ip6::Prefix  *aRioPrefix)
{
    Ip6::Nd::RouterAdvert::TxMessage ra;
    Ip6::Nd::RouterAdvert::Header    header;
    Ip6::Nd::Icmp6Packet             packet;

    header.SetToDefault();
    header.SetRouterLifetime(1800);
    SuccessOrQuit(ra.Append(header));

    if (aPioPrefix != nullptr)
    {
        SuccessOrQuit(ra.AppendPrefixInfoOption(*aPioPrefix, 1800, 1800,
                                                Ip6::Nd::PrefixInfoOption::kOnLinkFlag |
                                                    Ip6::Nd::PrefixInfoOption::kAutoConfigFlag));
    }

    if (aRioPrefix != nullptr)
    {
        SuccessOrQuit(ra.AppendRouteInfoOption(*aRioPrefix, 1800, NetworkData::kRoutePreferenceMedium));
    }

    ra.GetAsPacket(packet);

    SendIcmp6Nd(aDestination, packet.GetBytes(), packet.GetLength());
}

void InfraIf::StartRouterAdvertisement(const Ip6::Prefix &aPioPrefix, const Ip6::Prefix *aRioPrefix)
{
    mPioPrefix = aPioPrefix;
    if (aRioPrefix != nullptr)
    {
        mRioPrefix    = *aRioPrefix;
        mHasRioPrefix = true;
    }
    else
    {
        mHasRioPrefix = false;
    }

    // Trigger initial RA immediately
    SendPeriodicRouterAdvertisement();
}

void InfraIf::StopRouterAdvertisement(void) { mRaTimer.Stop(); }

void InfraIf::HandleRaTimer(void) { SendPeriodicRouterAdvertisement(); }

void InfraIf::SendPeriodicRouterAdvertisement(void)
{
    const uint32_t kRaInterval = 4000; // 4 seconds in milliseconds

    SendRouterAdvertisement(Ip6::Address::GetLinkLocalAllNodesMulticast(), &mPioPrefix,
                            mHasRioPrefix ? &mRioPrefix : nullptr);
    mRaTimer.Start(kRaInterval);
}

void InfraIf::ProcessIcmp6Nd(const Ip6::Address &aSrcAddress, const uint8_t *aBuffer, uint16_t aBufferLength)
{
    Ip6::Nd::Icmp6Packet packet;

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
        HandleRouterSolicitation(aSrcAddress);
        break;

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
    Ip6::Prefix      prefix;
    Ip6::Address     address;
    LinkLayerAddress mac;

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

void InfraIf::HandleRouterSolicitation(const Ip6::Address &aSrcAddress)
{
    if (mRaTimer.IsRunning())
    {
        const Ip6::Address &dest =
            aSrcAddress.IsUnspecified() ? Ip6::Address::GetLinkLocalAllNodesMulticast() : aSrcAddress;
        SendRouterAdvertisement(dest, &mPioPrefix, mHasRioPrefix ? &mRioPrefix : nullptr);
    }
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
                              uint16_t            aPayloadSize,
                              uint8_t             aHopLimit)
{
    Message          *message;
    Ip6::Header       ip6Header;
    Ip6::Icmp::Header icmpHeader;

    message = GetNode().Get<Ip6::Ip6>().NewMessage();
    VerifyOrQuit(message != nullptr);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(sizeof(Ip6::Icmp::Header) + aPayloadSize);
    ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
    ip6Header.SetHopLimit(aHopLimit);
    ip6Header.SetSource(aSrcAddress);
    ip6Header.SetDestination(aDestAddress);

    icmpHeader.Clear();
    icmpHeader.SetType(Ip6::Icmp::Header::kTypeEchoRequest);
    icmpHeader.SetCode(static_cast<Ip6::Icmp::Header::Code>(0));
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(0);

    SuccessOrQuit(message->Append(icmpHeader));
    SuccessOrQuit(message->IncreaseLength(aPayloadSize));

    Checksum::UpdateMessageChecksum(*message, aSrcAddress, aDestAddress, Ip6::kProtoIcmp6);

    SuccessOrQuit(message->Prepend(ip6Header));

    mPendingTxQueue.Enqueue(*message);
}

void InfraIf::SendUdp(const Ip6::Address &aSrcAddress,
                      const Ip6::Address &aDestAddress,
                      uint16_t            aSourcePort,
                      uint16_t            aDestPort,
                      uint16_t            aPayloadSize)
{
    Message *message = GetNode().Get<Ip6::Ip6>().NewMessage();

    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->IncreaseLength(aPayloadSize));
    SendUdp(aSrcAddress, aDestAddress, aSourcePort, aDestPort, *message);
}

void InfraIf::SendUdp(const Ip6::Address &aSrcAddress,
                      const Ip6::Address &aDestAddress,
                      uint16_t            aSourcePort,
                      uint16_t            aDestPort,
                      Message            &aPayload)
{
    Ip6::Header      ip6Header;
    Ip6::Udp::Header udpHeader;

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(sizeof(Ip6::Udp::Header) + aPayload.GetLength());
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(64);
    ip6Header.SetSource(aSrcAddress);
    ip6Header.SetDestination(aDestAddress);

    udpHeader.SetSourcePort(aSourcePort);
    udpHeader.SetDestinationPort(aDestPort);
    udpHeader.SetLength(sizeof(Ip6::Udp::Header) + aPayload.GetLength());
    udpHeader.SetChecksum(0);

    SuccessOrQuit(aPayload.Prepend(udpHeader));
    aPayload.SetOffset(0);
    Checksum::UpdateMessageChecksum(aPayload, aSrcAddress, aDestAddress, Ip6::kProtoUdp);
    SuccessOrQuit(aPayload.Prepend(ip6Header));
    aPayload.SetOffset(0);

    if (aDestAddress.IsMulticast())
    {
        Message *loopbackMessage = aPayload.Clone<kNoReservedHeader>();

        VerifyOrQuit(loopbackMessage != nullptr);
        Receive(*loopbackMessage);
        loopbackMessage->Free();
    }

    mPendingTxQueue.Enqueue(aPayload);
}

void InfraIf::Receive(Message &aMessage)
{
    Node        &node = GetNode();
    Ip6::Headers headers;

    aMessage.SetOffset(0);
    SuccessOrExit(headers.ParseFrom(aMessage));

    if (headers.IsIcmp6() && (headers.GetDestinationAddress() == Ip6::Address::GetLinkLocalAllNodesMulticast() ||
                              headers.GetDestinationAddress() == Ip6::Address::GetLinkLocalAllRoutersMulticast() ||
                              node.mInfraIf.HasAddress(headers.GetDestinationAddress())))
    {
        switch (headers.GetIcmpHeader().GetType())
        {
        case Ip6::Icmp::Header::kTypeRouterAdvert:
        case Ip6::Icmp::Header::kTypeRouterSolicit:
        case Ip6::Icmp::Header::kTypeNeighborAdvert:
        case Ip6::Icmp::Header::kTypeNeighborSolicit:
        {
            Heap::Data payload;
            uint16_t   offset = sizeof(Ip6::Header);

            SuccessOrQuit(payload.SetFrom(aMessage, offset, aMessage.GetLength() - offset));

            otPlatInfraIfRecvIcmp6Nd(&node.GetInstance(), mIfIndex,
                                     reinterpret_cast<const otIp6Address *>(&headers.GetSourceAddress()),
                                     payload.GetBytes(), payload.GetLength());
            node.mInfraIf.ProcessIcmp6Nd(headers.GetSourceAddress(), payload.GetBytes(), payload.GetLength());
            ExitNow();
        }

        case Ip6::Icmp::Header::kTypeEchoRequest:
            HandleEchoRequest(headers.GetIp6Header(), aMessage);
            ExitNow();

        case Ip6::Icmp::Header::kTypeEchoReply:
            HandleEchoReply(headers.GetIp6Header(), aMessage);
            ExitNow();

        default:
            break;
        }
    }

    if (headers.IsUdp() && headers.GetDestinationPort() == Mdns::kUdpPort)
    {
        if (headers.GetDestinationAddress().IsMulticast() || node.mInfraIf.HasAddress(headers.GetDestinationAddress()))
        {
            Mdns::AddressInfo senderAddress;
            Message          *payload = aMessage.Clone<kNoReservedHeader>();

            VerifyOrQuit(payload != nullptr);
            payload->RemoveHeader(sizeof(Ip6::Header) + sizeof(Ip6::Udp::Header));

            senderAddress.mAddress      = headers.GetSourceAddress();
            senderAddress.mPort         = headers.GetSourcePort();
            senderAddress.mInfraIfIndex = Mdns::kInfraIfIndex;

            node.mMdns.Receive(node.GetInstance(), *payload, !headers.GetDestinationAddress().IsMulticast(),
                               senderAddress);
            payload->Free();
        }
        ExitNow();
    }

    {
        // We also deliver generic IPv6 packets to the stack if they are NOT ICMPv6 ND packets.
        // (ND packets were already delivered via otPlatInfraIfRecvIcmp6Nd above).
        OwnedPtr<Message> messagePtr;
        Ip6::Header       updatedHeader = headers.GetIp6Header();

        VerifyOrExit(!node.mInfraIf.HasAddress(headers.GetSourceAddress()));
        VerifyOrExit(!node.Get<NetworkData::Leader>().IsOnMesh(headers.GetSourceAddress()));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
        if (headers.GetDestinationAddress().IsMulticastLargerThanRealmLocal())
        {
            VerifyOrExit(node.Get<BackboneRouter::Local>().IsPrimary());
            VerifyOrExit(node.Get<BackboneRouter::MulticastListenersTable>().Has(headers.GetDestinationAddress()));
        }
#endif

        VerifyOrExit(updatedHeader.GetHopLimit() > 1);
        updatedHeader.SetHopLimit(updatedHeader.GetHopLimit() - 1);

        messagePtr.Reset(aMessage.Clone<kNoReservedHeader>());
        VerifyOrQuit(messagePtr != nullptr);

        messagePtr->Write(0, updatedHeader);
        messagePtr->SetOrigin(Message::kOriginHostUntrusted);
        messagePtr->SetLoopbackToHostAllowed(false);

        IgnoreError(node.Get<Ip6::Ip6>().SendRaw(messagePtr.PassOwnership()));
    }

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

void InfraIf::HandleEchoReply(const Ip6::Header &aHeader, Message &aMessage)
{
    Ip6::Icmp::Header icmpHeader;

    SuccessOrQuit(aMessage.Read(sizeof(Ip6::Header), icmpHeader));
    mEchoReplyCallback.InvokeIfSet(aHeader.GetSource(), icmpHeader.GetId(), icmpHeader.GetSequence());
}

void InfraIf::GetLinkLayerAddress(LinkLayerAddress &aLinkLayerAddress) const
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
