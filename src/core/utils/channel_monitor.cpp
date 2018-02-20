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
 *   This file implements the channel monitoring module.
 */

#include "channel_monitor.hpp"

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR

namespace ot {
namespace Utils {

const uint32_t ChannelMonitor::mScanChannelMasks[kNumChannelMasks] = {
    OT_CHANNEL_11_MASK | OT_CHANNEL_15_MASK | OT_CHANNEL_19_MASK | OT_CHANNEL_23_MASK,
    OT_CHANNEL_12_MASK | OT_CHANNEL_16_MASK | OT_CHANNEL_20_MASK | OT_CHANNEL_24_MASK,
    OT_CHANNEL_13_MASK | OT_CHANNEL_17_MASK | OT_CHANNEL_21_MASK | OT_CHANNEL_25_MASK,
    OT_CHANNEL_14_MASK | OT_CHANNEL_18_MASK | OT_CHANNEL_22_MASK | OT_CHANNEL_26_MASK,
};

ChannelMonitor::ChannelMonitor(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMaskIndex(0)
    , mSampleCount(0)
    , mTimer(aInstance, &ChannelMonitor::HandleTimer, this)
{
    memset(mChannelQuality, 0, sizeof(mChannelQuality));
}

void ChannelMonitor::Start(void)
{
    Clear();
    mTimer.Start(kTimerInterval);
    otLogDebgUtil(GetInstance(), "ChannelMonitor: Starting");
}

void ChannelMonitor::Stop(void)
{
    mTimer.Stop();
    otLogDebgUtil(GetInstance(), "ChannelMonitor: Stopping");
}

void ChannelMonitor::Clear(void)
{
    mChannelMaskIndex = 0;
    mSampleCount      = 0;
    memset(mChannelQuality, 0, sizeof(mChannelQuality));

    otLogDebgUtil(GetInstance(), "ChannelMonitor: Clearing data");
}

uint16_t ChannelMonitor::GetChannelQuality(uint8_t aChannel) const
{
    uint16_t quality = 0;

    VerifyOrExit((OT_RADIO_CHANNEL_MIN <= aChannel) && (aChannel <= OT_RADIO_CHANNEL_MAX));
    quality = mChannelQuality[aChannel - OT_RADIO_CHANNEL_MIN];

exit:
    return quality;
}

void ChannelMonitor::RestartTimer(void)
{
    uint16_t interval = kTimerInterval;
    int16_t  jitter;

    jitter = (otPlatRandomGet() % (2 * kMaxJitterInterval)) - kMaxJitterInterval;

    if (jitter >= kTimerInterval)
    {
        jitter = kTimerInterval - 1;
    }

    if (jitter <= -kTimerInterval)
    {
        jitter = -kTimerInterval + 1;
    }

    interval += jitter;
    mTimer.StartAt(mTimer.GetFireTime(), interval);

    otLogDebgUtil(GetInstance(), "ChannelMonitor: Timer interval %u, jitter %d", interval, jitter);
}

void ChannelMonitor::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ChannelMonitor>().HandleTimer();
}

void ChannelMonitor::HandleTimer(void)
{
    GetInstance().Get<Mac::Mac>().EnergyScan(mScanChannelMasks[mChannelMaskIndex], 0,
                                             &ChannelMonitor::HandleEnergyScanResult, this);
    RestartTimer();
}

void ChannelMonitor::HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult)
{
    static_cast<ChannelMonitor *>(aContext)->HandleEnergyScanResult(aResult);
}

void ChannelMonitor::HandleEnergyScanResult(otEnergyScanResult *aResult)
{
    if (aResult == NULL)
    {
        if (mChannelMaskIndex == kNumChannelMasks - 1)
        {
            mChannelMaskIndex = 0;
            mSampleCount++;
            LogResults();
        }
        else
        {
            mChannelMaskIndex++;
        }
    }
    else
    {
        uint8_t  channelIndex = (aResult->mChannel - OT_RADIO_CHANNEL_MIN);
        uint32_t newAverage   = mChannelQuality[channelIndex];
        uint32_t newValue     = 0;
        uint32_t weight;

        assert(channelIndex < kNumChannels);

        otLogDebgUtil(GetInstance(), "ChannelMonitor: channel: %d, rssi:%d", aResult->mChannel, aResult->mMaxRssi);

        if (aResult->mMaxRssi != OT_RADIO_RSSI_INVALID)
        {
            newValue = (aResult->mMaxRssi >= kRssiThreshold) ? kMaxQualityIndicator : 0;
        }

        // `mChannelQuality` stores the average rate/percentage of RSS samples
        // that are higher than a given RSS threshold ("bad" RSS samples). For
        // the first `kSampleWindow` samples, the average is maintained as the
        // actual percentage (i.e., ratio of number of "bad" samples by total
        // number of samples). After `kSampleWindow` samples, the averager
        // uses an exponentially weighted moving average logic with weight
        // coefficient `1/kSampleWindow` for new values. Practically, this
        // means the quality is representative of up to `3 * kSampleWindow`
        // last samples with highest weight given to latest `kSampleWindow`
        // samples.

        if (mSampleCount >= kSampleWindow)
        {
            weight = kSampleWindow - 1;
        }
        else
        {
            weight = mSampleCount;
        }

        newAverage = (newAverage * weight + newValue) / (weight + 1);

        mChannelQuality[channelIndex] = static_cast<uint16_t>(newAverage);
    }
}

void ChannelMonitor::LogResults(void)
{
    otLogInfoUtil(
        GetInstance(),
        "ChannelMonitor: %u [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",
        mSampleCount, mChannelQuality[0] >> 8, mChannelQuality[1] >> 8, mChannelQuality[2] >> 8,
        mChannelQuality[3] >> 8, mChannelQuality[4] >> 8, mChannelQuality[5] >> 8, mChannelQuality[6] >> 8,
        mChannelQuality[7] >> 8, mChannelQuality[8] >> 8, mChannelQuality[9] >> 8, mChannelQuality[10] >> 8,
        mChannelQuality[11] >> 8, mChannelQuality[12] >> 8, mChannelQuality[13] >> 8, mChannelQuality[14] >> 8,
        mChannelQuality[15] >> 8);
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_CHANNEL_MONITOR
