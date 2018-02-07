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
#include <openthread/types.h>

#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"

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
         * Minimum delay in seconds used for network channel change.
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
     * @param[in] aChannel             The new channel for the Thread network.
     *
     * @retval OT_ERROR_NONE           Channel change request successfully processed.
     * @retval OT_ERROR_INVALID_ARGS   The new channel is not a supported channel.
     *
     */
    otError RequestChannelChange(uint8_t aChannel);

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
     * This method gets the supported channel mask.
     *
     * @returns  The supported channels mask.
     *
     */
    uint32_t GetSupportedChannels(void) const { return mSupportedChannels; }

    /**
     * This method sets the supported channel mask.
     *
     * @param[in]  aChannelMask  A channel mask.
     *
     */
    void SetSupportedChannels(uint32_t aChannelMask) {
        mSupportedChannels = (aChannelMask & OT_RADIO_SUPPORTED_CHANNELS);
    }

private:
    enum
    {
        kDefaultSupprotedChannelMask = OT_RADIO_SUPPORTED_CHANNELS,
        kMaxTimestampIncrease = 128,

        kPendingDatasetTxRetryInterval = 20000,    // in ms
        kChangeCheckWaitInterval = 30000,          // in ms
    };

    enum State
    {
        kStateIdle,
        kStateChangeRequested,
        kStateSentMgmtPendingDataset,
    };

    static void HandleTimer(Timer &aTimer);
    void HandleTimer(void);
    static void HandleStateChanged(Notifier::Callback &aCallback, uint32_t aChangedFlags);
    void HandleStateChanged(uint32_t aChangedFlags);
    void PreparePendingDataset(void);

    uint32_t           mSupportedChannels;
    uint64_t           mActiveTimestamp;
    Notifier::Callback mNotifierCallback;
    uint16_t           mDelay;
    uint8_t            mChannel;
    State              mState;
    TimerMilli         mTimer;
};

#else // OPENTHREAD_FTD

class ChannelManager
{
public:
    explicit ChannelManager(Instance &) { }
};

#endif // OPENTHREAD_FTD

#endif // OPENTHREAD_ENABLE_CHANNEL_MANAGER
/**
 * @}
 *
 */

}  // namespace Utils
}  // namespace ot

#endif  // CHANNEL_MANAGER_HPP_
