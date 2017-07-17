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
 * This class implements a Received Signal Strength (RSS) averager.
 *
 * The average is maintained using an adaptive exponentially weighted moving filter.
 *
 */
class RssAverager
{
    friend class LinkQualityInfo;

public:
    enum
    {
        kStringSize = 10,    ///< Max chars needed for a string representation of average (@sa ToString()).
    };

    /**
     * This method reset the averager and clears the average value.
     *
     */
    void Reset(void);

    /**
     * This method indicates whether the averager contains an average (i.e., at least one RSS value has been added).
     *
     * @retval true   If the average value is available (at least one RSS value has been added).
     * @retval false  Averager is empty (no RSS value added yet).
     *
     */
    bool HasAverage(void) const { return (mCount != 0); }

    /**
     * This method adds a received signal strength (RSS) value to the average.
     *
     * If @p aRss is OT_RADIO_RSSI_INVALID, it is ignored and error status OT_ERROR_INVALID_ARGS is returned.
     * The value of RSS is capped at 0dBm (i.e., for any given RSS value higher than 0dBm, 0dBm is used instead).
     *
     * @param[in] aRss                Received signal strength value (in dBm) to be added to the average.
     *
     * @retval OT_ERROR_NONE          New RSS value added to average successfully.
     * @retval OT_ERROR_INVALID_ARGS  Value of @p aRss is OT_RADIO_RSSI_INVALID.
     *
     */
    otError Add(int8_t aRss);

    /**
     * This method returns the current average signal strength value maintained by the averager.
     *
     * @returns The current average value (in dBm) or OT_RADIO_RSSI_INVALID if no average is available.
     *
     */
    int8_t GetAverage(void) const;

    /**
     * This method returns an raw/encoded version of current average signal strength value. The raw value is the
     * average multiplied by a precision factor (currently set as -8).
     *
     * @returns The current average multiplied by precision factor or zero if no average is available.
     *
     */
    uint16_t GetRaw(void) const { return mAverage; }

    /**
     * This method converts the current average RSS value to a human-readable string (e.g., "-80.375"). If the
     * average is unknown, empty string is returned.
     *
     * @param[out]  aBuf   A pointer to the char buffer.
     * @param[in]   aSize  The maximum size of the buffer.
     *
     * @returns A pointer to the char string buffer.
     *
     */
    const char *ToString(char *aBuf, uint16_t aSize) const;

private:
    /*
     * The RssAverager uses an adaptive exponentially weighted filter to maintain the average. It keeps track of
     * current average and the number of added RSS values (up to a 8).
     *
     * For the first 8 added RSS values, the average is the arithmetic mean of the added values (e.g., if 5 values are
     * added, the average is sum of the 5 added RSS values divided by 5. After the 8th RSS value, a weighted filter is
     * used with coefficients (1/8, 7/8), i.e., newAverage = 1/8 * newRss + 7/8 * oldAverage.
     *
     * To add to accuracy of the averaging process, the RSS values and the maintained average are multiplied by a
     * precision factor of -8.
     *
     */

    enum
    {
        kPrecisionBitShift      = 3,    // Precision multiple for RSS average (1 << PrecisionBitShift).
        kPrecision              = (1 << kPrecisionBitShift),
        kPrecisionBitMask       = (kPrecision - 1),

        kCoeffBitShift          = 3,    // Coefficient used for exponentially weighted filter (1 << kCoeffBitShift).
    };

    // Member variables fit into two bytes.

    uint16_t mAverage     : 11; // The raw average signal strength value (stored as RSS times precision multiple).
    uint8_t  mCount       : 3;  // Number of RSS values added to averager so far (limited to 2^kCoeffBitShift-1).
    uint8_t  mLinkQuality : 2;  // Used by friend class LinkQualityInfo to store LinkQuality (0-3) value.
};

/**
 * This class encapsulates/stores all relevant information about quality of a link, including average received signal
 * strength (RSS), last RSS, link margin, and link quality.
 *
 */
class LinkQualityInfo
{

public:
    enum
    {
        kInfoStringSize = 50,    ///< Max chars needed for the info string representation (@sa ToInfoString())
    };

    /**
     * This constructor initializes the object.
     *
     */
    LinkQualityInfo(void);

    /**
     * This method clears the all the data in the object.
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
    int8_t GetAverageRss(void) const { return mRssAverager.GetAverage(); }

    /**
     * This method returns an encoded version of current average signal strength value. The encoded value is the
     * average multiplied by a precision factor (currently -8).
     *
     * @returns The current average multiplied by precision factor or zero if no average is available.
     *
     */
    uint16_t GetAverageRssRaw(void) const { return mRssAverager.GetRaw(); }

    /**
     * This method converts the link quality info  to NULL-terminated info/debug human-readable string.
     *
     * @param[out]  aBuf   A pointer to the string buffer.
     * @param[in]   aSize  The maximum size of the string buffer.
     *
     * @returns A pointer to the char string buffer.
     *
     */
    const char *ToInfoString(char *aBuf, uint16_t aSize) const;

    /**
     * This method returns the link margin. The link margin is calculated using the link's current average received
     * signal strength (RSS) and average noise floor.
     *
     * @param[in]  aNoiseFloor  The noise floor value (in dBm).
     *
     * @returns Link margin derived from average received signal strength and average noise floor.
     *
     */
    uint8_t GetLinkMargin(int8_t aNoiseFloor) const { return ConvertRssToLinkMargin(aNoiseFloor, GetAverageRss()); }

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
     *
     */
    uint8_t GetLinkQuality(void) const { return mRssAverager.mLinkQuality; }

    /**
     * Returns the most recent RSS value.
     *
     * @returns The most recent RSS
     *
     */
    int8_t GetLastRss(void) const { return mLastRss; }

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

    /**
     * This method converts a link quality value to a typical received signal strength value .
     * @note only for test
     *
     * @param[in]  aNoiseFloor   The noise floor value (in dBm).
     * @param[in]  aLinkQuality  The link quality value in [0, 3].
     *
     * @returns The typical platform rssi.
     *
     */
    static int8_t ConvertLinkQualityToRss(int8_t aNoiseFloor, uint8_t aLinkQuality);

private:
    enum
    {
        // Constants for obtaining link quality from link margin:

        kThreshold3             = 20,   // Link margin threshold for quality 3 link.
        kThreshold2             = 10,   // Link margin threshold for quality 2 link.
        kThreshold1             = 2,    // Link margin threshold for quality 1 link.
        kHysteresisThreshold    = 2,    // Link margin hysteresis threshold.

        // constants for test:

        kLinkQuality3LinkMargin     = 50, ///< link margin for Link Quality 3 (21 - 255)
        kLinkQuality2LinkMargin     = 15, ///< link margin for Link Quality 3 (21 - 255)
        kLinkQuality1LinkMargin     = 5,  ///< link margin for Link Quality 3 (21 - 255)
        kLinkQuality0LinkMargin     = 0,  ///< link margin for Link Quality 3 (21 - 255)

        kNoLinkQuality          = 0xff, // Used to indicate that there is no previous/last link quality.
    };

    void SetLinkQuality(uint8_t aLinkQuality) { mRssAverager.mLinkQuality = aLinkQuality; }

    /* Static private method to calculate the link quality from a given link margin while taking into account the last
     * link quality value and adding the hysteresis value to the thresholds. If there is no previous value for link
     * quality, the constant kNoLinkQuality should be passed as the second argument.
     *
     */
    static uint8_t CalculateLinkQuality(uint8_t aLinkMargin, uint8_t aLastLinkQuality);

    RssAverager mRssAverager;
    int8_t      mLastRss;
};

/**
 * @}
 */

}  // namespace ot

#endif // LINK_QUALITY_HPP_
