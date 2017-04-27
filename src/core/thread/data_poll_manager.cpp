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

#include "openthread/platform/random.h"

#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/netif.hpp>
#include <thread/data_poll_manager.hpp>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>

namespace Thread {

DataPollManager::DataPollManager(MeshForwarder &aMeshForwarder):
    mMeshForwarder(aMeshForwarder),
    mTimer(aMeshForwarder.GetNetif().GetIp6().mTimerScheduler, &DataPollManager::HandlePollTimer, this),
    mExternalPollPeriod(0),
    mPollPeriod(0),
    mEnabled(false),
    mAttachMode(false),
    mRetxMode(false),
    mNoBufferRetxMode(false),
    mPollTimeoutCounter(0),
    mPollTxFailureCounter(0),
    mRemainingFastPolls(0)
{
}

otInstance *DataPollManager::GetInstance(void)
{
    return mMeshForwarder.GetInstance();
}

ThreadError DataPollManager::StartPolling(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!mEnabled, error = kThreadError_Already);
    VerifyOrExit((mMeshForwarder.GetNetif().GetMle().GetDeviceMode() & Mle::ModeTlv::kModeFFD) == 0,
                 error = kThreadError_InvalidState);

    mEnabled = true;
    ScheduleNextPoll(kRecalculatePollPeriod);

exit:
    return error;
}

void DataPollManager::StopPolling(void)
{
    mTimer.Stop();
    mAttachMode = false;
    mRetxMode = false;
    mNoBufferRetxMode = false;
    mPollTimeoutCounter = 0;
    mPollTxFailureCounter = 0;
    mRemainingFastPolls = 0;
    mEnabled = false;
}

ThreadError DataPollManager::SendDataPoll(void)
{
    ThreadError error;
    Message *message;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);
    VerifyOrExit(!mMeshForwarder.GetNetif().GetMac().GetRxOnWhenIdle(), error = kThreadError_InvalidState);

    mTimer.Stop();

    for (message = mMeshForwarder.GetSendQueue().GetHead(); message; message = message->GetNext())
    {
        VerifyOrExit(message->GetType() != Message::kTypeMacDataPoll, error = kThreadError_Already);
    }

    message = mMeshForwarder.GetNetif().GetIp6().mMessagePool.New(Message::kTypeMacDataPoll, 0);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    error = mMeshForwarder.SendMessage(*message);

    if (error != kThreadError_None)
    {
        message->Free();
    }

exit:

    switch (error)
    {
    case kThreadError_None:
        otLogDebgMac(GetInstance(), "Sent poll");

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

    case kThreadError_InvalidState:
        otLogWarnMac(GetInstance(), "Data poll tx requested while data polling was not enabled!");
        StopPolling();
        break;

    case kThreadError_Already:
        otLogDebgMac(GetInstance(), "Data poll tx requested when a previous data request still in send queue.");
        ScheduleNextPoll(kUsePreviousPollPeriod);
        break;

    case kThreadError_NoBufs:
    default:
        mNoBufferRetxMode = true;
        ScheduleNextPoll(kRecalculatePollPeriod);
        break;
    }

    return error;
}

void DataPollManager::SetExternalPollPeriod(uint32_t aPeriod)
{
    if (mExternalPollPeriod != aPeriod)
    {
        mExternalPollPeriod = aPeriod;

        if (mEnabled)
        {
            ScheduleNextPoll(kRecalculatePollPeriod);
        }
    }
}

void DataPollManager::HandlePollSent(ThreadError aError)
{
    bool shouldRecalculatePollPeriod = false;

    VerifyOrExit(mEnabled);

    switch (aError)
    {
    case kThreadError_None:

        if (mRemainingFastPolls != 0)
        {
            mRemainingFastPolls--;
            shouldRecalculatePollPeriod = (mRemainingFastPolls == 0);
        }

        if (mRetxMode == true)
        {
            mRetxMode = false;
            mPollTxFailureCounter = 0;
            shouldRecalculatePollPeriod = true;
        }

        break;

    default:
        mPollTxFailureCounter++;

        if (mPollTxFailureCounter < kMaxPollRetxAttempts)
        {
            if (mRetxMode == false)
            {
                mRetxMode = true;
                shouldRecalculatePollPeriod = true;
            }
        }
        else
        {
            otLogWarnMac(GetInstance(), "Data poll tx failed in %d back-to-back attempts.", mPollTxFailureCounter);

            mRetxMode = false;
            mPollTxFailureCounter = 0;
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

    if (mPollTimeoutCounter <= kQuickPollsAfterTimeout)
    {
        SendDataPoll();
    }
    else
    {
        otLogInfoMac(GetInstance(), "Data poll timeout happened %d times back-to-back.", mPollTimeoutCounter);
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
        uint32_t elapsedTime = Timer::GetNow() - mTimer.Gett0();

        if (elapsedTime >= mPollPeriod)
        {
            SendDataPoll();
        }
        else
        {
            mTimer.Start(mPollPeriod - elapsedTime);
        }
    }
    else
    {
        mTimer.Start(mPollPeriod);
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
        period = Timer::SecToMsec(mMeshForwarder.GetNetif().GetMle().GetTimeout()) -  kRetxPollPeriod * kMaxPollRetxAttempts;

        if (period == 0)
        {
            period = kMinPollPeriod;
        }
    }

    return period;
}

void DataPollManager::HandlePollTimer(void *aContext)
{
    static_cast<DataPollManager *>(aContext)->SendDataPoll();
}

}  // namespace Thread
