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
#include "openthread/platform/time.h"

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
{
    UpdateFrameRequestAhead();
}

Error WakeupTxScheduler::WakeUp(const Mac::WakeupRequest &aWakeupRequest, uint16_t aIntervalUs, uint16_t aDurationMs)
{
    Error    error = kErrorNone;
    uint32_t rendezvousTimeUs;

    VerifyOrExit(!mTimer.IsRunning(), error = kErrorInvalidState);

    mWakeupRequest = aWakeupRequest;

    mTxTimeUs    = TimerMicro::GetNow() + mTxRequestAheadTimeUs;
    mTxEndTimeUs = mTxTimeUs + aDurationMs * Time::kOneMsecInUsec + aIntervalUs;
    mIntervalUs  = aIntervalUs;

    if (mWakeupRequest.IsWakeupByGroupId())
    {
        rendezvousTimeUs = aDurationMs * Time::kOneMsecInUsec;
    }
    else
    {
        rendezvousTimeUs = aIntervalUs;
    }

    rendezvousTimeUs += (aIntervalUs - (kMaxWakeupFrameLength + kP2pLinkRequestLength) * kOctetDuration) / 2;
    mRendezvousTimeTS = rendezvousTimeUs / kUsPerTenSymbols;

    LogInfo("Started wake-up sequence to %s", aWakeupRequest.ToString().AsCString());

    ScheduleTimer();

exit:
    return error;
}

void WakeupTxScheduler::RequestWakeupFrameTransmission(void) { Get<Mac::Mac>().RequestWakeupFrameTransmission(); }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
Error WakeupTxScheduler::GenerateWakeupFrame(Mac::TxFrame             &aTxFrames,
                                             Mac::PanId                aPanId,
                                             const Mac::WakeupRequest &aWakeupRequest,
                                             const Mac::Address       &aSource)
{
    Error              error = kErrorNone;
    Mac::TxFrame::Info frameInfo;

    VerifyOrExit(aWakeupRequest.IsValid() && !aSource.IsNone(), error = kErrorInvalidArgs);

    frameInfo.mAddrs.mSource = aSource;
    if (aWakeupRequest.IsWakeupById() || aWakeupRequest.IsWakeupByGroupId())
    {
        frameInfo.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        frameInfo.mWakeupIdLength = Mac::GetWakeupIdLength(aWakeupRequest.GetWakeupId());
    }
    else
    {
        frameInfo.mAddrs.mDestination.SetExtended(aWakeupRequest.GetExtAddress());
        frameInfo.mWakeupIdLength = 0;
    }

    frameInfo.mPanIds.SetBothSourceDestination(aPanId);

    frameInfo.mType                   = Mac::Frame::kTypeData;
    frameInfo.mVersion                = Mac::Frame::kVersion2015;
    frameInfo.mSecurityLevel          = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode              = Mac::Frame::kKeyIdMode2;
    frameInfo.mSuppressSequence       = true;
    frameInfo.mAppendRendezvousTimeIe = true;
    frameInfo.mAppendConnectionIe     = true;
    frameInfo.mIsWakeupFrame          = true;

    frameInfo.PrepareHeadersIn(aTxFrames);

exit:

    return error;
}

Mac::TxFrame *WakeupTxScheduler::PrepareWakeupFrame(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame      *frame = nullptr;
    Mac::Address       source;
    uint32_t           radioTxDelay;
    TimeMicro          nowUs = TimerMicro::GetNow();
    Mac::ConnectionIe *connectionIe;

    source.SetExtended(Get<Mac::Mac>().GetExtAddress());
    if (mTxTimeUs >= nowUs)
    {
        radioTxDelay = mTxTimeUs - nowUs;
    }
    else
    {
        radioTxDelay = 0;
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Mac::kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(GenerateWakeupFrame(*frame, Get<Mac::Mac>().GetPanId(), mWakeupRequest, source) == kErrorNone,
                 frame = nullptr);
    frame->SetTxDelayBaseTime(static_cast<uint32_t>(Get<Radio>().GetNow()));
    frame->SetTxDelay(radioTxDelay);
    frame->SetCsmaCaEnabled(kWakeupFrameTxCca);
    frame->SetMaxCsmaBackoffs(0);
    frame->SetMaxFrameRetries(0);
    frame->SetRxChannelAfterTxDone(Get<Mac::Mac>().GetPanChannel());

    // Rendezvous Time is the time between the end of a wake-up frame and the start of the first payload frame.
    // For the n-th wake-up frame, set the Rendezvous Time so that the expected reception of a Parent Request
    // happens in the "free space" between the "n+1"-th and "n+2"-th wake-up frame.
    frame->GetRendezvousTimeIe()->SetRendezvousTime(mRendezvousTimeTS);

    connectionIe = frame->GetConnectionIe();
    connectionIe->SetRetryInterval(kConnectionRetryInterval);
    connectionIe->SetRetryCount(kConnectionRetryCount);

    if (mWakeupRequest.IsWakeupById() || mWakeupRequest.IsWakeupByGroupId())
    {
        VerifyOrExit(connectionIe->SetWakeupId(mWakeupRequest.GetWakeupId()) == kErrorNone, frame = nullptr);
    }

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
        LogInfo("Stopped wake-up sequence");
        ExitNow();
    }

    mTimer.FireAt(mTxTimeUs - mTxRequestAheadTimeUs);

exit:
    return;
}

void WakeupTxScheduler::Stop(void) { mTimer.Stop(); }

void WakeupTxScheduler::UpdateFrameRequestAhead(void)
{
    // A rough estimate of the size of data that has to be exchanged with the radio to schedule a wake-up frame TX.
    // This is used to make sure that a wake-up frame is received by the radio early enough to be transmitted on time.
    constexpr uint32_t kWakeupFrameSize = 100;

    mTxRequestAheadTimeUs = Mac::kCslRequestAhead + Get<Mac::Mac>().CalculateRadioBusTransferTime(kWakeupFrameSize);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
