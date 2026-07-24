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
 * Regression test: an unsecured Enh-Ack must not be accepted for a SECURED IEEE
 * 802.15.4-2015 transmission (`Mac::ProcessEnhAckSecurity()`).
 *
 * Setup (nexus): CSL parent->SSED link, so data frames are secured version-2015 and
 * expect Enh-Acks. The test-only nexus ack-intercept hook blocks the frame from
 * reaching the SSED and substitutes an UNSECURED ack for the parent's secured
 * transmission.
 *
 * - CONTROL: baseline UDP parent->SSED delivered; nexus radio generates SECURED
 *   Enh-Acks (the platform mirrors the acked frame's security) and they are accepted.
 * - CASE (fails without the fix): substituted unsecured Enh-Ack for the secured TX.
 *   Without the fix (a) the parent transmits the frame EXACTLY ONCE (no
 *   retransmission, despite the SSED never receiving it), and (b) the parent's
 *   neighbor link info for the SSED absorbs the substituted ack's RSSI (-15 dBm).
 * - Expected behavior (asserted here): the unsecured ack is REJECTED => the parent
 *   RETRANSMITS (attempts >= 2), and the substituted RSSI never reaches the neighbor
 *   entry.
 */

#include <stdio.h>
#include <string.h>

#include <openthread/link.h>
#include <openthread/udp.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "mac/mac.hpp"
#include "thread/child_table.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachAsSsedTime   = 20 * 1000;
static constexpr uint32_t kCslPeriodMs        = 100;
static constexpr uint32_t kCslPeriod          = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;
static constexpr uint32_t kCslSyncTime        = 5 * 1000;
static constexpr int8_t   kReplacementAckRssi = -15;

static bool     sUdpReceived;
static Node    *sParentNode;
static bool     sInterceptArmed;
static bool     sReplaceMode; // true = substitute an unsecured ack; false = block with no ack
static uint32_t sInterceptedAttempts;

static void HandleUdpReceive(void *, otMessage *, const otMessageInfo *) { sUdpReceived = true; }

static Core::AckInterceptResult AckHook(Node &aTxNode, Radio::Frame &aReplacementAck)
{
    Core::AckInterceptResult result  = Core::kPass;
    Mac::TxFrame            &txFrame = aTxNode.mRadio.mTxFrame;

    VerifyOrExit(sInterceptArmed);
    VerifyOrExit(&aTxNode == sParentNode);

    // Target only the parent's SECURED version-2015 data frames requesting ack
    // (the CSL transmissions to the SSED).
    VerifyOrExit(txFrame.GetType() == Mac::Frame::kTypeData);
    VerifyOrExit(txFrame.GetSecurityEnabled() && txFrame.IsVersion2015() && txFrame.GetAckRequest());

    sInterceptedAttempts++;

    if (!sReplaceMode)
    {
        ExitNow(result = Core::kBlockNoAck);
    }

    // Substituted UNSECURED Enh-Ack: FCF = type Ack | version 2015 (sequence
    // present), no addressing, NO security, no IEs. Radio metadata (RSSI/LQI)
    // is chosen distinct from the baseline so any effect on the neighbor link
    // info is observable.
    aReplacementAck.mPsdu[0] = 0x02; // FCF LSB: type = Ack
    aReplacementAck.mPsdu[1] = 0x20; // FCF MSB: version = 2015
    aReplacementAck.mPsdu[2] = txFrame.GetSequence();
    aReplacementAck.mLength  = 3 + 2; // + FCS
    aReplacementAck.mChannel = txFrame.mChannel;

    aReplacementAck.mInfo.mRxInfo.mRssi      = kReplacementAckRssi;
    aReplacementAck.mInfo.mRxInfo.mLqi       = 255;
    aReplacementAck.mInfo.mRxInfo.mTimestamp = 0;

    result = Core::kReplaceAck;

exit:
    return result;
}

static void SendUdpTo(Node &aSender, const Ip6::Address &aDst, const char *aText)
{
    otUdpSocket   socket;
    otMessageInfo msgInfo;
    otMessage    *msg;

    memset(&socket, 0, sizeof(socket));
    memset(&msgInfo, 0, sizeof(msgInfo));

    SuccessOrQuit(otUdpOpen(&aSender.GetInstance(), &socket, nullptr, nullptr));
    msgInfo.mPeerAddr = aDst;
    msgInfo.mPeerPort = 7777;

    msg = otUdpNewMessage(&aSender.GetInstance(), nullptr);
    VerifyOrQuit(msg != nullptr);
    SuccessOrQuit(otMessageAppend(msg, aText, static_cast<uint16_t>(strlen(aText))));
    SuccessOrQuit(otUdpSend(&aSender.GetInstance(), &socket, msg, &msgInfo));
    SuccessOrQuit(otUdpClose(&aSender.GetInstance(), &socket));
}

static int8_t GetParentAverageRssToSsed(Node &aParent, Node &aSsed)
{
    Child *child = aParent.Get<ChildTable>().FindChild(aSsed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);

    VerifyOrQuit(child != nullptr);
    return child->GetLinkInfo().GetAverageRss();
}

void TestUnsecuredEnhAckRejected(void)
{
    Core  nexus;
    Node &parent = nexus.CreateNode();
    Node &ssed   = nexus.CreateNode();

    Log("--------------------------------------------------------------");
    Log("Setup: parent (leader) + SSED with CSL");

    parent.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(parent.Get<Mle::Mle>().IsLeader());

    ssed.Join(parent, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed.Get<Mle::Mle>().IsAttached());

    ssed.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);
    VerifyOrQuit(ssed.Get<Mac::Mac>().IsCslEnabled());

    // SSED binds an application UDP socket.
    static otUdpSocket ssedSocket;
    otSockAddr         sockName;

    memset(&ssedSocket, 0, sizeof(ssedSocket));
    memset(&sockName, 0, sizeof(sockName));
    sockName.mPort = 7777;
    SuccessOrQuit(otUdpOpen(&ssed.GetInstance(), &ssedSocket, HandleUdpReceive, nullptr));
    SuccessOrQuit(otUdpBind(&ssed.GetInstance(), &ssedSocket, &sockName, OT_NETIF_THREAD_INTERNAL));

    sParentNode             = &parent;
    Core::sAckInterceptHook = AckHook;

    Log("--------------------------------------------------------------");
    Log("CONTROL 1: baseline UDP parent->SSED (legit SECURED enh-acks) must deliver");

    sUdpReceived = false;
    SendUdpTo(parent, ssed.Get<Mle::Mle>().GetMeshLocalEid(), "C2-BASE");
    nexus.AdvanceTime(3 * 1000);
    VerifyOrQuit(sUdpReceived);
    Log("baseline delivered (secured enh-ack path healthy)");

    int8_t baselineRss = GetParentAverageRssToSsed(parent, ssed);
    Log("baseline parent->SSED neighbor average RSS: %d dBm", baselineRss);

    Log("--------------------------------------------------------------");
    Log("Case: UNSECURED enh-ack substituted for the parent's SECURED CSL transmission");

    sUdpReceived         = false;
    sInterceptedAttempts = 0;
    sReplaceMode         = true;
    sInterceptArmed      = true;

    SendUdpTo(parent, ssed.Get<Mle::Mle>().GetMeshLocalEid(), "C2-CASE");
    nexus.AdvanceTime(5 * 1000);
    sInterceptArmed = false;

    Log("intercepted TX attempts of the targeted frame: %u", sInterceptedAttempts);
    Log("SSED received: %s", sUdpReceived ? "YES" : "NO");
    Log("parent->SSED neighbor average RSS now: %d dBm (substituted ack RSSI was %d)",
        GetParentAverageRssToSsed(parent, ssed), kReplacementAckRssi);

    VerifyOrQuit(sInterceptedAttempts > 0); // the targeted secured TX happened
    VerifyOrQuit(!sUdpReceived);            // SSED never got the data (frame was intercepted)

    // Expected behavior (fails without the fix): the unsecured ack must be
    // REJECTED, so the parent MUST retransmit (attempts >= 2) instead of
    // treating the frame as delivered...
    VerifyOrQuit(sInterceptedAttempts >= 2);

    // ...and the substituted ack's radio metadata must never enter the neighbor
    // link info: the average RSS must be EXACTLY the earlier baseline (otherwise
    // the average would have moved toward the substituted value).
    VerifyOrQuit(GetParentAverageRssToSsed(parent, ssed) == baselineRss);

    Log("TestUnsecuredEnhAckRejected passed (unsecured enh-ack for secured TX rejected; retransmissions preserved)");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestUnsecuredEnhAckRejected();
    printf("All tests passed\n");
    return 0;
}
