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

/*
 * Verifies that the protocol checks in `Ip6::HandleDatagram()` (the forwarded-ICMPv6 type
 * allowlist and the untrusted-origin TMF port filter) are applied to the final upper-layer
 * protocol after resolving any IPv6 extension headers, rather than to the first Next
 * Header value of the IPv6 header.
 *
 * Each case constructs a raw IPv6 datagram with a host-untrusted origin and a link-local
 * destination that is not on this node, so that `Ip6::SendRaw()` routes it through the
 * forward-to-Thread path of `Ip6::HandleDatagram()`, and asserts on the returned error:
 *
 *  case 1 (control): UDP to the TMF port with no extension header is dropped;
 *  case 2: the same UDP datagram with a Destination Options header preceding the UDP
 *          header is also dropped;
 *  case 3: the same UDP datagram with an atomic Fragment header (offset 0) preceding
 *          the UDP header is also dropped;
 *  case 4: ICMPv6 Echo Request with a Destination Options header is still matched by
 *          the forwarded-ICMPv6 type allowlist and forwarded;
 *  case 5: ICMPv6 Neighbor Solicitation (not in the allowlist) with a Destination
 *          Options header is dropped;
 *  case 6: a chain of extension headers longer than the resolution limit is dropped
 *          (the resolution fails closed);
 *  case 7: a continuation fragment (offset != 0), which does not contain the
 *          upper-layer header, is forwarded as-is;
 *  case 8: UDP to a non-TMF port with no extension header is forwarded;
 *  case 9: UDP to a non-TMF port with a Destination Options header is forwarded;
 *  case 10: TCP with no extension header is forwarded;
 *  case 11: Host -> Thread UDP to mDNS destination port (5353), no extension header -> dropped;
 *  case 12: Host -> Thread UDP to mDNS destination port (5353), with Destination Options header -> dropped;
 *  case 13: Host -> Thread UDP from mDNS source port (5353), no extension header -> dropped;
 *  case 14: Host -> Thread UDP from mDNS source port (5353), with Destination Options header -> dropped;
 *  case 15: Host -> Thread multicast mDNS (`ff03::fb`, `ff05::fb`, and IP-in-IP encapsulated) -> dropped;
 *  case 16: Internal `Ip6::SendDatagram()` UDP to/from mDNS port (5353) -> dropped (non-mDNS forwarded);
 *  case 17: Thread netif -> Host non-mDNS UDP (unicast and `ff02::1234` multicast) with valid checksum -> allowed;
 *  case 18: Thread netif -> Host UDP to mDNS destination port (5353) (unicast, DstOpts, and `ff02::fb`) -> dropped;
 *  case 19: Thread netif -> Host UDP from mDNS source port (5353) (unicast and `ff02::fb`) -> dropped;
 *  case 20: Thread netif IP-in-IP encapsulated mDNS -> dropped;
 *  case 21: Thread netif UDP with truncated header (< sizeof(UdpHeader)) -> dropped;
 *  case 22: Thread netif multicast mDNS with Hop-by-Hop MPL option -> dropped without MPL buffering;
 *  case 23: Fragmented mDNS and first fragments without full UdpHeader -> dropped or parse error;
 *  case 24: Thread netif doubly-encapsulated IP-in-IP mDNS -> dropped without host delivery or MPL buffering;
 *  case 25 (control): Thread netif non-mDNS multicast with Hop-by-Hop MPL option -> allowed and buffered by MPL.
 */

#include <stdio.h>

#include "net/checksum.hpp"
#include "net/dns_types.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kNonTmfPort      = 12345;
static constexpr uint16_t kSourcePort      = 1234;
static constexpr uint8_t  kHopLimit        = 64;
static constexpr uint8_t  kLongChainLen    = 9; // Longer than the resolution limit (8).
static constexpr uint16_t kMplSeedId       = 0x1234;
static constexpr uint16_t kHopByHopMplSize = sizeof(Ip6::HopByHopHeader) + sizeof(Ip6::MplOption);

static Node        *sNode;
static Ip6::Address sSource;
static Ip6::Address sDestination;
static uint16_t     sNumHostRx = 0;

static void HandleHostIp6Receive(otMessage *aMessage, void *aContext)
{
    sNumHostRx++;
    Node::HandleIp6Receive(aMessage, aContext);
}

static uint16_t GetMplBufferedMessageCount(void)
{
    MessageQueue::Info info;

    sNode->Get<Ip6::Mpl>().GetBufferedMessageSetInfo(info);

    return info.mNumMessages;
}

static void AppendIp6Header(Message            &aMessage,
                            const Ip6::Address &aSource,
                            const Ip6::Address &aDestination,
                            uint8_t             aNextHeader,
                            uint16_t            aPayloadLength)
{
    Ip6::Header header;

    header.Clear();
    header.InitVersionTrafficClassFlow();
    header.SetSource(aSource);
    header.SetDestination(aDestination);
    header.SetNextHeader(aNextHeader);
    header.SetPayloadLength(aPayloadLength);
    header.SetHopLimit(kHopLimit);

    SuccessOrQuit(aMessage.Append(header));
}

static Message &NewForwardedMessage(const Ip6::Address &aDestination, uint8_t aNextHeader, uint16_t aPayloadLength)
{
    Message *message = sNode->Get<MessagePool>().Allocate(Message::kTypeIp6);

    VerifyOrQuit(message != nullptr);
    AppendIp6Header(*message, sSource, aDestination, aNextHeader, aPayloadLength);

    return *message;
}

static Message &NewForwardedMessage(uint8_t aNextHeader, uint16_t aPayloadLength)
{
    // Allocates a new IPv6 message and appends an IPv6 header with a
    // link-local destination that is not on this node, so that the
    // message takes the forward-to-Thread path in `HandleDatagram()`.

    return NewForwardedMessage(sDestination, aNextHeader, aPayloadLength);
}

static void AppendDstOptsHeader(Message &aMessage, uint8_t aNextHeader)
{
    // Appends a minimal (8-byte) Destination Options header carrying a
    // single PadN option.

    Ip6::ExtensionHeader extHeader;
    Ip6::PadOption       padOption;

    extHeader.SetNextHeader(aNextHeader);
    extHeader.SetLength(0);
    padOption.InitForPadSize(6);

    SuccessOrQuit(aMessage.Append(extHeader));
    SuccessOrQuit(aMessage.AppendBytes(&padOption, padOption.GetSize()));
}

static void AppendHopByHopMplHeader(Message &aMessage, uint8_t aNextHeader, uint16_t aSeedId, uint8_t aSequence)
{
    Ip6::HopByHopHeader hbhHeader;
    Ip6::MplOption      mplOption;

    hbhHeader.SetNextHeader(aNextHeader);
    hbhHeader.SetLength(0);
    mplOption.Init(Ip6::MplOption::kSeedIdLength2);
    mplOption.SetSeedId(aSeedId);
    mplOption.SetSequence(aSequence);
    VerifyOrQuit(sizeof(hbhHeader) + mplOption.GetSize() == hbhHeader.GetSize());

    SuccessOrQuit(aMessage.Append(hbhHeader));
    SuccessOrQuit(aMessage.AppendBytes(&mplOption, mplOption.GetSize()));
}

static void AppendFragmentHeader(Message &aMessage, uint8_t aNextHeader, uint16_t aOffset, bool aMoreFragments = false)
{
    Ip6::FragmentHeader fragHeader;

    fragHeader.Init();
    fragHeader.SetNextHeader(aNextHeader);
    fragHeader.SetOffset(aOffset);

    if (aMoreFragments)
    {
        fragHeader.SetMoreFlag();
    }

    SuccessOrQuit(aMessage.Append(fragHeader));
}

static void AppendUdpHeader(Message &aMessage, uint16_t aSourcePort, uint16_t aDestPort)
{
    Ip6::UdpHeader udpHeader;

    udpHeader.Clear();
    udpHeader.SetSourcePort(aSourcePort);
    udpHeader.SetDestinationPort(aDestPort);
    udpHeader.SetLength(sizeof(Ip6::UdpHeader));

    SuccessOrQuit(aMessage.Append(udpHeader));
}

static void AppendUdpHeader(Message &aMessage, uint16_t aDestPort)
{
    AppendUdpHeader(aMessage, kSourcePort, aDestPort);
}

static void AppendIcmp6Header(Message &aMessage, Ip6::Icmp6Header::Type aType)
{
    Ip6::Icmp6Header icmp6Header;

    icmp6Header.Clear();
    icmp6Header.SetType(aType);

    SuccessOrQuit(aMessage.Append(icmp6Header));
}

static Error SendFromHost(Message &aMessage)
{
    // Marks the message as host-untrusted origin and passes it to
    // `SendRaw()`, which routes it through `HandleDatagram()`.

    aMessage.SetOrigin(Message::kOriginHostUntrusted);

    return sNode->Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(&aMessage));
}

static Message &NewThreadNetifUdpMessage(const Ip6::Address &aSource,
                                         const Ip6::Address &aDestination,
                                         bool                aWithDstOpts,
                                         uint16_t            aSourcePort,
                                         uint16_t            aDestPort)
{
    uint16_t    extOptSize = aWithDstOpts ? (sizeof(Ip6::ExtensionHeader) + 6) : 0;
    Message    *message    = sNode->Get<MessagePool>().Allocate(Message::kTypeIp6);
    Ip6::Header header;

    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.InitVersionTrafficClassFlow();
    header.SetSource(aSource);
    header.SetDestination(aDestination);
    header.SetNextHeader(aWithDstOpts ? Ip6::kProtoDstOpts : Ip6::kProtoUdp);
    header.SetPayloadLength(extOptSize + sizeof(Ip6::UdpHeader));
    header.SetHopLimit(kHopLimit);

    SuccessOrQuit(message->Append(header));

    if (aWithDstOpts)
    {
        AppendDstOptsHeader(*message, Ip6::kProtoUdp);
    }

    AppendUdpHeader(*message, aSourcePort, aDestPort);

    message->SetOffset(sizeof(Ip6::Header) + extOptSize);
    Checksum::UpdateMessageChecksum(*message, aSource, aDestination, Ip6::kProtoUdp);
    message->SetOffset(0);
    message->SetOrigin(Message::kOriginThreadNetif);

    return *message;
}

static Message &NewIpInIpUdpMessage(const Ip6::Address &aSource,
                                    const Ip6::Address &aOuterDest,
                                    const Ip6::Address &aInnerDest,
                                    uint16_t            aSourcePort,
                                    uint16_t            aDestPort,
                                    Message::Origin     aOrigin)
{
    Message    &message = NewForwardedMessage(aOuterDest, Ip6::kProtoIp6, sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
    Ip6::Header innerHeader;

    innerHeader.Clear();
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetSource(aSource);
    innerHeader.SetDestination(aInnerDest);
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetPayloadLength(sizeof(Ip6::UdpHeader));
    innerHeader.SetHopLimit(kHopLimit);

    SuccessOrQuit(message.Append(innerHeader));
    AppendUdpHeader(message, aSourcePort, aDestPort);

    message.SetOffset(2 * sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(message, aSource, aInnerDest, Ip6::kProtoUdp);
    message.SetOffset(0);
    message.SetOrigin(aOrigin);

    return message;
}

void TestIp6ForwardExtHeaders(void)
{
    Core  nexus;
    Node &node = nexus.CreateNode();
    Error error;

    node.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    sNode = &node;
    node.Get<Ip6::Ip6>().SetReceiveCallback(HandleHostIp6Receive, &node);
    sNumHostRx = 0;
    sSource    = node.Get<Mle::Mle>().GetLinkLocalAddress();

    // A link-local destination that is not on this node.
    sDestination = node.Get<Mle::Mle>().GetLinkLocalAddress();
    sDestination.mFields.m8[15] ^= 0x01;

    Log("--- Case 1 (control): UDP to TMF port, no extension header -> dropped");
    {
        Message &message = NewForwardedMessage(Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));

        AppendUdpHeader(message, Tmf::kUdpPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 2: UDP to TMF port behind Destination Options header -> dropped");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::UdpHeader));

        AppendDstOptsHeader(message, Ip6::kProtoUdp);
        AppendUdpHeader(message, Tmf::kUdpPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 3: UDP to TMF port behind atomic Fragment header -> dropped");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoFragment, sizeof(Ip6::FragmentHeader) + sizeof(Ip6::UdpHeader));

        AppendFragmentHeader(message, Ip6::kProtoUdp, /* aOffset */ 0);
        AppendUdpHeader(message, Tmf::kUdpPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 4: ICMPv6 Echo Request behind Destination Options header -> forwarded");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::Icmp6Header));

        AppendDstOptsHeader(message, Ip6::kProtoIcmp6);
        AppendIcmp6Header(message, Ip6::Icmp6Header::kTypeEchoRequest);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorNone);
    }

    Log("--- Case 5: ICMPv6 Neighbor Solicit behind Destination Options header -> dropped");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::Icmp6Header));

        AppendDstOptsHeader(message, Ip6::kProtoIcmp6);
        AppendIcmp6Header(message, Ip6::Icmp6Header::kTypeNeighborSolicit);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 6: extension header chain longer than resolution limit -> dropped");
    {
        uint16_t payloadLength = kLongChainLen * (sizeof(Ip6::ExtensionHeader) + 6) + sizeof(Ip6::UdpHeader);
        Message &message       = NewForwardedMessage(Ip6::kProtoDstOpts, payloadLength);

        for (uint8_t i = 0; i < kLongChainLen; i++)
        {
            AppendDstOptsHeader(message, (i + 1 < kLongChainLen) ? Ip6::kProtoDstOpts : Ip6::kProtoUdp);
        }

        AppendUdpHeader(message, kNonTmfPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error != kErrorNone);
    }

    Log("--- Case 7: continuation fragment (no upper-layer header present) -> forwarded");
    {
        static const uint8_t kFragmentData[16] = {0};

        Message &message =
            NewForwardedMessage(Ip6::kProtoFragment, sizeof(Ip6::FragmentHeader) + sizeof(kFragmentData));

        AppendFragmentHeader(message, Ip6::kProtoUdp, /* aOffset */ 64);
        SuccessOrQuit(message.AppendBytes(kFragmentData, sizeof(kFragmentData)));
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorNone);
    }

    Log("--- Case 8: UDP to non-TMF port, no extension header -> forwarded");
    {
        Message &message = NewForwardedMessage(Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));

        AppendUdpHeader(message, kNonTmfPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorNone);
    }

    Log("--- Case 9: UDP to non-TMF port behind Destination Options header -> forwarded");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::UdpHeader));

        AppendDstOptsHeader(message, Ip6::kProtoUdp);
        AppendUdpHeader(message, kNonTmfPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorNone);
    }

    Log("--- Case 10: TCP, no extension header -> forwarded");
    {
        static const uint8_t kTcpHeader[20] = {0};

        Message &message = NewForwardedMessage(Ip6::kProtoTcp, sizeof(kTcpHeader));

        SuccessOrQuit(message.AppendBytes(kTcpHeader, sizeof(kTcpHeader)));
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorNone);
    }

    Log("--- Case 11: UDP to mDNS destination port (5353), no extension header -> dropped");
    {
        Message &message = NewForwardedMessage(Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));

        AppendUdpHeader(message, Dns::kMdnsPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 12: UDP to mDNS destination port (5353) behind Destination Options header -> dropped");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::UdpHeader));

        AppendDstOptsHeader(message, Ip6::kProtoUdp);
        AppendUdpHeader(message, Dns::kMdnsPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 13: UDP from mDNS source port (5353), no extension header -> dropped");
    {
        Message &message = NewForwardedMessage(Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));

        AppendUdpHeader(message, Dns::kMdnsPort, kNonTmfPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 14: UDP from mDNS source port (5353) behind Destination Options header -> dropped");
    {
        Message &message =
            NewForwardedMessage(Ip6::kProtoDstOpts, sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::UdpHeader));

        AppendDstOptsHeader(message, Ip6::kProtoUdp);
        AppendUdpHeader(message, Dns::kMdnsPort, kNonTmfPort);
        error = SendFromHost(message);
        Log("SendRaw returned: %s", ErrorToString(error));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 15: Host -> Thread multicast mDNS -> dropped");
    {
        Ip6::Address realmMcast;
        Ip6::Address siteMcast;
        Ip6::Address mplOuterDest;
        Message     *message;

        SuccessOrQuit(realmMcast.FromString("ff03::fb"));
        SuccessOrQuit(siteMcast.FromString("ff05::fb"));
        SuccessOrQuit(mplOuterDest.FromString("ff03::fc"));

        message = &NewForwardedMessage(realmMcast, Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));
        AppendUdpHeader(*message, Dns::kMdnsPort);
        error = SendFromHost(*message);
        VerifyOrQuit(error == kErrorDrop);

        message = &NewForwardedMessage(siteMcast, Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));
        AppendUdpHeader(*message, Dns::kMdnsPort);
        error = SendFromHost(*message);
        VerifyOrQuit(error == kErrorDrop);

        message = &NewIpInIpUdpMessage(sSource, mplOuterDest, siteMcast, kSourcePort, Dns::kMdnsPort,
                                       Message::kOriginHostUntrusted);
        error   = sNode->Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
    }

    Log("--- Case 16: Internal SendDatagram UDP to/from mDNS port (5353) -> dropped");
    {
        Ip6::MessageInfo messageInfo;

        messageInfo.SetSockAddr(sSource);
        messageInfo.SetPeerAddr(sDestination);

        {
            OwnedPtr<Message> message(sNode->Get<Ip6::Ip6>().NewMessage());

            VerifyOrQuit(message != nullptr);
            AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
            error = sNode->Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoUdp);
            VerifyOrQuit(error == kErrorDrop);
        }

        {
            OwnedPtr<Message> message(sNode->Get<Ip6::Ip6>().NewMessage());

            VerifyOrQuit(message != nullptr);
            AppendUdpHeader(*message, Dns::kMdnsPort, kNonTmfPort);
            error = sNode->Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoUdp);
            VerifyOrQuit(error == kErrorDrop);
        }

        {
            OwnedPtr<Message> message(sNode->Get<Ip6::Ip6>().NewMessage());

            VerifyOrQuit(message != nullptr);
            AppendUdpHeader(*message, kSourcePort, kNonTmfPort);
            error = sNode->Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoUdp);
            VerifyOrQuit(error == kErrorNone);
            message.Release();
        }
    }

    Log("--- Case 17 (control): Thread Netif non-mDNS UDP with valid checksum -> allowed");
    {
        Ip6::Address nonMdnsMcast;
        Message     *message;

        SuccessOrQuit(nonMdnsMcast.FromString("ff02::1234"));

        message = &NewThreadNetifUdpMessage(sDestination, sSource, /* aWithDstOpts */ false, kSourcePort, kNonTmfPort);
        error   = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorNone);

        sNumHostRx = 0;
        message =
            &NewThreadNetifUdpMessage(sDestination, nonMdnsMcast, /* aWithDstOpts */ false, kSourcePort, kNonTmfPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorNone);
        VerifyOrQuit(sNumHostRx == 1);
    }

    Log("--- Case 18: Thread Netif UDP to mDNS destination port (5353) -> dropped");
    {
        Ip6::Address mdnsMcast;
        Message     *message;

        SuccessOrQuit(mdnsMcast.FromString("ff02::fb"));
        sNumHostRx = 0;

        message =
            &NewThreadNetifUdpMessage(sDestination, sSource, /* aWithDstOpts */ false, kSourcePort, Dns::kMdnsPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);

        message =
            &NewThreadNetifUdpMessage(sDestination, sSource, /* aWithDstOpts */ true, kSourcePort, Dns::kMdnsPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);

        message =
            &NewThreadNetifUdpMessage(sDestination, mdnsMcast, /* aWithDstOpts */ false, kSourcePort, Dns::kMdnsPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 19: Thread Netif UDP from mDNS source port (5353) -> dropped");
    {
        Ip6::Address mdnsMcast;
        Message     *message;

        SuccessOrQuit(mdnsMcast.FromString("ff02::fb"));
        sNumHostRx = 0;

        message =
            &NewThreadNetifUdpMessage(sDestination, sSource, /* aWithDstOpts */ false, Dns::kMdnsPort, kNonTmfPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);

        message =
            &NewThreadNetifUdpMessage(sDestination, mdnsMcast, /* aWithDstOpts */ false, Dns::kMdnsPort, kNonTmfPort);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 20: Thread Netif IP-in-IP encapsulated mDNS -> dropped");
    {
        Ip6::Address mplOuterDest;
        Ip6::Address siteMcast;
        Message     *message;

        SuccessOrQuit(mplOuterDest.FromString("ff03::fc"));
        SuccessOrQuit(siteMcast.FromString("ff05::fb"));
        sNumHostRx = 0;

        message = &NewIpInIpUdpMessage(sDestination, mplOuterDest, siteMcast, kSourcePort, Dns::kMdnsPort,
                                       Message::kOriginThreadNetif);
        error   = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 21: Thread Netif UDP with truncated header (< sizeof(UdpHeader)) -> dropped");
    {
        Message             *message = sNode->Get<MessagePool>().Allocate(Message::kTypeIp6);
        Ip6::Header          header;
        static const uint8_t kTruncatedUdp[4] = {0x12, 0x34, 0x14, 0xe9}; // 4 bytes only

        VerifyOrQuit(message != nullptr);
        sNumHostRx = 0;

        header.Clear();
        header.InitVersionTrafficClassFlow();
        header.SetSource(sDestination);
        header.SetDestination(sSource);
        header.SetNextHeader(Ip6::kProtoUdp);
        header.SetPayloadLength(sizeof(kTruncatedUdp));
        header.SetHopLimit(kHopLimit);

        SuccessOrQuit(message->Append(header));
        SuccessOrQuit(message->AppendBytes(kTruncatedUdp, sizeof(kTruncatedUdp)));
        message->SetOrigin(Message::kOriginThreadNetif);

        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error != kErrorNone);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 22: Thread Netif multicast mDNS with Hop-by-Hop MPL option -> dropped without MPL buffering");
    {
        Ip6::Address realmMcast;
        Ip6::Address siteMcast;
        Ip6::Address mplOuterDest;
        Message     *message;

        SuccessOrQuit(realmMcast.FromString("ff03::fb"));
        SuccessOrQuit(siteMcast.FromString("ff05::fb"));
        SuccessOrQuit(mplOuterDest.FromString("ff03::fc"));
        sNumHostRx = 0;
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        message = &NewForwardedMessage(realmMcast, Ip6::kProtoHopOpts, kHopByHopMplSize + sizeof(Ip6::UdpHeader));
        AppendHopByHopMplHeader(*message, Ip6::kProtoUdp, kMplSeedId, /* aSequence */ 1);
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        message = &NewForwardedMessage(mplOuterDest, Ip6::kProtoHopOpts,
                                       kHopByHopMplSize + sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
        AppendHopByHopMplHeader(*message, Ip6::kProtoIp6, kMplSeedId, /* aSequence */ 2);
        AppendIp6Header(*message, sSource, siteMcast, Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);
    }

    Log("--- Case 23: Fragmented mDNS and first fragments without full UdpHeader -> dropped / kErrorParse");
    {
        static const uint8_t kTrailingPayload[8] = {0};
        static const uint8_t kTruncatedUdp[4]    = {0x12, 0x34, 0x14, 0xe9};

        Ip6::Address realmMcast;
        Message     *message;

        SuccessOrQuit(realmMcast.FromString("ff03::fb"));
        sNumHostRx = 0;
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        // Host -> Thread first fragment (M = 1, offset = 0) carrying UDP port 5353 -> kErrorDrop.
        message = &NewForwardedMessage(Ip6::kProtoFragment,
                                       sizeof(Ip6::FragmentHeader) + sizeof(Ip6::UdpHeader) + sizeof(kTrailingPayload));
        AppendFragmentHeader(*message, Ip6::kProtoUdp, /* aOffset */ 0, /* aMoreFragments */ true);
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        SuccessOrQuit(message->AppendBytes(kTrailingPayload, sizeof(kTrailingPayload)));
        error = SendFromHost(*message);
        VerifyOrQuit(error == kErrorDrop);

        // Thread Netif first fragment (M = 1, offset = 0) carrying UDP port 5353 -> kErrorDrop.
        message = &NewForwardedMessage(sSource, Ip6::kProtoFragment,
                                       sizeof(Ip6::FragmentHeader) + sizeof(Ip6::UdpHeader) + sizeof(kTrailingPayload));
        AppendFragmentHeader(*message, Ip6::kProtoUdp, /* aOffset */ 0, /* aMoreFragments */ true);
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        SuccessOrQuit(message->AppendBytes(kTrailingPayload, sizeof(kTrailingPayload)));
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);

        // Thread Netif first fragment (M = 1, offset = 0) resolving to UDP with no UdpHeader -> kErrorParse.
        message = &NewForwardedMessage(sSource, Ip6::kProtoFragment, sizeof(Ip6::FragmentHeader));
        AppendFragmentHeader(*message, Ip6::kProtoUdp, /* aOffset */ 0, /* aMoreFragments */ true);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorParse);
        VerifyOrQuit(sNumHostRx == 0);

        // Thread Netif first fragment with HBH MPL + DstOpts-only (no UdpHeader) -> kErrorParse without MPL buffering.
        message =
            &NewForwardedMessage(realmMcast, Ip6::kProtoHopOpts,
                                 kHopByHopMplSize + sizeof(Ip6::FragmentHeader) + sizeof(Ip6::ExtensionHeader) + 6);
        AppendHopByHopMplHeader(*message, Ip6::kProtoFragment, kMplSeedId, /* aSequence */ 3);
        AppendFragmentHeader(*message, Ip6::kProtoDstOpts, /* aOffset */ 0, /* aMoreFragments */ true);
        AppendDstOptsHeader(*message, Ip6::kProtoUdp);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorParse);
        VerifyOrQuit(sNumHostRx == 0);
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        // Thread Netif first fragment with DstOpts + truncated UdpHeader -> kErrorParse.
        message = &NewForwardedMessage(sSource, Ip6::kProtoDstOpts,
                                       sizeof(Ip6::ExtensionHeader) + 6 + sizeof(Ip6::FragmentHeader) +
                                           sizeof(kTruncatedUdp));
        AppendDstOptsHeader(*message, Ip6::kProtoFragment);
        AppendFragmentHeader(*message, Ip6::kProtoUdp, /* aOffset */ 0, /* aMoreFragments */ true);
        SuccessOrQuit(message->AppendBytes(kTruncatedUdp, sizeof(kTruncatedUdp)));
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorParse);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 24: Thread Netif doubly-encapsulated IP-in-IP mDNS -> dropped without host delivery or MPL "
        "buffering");
    {
        Ip6::Address siteMcast;
        Ip6::Address mplOuterDest;
        Message     *message;

        SuccessOrQuit(siteMcast.FromString("ff05::fb"));
        SuccessOrQuit(mplOuterDest.FromString("ff03::fc"));
        sNumHostRx = 0;
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        // Multicast with HBH MPL -> IPv6 -> IPv6 -> mDNS UDP port 5353.
        message = &NewForwardedMessage(mplOuterDest, Ip6::kProtoHopOpts,
                                       kHopByHopMplSize + 2 * sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
        AppendHopByHopMplHeader(*message, Ip6::kProtoIp6, kMplSeedId, /* aSequence */ 4);
        AppendIp6Header(*message, sSource, mplOuterDest, Ip6::kProtoIp6, sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
        AppendIp6Header(*message, sSource, siteMcast, Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        // Unicast double tunnel on forward-only path (destination not on this node) -> dropped.
        message = &NewForwardedMessage(sDestination, Ip6::kProtoIp6, 2 * sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
        AppendIp6Header(*message, sSource, sDestination, Ip6::kProtoIp6, sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader));
        AppendIp6Header(*message, sSource, sDestination, Ip6::kProtoUdp, sizeof(Ip6::UdpHeader));
        AppendUdpHeader(*message, kSourcePort, Dns::kMdnsPort);
        message->SetOrigin(Message::kOriginThreadNetif);
        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        VerifyOrQuit(sNumHostRx == 0);
    }

    Log("--- Case 25 (control): Thread Netif non-mDNS multicast with Hop-by-Hop MPL option -> allowed and buffered");
    {
        Ip6::Address realmNonMdnsMcast;
        Message     *message;

        SuccessOrQuit(realmNonMdnsMcast.FromString("ff03::1234"));
        sNumHostRx = 0;
        VerifyOrQuit(GetMplBufferedMessageCount() == 0);

        message =
            &NewForwardedMessage(realmNonMdnsMcast, Ip6::kProtoHopOpts, kHopByHopMplSize + sizeof(Ip6::UdpHeader));
        AppendHopByHopMplHeader(*message, Ip6::kProtoUdp, kMplSeedId, /* aSequence */ 5);
        AppendUdpHeader(*message, kSourcePort, kNonTmfPort);
        message->SetOffset(sizeof(Ip6::Header) + kHopByHopMplSize);
        Checksum::UpdateMessageChecksum(*message, sSource, realmNonMdnsMcast, Ip6::kProtoUdp);
        message->SetOffset(0);
        message->SetOrigin(Message::kOriginThreadNetif);

        error = sNode->Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorNone);
        VerifyOrQuit(sNumHostRx == 1);
        VerifyOrQuit(GetMplBufferedMessageCount() == 1);
    }

    node.Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, &node);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIp6ForwardExtHeaders();
    printf("All tests passed\n");
    return 0;
}
