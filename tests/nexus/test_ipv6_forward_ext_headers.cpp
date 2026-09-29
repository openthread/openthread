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
 * Verifies that the forward-path protocol checks in `Ip6::HandleDatagram()` (the
 * forwarded-ICMPv6 type allowlist and the untrusted-origin TMF port filter) are applied
 * to the final upper-layer protocol after resolving any IPv6 extension headers, rather
 * than to the first Next Header value of the IPv6 header.
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
 *  case 10: TCP with no extension header is forwarded.
 */

#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kNonTmfPort   = 12345;
static constexpr uint16_t kSourcePort   = 1234;
static constexpr uint8_t  kHopLimit     = 64;
static constexpr uint8_t  kLongChainLen = 9; // Longer than the resolution limit (8).

static Node        *sNode;
static Ip6::Address sSource;
static Ip6::Address sDestination;

static Message &NewForwardedMessage(uint8_t aNextHeader, uint16_t aPayloadLength)
{
    // Allocates a new IPv6 message and appends an IPv6 header with a
    // link-local destination that is not on this node, so that the
    // message takes the forward-to-Thread path in `HandleDatagram()`.

    Message    *message = sNode->Get<MessagePool>().Allocate(Message::kTypeIp6);
    Ip6::Header header;

    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.InitVersionTrafficClassFlow();
    header.SetSource(sSource);
    header.SetDestination(sDestination);
    header.SetNextHeader(aNextHeader);
    header.SetPayloadLength(aPayloadLength);
    header.SetHopLimit(kHopLimit);

    SuccessOrQuit(message->Append(header));

    return *message;
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

static void AppendFragmentHeader(Message &aMessage, uint8_t aNextHeader, uint16_t aOffset)
{
    Ip6::FragmentHeader fragHeader;

    fragHeader.Init();
    fragHeader.SetNextHeader(aNextHeader);
    fragHeader.SetOffset(aOffset);

    SuccessOrQuit(aMessage.Append(fragHeader));
}

static void AppendUdpHeader(Message &aMessage, uint16_t aDestPort)
{
    Ip6::UdpHeader udpHeader;

    udpHeader.Clear();
    udpHeader.SetSourcePort(kSourcePort);
    udpHeader.SetDestinationPort(aDestPort);
    udpHeader.SetLength(sizeof(Ip6::UdpHeader));

    SuccessOrQuit(aMessage.Append(udpHeader));
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

void TestIp6ForwardExtHeaders(void)
{
    Core  nexus;
    Node &node = nexus.CreateNode();
    Error error;

    node.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    sNode   = &node;
    sSource = node.Get<Mle::Mle>().GetLinkLocalAddress();

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
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIp6ForwardExtHeaders();
    printf("All tests passed\n");
    return 0;
}
