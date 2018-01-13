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
 *   This file includes definitions for channel monitoring module.
 */

#ifndef CHANNEL_MONITOR_HPP_
#define CHANNEL_MONITOR_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>
#include <openthread/types.h>

#include "common/locator.hpp"
#include "common/timer.hpp"

namespace ot {
namespace Utils {

/**
 * @addtogroup utils-channel-monitor
 *
 * @brief
 *   This module includes definitions monitoring quality of channels.
 *
 * @{
 */

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR

/**
 * This class implements the channel monitoring logic.
 *
 * Channel Monitoring will periodically monitor all channels to help determine the cleaner channels (channels
 * with less interference).
 *
 * When Channel Monitoring is active, every `kSampleInterval`, a zero-duration Energy Scan is performed on every
 * channel collecting a single RSSI  sample per channel. The RSSI samples are compared with a pre-specified RSSI
 * threshold `kRssiThreshold`. As an indicator of channel quality, the `ChannelMonitor` maintains and provides the
 * average rate/percentage of RSSI samples that are above the threshold within (approximately) a specified sample window.
 *
 */
class ChannelMonitor : public InstanceLocator
{
public:

    enum
    {
        /**
         * The channel RSSI sample interval in milliseconds.
         *
         */
        kSampleInterval = OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_INTERVAL,

        /**
         * The RSSI threshold in dBm.
         *
         * It is recommended that this value is set to same value as the CCA threshold used by radio.
         *
         */
        kRssiThreshold = OPENTHREAD_CONFIG_CHANNEL_MONITOR_RSSI_THRESHOLD,

        /**
         * The averaging sample window length (in units of sample interval).
         *
         */
        kSampleWindow = OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_WINDOW,
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    ChannelMonitor(Instance &aInstance);

    /**
     * This method starts the Channel Monitoring operation.
     *
     * All previous data is cleared when the Channel Monitoring operation starts.
     *
     */
    void Start(void);

    /**
     * This method stops the Channel Monitoring operation.
     *
     * The previous data is still valid and can be read while the operation is stopped.
     *
     */
    void Stop(void);

    /**
     * This method clears all currently stored data.
     *
     */
    void Clear(void);

    /**
     * This method returns the total number of RSSI samples (per channel) taken so far (since call to `Start()`).
     *
     * @returns total number of RSSI sample taken since last call to `Start()`.
     *
     */
    uint32_t GetSampleCount(void) const { return mSampleCount; }

    /**
     * This method returns the current channel quality value for a given channel.
     *
     * The channel quality value represents the average rate/percentage of RSSI samples that were above RSSI threshold
     * `kRssiThreshold` ("bad" RSSI samples).
     *
     * For the first `kSampleWindow` samples, the average is maintained as the actual percentage (i.e., ratio of number
     * of "bad" samples by total number of samples). After `kSampleWindow` samples, the averager uses an exponentially
     * weighted moving average logic with weight coefficient `1/kSampleWindow` for new values. Practically, this means
     * the quality is representative of up to `3 * kSampleWindow` last samples with highest weight given to latest
     * `kSampleWindow` samples.
     *
     * Max value of `0xffff` indicates all RSSI samples were above RSSI threshold (i.e. 100% of samples were "bad").
     *
     * @param[in]  aChannel     The channel for which to get the link quality.
     *
     * @returns the current channel quality value for the given channel.
     *
     */
    uint16_t GetChannelQuality(uint8_t aChannel) const;

private:
    enum
    {
        kNumChannels = (OT_RADIO_CHANNEL_MAX - OT_RADIO_CHANNEL_MIN + 1),
        kNumChannelMasks = 4,
        kTimerInterval = (kSampleInterval / kNumChannelMasks),
        kMaxJitterInterval = 4096,
        kMaxQualityIndicator = 0xffff,
    };

    void RestartTimer(void);
    static void HandleTimer(Timer &aTimer);
    void HandleTimer(void);
    static void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);
    void LogResults(void);

    static const uint32_t mScanChannelMasks[kNumChannelMasks];

    uint8_t  mChannelMaskIndex : 2;
    uint32_t mSampleCount      : 30;
    uint16_t mChannelQuality[kNumChannels];
    TimerMilli mTimer;
};

#endif // OPENTHREAD_ENABLE_CHANNEL_MONITOR

/**
 * @}
 *
 */

}  // namespace Utils
}  // namespace ot

#endif  // CHANNEL_MONITOR_HPP_
