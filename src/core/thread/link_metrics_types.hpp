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

/**
 * This type represents Link Metric Flags indicating a set of metrics.
 *
 * @sa otLinkMetrics
 *
 */
class Metrics : public otLinkMetrics, public Clearable<Metrics>
{
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

/**
 * This class implements Link Metrics Type ID Flags generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN class TypeIdFlags
{
    static constexpr uint8_t kExtendedFlag     = 1 << 7;
    static constexpr uint8_t kLengthOffset     = 6;
    static constexpr uint8_t kLengthFlag       = 1 << kLengthOffset;
    static constexpr uint8_t kTypeEnumOffset   = 3;
    static constexpr uint8_t kTypeEnumMask     = 7 << kTypeEnumOffset;
    static constexpr uint8_t kMetricEnumOffset = 0;
    static constexpr uint8_t kMetricEnumMask   = 7 << kMetricEnumOffset;

public:
    /**
     * This enumeration specifies the Length field in Type ID Flags.
     *
     */
    enum Length
    {
        kShortLength    = 0, ///< Short value length (1 byte value)
        kExtendedLength = 1, ///< Extended value length (4 bytes value)
    };

    /**
     * This enumeration specifies the Type values in Type ID Flags.
     *
     */
    enum TypeEnum : uint8_t
    {
        kTypeCountSummation   = 0, ///< Count or summation
        kTypeExpMovingAverage = 1, ///< Exponential moving average.
        kTypeReserved         = 2, ///< Reserved for future use.
    };

    /**
     * This enumeration specifies the Metric values in Type ID Flag.
     *
     */
    enum MetricEnum : uint8_t
    {
        kMetricPdusReceived = 0, ///< Number of PDUs received.
        kMetricLqi          = 1, ///< Link Quality Indicator.
        kMetricLinkMargin   = 2, ///< Link Margin.
        kMetricRssi         = 3, ///< RSSI in dbm.
    };

    /**
     * This constant defines the raw value for Type ID Flag for PDU.
     *
     */
    static constexpr uint8_t kPdu = (kExtendedLength << kLengthOffset) | (kTypeCountSummation << kTypeEnumOffset) |
                                    (kMetricPdusReceived << kMetricEnumOffset);

    /**
     * This constant defines the raw value for Type ID Flag for LQI.
     *
     */
    static constexpr uint8_t kLqi = (kShortLength << kLengthOffset) | (kTypeExpMovingAverage << kTypeEnumOffset) |
                                    (kMetricLqi << kMetricEnumOffset);

    /**
     * This constant defines the raw value for Type ID Flag for Link Margin.
     *
     */
    static constexpr uint8_t kLinkMargin = (kShortLength << kLengthOffset) |
                                           (kTypeExpMovingAverage << kTypeEnumOffset) |
                                           (kMetricLinkMargin << kMetricEnumOffset);

    /**
     * This constant defines the raw value for Type ID Flag for RSSI
     *
     */
    static constexpr uint8_t kRssi = (kShortLength << kLengthOffset) | (kTypeExpMovingAverage << kTypeEnumOffset) |
                                     (kMetricRssi << kMetricEnumOffset);

    /**
     * Default constructor.
     *
     */
    TypeIdFlags(void) = default;

    /**
     * Constructor to initialize from raw value.
     *
     * @param[in] aFlags  The raw flags value.
     *
     */
    explicit TypeIdFlags(uint8_t aFlags)
        : mFlags(aFlags)
    {
    }

    /**
     * This method initializes the Type ID value
     *
     */
    void Init(void) { mFlags = 0; }

    /**
     * This method clears the Extended flag.
     *
     */
    void ClearExtendedFlag(void) { mFlags &= ~kExtendedFlag; }

    /**
     * This method sets the Extended flag, indicating an additional second flags byte after the current 1-byte flags.
     * MUST NOT set in Thread 1.2.1.
     *
     */
    void SetExtendedFlag(void) { mFlags |= kExtendedFlag; }

    /**
     * This method indicates whether or not the Extended flag is set.
     *
     * @retval true   The Extended flag is set.
     * @retval false  The Extended flag is not set.
     *
     */
    bool IsExtendedFlagSet(void) const { return (mFlags & kExtendedFlag) != 0; }

    /**
     * This method clears value length flag.
     *
     */
    void ClearLengthFlag(void) { mFlags &= ~kLengthFlag; }

    /**
     * This method sets the value length flag.
     *
     */
    void SetLengthFlag(void) { mFlags |= kLengthFlag; }

    /**
     * This method indicates whether or not the value length flag is set.
     *
     * @retval true   The value length flag is set, extended value length (4 bytes)
     * @retval false  The value length flag is not set, short value length (1 byte)
     *
     */
    bool IsLengthFlagSet(void) const { return (mFlags & kLengthFlag) != 0; }

    /**
     * This method sets the Type/Average Enum.
     *
     * @param[in]  aTypeEnum  Type/Average Enum.
     *
     */
    void SetTypeEnum(TypeEnum aTypeEnum)
    {
        mFlags = (mFlags & ~kTypeEnumMask) | ((aTypeEnum << kTypeEnumOffset) & kTypeEnumMask);
    }

    /**
     * This method returns the Type/Average Enum.
     *
     * @returns The Type/Average Enum.
     *
     */
    TypeEnum GetTypeEnum(void) const { return static_cast<TypeEnum>((mFlags & kTypeEnumMask) >> kTypeEnumOffset); }

    /**
     * This method sets the Metric Enum.
     *
     * @param[in]  aMetricEnum  Metric Enum.
     *
     */
    void SetMetricEnum(MetricEnum aMetricEnum)
    {
        mFlags = (mFlags & ~kMetricEnumMask) | ((aMetricEnum << kMetricEnumOffset) & kMetricEnumMask);
    }

    /**
     * This method returns the Metric Enum.
     *
     * @returns The Metric Enum.
     *
     */
    MetricEnum GetMetricEnum(void) const
    {
        return static_cast<MetricEnum>((mFlags & kMetricEnumMask) >> kMetricEnumOffset);
    }

    /**
     * This method returns the raw value of the entire TypeIdFlags.
     *
     * @returns The raw value of TypeIdFlags.
     *
     */
    uint8_t GetRawValue(void) const { return mFlags; }

    /**
     * This method sets the raw value of the entire TypeIdFlags.
     *
     * @param[in]  aFlags  The raw flags value.
     *
     */
    void SetRawValue(uint8_t aFlags) { mFlags = aFlags; }

private:
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * This class implements Series Flags for Forward Tracking Series.
 *
 */
OT_TOOL_PACKED_BEGIN
class SeriesFlags
{
public:
    /**
     * This type represents which frames to be accounted in a Forward Tracking Series.
     *
     * @sa otLinkMetricsSeriesFlags
     *
     */
    typedef otLinkMetricsSeriesFlags Info;

    /**
     * Default constructor.
     *
     */
    SeriesFlags(void)
        : mFlags(0)
    {
    }

    /**
     * This method sets the values from an `Info` object.
     *
     * @param[in]  aSeriesFlags  The `Info` object.
     *
     */
    void SetFrom(const Info &aSeriesFlags);

    /**
     * This method clears the Link Probe flag.
     *
     */
    void ClearLinkProbeFlag(void) { mFlags &= ~kLinkProbeFlag; }

    /**
     * This method sets the Link Probe flag.
     *
     */
    void SetLinkProbeFlag(void) { mFlags |= kLinkProbeFlag; }

    /**
     * This method indicates whether or not the Link Probe flag is set.
     *
     * @retval true   The Link Probe flag is set.
     * @retval false  The Link Probe flag is not set.
     *
     */
    bool IsLinkProbeFlagSet(void) const { return (mFlags & kLinkProbeFlag) != 0; }

    /**
     * This method clears the MAC Data flag.
     *
     */
    void ClearMacDataFlag(void) { mFlags &= ~kMacDataFlag; }

    /**
     * This method sets the MAC Data flag.
     *
     */
    void SetMacDataFlag(void) { mFlags |= kMacDataFlag; }

    /**
     * This method indicates whether or not the MAC Data flag is set.
     *
     * @retval true   The MAC Data flag is set.
     * @retval false  The MAC Data flag is not set.
     *
     */
    bool IsMacDataFlagSet(void) const { return (mFlags & kMacDataFlag) != 0; }

    /**
     * This method clears the MAC Data Request flag.
     *
     */
    void ClearMacDataRequestFlag(void) { mFlags &= ~kMacDataRequestFlag; }

    /**
     * This method sets the MAC Data Request flag.
     *
     */
    void SetMacDataRequestFlag(void) { mFlags |= kMacDataRequestFlag; }

    /**
     * This method indicates whether or not the MAC Data Request flag is set.
     *
     * @retval true   The MAC Data Request flag is set.
     * @retval false  The MAC Data Request flag is not set.
     *
     */
    bool IsMacDataRequestFlagSet(void) const { return (mFlags & kMacDataRequestFlag) != 0; }

    /**
     * This method clears the Mac Ack flag.
     *
     */
    void ClearMacAckFlag(void) { mFlags &= ~kMacAckFlag; }

    /**
     * This method sets the Mac Ack flag.
     *
     */
    void SetMacAckFlag(void) { mFlags |= kMacAckFlag; }

    /**
     * This method indicates whether or not the Mac Ack flag is set.
     *
     * @retval true   The Mac Ack flag is set.
     * @retval false  The Mac Ack flag is not set.
     *
     */
    bool IsMacAckFlagSet(void) const { return (mFlags & kMacAckFlag) != 0; }

    /**
     * This method returns the raw value of flags.
     *
     */
    uint8_t GetRawValue(void) const { return mFlags; }

    /**
     * This method clears the all the flags.
     *
     */
    void Clear(void) { mFlags = 0; }

private:
    static constexpr uint8_t kLinkProbeFlag      = 1 << 0;
    static constexpr uint8_t kMacDataFlag        = 1 << 1;
    static constexpr uint8_t kMacDataRequestFlag = 1 << 2;
    static constexpr uint8_t kMacAckFlag         = 1 << 3;

    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * This enumeration type represent Enhanced-ACK Flags.
 *
 */
enum EnhAckFlags : uint8_t
{
    kEnhAckClear    = OT_LINK_METRICS_ENH_ACK_CLEAR,    ///< Clear.
    kEnhAckRegister = OT_LINK_METRICS_ENH_ACK_REGISTER, ///< Register.
};

constexpr uint8_t kMaxTypeIdFlags = 4; ///< Maximum number of TypeIdFlags in a `Metrics`

uint8_t TypeIdFlagsFromMetrics(TypeIdFlags aTypeIdFlags[], const Metrics &aMetrics);

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
     * already `kFcfFrameData`, `kFcfFrameAck` and `kFcfFrameMacCmd`. This item is added so that we can
     * filter a Link Probe for series in the same way as other frames.
     *
     */
    static constexpr uint8_t kSeriesTypeLinkProbe = 0;

    /**
     * This method initializes the SeriesInfo object.
     *
     * @param[in]  aSeriesId      The Series ID.
     * @param[in]  aSeriesFlags   The Series Flags which specify what types of frames are to be accounted.
     * @param[in]  aMetrics       Metrics to query.
     *
     */
    void Init(uint8_t aSeriesId, const SeriesFlags &aSeriesFlags, const Metrics &aMetrics);

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
DefineMapEnum(otLinkMetricsEnhAckFlags, LinkMetrics::EnhAckFlags);
DefineMapEnum(otLinkMetricsStatus, LinkMetrics::Status);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // LINK_METRICS_TYPES_HPP_
