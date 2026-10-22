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

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

struct RxContext : public Clearable<RxContext>
{
    RxContext(void) { Clear(); }

    bool     mReceived;
    uint64_t mTimestamp;
    uint8_t  mChannel;
};

static void HandlePcapRx(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    RxContext *context = static_cast<RxContext *>(aContext);

    if (!aIsTx)
    {
        context->mReceived  = true;
        context->mTimestamp = aFrame->mInfo.mRxInfo.mTimestamp;
        context->mChannel   = aFrame->mChannel;
    }
}

void TestScheduledTx(void)
{
    static constexpr uint8_t           kNode0RxChannel     = 15;
    static constexpr uint8_t           kScheduledTxChannel = 11;
    static constexpr Mac::PanId        kPanId              = 0xabcd;
    static constexpr Mac::ShortAddress kNode0ShortAddr     = 0x1000;
    static constexpr Mac::ShortAddress kNode1ShortAddr     = 0x2000;

    Core                    nexus;
    Node                   &node0   = nexus.CreateNode();
    Node                   &node1   = nexus.CreateNode();
    Mac::TxFrame           &txFrame = node0.Get<ot::Radio::Radio>().GetScheduledTxFrame();
    Mac::TxFrame::BuildInfo buildInfo;
    RxContext               rxContext;
    uint64_t                txTime;

    node0.SetName("Node0");
    node1.SetName("Node1");

    AllowLinkBetween(node0, node1);

    nexus.AdvanceTime(0);

    VerifyOrQuit((node0.Get<ot::Radio::Radio>().GetCaps() & ot::Radio::kCapScheduledTx) != 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Verify ScheduleTx fails when radio is disabled");

    SuccessOrQuit(node0.Get<ot::Radio::Radio>().Disable());
    VerifyOrQuit(!node0.Get<ot::Radio::Radio>().IsEnabled());
    VerifyOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(1000) == kErrorInvalidState);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Enable radios and prepare scheduled TX frame");

    SuccessOrQuit(node0.Get<ot::Radio::Radio>().Enable());
    VerifyOrQuit(node0.Get<ot::Radio::Radio>().IsEnabled());

    node0.Get<ot::Radio::Radio>().SetPanId(kPanId);
    node0.Get<ot::Radio::Radio>().SetShortAddress(kNode0ShortAddr);
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().Receive(kNode0RxChannel));

    node1.Get<ot::Radio::Radio>().SetPanId(kPanId);
    node1.Get<ot::Radio::Radio>().SetShortAddress(kNode1ShortAddr);
    node1.Get<Mac::SubMac>().SetPcapCallback(HandlePcapRx, &rxContext);
    SuccessOrQuit(node1.Get<ot::Radio::Radio>().Receive(kScheduledTxChannel));

    buildInfo.mType    = Mac::Frame::kTypeData;
    buildInfo.mVersion = Mac::Frame::kVersion2006;
    buildInfo.mAddrs.mSource.SetShort(kNode0ShortAddr);
    buildInfo.mAddrs.mDestination.SetShort(kNode1ShortAddr);
    buildInfo.mPanIds.SetBothSourceDestination(kPanId);
    txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    txFrame.SetChannel(kScheduledTxChannel);
    txFrame.SetIsHeaderUpdated(true);
    txFrame.SetIsSecurityProcessed(true);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Verify busy check and CancelScheduledTx");

    txTime = node0.Get<ot::Radio::Radio>().GetNow() + 2500;
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(txTime));
    VerifyOrQuit(node0.mRadio.mScheduledTxPending);

    VerifyOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(txTime + 1000) == kErrorBusy);

    SuccessOrQuit(node0.Get<ot::Radio::Radio>().CancelScheduledTx());
    VerifyOrQuit(!node0.mRadio.mScheduledTxPending);

    nexus.AdvanceTime(5);
    VerifyOrQuit(!rxContext.mReceived);

    // Calling CancelScheduledTx when no schedule is active should succeed as a no-op.
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().CancelScheduledTx());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Verify scheduled TX executes at exact target time and preserves state/channel");

    txTime = node0.Get<ot::Radio::Radio>().GetNow() + 2500;
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(txTime));

    // Advance 2 ms (2000 us) - still before target time (+2500 us).
    nexus.AdvanceTime(2);
    VerifyOrQuit(node0.mRadio.mScheduledTxPending);
    VerifyOrQuit(!rxContext.mReceived);

    // Advance 1 ms (to +3000 us, crossing +2500 us).
    nexus.AdvanceTime(1);
    VerifyOrQuit(!node0.mRadio.mScheduledTxPending);
    VerifyOrQuit(rxContext.mReceived);
    VerifyOrQuit(rxContext.mTimestamp == txTime);
    VerifyOrQuit(rxContext.mChannel == kScheduledTxChannel);
    VerifyOrQuit(txFrame.mInfo.mTxInfo.mTimestamp == txTime);

    // Verify Node0's radio state and receive channel were preserved.
    VerifyOrQuit(node0.Get<ot::Radio::Radio>().GetState() == OT_RADIO_STATE_RECEIVE);
    VerifyOrQuit(node0.mRadio.mChannel == kNode0RxChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify past target time aborts asynchronously without transmitting");

    rxContext.Clear();

    txTime = node0.Get<ot::Radio::Radio>().GetNow() - 100;
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(txTime));
    VerifyOrQuit(node0.mRadio.mScheduledTxPending);

    nexus.AdvanceTime(1);
    VerifyOrQuit(!node0.mRadio.mScheduledTxPending);
    VerifyOrQuit(!rxContext.mReceived);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Verify disabling radio cancels pending scheduled TX");

    txTime = node0.Get<ot::Radio::Radio>().GetNow() + 2000;
    SuccessOrQuit(node0.Get<ot::Radio::Radio>().ScheduleTx(txTime));
    VerifyOrQuit(node0.mRadio.mScheduledTxPending);

    SuccessOrQuit(node0.Get<ot::Radio::Radio>().Disable());
    VerifyOrQuit(!node0.mRadio.mScheduledTxPending);

    nexus.AdvanceTime(5);
    VerifyOrQuit(!rxContext.mReceived);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestScheduledTx();
    printf("All tests passed\n");
    return 0;
}
