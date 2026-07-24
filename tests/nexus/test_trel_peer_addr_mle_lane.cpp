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
 * Regression test (MLE lane): a TREL peer socket-address rebind after an MLE-secured
 * receive must require the MLE-verified source identity to match the selected peer
 * AND the message to have passed the MLE frame-counter checks.
 *
 * The peer entry to rebind is selected from the unauthenticated TREL encapsulation
 * header Source field; MLE-secured messages can ride MAC-unsecured frames (the whole
 * attach exchange) whose MAC source is not covered by any verification; the IPv6
 * source is an inline 6LoWPAN IID with no cross-check against the MAC source; and a
 * sender that is not a currently valid neighbor is not subject to the MLE
 * frame-counter checks. Without the fix, a genuine attach-time MLE message from a
 * non-neighbor device X, re-emitted from a different socket address with only the
 * unverified fields rewritten (TREL header Source = P, MAC source = P), rebound P's
 * entry to that socket address.
 *
 * Cases:
 *  1. CONTROL: the same packet with a corrupted MLE MIC causes no rebind (MLE security
 *     is the authorizer; arbitrary garbage cannot drive the lane).
 *  2. POSITIVE (mobility preserved; guards against an over-broad always-deny change):
 *     an identity-consistent FRESH MLE message from valid neighbor B (TREL header B,
 *     MAC source B, IPv6 IID B, fresh MLE frame counter) from a NEW socket address
 *     MUST update B's entry.
 *  3. NEGATIVE (the fix): the re-emission described above (MLE payload of
 *     non-neighbor X carried verbatim; TREL header Source = P, MAC source = P) MUST
 *     NOT update P's entry directly.
 *
 * FAILS before the fix (case 3 rebinds P); PASSES after.
 */

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "crypto/aes_ccm.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/network_identity.hpp"
#include "radio/trel_link.hpp"
#include "radio/trel_packet.hpp"
#include "radio/trel_peer.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle.hpp"
#include "thread/neighbor_table.hpp"

namespace ot {
namespace Nexus {

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinTime        = 30 * 1000;
static constexpr uint16_t kMlePort         = 19788;

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

// Builds the MLE UDP payload: suite(0) + SecurityHeader + AES-CCM(cmd+TLVs) + MIC4,
// byte-exact as the genuine sender would emit it.
static uint16_t BuildSecuredMle(Node               &aKeyNode,
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

    aOut[idx++] = 0; // security suite: 802.15.4 security (k154Security)

    secHdr      = &aOut[idx];
    aOut[idx++] = 0x15;                                       // secCtl: EncMic32 | KeyIdMode2
    aOut[idx++] = static_cast<uint8_t>(aFrameCounter & 0xff); // frame counter LE
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 8) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 16) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aFrameCounter >> 24) & 0xff);
    aOut[idx++] = static_cast<uint8_t>((aKeySequence >> 24) & 0xff); // key source BE = keyseq
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

        aesCcm.SetKey(aKeyNode.Get<KeyManager>().GetTemporaryMleKey(aKeySequence));
        aesCcm.SetNonce(nonce);
        aesCcm.SetAuthData(aad, sizeof(aad));
        aesCcm.SetTagLength(4);
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, &aOut[cmdOffset], 1 + aTlvsLength));
    }

    idx += 4; // MIC written by Process at end of data

    return idx;
}

// Builds the MAC-unsecured 802.15.4 frame carrying IPHC(inline src IID, hop-limit
// 255)+UDP+MLE, wraps it in a TREL broadcast packet with the given header Source, and
// injects it at the platform TREL entry point from the given sender socket address.
static void InjectMleOverTrel(Node                  &aReceiver,
                              Core                  &aNexus,
                              const Mac::ExtAddress &aTrelHeaderSource,
                              const Mac::ExtAddress &aMacSource,
                              const otIp6Address    &aSrcIp6,
                              const otIp6Address    &aDstIp6,
                              bool                   aDstIsMulticast,
                              const uint8_t         *aMle,
                              uint16_t               aMleLength,
                              const Ip6::SockAddr   &aSenderSockAddr)
{
    static uint8_t      buf[512];
    ::ot::Trel::Header *header  = reinterpret_cast<::ot::Trel::Header *>(buf);
    uint16_t            hdrSize = ::ot::Trel::Header::GetSize(::ot::Trel::Header::kTypeBroadcast);
    uint8_t            *psdu    = buf + hdrSize;
    uint16_t            idx     = 0;
    uint16_t            panId   = aReceiver.Get<Mac::Mac>().GetPanId();
    uint16_t            udpLen  = 8 + aMleLength;
    uint8_t            *udp;
    uint16_t            csum;
    otSockAddr          sender;

    // MAC header: data | panid-comp | dst=short(broadcast) | ver=2006 | src=ext, UNSECURED
    uint16_t fcf = 0x0001 | 0x0040 | 0x0800 | 0x1000 | 0xC000;

    psdu[idx++] = static_cast<uint8_t>(fcf & 0xff);
    psdu[idx++] = static_cast<uint8_t>(fcf >> 8);
    psdu[idx++] = 0x99; // seq
    psdu[idx++] = static_cast<uint8_t>(panId & 0xff);
    psdu[idx++] = static_cast<uint8_t>(panId >> 8);
    psdu[idx++] = 0xff; // dst short = broadcast
    psdu[idx++] = 0xff;
    for (uint8_t i = 0; i < 8; i++)
    {
        psdu[idx++] = aMacSource.m8[7 - i]; // MAC ext addr little-endian on air
    }

    // IPHC: TF=11 elided, NH=0 inline, HLIM=11 (255); src 128-bit inline;
    // dst 128-bit inline (M bit set when multicast)
    psdu[idx++] = 0x7B;
    psdu[idx++] = aDstIsMulticast ? 0x08 : 0x00;
    psdu[idx++] = 17; // next header: UDP
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
    idx += udpLen; // NOTE: no FCS bytes - TREL frames carry none (Trel::Link::kFcsSize = 0)

    header->Init(::ot::Trel::Header::kTypeBroadcast);
    header->SetChannel(aReceiver.Get<Mac::Mac>().GetPanChannel());
    header->SetPanId(panId);
    header->SetPacketNumber(0x99);
    header->SetSource(aTrelHeaderSource);

    sender.mAddress = aSenderSockAddr.GetAddress();
    sender.mPort    = aSenderSockAddr.GetPort();

    otPlatTrelHandleReceived(&aReceiver.GetInstance(), buf, hdrSize + idx, &sender);
    aNexus.AdvanceTime(200);
}

static void SetLinkLocalFromExt(const Mac::ExtAddress &aExtAddress, otIp6Address &aAddress)
{
    memset(&aAddress, 0, sizeof(aAddress));
    aAddress.mFields.m8[0] = 0xfe;
    aAddress.mFields.m8[1] = 0x80;
    memcpy(&aAddress.mFields.m8[8], aExtAddress.m8, 8);
    aAddress.mFields.m8[8] ^= 0x02; // universal/local bit inversion (IID <-> ext addr)
}

void TestTrelPeerAddrMleLane(void)
{
    Core  nexus;
    Node &nodeA = nexus.CreateNode(); // receiver BR (leader)
    Node &nodeB = nexus.CreateNode(); // legitimate joined device / valid neighbor

    Mac::ExtAddress   fakePeerExtAddr; // P: another TREL peer of A (e.g., second BR)
    Mac::ExtAddress   strangerExtAddr; // X: network device, NOT a valid neighbor of A
    Ip6::SockAddr     sockAddrP;       // P's legitimate sockaddr
    Ip6::SockAddr     sockAddrB;       // B's registered sockaddr
    Ip6::SockAddr     sockAddrB2;      // B's new sockaddr (legit mobility, positive control)
    Ip6::SockAddr     sockAddr2;       // sender sockaddr used for the re-emission case
    otIp6Address      srcIp6, dstIp6;
    ::ot::Trel::Peer *peer;
    uint8_t           mle[128];
    uint16_t          mleLen;
    uint32_t          keySeq;

    // Parent Request TLVs: Mode(0x0f) + Challenge(8B) + Scan Mask(routers) + Version(4)
    static const uint8_t kParentRequestTlvs[] = {
        1,  1, 0x0f,                                           // Mode
        3,  8, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, // Challenge
        14, 1, 0x80,                                           // Scan Mask = routers
        18, 2, 0x00, 0x04                                      // Version = 4
    };

    SuccessOrQuit(nodeA.GetInstance().SetLogLevel(kLogLevelInfo));

    nodeA.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(nodeA.Get<Mle::Mle>().IsLeader());

    nodeB.Join(nodeA);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(nodeB.Get<Mle::Mle>().IsAttached());

    keySeq = nodeA.Get<KeyManager>().GetCurrentKeySequence();

    SuccessOrQuit(sockAddrP.GetAddress().FromString("fd00:abba::1"));
    sockAddrP.SetPort(11111);
    SuccessOrQuit(sockAddrB.GetAddress().FromString("fd00:abba::2"));
    sockAddrB.SetPort(11112);
    SuccessOrQuit(sockAddrB2.GetAddress().FromString("fd00:abba::22"));
    sockAddrB2.SetPort(11113);
    SuccessOrQuit(sockAddr2.GetAddress().FromString("fd00:badd::66"));
    sockAddr2.SetPort(22222);

    for (uint8_t i = 0; i < 8; i++)
    {
        fakePeerExtAddr.m8[i] = 0xB0 + i;
        strangerExtAddr.m8[i] = 0xC0 + i;
    }

    // X must NOT be a valid neighbor of A (such senders get no MLE replay protection).
    VerifyOrQuit(nodeA.Get<NeighborTable>().FindNeighbor(strangerExtAddr) == nullptr);

    // Emulate DNS-SD discovery of TREL peers P and B (nexus has no platform DNS-SD).
    {
        ::ot::Trel::Peer *p = nodeA.Get<::ot::Trel::PeerTable>().AllocateAndAddNewPeer();

        VerifyOrQuit(p != nullptr);
        p->SetInfoForTesting(fakePeerExtAddr, nodeA.Get<MeshCoP::NetworkIdentity>().GetExtPanId(), sockAddrP);

        p = nodeA.Get<::ot::Trel::PeerTable>().AllocateAndAddNewPeer();
        VerifyOrQuit(p != nullptr);
        p->SetInfoForTesting(nodeB.Get<Mac::Mac>().GetExtAddress(), nodeA.Get<MeshCoP::NetworkIdentity>().GetExtPanId(),
                             sockAddrB);
    }

    // ---- Case 1 (CONTROL): corrupted MLE MIC must NOT rebind anything. ----
    Log("Case 1 (control): packet with corrupted MLE MIC - no rebind expected");

    SetLinkLocalFromExt(strangerExtAddr, srcIp6);
    memset(&dstIp6, 0, sizeof(dstIp6));
    dstIp6.mFields.m8[0]  = 0xff; // ff02::2 all-routers (Parent Request destination)
    dstIp6.mFields.m8[1]  = 0x02;
    dstIp6.mFields.m8[15] = 0x02;

    mleLen = BuildSecuredMle(nodeA, srcIp6, dstIp6, keySeq, /* aFrameCounter */ 5,
                             /* kCommandParentRequest */ 9, kParentRequestTlvs, sizeof(kParentRequestTlvs), mle);
    mle[mleLen - 1] ^= 0xFF; // corrupt the MIC

    InjectMleOverTrel(nodeA, nexus, fakePeerExtAddr, fakePeerExtAddr, srcIp6, dstIp6,
                      /* aDstIsMulticast */ true, mle, mleLen, sockAddr2);

    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(fakePeerExtAddr);
    VerifyOrQuit(peer != nullptr);
    Log("  P sockaddr after corrupted-MIC packet: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddrP);

    // ---- Case 2 (POSITIVE): identity-consistent fresh MLE message from valid
    // neighbor B at a new sockaddr -> B's entry must update (peer mobility). ----
    Log("Case 2 (positive control): fresh identity-consistent MLE from valid neighbor B,");
    Log("  new sockaddr - B MUST rebind (peer mobility preserved)");

    SetLinkLocalFromExt(nodeB.Get<Mac::Mac>().GetExtAddress(), srcIp6);

    mleLen = BuildSecuredMle(nodeA, srcIp6, dstIp6, keySeq, /* aFrameCounter */ 0x4000,
                             /* kCommandParentRequest (command handling is irrelevant to
                                the sockaddr lane) */
                             9, kParentRequestTlvs, sizeof(kParentRequestTlvs), mle);

    InjectMleOverTrel(nodeA, nexus, nodeB.Get<Mac::Mac>().GetExtAddress(), nodeB.Get<Mac::Mac>().GetExtAddress(),
                      srcIp6, dstIp6, /* aDstIsMulticast */ true, mle, mleLen, sockAddrB2);

    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    Log("  B sockaddr now: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddrB2);

    // ---- Case 3 (NEGATIVE, the fix): re-emission. MLE payload = X's genuine
    // bytes (carried verbatim); only unverified fields rewritten: TREL header
    // Source = P, MAC source = P, sender sockaddr. MUST NOT rebind P. ----
    Log("Case 3 (fix): TREL header Source = P, MAC source = P (both unverified on a");
    Log("  MAC-unsecured frame), MLE payload = genuine attach-time Parent Request from");
    Log("  non-neighbor X carried verbatim - P MUST NOT be updated directly");

    SetLinkLocalFromExt(strangerExtAddr, srcIp6);

    mleLen = BuildSecuredMle(nodeA, srcIp6, dstIp6, keySeq, /* aFrameCounter */ 5,
                             /* kCommandParentRequest */ 9, kParentRequestTlvs, sizeof(kParentRequestTlvs), mle);

    InjectMleOverTrel(nodeA, nexus, fakePeerExtAddr, fakePeerExtAddr, srcIp6, dstIp6,
                      /* aDstIsMulticast */ true, mle, mleLen, sockAddr2);

    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(fakePeerExtAddr);
    VerifyOrQuit(peer != nullptr);
    Log("  P sockaddr now: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddrP);

    // B's entry must be untouched by the case-3 packet (still at its case-2 sockaddr).
    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    VerifyOrQuit(peer->GetSockAddr() == sockAddrB2);

    Log("TestTrelPeerAddrMleLane passed");
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ot::Nexus::TestTrelPeerAddrMleLane();
    printf("All tests passed\n");
#else
    printf("TREL is not enabled - test skipped\n");
#endif

    return 0;
}
