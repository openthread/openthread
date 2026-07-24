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
 * Verifies IPv6 fragment reassembly buffer sizing and payload placement when fragments
 * of the same datagram carry different-length unfragmentable parts.
 *
 * `Ip6::HandleFragment()` tracks contiguity in fragment-payload space only. It must size
 * the reassembly buffer and place fragment-payload writes using the FIRST fragment's
 * unfragmentable-part length (the one actually copied into the reassembled datagram).
 * Using the current fragment's length instead (e.g., when the final fragment carries an
 * extra Hop-by-Hop extension header) left a never-written region in the reassembled
 * datagram, and the buffer growth path (`SetLength()` -> `MessagePool::NewBuffer()`)
 * reuses pool buffers without clearing, so that region held stale bytes of previously
 * freed messages.
 *
 * This test asserts the corrected behavior:
 *  case A (positive control): a normally fragmented echo (consistent 40-byte headers,
 *          no length mismatch) still reassembles and is answered;
 *  case B: fragments shaped so that the old placement would leave such a region (with
 *          checksums precomputed against several possible region contents) must NOT
 *          produce an echo reply, since a reply only appears when the reassembled
 *          datagram incorporated bytes matching the checksum assumption.
 *
 * Gates: OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE (default 0; 1 in this nexus config),
 * default static message pool (heap builds zero via calloc).
 */

#include <stdio.h>
#include <string.h>

#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/udp.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/ip6.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;

static constexpr uint16_t kPay1     = 200; // fragment 1 payload (echo header + data)
static constexpr uint16_t kPay2     = 8;   // final fragment payload
static constexpr uint16_t kHole     = 8;   // hole size = HBH header length delta
static constexpr uint8_t  kMarker   = 0xC3;
static constexpr uint8_t  kPayByte  = 0xAA;
static constexpr uint8_t  kPay2Byte = 0x55;

static bool     sGotReply;
static bool     sReplyHasMarkerRun;
static uint16_t sReplyLength;

static void HandleIcmp(void *, otMessage *aMessage, const otMessageInfo *, const otIcmp6Header *aHeader)
{
    if (aHeader->mType != OT_ICMP6_TYPE_ECHO_REPLY)
    {
        return;
    }

    uint8_t  buf[512];
    uint16_t offset = otMessageGetOffset(aMessage);
    uint16_t len    = otMessageGetLength(aMessage) - offset;

    len = (len > sizeof(buf)) ? sizeof(buf) : len;
    otMessageRead(aMessage, offset, buf, len);

    sGotReply    = true;
    sReplyLength = len;

    // look for a run of kHole marker bytes (the reflected hole)
    for (uint16_t i = 0; i + kHole <= len; i++)
    {
        bool run = true;

        for (uint16_t j = 0; j < kHole; j++)
        {
            if (buf[i + j] != kMarker)
            {
                run = false;
                break;
            }
        }

        if (run)
        {
            sReplyHasMarkerRun = true;
            break;
        }
    }
}

static uint32_t ChecksumAdd(uint32_t aSum, const uint8_t *aData, uint16_t aLength)
{
    for (uint16_t i = 0; i < aLength; i++)
    {
        aSum += (i & 1) ? aData[i] : (static_cast<uint32_t>(aData[i]) << 8);
    }

    return aSum;
}

static uint16_t IcmpChecksum(const otIp6Address &aSrc,
                             const otIp6Address &aDst,
                             const uint8_t      *aIcmp,
                             uint16_t            aLen1,
                             const uint8_t      *aHole,
                             uint16_t            aHoleLen,
                             const uint8_t      *aTail,
                             uint16_t            aTailLen)
{
    uint32_t sum   = 0;
    uint16_t total = aLen1 + aHoleLen + aTailLen;

    sum = ChecksumAdd(sum, aSrc.mFields.m8, 16);
    sum = ChecksumAdd(sum, aDst.mFields.m8, 16);
    sum += total;
    sum += 58;
    sum = ChecksumAdd(sum, aIcmp, aLen1);
    sum = ChecksumAdd(sum, aHole, aHoleLen);
    sum = ChecksumAdd(sum, aTail, aTailLen);

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

static void SendRawIp6(Node &aNode, const uint8_t *aBuf, uint16_t aLen)
{
    otMessage *message = otIp6NewMessage(&aNode.GetInstance(), nullptr);

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(otMessageAppend(message, aBuf, aLen));
    SuccessOrQuit(otIp6Send(&aNode.GetInstance(), message));
}

static void SendPrimers(Node &aSender, const otIp6Address &aDst, uint8_t aFill)
{
    for (int i = 0; i < 3; i++)
    {
        static uint8_t primer[600];

        memset(primer, aFill, sizeof(primer));

        otMessageInfo info;
        otUdpSocket   socket;

        memset(&info, 0, sizeof(info));
        memset(&socket, 0, sizeof(socket));
        info.mPeerAddr = aDst;
        info.mPeerPort = 55555; // no listener on leader

        SuccessOrQuit(otUdpOpen(&aSender.GetInstance(), &socket, nullptr, nullptr));
        {
            otMessage *m = otUdpNewMessage(&aSender.GetInstance(), nullptr);
            VerifyOrQuit(m != nullptr);
            SuccessOrQuit(otMessageAppend(m, primer, sizeof(primer)));
            SuccessOrQuit(otUdpSend(&aSender.GetInstance(), &socket, m, &info));
        }
        SuccessOrQuit(otUdpClose(&aSender.GetInstance(), &socket));
    }
}

static void BuildAndSendFragments(Node               &aSender,
                                  Core               &aNexus,
                                  const otIp6Address &aSrc,
                                  const otIp6Address &aDst,
                                  uint32_t            aFragId,
                                  bool                aWithHole,
                                  uint8_t             aGapAssume,
                                  bool                aPrimeBetween,
                                  uint16_t            aEchoSeq)
{
    static uint8_t pkt[512];
    uint8_t        icmp[kPay1];
    uint8_t        hole[kHole];
    uint8_t        tail[kPay2];
    uint16_t       csum;

    // ICMP echo request: header + patterned data
    memset(icmp, kPayByte, sizeof(icmp));
    icmp[0] = 128; // echo request
    icmp[1] = 0;
    icmp[2] = 0; // checksum placeholder
    icmp[3] = 0;
    icmp[4] = 0x12;
    icmp[5] = 0x34;
    icmp[6] = static_cast<uint8_t>(aEchoSeq >> 8);
    icmp[7] = static_cast<uint8_t>(aEchoSeq & 0xff);

    memset(hole, aGapAssume, sizeof(hole));
    memset(tail, kPay2Byte, sizeof(tail));

    if (aWithHole)
    {
        // checksum computed assuming the unwritten region would read back the seeded marker bytes
        csum = IcmpChecksum(aSrc, aDst, icmp, kPay1, hole, kHole, tail, kPay2);
    }
    else
    {
        csum = IcmpChecksum(aSrc, aDst, icmp, kPay1, nullptr, 0, tail, kPay2);
    }
    icmp[2] = static_cast<uint8_t>(csum >> 8);
    icmp[3] = static_cast<uint8_t>(csum & 0xff);

    // ---- fragment 1: plain 40-byte header, offset 0, M=1, payload = icmp[kPay1]
    {
        uint16_t idx = 0;

        pkt[idx++] = 0x60;
        pkt[idx++] = 0;
        pkt[idx++] = 0;
        pkt[idx++] = 0;
        pkt[idx++] = static_cast<uint8_t>((8 + kPay1) >> 8);
        pkt[idx++] = static_cast<uint8_t>((8 + kPay1) & 0xff);
        pkt[idx++] = 44; // fragment header
        pkt[idx++] = 64;
        memcpy(&pkt[idx], aSrc.mFields.m8, 16);
        idx += 16;
        memcpy(&pkt[idx], aDst.mFields.m8, 16);
        idx += 16;
        // fragment header: NH=58, rsv, offset 0 | M=1, id
        pkt[idx++] = 58;
        pkt[idx++] = 0;
        pkt[idx++] = 0;
        pkt[idx++] = 1; // offset 0, M
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 24);
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 16);
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 8);
        pkt[idx++] = static_cast<uint8_t>(aFragId & 0xff);
        memcpy(&pkt[idx], icmp, kPay1);
        idx += kPay1;

        SendRawIp6(aSender, pkt, idx);
    }

    aNexus.AdvanceTime(400);

    if (aPrimeBetween)
    {
        // seed the message-pool free list with a known pattern right before the reassembly growth happens
        SendPrimers(aSender, aDst, aGapAssume);
        aNexus.AdvanceTime(400);
    }

    // ---- fragment 2 (final): optional HBH header widens the unfragmentable part
    {
        uint16_t idx      = 0;
        uint16_t extLen   = aWithHole ? 8 : 0;
        uint16_t payLen   = extLen + 8 + kPay2;
        uint16_t offUnits = kPay1 / 8;

        pkt[idx++] = 0x60;
        pkt[idx++] = 0;
        pkt[idx++] = 0;
        pkt[idx++] = 0;
        pkt[idx++] = static_cast<uint8_t>(payLen >> 8);
        pkt[idx++] = static_cast<uint8_t>(payLen & 0xff);
        pkt[idx++] = aWithHole ? 0 : 44; // HBH first if widening
        pkt[idx++] = 64;
        memcpy(&pkt[idx], aSrc.mFields.m8, 16);
        idx += 16;
        memcpy(&pkt[idx], aDst.mFields.m8, 16);
        idx += 16;

        if (aWithHole)
        {
            // HBH: NH=44, HdrExtLen=0, PadN(4)
            pkt[idx++] = 44;
            pkt[idx++] = 0;
            pkt[idx++] = 0x01;
            pkt[idx++] = 0x04;
            pkt[idx++] = 0;
            pkt[idx++] = 0;
            pkt[idx++] = 0;
            pkt[idx++] = 0;
        }

        pkt[idx++] = 58;
        pkt[idx++] = 0;
        pkt[idx++] = static_cast<uint8_t>((offUnits * 8) >> 8);
        pkt[idx++] = static_cast<uint8_t>((offUnits * 8) & 0xff); // M=0
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 24);
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 16);
        pkt[idx++] = static_cast<uint8_t>(aFragId >> 8);
        pkt[idx++] = static_cast<uint8_t>(aFragId & 0xff);
        memcpy(&pkt[idx], tail, kPay2);
        idx += kPay2;

        SendRawIp6(aSender, pkt, idx);
    }
}

void TestFragmentReassemblyHole(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();

    otIcmp6Handler handler;

    memset(&handler, 0, sizeof(handler));
    handler.mReceiveCallback = HandleIcmp;

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    child.Join(leader);
    nexus.AdvanceTime(20 * 1000);
    VerifyOrQuit(child.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(otIcmp6RegisterHandler(&child.GetInstance(), &handler));

    otIp6Address src = *reinterpret_cast<const otIp6Address *>(&child.Get<Mle::Mle>().GetMeshLocalEid());
    otIp6Address dst = *reinterpret_cast<const otIp6Address *>(&leader.Get<Mle::Mle>().GetMeshLocalEid());

    Log("Case A (positive control): consistent-header fragmented echo must be answered");
    sGotReply          = false;
    sReplyHasMarkerRun = false;
    BuildAndSendFragments(child, nexus, src, dst, 0x11111111, /* aWithHole */ false, 0, false, 1);
    nexus.AdvanceTime(3 * 1000);
    VerifyOrQuit(sGotReply);
    Log("control reply received (len %u)", sReplyLength);

    Log("Case B: final fragment carries an extra HBH header (longer unfragmentable part).");
    Log("A reply arrives only when the reassembled datagram content matches one of the");
    Log("checksum assumptions, i.e., only if an unwritten region was incorporated.");

    struct Attempt
    {
        uint8_t mGapAssume;
        bool    mPrimeBetween;
    };

    // (1) seed the free list with 0xC3 right before the growth; (2) assume the region
    // reuses fragment 1's own freed payload bytes; (3) assume cleared/zero bytes.
    Attempt attempts[] = {{kMarker, true}, {kPayByte, false}, {0x00, false}};
    bool    replied    = false;
    uint8_t repliedVal = 0;

    for (uint8_t i = 0; i < 3; i++)
    {
        sGotReply          = false;
        sReplyHasMarkerRun = false;
        BuildAndSendFragments(child, nexus, src, dst, 0x22220000u + i, /* aWithHole */ true, attempts[i].mGapAssume,
                              attempts[i].mPrimeBetween, static_cast<uint16_t>(10 + i));
        nexus.AdvanceTime(3 * 1000);

        Log("attempt %u (gap assumption 0x%02x, prime-between %u): %s", i, attempts[i].mGapAssume,
            attempts[i].mPrimeBetween, sGotReply ? "ECHO REPLY RECEIVED" : "no reply");

        if (sGotReply)
        {
            replied    = true;
            repliedVal = attempts[i].mGapAssume;
            break;
        }
    }

    if (replied)
    {
        Log("mismatched-length request answered; region bytes == 0x%02x pattern", repliedVal);
        Log("(reply len %u; unwritten region content was incorporated and reflected)", sReplyLength);
    }

    // Corrected behavior: no unwritten region exists in the reassembled datagram, so
    // none of the region-assuming checksums can match and no reply is produced for any
    // attempt.
    VerifyOrQuit(!replied);

    Log("TestFragmentReassemblyHole passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFragmentReassemblyHole();
    printf("All tests passed\n");
    return 0;
}
