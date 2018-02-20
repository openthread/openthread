/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements Channel Manager.
 *
 */

#include "channel_manager.hpp"

#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD

namespace ot {
namespace Utils {

ChannelManager::ChannelManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSupportedChannels(kDefaultSupprotedChannelMask)
    , mActiveTimestamp(0)
    , mNotifierCallback(&ChannelManager::HandleStateChanged, this)
    , mDelay(kMinimumDelay)
    , mChannel(0)
    , mState(kStateIdle)
    , mTimer(aInstance, &ChannelManager::HandleTimer, this)
{
}

otError ChannelManager::RequestChannelChange(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    otLogInfoUtil(GetInstance(), "ChannelManager: Request to change to channel %d with delay %d sec", aChannel, mDelay);

    VerifyOrExit(aChannel != GetInstance().Get<Mac::Mac>().GetChannel());

    if ((mSupportedChannels & (1U << aChannel)) == 0)
    {
        otLogInfoUtil(GetInstance(), "ChannelManager: Request rejected! Channel %d not in supported mask 0x%x",
                      aChannel, mSupportedChannels);

        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    mState           = kStateChangeRequested;
    mChannel         = aChannel;
    mActiveTimestamp = 0;

    mTimer.Start((otPlatRandomGet() % kRequestStartJitterInterval) + 1);

exit:
    return error;
}

otError ChannelManager::SetDelay(uint16_t aDelay)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aDelay >= kMinimumDelay, error = OT_ERROR_INVALID_ARGS);
    mDelay = aDelay;

exit:
    return error;
}

void ChannelManager::PreparePendingDataset(void)
{
    ThreadNetif &        netif                  = GetInstance().GetThreadNetif();
    uint64_t             pendingTimestamp       = 0;
    uint64_t             pendingActiveTimestamp = 0;
    uint32_t             delayInMs              = TimerMilli::SecToMsec(static_cast<uint32_t>(mDelay));
    otOperationalDataset dataset;
    otError              error;

    VerifyOrExit(mState == kStateChangeRequested);

    VerifyOrExit(mChannel != GetInstance().Get<Mac::Mac>().GetChannel());

    if ((mSupportedChannels & (1U << mChannel)) == 0)
    {
        otLogInfoUtil(GetInstance(), "ChannelManager: Request rejected! Channel %d not in supported mask 0x%x",
                      mChannel, mSupportedChannels);
        mState = kStateIdle;
        ExitNow();
    }

    if (netif.GetPendingDataset().Get(dataset) == OT_ERROR_NONE)
    {
        if (dataset.mIsPendingTimestampSet)
        {
            pendingTimestamp = dataset.mPendingTimestamp;
        }

        // We check whether the Pending Dataset is changing the
        // channel to same one as the current request (i.e., channel
        // should match and delay should be less than the requested
        // delay).

        if (dataset.mIsChannelSet && (mChannel == dataset.mChannel) && dataset.mIsDelaySet &&
            (dataset.mDelay <= delayInMs) && dataset.mIsActiveTimestampSet)
        {
            // We save the active timestamp to later check and ensure it
            // is ahead of current ActiveDataset timestamp.

            pendingActiveTimestamp = dataset.mActiveTimestamp;
        }
    }

    pendingTimestamp += (otPlatRandomGet() % kMaxTimestampIncrease) + 1;

    error = netif.GetActiveDataset().Get(dataset);

    if (error != OT_ERROR_NONE)
    {
        // If there is no valid Active Dataset but we are not disabled, set
        // the timer to try again after the retry interval. This handles the
        // situation where a channel change request comes right after the
        // network is formed but before the active dataset is created.

        if (GetInstance().Get<Mle::Mle>().GetRole() != OT_DEVICE_ROLE_DISABLED)
        {
            mTimer.Start(kPendingDatasetTxRetryInterval);
        }
        else
        {
            mState = kStateIdle;
            otLogInfoUtil(GetInstance(), "ChannelManager: Request to change to channel %d failed. Device is disabled",
                          mChannel);
        }

        ExitNow();
    }

    // `pendingActiveTimestamp` will be non-zero if the Pending
    // Dataset is valid and is performing the same channel change.
    // We check to ensure its timestamp is indeed ahead of current
    // Active Dataset's timestamp, and if so, we skip updating
    // the Pending Dataset.

    if (pendingActiveTimestamp != 0)
    {
        if (dataset.mActiveTimestamp < pendingActiveTimestamp)
        {
            otLogInfoUtil(GetInstance(), "ChannelManager: Pending Dataset is valid for change channel to %d", mChannel);
            mState = kStateSentMgmtPendingDataset;
            mTimer.Start(delayInMs + kChangeCheckWaitInterval);
            ExitNow();
        }
    }

    // A non-zero `mActiveTimestamp` indicates that this is not the first
    // attempt to update the Dataset for the ongoing requested channel
    // change. In that case, if the Timestamp in current Active Dataset is
    // more recent compared to `mActiveTimestamp`, the channel change
    // process is canceled. This helps address situations where two
    // different devices may be performing channel change around the same
    // time.

    if (mActiveTimestamp != 0)
    {
        if (dataset.mActiveTimestamp >= mActiveTimestamp)
        {
            otLogInfoUtil(GetInstance(),
                          "ChannelManager: Canceling channel change to %d since current ActiveDataset is more recent",
                          mChannel);

            ExitNow();
        }
    }
    else
    {
        mActiveTimestamp = dataset.mActiveTimestamp + 1 + (otPlatRandomGet() % kMaxTimestampIncrease);
    }

    dataset.mActiveTimestamp      = mActiveTimestamp;
    dataset.mIsActiveTimestampSet = true;

    dataset.mChannel      = mChannel;
    dataset.mIsChannelSet = true;

    dataset.mPendingTimestamp      = pendingTimestamp;
    dataset.mIsPendingTimestampSet = true;

    dataset.mDelay      = delayInMs;
    dataset.mIsDelaySet = true;

    error = netif.GetPendingDataset().SendSetRequest(dataset, NULL, 0);

    if (error == OT_ERROR_NONE)
    {
        otLogInfoUtil(GetInstance(), "ChannelManager: Sent PendingDatasetSet to change channel to %d", mChannel);

        mState = kStateSentMgmtPendingDataset;
        mTimer.Start(delayInMs + kChangeCheckWaitInterval);
    }
    else
    {
        otLogInfoUtil(GetInstance(), "ChannelManager: %s error in dataset update (channel change %d), retry in %d sec",
                      otThreadErrorToString(error), mChannel, TimerMilli::MsecToSec(kPendingDatasetTxRetryInterval));

        mTimer.Start(kPendingDatasetTxRetryInterval);
    }

exit:
    return;
}

void ChannelManager::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ChannelManager>().HandleTimer();
}

void ChannelManager::HandleTimer(void)
{
    switch (mState)
    {
    case kStateIdle:
        break;

    case kStateSentMgmtPendingDataset:
        otLogInfoUtil(GetInstance(), "ChannelManager: Timed out waiting for change to %d, trying again.", mChannel);
        mState = kStateChangeRequested;

        // fall through

    case kStateChangeRequested:
        PreparePendingDataset();
        break;
    }
}

void ChannelManager::HandleStateChanged(Notifier::Callback &aCallback, uint32_t aFlags)
{
    aCallback.GetOwner<ChannelManager>().HandleStateChanged(aFlags);
}

void ChannelManager::HandleStateChanged(uint32_t aFlags)
{
    VerifyOrExit((aFlags & OT_CHANGED_THREAD_CHANNEL) != 0);
    VerifyOrExit(mChannel == GetInstance().Get<Mac::Mac>().GetChannel());

    mState = kStateIdle;
    mTimer.Stop();

    otLogInfoUtil(GetInstance(), "ChannelManager: Channel successfully changed to %d", mChannel);

exit:
    return;
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_CHANNEL_MANAGER
