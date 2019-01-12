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
#include "common/random.hpp"

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD

namespace ot {
namespace Utils {

ChannelManager::ChannelManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSupportedChannelMask(0)
    , mFavoredChannelMask(0)
    , mActiveTimestamp(0)
    , mNotifierCallback(&ChannelManager::HandleStateChanged, this)
    , mDelay(kMinimumDelay)
    , mChannel(0)
    , mState(kStateIdle)
    , mTimer(aInstance, &ChannelManager::HandleTimer, this)
    , mAutoSelectInterval(kDefaultAutoSelectInterval)
    , mAutoSelectEnabled(false)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

void ChannelManager::RequestChannelChange(uint8_t aChannel)
{
    otLogInfoUtil("ChannelManager: Request to change to channel %d with delay %d sec", aChannel, mDelay);

    if (aChannel == GetInstance().Get<Mac::Mac>().GetPanChannel())
    {
        otLogInfoUtil("ChannelManager: Already operating on the requested channel %d", aChannel);
        ExitNow();
    }

    mState           = kStateChangeRequested;
    mChannel         = aChannel;
    mActiveTimestamp = 0;

    mTimer.Start(1 + Random::GetUint32InRange(0, kRequestStartJitterInterval));

    GetNotifier().Signal(OT_CHANGED_CHANNEL_MANAGER_NEW_CHANNEL);

exit:
    return;
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

    VerifyOrExit(mChannel != GetInstance().Get<Mac::Mac>().GetPanChannel());

    if (netif.GetPendingDataset().Get(dataset) == OT_ERROR_NONE)
    {
        if (dataset.mComponents.mIsPendingTimestampPresent)
        {
            pendingTimestamp = dataset.mPendingTimestamp;
        }

        // We check whether the Pending Dataset is changing the
        // channel to same one as the current request (i.e., channel
        // should match and delay should be less than the requested
        // delay).

        if (dataset.mComponents.mIsChannelPresent && (mChannel == dataset.mChannel) &&
            dataset.mComponents.mIsDelayPresent && (dataset.mDelay <= delayInMs) &&
            dataset.mComponents.mIsActiveTimestampPresent)
        {
            // We save the active timestamp to later check and ensure it
            // is ahead of current ActiveDataset timestamp.

            pendingActiveTimestamp = dataset.mActiveTimestamp;
        }
    }

    pendingTimestamp += 1 + Random::GetUint32InRange(0, kMaxTimestampIncrease);

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
            otLogInfoUtil("ChannelManager: Request to change to channel %d failed. Device is disabled", mChannel);

            mState = kStateIdle;
            StartAutoSelectTimer();
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
            otLogInfoUtil("ChannelManager: Pending Dataset is valid for change channel to %d", mChannel);
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
            otLogInfoUtil("ChannelManager: Canceling channel change to %d since current ActiveDataset is more recent",
                          mChannel);

            ExitNow();
        }
    }
    else
    {
        mActiveTimestamp = dataset.mActiveTimestamp + 1 + Random::GetUint32InRange(0, kMaxTimestampIncrease);
    }

    dataset.mActiveTimestamp                       = mActiveTimestamp;
    dataset.mComponents.mIsActiveTimestampPresent  = true;
    dataset.mChannel                               = mChannel;
    dataset.mComponents.mIsChannelPresent          = true;
    dataset.mPendingTimestamp                      = pendingTimestamp;
    dataset.mComponents.mIsPendingTimestampPresent = true;
    dataset.mDelay                                 = delayInMs;
    dataset.mComponents.mIsDelayPresent            = true;

    error = netif.GetPendingDataset().SendSetRequest(dataset, NULL, 0);

    if (error == OT_ERROR_NONE)
    {
        otLogInfoUtil("ChannelManager: Sent PendingDatasetSet to change channel to %d", mChannel);

        mState = kStateSentMgmtPendingDataset;
        mTimer.Start(delayInMs + kChangeCheckWaitInterval);
    }
    else
    {
        otLogInfoUtil("ChannelManager: %s error in dataset update (channel change %d), retry in %d sec",
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
        otLogInfoUtil("ChannelManager: Auto-triggered channel select");
        IgnoreReturnValue(RequestChannelSelect(false));
        StartAutoSelectTimer();
        break;

    case kStateSentMgmtPendingDataset:
        otLogInfoUtil("ChannelManager: Timed out waiting for change to %d, trying again.", mChannel);
        mState = kStateChangeRequested;

        // fall through

    case kStateChangeRequested:
        PreparePendingDataset();
        break;
    }
}

void ChannelManager::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<ChannelManager>().HandleStateChanged(aFlags);
}

void ChannelManager::HandleStateChanged(otChangedFlags aFlags)
{
    VerifyOrExit((aFlags & OT_CHANGED_THREAD_CHANNEL) != 0);
    VerifyOrExit(mChannel == GetInstance().Get<Mac::Mac>().GetPanChannel());

    mState = kStateIdle;
    StartAutoSelectTimer();

    otLogInfoUtil("ChannelManager: Channel successfully changed to %d", mChannel);

exit:
    return;
}

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR

/**
 * This function randomly chooses a channel from a given channel mask.
 *
 * @param[in]  aMask  A channel mask.
 *
 * @returns A randomly chosen channel from the given mask, or `ChannelMask::kChannelIteratorFirst` if the mask is empty.
 *
 */
static uint8_t ChooseRandomChannel(const Mac::ChannelMask &aMask)
{
    uint8_t channel     = Mac::ChannelMask::kChannelIteratorFirst;
    uint8_t numChannels = 0;
    uint8_t randomIndex;

    VerifyOrExit(!aMask.IsEmpty());

    while (aMask.GetNextChannel(channel) == OT_ERROR_NONE)
    {
        numChannels++;
    }

    randomIndex = Random::GetUint8InRange(0, numChannels);

    channel = Mac::ChannelMask::kChannelIteratorFirst;
    SuccessOrExit(aMask.GetNextChannel(channel));

    while (randomIndex-- != 0)
    {
        SuccessOrExit(aMask.GetNextChannel(channel));
    }

exit:
    return channel;
}

otError ChannelManager::FindBetterChannel(uint8_t &aNewChannel, uint16_t &aOccupancy)
{
    otError          error   = OT_ERROR_NONE;
    ChannelMonitor & monitor = GetInstance().GetChannelMonitor();
    Mac::ChannelMask favoredAndSupported;
    Mac::ChannelMask favoredBest;
    Mac::ChannelMask supportedBest;
    uint16_t         favoredOccupancy;
    uint16_t         supportedOccupancy;

    if (monitor.GetSampleCount() <= kMinChannelMonitorSampleCount)
    {
        otLogInfoUtil("ChannelManager: Too few samples (%d <= %d) to select channel", monitor.GetSampleCount(),
                      kMinChannelMonitorSampleCount);
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

    favoredAndSupported = mFavoredChannelMask;
    favoredAndSupported.Intersect(mSupportedChannelMask);

    favoredBest   = monitor.FindBestChannels(favoredAndSupported, favoredOccupancy);
    supportedBest = monitor.FindBestChannels(mSupportedChannelMask, supportedOccupancy);

    otLogInfoUtil("ChannelManager: Best favored %s, occupancy 0x%04x", favoredBest.ToString().AsCString(),
                  favoredOccupancy);
    otLogInfoUtil("ChannelManager: Best overall %s, occupancy 0x%04x", supportedBest.ToString().AsCString(),
                  supportedOccupancy);

    // Prefer favored channels unless there is no favored channel,
    // or the occupancy rate of the best favored channel is worse
    // than the best overall by at least `kThresholdToSkipFavored`.

    if (favoredBest.IsEmpty() || ((favoredOccupancy >= kThresholdToSkipFavored) &&
                                  (supportedOccupancy < favoredOccupancy - kThresholdToSkipFavored)))
    {
        if (!favoredBest.IsEmpty())
        {
            otLogInfoUtil("ChannelManager: Preferring an unfavored channel due to high occupancy rate diff");
        }

        favoredBest      = supportedBest;
        favoredOccupancy = supportedOccupancy;
    }

    VerifyOrExit(!favoredBest.IsEmpty(), error = OT_ERROR_NOT_FOUND);

    aNewChannel = ChooseRandomChannel(favoredBest);
    aOccupancy  = favoredOccupancy;

exit:
    return error;
}

bool ChannelManager::ShouldAttemptChannelChange(void)
{
    uint16_t ccaFailureRate = GetInstance().Get<Mac::Mac>().GetCcaFailureRate();
    bool     shouldAttempt  = (ccaFailureRate >= kCcaFailureRateThreshold);

    otLogInfoUtil("ChannelManager: CCA-err-rate: 0x%04x %s 0x%04x, selecting channel: %s", ccaFailureRate,
                  shouldAttempt ? ">=" : "<", kCcaFailureRateThreshold, shouldAttempt ? "yes" : "no");

    return shouldAttempt;
}

otError ChannelManager::RequestChannelSelect(bool aSkipQualityCheck)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  curChannel, newChannel;
    uint16_t curOccupancy, newOccupancy;

    otLogInfoUtil("ChannelManager: Request to select channel (skip quality check: %s)",
                  aSkipQualityCheck ? "yes" : "no");

    VerifyOrExit(GetInstance().Get<Mle::Mle>().GetRole() != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(aSkipQualityCheck || ShouldAttemptChannelChange());

    SuccessOrExit(error = FindBetterChannel(newChannel, newOccupancy));

    curChannel   = GetInstance().Get<Mac::Mac>().GetPanChannel();
    curOccupancy = GetInstance().GetChannelMonitor().GetChannelOccupancy(curChannel);

    if (newChannel == curChannel)
    {
        otLogInfoUtil("ChannelManager: Already on best possible channel %d", curChannel);
        ExitNow();
    }

    otLogInfoUtil("ChannelManager: Cur channel %d, occupancy 0x%04x - Best channel %d, occupancy 0x%04x", curChannel,
                  curOccupancy, newChannel, newOccupancy);

    // Switch only if new channel's occupancy rate is better than current
    // channel's occupancy rate by threshold `kThresholdToChangeChannel`.

    if ((newOccupancy >= curOccupancy) ||
        (static_cast<uint16_t>(curOccupancy - newOccupancy) < kThresholdToChangeChannel))
    {
        otLogInfoUtil("ChannelManager: Occupancy rate diff too small to change channel");
        ExitNow();
    }

    RequestChannelChange(newChannel);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoUtil("ChannelManager: Request to select better channel failed, error: %s",
                      otThreadErrorToString(error));
    }

    return error;
}

#else // OPENTHREAD_ENABLE_CHANNEL_MONITOR

otError ChannelManager::RequestChannelSelect(bool)
{
    otLogInfoUtil("ChannelManager: ChannelMonitor feature is disabled - cannot select channel");
    return OT_ERROR_DISABLED_FEATURE;
}

#endif // OPENTHREAD_ENABLE_CHANNEL_MONITOR

void ChannelManager::StartAutoSelectTimer(void)
{
    VerifyOrExit(mState == kStateIdle);

    if (mAutoSelectEnabled)
    {
        mTimer.Start(TimerMilli::SecToMsec(mAutoSelectInterval));
    }
    else
    {
        mTimer.Stop();
    }

exit:
    return;
}

void ChannelManager::SetAutoChannelSelectionEnabled(bool aEnabled)
{
    if (aEnabled != mAutoSelectEnabled)
    {
        mAutoSelectEnabled = aEnabled;
        IgnoreReturnValue(RequestChannelSelect(false));
        StartAutoSelectTimer();
    }
}

otError ChannelManager::SetAutoChannelSelectionInterval(uint32_t aInterval)
{
    otError  error        = OT_ERROR_NONE;
    uint32_t prevInterval = mAutoSelectInterval;

    VerifyOrExit((aInterval != 0) && (aInterval < TimerMilli::MsecToSec(Timer::kMaxDt)), error = OT_ERROR_INVALID_ARGS);

    mAutoSelectInterval = aInterval;

    if (mAutoSelectEnabled && (mState == kStateIdle) && mTimer.IsRunning() && (prevInterval != aInterval))
    {
        mTimer.StartAt(mTimer.GetFireTime() - prevInterval, aInterval);
    }

exit:
    return error;
}

void ChannelManager::SetSupportedChannels(uint32_t aChannelMask)
{
    mSupportedChannelMask.SetMask(aChannelMask & OT_RADIO_SUPPORTED_CHANNELS);

    otLogInfoUtil("ChannelManager: Supported channels: %s", mSupportedChannelMask.ToString().AsCString());
}

void ChannelManager::SetFavoredChannels(uint32_t aChannelMask)
{
    mFavoredChannelMask.SetMask(aChannelMask & OT_RADIO_SUPPORTED_CHANNELS);

    otLogInfoUtil("ChannelManager: Favored channels: %s", mFavoredChannelMask.ToString().AsCString());
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_CHANNEL_MANAGER
