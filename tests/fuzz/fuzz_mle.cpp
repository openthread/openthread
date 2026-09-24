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
 *  MLE (Mesh Link Establishment) message parser fuzz harness (added for
 *  security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  MLE is the protocol a node uses to join and maintain the Thread mesh; its
 *  command handlers walk attacker-supplied TLVs (Challenge/Response, LeaderData,
 *  Connectivity, Route, TlvRequest, Address Registration, ...) received from a
 *  neighbor over UDP port 19788. These parsers are attacker-reachable but not
 *  covered by OSS-Fuzz: the `ip6` target would have to synthesize a full
 *  IPv6/UDP packet addressed to port 19788 with a valid checksum to reach them,
 *  which essentially never happens under mutation.
 *
 *  This harness forms a one-node network (so the MLE socket is open and the node
 *  is attached), wraps the fuzz bytes as the UDP payload of a well-formed
 *  IPv6/UDP datagram addressed to the MLE port, and delivers it through the real
 *  receive path `Ip6::HandleDatagram` -> `Mle::HandleUdpReceive`. The IPv6
 *  header fields are set to satisfy the MLE receive preconditions (thread-netif
 *  origin, link-local peer/sock addresses, MLE hop limit). Under the standard
 *  fuzzing build (`FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION`) MLE decryption is
 *  skipped, so the secured-command TLV parsers become reachable from raw input.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/checksum.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxPayload = 1024;

    // Need at least the security-suite byte; cap the UDP payload.
    if (size < 1 || size > kMaxPayload)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    // Destination: the node's own link-local address (satisfies the MLE
    // `GetSockAddr().IsLinkLocalUnicastOrMulticast()` check and makes the IPv6
    // stack accept the datagram for local delivery).
    Ip6::Address dst = node.Get<Mle::Mle>().GetLinkLocalAddress();

    // Source: a synthetic link-local unicast address of an unknown neighbor
    // (satisfies `GetPeerAddr().IsLinkLocalUnicast()`; being unknown, the frame
    // counter / key sequence checks are skipped and parsing proceeds).
    Ip6::Address src;
    src.Clear();
    src.mFields.m8[0]  = 0xfe;
    src.mFields.m8[1]  = 0x80;
    src.mFields.m8[15] = 0x02;

    Message *message = node.Get<Ip6::Ip6>().NewMessage();
    VerifyOrQuit(message != nullptr);

    Ip6::Header ip6Header;
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(255); // MLE hop limit (`Mle::kMleHopLimit`); required by the MLE receive path.
    ip6Header.SetSource(src);
    ip6Header.SetDestination(dst);

    Ip6::UdpHeader udpHeader;
    udpHeader.SetSourcePort(12345);
    udpHeader.SetDestinationPort(Mle::kUdpPort);
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    udpHeader.SetChecksum(0);

    if ((message->Append(ip6Header) != kErrorNone) || (message->Append(udpHeader) != kErrorNone) ||
        (message->AppendBytes(data, static_cast<uint16_t>(size)) != kErrorNone))
    {
        message->Free();
        return 0;
    }

    // Compute the UDP checksum over the pseudo-header + UDP header + payload
    // (the receive path drops datagrams with a bad checksum). The offset must
    // point at the start of the UDP header while updating, then be reset so
    // `HandleDatagram` reads the IPv6 header at offset zero.
    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, src, dst, Ip6::kProtoUdp);
    message->SetOffset(0);

    // Mark the datagram as arriving from the Thread interface (required by the
    // MLE receive path's origin check).
    message->SetOrigin(Message::kOriginThreadNetif);

    IgnoreError(node.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));

    nexus.AdvanceTime(10 * 1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
