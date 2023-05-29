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

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/string.hpp"
#include "thread/mle_types.hpp"

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
 * Implements an operation Success Rate Tracker.
 *
 * This can be used to track different link quality related metrics, e.g., CCA failure rate, frame tx success rate).
 * The success rate is maintained using an exponential moving IIR averaging filter with a `uint16_t` as the storage.
 *
 */
class SuccessRateTracker : public Clearable<SuccessRateTracker>
{
public:
    static constexpr uint16_t kMaxRateValue = 0xffff; ///< Value corresponding to max (failure/success) rate of 100%.

    /**
     * Adds a sample (success or failure) to `SuccessRateTracker`.
     *
     * @param[in] aSuccess   The sample status be added, `true` for success, `false` for failure.
     * @param[in] aWeight    The weight coefficient used for adding the new sample into average.
     *
     */
    void AddSample(bool aSuccess, uint16_t aWeight = kDefaultWeight);

    /**
     * Returns the average failure rate.
     *
     * @retval the average failure rate `[0-kMaxRateValue]` with `kMaxRateValue` corresponding to 100%.
     *
     */
    uint16_t GetFailureRate(void) const { return mFailureRate; }

    /**
     * Returns the average success rate.
     *
     * @retval the average success rate as [0-kMaxRateValue] with `kMaxRateValue` corresponding to 100%.
     *
     */
    uint16_t GetSuccessRate(void) const { return kMaxRateValue - mFailureRate; }

private:
    static constexpr uint16_t kDefaultWeight = 64;

    uint16_t mFailureRate;
};

/**
 * Implements a Received Signal Strength (RSS) averager.
 *
 * The average is maintained using an adaptive exponentially weighted moving filter.
 *
 */
class RssAverager : public Clearable<RssAverager>
{
public:
    static constexpr uint16_t kStringSize = 10; ///< Max string size for average (@sa ToString()).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kStringSize> InfoString;

    /**
     * Indicates whether the averager contains an average (i.e., at least one RSS value has been added).
     *
     * @retval true   If the average value is available (at least one RSS value has been added).
     * @retval false  Averager is empty (no RSS value added yet).
     *
     */
    bool HasAverage(void) const { return (mCount != 0); }

    /**
     * Adds a received signal strength (RSS) value to the average.
     *
     * If @p aRss is `Radio::kInvalidRssi`, it is ignored and error status kErrorInvalidArgs is returned.
     * The value of RSS is capped at 0dBm (i.e., for any given RSS value higher than 0dBm, 0dBm is used instead).
     *
     * @param[in] aRss                Received signal strength value (in dBm) to be added to the average.
     *
     * @retval kErrorNone         New RSS value added to average successfully.
     * @retval kErrorInvalidArgs  Value of @p aRss is `Radio::kInvalidRssi`.
     *
     */
    Error Add(int8_t aRss);

    /**
     * Returns the current average signal strength value maintained by the averager.
     *
     * @returns The current average value (in dBm) or `Radio::kInvalidRssi` if no average is available.
     *
     */
    int8_t GetAverage(void) const;

    /**
     * Returns an raw/encoded version of current average signal strength value. The raw value is the
     * average multiplied by a precision factor (currently set as -8).
     *
     * @returns The current average multiplied by precision factor or zero if no average is available.
     *
     */
    uint16_t GetRaw(void) const { return mAverage; }

    /**
     * Converts the current average RSS value to a human-readable string (e.g., "-80.375"). If the
     * average is unknown, empty string is returned.
     *
     * @returns An `InfoString` object containing the string representation of average RSS.
     *
     */
    InfoString ToString(void) const;

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
    static constexpr uint8_t kPrecisionBitShift = 3; // Precision multiple for RSS average (1 << PrecisionBitShift).
    static constexpr uint8_t kPrecision         = (1 << kPrecisionBitShift);
    static constexpr uint8_t kPrecisionBitMask  = (kPrecision - 1);
    static constexpr uint8_t kCoeffBitShift     = 3; // Coeff for exp weighted filter (1 << kCoeffBitShift).

    // Member variables fit into two bytes.

    uint16_t mAverage : 11; // The raw average signal strength value (stored as RSS times precision multiple).
    uint16_t mCount : 5;    // Number of RSS values added to averager so far (limited to 2^kCoeffBitShift-1).
};

/**
 * Implements a Link Quality Indicator (LQI) averager.
 *
 * It maintains the exponential moving average value of LQI.
 *
 */
class LqiAverager : public Clearable<LqiAverager>
{
public:
    /**
     * Adds a link quality indicator (LQI) value to the average.
     *
     * @param[in] aLqi  Link Quality Indicator value to be added to the average.
     *
     */
    void Add(uint8_t aLqi);

    /**
     * Returns the current average link quality value maintained by the averager.
     *
     * @returns The current average value.
     *
     */
    uint8_t GetAverage(void) const { return mAverage; }

    /**
     * Returns the count of frames calculated so far.
     *
     * @returns The count of frames calculated.
     *
     */
    uint8_t GetCount(void) const { return mCount; }

private:
    static constexpr uint8_t kCoeffBitShift = 3; // Coeff used for exp weighted filter (1 << kCoeffBitShift).

    uint8_t mAverage; // The average link quality indicator value.
    uint8_t mCount;   // Number of LQI values added to averager so far.
};

/**
 * Represents the link quality constants.
 *
 * Link Quality is an integer in [0, 3]. A higher link quality indicates a more usable link, with 0 indicating that the
 * link is non-existent or unusable.
 *
 */
enum LinkQuality : uint8_t
{
    kLinkQuality0 = 0, ///< Link quality 0 (non-existent link)
    kLinkQuality1 = 1, ///< Link quality 1
    kLinkQuality2 = 2, ///< Link quality 2
    kLinkQuality3 = 3, ///< Link quality 3
};

constexpr uint8_t kCostForLinkQuality0 = Mle::kMaxRouteCost; ///< Link Cost for Link Quality 0.
constexpr uint8_t kCostForLinkQuality1 = 4;                  ///< Link Cost for Link Quality 1.
constexpr uint8_t kCostForLinkQuality2 = 2;                  ///< Link Cost for Link Quality 2.
constexpr uint8_t kCostForLinkQuality3 = 1;                  ///< Link Cost for Link Quality 3.

/**
 * Converts link quality to route cost.
 *
 * @param[in]  aLinkQuality  The link quality to convert.
 *
 * @returns The route cost corresponding to @p aLinkQuality.
 *
 */
uint8_t CostForLinkQuality(LinkQuality aLinkQuality);

/**
 * Computes the link margin from a given noise floor and received signal strength.
 *
 * @param[in]  aNoiseFloor  The noise floor value (in dBm).
 * @param[in]  aRss         The received signal strength value (in dBm).
 *
 * @returns The link margin value in dB.
 *
 */
uint8_t ComputeLinkMargin(int8_t aNoiseFloor, int8_t aRss);

/**
 * Converts a link margin value to a link quality value.
 *
 * @param[in]  aLinkMargin  The Link Margin in dB.
 *
 * @returns The link quality value (0-3).
 *
 */
LinkQuality LinkQualityForLinkMargin(uint8_t aLinkMargin);

/**
 * Gets the typical received signal strength value for a given link quality.
 *
 * @param[in]  aNoiseFloor   The noise floor value (in dBm).
 * @param[in]  aLinkQuality  The link quality value in [0, 3].
 *
 * @returns The typical platform RSSI in dBm.
 *
 */
int8_t GetTypicalRssForLinkQuality(int8_t aNoiseFloor, LinkQuality aLinkQuality);

/**
 * Encapsulates/stores all relevant information about quality of a link, including average received signal
 * strength (RSS), last RSS, link margin, and link quality.
 *
 */
class LinkQualityInfo : public InstanceLocatorInit
{
    friend LinkQuality LinkQualityForLinkMargin(uint8_t aLinkMargin);
    friend int8_t      GetTypicalRssForLinkQuality(int8_t aNoiseFloor, LinkQuality aLinkQuality);

public:
    static constexpr uint16_t kInfoStringSize = 50; ///< `InfoString` size (@sa ToInfoString()).

    /**
     * Defines the fixed-length `String` object returned from `ToInfoString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Initializes the `LinkQualityInfo` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { InstanceLocatorInit::Init(aInstance); }

    /**
     * Clears the all the data in the object.
     *
     */
    void Clear(void);

    /**
     * Clears the average RSS value.
     *
     */
    void ClearAverageRss(void) { mRssAverager.Clear(); }

    /**
     * Adds a new received signal strength (RSS) value to the average.
     *
     * @param[in] aRss         A new received signal strength value (in dBm) to be added to the average.
     *
     */
    void AddRss(int8_t aRss);

    /**
     * Returns the current average received signal strength value.
     *
     * @returns The current average value or `Radio::kInvalidRssi` if no average is available.
     *
     */
    int8_t GetAverageRss(void) const { return mRssAverager.GetAverage(); }

    /**
     * Returns an encoded version of current average signal strength value. The encoded value is the
     * average multiplied by a precision factor (currently -8).
     *
     * @returns The current average multiplied by precision factor or zero if no average is available.
     *
     */
    uint16_t GetAverageRssRaw(void) const { return mRssAverager.GetRaw(); }

    /**
     * Converts the link quality info to info/debug human-readable string.
     *
     * @returns An `InfoString` representing the link quality info.
     *
     */
    InfoString ToInfoString(void) const;

    /**
     * Returns the link margin. The link margin is calculated using the link's current average received
     * signal strength (RSS) and average noise floor.
     *
     * @returns Link margin derived from average received signal strength and average noise floor.
     *
     */
    uint8_t GetLinkMargin(void) const;

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
     * @returns The current link quality value (value 0-3 as per Thread specification).
     *
     */
    LinkQuality GetLinkQuality(void) const { return mLinkQuality; }

    /**
     * Returns the most recent RSS value.
     *
     * @returns The most recent RSS
     *
     */
    int8_t GetLastRss(void) const { return mLastRss; }

    /**
     * Adds a MAC frame transmission status (success/failure) and updates the frame tx error rate.
     *
     * @param[in]  aTxStatus   Success/Failure of MAC frame transmission (`true` -> success, `false` -> failure).
     *
     */
    void AddFrameTxStatus(bool aTxStatus)
    {
        mFrameErrorRate.AddSample(aTxStatus, OPENTHREAD_CONFIG_FRAME_TX_ERR_RATE_AVERAGING_WINDOW);
    }

    /**
     * Adds a message transmission status (success/failure) and updates the message error rate.
     *
     * @param[in]  aTxStatus   Success/Failure of message (`true` -> success, `false` -> message tx failed).
     *                         A larger (IPv6) message may be fragmented and sent as multiple MAC frames. The message
     *                         transmission is considered a failure, if any of its fragments fail after all MAC retry
     *                         attempts.
     *
     */
    void AddMessageTxStatus(bool aTxStatus)
    {
        mMessageErrorRate.AddSample(aTxStatus, OPENTHREAD_CONFIG_IPV6_TX_ERR_RATE_AVERAGING_WINDOW);
    }

    /**
     * Returns the MAC frame transmission error rate for the link.
     *
     * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_FRAME_TX_ERR_RATE_AVERAGING_WINDOW`
     * frame transmissions.
     *
     * @returns The error rate with maximum value `0xffff` corresponding to 100% failure rate.
     *
     */
    uint16_t GetFrameErrorRate(void) const { return mFrameErrorRate.GetFailureRate(); }

    /**
     * Returns the message error rate for the link.
     *
     * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_IPV6_TX_ERR_RATE_AVERAGING_WINDOW`
     * (IPv6) messages.
     *
     * Note that a larger (IPv6) message can be fragmented and sent as multiple MAC frames. The message transmission is
     * considered a failure, if any of its fragments fail after all MAC retry attempts.
     *
     * @returns The error rate with maximum value `0xffff` corresponding to 100% failure rate.
     *
     */
    uint16_t GetMessageErrorRate(void) const { return mMessageErrorRate.GetFailureRate(); }

private:
    // Constants for obtaining link quality from link margin:

    static constexpr uint8_t kThreshold3          = 20; // Link margin threshold for quality 3 link.
    static constexpr uint8_t kThreshold2          = 10; // Link margin threshold for quality 2 link.
    static constexpr uint8_t kThreshold1          = 2;  // Link margin threshold for quality 1 link.
    static constexpr uint8_t kHysteresisThreshold = 2;  // Link margin hysteresis threshold.

    static constexpr int8_t kLinkQuality3LinkMargin = 50; // link margin for Link Quality 3 (21 - 255)
    static constexpr int8_t kLinkQuality2LinkMargin = 15; // link margin for Link Quality 3 (21 - 255)
    static constexpr int8_t kLinkQuality1LinkMargin = 5;  // link margin for Link Quality 3 (21 - 255)
    static constexpr int8_t kLinkQuality0LinkMargin = 0;  // link margin for Link Quality 3 (21 - 255)

    static constexpr uint8_t kNoLinkQuality = 0xff; // Indicate that there is no previous/last link quality.

    void SetLinkQuality(LinkQuality aLinkQuality) { mLinkQuality = aLinkQuality; }

    static LinkQuality CalculateLinkQuality(uint8_t aLinkMargin, uint8_t aLastLinkQuality);

    RssAverager mRssAverager;
    LinkQuality mLinkQuality;
    int8_t      mLastRss;

    SuccessRateTracker mFrameErrorRate;
    SuccessRateTracker mMessageErrorRate;
};

/**
 * @}
 */

} // namespace ot

#endif // LINK_QUALITY_HPP_
