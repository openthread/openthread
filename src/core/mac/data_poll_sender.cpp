/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

/**
 * @file
 *   This file implements data poll (mac data request command) sender class.
 */

#include "data_poll_sender.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("DataPollSender");

DataPollSender::DataPollSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimerStartTime(0)
    , mPollPeriod(0)
    , mExternalPollPeriod(0)
    , mFastPollsUsers(0)
    , mTimer(aInstance)
    , mEnabled(false)
    , mAttachMode(false)
    , mRetxMode(false)
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    ,mIsRadioPollRunning(false)
    ,mDelayNextPollSchedule(false)
#endif
    , mPollTimeoutCounter(0)
    , mPollTxFailureCounter(0)
    , mRemainingFastPolls(0)
{
}

const Neighbor &DataPollSender::GetParent(void) const
{
    const Neighbor &parentCandidate = Get<Mle::Mle>().GetParentCandidate();

    return parentCandidate.IsStateValid() ? parentCandidate : Get<Mle::Mle>().GetParent();
}

void DataPollSender::StartPolling(void)
{
    VerifyOrExit(!mEnabled);

    OT_ASSERT(!Get<Mle::Mle>().IsRxOnWhenIdle());

    mEnabled = true;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return;
}

void DataPollSender::StopPolling(void)
{
    StopPollTimer();

    mAttachMode            = false;
    mRetxMode              = false;
    mPollTimeoutCounter    = 0;
    mPollTxFailureCounter  = 0;
    mRemainingFastPolls    = 0;
    mFastPollsUsers        = 0;
    mEnabled               = false;
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    mDelayNextPollSchedule = false;
#endif
}

Error DataPollSender::SendDataPoll(void)
{
    Error error;

    VerifyOrExit(mEnabled, error = kErrorInvalidState);
    VerifyOrExit(!Get<Mac::Mac>().GetRxOnWhenIdle(), error = kErrorInvalidState);

    VerifyOrExit(GetParent().IsStateValidOrRestoring(), error = kErrorInvalidState);

    StopPollTimer();

    SuccessOrExit(error = Get<Mac::Mac>().RequestDataPollTransmission());

exit:

    switch (error)
    {
    case kErrorNone:
        LogDebg("Sending data poll");
        ScheduleNextPoll(kUsePreviousPollPeriod);
        break;

    case kErrorInvalidState:
        LogWarn("Data poll tx requested while data polling was not enabled!");
        StopPolling();
        break;

    default:
        LogWarn("Unexpected error %s requesting data poll", ErrorToString(error));
        ScheduleNextPoll(kRecalculatePollPeriod);
        break;
    }

    return error;
}

#if OPENTHREAD_CONFIG_MULTI_RADIO
Error DataPollSender::GetPollDestinationAddress(Mac::Address &aDest, Mac::RadioType &aRadioType) const
#else
Error DataPollSender::GetPollDestinationAddress(Mac::Address &aDest) const
#endif
{
    Error           error  = kErrorNone;
    const Neighbor &parent = GetParent();

    VerifyOrExit(parent.IsStateValidOrRestoring(), error = kErrorAbort);

    // Use extended address attaching to a new parent (i.e. parent is the parent candidate).
    if ((Get<Mac::Mac>().GetShortAddress() == Mac::kShortAddrInvalid) ||
        (&parent == &Get<Mle::Mle>().GetParentCandidate()))
    {
        aDest.SetExtended(parent.GetExtAddress());
    }
    else
    {
        aDest.SetShort(parent.GetRloc16());
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    aRadioType = Get<RadioSelector>().SelectPollFrameRadio(parent);
#endif

exit:
    return error;
}

Error DataPollSender::SetExternalPollPeriod(uint32_t aPeriod)
{
    Error error = kErrorNone;

    if (aPeriod != 0)
    {
        VerifyOrExit(aPeriod >= OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD, error = kErrorInvalidArgs);

        aPeriod = Min(aPeriod, kMaxExternalPeriod);
    }

    if (mExternalPollPeriod != aPeriod)
    {
        mExternalPollPeriod = aPeriod;

        if (mEnabled)
        {
            ScheduleNextPoll(kRecalculatePollPeriod);
        }
    }

exit:
    return error;
}

uint32_t DataPollSender::GetKeepAlivePollPeriod(void) const
{
    uint32_t period = GetDefaultPollPeriod();

    if (mExternalPollPeriod != 0)
    {
        period = Min(period, mExternalPollPeriod);
    }

    return period;
}

#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
void DataPollSender::HandlePollSent(Mac::TxFrame &aFrame, Error aError, uint32_t aNbOfPolls)
#else
void  DataPollSender::HandlePollSent(Mac::TxFrame &aFrame, Error aError)
#endif
{
    Mac::Address macDest;
    bool         shouldRecalculatePollPeriod = false;

    VerifyOrExit(mEnabled);

    if (!aFrame.IsEmpty())
    {
        IgnoreError(aFrame.GetDstAddr(macDest));
        Get<MeshForwarder>().UpdateNeighborOnSentFrame(aFrame, aError, macDest, /* aIsDataPoll */ true);
    }

    if (GetParent().IsStateInvalid())
    {
        StopPolling();
        IgnoreError(Get<Mle::Mle>().BecomeDetached());
        ExitNow();
    }

#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
        // Update the start time with the number of offloaded polls
        if (aNbOfPolls)
        {
            mTimerStartTime += aNbOfPolls * mPollPeriod;
        }

        // When the poll schedule delay flag is set it means that the poll offload has stopped
        // and the start time has been updated. The next poll can be scheduled at this point 
        // without any other state processing.
        if (mDelayNextPollSchedule)
        {
            shouldRecalculatePollPeriod = true;
            ExitNow();
        }

        // If the Radio Poll Offload is enabled and the last transaction completed with an error
        // the radio poll offload flag is set to false, and the processing of the error will continue
        // as before. 
        if (mIsRadioPollRunning && (aError != kErrorNone))
        {
            mIsRadioPollRunning = false;
        }
#endif

    switch (aError)
    {
    case kErrorNone:
        if (mRemainingFastPolls != 0)
        {
            mRemainingFastPolls--;

            if (mRemainingFastPolls == 0)
            {
                shouldRecalculatePollPeriod = true;
                mFastPollsUsers             = 0;
            }
        }

        if (mRetxMode)
        {
            mRetxMode                   = false;
            mPollTxFailureCounter       = 0;
            shouldRecalculatePollPeriod = true;
        }

#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
        if (mIsRadioPollRunning)
        {
            // If the poll offload is enabled, restart the offload with the recalculated start time
            StartPollTimer(mTimerStartTime, mPollPeriod);
        }
#endif
        break;

    case kErrorChannelAccessFailure:
    case kErrorAbort:
        mRetxMode                   = true;
        shouldRecalculatePollPeriod = true;
        break;

    default:
        mPollTxFailureCounter++;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        LogInfo("Failed to send data poll, error:%s, retx:%d/%d", ErrorToString(aError), mPollTxFailureCounter,
                aFrame.HasCslIe() ? kMaxCslPollRetxAttempts : kMaxPollRetxAttempts);
#else
        LogInfo("Failed to send data poll, error:%s, retx:%d/%d", ErrorToString(aError), mPollTxFailureCounter,
                 kMaxPollRetxAttempts);
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (mPollTxFailureCounter < (aFrame.HasCslIe() ? kMaxCslPollRetxAttempts : kMaxPollRetxAttempts))
#else
        if (mPollTxFailureCounter < kMaxPollRetxAttempts)
#endif
        {
            if (!mRetxMode)
            {
                mRetxMode                   = true;
                shouldRecalculatePollPeriod = true;
            }
        }
        else
        {
            mRetxMode                   = false;
            mPollTxFailureCounter       = 0;
            shouldRecalculatePollPeriod = true;
        }

        break;
    }

exit:

    if (shouldRecalculatePollPeriod)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }
    return;
}

void DataPollSender::HandlePollTimeout(void)
{
    // A data poll timeout happened, i.e., the ack in response to
    // a data poll indicated that a frame was pending, but no frame
    // was received after timeout interval.

    VerifyOrExit(mEnabled);

    mPollTimeoutCounter++;

    LogInfo("Data poll timeout, retry:%d/%d", mPollTimeoutCounter, kQuickPollsAfterTimeout);

    if (mPollTimeoutCounter < kQuickPollsAfterTimeout)
    {
        IgnoreError(SendDataPoll());
    }
    else
    {
        mPollTimeoutCounter = 0;
    }

exit:
    return;
}

void DataPollSender::ProcessRxFrame(const Mac::RxFrame &aFrame)
{
    VerifyOrExit(mEnabled);

    mPollTimeoutCounter = 0;

    if (aFrame.GetFramePending())
    {
        IgnoreError(SendDataPoll());
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
void DataPollSender::ProcessTxDone(const Mac::TxFrame &aFrame, const Mac::RxFrame *aAckFrame, Error aError)
{
    bool sendDataPoll = false;

    VerifyOrExit(mEnabled);
    VerifyOrExit(Get<Mle::Mle>().GetParent().IsEnhancedKeepAliveSupported());
    VerifyOrExit(aFrame.GetSecurityEnabled());

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (aFrame.mInfo.mTxInfo.mIsARetx && aFrame.HasCslIe())
    {
        // For retransmission frame, use a data poll to resync its parent with correct CSL phase
        sendDataPoll = true;
    }
#endif

    if (aError == kErrorNone && aAckFrame != nullptr)
    {
        mPollTimeoutCounter = 0;

        if (aAckFrame->GetFramePending())
        {
            sendDataPoll = true;
        }
        else
        {
            ResetKeepAliveTimer();
        }
    }

    if (sendDataPoll)
    {
        IgnoreError(SendDataPoll());
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

void DataPollSender::RecalculatePollPeriod(void)
{
    if (mEnabled)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }
}

void DataPollSender::SetAttachMode(bool aMode)
{
    if (mAttachMode != aMode)
    {
        mAttachMode = aMode;

        if (mEnabled)
        {
            ScheduleNextPoll(kRecalculatePollPeriod);
        }
    }
}

void DataPollSender::SendFastPolls(uint8_t aNumFastPolls)
{
    bool shouldRecalculatePollPeriod = (mRemainingFastPolls == 0);

    if (mFastPollsUsers < kMaxFastPollsUsers)
    {
        mFastPollsUsers++;
    }

    if (aNumFastPolls == 0)
    {
        aNumFastPolls = kDefaultFastPolls;
    }

    aNumFastPolls       = Min(aNumFastPolls, kMaxFastPolls);
    mRemainingFastPolls = Max(mRemainingFastPolls, aNumFastPolls);

    if (mEnabled && shouldRecalculatePollPeriod)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }
}

void DataPollSender::StopFastPolls(void)
{
    VerifyOrExit(mFastPollsUsers != 0);

    // If `mFastPollsUsers` hits the max, let it be cleared
    // from `HandlePollSent()` (after all fast polls are sent).
    VerifyOrExit(mFastPollsUsers < kMaxFastPollsUsers);

    mFastPollsUsers--;

    VerifyOrExit(mFastPollsUsers == 0);

    mRemainingFastPolls = 0;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return;
}

void DataPollSender::ResetKeepAliveTimer(void)
{
    if (IsPollTimerRunning() && mPollPeriod == GetDefaultPollPeriod())
    {
        mTimerStartTime = TimerMilli::GetNow();
        StartPollTimer(mTimerStartTime, mPollPeriod);
    }
}

void DataPollSender::ScheduleNextPoll(PollPeriodSelector aPollPeriodSelector)
{
    TimeMilli now;
    uint32_t  oldPeriod = mPollPeriod;

    if (aPollPeriodSelector == kRecalculatePollPeriod)
    {
        mPollPeriod = CalculatePollPeriod();
    }

    now = TimerMilli::GetNow();

    if (IsPollTimerRunning())
    {
        if (ShouldURestartTimer(oldPeriod))
        {
            // If poll interval did change and re-starting the timer from
            // last start time with new poll interval would fire quickly
            // (i.e., fires within window `[now, now + kMinPollPeriod]`)
            // add an extra minimum delay of `kMinPollPeriod`. This
            // ensures that when an internal or external request triggers
            // a switch to a shorter poll interval, the first data poll
            // will not be sent too quickly (and possibly before the
            // response is available/prepared on the parent node).

            if (mTimerStartTime + mPollPeriod < now + kMinPollPeriod)
            {
                StartPollTimer(now, kMinPollPeriod);
            }
            else
            {
                StartPollTimer(mTimerStartTime, mPollPeriod);
            }
        }
        // Do nothing on the running poll timer if the poll interval doesn't change
    }
    else
    {
        mTimerStartTime = now;
        StartPollTimer(mTimerStartTime, mPollPeriod);
    }
}

void DataPollSender::StartPollTimer(TimeMilli aStartTime, uint32_t aPollPeriod)
{
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    if (ShouldUseDataPollOffload())
    {
        Error error = Get<Mac::Mac>().StartRadioAutoPoll(aStartTime, aPollPeriod);

        switch (error)
        {
        case kErrorNone:
            LogDebg("Started radio data poll");
            mIsRadioPollRunning = true;
            break;

        case kErrorInvalidState:
            LogWarn("Radio Data poll requested while MAC was not enabled!");
            StopPolling();
            break;

        default:
            LogWarn("Unexpected error %s requesting radio data poll", ErrorToString(error));
            // Use normal poll in case of unexpected error.
            mTimer.StartAt(aStartTime, aPollPeriod);
            break;
        }
    }
    else
#endif
    {
        mTimer.StartAt(aStartTime, aPollPeriod);
    }
}

void DataPollSender::StopPollTimer()
{
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    if (mIsRadioPollRunning)
    {
        mIsRadioPollRunning = false;
        Get<Mac::Mac>().StopRadioAutoPoll();
    }
    else
#endif
    {
        mTimer.Stop();
    }
}

bool DataPollSender::IsPollTimerRunning()
{
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    if (mIsRadioPollRunning)
    {
        return true;
    }
    else
#endif
    {
        return mTimer.IsRunning();
    }
}

bool DataPollSender::ShouldURestartTimer(uint32_t  oldPeriod)
{
#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
    bool bRestartTimer = false;

    // In case the poll schedule delay flag is set we can restart the timer as
    // start time was updated and we can properly calculate the when the next poll 
    // needs to happen.
    if (mDelayNextPollSchedule)
    {
        mDelayNextPollSchedule = false;
        mIsRadioPollRunning = false;
        bRestartTimer = true;
    }
    // Try to determine if we are switching from offloaded poll to the regular one or the 
    // other way around when the poll period is the same.
    else if ((oldPeriod != mPollPeriod) ||
             (IsPollNumberSet() && mIsRadioPollRunning) ||
             ((IsPollNumberSet() == false) && !mIsRadioPollRunning))
    {
        StopPollTimer();

        if (Get<Mac::Mac>().IsInRadioPollState())
        {
            // In this case we need to delay the scheduling of the next poll until the start time is 
            // updated with the correct valued based on the number of offloaded polls sent.
            mPollPeriod            = oldPeriod;
            mDelayNextPollSchedule = true;
            mIsRadioPollRunning    = true;
        }
        else
        {
            bRestartTimer = true;
        }
    }

    return bRestartTimer;
#else
    return (oldPeriod != mPollPeriod);
#endif
}

#if OPENTHREAD_CONFIG_MAC_DATA_POLL_OFFLOAD_ENABLE
bool DataPollSender::ShouldUseDataPollOffload()
{
    bool bRetValue = false;

    // 1. Radio supports MAC data poll offload
    // 2. Node is attached to parent, during attach phase there is no significant gain in using poll offload
    // 3. There is no fixed number of polls that need to be sent. This is checked by the IsPollNumberSet 
    // method. This method will need to be updated to reflect any new state that contains a fixed number of
    // polls.
    if (Get<Mac::SubMac>().IsRadioAutoPollSupported() && Get<Mle::Mle>().IsChild() && !IsPollNumberSet())
    {
#if OPENTHREAD_CONFIG_MULTI_RADIO
        // 4. When OPENTHREAD_CONFIG_MULTI_RADIO is enabled Parent is present on 15.4 link
        bRetValue = Mac::kRadioTypeIeee802154 == Get<RadioSelector>().SelectPollFrameRadio(GetParent());
#else
        bRetValue = true;
#endif
    }

    return bRetValue;
}

bool DataPollSender::IsPollNumberSet(void) const
{
    // This method should be updated if a new state is added to the data poll sender class that has a fixed number of
    // polls to be sent.
    return (mRemainingFastPolls != 0);
}
#endif

uint32_t DataPollSender::CalculatePollPeriod(void) const
{
    uint32_t period = GetDefaultPollPeriod();

    if (mAttachMode)
    {
        period = Min(period, kAttachDataPollPeriod);
    }

    if (mRetxMode)
    {
        period = Min(period, kRetxPollPeriod);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (Get<Mac::Mac>().GetCslPeriodInMsec() > 0)
        {
            period = Min(period, Get<Mac::Mac>().GetCslPeriodInMsec());
        }
#endif
    }

    if (mRemainingFastPolls != 0)
    {
        period = Min(period, kFastPollPeriod);
    }

    if (mExternalPollPeriod != 0)
    {
        period = Min(period, mExternalPollPeriod);
    }

    if (period == 0)
    {
        period = kMinPollPeriod;
    }
    return period;
}

uint32_t DataPollSender::GetDefaultPollPeriod(void) const
{
    uint32_t pollAhead = static_cast<uint32_t>(kRetxPollPeriod) * kMaxPollRetxAttempts;
    uint32_t period;

    period = Time::SecToMsec(Min(Get<Mle::Mle>().GetTimeout(), Time::MsecToSec(TimerMilli::kMaxDelay)));

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE && OPENTHREAD_CONFIG_MAC_CSL_AUTO_SYNC_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled())
    {
        period    = Min(period, Time::SecToMsec(Get<Mle::Mle>().GetCslTimeout()));
        pollAhead = static_cast<uint32_t>(kRetxPollPeriod);
    }
#endif

    if (period > pollAhead)
    {
        period -= pollAhead;
    }

    return period;
}

Mac::TxFrame *DataPollSender::PrepareDataRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame      *frame = nullptr;
    Mac::TxFrame::Info frameInfo;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    Mac::RadioType radio;

    SuccessOrExit(GetPollDestinationAddress(frameInfo.mAddrs.mDestination, radio));
    frame = &aTxFrames.GetTxFrame(radio);
#else
    SuccessOrExit(GetPollDestinationAddress(frameInfo.mAddrs.mDestination));
     frame = &aTxFrames.GetTxFrame();
#endif

    if (frameInfo.mAddrs.mDestination.IsExtended())
    {
        frameInfo.mAddrs.mSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }
    else
    {
        frameInfo.mAddrs.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());
    }

    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    frameInfo.mType          = Mac::Frame::kTypeMacCmd;
    frameInfo.mCommandId     = Mac::Frame::kMacCmdDataRequest;
    frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;

    Get<MeshForwarder>().PrepareMacHeaders(*frame, frameInfo, nullptr);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (frame->HasCslIe())
    {
        // Disable frame retransmission when the data poll has CSL IE included
        aTxFrames.SetMaxFrameRetries(0);
    }
#endif

exit:
    return frame;
}

} // namespace ot
