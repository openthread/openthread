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

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#include <openthread/link_metrics.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "thread/link_metrics_types.hpp"

namespace ot {
namespace LinkMetrics {

using ot::Encoding::BigEndian::HostSwap32;

/**
 * This class defines constants related to Link Metrics Sub-TLVs.
 *
 */
class SubTlv
{
public:
    /**
     * Link Metrics Sub-TLV types.
     *
     */
    enum Type : uint8_t
    {
        kReport        = 0, ///< Report Sub-TLV
        kQueryId       = 1, ///< Query ID Sub-TLV
        kQueryOptions  = 2, ///< Query Options Sub-TLV
        kFwdProbingReg = 3, ///< Forward Probing Registration Sub-TLV
        kStatus        = 5, ///< Status Sub-TLV
        kEnhAckConfig  = 7, ///< Enhanced ACK Configuration Sub-TLV
    };
};

/**
 * This class defines Link Metrics Query ID Sub-TLV constants and types.
 *
 */
typedef UintTlvInfo<SubTlv::kQueryId, uint8_t> QueryIdSubTlv;

/**
 * This type defines a Link Metrics Status Sub-Tlv.
 *
 */
typedef UintTlvInfo<SubTlv::kStatus, uint8_t> StatusSubTlv;

/**
 * This class implements Link Metrics Report Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ReportSubTlv : public Tlv, public TlvInfo<SubTlv::kReport>
{
public:
    static constexpr uint8_t kMinLength = 2; ///< Minimum expected TLV length (type ID and u8 value).

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(SubTlv::kReport); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval true   The TLV appears to be well-formed.
     * @retval false  The TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= kMinLength; }

    /**
     * This method returns the Link Metrics Type ID.
     *
     * @returns The Link Metrics Type ID.
     *
     */
    uint8_t GetMetricsTypeId(void) const { return mMetricsTypeId; }

    /**
     * This method sets the Link Metrics Type ID.
     *
     * @param[in]  aMetricsTypeId  The Link Metrics Type ID to set.
     *
     */
    void SetMetricsTypeId(uint8_t aMetricsTypeId) { mMetricsTypeId = aMetricsTypeId; }

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
    uint32_t GetMetricsValue32(void) const { return HostSwap32(mMetricsValue.m32); }

    /**
     * This method sets the metric value (8 bits).
     *
     * @param[in]  aMetricsValue  Metrics value.
     *
     */
    void SetMetricsValue8(uint8_t aMetricsValue)
    {
        mMetricsValue.m8 = aMetricsValue;
        SetLength(kMinLength);
    }

    /**
     * This method sets the metric value (32 bits).
     *
     * @param[in]  aMetricsValue  Metrics value.
     *
     */
    void SetMetricsValue32(uint32_t aMetricsValue)
    {
        mMetricsValue.m32 = HostSwap32(aMetricsValue);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

private:
    uint8_t mMetricsTypeId;
    union
    {
        uint8_t  m8;
        uint32_t m32;
    } mMetricsValue;
} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Query Options Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class QueryOptionsSubTlv : public Tlv, public TlvInfo<SubTlv::kQueryOptions>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(SubTlv::kQueryOptions);
        SetLength(0);
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(uint8_t); }

} OT_TOOL_PACKED_END;

/**
 * This class defines Link Metrics Forward Probing Registration Sub-TLV.
 *
 */
OT_TOOL_PACKED_BEGIN
class FwdProbingRegSubTlv : public Tlv, public TlvInfo<SubTlv::kFwdProbingReg>
{
public:
    static constexpr uint8_t kMinLength = sizeof(uint8_t) + sizeof(uint8_t); ///< Minimum expected TLV length

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(SubTlv::kFwdProbingReg);
        SetLength(kMinLength);
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval true   The TLV appears to be well-formed.
     * @retval false  The TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= kMinLength; }

    /**
     * This method gets the Forward Series ID value.
     *
     * @returns The Forward Series ID.
     *
     */
    uint8_t GetSeriesId(void) const { return mSeriesId; }

    /**
     * This method sets the Forward Series ID value.
     *
     * @param[in] aSeriesId  The Forward Series ID.
     *
     */
    void SetSeriesId(uint8_t aSeriesId) { mSeriesId = aSeriesId; }

    /**
     * This method gets the Forward Series Flags bit-mask.
     *
     * @returns The Forward Series Flags mask.
     *
     */
    uint8_t GetSeriesFlagsMask(void) const { return mSeriesFlagsMask; }

    /**
     * This method sets the Forward Series Flags bit-mask
     *
     * @param[in] aSeriesFlagsMask  The Forward Series Flags.
     *
     */
    void SetSeriesFlagsMask(uint8_t aSeriesFlagsMask) { mSeriesFlagsMask = aSeriesFlagsMask; }

    /**
     * This method gets the start of Type ID array.
     *
     * @returns The start of Type ID array. Array has `kMaxTypeIds` max length.
     *
     */
    uint8_t *GetTypeIds(void) { return mTypeIds; }

private:
    uint8_t mSeriesId;
    uint8_t mSeriesFlagsMask;
    uint8_t mTypeIds[kMaxTypeIds];
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class EnhAckConfigSubTlv : public Tlv, public TlvInfo<SubTlv::kEnhAckConfig>
{
public:
    static constexpr uint8_t kMinLength = sizeof(uint8_t); ///< Minimum TLV length (only `EnhAckFlags`).

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(SubTlv::kEnhAckConfig);
        SetLength(kMinLength);
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval true   The TLV appears to be well-formed.
     * @retval false  The TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= kMinLength; }

    /**
     * This method gets the Enhanced ACK Flags.
     *
     * @returns The Enhanced ACK Flags.
     *
     */
    uint8_t GetEnhAckFlags(void) const { return mEnhAckFlags; }

    /**
     * This method sets Enhanced ACK Flags.
     *
     * @param[in] aEnhAckFlags  The value of Enhanced ACK Flags.
     *
     */
    void SetEnhAckFlags(EnhAckFlags aEnhAckFlags) { mEnhAckFlags = aEnhAckFlags; }

    /**
     * This method gets the start of Type ID array.
     *
     * @returns The start of Type ID array. Array has `kMaxTypeIds` max length.
     *
     */
    uint8_t *GetTypeIds(void) { return mTypeIds; }

private:
    uint8_t mEnhAckFlags;
    uint8_t mTypeIds[kMaxTypeIds];
} OT_TOOL_PACKED_END;

} // namespace LinkMetrics
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // LINK_METRICS_TLVS_HPP_
