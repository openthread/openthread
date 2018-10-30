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
#include "common/random.hpp"

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
    memset(mChannelOccupancy, 0, sizeof(mChannelOccupancy));
}

otError ChannelMonitor::Start(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsRunning(), error = OT_ERROR_ALREADY);
    Clear();
    mTimer.Start(kTimerInterval);
    otLogDebgUtil("ChannelMonitor: Starting");

exit:
    return error;
}

otError ChannelMonitor::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsRunning(), error = OT_ERROR_ALREADY);
    mTimer.Stop();
    otLogDebgUtil("ChannelMonitor: Stopping");

exit:
    return error;
}

void ChannelMonitor::Clear(void)
{
    mChannelMaskIndex = 0;
    mSampleCount      = 0;
    memset(mChannelOccupancy, 0, sizeof(mChannelOccupancy));

    otLogDebgUtil("ChannelMonitor: Clearing data");
}

uint16_t ChannelMonitor::GetChannelOccupancy(uint8_t aChannel) const
{
    uint16_t occupancy = 0;

    VerifyOrExit((OT_RADIO_CHANNEL_MIN <= aChannel) && (aChannel <= OT_RADIO_CHANNEL_MAX));
    occupancy = mChannelOccupancy[aChannel - OT_RADIO_CHANNEL_MIN];

exit:
    return occupancy;
}

void ChannelMonitor::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<ChannelMonitor>().HandleTimer();
}

void ChannelMonitor::HandleTimer(void)
{
    GetInstance().Get<Mac::Mac>().EnergyScan(mScanChannelMasks[mChannelMaskIndex], 0,
                                             &ChannelMonitor::HandleEnergyScanResult, this);

    mTimer.StartAt(mTimer.GetFireTime(), Random::AddJitter(kTimerInterval, kMaxJitterInterval));
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
        uint32_t newAverage   = mChannelOccupancy[channelIndex];
        uint32_t newValue     = 0;
        uint32_t weight;

        assert(channelIndex < kNumChannels);

        otLogDebgUtil("ChannelMonitor: channel: %d, rssi:%d", aResult->mChannel, aResult->mMaxRssi);

        if (aResult->mMaxRssi != OT_RADIO_RSSI_INVALID)
        {
            newValue = (aResult->mMaxRssi >= kRssiThreshold) ? kMaxOccupancy : 0;
        }

        // `mChannelOccupancy` stores the average rate/percentage of RSS
        // samples that are higher than a given RSS threshold ("bad" RSS
        // samples). For the first `kSampleWindow` samples, the average is
        // maintained as the actual percentage (i.e., ratio of number of
        // "bad" samples by total number of samples). After `kSampleWindow`
        // samples, the averager uses an exponentially weighted moving
        // average logic with weight coefficient `1/kSampleWindow` for new
        // values. Practically, this means the average is representative
        // of up to `3 * kSampleWindow` samples with highest weight given
        // to the latest `kSampleWindow` samples.

        if (mSampleCount >= kSampleWindow)
        {
            weight = kSampleWindow - 1;
        }
        else
        {
            weight = mSampleCount;
        }

        newAverage = (newAverage * weight + newValue) / (weight + 1);

        mChannelOccupancy[channelIndex] = static_cast<uint16_t>(newAverage);
    }
}

void ChannelMonitor::LogResults(void)
{
    otLogInfoUtil(
        "ChannelMonitor: %u [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",
        mSampleCount, mChannelOccupancy[0] >> 8, mChannelOccupancy[1] >> 8, mChannelOccupancy[2] >> 8,
        mChannelOccupancy[3] >> 8, mChannelOccupancy[4] >> 8, mChannelOccupancy[5] >> 8, mChannelOccupancy[6] >> 8,
        mChannelOccupancy[7] >> 8, mChannelOccupancy[8] >> 8, mChannelOccupancy[9] >> 8, mChannelOccupancy[10] >> 8,
        mChannelOccupancy[11] >> 8, mChannelOccupancy[12] >> 8, mChannelOccupancy[13] >> 8, mChannelOccupancy[14] >> 8,
        mChannelOccupancy[15] >> 8);
}

Mac::ChannelMask ChannelMonitor::FindBestChannels(const Mac::ChannelMask &aMask, uint16_t &aOccupancy)
{
    uint8_t          channel;
    Mac::ChannelMask bestMask;
    uint16_t         minOccupancy = 0xffff;

    bestMask.Clear();

    channel = Mac::ChannelMask::kChannelIteratorFirst;

    while (aMask.GetNextChannel(channel) == OT_ERROR_NONE)
    {
        uint16_t occupancy = GetChannelOccupancy(channel);

        if (bestMask.IsEmpty() || (occupancy <= minOccupancy))
        {
            if (occupancy < minOccupancy)
            {
                bestMask.Clear();
            }

            bestMask.AddChannel(channel);
            minOccupancy = occupancy;
        }
    }

    aOccupancy = minOccupancy;

    return bestMask;
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_CHANNEL_MONITOR
