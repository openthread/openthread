/*
 * Regression test: TREL peer socket-address updates must be authorized only by frames
 * secured with the network key (key ID mode 1) or KEK (mode 0) - NOT by frames secured
 * with the well-known key-ID-mode-2 key (which provides no sender authentication)
 * and NOT by unsecured frames.
 *
 * Cases:
 *  1. NEGATIVE (the fix): a mode-2 "secured" frame whose source does not correspond to
 *     the TREL header Source, sent from a new sockaddr, MUST NOT update.
 *  2. NEGATIVE: an unsecured frame shaped the same way MUST NOT update.
 *  3. POSITIVE (mobility preserved; guards against an over-broad always-deny change):
 *     a genuine key-ID-mode-1-secured frame from the peer arriving from a NEW sockaddr
 *     MUST update the peer entry (peer mobility). MLE-secured messages additionally
 *     retain sockaddr updates via the MLE receive path (`Mle` calls
 *     `CheckPeerAddrOnRxSuccess(kAllowPeerSockAddrUpdate)` after MLE security passes),
 *     which this change does not touch.
 *
 * FAILS before the fix (case 1 updates the sockaddr); PASSES after.
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
#include "thread/neighbor_table.hpp"

namespace ot {
namespace Nexus {

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinTime        = 30 * 1000;

// IEEE 802.15.4 / Thread well-known key-ID-mode-2 constants (public; every device
// compiles them in, see `Mac::kMode2Key` / `Mac::kMode2ExtAddress`).
static const otExtAddress kMode2ExtAddress = {{0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12}};
static const otMacKey     kMode2Key        = {
    {0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f, 0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66}};
static const uint8_t kMode2KeySource[] = {0xff, 0xff, 0xff, 0xff};

enum FrameSecurity : uint8_t
{
    kFrameUnsecured,
    kFrameMode2Secured, // well-known key (constructible without network credentials)
    kFrameMode1Secured, // genuine network-key security
};

// Builds an empty-payload data frame from `aSrcExtAddress` using the public
// `TxFrame::BuildInfo` API (no hand-rolled FCF bytes). TREL frames carry no FCS.
static uint16_t BuildFrame(Node                  &aReceiverNode,
                           Mac::TxFrame          &aFrame,
                           const Mac::ExtAddress &aSrcExtAddress,
                           FrameSecurity          aSecurity)
{
    Mac::TxFrame::BuildInfo buildInfo;
    Crypto::AesCcm          aesCcm;
    Crypto::AesCcm::Nonce   nonce;
    Mac::KeyMaterial        keyMaterial;
    uint16_t                panId        = aReceiverNode.Get<Mac::Mac>().GetPanId();
    uint32_t                frameCounter = 0x00100000; // above any stored neighbor counter
    uint8_t                 payloadIndex;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    aFrame.SetRadioType(::ot::Radio::kTypeTrel); // TREL: no FCS in PSDU
#endif

    buildInfo.mType    = Mac::Frame::kTypeData;
    buildInfo.mVersion = Mac::Frame::kVersion2006;
    buildInfo.mAddrs.mSource.SetExtended(aSrcExtAddress);
    buildInfo.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
    buildInfo.mPanIds.SetSource(panId);
    buildInfo.mPanIds.SetDestination(panId);

    if (aSecurity != kFrameUnsecured)
    {
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
        buildInfo.mKeyIdMode = (aSecurity == kFrameMode2Secured) ? Mac::Frame::kKeyIdMode2 : Mac::Frame::kKeyIdMode1;
    }

    buildInfo.PrepareHeadersIn(aFrame);

    if (aSecurity == kFrameUnsecured)
    {
        return aFrame.GetLength();
    }

    aFrame.SetFrameCounter(frameCounter);

    if (aSecurity == kFrameMode2Secured)
    {
        aFrame.SetKeySource(kMode2KeySource);
        aFrame.SetKeyIndex(1);
        keyMaterial.SetFrom(AsCoreType(&kMode2Key));
        nonce.InitFrom(AsCoreType(&kMode2ExtAddress), frameCounter, Mac::Frame::kSecurityEncMic32);
    }
    else
    {
        // Genuine mode-1 security: TREL frames are secured with the TREL MAC key
        // (`KeyManager::GetCurrentTrelMacKey()`), exactly as the real peer would.
        uint32_t keySequence = aReceiverNode.Get<KeyManager>().GetCurrentKeySequence();

        aFrame.SetKeyIndex(static_cast<uint8_t>((keySequence & 0x7f) + 1));
        keyMaterial = aReceiverNode.Get<KeyManager>().GetCurrentTrelMacKey();
        nonce.InitFrom(aSrcExtAddress, frameCounter, Mac::Frame::kSecurityEncMic32);
    }

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
                           const Ip6::SockAddr   &aSenderAddr,
                           FrameSecurity          aSecurity)
{
    static uint8_t      buf[200];
    ::ot::Trel::Header *header  = reinterpret_cast<::ot::Trel::Header *>(buf);
    uint16_t            hdrSize = ::ot::Trel::Header::GetSize(::ot::Trel::Header::kTypeBroadcast);
    Radio::Frame        frame;
    uint16_t            frameLen;
    otSockAddr          sender;

    frameLen = BuildFrame(aNode, frame, aFrameSrcExtAddress, aSecurity);
    memcpy(buf + hdrSize, frame.GetPsdu(), frameLen);

    header->Init(::ot::Trel::Header::kTypeBroadcast);
    header->SetChannel(aNode.Get<Mac::Mac>().GetPanChannel());
    header->SetPanId(aNode.Get<Mac::Mac>().GetPanId());
    header->SetPacketNumber(0x42);
    header->SetSource(aTrelHeaderSource);

    sender.mAddress = aSenderAddr.mAddress;
    sender.mPort    = aSenderAddr.mPort;

    otPlatTrelHandleReceived(&aNode.GetInstance(), buf, hdrSize + frameLen, &sender);
    aNexus.AdvanceTime(100);
}

void TestTrelPeerSockAddrUpdateSecurity(void)
{
    Core  nexus;
    Node &nodeA = nexus.CreateNode(); // leader
    Node &nodeB = nexus.CreateNode(); // genuine peer / neighbor

    Mac::ExtAddress   otherExtAddr;
    Ip6::SockAddr     sockAddr1; // peer B's legitimate TREL sockaddr
    Ip6::SockAddr     sockAddr2; // unrelated sockaddr
    Ip6::SockAddr     sockAddr3; // peer B's NEW sockaddr (mobility)
    ::ot::Trel::Peer *peer;

    SuccessOrQuit(nodeA.GetInstance().SetLogLevel(kLogLevelInfo));

    nodeA.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(nodeA.Get<Mle::Mle>().IsLeader());

    nodeB.Join(nodeA);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(nodeB.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(nodeA.Get<NeighborTable>().FindNeighbor(nodeB.Get<Mac::Mac>().GetExtAddress()) != nullptr);

    SuccessOrQuit(sockAddr1.GetAddress().FromString("fd00:abba::1"));
    sockAddr1.SetPort(11111);
    SuccessOrQuit(sockAddr2.GetAddress().FromString("fd00:badd::66"));
    sockAddr2.SetPort(22222);
    SuccessOrQuit(sockAddr3.GetAddress().FromString("fd00:abba::99"));
    sockAddr3.SetPort(33333);

    for (uint8_t i = 0; i < 8; i++)
    {
        otherExtAddr.m8[i] = 0xC0 + i;
    }

    // Emulate DNS-SD discovery of peer B at `sockAddr1` (nexus has no platform DNS-SD).
    {
        ::ot::Trel::Peer *newPeer = nodeA.Get<::ot::Trel::PeerTable>().AllocateAndAddNewPeer();

        VerifyOrQuit(newPeer != nullptr);
        newPeer->SetInfoForTesting(nodeB.Get<Mac::Mac>().GetExtAddress(),
                                   nodeA.Get<MeshCoP::NetworkIdentity>().GetExtPanId(), sockAddr1);
    }

    Log("Case 1 (fix): key-ID-mode-2 frame with mismatched source from unrelated sockaddr MUST NOT update");

    SendTrelPacket(nodeA, nexus, nodeB.Get<Mac::Mac>().GetExtAddress(), otherExtAddr, sockAddr2, kFrameMode2Secured);
    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    Log("  peer sockaddr: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddr1);

    Log("Case 2: unsecured frame with mismatched source from unrelated sockaddr MUST NOT update");

    SendTrelPacket(nodeA, nexus, nodeB.Get<Mac::Mac>().GetExtAddress(), otherExtAddr, sockAddr2, kFrameUnsecured);
    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    VerifyOrQuit(peer->GetSockAddr() == sockAddr1);

    Log("Case 3 (positive control): genuine mode-1 frame from B at NEW sockaddr MUST update");

    SendTrelPacket(nodeA, nexus, nodeB.Get<Mac::Mac>().GetExtAddress(), nodeB.Get<Mac::Mac>().GetExtAddress(),
                   sockAddr3, kFrameMode1Secured);
    peer = nodeA.Get<::ot::Trel::PeerTable>().FindMatching(nodeB.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer != nullptr);
    Log("  peer sockaddr: %s", peer->GetSockAddr().ToString().AsCString());
    VerifyOrQuit(peer->GetSockAddr() == sockAddr3);

    Log("TestTrelPeerSockAddrUpdateSecurity passed");
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ot::Nexus::TestTrelPeerSockAddrUpdateSecurity();
    printf("All tests passed\n");
#else
    printf("TREL is not enabled - test skipped\n");
#endif

    return 0;
}
