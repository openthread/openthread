/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "wakeup_tx_scheduler.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

#include "common/code_utils.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/time.hpp"
#include "core/instance/instance.hpp"
#include "mac/sub_mac.hpp"
#include "radio/radio.hpp"

namespace ot {

RegisterLogModule("WakeupTxSched");

WakeupTxScheduler::WakeupTxScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mWakeFrameType(Mac::Frame::kWakeFrameTypeDirectLink)
    , mTxTimeUs(0)
    , mTxEndTimeUs(0)
    , mKeyIndex(Mac::Frame::kWakeKeyIndex)
    , mTimer(aInstance)
    , mConnectionWindowTimer(aInstance)
    , mTdWakeState(kTdWakeIdle)
    , mIsRunning(false)
{
    UpdateFrameRequestAhead();
}

Error WakeupTxScheduler::StartWakeup(const Mac::ExtAddress    &aWlExtAddress,
                                     Mac::Frame::WakeFrameType aWakeType,
                                     uint16_t                  aIntervalUs,
                                     uint16_t                  aDurationMs,
                                     uint8_t                   aKeyIndex)
{
    Error error = kErrorNone;

    VerifyOrExit((aIntervalUs > 0) && (aDurationMs > 0), error = kErrorInvalidArgs);
    VerifyOrExit(aIntervalUs < aDurationMs * Time::kOneMsecInUsec, error = kErrorInvalidArgs);
    VerifyOrExit(mTdWakeState == kTdWakeIdle, error = kErrorInvalidState);
    VerifyOrExit(aKeyIndex == Mac::Frame::kWakeKeyIndex || (aKeyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN &&
                                                            aKeyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX),
                 error = kErrorInvalidArgs);

    mKeyIndex = aKeyIndex;

    // Inform SubMac of the key index so ProcessTransmitSecurity stamps the correct key ID.
    Get<Mac::SubMac>().SetActiveBurstWakeKeyIndex(aKeyIndex);

    SuccessOrExit(error = WakeUp(aWlExtAddress, aWakeType, aIntervalUs, aDurationMs));

    mTdWakeState = kTdWaking;

    if (aWakeType == Mac::Frame::kWakeFrameTypeDirectLink)
    {
        // Open the connection window: WI waits for the WL's TD Link Command.
        mConnectionWindowTimer.FireAt(mTxEndTimeUs + GetConnectionWindowUs());
        LogInfo("TD connection window open");
    }
    else
    {
        // Non-link wake: no connection window needed.
        mTdWakeState = kTdWakeIdle;
    }

exit:
    return error;
}

void WakeupTxScheduler::HandleConnectionWindowTimer(void)
{
    if (mTdWakeState == kTdWaking)
    {
        LogInfo("TD connection window closed without response");
        mTdWakeState = kTdWakeIdle;
        Get<Mac::Mac>().InvokeDirectEvent(OT_THREAD_DIRECT_EVENT_LINK_FAILED, nullptr);
    }
}

Error WakeupTxScheduler::WakeUp(const Mac::ExtAddress    &aPeerExtAddress,
                                Mac::Frame::WakeFrameType aWakeType,
                                uint16_t                  aIntervalUs,
                                uint16_t                  aDurationMs)
{
    Error error = kErrorNone;

    VerifyOrExit(!mIsRunning, error = kErrorInvalidState);

    mPeerExtAddress = aPeerExtAddress;
    mWakeFrameType  = aWakeType;
    mTxTimeUs       = TimerMicro::GetNow() + mTxRequestAheadTimeUs;
    mTxEndTimeUs    = mTxTimeUs + aDurationMs * Time::kOneMsecInUsec + aIntervalUs;
    mIntervalUs     = aIntervalUs;
    mIsRunning      = true;

    LogInfo("Started Wake Frame sequence to %s", aPeerExtAddress.ToString().AsCString());

    ScheduleTimer();

exit:
    return error;
}

void WakeupTxScheduler::RequestWakeupFrameTransmission(void) { Get<Mac::Mac>().RequestWakeupFrameTransmission(); }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *WakeupTxScheduler::PrepareWakeupFrame(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;
    uint32_t      remainingUs;
    uint8_t       rendezvousTimeTenSym;

    VerifyOrExit(mIsRunning);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Mac::kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    remainingUs          = (mTxEndTimeUs > mTxTimeUs) ? static_cast<uint32_t>(mTxEndTimeUs - mTxTimeUs) : 0;
    rendezvousTimeTenSym = static_cast<uint8_t>(Min(remainingUs / kUsPerTenSymbols, static_cast<uint32_t>(0xffu)));

    IgnoreError(frame->GenerateThreadDirectWakeCommand(
        Get<Mac::Mac>().GetPanId(), mPeerExtAddress, Get<Mac::Mac>().GetExtAddress(), mWakeFrameType,
        rendezvousTimeTenSym, kConnectionRetryInterval, kConnectionRetryCount));

    frame->SetChannel(Get<Mac::Mac>().GetWakeupChannel());
    frame->SetCsmaCaEnabled(kWakeFrameTxCca);
    frame->SetMaxCsmaBackoffs(0);
    frame->SetMaxFrameRetries(0);

    mTxTimeUs = Max(mTxTimeUs + mIntervalUs, TimerMicro::GetNow() + mTxRequestAheadTimeUs);
    ScheduleTimer();

exit:
    return frame;
}

#else // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *WakeupTxScheduler::PrepareWakeupFrame(Mac::TxFrames &) { return nullptr; }

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void WakeupTxScheduler::ScheduleTimer(void)
{
    if (mTxTimeUs >= mTxEndTimeUs)
    {
        mIsRunning = false;
        LogInfo("Stopped Wake Frame sequence");

        // After a burst, reset active key index to default kWakeKeyIndex.
        Get<Mac::SubMac>().SetActiveBurstWakeKeyIndex(Mac::Frame::kWakeKeyIndex);

        ExitNow();
    }

    mTimer.FireAt(mTxTimeUs - mTxRequestAheadTimeUs);

exit:
    return;
}

void WakeupTxScheduler::Stop(void)
{
    mIsRunning   = false;
    mTdWakeState = kTdWakeIdle;
    mTimer.Stop();
    mConnectionWindowTimer.Stop();

    // Reset active key index to default kWakeKeyIndex.
    Get<Mac::SubMac>().SetActiveBurstWakeKeyIndex(Mac::Frame::kWakeKeyIndex);
}

void WakeupTxScheduler::UpdateFrameRequestAhead(void)
{
    // Rough estimate of data exchanged with the radio to schedule a Wake Frame TX.
    constexpr uint32_t kWakeFrameSize = 100;

    mTxRequestAheadTimeUs = Mac::kCslRequestAhead + Get<Mac::Mac>().CalculateRadioBusTransferTime(kWakeFrameSize);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
