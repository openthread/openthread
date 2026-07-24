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
 * Regression test: a child MUST verify the Response TLV in an MLE Child Update
 * Response against the challenge it sent to its parent before treating the
 * message as authoritative.
 *
 * An unverified Response TLV would let an unsolicited Child Update Response
 * carrying the parent's identity be classified as an authoritative message and
 * force adoption of an arbitrary key sequence with `KeyManager::kForceUpdate`,
 * exempting it from the key-switch guard. The parent side performs the mirror
 * verification in `HandleChildUpdateResponseOnParent()`; this test covers the
 * child side.
 *
 * Cases:
 *  1. An unsolicited Child Update Response carrying an arbitrary
 *     Response TLV MUST be rejected (no key sequence adoption).
 *  2. A unsolicited Response TLV of all zeros (the value of a stored challenge that
 *     was never generated) MUST also be rejected: the stored challenge must
 *     never remain in a predictable state while in the child role.
 *  3. Positive control: the genuine re-sync flow (parent moves to a new key
 *     sequence, child sends Child Update Request with a Challenge TLV, parent
 *     responds echoing it) MUST still be accepted, and the child adopts the
 *     new key sequence.
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

static constexpr uint16_t kMlePort                  = 19788;
static constexpr uint32_t kMaxAdvertisementInterval = 32 * 1000;

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

// Builds the MLE UDP payload: suite(0) + SecurityHeader + AES-CCM(cmd+TLVs) + MIC4.
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

    aOut[idx++] = 0; // security suite: 802.15.4 security

    secHdr      = &aOut[idx];
    aOut[idx++] = 0x15;                                       // secCtl: EncMic32 | KeyIdMode2
    aOut[idx++] = static_cast<uint8_t>(aFrameCounter & 0xff); // frame counter LE
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 8) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 16) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 24) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 24) & 0xff); // key source BE = key sequence
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 16) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 8) & 0xff);
    aOut[idx++] = static_cast<uint8_t>(aKeySequence & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence & 0x7f) + 1); // key index

    uint16_t cmdOffset = idx;
    aOut[idx++]        = aCommand;
    memcpy(&aOut[idx], aTlvs, aTlvsLength);
    idx += aTlvsLength;

    // AAD = sender IPv6 + receiver IPv6 + security header (10 bytes)
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

    idx += 4; // MIC written by Process at end of data

    return idx;
}

// Wraps an MLE payload in IPHC(hop-limit 255)+UDP and injects it into the
// receiver's radio as a MAC-unsecured 802.15.4 frame.
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

    // MAC header: data | panid-comp | dst=short | ver=2006 | src=ext (unsecured)
    uint16_t fcf = 0x0001 | 0x0040 | 0x0800 | 0x1000 | 0xC000;

    psdu[idx++] = static_cast<uint8_t>(fcf & 0xff);
    psdu[idx++] = static_cast<uint8_t>(fcf >> 8);
    psdu[idx++] = 0x99;
    psdu[idx++] = static_cast<uint8_t>(panId & 0xff);
    psdu[idx++] = static_cast<uint8_t>(panId >> 8);
    psdu[idx++] = 0xff; // broadcast
    psdu[idx++] = 0xff;
    for (uint8_t i = 0; i < 8; i++)
    {
        psdu[idx++] = 0xD0 + i; // arbitrary MAC ext src (not the MLE identity)
    }

    // IPHC: TF elided, NH inline, HLIM=255, inline src+dst
    psdu[idx++] = 0x7B;
    psdu[idx++] = 0x00;
    psdu[idx++] = 17; // UDP
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
    frame.mLength                  = idx + 2; // + FCS
    frame.mChannel                 = aReceiver.Get<Mac::Mac>().GetPanChannel();
    frame.mInfo.mRxInfo.mRssi      = -40;
    frame.mInfo.mRxInfo.mLqi       = 0x7f;
    frame.mInfo.mRxInfo.mTimestamp = 0;

    otPlatRadioReceiveDone(&aReceiver.GetInstance(), &frame, OT_ERROR_NONE);
    aNexus.AdvanceTime(200);
}

// Builds and injects a unsolicited Child Update Response (parent link-local source
// source) whose Response TLV carries `aResponseBytes`.
static void InjectUnsolicitedChildUpdateResponse(Node    &aChild,
                                                 Node    &aParent,
                                                 Core    &aNexus,
                                                 uint8_t  aResponseByte,
                                                 uint32_t aKeySequence,
                                                 uint32_t aFrameCounter)
{
    otIp6Address srcIp6;
    otIp6Address dstIp6 = aChild.Get<Mle::Mle>().GetLinkLocalAddress();

    // Source: parent's link-local (fe80:: + IID(parent ext addr)).
    memset(&srcIp6, 0, sizeof(srcIp6));
    srcIp6.mFields.m8[0] = 0xfe;
    srcIp6.mFields.m8[1] = 0x80;
    memcpy(srcIp6.mFields.m8 + 8, aParent.Get<Mac::Mac>().GetExtAddress().m8, 8);
    srcIp6.mFields.m8[8] ^= 0x02; // ext addr -> IID

    uint8_t  tlvs[64];
    uint16_t t = 0;

    tlvs[t++] = 4; // Response TLV
    tlvs[t++] = 8;
    for (uint8_t i = 0; i < 8; i++)
    {
        tlvs[t++] = aResponseByte; // never matches a generated challenge
    }
    tlvs[t++] = 1; // Mode TLV
    tlvs[t++] = 1;
    tlvs[t++] = aChild.Get<Mle::Mle>().GetDeviceMode().Get();
    tlvs[t++] = 0; // Source Address TLV
    tlvs[t++] = 2;
    {
        uint16_t rloc = aParent.Get<Mle::Mle>().GetRloc16();
        tlvs[t++]     = static_cast<uint8_t>(rloc >> 8);
        tlvs[t++]     = static_cast<uint8_t>(rloc & 0xff);
    }
    tlvs[t++] = 11; // Leader Data TLV
    tlvs[t++] = 8;
    {
        const Mle::LeaderData &ld  = aChild.Get<Mle::Mle>().GetLeaderData();
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

    static uint8_t mle[160];
    uint16_t       mleLen;

    mleLen = BuildSecuredMle(aChild, srcIp6, dstIp6, aKeySequence, aFrameCounter,
                             /* kCommandChildUpdateResponse */ 14, tlvs, t, mle);
    InjectMle(aChild, aNexus, srcIp6, dstIp6, mle, mleLen);
}

void TestChildUpdateResponseChallenge(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();

    leader.SetName("LEADER");
    child.SetName("CHILD");

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Form the network and attach the child");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    child.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(30 * 1000);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    uint32_t seq0 = child.Get<KeyManager>().GetCurrentKeySequence();

    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == seq0);

    Log("---------------------------------------------------------------------------------------");
    Log("Case 1: unsolicited Child Update Response (arbitrary Response TLV) must be rejected");

    InjectUnsolicitedChildUpdateResponse(child, leader, nexus, /* aResponseByte */ 0xA5,
                                         /* aKeySequence */ seq0 + 5, /* aFrameCounter */ 0x1000);

    // The unsolicited message must NOT adopt the injected key sequence.
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == seq0);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Case 2: unsolicited all-zero Response TLV (never-generated challenge value) must be rejected");

    InjectUnsolicitedChildUpdateResponse(child, leader, nexus, /* aResponseByte */ 0x00,
                                         /* aKeySequence */ seq0 + 9, /* aFrameCounter */ 0x1001);

    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == seq0);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Case 3: genuine solicited re-sync (Challenge/Response echo with the real parent) must work");

    // Move the leader to a much higher key sequence. The child sees a large
    // key sequence jump in a peer-class message, sends a Child Update Request
    // with a Challenge TLV, and the parent's Child Update Response echoing the
    // challenge (an authoritative message) makes the child adopt the new key
    // sequence. The jump target is chosen above any unsolicited value so the
    // parent's messages are not shadowed by the neighbor key sequence records
    // updated during the unsolicited-message cases.
    leader.Get<KeyManager>().SetCurrentKeySequence(seq0 + 20, KeyManager::kForceUpdate);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == seq0 + 20);

    nexus.AdvanceTime(3 * kMaxAdvertisementInterval);

    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == seq0 + 20);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("TestChildUpdateResponseChallenge passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildUpdateResponseChallenge();
    printf("All tests passed\n");
    return 0;
}
