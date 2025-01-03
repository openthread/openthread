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
 */

#include "channel_manager.hpp"

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE
#if (OPENTHREAD_FTD || OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)

#include "instance/instance.hpp"

namespace ot {
namespace Utils {

RegisterLogModule("ChannelManager");

ChannelManager::ChannelManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSupportedChannelMask(0)
    , mFavoredChannelMask(0)
#if OPENTHREAD_FTD
    , mDelay(kMinimumDelay)
#endif
    , mChannel(0)
    , mChannelSelected(0)
    , mState(kStateIdle)
    , mTimer(aInstance)
    , mAutoSelectInterval(kDefaultAutoSelectInterval)
#if OPENTHREAD_FTD
    , mAutoSelectEnabled(false)
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    , mAutoSelectCslEnabled(false)
#endif
    , mCcaFailureRateThreshold(kCcaFailureRateThreshold)
{
}

void ChannelManager::RequestChannelChange(uint8_t aChannel)
{
#if OPENTHREAD_FTD
    if (Get<Mle::Mle>().IsFullThreadDevice() && Get<Mle::Mle>().IsRxOnWhenIdle() && mAutoSelectEnabled)
    {
        RequestNetworkChannelChange(aChannel);
    }
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    if (mAutoSelectCslEnabled)
    {
        ChangeCslChannel(aChannel);
    }
#endif
}

#if OPENTHREAD_FTD
void ChannelManager::RequestNetworkChannelChange(uint8_t aChannel)
{
    // Check requested channel != current channel
    if (aChannel == Get<Mac::Mac>().GetPanChannel())
    {
        LogInfo("Already operating on the requested channel %d", aChannel);
        ExitNow();
    }

    LogInfo("Request to change to channel %d with delay %d sec", aChannel, mDelay);
    if (mState == kStateChangeInProgress)
    {
        VerifyOrExit(mChannel != aChannel);
    }

    mState   = kStateChangeRequested;
    mChannel = aChannel;

    mTimer.Start(1 + Random::NonCrypto::GetUint32InRange(0, kRequestStartJitterInterval));

    Get<Notifier>().Signal(kEventChannelManagerNewChannelChanged);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
void ChannelManager::ChangeCslChannel(uint8_t aChannel)
{
    if (!(!Get<Mle::Mle>().IsRxOnWhenIdle() && Get<Mac::Mac>().IsCslEnabled()))
    {
        // cannot select or use other channel
        ExitNow();
    }

    if (aChannel == Get<Mac::Mac>().GetCslChannel())
    {
        LogInfo("Already operating on the requested channel %d", aChannel);
        ExitNow();
    }

    VerifyOrExit(Radio::IsCslChannelValid(aChannel));

    LogInfo("Change to Csl channel %d now.", aChannel);

    mChannel = aChannel;
    Get<Mac::Mac>().SetCslChannel(aChannel);

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE

#if OPENTHREAD_FTD
Error ChannelManager::SetDelay(uint16_t aDelay)
{
    Error error = kErrorNone;

    VerifyOrExit(aDelay >= kMinimumDelay, error = kErrorInvalidArgs);
    mDelay = aDelay;

exit:
    return error;
}

void ChannelManager::StartDatasetUpdate(void)
{
    MeshCoP::Dataset::Info dataset;

    dataset.Clear();
    dataset.Set<MeshCoP::Dataset::kChannel>(mChannel);
    dataset.Set<MeshCoP::Dataset::kDelay>(Time::SecToMsec(mDelay));

    switch (Get<MeshCoP::DatasetUpdater>().RequestUpdate(dataset, HandleDatasetUpdateDone, this))
    {
    case kErrorNone:
        mState = kStateChangeInProgress;
        // Wait for the `HandleDatasetUpdateDone()` callback.
        break;

    case kErrorBusy:
    case kErrorNoBufs:
        mTimer.Start(kPendingDatasetTxRetryInterval);
        break;

    case kErrorInvalidState:
        LogInfo("Request to change to channel %d failed. Device is disabled", mChannel);

        OT_FALL_THROUGH;

    default:
        mState = kStateIdle;
        StartAutoSelectTimer();
        break;
    }
}

void ChannelManager::HandleDatasetUpdateDone(Error aError, void *aContext)
{
    static_cast<ChannelManager *>(aContext)->HandleDatasetUpdateDone(aError);
}

void ChannelManager::HandleDatasetUpdateDone(Error aError)
{
    if (aError == kErrorNone)
    {
        LogInfo("Channel changed to %d", mChannel);
    }
    else
    {
        LogInfo("Canceling channel change to %d%s", mChannel,
                (aError == kErrorAlready) ? " since current ActiveDataset is more recent" : "");
    }

    mState = kStateIdle;
    StartAutoSelectTimer();
}
#endif // OPENTHREAD_FTD

void ChannelManager::HandleTimer(void)
{
    switch (mState)
    {
    case kStateIdle:
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
        LogInfo("Auto-triggered channel select");
        IgnoreError(RequestAutoChannelSelect(false));
#endif
        StartAutoSelectTimer();
        break;

    case kStateChangeRequested:
#if OPENTHREAD_FTD
        StartDatasetUpdate();
#endif
        break;

    case kStateChangeInProgress:
        break;
    }
}

#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE

Error ChannelManager::FindBetterChannel(uint8_t &aNewChannel, uint16_t &aOccupancy)
{
    Error            error = kErrorNone;
    Mac::ChannelMask favoredAndSupported;
    Mac::ChannelMask favoredBest;
    Mac::ChannelMask supportedBest;
    uint16_t         favoredOccupancy;
    uint16_t         supportedOccupancy;

    if (Get<ChannelMonitor>().GetSampleCount() <= kMinChannelMonitorSampleCount)
    {
        LogInfo("Too few samples (%lu <= %lu) to select channel", ToUlong(Get<ChannelMonitor>().GetSampleCount()),
                ToUlong(kMinChannelMonitorSampleCount));
        ExitNow(error = kErrorInvalidState);
    }

    favoredAndSupported = mFavoredChannelMask;
    favoredAndSupported.Intersect(mSupportedChannelMask);

    favoredBest   = Get<ChannelMonitor>().FindBestChannels(favoredAndSupported, favoredOccupancy);
    supportedBest = Get<ChannelMonitor>().FindBestChannels(mSupportedChannelMask, supportedOccupancy);

    LogInfo("Best favored %s, occupancy 0x%04x", favoredBest.ToString().AsCString(), favoredOccupancy);
    LogInfo("Best overall %s, occupancy 0x%04x", supportedBest.ToString().AsCString(), supportedOccupancy);

    // Prefer favored channels unless there is no favored channel,
    // or the occupancy rate of the best favored channel is worse
    // than the best overall by at least `kThresholdToSkipFavored`.

    if (favoredBest.IsEmpty() || ((favoredOccupancy >= kThresholdToSkipFavored) &&
                                  (supportedOccupancy < favoredOccupancy - kThresholdToSkipFavored)))
    {
        if (!favoredBest.IsEmpty())
        {
            LogInfo("Preferring an unfavored channel due to high occupancy rate diff");
        }

        favoredBest      = supportedBest;
        favoredOccupancy = supportedOccupancy;
    }

    VerifyOrExit(!favoredBest.IsEmpty(), error = kErrorNotFound);

    aNewChannel = favoredBest.ChooseRandomChannel();
    aOccupancy  = favoredOccupancy;

exit:
    return error;
}

bool ChannelManager::ShouldAttemptChannelChange(void)
{
    uint16_t ccaFailureRate = Get<Mac::Mac>().GetCcaFailureRate();
    bool     shouldAttempt  = (ccaFailureRate >= mCcaFailureRateThreshold);

    LogInfo("CCA-err-rate: 0x%04x %s 0x%04x, selecting channel: %s", ccaFailureRate, shouldAttempt ? ">=" : "<",
            mCcaFailureRateThreshold, ToYesNo(shouldAttempt));

    return shouldAttempt;
}

#if OPENTHREAD_FTD
Error ChannelManager::RequestNetworkChannelSelect(bool aSkipQualityCheck)
{
    Error error = kErrorNone;

    SuccessOrExit(error = RequestChannelSelect(aSkipQualityCheck));
    RequestNetworkChannelChange(mChannelSelected);

exit:
    if ((error == kErrorAbort) || (error == kErrorAlready))
    {
        // ignore aborted channel change
        error = kErrorNone;
    }
    return error;
}
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
Error ChannelManager::RequestCslChannelSelect(bool aSkipQualityCheck)
{
    Error error = kErrorNone;

    SuccessOrExit(error = RequestChannelSelect(aSkipQualityCheck));
    ChangeCslChannel(mChannelSelected);

exit:
    if ((error == kErrorAbort) || (error == kErrorAlready))
    {
        // ignore aborted channel change
        error = kErrorNone;
    }
    return error;
}
#endif

Error ChannelManager::RequestAutoChannelSelect(bool aSkipQualityCheck)
{
    Error error = kErrorNone;

    SuccessOrExit(error = RequestChannelSelect(aSkipQualityCheck));
    RequestChannelChange(mChannelSelected);

exit:
    return error;
}

Error ChannelManager::RequestChannelSelect(bool aSkipQualityCheck)
{
    Error    error = kErrorNone;
    uint8_t  curChannel, newChannel;
    uint16_t curOccupancy, newOccupancy;

    LogInfo("Request to select channel (skip quality check: %s)", ToYesNo(aSkipQualityCheck));

    VerifyOrExit(!Get<Mle::Mle>().IsDisabled(), error = kErrorInvalidState);

    VerifyOrExit(aSkipQualityCheck || ShouldAttemptChannelChange(), error = kErrorAbort);

    SuccessOrExit(error = FindBetterChannel(newChannel, newOccupancy));

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled() && (Get<Mac::Mac>().GetCslChannel() != 0))
    {
        curChannel = Get<Mac::Mac>().GetCslChannel();
    }
    else
#endif
    {
        curChannel = Get<Mac::Mac>().GetPanChannel();
    }

    curOccupancy = Get<ChannelMonitor>().GetChannelOccupancy(curChannel);

    if (newChannel == curChannel)
    {
        LogInfo("Already on best possible channel %d", curChannel);
        ExitNow(error = kErrorAlready);
    }

    LogInfo("Cur channel %d, occupancy 0x%04x - Best channel %d, occupancy 0x%04x", curChannel, curOccupancy,
            newChannel, newOccupancy);

    // Switch only if new channel's occupancy rate is better than current
    // channel's occupancy rate by threshold `kThresholdToChangeChannel`.

    if ((newOccupancy >= curOccupancy) ||
        (static_cast<uint16_t>(curOccupancy - newOccupancy) < kThresholdToChangeChannel))
    {
        LogInfo("Occupancy rate diff too small to change channel");
        ExitNow(error = kErrorAbort);
    }

    mChannelSelected = newChannel;

exit:
    LogWarnOnError(error, "select better channel");
    return error;
}
#endif // OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE

void ChannelManager::StartAutoSelectTimer(void)
{
    VerifyOrExit(mState == kStateIdle);

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)
    if (mAutoSelectEnabled || mAutoSelectCslEnabled)
#elif OPENTHREAD_FTD
    if (mAutoSelectEnabled)
#elif OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    if (mAutoSelectCslEnabled)
#endif
    {
        mTimer.Start(Time::SecToMsec(mAutoSelectInterval));
    }
    else
    {
        mTimer.Stop();
    }

exit:
    return;
}

#if OPENTHREAD_FTD
void ChannelManager::SetAutoNetworkChannelSelectionEnabled(bool aEnabled)
{
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    if (aEnabled != mAutoSelectEnabled)
    {
        mAutoSelectEnabled = aEnabled;
        IgnoreError(RequestNetworkChannelSelect(false));
        StartAutoSelectTimer();
    }
#endif
}
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
void ChannelManager::SetAutoCslChannelSelectionEnabled(bool aEnabled)
{
    if (aEnabled != mAutoSelectCslEnabled)
    {
        mAutoSelectCslEnabled = aEnabled;
        IgnoreError(RequestAutoChannelSelect(false));
        StartAutoSelectTimer();
    }
}
#endif

Error ChannelManager::SetAutoChannelSelectionInterval(uint32_t aInterval)
{
    Error    error        = kErrorNone;
    uint32_t prevInterval = mAutoSelectInterval;

    VerifyOrExit((aInterval != 0) && (aInterval <= Time::MsecToSec(Timer::kMaxDelay)), error = kErrorInvalidArgs);

    mAutoSelectInterval = aInterval;

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)
    if (mAutoSelectEnabled || mAutoSelectCslEnabled)
#elif OPENTHREAD_FTD
    if (mAutoSelectEnabled)
#elif OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    if (mAutoSelectCslEnabled)
#endif
    {
        if ((mState == kStateIdle) && mTimer.IsRunning() && (prevInterval != aInterval))
        {
            mTimer.StartAt(mTimer.GetFireTime() - Time::SecToMsec(prevInterval), Time::SecToMsec(aInterval));
        }
    }

exit:
    return error;
}

void ChannelManager::SetSupportedChannels(uint32_t aChannelMask)
{
    mSupportedChannelMask.SetMask(aChannelMask & Get<Mac::Mac>().GetSupportedChannelMask().GetMask());

    LogInfo("Supported channels: %s", mSupportedChannelMask.ToString().AsCString());
}

void ChannelManager::SetFavoredChannels(uint32_t aChannelMask)
{
    mFavoredChannelMask.SetMask(aChannelMask & Get<Mac::Mac>().GetSupportedChannelMask().GetMask());

    LogInfo("Favored channels: %s", mFavoredChannelMask.ToString().AsCString());
}

void ChannelManager::SetCcaFailureRateThreshold(uint16_t aThreshold)
{
    mCcaFailureRateThreshold = aThreshold;

    LogInfo("CCA threshold: 0x%04x", mCcaFailureRateThreshold);
}

} // namespace Utils
} // namespace ot

#endif // #if (OPENTHREAD_FTD || OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)
#endif // #if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE
