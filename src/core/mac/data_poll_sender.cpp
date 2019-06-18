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

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

DataPollSender::DataPollSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimerStartTime(0)
    , mPollPeriod(0)
    , mExternalPollPeriod(0)
    , mFastPollsUsers(0)
    , mTimer(aInstance, &DataPollSender::HandlePollTimer, this)
    , mEnabled(false)
    , mAttachMode(false)
    , mRetxMode(false)
    , mPollTimeoutCounter(0)
    , mPollTxFailureCounter(0)
    , mRemainingFastPolls(0)
{
}

otError DataPollSender::StartPolling(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mEnabled, error = OT_ERROR_ALREADY);
    VerifyOrExit(!Get<Mle::MleRouter>().IsRxOnWhenIdle(), error = OT_ERROR_INVALID_STATE);

    mEnabled = true;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return error;
}

void DataPollSender::StopPolling(void)
{
    mTimer.Stop();
    mAttachMode           = false;
    mRetxMode             = false;
    mPollTimeoutCounter   = 0;
    mPollTxFailureCounter = 0;
    mRemainingFastPolls   = 0;
    mFastPollsUsers       = 0;
    mEnabled              = false;
}

otError DataPollSender::SendDataPoll(void)
{
    otError   error;
    Neighbor *parent;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!Get<Mac::Mac>().GetRxOnWhenIdle(), error = OT_ERROR_INVALID_STATE);

    parent = Get<Mle::MleRouter>().GetParentCandidate();
    VerifyOrExit((parent != NULL) && parent->IsStateValidOrRestoring(), error = OT_ERROR_INVALID_STATE);

    mTimer.Stop();

    SuccessOrExit(error = Get<Mac::Mac>().RequestDataPollTransmission());

exit:

    switch (error)
    {
    case OT_ERROR_NONE:
        otLogDebgMac("Sending data poll");
        ScheduleNextPoll(kUsePreviousPollPeriod);
        break;

    case OT_ERROR_INVALID_STATE:
        otLogWarnMac("Data poll tx requested while data polling was not enabled!");
        StopPolling();
        break;

    case OT_ERROR_ALREADY:
        otLogDebgMac("Data poll tx requested when a previous data request still in send queue.");
        ScheduleNextPoll(kUsePreviousPollPeriod);
        break;

    default:
        otLogWarnMac("Unexpected error %s requesting data poll", otThreadErrorToString(error));
        ScheduleNextPoll(kRecalculatePollPeriod);
        break;
    }

    return error;
}

otError DataPollSender::SetExternalPollPeriod(uint32_t aPeriod)
{
    otError error = OT_ERROR_NONE;

    if (aPeriod != 0)
    {
        VerifyOrExit(aPeriod >= OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD, error = OT_ERROR_INVALID_ARGS);

        // Clipped by the maximal value.
        if (aPeriod > kMaxExternalPeriod)
        {
            aPeriod = kMaxExternalPeriod;
        }
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
    uint32_t period = 0;

    if (mExternalPollPeriod != 0)
    {
        period = mExternalPollPeriod;
    }
    else
    {
        period = GetDefaultPollPeriod();
    }

    return period;
}

void DataPollSender::HandlePollSent(Mac::Frame &aFrame, otError aError)
{
    Mac::Address macDest;
    bool         shouldRecalculatePollPeriod = false;

    VerifyOrExit(mEnabled);

    aFrame.GetDstAddr(macDest);
    Get<MeshForwarder>().UpdateNeighborOnSentFrame(aFrame, aError, macDest);

    if (Get<Mle::MleRouter>().GetParentCandidate()->GetState() == Neighbor::kStateInvalid)
    {
        StopPolling();
        Get<Mle::MleRouter>().BecomeDetached();
        ExitNow();
    }

    switch (aError)
    {
    case OT_ERROR_NONE:

        if (mRemainingFastPolls != 0)
        {
            mRemainingFastPolls--;

            if (mRemainingFastPolls == 0)
            {
                shouldRecalculatePollPeriod = true;
                mFastPollsUsers             = 0;
            }
        }

        if (mRetxMode == true)
        {
            mRetxMode                   = false;
            mPollTxFailureCounter       = 0;
            shouldRecalculatePollPeriod = true;
        }

        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:
        mRetxMode                   = true;
        shouldRecalculatePollPeriod = true;
        break;

    default:
        mPollTxFailureCounter++;

        otLogInfoMac("Failed to send data poll, error:%s, retx:%d/%d", otThreadErrorToString(aError),
                     mPollTxFailureCounter, kMaxPollRetxAttempts);

        if (mPollTxFailureCounter < kMaxPollRetxAttempts)
        {
            if (mRetxMode == false)
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

    if (shouldRecalculatePollPeriod)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }

exit:
    return;
}

void DataPollSender::HandlePollTimeout(void)
{
    // A data poll timeout happened, i.e., the ack in response to
    // a data poll indicated that a frame was pending, but no frame
    // was received after timeout interval.

    VerifyOrExit(mEnabled);

    mPollTimeoutCounter++;

    otLogInfoMac("Data poll timeout, retry:%d/%d", mPollTimeoutCounter, kQuickPollsAfterTimeout);

    if (mPollTimeoutCounter < kQuickPollsAfterTimeout)
    {
        SendDataPoll();
    }
    else
    {
        mPollTimeoutCounter = 0;
    }

exit:
    return;
}

void DataPollSender::CheckFramePending(Mac::Frame &aFrame)
{
    VerifyOrExit(mEnabled);

    mPollTimeoutCounter = 0;

    if (aFrame.GetFramePending() == true)
    {
        SendDataPoll();
    }

exit:
    return;
}

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

    if (aNumFastPolls > kMaxFastPolls)
    {
        aNumFastPolls = kMaxFastPolls;
    }

    if (mRemainingFastPolls < aNumFastPolls)
    {
        mRemainingFastPolls = aNumFastPolls;
    }

    if (mEnabled && shouldRecalculatePollPeriod)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }
}

otError DataPollSender::StopFastPolls(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mFastPollsUsers != 0);

    // If `mFastPollsUsers` hits the max, let it be cleared
    // from `HandlePollSent()` (after all fast polls are sent).
    VerifyOrExit(mFastPollsUsers < kMaxFastPollsUsers);

    mFastPollsUsers--;

    VerifyOrExit(mFastPollsUsers == 0, error = OT_ERROR_BUSY);

    mRemainingFastPolls = 0;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return error;
}

void DataPollSender::ScheduleNextPoll(PollPeriodSelector aPollPeriodSelector)
{
    uint32_t now;
    uint32_t oldPeriod = mPollPeriod;

    if (aPollPeriodSelector == kRecalculatePollPeriod)
    {
        mPollPeriod = CalculatePollPeriod();
    }

    now = TimerMilli::GetNow();

    if (mTimer.IsRunning())
    {
        if (oldPeriod != mPollPeriod)
        {
            // If poll interval did change and re-starting the timer from
            // last start time with new poll interval would fire quickly
            // (i.e., fires within window `[now, now + kMinPollPeriod]`)
            // add an extra minimum delay of `kMinPollPeriod`. This
            // ensures that when an internal or external request triggers
            // a switch to a shorter poll interval, the first data poll
            // will not be sent too quickly (and possibly before the
            // response is available/prepared on the parent node).
            if (TimerScheduler::IsStrictlyBefore(mTimerStartTime + mPollPeriod, now + kMinPollPeriod))
            {
                mTimer.StartAt(now, kMinPollPeriod);
            }
            else
            {
                mTimer.StartAt(mTimerStartTime, mPollPeriod);
            }
        }
        // Do nothing on the running poll timer if the poll interval doesn't change
    }
    else
    {
        mTimerStartTime = now;
        mTimer.StartAt(mTimerStartTime, mPollPeriod);
    }
}

uint32_t DataPollSender::CalculatePollPeriod(void) const
{
    uint32_t period = 0;

    if (mAttachMode == true)
    {
        period = kAttachDataPollPeriod;
    }

    if (mRetxMode == true)
    {
        if ((period == 0) || (period > kRetxPollPeriod))
        {
            period = kRetxPollPeriod;
        }
    }

    if (mRemainingFastPolls != 0)
    {
        if ((period == 0) || (period > kFastPollPeriod))
        {
            period = kFastPollPeriod;
        }
    }

    if (mExternalPollPeriod != 0)
    {
        if ((period == 0) || (period > mExternalPollPeriod))
        {
            period = mExternalPollPeriod;
        }
    }

    if (period == 0)
    {
        period = GetDefaultPollPeriod();

        if (period == 0)
        {
            period = kMinPollPeriod;
        }
    }

    return period;
}

void DataPollSender::HandlePollTimer(Timer &aTimer)
{
    aTimer.GetOwner<DataPollSender>().SendDataPoll();
}

uint32_t DataPollSender::GetDefaultPollPeriod(void) const
{
    return TimerMilli::SecToMsec(Get<Mle::MleRouter>().GetTimeout()) -
           static_cast<uint32_t>(kRetxPollPeriod) * kMaxPollRetxAttempts;
}

} // namespace ot
