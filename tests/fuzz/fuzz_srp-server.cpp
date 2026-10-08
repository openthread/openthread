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
 *  SRP server DNS Update message parser fuzz harness (added for security
 *  research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  The SRP server parses DNS Update messages that any SRP client on the Thread
 *  mesh sends to it over UDP (`Server::ProcessMessage` -> `ProcessDnsUpdate` ->
 *  zone/update/additional sections, including DNS name decompression and
 *  resource-record parsing). This is attacker-reachable, security-relevant
 *  parsing that is not covered by OSS-Fuzz.
 *
 *  This harness brings up a Border Router node with the SRP server running,
 *  wraps the fuzz bytes as the UDP payload (the DNS message, starting at the DNS
 *  header) of a well-formed IPv6/UDP datagram addressed to the SRP server port,
 *  and delivers it through the real receive path `Ip6::HandleDatagram` ->
 *  `Srp::Server::HandleUdpReceive` -> `ProcessMessage`.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/checksum.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"
#include "net/srp_server.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxPayload = 1024;

    if (size < 1 || size > kMaxPayload)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Get<BorderRouter::InfraIf>().Init(/* aInfraIfIndex */ 1, /* aInfraIfIsRunning */ true);
    SuccessOrQuit(node.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    node.Get<Srp::Server>().SetAutoEnableMode(true);

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    // The SRP server listens on the mesh-local EID; deliver the datagram there.
    Ip6::Address addr = node.Get<Mle::Mle>().GetMeshLocalEid();

    Message *message = node.Get<Ip6::Ip6>().NewMessage();
    VerifyOrQuit(message != nullptr);

    Ip6::Header ip6Header;
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(64);
    ip6Header.SetSource(addr);
    ip6Header.SetDestination(addr);

    Ip6::UdpHeader udpHeader;
    udpHeader.SetSourcePort(12345);
    udpHeader.SetDestinationPort(node.Get<Srp::Server>().GetPort());
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    udpHeader.SetChecksum(0);

    if ((message->Append(ip6Header) != kErrorNone) || (message->Append(udpHeader) != kErrorNone) ||
        (message->AppendBytes(data, static_cast<uint16_t>(size)) != kErrorNone))
    {
        message->Free();
        return 0;
    }

    // Compute the UDP checksum (bad-checksum datagrams are dropped on receive).
    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, addr, addr, Ip6::kProtoUdp);
    message->SetOffset(0);

    message->SetOrigin(Message::kOriginThreadNetif);

    IgnoreError(node.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));

    nexus.AdvanceTime(10 * 1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
