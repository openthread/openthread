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
 *  IPv6 extension-header / fragment-reassembly fuzz harness (added for security
 *  research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli,
 *  and the existing `ip6` target injects on the *send* path (`otIp6Send`). The
 *  receive-side extension-header processing (`Ip6::HandleExtensionHeaders`:
 *  hop-by-hop / destination options TLV walking, routing header) and, above all,
 *  the IPv6 fragment reassembly engine (`Ip6::HandleFragment`) are not exercised
 *  by that target. Reassembly is a classic memory-safety surface: it stitches
 *  attacker fragments into a growable buffer using per-fragment offset / length
 *  arithmetic, and it explicitly reconciles a different unfragmentable-header
 *  length between the first fragment and later fragments (see `unfragLength` /
 *  `WriteBytesFromMessage` in `HandleFragment`).
 *
 *  This harness forms a node and delivers a sequence of IPv6 datagrams to the
 *  node's own address (so they take the receive path) through the real receive
 *  entry `Ip6::HandleDatagram`. The harness builds only the fixed 40-byte IPv6
 *  base header; the fuzzer controls the base header's Next Header value and every
 *  byte after it (the extension-header chain, the Fragment header's
 *  identification / offset / M-flag / next-header, and the payload). The input is
 *  split into multiple length-prefixed datagrams so several fragments sharing an
 *  identification can drive the reassembly list across calls.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxFragment = 1500;
    const uint8_t  kMaxCount    = 32;

    if (size < 3 || size > 8192)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    Ip6::Address addr = node.Get<Mle::Mle>().GetMeshLocalEid();

    size_t  offset = 0;
    uint8_t count  = 0;

    while ((offset + 2 <= size) && (count < kMaxCount))
    {
        uint16_t chunkLen = static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
        offset += 2;

        size_t avail = size - offset;
        if (chunkLen > avail)
        {
            chunkLen = static_cast<uint16_t>(avail);
        }
        if (chunkLen == 0)
        {
            break;
        }
        if (chunkLen > kMaxFragment)
        {
            chunkLen = kMaxFragment;
        }

        const uint8_t *chunk   = &data[offset];
        uint8_t        nextHdr = chunk[0];
        uint16_t       bodyLen = static_cast<uint16_t>(chunkLen - 1);
        const uint8_t *body    = chunk + 1;

        offset += chunkLen;
        count++;

        Message *message = node.Get<Ip6::Ip6>().NewMessage();
        if (message == nullptr)
        {
            break;
        }

        Ip6::Header ip6Header;
        ip6Header.InitVersionTrafficClassFlow();
        ip6Header.SetPayloadLength(bodyLen);
        ip6Header.SetNextHeader(nextHdr);
        ip6Header.SetHopLimit(64);
        ip6Header.SetSource(addr);
        ip6Header.SetDestination(addr);

        if ((message->Append(ip6Header) != kErrorNone) ||
            ((bodyLen > 0) && (message->AppendBytes(body, bodyLen) != kErrorNone)))
        {
            message->Free();
            continue;
        }

        message->SetOrigin(Message::kOriginThreadNetif);

        IgnoreError(node.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));
    }

    nexus.AdvanceTime(1 * 1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
