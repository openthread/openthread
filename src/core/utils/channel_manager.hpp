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

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE

#if (OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE && !OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE)
#error "CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE requires OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE";
#endif

#if (OPENTHREAD_FTD || OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)

#include <openthread/channel_manager.h>

#include <openthread/platform/radio.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
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

/**
 * Implements the Channel Manager.
 */
class ChannelManager : public InstanceLocator, private NonCopyable
{
public:
#if OPENTHREAD_FTD
    /**
     * Minimum delay (in seconds) used for network channel change.
     */
    static constexpr uint16_t kMinimumDelay = OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY;
#endif

    /**
     * Initializes a `ChanelManager` object.
     *
     * @param[in]   aInstance  A reference to the OpenThread instance.
     */
    explicit ChannelManager(Instance &aInstance);

#if OPENTHREAD_FTD
    /**
     * Requests a Thread network channel change.
     *
     * The Thread network switches to the given channel after a specified delay (@sa GetDelay()). The channel change is
     * performed by updating the Pending Operational Dataset.
     *
     * A subsequent call to this method will cancel an ongoing previously requested channel change.
     *
     * If the requested channel changes, it will trigger a `Notifier` event `kEventChannelManagerNewChannelChanged`.
     *
     * @param[in] aChannel             The new channel for the Thread network.
     */
    void RequestNetworkChannelChange(uint8_t aChannel);
#endif

    /**
     * Gets the channel from the last successful call to `RequestNetworkChannelChange()` or `ChangeCslChannel()`.
     *
     * @returns The last requested channel, or zero if there has been no channel change request yet.
     */
    uint8_t GetRequestedChannel(void) const { return mChannel; }

#if OPENTHREAD_FTD
    /**
     * Gets the delay (in seconds) used for a channel change.
     *
     * @returns The delay (in seconds)
     */
    uint16_t GetDelay(void) const { return mDelay; }

    /**
     * Sets the delay (in seconds) used for a channel change.
     *
     * The delay should preferably be longer than maximum data poll interval used by all sleepy-end-devices within the
     * Thread network.
     *
     * @param[in]  aDelay             Delay in seconds.
     *
     * @retval kErrorNone          Delay was updated successfully.
     * @retval kErrorInvalidArgs   The given delay @p aDelay is shorter than `kMinimumDelay`.
     */
    Error SetDelay(uint16_t aDelay);
#endif // OPENTHREAD_FTD

#if OPENTHREAD_FTD
    /**
     * Requests that `ChannelManager` checks and selects a new network channel and starts a network channel change.
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
     *    channel change process (internally invoking a `RequestNetworkChannelChange()`).
     *
     *
     * @param[in] aSkipQualityCheck        Indicates whether the quality check (step 1) should be skipped.
     *
     * @retval kErrorNone              Channel selection finished successfully.
     * @retval kErrorNotFound          Supported channels is empty, therefore could not select a channel.
     * @retval kErrorInvalidState      Thread is not enabled or not enough data to select new channel.
     */
    Error RequestNetworkChannelSelect(bool aSkipQualityCheck);
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    /**
     * Requests that `ChannelManager` checks and selects a new Csl channel and starts a channel change.
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
     * 3) If the newly selected channel is different from the current Csl channel, `ChannelManager` starts the
     *    channel change process (internally invoking a `ChangeCslChannel()`).
     *
     *
     * @param[in] aSkipQualityCheck        Indicates whether the quality check (step 1) should be skipped.
     *
     * @retval kErrorNone              Channel selection finished successfully.
     * @retval kErrorNotFound          Supported channels is empty, therefore could not select a channel.
     * @retval kErrorInvalidState      Thread is not enabled or not enough data to select new channel.
     */
    Error RequestCslChannelSelect(bool aSkipQualityCheck);
#endif // OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE

#if OPENTHREAD_FTD
    /**
     * Enables/disables the auto-channel-selection functionality.
     *
     * When enabled, `ChannelManager` will periodically invoke a `RequestChannelSelect(false)`. The period interval
     * can be set by `SetAutoChannelSelectionInterval()`.
     *
     * @param[in]  aEnabled  Indicates whether to enable or disable this functionality.
     */
    void SetAutoNetworkChannelSelectionEnabled(bool aEnabled);

    /**
     * Indicates whether the auto-channel-selection functionality is enabled or not.
     *
     * @returns TRUE if enabled, FALSE if disabled.
     */
    bool GetAutoNetworkChannelSelectionEnabled(void) const { return mAutoSelectEnabled; }
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    /**
     * Enables/disables the auto-channel-selection functionality.
     *
     * When enabled, `ChannelManager` will periodically invoke a `RequestChannelSelect(false)`. The period interval
     * can be set by `SetAutoChannelSelectionInterval()`.
     *
     * @param[in]  aEnabled  Indicates whether to enable or disable this functionality.
     */
    void SetAutoCslChannelSelectionEnabled(bool aEnabled);

    /**
     * Indicates whether the auto-channel-selection functionality is enabled or not.
     *
     * @returns TRUE if enabled, FALSE if disabled.
     */
    bool GetAutoCslChannelSelectionEnabled(void) const { return mAutoSelectCslEnabled; }
#endif

    /**
     * Sets the period interval (in seconds) used by auto-channel-selection functionality.
     *
     * @param[in] aInterval            The interval (in seconds).
     *
     * @retval kErrorNone          The interval was set successfully.
     * @retval kErrorInvalidArgs   The @p aInterval is not valid (zero).
     */
    Error SetAutoChannelSelectionInterval(uint32_t aInterval);

    /**
     * Gets the period interval (in seconds) used by auto-channel-selection functionality.
     *
     * @returns The interval (in seconds).
     */
    uint32_t GetAutoChannelSelectionInterval(void) const { return mAutoSelectInterval; }

    /**
     * Gets the supported channel mask.
     *
     * @returns  The supported channels mask.
     */
    uint32_t GetSupportedChannels(void) const { return mSupportedChannelMask.GetMask(); }

    /**
     * Sets the supported channel mask.
     *
     * @param[in]  aChannelMask  A channel mask.
     */
    void SetSupportedChannels(uint32_t aChannelMask);

    /**
     * Gets the favored channel mask.
     *
     * @returns  The favored channels mask.
     */
    uint32_t GetFavoredChannels(void) const { return mFavoredChannelMask.GetMask(); }

    /**
     * Sets the favored channel mask.
     *
     * @param[in]  aChannelMask  A channel mask.
     */
    void SetFavoredChannels(uint32_t aChannelMask);

    /**
     * Gets the CCA failure rate threshold
     *
     * @returns  The CCA failure rate threshold
     */
    uint16_t GetCcaFailureRateThreshold(void) const { return mCcaFailureRateThreshold; }

    /**
     * Sets the CCA failure rate threshold
     *
     * @param[in]  aThreshold  A CCA failure rate threshold.
     */
    void SetCcaFailureRateThreshold(uint16_t aThreshold);

private:
    // Retry interval to resend Pending Dataset in case of tx failure (in ms).
    static constexpr uint32_t kPendingDatasetTxRetryInterval = 20000;

    // Maximum jitter/wait time to start a requested network channel change (in ms).
    static constexpr uint32_t kRequestStartJitterInterval = 10000;

    // The minimum number of RSSI samples required before using the collected data (by `ChannelMonitor`) to select
    // a channel.
    static constexpr uint32_t kMinChannelMonitorSampleCount =
        OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_MONITOR_SAMPLE_COUNT;

    // Minimum channel occupancy difference to prefer an unfavored channel over a favored one.
    static constexpr uint16_t kThresholdToSkipFavored = OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED;

    // Minimum channel occupancy difference between current channel and the selected channel to trigger the channel
    // change process to start.
    static constexpr uint16_t kThresholdToChangeChannel = OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL;

    // Default auto-channel-selection period (in seconds).
    static constexpr uint32_t kDefaultAutoSelectInterval =
        OPENTHREAD_CONFIG_CHANNEL_MANAGER_DEFAULT_AUTO_SELECT_INTERVAL;

    // Minimum CCA failure rate on current channel to start the channel selection process.
    static constexpr uint16_t kCcaFailureRateThreshold = OPENTHREAD_CONFIG_CHANNEL_MANAGER_CCA_FAILURE_THRESHOLD;

    enum State : uint8_t
    {
        kStateIdle,
        kStateChangeRequested,
        kStateChangeInProgress,
    };

#if OPENTHREAD_FTD
    void        StartDatasetUpdate(void);
    static void HandleDatasetUpdateDone(Error aError, void *aContext);
    void        HandleDatasetUpdateDone(Error aError);
#endif
    void  HandleTimer(void);
    void  StartAutoSelectTimer(void);
    Error RequestChannelSelect(bool aSkipQualityCheck);
    Error RequestAutoChannelSelect(bool aSkipQualityCheck);
    void  RequestChannelChange(uint8_t aChannel);

#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    Error FindBetterChannel(uint8_t &aNewChannel, uint16_t &aOccupancy);
    bool  ShouldAttemptChannelChange(void);
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    void ChangeCslChannel(uint8_t aChannel);
#endif

    using ManagerTimer = TimerMilliIn<ChannelManager, &ChannelManager::HandleTimer>;

    Mac::ChannelMask mSupportedChannelMask;
    Mac::ChannelMask mFavoredChannelMask;
#if OPENTHREAD_FTD
    uint16_t mDelay;
#endif
    uint8_t      mChannel;
    uint8_t      mChannelSelected;
    State        mState;
    ManagerTimer mTimer;
    uint32_t     mAutoSelectInterval;
#if OPENTHREAD_FTD
    bool mAutoSelectEnabled;
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE
    bool mAutoSelectCslEnabled;
#endif
    uint16_t mCcaFailureRateThreshold;
};

/**
 * @}
 */

} // namespace Utils
} // namespace ot

#endif // #if (OPENTHREAD_FTD || OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)
#endif // #if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE

#endif // CHANNEL_MANAGER_HPP_
