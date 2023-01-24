/*
 *  Copyright (c) 2020-22, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing Link Metrics TLVs.
 *
 */

#ifndef LINK_METRICS_TYPES_HPP_
#define LINK_METRICS_TYPES_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#include <openthread/link_metrics.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"

namespace ot {
namespace LinkMetrics {

constexpr uint8_t kMaxTypeIds = 4; ///< Maximum number of Type IDs in a `Metrics`.

/**
 * This type represents Link Metric Flags indicating a set of metrics.
 *
 * @sa otLinkMetrics
 *
 */
class Metrics : public otLinkMetrics, public Clearable<Metrics>
{
public:
    /**
     * This method converts the `Metrics` into an array of Type IDs.
     *
     * @param[out] aTypeIds   The array of Type IDs to populate. MUST have at least `kMaxTypeIds` elements.
     *
     * @returns Number of entries added in the array @p aTypeIds.
     *
     */
    uint8_t ConvertToTypeIds(uint8_t aTypeIds[]) const;
};

/**
 * This type represents the results (values) for a set of metrics.
 *
 * @sa otLinkMetricsValues
 *
 */
class MetricsValues : public otLinkMetricsValues, public Clearable<MetricsValues>
{
public:
    /**
     * This method gets the metrics flags.
     *
     * @returns The metrics flags.
     *
     */
    Metrics &GetMetrics(void) { return static_cast<Metrics &>(mMetrics); }

    /**
     * This method gets the metrics flags.
     *
     * @returns The metrics flags.
     *
     */
    const Metrics &GetMetrics(void) const { return static_cast<const Metrics &>(mMetrics); }

    /**
     * This method set the metrics flags.
     *
     * @param[in] aMetrics  The metrics flags to set from.
     *
     */
    void SetMetrics(const Metrics &aMetrics) { mMetrics = aMetrics; }
};

class TypeId
{
    // Type ID Flags
    //
    //   7   6   5   4   3   2   1   0
    // +---+---+---+---+---+---+---+---+
    // | E | L |   Type    |   Metric  |
    // +---+---+---+---+---+---+---+---+
    //

    static constexpr uint8_t kExtendedFlag = 1 << 7;
    static constexpr uint8_t kLengthFlag   = 1 << 6;
    static constexpr uint8_t kTypeOffset   = 3;
    static constexpr uint8_t kMetricOffset = 0;
    static constexpr uint8_t kTypeMask     = (7 << kTypeOffset);

    static constexpr uint8_t kTypeCount    = (0 << kTypeOffset); // Count/summation
    static constexpr uint8_t kTypeAve      = (1 << kTypeOffset); // Exponential Moving average
    static constexpr uint8_t kTypeReserved = (2 << kTypeOffset); // Reserved

    static constexpr uint8_t kMetricPdu        = (0 << kMetricOffset); // Number of PDUs received.
    static constexpr uint8_t kMetricLqi        = (1 << kMetricOffset);
    static constexpr uint8_t kMetricLinkMargin = (2 << kMetricOffset);
    static constexpr uint8_t kMetricRssi       = (3 << kMetricOffset);

public:
    static constexpr uint8_t kPdu        = (kMetricPdu | kTypeCount | kLengthFlag); ///< Type ID for num PDU received.
    static constexpr uint8_t kLqi        = (kMetricLqi | kTypeAve);                 ///< Type ID for LQI.
    static constexpr uint8_t kLinkMargin = (kMetricLinkMargin | kTypeAve);          ///< Type ID for Link Margin.
    static constexpr uint8_t kRssi       = (kMetricRssi | kTypeAve);                ///< Type ID for RSSI.

    /**
     * This static method indicates whether or not a given Type ID is extended.
     *
     * Extended Type IDs are reserved for future use. When set an additional second byte follows the current ID flags.
     *
     * @param[in] aTypeId   The Type ID to check.
     *
     * @retval TRUE  The @p aTypeId is extended.
     * @retval FALSE The @p aTypeId is not extended.
     *
     */
    static bool IsExtended(uint8_t aTypeId) { return (aTypeId & kExtendedFlag); }

    /**
     * This static method determines the value length (number of bytes) associated with a given Type ID.
     *
     * Type IDs can either have a short value as a `uint8_t` (e.g., `kLqi`, `kLinkMargin` or `kRssi`) or a long value as
     * a `uint32_t` (`kPdu`).
     *
     * @param[in] aTypeId   The Type ID.
     *
     * @returns the associated value length of @p aTypeId.
     *
     */
    static uint8_t GetValueLength(uint8_t aTypeId)
    {
        return (aTypeId & kLengthFlag) ? sizeof(uint32_t) : sizeof(uint8_t);
    }

    /**
     * This static method updates a Type ID to mark it as reversed.
     *
     * This is used for testing only.
     *
     * @param[in, out] aTypeId    A reference to a Type ID variable to update.
     *
     */
    static void MarkAsReserverd(uint8_t &aTypeId) { aTypeId = (aTypeId & ~kTypeMask) | kTypeReserved; }

    TypeId(void) = delete;
};

/**
 * This class represents the Series Flags for Forward Tracking Series.
 *
 */
class SeriesFlags : public otLinkMetricsSeriesFlags
{
public:
    /**
     * This method converts the `SeriesFlags` to `uint8_t` bit-mask (for inclusion in TLVs).
     *
     * @returns The bit-mask representation.
     *
     */
    uint8_t ConvertToMask(void) const;

    /**
     * This method sets the `SeriesFlags` from a given bit-mask value.
     *
     * @param[in] aFlagsMask  The bit-mask flags.
     *
     */
    void SetFrom(uint8_t aFlagsMask);

    /**
     * This method indicates whether or not the Link Probe flag is set.
     *
     * @retval true   The Link Probe flag is set.
     * @retval false  The Link Probe flag is not set.
     *
     */
    bool IsLinkProbeFlagSet(void) const { return mLinkProbe; }

    /**
     * This method indicates whether or not the MAC Data flag is set.
     *
     * @retval true   The MAC Data flag is set.
     * @retval false  The MAC Data flag is not set.
     *
     */
    bool IsMacDataFlagSet(void) const { return mMacData; }

    /**
     * This method indicates whether or not the MAC Data Request flag is set.
     *
     * @retval true   The MAC Data Request flag is set.
     * @retval false  The MAC Data Request flag is not set.
     *
     */
    bool IsMacDataRequestFlagSet(void) const { return mMacDataRequest; }

    /**
     * This method indicates whether or not the Mac Ack flag is set.
     *
     * @retval true   The Mac Ack flag is set.
     * @retval false  The Mac Ack flag is not set.
     *
     */
    bool IsMacAckFlagSet(void) const { return mMacAck; }

private:
    static constexpr uint8_t kLinkProbeFlag      = 1 << 0;
    static constexpr uint8_t kMacDataFlag        = 1 << 1;
    static constexpr uint8_t kMacDataRequestFlag = 1 << 2;
    static constexpr uint8_t kMacAckFlag         = 1 << 3;
};

/**
 * This enumeration type represent Enhanced-ACK Flags.
 *
 */
enum EnhAckFlags : uint8_t
{
    kEnhAckClear    = OT_LINK_METRICS_ENH_ACK_CLEAR,    ///< Clear.
    kEnhAckRegister = OT_LINK_METRICS_ENH_ACK_REGISTER, ///< Register.
};

/**
 * This class represents one Series that is being tracked by the Subject.
 *
 * When an Initiator successfully configured a Forward Tracking Series, the Subject would use an instance of this class
 * to track the information of the Series. The Subject has a `Pool` of `SeriesInfo`. It would allocate one when a new
 * Series comes, and free it when a Series finishes.
 *
 * This class inherits `LinkedListEntry` and each `Neighbor` has a list of `SeriesInfo` so that the Subject could track
 * per Series initiated by neighbors as long as it has available resources.
 *
 */
class SeriesInfo : public LinkedListEntry<SeriesInfo>
{
    friend class LinkedList<SeriesInfo>;
    friend class LinkedListEntry<SeriesInfo>;

public:
    /**
     * This constant represents Link Probe when filtering frames to be accounted using Series Flag. There's
     * already `Mac::Frame::kTypeData`, `Mac::Frame::kTypeAck` and `Mac::Frame::kTypeMacCmd`. This item is
     * added so that we can filter a Link Probe for series in the same way as other frames.
     *
     */
    static constexpr uint8_t kSeriesTypeLinkProbe = 0;

    /**
     * This method initializes the SeriesInfo object.
     *
     * @param[in]  aSeriesId          The Series ID.
     * @param[in]  aSeriesFlagsMask   The Series Flags bitmask which specify what types of frames are to be accounted.
     * @param[in]  aMetrics           Metrics to query.
     *
     */
    void Init(uint8_t aSeriesId, uint8_t aSeriesFlagsMask, const Metrics &aMetrics);

    /**
     * This method gets the Series ID.
     *
     * @returns  The Series ID.
     *
     */
    uint8_t GetSeriesId(void) const { return mSeriesId; }

    /**
     * This method gets the PDU count.
     *
     * @returns  The PDU count.
     *
     */
    uint32_t GetPduCount(void) const { return mPduCount; }

    /**
     * This method gets the average LQI.
     *
     * @returns  The average LQI.
     *
     */
    uint8_t GetAverageLqi(void) const { return mLqiAverager.GetAverage(); }

    /**
     * This method gets the average RSS.
     *
     * @returns  The average RSS.
     *
     */
    int8_t GetAverageRss(void) const { return mRssAverager.GetAverage(); }

    /**
     * This method aggregates the Link Metrics data of a frame into this series.
     *
     * @param[in]  aFrameType    The type of the frame.
     * @param[in]  aLqi          The LQI value.
     * @param[in]  aRss          The RSS value.
     *
     */
    void AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss);

    /**
     * This methods gets the metrics.
     *
     * @returns  The metrics associated with `SeriesInfo`.
     *
     */
    const Metrics &GetLinkMetrics(void) const { return mMetrics; }

private:
    bool Matches(const uint8_t &aSeriesId) const { return mSeriesId == aSeriesId; }
    bool IsFrameTypeMatch(uint8_t aFrameType) const;

    SeriesInfo *mNext;
    uint8_t     mSeriesId;
    SeriesFlags mSeriesFlags;
    Metrics     mMetrics;
    RssAverager mRssAverager;
    LqiAverager mLqiAverager;
    uint32_t    mPduCount;
};

/**
 * This enumeration type represents Link Metrics Status.
 *
 */
enum Status : uint8_t
{
    kStatusSuccess                   = OT_LINK_METRICS_STATUS_SUCCESS,
    kStatusCannotSupportNewSeries    = OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES,
    kStatusSeriesIdAlreadyRegistered = OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED,
    kStatusSeriesIdNotRecognized     = OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED,
    kStatusNoMatchingFramesReceived  = OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED,
    kStatusOtherError                = OT_LINK_METRICS_STATUS_OTHER_ERROR,
};

} // namespace LinkMetrics

DefineCoreType(otLinkMetrics, LinkMetrics::Metrics);
DefineCoreType(otLinkMetricsValues, LinkMetrics::MetricsValues);
DefineCoreType(otLinkMetricsSeriesFlags, LinkMetrics::SeriesFlags);
DefineMapEnum(otLinkMetricsEnhAckFlags, LinkMetrics::EnhAckFlags);
DefineMapEnum(otLinkMetricsStatus, LinkMetrics::Status);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // LINK_METRICS_TYPES_HPP_
