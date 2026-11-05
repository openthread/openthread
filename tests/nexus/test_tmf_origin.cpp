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

#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kPropagationTime    = 10 * 1000;

static constexpr uint16_t kNonTmfPort = 12345;

static constexpr uint8_t  kMaxMplRetxAttempts = 10;
static constexpr uint16_t kMaxSendWaitTime    = 1000;

static Message *NewAddressErrorDatagram(Node               &aNode,
                                        const Ip6::Address &aSource,
                                        const Ip6::Address &aDestination,
                                        const Ip6::Address &aTarget,
                                        uint16_t            aDestPort = Tmf::kUdpPort)
{
    // Returns a new message holding a UDP datagram (its UDP header and
    // payload, with its checksum) of an Address Error Notification for
    // `aTarget` from `aSource` to `aDestination` and `aDestPort`.
    //
    // Each message gets a new CoAP Message ID: a confirmable request
    // with the Message ID of one already processed is answered from
    // the CoAP response cache and is not processed again.

    static uint16_t sMessageId = 0;

    Tmf::Message            *tmfMessage;
    Message                 *message;
    Ip6::UdpHeader           udpHeader;
    Ip6::InterfaceIdentifier meshLocalIid;

    meshLocalIid.Clear();

    tmfMessage = aNode.Get<Tmf::Agent>().AllocateAndInitPostMessageTo(kUriAddressError, aDestination);
    VerifyOrQuit(tmfMessage != nullptr);
    tmfMessage->WriteMessageId(++sMessageId);
    SuccessOrQuit(Tlv::Append<ThreadTargetTlv>(*tmfMessage, aTarget));
    SuccessOrQuit(Tlv::Append<ThreadMeshLocalEidTlv>(*tmfMessage, meshLocalIid));

    udpHeader.Clear();
    udpHeader.SetSourcePort(Tmf::kUdpPort);
    udpHeader.SetDestinationPort(aDestPort);
    udpHeader.SetLength(sizeof(udpHeader) + tmfMessage->GetLength());

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Append(udpHeader));
    SuccessOrQuit(message->AppendBytesFromMessage(*tmfMessage, 0, tmfMessage->GetLength()));
    tmfMessage->Free();

    Checksum::UpdateMessageChecksum(*message, aSource, aDestination, Ip6::kProtoUdp);

    return message;
}

static Error SendUntrustedAddressError(Node               &aNode,
                                       const Ip6::Address &aSource,
                                       const Ip6::Address &aDestination,
                                       const Ip6::Address &aTarget,
                                       uint16_t            aDestPort = Tmf::kUdpPort)
{
    // Sends an Address Error Notification for `aTarget` from `aSource`
    // to `aDestination` and `aDestPort` as a raw IPv6 datagram with a
    // host-untrusted origin, using `SendRaw()`, and returns the error
    // it returned.

    Message    *datagram;
    Message    *message;
    Ip6::Header ip6Header;

    datagram = NewAddressErrorDatagram(aNode, aSource, aDestination, aTarget, aDestPort);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetPayloadLength(datagram->GetLength());

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Append(ip6Header));
    SuccessOrQuit(message->AppendBytesFromMessage(*datagram, 0, datagram->GetLength()));
    datagram->Free();

    message->SetOrigin(Message::kOriginHostUntrusted);

    return aNode.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message));
}

static uint16_t GetMplBufferedMessageCount(Node &aNode)
{
    MessageQueue::Info info;

    aNode.Get<Ip6::Mpl>().GetBufferedMessageSetInfo(info);

    return info.mNumMessages;
}

static void SendUntrustedAddressErrorByMplRetx(Core               &aNexus,
                                               Node               &aSender,
                                               Node               &aReceiver,
                                               const Ip6::Address &aTarget,
                                               bool                aFilterEnabled)
{
    // Sends a host-untrusted Address Error Notification for `aTarget`
    // from `aSender` to the realm-local all-routers address, so that
    // only its MPL retransmission can reach `aReceiver`.
    //
    // The message is sent with the TMF origin filter disabled and the
    // link to `aReceiver` down, so it is buffered by MPL on `aSender`
    // without reaching `aReceiver`. The filter is then set to
    // `aFilterEnabled` for the retransmission, and the link is
    // restored once the message itself has been sent. MPL schedules
    // the retransmission at a random time: if it was already sent by
    // then, this is repeated with a new message.

    Ip6::Ip6 &ip6 = aSender.Get<Ip6::Ip6>();

    UnallowLinkBetween(aSender, aReceiver);

    for (uint8_t attempt = 0;; attempt++)
    {
        PriorityQueue::Info sendQueueInfo;
        MessageQueue::Info  reassemblyQueueInfo;
        uint16_t            waitTime = 0;

        VerifyOrQuit(attempt < kMaxMplRetxAttempts);
        VerifyOrQuit(GetMplBufferedMessageCount(aSender) == 0);

        ip6.SetTmfOriginFilterEnabled(false);
        SuccessOrQuit(SendUntrustedAddressError(aSender, aSender.Get<Mle::Mle>().GetMeshLocalEid(),
                                                Ip6::Address::GetRealmLocalAllRoutersMulticast(), aTarget));
        ip6.SetTmfOriginFilterEnabled(aFilterEnabled);

        VerifyOrQuit(GetMplBufferedMessageCount(aSender) == 1);

        do
        {
            VerifyOrQuit(waitTime++ < kMaxSendWaitTime);
            aNexus.AdvanceTime(1);
            aSender.Get<MeshForwarder>().GetQueueInfo(sendQueueInfo, reassemblyQueueInfo);
        } while (sendQueueInfo.mNumMessages != 0);

        if (GetMplBufferedMessageCount(aSender) != 0)
        {
            break;
        }
    }

    AllowLinkBetween(aSender, aReceiver);
}

static Error SendUntrustedLargeAddressError(Node &aNode, const Ip6::Address &aDestination, const Ip6::Address &aTarget)
{
    // Sends an Address Error Notification for `aTarget` to
    // `aDestination` with a host-untrusted origin, using
    // `Udp::SendDatagram()`, and returns the error it returned. It is
    // padded with an unknown TLV so that the IPv6 datagram is larger
    // than the minimum MTU and is sent as IPv6 fragments.

    static constexpr uint8_t  kUnknownTlvType = 0xfe;
    static constexpr uint16_t kPaddingLength  = 1280;

    Tmf::Message            *message;
    Ip6::MessageInfo         messageInfo;
    Ip6::InterfaceIdentifier meshLocalIid;
    uint8_t                  padding[kPaddingLength];
    Error                    error;

    meshLocalIid.Clear();
    ClearAllBytes(padding);

    message = aNode.Get<Tmf::Agent>().AllocateAndInitPostMessageTo(kUriAddressError, aDestination);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<ThreadTargetTlv>(*message, aTarget));
    SuccessOrQuit(Tlv::Append<ThreadMeshLocalEidTlv>(*message, meshLocalIid));
    SuccessOrQuit(Tlv::AppendTlv(*message, kUnknownTlvType, padding, sizeof(padding)));

    message->SetOrigin(Message::kOriginHostUntrusted);

    messageInfo.SetSockAddr(aNode.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetSockPort(Tmf::kUdpPort);
    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetPeerPort(Tmf::kUdpPort);

    error = aNode.Get<Ip6::Udp>().SendDatagram(*message, messageInfo);
    FreeMessageOnError(message, error);

    return error;
}

struct ReceivedDatagramInfo
{
    // Records the datagrams received by a UDP socket, and the origin,
    // link security setting and link info of the last one.

    uint16_t       mCount;
    bool           mIsOriginHostUntrusted;
    bool           mIsLinkSecurityEnabled;
    Error          mLinkInfoError;
    ThreadLinkInfo mLinkInfo;
#if OPENTHREAD_CONFIG_MULTI_RADIO
    bool mIsRadioTypeSet;
#endif
};

static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    ReceivedDatagramInfo *info    = static_cast<ReceivedDatagramInfo *>(aContext);
    const Message        &message = AsCoreType(aMessage);

    OT_UNUSED_VARIABLE(aMessageInfo);

    info->mCount++;
    info->mIsOriginHostUntrusted = message.IsOriginHostUntrusted();
    info->mIsLinkSecurityEnabled = message.IsLinkSecurityEnabled();
    info->mLinkInfoError         = message.GetLinkInfo(info->mLinkInfo);
#if OPENTHREAD_CONFIG_MULTI_RADIO
    info->mIsRadioTypeSet = message.IsRadioTypeSet();
#endif
}

static Error SendRawFragment(Node               &aNode,
                             const Ip6::Address &aSource,
                             const Ip6::Address &aDestination,
                             uint8_t             aNextHeader,
                             const Message      &aPayload,
                             uint16_t            aOffset,
                             uint16_t            aLength,
                             Message::Origin     aOrigin,
                             bool                aLinkSecurity)
{
    // Sends `aLength` bytes of `aPayload` (the fragmentable part of a
    // datagram from `aSource` to `aDestination`, starting with a header
    // of type `aNextHeader`) from `aOffset` as an IPv6 fragment, using
    // `SendRaw()` with `aOrigin` and `aLinkSecurity`, and returns the
    // error it returned. It is the last fragment if it ends at the end
    // of `aPayload`.

    static constexpr uint32_t kIdentification = 0x12345678;

    Message            *message;
    Ip6::Header         ip6Header;
    Ip6::FragmentHeader fragmentHeader;

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);
    ip6Header.SetNextHeader(Ip6::kProtoFragment);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetPayloadLength(sizeof(fragmentHeader) + aLength);

    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(aNextHeader);
    fragmentHeader.SetIdentification(kIdentification);
    fragmentHeader.SetOffset(Ip6::FragmentHeader::BytesToFragmentOffset(aOffset));

    if (aOffset + aLength < aPayload.GetLength())
    {
        fragmentHeader.SetMoreFlag();
    }

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Append(ip6Header));
    SuccessOrQuit(message->Append(fragmentHeader));
    SuccessOrQuit(message->AppendBytesFromMessage(aPayload, aOffset, aLength));

    message->SetOrigin(aOrigin);
    message->SetLinkSecurityEnabled(aLinkSecurity);

    return aNode.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message));
}

static void SendRawFragment(Node               &aNode,
                            const Message      &aDatagram,
                            const Ip6::Address &aAddress,
                            uint16_t            aOffset,
                            uint16_t            aLength,
                            Message::Origin     aOrigin,
                            bool                aLinkSecurity)
{
    // Sends `aLength` bytes of `aDatagram` (a UDP datagram from and to
    // `aAddress`) from `aOffset` as an IPv6 fragment, using
    // `SendRaw()` with `aOrigin` and `aLinkSecurity`.
    //
    // `HandleFragment()` consumes every fragment (its payload is
    // either added to a reassembly or dropped), so `SendRaw()` returns
    // `kErrorDrop`.

    IgnoreError(SendRawFragment(aNode, aAddress, aAddress, Ip6::kProtoUdp, aDatagram, aOffset, aLength, aOrigin,
                                aLinkSecurity));
}

void TestTmfOrigin(void)
{
    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    Ip6::Address mlEid     = node.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address linkLocal = node.Get<Mle::Mle>().GetLinkLocalAddress();

    // Allocate and construct the message:
    // Outer IPv6 Header + Inner IPv6 Header + Inner UDP Header
    Message *message = node.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Inner UDP Header
    Ip6::UdpHeader udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(1234);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(sizeof(Ip6::UdpHeader));

    // 2. Inner IPv6 Header
    Ip6::Header innerHeader;
    innerHeader.Clear();
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetSource(Ip6::Address::GetLinkLocalAllNodesMulticast()); // dummy LL source
    innerHeader.SetDestination(linkLocal);
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetPayloadLength(sizeof(Ip6::UdpHeader));

    // 3. Outer IPv6 Header (IP-in-IP)
    Ip6::Header outerHeader;
    outerHeader.Clear();
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetSource(linkLocal);
    outerHeader.SetDestination(mlEid);
    outerHeader.SetNextHeader(Ip6::kProtoIp6);
    outerHeader.SetPayloadLength(sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));

    // Append all headers to the message
    SuccessOrQuit(message->Append(outerHeader));
    SuccessOrQuit(message->Append(innerHeader));
    SuccessOrQuit(message->Append(udpHeader));

    // Mark the message origin as Host Untrusted
    message->SetOrigin(Message::kOriginHostUntrusted);

    // Call SendRaw, which will trigger HandleDatagram
    Error error = node.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message));

    Log("SendRaw returned error: %s", ErrorToString(error));

    // Verify that SendRaw returned kErrorDrop
    VerifyOrQuit(error == kErrorDrop);

    Log("Verified that untrusted IP-in-IP packet was dropped successfully.");
}

void TestTmfOriginBypassed(void)
{
    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    Ip6::Address mlEid     = node.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address linkLocal = node.Get<Mle::Mle>().GetLinkLocalAddress();

    // Allocate and construct the message:
    // Outer IPv6 Header + Destination Options Header + Inner IPv6 Header + Inner UDP Header
    Message *message = node.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Inner UDP Header
    Ip6::UdpHeader udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(1234);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(sizeof(Ip6::UdpHeader));

    // 2. Inner IPv6 Header
    Ip6::Header innerHeader;
    innerHeader.Clear();
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetSource(Ip6::Address::GetLinkLocalAllNodesMulticast()); // dummy LL source
    innerHeader.SetDestination(linkLocal);
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetPayloadLength(sizeof(Ip6::UdpHeader));

    // 3. Destination Options Header (containing 6 bytes of padding to make it 8 bytes total)
    Ip6::ExtensionHeader dstHeader;
    dstHeader.SetNextHeader(Ip6::kProtoIp6);
    dstHeader.SetLength(0);

    Ip6::PadOption padOption;
    padOption.InitForPadSize(6);

    // 4. Outer IPv6 Header (IP-in-IP)
    Ip6::Header outerHeader;
    outerHeader.Clear();
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetSource(linkLocal);
    outerHeader.SetDestination(mlEid);
    outerHeader.SetNextHeader(Ip6::kProtoDstOpts);
    outerHeader.SetPayloadLength(dstHeader.GetSize() + sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));

    // Append all headers to the message
    SuccessOrQuit(message->Append(outerHeader));
    SuccessOrQuit(message->Append(dstHeader));
    SuccessOrQuit(message->AppendBytes(&padOption, padOption.GetSize()));
    SuccessOrQuit(message->Append(innerHeader));
    SuccessOrQuit(message->Append(udpHeader));

    // Mark the message origin as Host Untrusted
    message->SetOrigin(Message::kOriginHostUntrusted);

    // Call SendRaw, which will trigger HandleDatagram
    Error error = node.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message));

    Log("SendRaw bypassed returned error: %s", ErrorToString(error));

    // Verify that SendRaw returned kErrorDrop due to the traversal detection
    VerifyOrQuit(error == kErrorDrop);

    Log("Verified that untrusted bypassed IP-in-IP packet was dropped successfully.");
}

void TestTmfOriginLocalReceive(void)
{
    Core                       nexus;
    Node                      &node = nexus.CreateNode();
    Ip6::Netif::UnicastAddress eid;
    Error                      error;

    node.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    const Ip6::Address rloc  = node.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address mlEid = node.Get<Mle::Mle>().GetMeshLocalEid();

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(node.Get<ThreadNetif>().AddExternalUnicastAddress(eid));

    // A host-untrusted TMF message destined to the node itself must
    // be dropped and not be processed by the node's TMF agent.

    error = SendUntrustedAddressError(node, rloc, mlEid, eid.GetAddress());
    Log("SendRaw (local, TMF port) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorDrop);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // A host-untrusted datagram to another port is still received.

    error = SendUntrustedAddressError(node, rloc, mlEid, eid.GetAddress(), kNonTmfPort);
    Log("SendRaw (local, non-TMF port) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorNone);

    // With the filter disabled, the same TMF message is processed.

    node.Get<Ip6::Ip6>().SetTmfOriginFilterEnabled(false);

    error = SendUntrustedAddressError(node, rloc, mlEid, eid.GetAddress());
    Log("SendRaw (local, TMF port, filter disabled) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorNone);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("Verified that untrusted TMF message to the node itself was dropped successfully.");
}

void TestTmfOriginMplRetransmission(void)
{
    Core                       nexus;
    Node                      &leader = nexus.CreateNode();
    Node                      &router = nexus.CreateNode();
    Ip6::Netif::UnicastAddress eid;
    Error                      error;

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    const Ip6::Address leaderMlEid = leader.Get<Mle::Mle>().GetMeshLocalEid();
    const Ip6::Address allRouters  = Ip6::Address::GetRealmLocalAllRoutersMulticast();

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(router.Get<ThreadNetif>().AddExternalUnicastAddress(eid));

    // A realm-local multicast message from the host is buffered by MPL
    // on the leader for retransmission. For a host-untrusted TMF
    // message, neither the message nor an MPL retransmission of it may
    // reach the router.

    error = SendUntrustedAddressError(leader, leaderMlEid, allRouters, eid.GetAddress());
    Log("SendRaw (realm-local multicast) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorDrop);
    VerifyOrQuit(GetMplBufferedMessageCount(leader) == 0);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // An MPL retransmission of a buffered host-untrusted TMF message
    // keeps its origin and is dropped by the filter.

    SendUntrustedAddressErrorByMplRetx(nexus, leader, router, eid.GetAddress(), /* aFilterEnabled */ true);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // With the filter disabled, the MPL retransmission reaches the
    // router and is processed.

    SendUntrustedAddressErrorByMplRetx(nexus, leader, router, eid.GetAddress(), /* aFilterEnabled */ false);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("Verified that untrusted TMF message was not delivered through MPL retransmission.");
}

void TestTmfOriginLargerScopeMulticast(void)
{
    Core         nexus;
    Node        &node = nexus.CreateNode();
    Ip6::Address siteLocalAllNodes;
    Ip6::Address target;
    Error        error;

    node.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    const Ip6::Address mlEid = node.Get<Mle::Mle>().GetMeshLocalEid();

    SuccessOrQuit(siteLocalAllNodes.FromString("ff05::1"));
    SuccessOrQuit(target.FromString("2001:db8::1"));

    // A multicast message to a scope larger than realm-local is
    // encapsulated in IP-in-IP before it is processed. A host-untrusted
    // TMF message embedded in it must be dropped.

    error = SendUntrustedAddressError(node, mlEid, siteLocalAllNodes, target);
    Log("SendRaw (larger-scope multicast, TMF port) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorDrop);

    // With the filter disabled, the same message is sent.

    node.Get<Ip6::Ip6>().SetTmfOriginFilterEnabled(false);

    error = SendUntrustedAddressError(node, mlEid, siteLocalAllNodes, target);
    Log("SendRaw (larger-scope multicast, TMF port, filter disabled) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorNone);

    Log("Verified that untrusted TMF message to a larger-scope multicast address was dropped successfully.");
}

void TestTmfOriginFragmented(void)
{
    Core                       nexus;
    Node                      &leader = nexus.CreateNode();
    Node                      &router = nexus.CreateNode();
    Ip6::Netif::UnicastAddress eid;

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    const Ip6::Address routerMlEid = router.Get<Mle::Mle>().GetMeshLocalEid();
    const Ip6::Address allRouters  = Ip6::Address::GetRealmLocalAllRoutersMulticast();

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(router.Get<ThreadNetif>().AddExternalUnicastAddress(eid));

    // A host-untrusted TMF message larger than the minimum MTU is
    // dropped by `SendDatagram()` as a whole, before it is sent as
    // IPv6 fragments, so no fragment of it is sent. The same applies
    // to a realm-local multicast destination, for which an MPL option
    // is added before the message is fragmented.

    VerifyOrQuit(SendUntrustedLargeAddressError(leader, routerMlEid, eid.GetAddress()) == kErrorDrop);
    VerifyOrQuit(SendUntrustedLargeAddressError(leader, allRouters, eid.GetAddress()) == kErrorDrop);
    VerifyOrQuit(GetMplBufferedMessageCount(leader) == 0);

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // With the filter disabled, the router reassembles the same message
    // and processes it.

    leader.Get<Ip6::Ip6>().SetTmfOriginFilterEnabled(false);

    SuccessOrQuit(SendUntrustedLargeAddressError(leader, routerMlEid, eid.GetAddress()));

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("Verified that fragmented untrusted TMF message was dropped successfully.");
}

void TestTmfOriginReassembledLinkInfo(void)
{
    static constexpr uint16_t kPayloadLength = 1280;

    Core                 nexus;
    Node                &leader = nexus.CreateNode();
    Node                &router = nexus.CreateNode();
    ReceivedDatagramInfo info;
    Message             *message;
    Ip6::MessageInfo     messageInfo;
    uint8_t              payload[kPayloadLength];

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Ip6::Udp::Socket leaderSocket(leader, HandleUdpReceive, &info);
    Ip6::Udp::Socket routerSocket(router, nullptr, nullptr);

    SuccessOrQuit(leaderSocket.Open(Ip6::kNetifUnspecified));
    SuccessOrQuit(leaderSocket.Bind(kNonTmfPort));
    SuccessOrQuit(routerSocket.Open(Ip6::kNetifThreadInternal));

    // A datagram larger than the minimum MTU is sent by the router as
    // IPv6 fragments over Thread radio. The datagram reassembled by
    // the leader has the link info of the first fragment.

    ClearAllBytes(info);
    ClearAllBytes(payload);

    message = routerSocket.NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->AppendBytes(payload, sizeof(payload)));

    messageInfo.SetPeerAddr(leader.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetPeerPort(kNonTmfPort);
    SuccessOrQuit(routerSocket.SendTo(*message, messageInfo));

    nexus.AdvanceTime(kPropagationTime);

    VerifyOrQuit(info.mCount == 1);
    SuccessOrQuit(info.mLinkInfoError);
    VerifyOrQuit(info.mLinkInfo.mPanId == leader.Get<Mac::Mac>().GetPanId());
    VerifyOrQuit(info.mLinkInfo.mChannel == leader.Get<Mac::Mac>().GetPanChannel());
    VerifyOrQuit(info.mLinkInfo.mLinkSecurity);
#if OPENTHREAD_CONFIG_MULTI_RADIO
    VerifyOrQuit(info.mIsRadioTypeSet);
#endif

    SuccessOrQuit(routerSocket.Close());
    SuccessOrQuit(leaderSocket.Close());

    Log("Verified that a reassembled datagram has the link info of its fragments.");
}

void TestTmfOriginReassemblyMatch(void)
{
    static constexpr uint16_t kPayloadLength       = 64;
    static constexpr uint16_t kFirstFragmentLength = 40;

    Core                 nexus;
    Node                &node = nexus.CreateNode();
    ReceivedDatagramInfo info;
    Message             *datagram;
    Ip6::UdpHeader       udpHeader;
    uint8_t              payload[kPayloadLength];
    uint16_t             lastFragmentLength;

    node.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    const Ip6::Address mlEid = node.Get<Mle::Mle>().GetMeshLocalEid();

    Ip6::Udp::Socket socket(node, HandleUdpReceive, &info);

    SuccessOrQuit(socket.Open(Ip6::kNetifUnspecified));
    SuccessOrQuit(socket.Bind(kNonTmfPort));

    ClearAllBytes(info);
    ClearAllBytes(payload);

    udpHeader.Clear();
    udpHeader.SetSourcePort(kNonTmfPort);
    udpHeader.SetDestinationPort(kNonTmfPort);
    udpHeader.SetLength(sizeof(udpHeader) + sizeof(payload));

    datagram = node.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(datagram != nullptr);
    SuccessOrQuit(datagram->Append(udpHeader));
    SuccessOrQuit(datagram->AppendBytes(payload, sizeof(payload)));
    Checksum::UpdateMessageChecksum(*datagram, mlEid, mlEid, Ip6::kProtoUdp);

    lastFragmentLength = datagram->GetLength() - kFirstFragmentLength;

    // A UDP datagram from the host to the node itself is sent in two
    // IPv6 fragments. The first fragment, with a host-untrusted
    // origin, starts a reassembly. A last fragment with another
    // origin (the host-trusted origin a new reassembly buffer has by
    // default) or link security setting is not added to it, so the
    // datagram is not received.

    SendRawFragment(node, *datagram, mlEid, 0, kFirstFragmentLength, Message::kOriginHostUntrusted,
                    /* aLinkSecurity */ true);

    SendRawFragment(node, *datagram, mlEid, kFirstFragmentLength, lastFragmentLength, Message::kOriginHostTrusted,
                    /* aLinkSecurity */ true);
    nexus.AdvanceTime(0);
    VerifyOrQuit(info.mCount == 0);

    SendRawFragment(node, *datagram, mlEid, kFirstFragmentLength, lastFragmentLength, Message::kOriginHostUntrusted,
                    /* aLinkSecurity */ false);
    nexus.AdvanceTime(0);
    VerifyOrQuit(info.mCount == 0);

    // A last fragment with the same origin and link security setting
    // completes the reassembly, and the datagram is received with
    // the host-untrusted origin.

    SendRawFragment(node, *datagram, mlEid, kFirstFragmentLength, lastFragmentLength, Message::kOriginHostUntrusted,
                    /* aLinkSecurity */ true);
    nexus.AdvanceTime(0);
    VerifyOrQuit(info.mCount == 1);
    VerifyOrQuit(info.mIsOriginHostUntrusted);
    VerifyOrQuit(info.mIsLinkSecurityEnabled);

    // The datagram reassembled from host-trusted fragments without
    // link security keeps that setting.

    SendRawFragment(node, *datagram, mlEid, 0, kFirstFragmentLength, Message::kOriginHostTrusted,
                    /* aLinkSecurity */ false);
    SendRawFragment(node, *datagram, mlEid, kFirstFragmentLength, lastFragmentLength, Message::kOriginHostTrusted,
                    /* aLinkSecurity */ false);
    nexus.AdvanceTime(0);
    VerifyOrQuit(info.mCount == 2);
    VerifyOrQuit(!info.mIsOriginHostUntrusted);
    VerifyOrQuit(!info.mIsLinkSecurityEnabled);

    datagram->Free();
    SuccessOrQuit(socket.Close());

    Log("Verified that fragments with another origin or link security setting are not reassembled together.");
}

static void SendLargeDatagram(Ip6::Udp::Socket   &aSocket,
                              const Ip6::Address &aDestination,
                              Message::Origin     aOrigin,
                              bool                aLinkSecurity)
{
    // Sends a UDP datagram to `aDestination` and `kNonTmfPort` from
    // `aSocket` with `aOrigin` and `aLinkSecurity`, with a payload
    // large enough for the IPv6 datagram to be sent as IPv6 fragments.

    static constexpr uint16_t kPayloadLength = 1280;

    Message         *message;
    Ip6::MessageInfo messageInfo;
    uint8_t          payload[kPayloadLength];

    ClearAllBytes(payload);

    message = aSocket.NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->AppendBytes(payload, sizeof(payload)));

    message->SetOrigin(aOrigin);
    message->SetLinkSecurityEnabled(aLinkSecurity);

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetPeerPort(kNonTmfPort);
    SuccessOrQuit(aSocket.SendTo(*message, messageInfo));
}

void TestTmfOriginFragmentedDatagramOrigin(void)
{
    Core                 nexus;
    Node                &node = nexus.CreateNode();
    ReceivedDatagramInfo info;

    node.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    const Ip6::Address mlEid = node.Get<Mle::Mle>().GetMeshLocalEid();

    Ip6::Udp::Socket socket(node, HandleUdpReceive, &info);

    SuccessOrQuit(socket.Open(Ip6::kNetifUnspecified));
    SuccessOrQuit(socket.Bind(kNonTmfPort));

    ClearAllBytes(info);

    // A host-untrusted datagram (not a TMF message) larger than the
    // minimum MTU is sent by `FragmentDatagram()` as IPv6 fragments,
    // each with the host-untrusted origin, so the datagram reassembled
    // on the node itself is received with that origin.

    SendLargeDatagram(socket, mlEid, Message::kOriginHostUntrusted, /* aLinkSecurity */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(info.mCount == 1);
    VerifyOrQuit(info.mIsOriginHostUntrusted);
    VerifyOrQuit(info.mIsLinkSecurityEnabled);

    // The fragments of a datagram sent without link security are
    // also sent without it.

    SendLargeDatagram(socket, mlEid, Message::kOriginHostTrusted, /* aLinkSecurity */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(info.mCount == 2);
    VerifyOrQuit(!info.mIsOriginHostUntrusted);
    VerifyOrQuit(!info.mIsLinkSecurityEnabled);

    SuccessOrQuit(socket.Close());

    Log("Verified that IPv6 fragments keep the origin and link security setting of their datagram.");
}

void TestTmfOriginRawFragments(void)
{
    static constexpr uint16_t kFirstFragmentLength = 40;

    Core                       nexus;
    Node                      &node = nexus.CreateNode();
    Ip6::Netif::UnicastAddress eid;
    Message                   *datagram;
    Message                   *dstOptsDatagram;
    Message                   *udpDatagram;
    Ip6::ExtensionHeader       dstOptsHeader;
    Ip6::PadOption             padOption;
    uint16_t                   dstOptsFirstFragmentLength;
    Error                      error;

    node.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    const Ip6::Address rloc  = node.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address mlEid = node.Get<Mle::Mle>().GetMeshLocalEid();

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(node.Get<ThreadNetif>().AddExternalUnicastAddress(eid));

    // `datagram` is the UDP datagram of a TMF message to the node
    // itself. `dstOptsDatagram` is the fragmentable part of an IPv6
    // datagram with a Destination Options header (8 bytes, with a
    // PadN option) and the UDP datagram of another such TMF message.

    datagram    = NewAddressErrorDatagram(node, rloc, mlEid, eid.GetAddress());
    udpDatagram = NewAddressErrorDatagram(node, rloc, mlEid, eid.GetAddress());

    dstOptsHeader.SetNextHeader(Ip6::kProtoUdp);
    dstOptsHeader.SetLength(0);
    padOption.InitForPadSize(6);
    dstOptsFirstFragmentLength = dstOptsHeader.GetSize() + kFirstFragmentLength;

    dstOptsDatagram = node.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(dstOptsDatagram != nullptr);
    SuccessOrQuit(dstOptsDatagram->Append(dstOptsHeader));
    SuccessOrQuit(dstOptsDatagram->AppendBytes(&padOption, padOption.GetSize()));
    VerifyOrQuit(dstOptsDatagram->GetLength() == dstOptsHeader.GetSize());
    SuccessOrQuit(dstOptsDatagram->AppendBytesFromMessage(*udpDatagram, 0, udpDatagram->GetLength()));
    udpDatagram->Free();

    // A host-untrusted first fragment carrying the UDP header of a TMF
    // message is dropped, so the last fragment cannot complete a
    // reassembly and the TMF message is not processed.

    error = SendRawFragment(node, rloc, mlEid, Ip6::kProtoUdp, *datagram, 0, kFirstFragmentLength,
                            Message::kOriginHostUntrusted, /* aLinkSecurity */ true);
    Log("SendRaw (first fragment, TMF port) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorDrop);

    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoUdp, *datagram, kFirstFragmentLength,
                                datagram->GetLength() - kFirstFragmentLength, Message::kOriginHostUntrusted,
                                /* aLinkSecurity */ true));

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // A host-untrusted first fragment whose upper-layer header is UDP
    // but which does not contain the UDP header cannot be checked, so
    // it is rejected with `kErrorParse`.

    error = SendRawFragment(node, rloc, mlEid, Ip6::kProtoUdp, *datagram, 0, 0, Message::kOriginHostUntrusted,
                            /* aLinkSecurity */ true);
    Log("SendRaw (first fragment, no UDP header) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorParse);

    // The same applies to a first fragment holding only a Destination
    // Options header, with the UDP header in the next fragment. The
    // next fragment cannot complete a reassembly, so the TMF message
    // is not processed.

    error = SendRawFragment(node, rloc, mlEid, Ip6::kProtoDstOpts, *dstOptsDatagram, 0, dstOptsHeader.GetSize(),
                            Message::kOriginHostUntrusted, /* aLinkSecurity */ true);
    Log("SendRaw (first fragment, Destination Options only) returned error: %s", ErrorToString(error));
    VerifyOrQuit(error == kErrorParse);

    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoDstOpts, *dstOptsDatagram, dstOptsHeader.GetSize(),
                                dstOptsDatagram->GetLength() - dstOptsHeader.GetSize(), Message::kOriginHostUntrusted,
                                /* aLinkSecurity */ true));

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // With the filter disabled, fragments containing the UDP header
    // are reassembled and the TMF message is processed.

    node.Get<Ip6::Ip6>().SetTmfOriginFilterEnabled(false);

    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoUdp, *datagram, 0, kFirstFragmentLength,
                                Message::kOriginHostUntrusted, /* aLinkSecurity */ true));
    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoUdp, *datagram, kFirstFragmentLength,
                                datagram->GetLength() - kFirstFragmentLength, Message::kOriginHostUntrusted,
                                /* aLinkSecurity */ true));

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SuccessOrQuit(node.Get<ThreadNetif>().AddExternalUnicastAddress(eid));

    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoDstOpts, *dstOptsDatagram, 0, dstOptsFirstFragmentLength,
                                Message::kOriginHostUntrusted, /* aLinkSecurity */ true));
    IgnoreError(SendRawFragment(node, rloc, mlEid, Ip6::kProtoDstOpts, *dstOptsDatagram, dstOptsFirstFragmentLength,
                                dstOptsDatagram->GetLength() - dstOptsFirstFragmentLength,
                                Message::kOriginHostUntrusted, /* aLinkSecurity */ true));

    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    dstOptsDatagram->Free();
    datagram->Free();

    Log("Verified that untrusted raw TMF fragments were dropped successfully.");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestTmfOrigin();
    ot::Nexus::TestTmfOriginBypassed();
    ot::Nexus::TestTmfOriginLocalReceive();
    ot::Nexus::TestTmfOriginMplRetransmission();
    ot::Nexus::TestTmfOriginLargerScopeMulticast();
    ot::Nexus::TestTmfOriginFragmented();
    ot::Nexus::TestTmfOriginReassembledLinkInfo();
    ot::Nexus::TestTmfOriginReassemblyMatch();
    ot::Nexus::TestTmfOriginFragmentedDatagramOrigin();
    ot::Nexus::TestTmfOriginRawFragments();
    printf("All tests passed\n");
    return 0;
}
