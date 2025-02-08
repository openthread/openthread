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

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE

#include "common/code_utils.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/time.hpp"
#include "core/instance/instance.hpp"
#include "radio/radio.hpp"

namespace ot {

RegisterLogModule("WakeupTxSched");

WakeupTxScheduler::WakeupTxScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxTimeUs(0)
    , mTxEndTimeUs(0)
    , mTimer(aInstance)
    , mIsRunning(false)
{
    UpdateFrameRequestAhead();
}

Error WakeupTxScheduler::WakeUp(const Mac::ExtAddress &aWedAddress, uint16_t aIntervalUs, uint16_t aDurationMs)
{
    Error error = kErrorNone;

    VerifyOrExit(!mIsRunning, error = kErrorInvalidState);

    mWedAddress  = aWedAddress;
    mTxTimeUs    = TimerMicro::GetNow() + mTxRequestAheadTimeUs;
    mTxEndTimeUs = mTxTimeUs + aDurationMs * Time::kOneMsecInUsec + aIntervalUs;
    mIntervalUs  = aIntervalUs;
    mIsRunning   = true;

    LogInfo("Started wake-up sequence to %s", aWedAddress.ToString().AsCString());

    ScheduleTimer();

exit:
    return error;
}

void WakeupTxScheduler::RequestWakeupFrameTransmission(void) { Get<Mac::Mac>().RequestWakeupFrameTransmission(); }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *WakeupTxScheduler::PrepareWakeupFrame(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame      *frame = nullptr;
    Mac::Address       target;
    Mac::Address       source;
    uint32_t           radioTxDelay;
    uint32_t           rendezvousTimeUs;
    TimeMicro          nowUs = TimerMicro::GetNow();
    Mac::ConnectionIe *connectionIe;

    VerifyOrExit(mIsRunning);

    target.SetExtended(mWedAddress);
    source.SetExtended(Get<Mac::Mac>().GetExtAddress());
    VerifyOrExit(mTxTimeUs >= nowUs);
    radioTxDelay = mTxTimeUs - nowUs;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Mac::kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(frame->GenerateWakeupFrame(Get<Mac::Mac>().GetPanId(), target, source) == kErrorNone, frame = nullptr);
    frame->SetTxDelayBaseTime(static_cast<uint32_t>(Get<Radio>().GetNow()));
    frame->SetTxDelay(radioTxDelay);
    frame->SetCsmaCaEnabled(kWakeupFrameTxCca);
    frame->SetMaxCsmaBackoffs(0);
    frame->SetMaxFrameRetries(0);

    // Rendezvous Time is the time between the end of a wake-up frame and the start of the first payload frame.
    // For the n-th wake-up frame, set the Rendezvous Time so that the expected reception of a Parent Request happens in
    // the "free space" between the "n+1"-th and "n+2"-th wake-up frame.
    rendezvousTimeUs = mIntervalUs;
    rendezvousTimeUs += (mIntervalUs - (kWakeupFrameLength + kParentRequestLength) * kOctetDuration) / 2;
    frame->GetRendezvousTimeIe()->SetRendezvousTime(rendezvousTimeUs / kUsPerTenSymbols);

    connectionIe = frame->GetConnectionIe();
    connectionIe->SetRetryInterval(kConnectionRetryInterval);
    connectionIe->SetRetryCount(kConnectionRetryCount);

    // Advance to the time of the next wake-up frame.
    mTxTimeUs = Max(mTxTimeUs + mIntervalUs, TimerMicro::GetNow() + mTxRequestAheadTimeUs);

    // Schedule the next timer right away before waiting for the transmission completion
    // to keep up with the high rate of wake-up frames in the RCP architecture.
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
        LogInfo("Stopped wake-up sequence");
        ExitNow();
    }

    mTimer.FireAt(mTxTimeUs - mTxRequestAheadTimeUs);

exit:
    return;
}

void WakeupTxScheduler::Stop(void)
{
    mIsRunning = false;
    mTimer.Stop();
}

void WakeupTxScheduler::UpdateFrameRequestAhead(void)
{
    // A rough estimate of the size of data that has to be exchanged with the radio to schedule a wake-up frame TX.
    // This is used to make sure that a wake-up frame is received by the radio early enough to be transmitted on time.
    constexpr uint32_t kWakeupFrameSize = 100;

    mTxRequestAheadTimeUs = Mac::kCslRequestAhead + Get<Mac::Mac>().CalculateRadioBusTransferTime(kWakeupFrameSize);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
