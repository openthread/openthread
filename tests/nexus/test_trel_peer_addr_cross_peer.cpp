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
 * Regression test (MAC lane, cross-peer): a TREL peer socket-address rebind must be
 * authorized only for the peer whose identity MAC frame security actually verified.
 *
 * The peer entry to rebind is selected from the UNAUTHENTICATED TREL encapsulation
 * header Source field, while the authorization comes from MAC frame security. Without
 * the fix, a network-key device could send from its own socket address one fresh
 * key-ID-mode-1 frame whose security-verified source is itself, wrapped in a TREL
 * header carrying Source = a different peer P, and P's entry was rebound to the
 * sender's socket address even though P never sent anything.
 *
 * Cases:
 *  1. NEGATIVE (the fix): TREL header Source = P, fresh mode-1 frame with verified
 *     source B, from a new socket address -> P's entry MUST NOT be updated directly
 *     (and B's entry must be untouched: the frame did not claim B in the TREL header).
 *  2. POSITIVE (mobility preserved; guards against an over-broad always-deny change):
 *     an identity-consistent fresh mode-1 frame (TREL header Source = B, verified
 *     source B) from a NEW socket address MUST update B's entry.
 *
 * FAILS before the fix (case 1 rebinds P); PASSES after.
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
#include "thread/neighbor_table.hpp"

namespace ot {
namespace Nexus {

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinTime        = 30 * 1000;

static uint16_t BuildMode1Frame(Node                  &aReceiverNode,
                                Mac::TxFrame          &aFrame,
                                const Mac::ExtAddress &aSrcExtAddress,
                                uint32_t               aFrameCounter)
{
    Mac::TxFrame::BuildInfo buildInfo;
    Crypto::AesCcm          aesCcm;
    Crypto::AesCcm::Nonce   nonce;
    Mac::KeyMaterial        keyMaterial;
    uint16_t                panId       = aReceiverNode.Get<Mac::Mac>().GetPanId();
    uint32_t                keySequence = aReceiverNode.Get<KeyManager>().GetCurrentKeySequence();
    uint8_t                 payloadIndex;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    aFrame.SetRadioType(::ot::Radio::kTypeTrel);
#endif

    buildInfo.mType    = Mac::Frame::kTypeData;
    buildInfo.mVersion = Mac::Frame::kVersion2006;
    buildInfo.mAddrs.mSource.SetExtended(aSrcExtAddress);
    buildInfo.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
    buildInfo.mPanIds.SetSource(panId);
    buildInfo.mPanIds.SetDestination(panId);
    buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    buildInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;

    buildInfo.PrepareHeadersIn(aFrame);

    aFrame.SetFrameCounter(aFrameCounter);
    aFrame.SetKeyIndex(static_cast<uint8_t>((keySequence & 0x7f) + 1));

    keyMaterial = aReceiverNode.Get<KeyManager>().GetCurrentTrelMacKey();
    nonce.InitFrom(aSrcExtAddress, aFrameCounter, Mac::Frame::kSecurityEncMic32);

    {
        const uint8_t *payload = aFrame.GetPayload();

        VerifyOrQuit(payload != nullptr);
        payloadIndex = static_cast<uint8_t>(payload - aFrame.GetPsdu());
    }

    aesCcm.SetKey(keyMaterial);
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(aFrame.GetPsdu(), payloadIndex);
    aesCcm.SetTagLength(4);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, aFrame.GetPsdu() + payloadIndex, 0));

    return aFrame.GetLength();
}

static void SendTrelPacket(Node                  &aNode,
                           Core                  &aNexus,
                           const Mac::ExtAddress &aTrelHeaderSource,
                           const Mac::ExtAddress &aFrameSrcExtAddress,
                           uint32_t               aFrameCounter,
                           const Ip6::SockAddr   &aSenderAddr)
{
    static uint8_t      buf[200];
    ::ot::Trel::Header *header  = reinterpret_cast<::ot::Trel::Header *>(buf);
    uint16_t            hdrSize = ::ot::Trel::Header::GetSize(::ot::Trel::Header::kTypeBroadcast);
    Radio::Frame        frame;
    uint16_t            frameLen;
    otSockAddr          sender;

    frameLen = BuildMode1Frame(aNode, frame, aFrameSrcExtAddress, aFrameCounter);
    memcpy(buf + hdrSize, frame.GetPsdu(), frameLen);

    header->Init(::ot::Trel::Header::kTypeBroadcast);
    header->SetChannel(aNode.Get<Mac::Mac>().GetPanChannel());
    header->SetPanId(aNode.Get<Mac::Mac>().GetPanId());
    header->SetPacketNumber(0x77);
    header->SetSource(aTrelHeaderSource);

    sender.mAddress = aSenderAddr.GetAddress();
    sender.mPort    = aSenderAddr.GetPort();

    otPlatTrelHandleReceived(&aNode.GetInstance(), buf, hdrSize + frameLen, &sender);
    aNexus.AdvanceTime(100);
}

void TestTrelPeerAddrCrossPeer(void)
{
    Core  nexus;
    Node &nodeA = nexus.CreateNode(); // receiver (leader)
    Node &nodeB = nexus.CreateNode(); // legitimately joined device

    Mac::ExtAddress   fakePeerExtAddr; // P: another TREL peer of A (e.g., second BR)
    Ip6::SockAddr     sockAddrP;       // P's legitimate sockaddr
    Ip6::SockAddr     sockAddrB;       // B's registered sockaddr
    Ip6::SockAddr     sockAddrB2;      // B's new sockaddr (mobility, positive control)
    Ip6::SockAddr     sockAddr2;       // sender sockaddr used for the cross-peer attempt
    ::ot::Trel::Peer *peer;

    SuccessOrQuit(nodeA.GetInstance().SetLogLevel(kLogLevelInfo));

    nodeA.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(nodeA.Get<Mle::Mle>().IsLeader());

    nodeB.Join(nodeA);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(nodeB.Get<Mle::Mle>().IsAttached());

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
    }

    // Emulate DNS-SD discovery of TREL peers P and B (nexus has no platform DNS-SD).
    {
        ::ot::Trel::Peer *p = nodeA.Get<::ot::Trel::PeerTable>().AllocateAndAddNewPeer();

        VerifyOrQuit(p != nullptr);
        p->SetInfoForTesting(fakePeerExtAddr, nodeA.Get<MeshCoP::NetworkIdentity>().GetExtPanId(), sockAddrP);

        p = nodeA.Get<::ot::Trel::PeerTable>().AllocateAndAddNewPeer();
        VerifyOrQuit(p != nullptr);
        p->SetInfoForTesting(nodeB.Get<Mac::Mac>().GetExtAddress(),
                             nodeA.Get<MeshCoP::NetworkIdentity>().GetExtPanId(), sockAddrB);
    }

    Log("Case 1 (fix): TREL header Source = P, fresh mode-1 frame verified as B, new");
    Log("  sockaddr - P MUST NOT be updated directly (cross-peer rebind)");

    SendTrelPacket(nodeA, nexus, fakePeerExtAddr, nodeB.Get<Mac::Mac>().GetExtAddress(),
                   /* aFrameCounter */ 0x00200000, sockAddr2);

    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(fakePeerExtAddr);
    VerifyOrQuit(peer != nullptr);
    Log("  P sockaddr now: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddrP);

    // B's own entry must also be untouched (the frame did not claim B in the TREL header).
    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    VerifyOrQuit(peer->GetSockAddr() == sockAddrB);

    Log("Case 2 (positive control): identity-consistent fresh mode-1 frame from B at a");
    Log("  NEW sockaddr MUST update B's entry (peer mobility preserved)");

    SendTrelPacket(nodeA, nexus, nodeB.Get<Mac::Mac>().GetExtAddress(), nodeB.Get<Mac::Mac>().GetExtAddress(),
                   /* aFrameCounter */ 0x00210000, sockAddrB2);

    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    Log("  B sockaddr now: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddrB2);

    Log("TestTrelPeerAddrCrossPeer passed");
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ot::Nexus::TestTrelPeerAddrCrossPeer();
    printf("All tests passed\n");
#else
    printf("TREL is not enabled - test skipped\n");
#endif

    return 0;
}
