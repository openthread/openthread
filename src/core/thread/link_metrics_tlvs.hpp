/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include "utils/wrap_string.h"

#include <openthread/link.h>

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"

using ot::Encoding::BigEndian::HostSwap16;

#if OPENTHREAD_CONFIG_LINK_PROBE_ENABLE

namespace ot {
namespace LinkProbing {

/**
 *  Link Metrics parameters
 *
 */
enum
{
    kLinkMetricsMaxNum =
        OPENTHREAD_CONFIG_MAX_LINK_METRICS_NUM, ///< The max link metrics number during each link probing
};

/**
 * This class implements Link Metrics TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsTlv : public ot::Tlv
{
public:
    /**
     *  Link Metrics TLV Types.
     *
     */
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
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

    /**
     * This method returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext(void) { return static_cast<Tlv *>(ot::Tlv::GetNext()); }

    const Tlv *GetNext(void) const { return static_cast<const Tlv *>(ot::Tlv::GetNext()); }

    /**
     * This static method reads the requested TLV out of @p aMessage.
     *
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aType       The Type value to search for.
     * @param[in]   aMaxLength  Maximum number of bytes to read.
     * @param[out]  aTlv        A reference to the TLV that will be copied to.
     *
     * @retval OT_ERROR_NONE       Successfully copied the TLV.
     * @retval OT_ERROR_NOT_FOUND  Could not find the TLV with Type @p aType.
     *
     */
    static otError GetTlv(const Message &aMessage, Type aType, uint16_t aMaxLength, Tlv &aTlv)
    {
        return ot::Tlv::Get(aMessage, static_cast<uint8_t>(aType), aMaxLength, aTlv);
    }

    /**
     * This static method finds the offset and length of a given TLV type.
     *
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aType       The Type value to search for.
     * @param[out]  aOffset     The offset where the value starts.
     * @param[out]  aLength     The length of the value.
     *
     * @retval OT_ERROR_NONE       Successfully found the TLV.
     * @retval OT_ERROR_NOT_FOUND  Could not find the TLV with Type @p aType.
     *
     */
    static otError GetValueOffset(const Message &aMessage, Type aType, uint16_t &aOffset, uint16_t &aLength)
    {
        return ot::Tlv::GetValueOffset(aMessage, static_cast<uint8_t>(aType), aOffset, aLength);
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metric Type Id Flags generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricTypeId
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
    bool IsFollowFlagSet(void) { return (mTypeId & kFollowFlag) != 0; }

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
    bool IsLengthFlagSet(void) { return (mTypeId & kLengthFlag) != 0; }

    /**
     * This method sets the link metric type.
     *
     * @param[in]  aMetricId  Link metric type.
     *
     */
    void SetMetricType(uint8_t aMetricType)
    {
        mTypeId = (mTypeId & ~kTypeMask) | ((aMetricType << kTypeOffset) & kTypeMask);
    }

    /**
     * This method returns the link metric type.
     *
     * @returns The link metric type.
     *
     */
    uint8_t GetMetricType(void) const { return (mTypeId & kTypeMask) >> kTypeOffset; }

    /**
     * This method sets the link metric Id.
     *
     * @param[in]  aMetricId  Link metric Id.
     *
     */
    void SetMetricId(uint8_t aMetricId) { mTypeId = (mTypeId & ~kIdMask) | ((aMetricId << kIdOffset) & kIdMask); }

    /**
     * This method returns the link metric Id.
     *
     * @returns The link metric Id.
     *
     */
    uint8_t GetMetricId(void) const { return (mTypeId & kIdMask) >> kIdOffset; }

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
class LinkMetricsReportSubTlv : public LinkMetricsTlv
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
    LinkMetricTypeId GetMetricTypeId(void) const { return mMetricTypeId; }

    /**
     * This method sets the metric type ID.
     *
     * @param[in]  aMetricTypeID  Metric type ID.
     *
     */
    void SetMetricTypeId(LinkMetricTypeId aMetricTypeId)
    {
        mMetricTypeId = aMetricTypeId;
        if (!aMetricTypeId.IsLengthFlagSet())
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
    LinkMetricTypeId mMetricTypeId;
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
class LinkMetricsQueryId : public LinkMetricsTlv
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
class LinkMetricsQueryOptions : public LinkMetricsTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMetricsQueryOptions);
        SetLength(sizeof(*this) - sizeof(Tlv) - sizeof(mMetricTypeIds));
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
     * This method returns the link metrics type Id flags.
     *
     * @param[out]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[out]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void GetLinkMetricTypeIdList(LinkMetricTypeId aTypeId[], uint8_t *aCount) const
    {
        uint8_t count = GetLength() / sizeof(LinkMetricTypeId);

        *aCount = count > *aCount ? *aCount : count;

        memcpy(aTypeId, mMetricTypeIds, *aCount * sizeof(LinkMetricTypeId));
    }

    /**
     * This method sets the the link metrics type Id flags.
     *
     * @param[in]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[in]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void SetLinkMetricTypeIdList(LinkMetricTypeId aTypeId[], uint8_t aCount)
    {
        uint8_t count = kLinkMetricsMaxNum;

        count = aCount < count ? aCount : count;

        memcpy(mMetricTypeIds, aTypeId, count * sizeof(LinkMetricTypeId));

        SetLength(count * sizeof(LinkMetricTypeId));
    }

private:
    uint8_t mMetricTypeIds[kLinkMetricsMaxNum];
} OT_TOOL_PACKED_END;

/**
 * This structure presents Probing Registration contents used in ForwardProbingRegistrationTlv.
 *
 */
OT_TOOL_PACKED_BEGIN
struct ProbingRegistration
{
    uint8_t mSeriesId;
    uint8_t mSeriesFlags;
    uint8_t mMetricTypeIds[kLinkMetricsMaxNum];
} OT_TOOL_PACKED_END;

/**
 * This class implements Forward Probing Registration TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ForwardProbingRegistrationTlv : public LinkMetricsTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kForwardProbingRegistration);
        SetLength(sizeof(*this) - sizeof(Tlv) - sizeof(mProbingRegistration.mMetricTypeIds));
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
     * This method returns the forward series Id.
     *
     * @returns The forward series Id.
     *
     */
    uint8_t GetSeriesId(void) const { return mProbingRegistration.mSeriesId; }

    /**
     * This method sets the forward series Id.
     *
     * @param[in]  aSeriesID  The forward series Id.
     *
     */
    void SetSeriesId(uint8_t aSeriesId) { mProbingRegistration.mSeriesId = aSeriesId; }

    /**
     * This method returns the forward series flags.
     *
     * @returns The forward series flags.
     *
     */
    uint8_t GetSeriesFlags(void) const { return mProbingRegistration.mSeriesFlags; }

    /**
     * This method sets the forward series flags.
     *
     * @param[in]  aSeriesFlag  The forward series Flags.
     *
     */
    void SetSeriesFlags(uint8_t aSeriesFlags) { mProbingRegistration.mSeriesFlags = aSeriesFlags; }

    /**
     * This method returns the link metrics type Id flags.
     *
     * @param[out]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[out]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void GetLinkMetricTypeIdList(LinkMetricTypeId aTypeId[], uint8_t *aCount) const
    {
        uint8_t count = (GetLength() - offsetof(ProbingRegistration, mMetricTypeIds)) / sizeof(LinkMetricTypeId);

        *aCount = count > *aCount ? *aCount : count;

        memcpy(aTypeId, mProbingRegistration.mMetricTypeIds, *aCount * sizeof(LinkMetricTypeId));
    }

    /**
     * This method
     *
     */
    uint8_t *GetLinkMetricsTypeIdList() { return mProbingRegistration.mMetricTypeIds; }

    /**
     * This method
     *
     */
    uint8_t GetLinkMetricsTypeIdCount()
    {
        return (GetLength() - offsetof(ProbingRegistration, mMetricTypeIds)) / sizeof(LinkMetricTypeId);
    }

    /**
     * This method sets the the link metrics type Id flags.
     *
     * @param[in]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[in]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void SetLinkMetricTypeIdList(LinkMetricTypeId aTypeId[], uint8_t aCount)
    {
        uint8_t count = kLinkMetricsMaxNum;

        count = aCount < count ? aCount : count;

        memcpy(mProbingRegistration.mMetricTypeIds, aTypeId, count * sizeof(LinkMetricTypeId));

        SetLength(count * sizeof(LinkMetricTypeId) + offsetof(ProbingRegistration, mMetricTypeIds));
    }

private:
    ProbingRegistration mProbingRegistration;
} OT_TOOL_PACKED_END;

/**
 * This class implements Enhanced ACK Configuration TLV generation and parsing.
 *
 */
class EnhancedAckConfigurationTlv : public LinkMetricsTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kEnhancedACKConfiguration);
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
     * This method
     *
     */
    uint8_t GetEnhAckFlags() { return mEnhAckFlags; }

    /**
     * This method
     *
     */
    void SetEnhAckFlags(uint8_t aEnhAckFlags) { mEnhAckFlags = aEnhAckFlags; }

    /**
     * This method
     *
     */
    uint8_t *GetLinkMetricsTypeIdList() { return mMetricTypeIds; }

    /**
     * This method
     *
     */
    uint8_t GetLinkMetricsTypeIdCount() { return (GetLength() - sizeof(mEnhAckFlags)) / sizeof(LinkMetricTypeId); }

    /**
     * This method sets the the link metrics type Id flags.
     *
     * @param[in]  aTypeId   The pointer to the array of link metrics type Id flags.
     * @param[in]  aCount    The count of link metrics type Id flags in the array.
     *
     */
    void SetLinkMetricTypeIdList(LinkMetricTypeId aTypeId[], uint8_t aCount)
    {
        uint8_t count = kLinkMetricsMaxNum;

        count = aCount < count ? aCount : count;

        memcpy(mMetricTypeIds, aTypeId, count * sizeof(LinkMetricTypeId));

        SetLength(count * sizeof(LinkMetricTypeId) + sizeof(mEnhAckFlags));
    }

private:
    uint8_t mEnhAckFlags;
    uint8_t mMetricTypeIds[kLinkMetricsMaxNum];

} OT_TOOL_PACKED_END;

/**
 * This class implements Link Metrics Status Sub-TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMetricsStatusTlv : public LinkMetricsTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMetricsStatus);
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
     * This method returns the status.
     *
     * @returns The status.
     *
     */
    uint8_t GetStatus(void) const { return mStatus; }

    /**
     * This method sets the status.
     *
     * @param[in]  aStatus  The status.
     *
     */
    void SetStatus(uint8_t aStatus) { mStatus = aStatus; }

private:
    uint8_t mStatus;
} OT_TOOL_PACKED_END;

} // namespace LinkProbing
} // namespace ot

#endif // OPENTHREAD_CONFIG_LINK_PROBE_ENABLE

#endif // MESHCOP_TLVS_HPP_
