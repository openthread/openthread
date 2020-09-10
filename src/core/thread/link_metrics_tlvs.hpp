/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#ifndef LINK_METRICS_TLVS_HPP_
#define LINK_METRICS_TLVS_HPP_

#include <openthread/link_metrics.h>

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

namespace ot {

/**
 *  Link Metrics parameters
 *
 */
enum
{
    kLinkMetricsMaxTypeIdFlags =
        OT_LINK_METRICS_TYPE_ID_MAX_COUNT, ///< Max link metrics type id flags count in one query.
};

enum Type
{
    kLinkMetricsReportSub       = 0, ///< Link Metric Report Sub-TLV
    kLinkMetricsQueryId         = 1, ///< Link Metrics Query ID Sub-TLV
    kLinkMetricsQueryOptions    = 2, ///< Link Metrics Query Options Sub-TLV
    kForwardProbingRegistration = 3, ///< Forward Probing Registration Sub-TLV
    kReverseProbingRegistration = 4, ///< Reverse Probing Registration Sub-TLV
    kLinkMetricsStatus          = 5, ///< Link Metrics Status Sub-TLV
    kSeriesTrackingCapabilities = 6, ///< Series Tracking Capabilities Sub-TLV
    kEnhancedACKConfiguration   = 7, ///< Enhanced ACK Configuration Sub-TLV
};

/**
 * This class implements Link Metric Type Id Flags generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsTypeId
{
public:
    /**
     * This method init the Type Id value
     *
     */
    void Init(void) { mTypeId = 0; }

    /**
     * This method clears value follow flag.
     *
     */
    void ClearFollowFlag(void) { mTypeId &= ~kFollowFlag; }

    /**
     * This method sets the value follow flag.
     *
     */
    void SetFollowFlag(void) { mTypeId |= kFollowFlag; }

    /**
     * This method indicates whether or not the value follow flag is set.
     *
     * @retval TRUE   If the value follow flag is set, value follows after current 1 byte flags
     * @retval FALSE  If the value follow flag is not set, escape flags byte follows
     */
    bool IsFollowFlagSet(void) const { return (mTypeId & kFollowFlag) != 0; }

    /**
     * This method clears value length flag.
     *
     */
    void ClearLengthFlag(void) { mTypeId &= ~kLengthFlag; }

    /**
     * This method sets the value length flag.
     *
     */
    void SetLengthFlag(void) { mTypeId |= kLengthFlag; }

    /**
     * This method indicates whether or not the value length flag is set.
     *
     * @retval TRUE   If the value length flag is set, extended value length (4 bytes)
     * @retval FALSE  If the value length flag is not set, short value length (1 byte)
     */
    bool IsLengthFlagSet(void) const { return (mTypeId & kLengthFlag) != 0; }

    /**
     * This method sets the link metric type.
     *
     * @param[in]  aMetricsId  Link metric type.
     *
     */
    void SetMetricsType(uint8_t aMetricsType)
    {
        mTypeId = (mTypeId & ~kTypeMask) | ((aMetricsType << kTypeOffset) & kTypeMask);
    }

    /**
     * This method returns the link metric type.
     *
     * @returns The link metric type.
     *
     */
    uint8_t GetMetricsType(void) const { return (mTypeId & kTypeMask) >> kTypeOffset; }

    /**
     * This method sets the link metric Id.
     *
     * @param[in]  aMetricsId  Link metric Id.
     *
     */
    void SetMetricsId(uint8_t aMetricsId) { mTypeId = (mTypeId & ~kIdMask) | ((aMetricsId << kIdOffset) & kIdMask); }

    /**
     * This method returns the link metric Id.
     *
     * @returns The link metric Id.
     *
     */
    uint8_t GetMetricsId(void) const { return (mTypeId & kIdMask) >> kIdOffset; }

private:
    enum
    {
        kLengthFlag = 1 << 6,
        kFollowFlag = 1 << 7,
        kTypeOffset = 3,
        kTypeMask   = 7 << kTypeOffset,
        kIdOffset   = 0,
        kIdMask     = 7 << kIdOffset,
    };

    uint8_t mTypeId;
} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Report Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsReportSubTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMetricsReportSub);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the metric type ID.
     *
     * @returns The metric type ID.
     *
     */
    LinkMetricsTypeId GetMetricsTypeId(void) const { return mMetricsTypeId; }

    /**
     * This method sets the metric type ID.
     *
     * @param[in]  aMetricsTypeID  Metric type ID.
     *
     */
    void SetMetricsTypeId(LinkMetricsTypeId aMetricsTypeId)
    {
        mMetricsTypeId = aMetricsTypeId;
        if (!aMetricsTypeId.IsLengthFlagSet())
        {
            SetLength(sizeof(*this) - sizeof(Tlv) - sizeof(uint32_t) + sizeof(uint8_t)); // The value is 1 byte long
        }
    }

    /**
     * This method returns the metric value in 8 bits.
     *
     * @returns The metric value.
     *
     */
    uint8_t GetMetricValue8(void) const { return mMetricValue.m8; }

    /**
     * This method returns the metric value in 32 bits.
     *
     * @returns The metric value.
     *
     */
    uint32_t GetMetricValue32(void) const { return mMetricValue.m32; }

    /**
     * This method sets the metric value(8 bits).
     *
     * @param[in]  aMetricValue  Metric value.
     *
     */
    void SetMetricValue8(uint8_t aMetricValue) { mMetricValue.m8 = aMetricValue; }

    /**
     * This method sets the metric value(32 bits).
     *
     * @param[in]  aMetricValue  Metric value.
     *
     */
    void SetMetricValue32(uint32_t aMetricValue) { mMetricValue.m32 = aMetricValue; }

private:
    LinkMetricsTypeId mMetricsTypeId;
    union
    {
        uint8_t  m8;
        uint32_t m32;
    } mMetricValue;
} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Query Id Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsQueryId : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMetricsQueryId);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the series Id.
     *
     * @returns The series Id.
     *
     */
    uint8_t GetSeriesId(void) const { return static_cast<uint8_t>(mSeriesId); }

    /**
     * This method sets the series Id.
     *
     * @param[in]  aSeriesId  The series Id.
     *
     */
    void SetSeriesId(uint8_t aSeriesId) { mSeriesId = static_cast<uint8_t>(aSeriesId); }

private:
    uint8_t mSeriesId;
} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Query Options Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsQueryOptions : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMetricsQueryOptions);
        SetLength(sizeof(*this) - sizeof(Tlv) - sizeof(mMetricsTypeIds));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Link Metrics Type Id flags.
     *
     * @param[out]  aTypeId   The pointer to the array of Link Metrics Type Id flags.
     *
     * @retval  The pointer to the array of Link Metrics Type Id flags.
     *
     */
    const LinkMetricsTypeId *GetLinkMetricsTypeIdList(uint8_t &aCount) const
    {
        aCount = GetLength() / sizeof(LinkMetricsTypeId);

        return mMetricsTypeIds;
    }

    /**
     * This method sets the the link metrics type Id flags.
     *
     * @param[in]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[in]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void SetLinkMetricsTypeIdList(const LinkMetricsTypeId aTypeId[], uint8_t aCount)
    {
        uint8_t count = kLinkMetricsMaxTypeIdFlags;

        count = aCount < count ? aCount : count;

        memcpy(mMetricsTypeIds, aTypeId, count * sizeof(LinkMetricsTypeId));

        SetLength(count * sizeof(LinkMetricsTypeId));
    }

private:
    LinkMetricsTypeId mMetricsTypeIds[kLinkMetricsMaxTypeIdFlags];
} OT_TOOL_PACKED_END;

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#endif // LINK_METRICS_TLVS_HPP_
