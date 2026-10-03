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
 *  CoAP header / option / block-wise parser fuzz harness (added for security
 *  research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  Every CoAP message received by the application CoAP endpoint is parsed by
 *  `Coap::Message::ParseHeaderAndOptions` -> `Option::Iterator::Advance`, whose
 *  option delta/length extension-field arithmetic (the +13 / +269 encodings) is
 *  a classic memory-safety surface, and by the block-wise reassembly path
 *  (`ProcessBlockwiseRequest` / `ProcessBlock1Request`) which copies attacker
 *  block payloads into a fixed buffer using block offset/size arithmetic across
 *  datagrams. The application CoAP receive path has no interceptor and no
 *  link-security gate, so it is reachable from raw bytes; it is not covered by
 *  OSS-Fuzz nor by the other harnesses.
 *
 *  This harness starts the application CoAP endpoint (with a block-wise resource
 *  registered so the block-wise path is reachable), wraps the fuzz bytes as the
 *  UDP payload (the CoAP message, starting at the CoAP header) of a well-formed
 *  IPv6/UDP datagram addressed to the CoAP port, and delivers it through the
 *  real receive path `Ip6::HandleDatagram` -> `Coap::HandleUdpReceive` ->
 *  `CoapBase::Receive` -> `ParseHeaderAndOptions`.
 */

#include <stddef.h>
#include <stdint.h>

#include <openthread/coap.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "coap/coap.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Nexus {

static void FuzzCoapHandler(void *, otMessage *, const otMessageInfo *) {}

static otError FuzzReceiveHook(void *, const uint8_t *, uint32_t, uint16_t, bool, uint32_t) { return OT_ERROR_NONE; }

static otError FuzzTransmitHook(void *, uint8_t *, uint32_t, uint16_t *aBlockLength, bool *aMore)
{
    if (aBlockLength != nullptr)
    {
        *aBlockLength = 0;
    }
    if (aMore != nullptr)
    {
        *aMore = false;
    }
    return OT_ERROR_NONE;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxPayload = 1280;

    if (size < 1 || size > kMaxPayload)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    // Register a block-wise resource (short URI to be reachable under mutation)
    // and a catch-all default handler so requests fully traverse the dispatch
    // and block-wise reassembly code paths.
    Coap::ResourceBlockWise resource("a", &FuzzCoapHandler, nullptr, &FuzzReceiveHook, &FuzzTransmitHook);
    node.Get<Coap::ApplicationCoap>().AddBlockWiseResource(resource);
    node.Get<Coap::ApplicationCoap>().SetDefaultHandler(&FuzzCoapHandler, nullptr);

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
    udpHeader.SetDestinationPort(OT_DEFAULT_COAP_PORT);
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + size));
    udpHeader.SetChecksum(0);

    if ((message->Append(ip6Header) != kErrorNone) || (message->Append(udpHeader) != kErrorNone) ||
        (message->AppendBytes(data, static_cast<uint16_t>(size)) != kErrorNone))
    {
        message->Free();
        node.Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(resource);
        return 0;
    }

    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, addr, addr, Ip6::kProtoUdp);
    message->SetOffset(0);

    message->SetOrigin(Message::kOriginThreadNetif);

    IgnoreError(node.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));

    nexus.AdvanceTime(10 * 1000);

    node.Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(resource);

    return 0;
}

} // namespace Nexus
} // namespace ot
