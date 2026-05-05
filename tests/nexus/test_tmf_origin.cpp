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

namespace ot {
namespace Nexus {

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
    Ip6::Udp::Header udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(1234);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(sizeof(Ip6::Udp::Header));

    // 2. Inner IPv6 Header
    Ip6::Header innerHeader;
    innerHeader.Clear();
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetSource(Ip6::Address::GetLinkLocalAllNodesMulticast()); // dummy LL source
    innerHeader.SetDestination(linkLocal);
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetPayloadLength(sizeof(Ip6::Udp::Header));

    // 3. Outer IPv6 Header (IP-in-IP)
    Ip6::Header outerHeader;
    outerHeader.Clear();
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetSource(linkLocal);
    outerHeader.SetDestination(mlEid);
    outerHeader.SetNextHeader(Ip6::kProtoIp6);
    outerHeader.SetPayloadLength(sizeof(Ip6::Header) + sizeof(Ip6::Udp::Header));

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
    Ip6::Udp::Header udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(1234);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(sizeof(Ip6::Udp::Header));

    // 2. Inner IPv6 Header
    Ip6::Header innerHeader;
    innerHeader.Clear();
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetSource(Ip6::Address::GetLinkLocalAllNodesMulticast()); // dummy LL source
    innerHeader.SetDestination(linkLocal);
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetPayloadLength(sizeof(Ip6::Udp::Header));

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
    outerHeader.SetPayloadLength(dstHeader.GetSize() + sizeof(Ip6::Header) + sizeof(Ip6::Udp::Header));

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

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestTmfOrigin();
    ot::Nexus::TestTmfOriginBypassed();
    printf("All tests passed\n");
    return 0;
}
