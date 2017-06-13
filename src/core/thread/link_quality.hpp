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
 *   This file includes definitions for storing and processing link quality information.
 */

#ifndef LINK_QUALITY_HPP_
#define LINK_QUALITY_HPP_

#include <stdio.h>

#include <openthread/platform/radio.h>
#include <openthread/types.h>

namespace ot {

/**
 * @addtogroup core-link-quality
 *
 * @brief
 *   This module includes definitions for Thread link quality metrics.
 *
 * @{
 */

/**
 * This class encapsulates/stores all relevant information about quality of a link, including average received signal
 * strength (RSS), link margin and link quality value (value in 0-3). The average is obtained using an adaptive
 * exponential moving average filter.
 *
 */
class LinkQualityInfo
{
public:
    /**
     * This constructor initializes an instance of the LinkQualityInfo class.
     *
     */
    LinkQualityInfo(void);

    /**
     * This method clears the all the data in this instance.
     *
     */
    void Clear(void);

    /**
     * This method adds a new received signal strength (RSS) value to the average.
     *
     * @param[in] aNoiseFloor  The noise floor value (in dBm).
     * @param[in] aRss         A new received signal strength value (in dBm) to be added to the average.
     *
     */
    void AddRss(int8_t aNoiseFloor, int8_t aRss);

    /**
     * This method returns the current average signal strength value.
     *
     * @returns The current average value or @c OT_RADIO_RSSI_INVALID if no average is available.
     *
     */
    int8_t GetAverageRss(void) const;

    /**
     * This method returns an encoded version of current average signal strength value. The encoded value is the
     * average multiplied by a precision factor (currently -8).
     *
     * @returns The current average multiplied by precision factor or zero if no average is available.
     *
     */
    uint16_t GetAverageRssAsEncodedWord(void) const;

    /**
     * This method provides the current average received signal strength (RSS) as a human-readable string (e.g.,
     * "-80.375 dBm"). If the average is unknown, "unknown RSS" is used/returned in the buffer.
     *
     * @param[out]    aCharBuffer    A char buffer to store the string corresponding to current average value.
     * @param[in]     aBufferLen     The char buffer length.
     *
     * @retval OT_ERROR_NONE      Successfully formed the string in the given char buffer.
     * @retval OT_ERROR_NO_BUFS   The string representation of the average value could not fit in the given buffer.
     *
     */
    otError GetAverageRssAsString(char *aCharBuffer, size_t aBufferLen) const;

    /**
     * This method returns the link margin. The link margin is calculated using the link's current average received
     * signal strength (RSS) and average noise floor.
     *
     * @param[in]  aNoiseFloor  The noise floor value (in dBm).
     *
     * @returns Link margin derived from average received signal strength and average noise floor.
     *
     */
    uint8_t GetLinkMargin(int8_t aNoiseFloor) const;

    /**
     * Returns the current one-way link quality value. The link quality value is a number 0-3.
     *
     * The link quality is calculated by comparing the current link margin with a set of thresholds (per Thread spec).
     * More specifically, link margin > 20 dB gives link quality 3, link margin > 10 dB gives link quality 2,
     * link margin > 2 dB gives link quality 1, and link margin below or equal to 2 dB yields link quality of 0.
     *
     * In order to ensure that a link margin near the boundary of two different link quality values does not cause
     * frequent changes, a hysteresis of 2 dB is applied when determining the link quality. For example, the average
     * link margin must be at least 12 dB to change a quality 1 link to a quality 2 link.
     *
     * @param[in]  aNoiseFloor  The noise floor value (in dBm).
     *
     * @returns The current link quality value (value 0-3 as per Thread specification).
     */
    uint8_t GetLinkQuality(int8_t aNoiseFloor);

    /**
     * Returns the most recent RSS value.
     *
     * @returns The most recent RSS
     *
     */
    int8_t GetLastRss(void) const;

    /**
     * This method converts a received signal strength value to a link margin value.
     *
     * @param[in]  aNoiseFloor  The noise floor value (in dBm).
     * @param[in]  aRss         The received signal strength value (in dBm).
     *
     * @returns The link margin value.
     *
     */
    static uint8_t ConvertRssToLinkMargin(int8_t aNoiseFloor, int8_t aRss);

    /**
     * This method converts a link margin value to a link quality value.
     *
     * @param[in]  aLinkMargin  The Link Margin in dB.
     *
     * @returns The link quality value (0-3).
     *
     */
    static uint8_t ConvertLinkMarginToLinkQuality(uint8_t aLinkMargin);

    /**
     * This method converts a received signal strength value to a link quality value.
     *
     * @param[in]  aNoiseFloor  The noise floor value (in dBm).
     * @param[in]  aRss         The received signal strength value (in dBm).
     *
     * @returns The link quality value (0-3).
     *
     */
    static uint8_t ConvertRssToLinkQuality(int8_t aNoiseFloor, int8_t aRss);

private:
    enum
    {
        // Constants for obtaining link quality from link margin:

        kLinkMarginThresholdForLinkQuality3    = 20,   // Link margin threshold for quality 3 link.
        kLinkMarginThresholdForLinkQuality2    = 10,   // Link margin threshold for quality 2 link.
        kLinkMarginThresholdForLinkQuality1    = 2,    // Link margin threshold for quality 1 link.
        kLinkMarginHysteresisThreshold         = 2,    // Link margin hysteresis threshold.

        kNoLastLinkQualityValue                = 0xff, // Used to indicate that there is no previous/last link quality.

        // Constants related to RSS adaptive exponential moving average filter:

        kRssAveragePrecisionMultipleBitShift   = 3, // Precision multiple for RSS average (1 << PrecisionBitShift).
        kRssAveragePrecisionMultiple           = (1 << kRssAveragePrecisionMultipleBitShift),
        kRssAveragePrecisionMultipleBitMask    = (kRssAveragePrecisionMultiple - 1),

        kRssCountMax                           = 7, // mCount max limit value.

        kRssCountForWeightCoefficientOneEighth = 5, // mCount threshold to use average weight coefficient of 1/8.
        kRssCountForWeightCoefficientOneFourth = 2, // mCount threshold to use average weight coefficient of 1/4.
        kRssCountForWeightCoefficientOneHalf   = 1, // mCount threshold to use average weight coefficient of 1/2.
    };

    /* Private method to update the mLinkQuality value. This is called when a new RSS value is added to average
     * or when GetLinkQuality() is invoked.
     */
    void UpdateLinkQuality(int8_t aNoiseFloor);

    /* Static private method to calculate the link quality from a given link margin while taking into account the last
     * link quality value and adding the hysteresis value to the thresholds. If there is no previous value for link
     * quality, the constant kNoLastLinkQualityValue should be passed as the second argument.
     *
     */
    static uint8_t CalculateLinkQuality(uint8_t aLinkMargin, uint8_t aLastLinkQuality);

    static const char kUnknownRssString[]; // Constant string used when RSS average is unknown.

    // All data should fit into a 16-bit (uint16_t) value.

    uint16_t mRssAverage  : 11; // The encoded average signal strength value (stored as rss times precision multiple).
    uint8_t  mCount       : 3;  // Number of RSS values added to average so far (limited to kRssCountMax).
    uint8_t  mLinkQuality : 2;  // Current link quality value (0-3).
    int8_t   mLastRss;
};

/**
 * @}
 */

} // namespace ot

#endif  // LINK_QUALITY_HPP_
