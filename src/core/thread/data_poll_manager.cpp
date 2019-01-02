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
 *   This file implements data poll (mac data request command) manager class.
 */

#define WPP_NAME "data_poll_manager.tmh"

#include "data_poll_manager.hpp"

#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/owner-locator.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

DataPollManager::DataPollManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimerStartTime(0)
    , mExternalPollPeriod(0)
    , mPollPeriod(0)
    , mTimer(aInstance, &DataPollManager::HandlePollTimer, this)
    , mEnabled(false)
    , mAttachMode(false)
    , mRetxMode(false)
    , mNoBufferRetxMode(false)
    , mPollTimeoutCounter(0)
    , mPollTxFailureCounter(0)
    , mRemainingFastPolls(0)
{
}

otError DataPollManager::StartPolling(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mEnabled, error = OT_ERROR_ALREADY);
    VerifyOrExit(!GetNetif().GetMle().IsRxOnWhenIdle(), error = OT_ERROR_INVALID_STATE);

    mEnabled = true;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return error;
}

void DataPollManager::StopPolling(void)
{
    mTimer.Stop();
    mAttachMode           = false;
    mRetxMode             = false;
    mNoBufferRetxMode     = false;
    mPollTimeoutCounter   = 0;
    mPollTxFailureCounter = 0;
    mRemainingFastPolls   = 0;
    mEnabled              = false;
}

otError DataPollManager::SendDataPoll(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error;
    Message *    message;
    Neighbor *   parent;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!netif.GetMac().GetRxOnWhenIdle(), error = OT_ERROR_INVALID_STATE);

    parent = netif.GetMle().GetParentCandidate();
    VerifyOrExit((parent != NULL) && parent->IsStateValidOrRestoring(), error = OT_ERROR_INVALID_STATE);

    mTimer.Stop();

    for (message = netif.GetMeshForwarder().GetSendQueue().GetHead(); message; message = message->GetNext())
    {
        VerifyOrExit(message->GetType() != Message::kTypeMacDataPoll, error = OT_ERROR_ALREADY);
    }

    message = GetInstance().GetMessagePool().New(Message::kTypeMacDataPoll, 0);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    error = netif.GetMeshForwarder().SendMessage(*message);

    if (error != OT_ERROR_NONE)
    {
        message->Free();
    }

exit:

    switch (error)
    {
    case OT_ERROR_NONE:
        otLogDebgMac("Sending data poll");

        if (mNoBufferRetxMode == true)
        {
            mNoBufferRetxMode = false;
            ScheduleNextPoll(kRecalculatePollPeriod);
        }
        else
        {
            ScheduleNextPoll(kUsePreviousPollPeriod);
        }

        break;

    case OT_ERROR_INVALID_STATE:
        otLogWarnMac("Data poll tx requested while data polling was not enabled!");
        StopPolling();
        break;

    case OT_ERROR_ALREADY:
        otLogDebgMac("Data poll tx requested when a previous data request still in send queue.");
        ScheduleNextPoll(kUsePreviousPollPeriod);
        break;

    case OT_ERROR_NO_BUFS:
    default:
        mNoBufferRetxMode = true;
        ScheduleNextPoll(kRecalculatePollPeriod);
        break;
    }

    return error;
}

otError DataPollManager::SetExternalPollPeriod(uint32_t aPeriod)
{
    otError error = OT_ERROR_NONE;

    if (aPeriod != 0 && aPeriod < OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD)
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
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

uint32_t DataPollManager::GetKeepAlivePollPeriod(void) const
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

void DataPollManager::HandlePollSent(otError aError)
{
    bool shouldRecalculatePollPeriod = false;

    VerifyOrExit(mEnabled);

    switch (aError)
    {
    case OT_ERROR_NONE:

        if (mRemainingFastPolls != 0)
        {
            mRemainingFastPolls--;
            shouldRecalculatePollPeriod = (mRemainingFastPolls == 0);
        }

        if (mRetxMode == true)
        {
            mRetxMode                   = false;
            mPollTxFailureCounter       = 0;
            shouldRecalculatePollPeriod = true;
        }

        otLogInfoMac("Sent data poll");

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

void DataPollManager::HandlePollTimeout(void)
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

void DataPollManager::CheckFramePending(Mac::Frame &aFrame)
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

void DataPollManager::RecalculatePollPeriod(void)
{
    if (mEnabled)
    {
        ScheduleNextPoll(kRecalculatePollPeriod);
    }
}

void DataPollManager::SetAttachMode(bool aMode)
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

void DataPollManager::SendFastPolls(uint8_t aNumFastPolls)
{
    bool shouldRecalculatePollPeriod = (mRemainingFastPolls == 0);

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

void DataPollManager::ScheduleNextPoll(PollPeriodSelector aPollPeriodSelector)
{
    if (aPollPeriodSelector == kRecalculatePollPeriod)
    {
        mPollPeriod = CalculatePollPeriod();
    }

    if (mTimer.IsRunning())
    {
        mTimer.StartAt(mTimerStartTime, mPollPeriod);
    }
    else
    {
        mTimerStartTime = TimerMilli::GetNow();
        mTimer.StartAt(mTimerStartTime, mPollPeriod);
    }
}

uint32_t DataPollManager::CalculatePollPeriod(void) const
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

    if (mNoBufferRetxMode == true)
    {
        if ((period == 0) || (period > kNoBufferRetxPollPeriod))
        {
            period = kNoBufferRetxPollPeriod;
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

void DataPollManager::HandlePollTimer(Timer &aTimer)
{
    aTimer.GetOwner<DataPollManager>().SendDataPoll();
}

uint32_t DataPollManager::GetDefaultPollPeriod(void) const
{
    return TimerMilli::SecToMsec(GetNetif().GetMle().GetTimeout()) -
           static_cast<uint32_t>(kRetxPollPeriod) * kMaxPollRetxAttempts;
}

} // namespace ot
