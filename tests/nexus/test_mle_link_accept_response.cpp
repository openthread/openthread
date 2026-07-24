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
 * Regression test for Link Accept Response-TLV verification (router variant): a Link
 * Accept from a `kStateValid` neighbor router may only be classified as an
 * AUTHORITATIVE message (forced key-sequence adoption, `KeyManager::kForceUpdate`)
 * when its Response TLV matches the challenge of an outstanding solicited Link
 * Request to that router.
 *
 * Phases:
 *  1. Routine maintenance (quiet-neighbor probe): a valid neighbor router goes silent
 *     for `kMaxNeighborAge` (100 s) so the receiver sends a Link Request probe to the
 *     still-valid neighbor. The neighbor's stored frame counters and key sequence must
 *     be BIT-IDENTICAL across the probe send (the probe must not corrupt the replay
 *     state), and once the neighbor answers, the link must survive with no neighbor
 *     removal (no churn).
 *  2. Solicited large-jump key-sequence resync (positive assertion): a genuine neighbor
 *     legitimately jumps its key sequence by +20; the receiver must adopt it through the
 *     solicited Link Request/Link Accept exchange (the Link Accept must be classified
 *     authoritative when its Response TLV matches the stored challenge).
 *  3. Unsolicited suite: unsolicited Link Accepts carrying the valid neighbor's
 *     identity (arbitrary Response TLV, then all-zero Response TLV) with inflated key
 *     sequences must NOT force key-sequence adoption and must not affect the
 *     receiver's role.
 *  4. Recovery and stability: after the unsolicited messages (which update the
 *     neighbor's record key sequence at ingress, a documented behavior), a further
 *     legitimate key-sequence jump re-syncs the network, and roles/links stay stable
 *     for 120 s.
 *
 * FAILS without the fix at phase 3 (unsolicited message force-adopted); PASSES with it.
 */

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "crypto/aes_ccm.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint16_t kMlePort         = 19788;

static uint32_t sRouterRemovedCount = 0;

static void HandleNeighborTableEvent(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aInfo)
{
    OT_UNUSED_VARIABLE(aInfo);

    if (aEvent == OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED)
    {
        sRouterRemovedCount++;
    }
}

static uint16_t UdpChecksum(const otIp6Address &aSrc, const otIp6Address &aDst, const uint8_t *aUdp, uint16_t aUdpLen)
{
    uint32_t sum = 0;

    for (int i = 0; i < 16; i += 2)
    {
        sum += (static_cast<uint16_t>(aSrc.mFields.m8[i]) << 8) + aSrc.mFields.m8[i + 1];
        sum += (static_cast<uint16_t>(aDst.mFields.m8[i]) << 8) + aDst.mFields.m8[i + 1];
    }
    sum += aUdpLen;
    sum += 17;
    for (uint16_t i = 0; i < aUdpLen; i++)
    {
        sum += (i & 1) ? aUdp[i] : (static_cast<uint16_t>(aUdp[i]) << 8);
    }
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

static uint16_t BuildSecuredMle(Node               &aReceiver,
                                const otIp6Address &aSrcIp6,
                                const otIp6Address &aDstIp6,
                                uint32_t            aKeySequence,
                                uint32_t            aFrameCounter,
                                uint8_t             aCommand,
                                const uint8_t      *aTlvs,
                                uint16_t            aTlvsLength,
                                uint8_t            *aOut)
{
    uint16_t idx = 0;
    uint8_t  aad[16 + 16 + 10];
    uint8_t *secHdr;

    aOut[idx++] = 0;

    secHdr      = &aOut[idx];
    aOut[idx++] = 0x15;
    aOut[idx++] = static_cast<uint8_t>(aFrameCounter & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 8) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 16) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 24) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 24) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 16) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 8) & 0xff);
    aOut[idx++] = static_cast<uint8_t>(aKeySequence & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence & 0x7f) + 1);

    uint16_t cmdOffset = idx;
    aOut[idx++]        = aCommand;
    memcpy(&aOut[idx], aTlvs, aTlvsLength);
    idx += aTlvsLength;

    memcpy(aad, aSrcIp6.mFields.m8, 16);
    memcpy(aad + 16, aDstIp6.mFields.m8, 16);
    memcpy(aad + 32, secHdr, 10);

    {
        Crypto::AesCcm           aesCcm;
        Crypto::AesCcm::Nonce    nonce;
        Mac::ExtAddress          extAddress;
        Ip6::InterfaceIdentifier iid;

        memcpy(iid.mFields.m8, aSrcIp6.mFields.m8 + 8, 8);
        extAddress.SetFromIid(iid);
        nonce.InitFrom(extAddress, aFrameCounter, Mac::Frame::kSecurityEncMic32);

        aesCcm.SetKey(aReceiver.Get<KeyManager>().GetTemporaryMleKey(aKeySequence));
        aesCcm.SetNonce(nonce);
        aesCcm.SetAuthData(aad, sizeof(aad));
        aesCcm.SetTagLength(4);
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, &aOut[cmdOffset], 1 + aTlvsLength));
    }

    idx += 4;

    return idx;
}

static void InjectMle(Node               &aReceiver,
                      Core               &aNexus,
                      const otIp6Address &aSrcIp6,
                      const otIp6Address &aDstIp6,
                      const uint8_t      *aMle,
                      uint16_t            aMleLength)
{
    static uint8_t psdu[OT_RADIO_FRAME_MAX_SIZE];
    uint16_t       idx    = 0;
    uint16_t       panId  = aReceiver.Get<Mac::Mac>().GetPanId();
    uint16_t       udpLen = 8 + aMleLength;
    uint8_t       *udp;
    uint16_t       csum;
    otRadioFrame   frame;

    uint16_t fcf = 0x0001 | 0x0040 | 0x0800 | 0x1000 | 0xC000;

    psdu[idx++] = static_cast<uint8_t>(fcf & 0xff);
    psdu[idx++] = static_cast<uint8_t>(fcf >> 8);
    psdu[idx++] = 0x77;
    psdu[idx++] = static_cast<uint8_t>(panId & 0xff);
    psdu[idx++] = static_cast<uint8_t>(panId >> 8);
    psdu[idx++] = 0xff;
    psdu[idx++] = 0xff;
    for (uint8_t i = 0; i < 8; i++)
    {
        psdu[idx++] = 0xD0 + i;
    }

    psdu[idx++] = 0x7B;
    psdu[idx++] = 0x00;
    psdu[idx++] = 17;
    memcpy(&psdu[idx], aSrcIp6.mFields.m8, 16);
    idx += 16;
    memcpy(&psdu[idx], aDstIp6.mFields.m8, 16);
    idx += 16;

    udp    = &psdu[idx];
    udp[0] = static_cast<uint8_t>(kMlePort >> 8);
    udp[1] = static_cast<uint8_t>(kMlePort & 0xff);
    udp[2] = static_cast<uint8_t>(kMlePort >> 8);
    udp[3] = static_cast<uint8_t>(kMlePort & 0xff);
    udp[4] = static_cast<uint8_t>(udpLen >> 8);
    udp[5] = static_cast<uint8_t>(udpLen & 0xff);
    udp[6] = 0;
    udp[7] = 0;
    memcpy(&udp[8], aMle, aMleLength);
    csum   = UdpChecksum(aSrcIp6, aDstIp6, udp, udpLen);
    udp[6] = static_cast<uint8_t>(csum >> 8);
    udp[7] = static_cast<uint8_t>(csum & 0xff);
    idx += udpLen;

    memset(&frame, 0, sizeof(frame));
    frame.mPsdu                    = psdu;
    frame.mLength                  = idx + 2;
    frame.mChannel                 = aReceiver.Get<Mac::Mac>().GetPanChannel();
    frame.mInfo.mRxInfo.mRssi      = -40;
    frame.mInfo.mRxInfo.mLqi       = 0x7f;
    frame.mInfo.mRxInfo.mTimestamp = 0;

    otPlatRadioReceiveDone(&aReceiver.GetInstance(), &frame, OT_ERROR_NONE);
    aNexus.AdvanceTime(200);
}

void TestLinkAcceptResponseVerification(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &x      = nexus.CreateNode();
    Node &p      = nexus.CreateNode();

    leader.SetName("LEADER");
    x.SetName("X");
    p.SetName("P");

    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    x.Join(leader);
    p.Join(leader);
    nexus.AdvanceTime(300 * 1000);
    VerifyOrQuit(x.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(p.Get<Mle::Mle>().IsRouter());

    uint8_t leaderId = Mle::RouterIdFromRloc16(leader.Get<Mle::Mle>().GetRloc16());
    uint8_t xId      = Mle::RouterIdFromRloc16(x.Get<Mle::Mle>().GetRloc16());
    uint8_t pId      = Mle::RouterIdFromRloc16(p.Get<Mle::Mle>().GetRloc16());

    Router *pX = p.Get<RouterTable>().FindRouterById(xId);
    Router *lX = leader.Get<RouterTable>().FindRouterById(xId);
    Router *xP = x.Get<RouterTable>().FindRouterById(pId);
    Router *xL = x.Get<RouterTable>().FindRouterById(leaderId);

    VerifyOrQuit((pX != nullptr) && pX->IsStateValid());
    VerifyOrQuit((lX != nullptr) && lX->IsStateValid());
    VerifyOrQuit((xP != nullptr) && xP->IsStateValid());
    VerifyOrQuit((xL != nullptr) && xL->IsStateValid());

    sRouterRemovedCount = 0;
    p.Get<NeighborTable>().RegisterCallback(HandleNeighborTableEvent);

    Log("=============================================================================");
    Log("Phase 1: quiet-neighbor probe (kMaxNeighborAge) leaves neighbor counters intact");

    uint32_t mleC0  = pX->GetMleFrameCounter();
    uint32_t linkC0 = pX->GetLinkFrameCounters().GetMaximum();
    uint32_t nks0   = pX->GetKeySequence();

    // Silence X (drops all rx, pretends tx success) and pin the last-heard
    // times so the age probes on both sides fire deterministically at +100 s.
    x.Get<Mac::Mac>().SetRadioFilterEnabled(true);
    pX->SetLastHeard(TimerMilli::GetNow());
    lX->SetLastHeard(TimerMilli::GetNow());
    xP->SetLastHeard(TimerMilli::GetNow());
    xL->SetLastHeard(TimerMilli::GetNow());

    {
        bool     probeSent = false;
        uint32_t waited    = 0;

        while (waited < 110 * 1000)
        {
            nexus.AdvanceTime(500);
            waited += 500;

            if (pX->IsWaitingForLinkAccept())
            {
                probeSent = true;
                break;
            }
        }

        Log("Link Request probe to valid neighbor X observed after %lu ms of silence", (unsigned long)(waited));
        VerifyOrQuit(probeSent);
    }

    // THE core assertion: sending a Link Request probe to a valid neighbor
    // must leave the neighbor's stored replay state bit-identical.
    VerifyOrQuit(pX->IsStateValid());
    VerifyOrQuit(pX->GetMleFrameCounter() == mleC0);
    VerifyOrQuit(pX->GetLinkFrameCounters().GetMaximum() == linkC0);
    VerifyOrQuit(pX->GetKeySequence() == nks0);
    Log("Neighbor record counters BIT-IDENTICAL across the probe send");

    x.Get<Mac::Mac>().SetRadioFilterEnabled(false);
    nexus.AdvanceTime(15 * 1000);

    VerifyOrQuit(pX->IsStateValid());
    VerifyOrQuit(!pX->IsWaitingForLinkAccept()); // genuine solicited Link Accept was accepted
    VerifyOrQuit(xP->IsStateValid());
    VerifyOrQuit(xL->IsStateValid());
    VerifyOrQuit(sRouterRemovedCount == 0); // no link churn
    {
        uint32_t mleC1 = pX->GetMleFrameCounter();

        VerifyOrQuit(mleC1 >= mleC0);
        VerifyOrQuit((mleC1 - mleC0) < 100000); // legitimate small advance, no random jump
    }
    Log("Probe answered, link intact, no neighbor removal, counters advanced sanely");

    Log("=============================================================================");
    Log("Phase 2: solicited large-jump key-sequence resync still works (authoritative)");

    uint32_t base = p.Get<KeyManager>().GetCurrentKeySequence();

    x.Get<KeyManager>().SetCurrentKeySequence(base + 20, KeyManager::kForceUpdate);

    {
        uint32_t waited = 0;

        while ((p.Get<KeyManager>().GetCurrentKeySequence() != base + 20) && (waited < 80 * 1000))
        {
            nexus.AdvanceTime(1000);
            waited += 1000;
        }
        Log("P adopted key sequence %lu -> %lu after %lu ms (solicited Link Accept resync)",
            (unsigned long)(base), (unsigned long)(p.Get<KeyManager>().GetCurrentKeySequence()), (unsigned long)(waited));
    }

    VerifyOrQuit(p.Get<KeyManager>().GetCurrentKeySequence() == base + 20);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == base + 20);
    VerifyOrQuit(pX->IsStateValid());
    VerifyOrQuit(sRouterRemovedCount == 0);

    Log("=============================================================================");
    Log("Phase 3: unsolicited Link Accepts must not force key-sequence adoption");

    uint32_t cur = p.Get<KeyManager>().GetCurrentKeySequence();

    otIp6Address srcIp6;
    memset(&srcIp6, 0, sizeof(srcIp6));
    srcIp6.mFields.m8[0] = 0xfe;
    srcIp6.mFields.m8[1] = 0x80;
    memcpy(srcIp6.mFields.m8 + 8, x.Get<Mac::Mac>().GetExtAddress().m8, 8);
    srcIp6.mFields.m8[8] ^= 0x02;

    otIp6Address dstIp6 = p.Get<Mle::Mle>().GetLinkLocalAddress();

    uint8_t  tlvs[96];
    uint16_t t = 0;

    auto appendCommon = [&](uint8_t aResponseByte) {
        t         = 0;
        tlvs[t++] = 0; // Source Address
        tlvs[t++] = 2;
        {
            uint16_t rloc = x.Get<Mle::Mle>().GetRloc16();
            tlvs[t++]     = static_cast<uint8_t>(rloc >> 8);
            tlvs[t++]     = static_cast<uint8_t>(rloc & 0xff);
        }
        tlvs[t++] = 4; // Response TLV, arbitrary bytes
        tlvs[t++] = 8;
        for (uint8_t i = 0; i < 8; i++)
        {
            tlvs[t++] = aResponseByte;
        }
        tlvs[t++] = 18; // Version
        tlvs[t++] = 2;
        tlvs[t++] = 0;
        tlvs[t++] = 4;
        tlvs[t++] = 5; // Link Frame Counter
        tlvs[t++] = 4;
        tlvs[t++] = 0x00;
        tlvs[t++] = 0x30;
        tlvs[t++] = 0x00;
        tlvs[t++] = 0x00;
        tlvs[t++] = 16; // Link Margin
        tlvs[t++] = 1;
        tlvs[t++] = 20;
        tlvs[t++] = 11; // Leader Data
        tlvs[t++] = 8;
        {
            const Mle::LeaderData &ld  = p.Get<Mle::Mle>().GetLeaderData();
            uint32_t               pid = ld.GetPartitionId();

            tlvs[t++] = static_cast<uint8_t>(pid >> 24);
            tlvs[t++] = static_cast<uint8_t>(pid >> 16);
            tlvs[t++] = static_cast<uint8_t>(pid >> 8);
            tlvs[t++] = static_cast<uint8_t>(pid & 0xff);
            tlvs[t++] = ld.GetWeighting();
            tlvs[t++] = ld.GetDataVersion(NetworkData::kFullSet);
            tlvs[t++] = ld.GetDataVersion(NetworkData::kStableSubset);
            tlvs[t++] = ld.GetLeaderRouterId();
        }
    };

    static uint8_t mle[192];
    uint16_t       mleLen;

    Log("Case A: unsolicited Link Accept carrying valid neighbor X's identity, arbitrary Response TLV, keyseq +7");
    appendCommon(0x5A);
    mleLen = BuildSecuredMle(p, srcIp6, dstIp6, cur + 7, 0x2000, /* kCommandLinkAccept */ 1, tlvs, t, mle);
    InjectMle(p, nexus, srcIp6, dstIp6, mle, mleLen);

    VerifyOrQuit(p.Get<KeyManager>().GetCurrentKeySequence() == cur);
    VerifyOrQuit(p.Get<Mle::Mle>().IsRouter());

    Log("Case B: all-zero Response TLV (never-generated challenge), keyseq +13");
    appendCommon(0x00);
    mleLen = BuildSecuredMle(p, srcIp6, dstIp6, cur + 13, 0x2100, 1, tlvs, t, mle);
    InjectMle(p, nexus, srcIp6, dstIp6, mle, mleLen);

    VerifyOrQuit(p.Get<KeyManager>().GetCurrentKeySequence() == cur);
    VerifyOrQuit(p.Get<Mle::Mle>().IsRouter());

    Log("Both unsolicited messages rejected: key sequence unchanged, role unaffected");

    Log("=============================================================================");
    Log("Phase 4: legitimate resync above the record value, then 120 s stability");

    // The unsolicited messages update the neighbor's RECORD key sequence at
    // ingress (pre-handler, class-independent; documented behavior). A
    // legitimate jump above that value re-syncs the network.
    x.Get<KeyManager>().SetCurrentKeySequence(cur + 30, KeyManager::kForceUpdate);

    {
        uint32_t waited = 0;

        while ((p.Get<KeyManager>().GetCurrentKeySequence() != cur + 30) && (waited < 80 * 1000))
        {
            nexus.AdvanceTime(1000);
            waited += 1000;
        }
        Log("P re-synced to key sequence %lu after %lu ms", (unsigned long)(p.Get<KeyManager>().GetCurrentKeySequence()),
            (unsigned long)(waited));
    }

    VerifyOrQuit(p.Get<KeyManager>().GetCurrentKeySequence() == cur + 30);

    nexus.AdvanceTime(120 * 1000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(x.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(p.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(p.Get<KeyManager>().GetCurrentKeySequence() == cur + 30);
    {
        Neighbor *n = p.Get<NeighborTable>().FindNeighbor(x.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit((n != nullptr) && n->IsStateValid());
    }
    VerifyOrQuit(xP->IsStateValid());

    Log("TestLinkAcceptResponseVerification passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestLinkAcceptResponseVerification();
    printf("All tests passed\n");
    return 0;
}
