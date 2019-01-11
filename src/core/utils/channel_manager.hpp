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
 *   This file includes definitions for Channel Manager.
 */

#ifndef CHANNEL_MANAGER_HPP_
#define CHANNEL_MANAGER_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"

namespace ot {
namespace Utils {

/**
 * @addtogroup utils-channel-manager
 *
 * @brief
 *   This module includes definitions for Channel Manager.
 *
 * @{
 */

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER

#if OPENTHREAD_FTD

/**
 * This class implements the Channel Manager.
 *
 */
class ChannelManager : public InstanceLocator
{
public:
    enum
    {
        /**
         * Minimum delay (in seconds) used for network channel change.
         *
         */
        kMinimumDelay = OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY,
    };

    /**
     * This constructor initializes a `ChanelManager` object.
     *
     * @param[in]   aInstance  A reference to the OpenThread instance.
     *
     */
    explicit ChannelManager(Instance &aInstance);

    /**
     * This method requests a Thread network channel change.
     *
     * The Thread network switches to the given channel after a specified delay (@sa GetDelay()). The channel change is
     * performed by updating the Pending Operational Dataset.
     *
     * A subsequent call to this method will cancel an ongoing previously requested channel change.
     *
     * If the requested channel changes, it will trigger a `Notifier` event `OT_CHANGED_CHANNEL_MANAGER_NEW_CHANNEL`.
     *
     * @param[in] aChannel             The new channel for the Thread network.
     *
     */
    void RequestChannelChange(uint8_t aChannel);

    /**
     * This method gets the channel from the last successful call to `RequestChannelChange()`.
     *
     * @returns The last requested channel, or zero if there has been no channel change request yet.
     *
     */
    uint8_t GetRequestedChannel(void) const { return mChannel; }

    /**
     * This method gets the delay (in seconds) used for a channel change.
     *
     * @returns The delay (in seconds)
     *
     */
    uint16_t GetDelay(void) const { return mDelay; }

    /**
     * This method sets the delay (in seconds) used for a channel change.
     *
     * The delay should preferably be longer than maximum data poll interval used by all sleepy-end-devices within the
     * Thread network.
     *
     * @param[in]  aDelay             Delay in seconds.
     *
     * @retval OT_ERROR_NONE          Delay was updated successfully.
     * @retval OT_ERROR_INVALID_ARGS  The given delay @p aDelay is shorter than `kMinimumDelay`.
     *
     */
    otError SetDelay(uint16_t aDelay);

    /**
     * This method requests that `ChannelManager` checks and selects a new channel and starts a channel change.
     *
     * Unlike the `RequestChannelChange()`  where the channel must be given as a parameter, this method asks the
     * `ChannelManager` to select a channel by itself (based on the collected channel quality info).
     *
     * Once called, the `ChannelManager` will perform the following 3 steps:
     *
     * 1) `ChannelManager` decides if the channel change would be helpful. This check can be skipped if
     *    `aSkipQualityCheck` is set to true (forcing a channel selection to happen and skipping the quality check).
     *    This step uses the collected link quality metrics on the device (such as CCA failure rate, frame and message
     *    error rates per neighbor, etc.) to determine if the current channel quality is at the level that justifies
     *    a channel change.
     *
     * 2) If the first step passes, then `ChannelManager` selects a potentially better channel. It uses the collected
     *    channel occupancy data by `ChannelMonitor` module. The supported and favored channels are used at this step.
     *    (@sa SetSupportedChannels, @sa SetFavoredChannels).
     *
     * 3) If the newly selected channel is different from the current channel, `ChannelManager` requests/starts the
     *    channel change process (internally invoking a `RequestChannelChange()`).
     *
     *
     * @param[in] aSkipQualityCheck        Indicates whether the quality check (step 1) should be skipped.
     *
     * @retval OT_ERROR_NONE               Channel selection finished successfully.
     * @retval OT_ERROR_NOT_FOUND          Supported channels is empty, therefore could not select a channel.
     * @retval OT_ERROR_INVALID_STATE      Thread is not enabled or not enough data to select new channel.
     * @retval OT_ERROR_DISABLED_FEATURE   `ChannelMonitor` feature is disabled by build-time configuration options.
     *
     */
    otError RequestChannelSelect(bool aSkipQualityCheck);

    /**
     * This method enables/disables the auto-channel-selection functionality.
     *
     * When enabled, `ChannelManager` will periodically invoke a `RequestChannelSelect(false)`. The period interval
     * can be set by `SetAutoChannelSelectionInterval()`.
     *
     * @param[in]  aEnabled  Indicates whether to enable or disable this functionality.
     *
     */
    void SetAutoChannelSelectionEnabled(bool aEnabled);

    /**
     * This method indicates whether the auto-channel-selection functionality is enabled or not.
     *
     * @returns TRUE if enabled, FALSE if disabled.
     *
     */
    bool GetAutoChannelSelectionEnabled(void) const { return mAutoSelectEnabled; }

    /**
     * This method sets the period interval (in seconds) used by auto-channel-selection functionality.
     *
     * @param[in] aInterval            The interval (in seconds).
     *
     * @retval OT_ERROR_NONE           The interval was set successfully.
     * @retval OT_ERROR_INVALID_ARGS   The @p aInterval is not valid (zero).
     *
     */
    otError SetAutoChannelSelectionInterval(uint32_t aInterval);

    /**
     * This method gets the period interval (in seconds) used by auto-channel-selection functionality.
     *
     * @returns The interval (in seconds).
     *
     */
    uint32_t GetAutoChannelSelectionInterval(void) { return mAutoSelectInterval; }

    /**
     * This method gets the supported channel mask.
     *
     * @returns  The supported channels mask.
     *
     */
    uint32_t GetSupportedChannels(void) const { return mSupportedChannelMask.GetMask(); }

    /**
     * This method sets the supported channel mask.
     *
     * @param[in]  aChannelMask  A channel mask.
     *
     */
    void SetSupportedChannels(uint32_t aChannelMask);

    /**
     * This method gets the favored channel mask.
     *
     * @returns  The favored channels mask.
     *
     */
    uint32_t GetFavoredChannels(void) const { return mFavoredChannelMask.GetMask(); }

    /**
     * This method sets the favored channel mask.
     *
     * @param[in]  aChannelMask  A channel mask.
     *
     */
    void SetFavoredChannels(uint32_t aChannelMask);

private:
    enum
    {
        // Maximum increase of Pending/Active Dataset Timestamp per channel change request.
        kMaxTimestampIncrease = 128,

        // Retry interval to resend Pending Dataset in case of tx failure (in ms).
        kPendingDatasetTxRetryInterval = 20000,

        // Wait time after sending Pending Dataset to check whether the channel was changed (in ms).
        kChangeCheckWaitInterval = 30000,

        // Maximum jitter/wait time to start a requested channel change (in ms).
        kRequestStartJitterInterval = 10000,

        // The minimum number of RSSI samples required before using the collected data (by `ChannelMonitor`) to select
        // a channel.
        kMinChannelMonitorSampleCount = OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_MONITOR_SAMPLE_COUNT,

        // Minimum channel occupancy difference to prefer an unfavored channel over a favored one.
        kThresholdToSkipFavored = OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED,

        // Minimum channel occupancy difference between current channel and the selected channel to trigger the channel
        // change process to start.
        kThresholdToChangeChannel = OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL,

        // Default auto-channel-selection period (in seconds).
        kDefaultAutoSelectInterval = OPENTHREAD_CONFIG_CHANNEL_MANAGER_DEFAULT_AUTO_SELECT_INTERVAL,

        // Minimum CCA failure rate on current channel to start the channel selection process.
        kCcaFailureRateThreshold = OPENTHREAD_CONFIG_CHANNEL_MANAGER_CCA_FAILURE_THRESHOLD,
    };

    enum State
    {
        kStateIdle,
        kStateChangeRequested,
        kStateSentMgmtPendingDataset,
    };

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);
    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aChangedFlags);
    void        HandleStateChanged(otChangedFlags aChangedFlags);
    void        PreparePendingDataset(void);
    void        StartAutoSelectTimer(void);

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    otError FindBetterChannel(uint8_t &aNewChannel, uint16_t &aOccupancy);
    bool    ShouldAttemptChannelChange(void);
#endif

    Mac::ChannelMask   mSupportedChannelMask;
    Mac::ChannelMask   mFavoredChannelMask;
    uint64_t           mActiveTimestamp;
    Notifier::Callback mNotifierCallback;
    uint16_t           mDelay;
    uint8_t            mChannel;
    State              mState;
    TimerMilli         mTimer;
    uint32_t           mAutoSelectInterval;
    bool               mAutoSelectEnabled;
};

#else // OPENTHREAD_FTD

class ChannelManager
{
public:
    explicit ChannelManager(Instance &) {}
};

#endif // OPENTHREAD_FTD

#endif // OPENTHREAD_ENABLE_CHANNEL_MANAGER
/**
 * @}
 *
 */

} // namespace Utils
} // namespace ot

#endif // CHANNEL_MANAGER_HPP_
