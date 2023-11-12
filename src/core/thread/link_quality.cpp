/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements link quality information processing and storage.
 */

#include "link_quality.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

// This array gives the decimal point digits representing 0/8, 1/8, ..., 7/8 (does not include the '.').
static const char *const kDigitsString[8] = {
    // 0/8,  1/8,   2/8,   3/8,   4/8,   5/8,   6/8,   7/8
    "0", "125", "25", "375", "5", "625", "75", "875"};

void SuccessRateTracker::AddSample(bool aSuccess, uint16_t aWeight)
{
    uint32_t oldAverage = mFailureRate;
    uint32_t newValue   = (aSuccess) ? 0 : kMaxRateValue;
    uint32_t n          = aWeight;

    // `n/2` is added to the sum to ensure rounding the value to the nearest integer when dividing by `n`
    // (e.g., 1.2 -> 1, 3.5 -> 4).

    mFailureRate = static_cast<uint16_t>(((oldAverage * (n - 1)) + newValue + (n / 2)) / n);
}

Error RssAverager::Add(int8_t aRss)
{
    Error    error = kErrorNone;
    uint16_t newValue;

    VerifyOrExit(aRss != Radio::kInvalidRssi, error = kErrorInvalidArgs);

    // Restrict the RSS value to the closed range [-128, 0]
    // so the RSS times precision multiple can fit in 11 bits.
    aRss = Min<int8_t>(aRss, 0);

    // Multiply the RSS value by a precision multiple (currently -8).

    newValue = static_cast<uint16_t>(-aRss);
    newValue <<= kPrecisionBitShift;

    mCount += (mCount < (1 << kCoeffBitShift));
    // Maintain arithmetic mean.
    // newAverage = newValue * (1/mCount) + oldAverage * ((mCount -1)/mCount)
    mAverage = static_cast<uint16_t>(((mAverage * (mCount - 1)) + newValue) / mCount);

exit:
    return error;
}

int8_t RssAverager::GetAverage(void) const
{
    int8_t average;

    VerifyOrExit(mCount != 0, average = Radio::kInvalidRssi);

    average = -static_cast<int8_t>(mAverage >> kPrecisionBitShift);

    // Check for possible round up (e.g., average of -71.5 --> -72)

    if ((mAverage & kPrecisionBitMask) >= (kPrecision >> 1))
    {
        average--;
    }

exit:
    return average;
}

RssAverager::InfoString RssAverager::ToString(void) const
{
    InfoString string;

    VerifyOrExit(mCount != 0);
    string.Append("%d.%s", -(mAverage >> kPrecisionBitShift), kDigitsString[mAverage & kPrecisionBitMask]);

exit:
    return string;
}

void LqiAverager::Add(uint8_t aLqi)
{
    uint8_t count;

    if (mCount < UINT8_MAX)
    {
        mCount++;
    }

    count = Min(static_cast<uint8_t>(1 << kCoeffBitShift), mCount);

    mAverage = static_cast<uint8_t>(((mAverage * (count - 1)) + aLqi) / count);
}

void LinkQualityInfo::Clear(void)
{
    mRssAverager.Clear();
    SetLinkQuality(kLinkQuality0);
    mLastRss = Radio::kInvalidRssi;

    mFrameErrorRate.Clear();
    mMessageErrorRate.Clear();
}

void LinkQualityInfo::AddRss(int8_t aRss)
{
    uint8_t oldLinkQuality = kNoLinkQuality;

    VerifyOrExit(aRss != Radio::kInvalidRssi);

    mLastRss = aRss;

    if (mRssAverager.HasAverage())
    {
        oldLinkQuality = GetLinkQuality();
    }

    SuccessOrExit(mRssAverager.Add(aRss));

    SetLinkQuality(CalculateLinkQuality(GetLinkMargin(), oldLinkQuality));

exit:
    return;
}

uint8_t LinkQualityInfo::GetLinkMargin(void) const
{
    return ComputeLinkMargin(Get<Mac::SubMac>().GetNoiseFloor(), GetAverageRss());
}

LinkQualityInfo::InfoString LinkQualityInfo::ToInfoString(void) const
{
    InfoString string;

    string.Append("aveRss:%s, lastRss:%d, linkQuality:%d", mRssAverager.ToString().AsCString(), GetLastRss(),
                  GetLinkQuality());

    return string;
}

uint8_t ComputeLinkMargin(int8_t aNoiseFloor, int8_t aRss)
{
    int8_t linkMargin = aRss - aNoiseFloor;

    if (linkMargin < 0 || aRss == Radio::kInvalidRssi)
    {
        linkMargin = 0;
    }

    return static_cast<uint8_t>(linkMargin);
}

LinkQuality LinkQualityForLinkMargin(uint8_t aLinkMargin)
{
    return LinkQualityInfo::CalculateLinkQuality(aLinkMargin, LinkQualityInfo::kNoLinkQuality);
}

int8_t GetTypicalRssForLinkQuality(int8_t aNoiseFloor, LinkQuality aLinkQuality)
{
    int8_t linkMargin = 0;

    switch (aLinkQuality)
    {
    case kLinkQuality3:
        linkMargin = LinkQualityInfo::kLinkQuality3LinkMargin;
        break;

    case kLinkQuality2:
        linkMargin = LinkQualityInfo::kLinkQuality2LinkMargin;
        break;

    case kLinkQuality1:
        linkMargin = LinkQualityInfo::kLinkQuality1LinkMargin;
        break;

    default:
        linkMargin = LinkQualityInfo::kLinkQuality0LinkMargin;
        break;
    }

    return linkMargin + aNoiseFloor;
}

uint8_t CostForLinkQuality(LinkQuality aLinkQuality)
{
    static const uint8_t kCostsForLinkQuality[] = {
        kCostForLinkQuality0, // Link cost for `kLinkQuality0` (0).
        kCostForLinkQuality1, // Link cost for `kLinkQuality1` (1).
        kCostForLinkQuality2, // Link cost for `kLinkQuality2` (2).
        kCostForLinkQuality3, // Link cost for `kLinkQuality3` (3).
    };

    static_assert(kLinkQuality0 == 0, "kLinkQuality0 is invalid");
    static_assert(kLinkQuality1 == 1, "kLinkQuality1 is invalid");
    static_assert(kLinkQuality2 == 2, "kLinkQuality2 is invalid");
    static_assert(kLinkQuality3 == 3, "kLinkQuality3 is invalid");

    uint8_t cost = Mle::kMaxRouteCost;

    VerifyOrExit(aLinkQuality <= kLinkQuality3);
    cost = kCostsForLinkQuality[aLinkQuality];

exit:
    return cost;
}

LinkQuality LinkQualityInfo::CalculateLinkQuality(uint8_t aLinkMargin, uint8_t aLastLinkQuality)
{
    // Static private method to calculate the link quality from a given
    // link margin while taking into account the last link quality
    // value and adding the hysteresis value to the thresholds. If
    // there is no previous value for link quality, the constant
    // kNoLinkQuality should be passed as the second argument.

    uint8_t     threshold1, threshold2, threshold3;
    LinkQuality linkQuality = kLinkQuality0;

    threshold1 = kThreshold1;
    threshold2 = kThreshold2;
    threshold3 = kThreshold3;

    // Apply the hysteresis threshold based on the last link quality value.

    switch (aLastLinkQuality)
    {
    case 0:
        threshold1 += kHysteresisThreshold;

        OT_FALL_THROUGH;

    case 1:
        threshold2 += kHysteresisThreshold;

        OT_FALL_THROUGH;

    case 2:
        threshold3 += kHysteresisThreshold;

        OT_FALL_THROUGH;

    default:
        break;
    }

    if (aLinkMargin > threshold3)
    {
        linkQuality = kLinkQuality3;
    }
    else if (aLinkMargin > threshold2)
    {
        linkQuality = kLinkQuality2;
    }
    else if (aLinkMargin > threshold1)
    {
        linkQuality = kLinkQuality1;
    }

    return linkQuality;
}

} // namespace ot
