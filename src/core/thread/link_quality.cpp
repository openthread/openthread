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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>
#include "utils/wrap_string.h"

#include "openthread/types.h"

#include <common/code_utils.hpp>
#include <thread/link_quality.hpp>

namespace ot {

enum
{
    kDefaultNoiseFloor = -100, // Default noise floor used if no average value is available.
};

// This array gives the decimal point digits representing 0/8, 1/8, ..., 7/8 (it does not include the '.').
static const char *const kLinkQualityDecimalDigitsString[8] =
{
    // 0/8, 1/8,   2/8,   3/8,   4/8,   5/8,   6/8,   7/8
    "000", "125", "250", "375", "500", "625", "750", "875"
};

const char LinkQualityInfo::kUnknownRssString[] = "Unknown RSS";

//-------------------------------------------------------------------------------

LinkQualityInfo::LinkQualityInfo(void)
{
    Clear();
}

void LinkQualityInfo::Clear(void)
{
    mRssAverage  = 0;
    mCount       = 0;
    mLinkQuality = 0;
    mLastRss     = 0;
}

void LinkQualityInfo::AddRss(int8_t aNoiseFloor, int8_t aRss)
{
    uint16_t    newValue;
    uint16_t    oldAverage;

    VerifyOrExit(aRss != kUnknownRss);

    mLastRss = aRss;

    // Restrict/Cap the RSS value to the closed range [0, -128] so the value can fit in 8 bits.

    if (aRss > 0)
    {
        aRss = 0;
    }

    // Multiply the the RSS value by a precision multiple (currently -8).

    newValue = static_cast<uint16_t>(-aRss);
    newValue <<= kRssAveragePrecisionMultipleBitShift;

    oldAverage = mRssAverage;

    if (mCount >= kRssCountForWeightCoefficientOneEighth)
    {
        // New average = old average * 7/8 + new value * 1/8
        mRssAverage = static_cast<uint16_t>(((oldAverage << 3) - oldAverage + newValue) >> 3);
    }
    else if (mCount >= kRssCountForWeightCoefficientOneFourth)
    {
        // New average = old average * 3/4 + new value * 1/4
        mRssAverage = static_cast<uint16_t>(((oldAverage << 2) - oldAverage + newValue) >> 2);
    }
    else if (mCount >= kRssCountForWeightCoefficientOneHalf)
    {
        // New average = old average * 1/2 + new value * 1/2
        mRssAverage = (oldAverage + newValue) >> 1;
    }
    else
    {
        mRssAverage = newValue;
    }

    if (mCount < kRssCountMax)
    {
        mCount++;
    }

    UpdateLinkQuality(aNoiseFloor);

exit:
    return;
}

int8_t LinkQualityInfo::GetAverageRss(void) const
{
    int8_t average = kUnknownRss;

    if (mCount != 0)
    {
        average = -static_cast<int8_t>(mRssAverage >> kRssAveragePrecisionMultipleBitShift);

        // Check for round up (e.g. average of -71.5 --> -72)

        if ((mRssAverage & kRssAveragePrecisionMultipleBitMask) >= (kRssAveragePrecisionMultiple >> 1))
        {
            average--;
        }
    }

    return average;
}

uint16_t LinkQualityInfo::GetAverageRssAsEncodedWord(void) const
{
    return mRssAverage;
}

ThreadError LinkQualityInfo::GetAverageRssAsString(char *aCharBuffer, size_t aBufferLen) const
{
    ThreadError error = kThreadError_None;
    int charsWritten = 0;

    if (mCount == 0)
    {
        charsWritten = static_cast<int>(strlcpy(aCharBuffer, kUnknownRssString, aBufferLen));
    }
    else
    {
        charsWritten = snprintf(aCharBuffer, aBufferLen, "%d.%s dBm",
                                -(mRssAverage >> kRssAveragePrecisionMultipleBitShift),
                                kLinkQualityDecimalDigitsString[mRssAverage & kRssAveragePrecisionMultipleBitMask]);
    }

    VerifyOrExit(charsWritten >= 0, error = kThreadError_NoBufs);

    VerifyOrExit(charsWritten < static_cast<int>(aBufferLen), error = kThreadError_NoBufs);

exit:
    return error;
}

uint8_t LinkQualityInfo::GetLinkMargin(int8_t aNoiseFloor) const
{
    return ConvertRssToLinkMargin(aNoiseFloor, GetAverageRss());
}

uint8_t LinkQualityInfo::GetLinkQuality(int8_t aNoiseFloor)
{
    UpdateLinkQuality(aNoiseFloor);

    return mLinkQuality;
}

int8_t LinkQualityInfo::GetLastRss(void) const
{
    return mLastRss;
}

void LinkQualityInfo::UpdateLinkQuality(int8_t aNoiseFloor)
{
    if (mCount != 0)
    {
        mLinkQuality = CalculateLinkQuality(GetLinkMargin(aNoiseFloor), mLinkQuality);
    }
    else
    {
        mLinkQuality = CalculateLinkQuality(GetLinkMargin(aNoiseFloor), kNoLastLinkQualityValue);
    }
}

uint8_t LinkQualityInfo::ConvertRssToLinkMargin(int8_t aNoiseFloor, int8_t aRss)
{
    int8_t linkMargin = aRss - aNoiseFloor;

    if (linkMargin < 0 || aRss == kUnknownRss)
    {
        linkMargin = 0;
    }

    return static_cast<uint8_t>(linkMargin);
}

uint8_t LinkQualityInfo::ConvertLinkMarginToLinkQuality(uint8_t aLinkMargin)
{
    return CalculateLinkQuality(aLinkMargin, kNoLastLinkQualityValue);
}

uint8_t LinkQualityInfo::ConvertRssToLinkQuality(int8_t aNoiseFloor, int8_t aRss)
{
    return ConvertLinkMarginToLinkQuality(ConvertRssToLinkMargin(aNoiseFloor, aRss));
}

uint8_t LinkQualityInfo::CalculateLinkQuality(uint8_t aLinkMargin, uint8_t aLastLinkQuality)
{
    uint8_t threshold1, threshold2, threshold3;
    uint8_t linkQuality = 0;

    threshold1 = kLinkMarginThresholdForLinkQuality1;
    threshold2 = kLinkMarginThresholdForLinkQuality2;
    threshold3 = kLinkMarginThresholdForLinkQuality3;

    // Apply the hysteresis threshold based on the last link quality value.

    switch (aLastLinkQuality)
    {
    case 0:
        threshold1 += kLinkMarginHysteresisThreshold;

    // Intentional fall-through to next case.

    case 1:
        threshold2 += kLinkMarginHysteresisThreshold;

    // Intentional fall-through to next case.

    case 2:
        threshold3 += kLinkMarginHysteresisThreshold;

    // Intentional fall-through to next case.

    default:
        break;
    }

    if (aLinkMargin > threshold3)
    {
        linkQuality = 3;
    }
    else if (aLinkMargin > threshold2)
    {
        linkQuality = 2;
    }
    else if (aLinkMargin > threshold1)
    {
        linkQuality = 1;
    }

    return linkQuality;
}

}  // namespace ot
