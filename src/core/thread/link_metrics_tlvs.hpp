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
    kLinkMetricsMaxTypeIdFlags = OT_LINK_METRICS_TYPE_ID_MAX_COUNT, ///< Max count of Link Metrics Type ID Flags.
};

enum Type
{
    kLinkMetricsReportSub       = 0, ///< Link Metrics Report Sub-TLV
    kLinkMetricsQueryId         = 1, ///< Link Metrics Query ID Sub-TLV
    kLinkMetricsQueryOptions    = 2, ///< Link Metrics Query Options Sub-TLV
    kForwardProbingRegistration = 3, ///< Forward Probing Registration Sub-TLV
    kReverseProbingRegistration = 4, ///< Reverse Probing Registration Sub-TLV
    kLinkMetricsStatus          = 5, ///< Link Metrics Status Sub-TLV
    kSeriesTrackingCapabilities = 6, ///< Series Tracking Capabilities Sub-TLV
    kEnhancedACKConfiguration   = 7, ///< Enhanced ACK Configuration Sub-TLV
};

/**
 * This class implements Link Metrics Type Id Flags generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsTypeIdFlags
{
public:
    /**
     * This method init the Type Id value
     *
     */
    void Init(void) { mTypeIdFlags = 0; }

    /**
     * This method clears the Extended flag.
     *
     */
    void ClearExtendedFlag(void) { mTypeIdFlags &= ~kExtendedFlag; }

    /**
     * This method sets the Extended flag, indicating an additional second flags byte after the current 1-byte flags.
     * MUST NOT set in Thread 1.2.1.
     *
     */
    void SetExtendedFlag(void) { mTypeIdFlags |= kExtendedFlag; }

    /**
     * This method indicates whether or not the Extended flag is set.
     *
     * @retval true   The Extended flag is set.
     * @retval false  The Extended flag is not set.
     */
    bool IsExtendedFlagSet(void) const { return (mTypeIdFlags & kExtendedFlag) != 0; }

    /**
     * This method clears value length flag.
     *
     */
    void ClearLengthFlag(void) { mTypeIdFlags &= ~kLengthFlag; }

    /**
     * This method sets the value length flag.
     *
     */
    void SetLengthFlag(void) { mTypeIdFlags |= kLengthFlag; }

    /**
     * This method indicates whether or not the value length flag is set.
     *
     * @retval true   The value length flag is set, extended value length (4 bytes)
     * @retval false  The value length flag is not set, short value length (1 byte)
     */
    bool IsLengthFlagSet(void) const { return (mTypeIdFlags & kLengthFlag) != 0; }

    /**
     * This method sets the Link Metrics type.
     *
     * @param[in]  aMetricsType  Link Metrics type.
     *
     */
    void SetMetricsType(uint8_t aMetricsType)
    {
        mTypeIdFlags = (mTypeIdFlags & ~kTypeMask) | ((aMetricsType << kTypeOffset) & kTypeMask);
    }

    /**
     * This method returns the Link Metrics type.
     *
     * @returns The Link Metrics type.
     *
     */
    uint8_t GetMetricsType(void) const { return (mTypeIdFlags & kTypeMask) >> kTypeOffset; }

    /**
     * This method sets the Link Metrics ID.
     *
     * @param[in]  aMetricsId  Link Metrics ID.
     *
     */
    void SetMetricsId(uint8_t aMetricsId)
    {
        mTypeIdFlags = (mTypeIdFlags & ~kIdMask) | ((aMetricsId << kIdOffset) & kIdMask);
    }

    /**
     * This method returns the Link Metrics ID.
     *
     * @returns The Link Metrics ID.
     *
     */
    uint8_t GetMetricsId(void) const { return (mTypeIdFlags & kIdMask) >> kIdOffset; }

private:
    enum
    {
        kLengthFlag   = 1 << 6,
        kExtendedFlag = 1 << 7,
        kTypeOffset   = 3,
        kTypeMask     = 7 << kTypeOffset,
        kIdOffset     = 0,
        kIdMask       = 7 << kIdOffset,
    };

    uint8_t mTypeIdFlags;
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
     * @retval true   The TLV appears to be well-formed.
     * @retval false  The TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Link Metrics Type ID.
     *
     * @returns The Link Metrics Type ID.
     *
     */
    LinkMetricsTypeIdFlags GetMetricsTypeId(void) const { return mMetricsTypeId; }

    /**
     * This method sets the Link Metrics Type ID.
     *
     * @param[in]  aMetricsTypeID  The Link Metrics Type ID to set.
     *
     */
    void SetMetricsTypeId(LinkMetricsTypeIdFlags aMetricsTypeId)
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
    uint8_t GetMetricsValue8(void) const { return mMetricsValue.m8; }

    /**
     * This method returns the metric value in 32 bits.
     *
     * @returns The metric value.
     *
     */
    uint32_t GetMetricsValue32(void) const { return mMetricsValue.m32; }

    /**
     * This method sets the metric value(8 bits).
     *
     * @param[in]  aMetricsValue  Metrics value.
     *
     */
    void SetMetricsValue8(uint8_t aMetricsValue) { mMetricsValue.m8 = aMetricsValue; }

    /**
     * This method sets the metric value(32 bits).
     *
     * @param[in]  aMetricsValue  Metrics value.
     *
     */
    void SetMetricsValue32(uint32_t aMetricsValue) { mMetricsValue.m32 = aMetricsValue; }

private:
    LinkMetricsTypeIdFlags mMetricsTypeId;
    union
    {
        uint8_t  m8;
        uint32_t m32;
    } mMetricsValue;
} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Query ID Sub-TLV generation and parsing.
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
     * This method returns the Series ID.
     *
     * @returns The Series ID.
     *
     */
    uint8_t GetSeriesId(void) const { return mSeriesId; }

    /**
     * This method sets the Series ID.
     *
     * @param[in]  aSeriesId  The Series ID.
     *
     */
    void SetSeriesId(uint8_t aSeriesId) { mSeriesId = aSeriesId; }

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
     * This method returns the Link Metrics Type ID Flags and its count.
     *
     * @param[out]  aCount   The count of Link Metrics Type ID Flags in the returned array.
     *
     * @retval  The pointer to the array of Link Metrics Type ID Flags.
     *
     */
    const LinkMetricsTypeIdFlags *GetLinkMetricsTypeIdFlagsList(uint8_t &aCount) const
    {
        aCount = GetLength() / sizeof(LinkMetricsTypeIdFlags);

        return mMetricsTypeIds;
    }

    /**
     * This method sets the the Link Metrics Type ID Flags.
     *
     * @param[in]  aTypeId   The pointer to the array of Link Metrics Type ID Flags.
     * @param[in]  aCount    The count of Link Metrics Type ID Flags in the array.
     *
     */
    void SetLinkMetricsTypeIdFlagsList(const LinkMetricsTypeIdFlags aTypeId[], uint8_t aCount)
    {
        uint8_t count = kLinkMetricsMaxTypeIdFlags;

        count = aCount < count ? aCount : count;

        memcpy(mMetricsTypeIds, aTypeId, count * sizeof(LinkMetricsTypeIdFlags));

        SetLength(count * sizeof(LinkMetricsTypeIdFlags));
    }

private:
    LinkMetricsTypeIdFlags mMetricsTypeIds[kLinkMetricsMaxTypeIdFlags];
} OT_TOOL_PACKED_END;

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#endif // LINK_METRICS_TLVS_HPP_
