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
 *  ARISING IN AY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Received-IPv6-datagram fuzzer.
 *
 * Attacker model: a device that holds the Thread Network Key -- any joined
 * mesh member, for example a sensor or bulb -- sending IPv6 datagrams to the
 * target. Frames from such a device pass MAC security, so everything above
 * IPv6 parses attacker-controlled bytes.
 *
 * No existing fuzz target reaches this code:
 *
 *   - `ip6-fuzzer` calls `otIp6Send` -> `Ip6::SendRaw`, which is the HOST
 *     injection path. `SendRaw` drops any datagram whose source is on-mesh or
 *     whose destination is mesh-local unless the address belongs to this
 *     device (ip6.cpp:1146-1150), which is exactly the addressing every TMF,
 *     MeshCoP and SRP exchange uses. The whole mesh-internal service surface
 *     is therefore unreachable through it.
 *   - `radio-one-node-fuzzer` would have to produce a frame carrying a valid
 *     MAC MIC under a key it does not have.
 *
 * Injecting at `Ip6::HandleDatagram` with `kOriginThreadNetif` and link
 * security marked enabled models the post-decrypt state precisely, and reaches
 * IPv6 extension headers, MPL, ICMPv6, UDP and TCP demultiplexing, and from
 * there MLE, TMF/CoAP, MeshCoP, SRP server, DNS-SD server and DHCPv6.
 *
 * Several datagrams are delivered per test case so that state built by one
 * message is visible to the next.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kMaxDatagrams   = 8;
static constexpr uint16_t kMaxDatagramLen = 1280;

class FuzzDataProvider
{
public:
    FuzzDataProvider(const uint8_t *aData, size_t aSize)
        : mData(aData)
        , mSize(aSize)
    {
    }

    bool ConsumeData(void *aBuf, size_t aLength)
    {
        if (aLength > mSize)
        {
            return false;
        }

        memcpy(aBuf, mData, aLength);
        mData += aLength;
        mSize -= aLength;

        return true;
    }

    uint8_t ConsumeByte(void)
    {
        uint8_t result = 0;

        if (mSize > 0)
        {
            result = *mData;
            mData++;
            mSize--;
        }

        return result;
    }

    const uint8_t *Peek(void) const { return mData; }

    void Skip(size_t aLength)
    {
        aLength = (aLength > mSize) ? mSize : aLength;
        mData += aLength;
        mSize -= aLength;
    }

    size_t RemainingBytes(void) const { return mSize; }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzDataProvider fdp(data, size);

    unsigned int seed;

    // Header: u32 seed. Then records:
    //     u16 length  (little endian, clamped to kMaxDatagramLen)
    //     u8  flags   (bit0 link security, bit1 advance time)
    //     u8  datagram[length]

    if (size < sizeof(seed) + 3)
    {
        return 0;
    }

    if (size > sizeof(seed) + kMaxDatagrams * (3 + kMaxDatagramLen))
    {
        return 0;
    }

    if (!fdp.ConsumeData(&seed, sizeof(seed)))
    {
        return 0;
    }

    // The network is formed from a FIXED seed rather than the input's. Node
    // addresses (mesh-local prefix, RLOC, ML-EID, ExtAddress) are derived from
    // `rand()` in `Dataset::Info::GenerateRandom()`, so seeding from the input
    // would give every test case a different 128-bit destination address that
    // the fuzzer cannot guess -- almost no datagram would ever reach a bound
    // socket. Fixing it also makes any reproducer deterministic, which a
    // submitted PoC requires.
    OT_UNUSED_VARIABLE(seed);
    srand(0x0764A17E);

    Core nexus;

    Node &node = nexus.CreateNode();

    SuccessOrQuit(node.GetInstance().SetLogLevel(kLogLevelInfo));

    node.GetInstance().Get<BorderRouter::InfraIf>().Init(/* aInfraIfIndex */ 1, /* aInfraIfIsRunning */ true);
    SuccessOrQuit(node.GetInstance().Get<BorderRouter::RoutingManager>().SetEnabled(true));
    node.GetInstance().Get<Srp::Server>().SetAutoEnableMode(true);
    node.GetInstance().Get<BorderRouter::RoutingManager>().SetDhcp6PdEnabled(true);
    node.GetInstance().Get<BorderRouter::RoutingManager>().SetNat64PrefixManagerEnabled(true);
    node.GetInstance().Get<Nat64::Translator>().SetEnabled(true);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    Log("---------------------------------------------------------------------------------------");
    Log("Fuzz");

    for (uint32_t num = 0; (num < kMaxDatagrams) && (fdp.RemainingBytes() >= 3); num++)
    {
        Message *message;
        uint16_t length;
        uint8_t  flags;

        length = fdp.ConsumeByte();
        length |= static_cast<uint16_t>(fdp.ConsumeByte()) << 8;
        flags = fdp.ConsumeByte();

        if (length > kMaxDatagramLen)
        {
            length = static_cast<uint16_t>(length % (kMaxDatagramLen + 1));
        }

        if (length > fdp.RemainingBytes())
        {
            length = static_cast<uint16_t>(fdp.RemainingBytes());
        }

        message = node.GetInstance().Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrExit(message != nullptr);

        if (message->AppendBytes(fdp.Peek(), length) != kErrorNone)
        {
            message->Free();
            break;
        }

        fdp.Skip(length);

        // Optionally rewrite the destination address to one the node actually
        // holds. Services such as TMF, the SRP server and the DNS-SD server
        // bind to unicast mesh-local addresses; without this the fuzzer would
        // have to guess a 128-bit address to reach any of them, so nearly
        // every datagram would die in the address filter.
        if (((flags & 0x04) != 0) && (length >= sizeof(Ip6::Header)))
        {
            // Offset of the destination address inside an IPv6 header:
            // 4 (version/class/flow) + 2 (payload length) + 1 (next header)
            // + 1 (hop limit) + 16 (source address).
            static constexpr uint16_t kDstOffset = 24;
            static_assert(sizeof(Ip6::Header) == kDstOffset + sizeof(Ip6::Address), "unexpected Ip6::Header layout");

            uint8_t wanted = (flags >> 4) & 0x0F;
            uint8_t seen   = 0;

            for (const Ip6::Netif::UnicastAddress &addr : node.GetInstance().Get<ThreadNetif>().GetUnicastAddresses())
            {
                if (seen == wanted)
                {
                    message->WriteBytes(kDstOffset, &addr.GetAddress(), sizeof(Ip6::Address));
                    break;
                }

                seen++;
            }
        }

        // Optionally rewrite the source to a mesh-local address, i.e. make the
        // datagram look like it came from another node on the mesh. TMF admits
        // a message only when BOTH source and destination are mesh-local
        // (tmf.cpp `Agent::IsTmfMessage`), so without this the entire TMF
        // surface -- MeshCoP dataset management, network data registration,
        // address resolution, diagnostics, commissioning -- is unreachable.
        if (((flags & 0x08) != 0) && (length >= sizeof(Ip6::Header)))
        {
            static constexpr uint16_t kSrcOffset = 8;

            Ip6::Address src;

            src.InitAsLocator(node.GetInstance().Get<Mle::Mle>().GetMeshLocalPrefix(),
                              static_cast<uint16_t>(0x2800 | ((flags >> 4) & 0x0F)));

            message->WriteBytes(kSrcOffset, &src, sizeof(Ip6::Address));
        }

        // Model the message as one that arrived over the Thread radio. TMF and
        // several other consumers gate on the origin, so a message injected
        // with any other origin never reaches them.
        message->SetOrigin(Message::kOriginThreadNetif);
        message->SetLinkSecurityEnabled((flags & 0x01) != 0);

        Log("--- datagram %u: len=%u sec=%u ---", num, length, (flags & 0x01) != 0);

        IgnoreError(node.GetInstance().Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));

        if ((flags & 0x02) != 0)
        {
            nexus.AdvanceTime(50);
        }
    }

    nexus.AdvanceTime(10 * 1000);

exit:
    return 0;
}

} // namespace Nexus
} // namespace ot
