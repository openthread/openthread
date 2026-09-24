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
 *  Multicast Listener Registration (MLR) TLV parser fuzz harness (added for
 *  security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  A primary Backbone Router parses an MLR.req (`n/mr`) sent by any node on the
 *  mesh: `Manager::HandleMulticastListenerRegistration` walks an
 *  `Ip6AddressesTlv` address list plus Commissioner-Session-Id / Timeout TLVs
 *  and drives the multicast listener table. This is attacker-reachable TLV
 *  parsing not covered by OSS-Fuzz nor by the other harnesses.
 *
 *  The harness forms a node, enables the Backbone Router and lets it become
 *  primary, then hand-builds a valid Confirmable CoAP POST to `n/mr` and
 *  appends the fuzz bytes as the payload (the registration TLVs), delivered to
 *  the TMF port via `Ip6::HandleDatagram` -> `Manager::HandleTmf<kUriMlr>`.
 *  The only TMF gate is mesh-local addressing + the link-security metadata flag
 *  (no DTLS on this path).
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "backbone_router/bbr_local.hpp"
#include "coap/coap.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxPayload = 1024;

    if (size > kMaxPayload)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    // Bring the Backbone Router up and let it become primary so the MLR handler
    // parses the registration (it early-exits when not primary).
    node.Get<BackboneRouter::Local>().SetEnabled(true);
    node.Get<BackboneRouter::Local>().SetRegistrationJitter(1);
    nexus.AdvanceTime(90 * 1000);
    VerifyOrQuit(node.Get<BackboneRouter::Local>().IsPrimary());

    // Build a Confirmable CoAP POST to the MLR URI ("n/mr") with the fuzz bytes
    // as the payload (the registration TLVs).
    Coap::Message *coap = node.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(coap != nullptr);

    if ((coap->Init(Coap::kTypeConfirmable, Coap::kCodePost) != kErrorNone) ||
        (coap->AppendUriPathOptions("n/mr") != kErrorNone) || (coap->AppendPayloadMarker() != kErrorNone) ||
        ((size > 0) && (coap->AppendBytes(data, static_cast<uint16_t>(size)) != kErrorNone)))
    {
        coap->Free();
        return 0;
    }

    uint16_t     coapLength = coap->GetLength();
    Ip6::Address addr       = node.Get<Mle::Mle>().GetMeshLocalEid();

    Message *message = node.Get<Ip6::Ip6>().NewMessage();
    VerifyOrQuit(message != nullptr);

    Ip6::Header ip6Header;
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + coapLength));
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(64);
    ip6Header.SetSource(addr);
    ip6Header.SetDestination(addr);

    Ip6::UdpHeader udpHeader;
    udpHeader.SetSourcePort(12345);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + coapLength));
    udpHeader.SetChecksum(0);

    if ((message->Append(ip6Header) != kErrorNone) || (message->Append(udpHeader) != kErrorNone) ||
        (message->AppendBytesFromMessage(*coap, 0, coapLength) != kErrorNone))
    {
        message->Free();
        coap->Free();
        return 0;
    }

    coap->Free();

    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, addr, addr, Ip6::kProtoUdp);
    message->SetOffset(0);

    message->SetOrigin(Message::kOriginThreadNetif);
    message->SetLinkSecurityEnabled(true);

    IgnoreError(node.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));

    nexus.AdvanceTime(10 * 1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
