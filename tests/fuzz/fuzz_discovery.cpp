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
 * MLE Discovery fuzzer -- THE unauthenticated MeshCoP TLV surface.
 *
 * This is the only entry point in OpenThread that parses attacker bytes with no
 * key of any kind involved. `Mle::HandleUdpReceive` dispatches
 * `kCommandDiscoveryRequest` (16) and `kCommandDiscoveryResponse` (17) on
 * security suite `kNoSecurity` BEFORE `VerifyOrExit(!IsDisabled())` and before
 * any MIC verification (mle.cpp:1587-1607). Attacker position: radio range. No
 * Network Key, no PSKc, no credentials.
 *
 * Everything else that parses pre-key bytes is either compiled out or capped:
 *   - Beacon payload parsing: OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
 *     defaults to 0 (config/mac.h:528), and NetworkName::Set is bounded anyway.
 *   - Header IE / CSL / link metrics: behind ProcessReceiveSecurity, so any
 *     finding there is capped at the compromised-device premise.
 *   - Mac::Frame field accessors: gated by ValidatePsdu (see F-002).
 *
 * Response-side parse chain (discover_scanner.cpp:315):
 *     Tlv::FindTlvValueOffsetRange(msg, kDiscovery, range)
 *     msg.SetOffset(range.GetOffset()); msg.SetLength(range.GetEndOffset())
 *     Tlv::Find<DiscoveryResponseTlv>   version + flags
 *     Tlv::Find<ExtendedPanIdTlv>       fixed 8 bytes
 *     Tlv::Find<NetworkNameTlv>         wire length -> fixed char[] in a stack ScanResult
 *     Tlv::Find<JoinerUdpPortTlv>       uint16
 *     SteeringDataTlv::FindIn           variable-length bloom filter, then a bit walk
 * Request side: Mle::HandleDiscoveryRequest (mle_ftd.cpp:2700).
 *
 * THROUGHPUT: node construction, interface bring-up and the initial scan run
 * ONCE in LLVMFuzzerInitialize. The previous revision rebuilt a Nexus Core and
 * a node per input and managed ~400 exec/s; only the message build and the
 * datagram handoff belong on the per-input path.
 *
 * SCAN LIVENESS: `DiscoverScanner::HandleDiscoveryResponse` exits on its third
 * line unless `mState == kStateScanning`. With a persistent node the scan can
 * finish and silently turn every later input into an early exit -- a textbook
 * clean-looking negative. The scan is therefore re-armed before every input,
 * and OT_FUZZ_TRACE=1 periodically reports the scan state.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kMaxMlePayload = 1024;

static Core *sCore = nullptr;
static Node *sNode = nullptr;

static void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    OT_UNUSED_VARIABLE(aResult);
    OT_UNUSED_VARIABLE(aContext);
}

// Re-arm the discovery scan. Without this the scan ends and every subsequent
// input becomes a silent early exit indistinguishable from a clean negative.
static void EnsureScanning(void)
{
    if (!sNode->Get<Mle::DiscoverScanner>().IsInProgress())
    {
        Mac::ChannelMask mask(0);

        mask.SetMask(::ot::Radio::kSupportedChannels);
        IgnoreError(sNode->Get<Mle::DiscoverScanner>().Discover(
            mask, Mac::kPanIdBroadcast, /* aJoiner */ false, /* aEnableFiltering */ false,
            /* aFilterIndexes */ nullptr, HandleDiscoverResult, nullptr));
    }
}

// Wrap an attacker-controlled MLE payload in the framing
// `Mle::HandleUdpReceive` requires: link-local source, a destination this node
// holds, hop limit exactly 255 (mle.cpp:1583), MLE port both ways.
static Message *BuildMleDatagram(const uint8_t *aPayload, uint16_t aLength, bool aMulticastDst)
{
    Instance      &instance = sNode->GetInstance();
    Message       *message  = nullptr;
    Ip6::Header    ip6;
    Ip6::UdpHeader udp;
    Ip6::Address   src;
    Ip6::Address   dst;

    src.Clear();
    src.mFields.m8[0]  = 0xfe;
    src.mFields.m8[1]  = 0x80;
    src.mFields.m8[8]  = 0x02;
    src.mFields.m8[15] = 0x77;

    dst =
        aMulticastDst ? Ip6::Address::GetLinkLocalAllNodesMulticast() : instance.Get<Mle::Mle>().GetLinkLocalAddress();

    udp.SetSourcePort(Mle::kUdpPort);
    udp.SetDestinationPort(Mle::kUdpPort);
    udp.SetLength(static_cast<uint16_t>(sizeof(udp) + aLength));
    udp.SetChecksum(0);

    ip6.Clear();
    ip6.InitVersionTrafficClassFlow();
    ip6.SetPayloadLength(static_cast<uint16_t>(sizeof(udp) + aLength));
    ip6.SetNextHeader(Ip6::kProtoUdp);
    ip6.SetHopLimit(255); // Mle::Mle::kMleHopLimit, which is private
    ip6.SetSource(src);
    ip6.SetDestination(dst);

    message = instance.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrExit(message != nullptr);

    if ((message->Append(ip6) != kErrorNone) || (message->Append(udp) != kErrorNone) ||
        (message->AppendBytes(aPayload, aLength) != kErrorNone))
    {
        message->Free();
        message = nullptr;
        ExitNow();
    }

    // `UpdateMessageChecksum` computes from `Message::GetOffset()`, so the
    // offset must point at the UDP header while it runs.
    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, src, dst, Ip6::kProtoUdp);
    message->SetOffset(0);

    message->SetOrigin(Message::kOriginThreadNetif);
    message->SetLinkSecurityEnabled(false);

exit:
    return message;
}

} // namespace Nexus
} // namespace ot

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    using namespace ot;
    using namespace ot::Nexus;

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    srand(0x0764A17E);

    sCore = new Core();
    sNode = &sCore->CreateNode();

    sNode->GetInstance().SetLogLevel((getenv("OT_FUZZ_VERBOSE") != nullptr) ? kLogLevelInfo : kLogLevelCrit);

    // Bring the interface up so the MLE socket is bound. No attach: Discovery
    // dispatches before the IsDisabled() check.
    sNode->Get<ThreadNetif>().Up();
    IgnoreError(sNode->Get<Mle::Mle>().Start());
    sCore->AdvanceTime(20);

    EnsureScanning();

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    using namespace ot;
    using namespace ot::Nexus;

    static uint64_t sInputs = 0;

    Message *message;
    bool     multicast;

    if ((size < 2) || (size > kMaxMlePayload))
    {
        return 0;
    }

    sInputs++;

    // First byte selects unicast vs link-local-multicast destination; the rest
    // is the MLE payload, so entropy goes to the parser rather than to
    // rediscovering the framing.
    multicast = (data[0] & 0x01) != 0;

    EnsureScanning();

    message = BuildMleDatagram(data + 1, static_cast<uint16_t>(size - 1), multicast);

    if (message != nullptr)
    {
        IgnoreError(sNode->GetInstance().Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message)));
    }

    if ((getenv("OT_FUZZ_TRACE") != nullptr) && ((sInputs % 200000) == 0))
    {
        fprintf(stderr, "[trace] inputs=%llu scanning=%d\n", (unsigned long long)sInputs,
                sNode->Get<Mle::DiscoverScanner>().IsInProgress());
    }

    return 0;
}
