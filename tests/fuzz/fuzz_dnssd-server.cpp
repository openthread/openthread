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
 *  DNS-SD server DNS query parser fuzz harness (added for security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  The DNS-SD server answers DNS queries sent by any client on the mesh:
 *  `Server::HandleUdpReceive` -> `Server::ProcessQuery` parses the DNS question
 *  section (name decompression, query type/class) and drives service/host
 *  resolution. This is attacker-reachable DNS query parsing distinct from the
 *  SRP server's DNS *update* parsing (already fuzzed by `srp-server`) and not
 *  covered by OSS-Fuzz.
 *
 *  The harness forms a node, starts the DNS-SD server (binding its UDP port),
 *  wraps the fuzz bytes as the UDP payload (the DNS message, starting at the DNS
 *  header) of a well-formed IPv6/UDP datagram addressed to the server port, and
 *  delivers it through the real receive path `Ip6::HandleDatagram` ->
 *  `Server::HandleUdpReceive` -> `ProcessQuery`.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/checksum.hpp"
#include "net/dnssd_server.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"

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

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node.Get<Dns::ServiceDiscovery::Server>().Start());

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
    udpHeader.SetDestinationPort(Dns::ServiceDiscovery::Server::kPort);
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    udpHeader.SetChecksum(0);

    if ((message->Append(ip6Header) != kErrorNone) || (message->Append(udpHeader) != kErrorNone) ||
        (message->AppendBytes(data, static_cast<uint16_t>(size)) != kErrorNone))
    {
        message->Free();
        return 0;
    }

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
