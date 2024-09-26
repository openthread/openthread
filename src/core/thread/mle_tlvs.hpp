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
 *   This file includes definitions for generating and processing MLE TLVs.
 */

#ifndef MLE_TLVS_HPP_
#define MLE_TLVS_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/preference.hpp"
#include "common/tlvs.hpp"
#include "meshcop/timestamp.hpp"
#include "net/ip6_address.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {

namespace Mle {

/**
 * @addtogroup core-mle-tlvs
 *
 * @brief
 *   This module includes definitions for generating and processing MLE TLVs.
 *
 * @{
 */

/**
 * Implements MLE TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * MLE TLV Types.
     */
    enum Type : uint8_t
    {
        kSourceAddress         = 0,  ///< Source Address TLV
        kMode                  = 1,  ///< Mode TLV
        kTimeout               = 2,  ///< Timeout TLV
        kChallenge             = 3,  ///< Challenge TLV
        kResponse              = 4,  ///< Response TLV
        kLinkFrameCounter      = 5,  ///< Link-Layer Frame Counter TLV
        kLinkQuality           = 6,  ///< Link Quality TLV
        kNetworkParameter      = 7,  ///< Network Parameter TLV
        kMleFrameCounter       = 8,  ///< MLE Frame Counter TLV
        kRoute                 = 9,  ///< Route64 TLV
        kAddress16             = 10, ///< Address16 TLV
        kLeaderData            = 11, ///< Leader Data TLV
        kNetworkData           = 12, ///< Network Data TLV
        kTlvRequest            = 13, ///< TLV Request TLV
        kScanMask              = 14, ///< Scan Mask TLV
        kConnectivity          = 15, ///< Connectivity TLV
        kLinkMargin            = 16, ///< Link Margin TLV
        kStatus                = 17, ///< Status TLV
        kVersion               = 18, ///< Version TLV
        kAddressRegistration   = 19, ///< Address Registration TLV
        kChannel               = 20, ///< Channel TLV
        kPanId                 = 21, ///< PAN ID TLV
        kActiveTimestamp       = 22, ///< Active Timestamp TLV
        kPendingTimestamp      = 23, ///< Pending Timestamp TLV
        kActiveDataset         = 24, ///< Active Operational Dataset TLV
        kPendingDataset        = 25, ///< Pending Operational Dataset TLV
        kDiscovery             = 26, ///< Thread Discovery TLV
        kSupervisionInterval   = 27, ///< Supervision Interval TLV
        kWakeupChannel         = 74, ///< Wakeup Channel TLV
        kCslChannel            = 80, ///< CSL Channel TLV
        kCslTimeout            = 85, ///< CSL Timeout TLV
        kCslClockAccuracy      = 86, ///< CSL Clock Accuracy TLV
        kLinkMetricsQuery      = 87, ///< Link Metrics Query TLV
        kLinkMetricsManagement = 88, ///< Link Metrics Management TLV
        kLinkMetricsReport     = 89, ///< Link Metrics Report TLV
        kLinkProbe             = 90, ///< Link Probe TLV

        /**
         * Applicable/Required only when time synchronization service
         * (`OPENTHREAD_CONFIG_TIME_SYNC_ENABLE`) is enabled.
         */
        kTimeRequest   = 252, ///< Time Request TLV
        kTimeParameter = 253, ///< Time Parameter TLV
        kXtalAccuracy  = 254, ///< XTAL Accuracy TLV

        kInvalid = 255,
    };

    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

} OT_TOOL_PACKED_END;

/**
 * Defines Source Address TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kSourceAddress, uint16_t> SourceAddressTlv;

/**
 * Defines Mode TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * Defines Challenge TLV constants and types.
 */
typedef TlvInfo<Tlv::kChallenge> ChallengeTlv;

/**
 * Defines Response TLV constants and types.
 */
typedef TlvInfo<Tlv::kResponse> ResponseTlv;

/**
 * Defines Link Frame Counter TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kLinkFrameCounter, uint32_t> LinkFrameCounterTlv;

/**
 * Defines MLE Frame Counter TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMleFrameCounter, uint32_t> MleFrameCounterTlv;

/**
 * Defines Address16 TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kAddress16, uint16_t> Address16Tlv;

/**
 * Defines Network Data TLV constants and types.
 */
typedef TlvInfo<Tlv::kNetworkData> NetworkDataTlv;

/**
 * Defines TLV Request TLV constants and types.
 */
typedef TlvInfo<Tlv::kTlvRequest> TlvRequestTlv;

/**
 * Defines Link Margin TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kLinkMargin, uint8_t> LinkMarginTlv;

/**
 * Defines Version TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kVersion, uint16_t> VersionTlv;

/**
 * Defines PAN ID TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kPanId, uint16_t> PanIdTlv;

/**
 * Defines Active Timestamp TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kActiveTimestamp, MeshCoP::Timestamp> ActiveTimestampTlv;

/**
 * Defines Pending Timestamp TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kPendingTimestamp, MeshCoP::Timestamp> PendingTimestampTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kSupervisionInterval, uint16_t> SupervisionIntervalTlv;

/**
 * Defines CSL Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kCslTimeout, uint32_t> CslTimeoutTlv;

/**
 * Defines XTAL Accuracy TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kXtalAccuracy, uint16_t> XtalAccuracyTlv;

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

/**
 * Implements Route TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Tlv, public TlvInfo<Tlv::kRoute>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const;

    /**
     * Returns the Router ID Sequence value.
     *
     * @returns The Router ID Sequence value.
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * Sets the Router ID Sequence value.
     *
     * @param[in]  aSequence  The Router ID Sequence value.
     */
    void SetRouterIdSequence(uint8_t aSequence) { mRouterIdSequence = aSequence; }

    /**
     * Gets the Router ID Mask.
     */
    const RouterIdSet &GetRouterIdMask(void) const { return mRouterIdMask; }

    /**
     * Sets the Router ID Mask.
     *
     * @param[in]  aRouterIdSet The Router ID Mask to set.
     */
    void SetRouterIdMask(const RouterIdSet &aRouterIdSet) { mRouterIdMask = aRouterIdSet; }

    /**
     * Indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID bit.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     */
    bool IsRouterIdSet(uint8_t aRouterId) const { return mRouterIdMask.Contains(aRouterId); }

    /**
     * Indicates whether the `RouteTlv` is a singleton, i.e., only one router is allocated.
     *
     * @retval TRUE   It is a singleton.
     * @retval FALSE  It is not a singleton.
     */
    bool IsSingleton(void) const { return IsValid() && (mRouterIdMask.GetNumberOfAllocatedIds() <= 1); }

    /**
     * Returns the Route Data Length value.
     *
     * @returns The Route Data Length value.
     */
    uint8_t GetRouteDataLength(void) const { return GetLength() - sizeof(mRouterIdSequence) - sizeof(mRouterIdMask); }

    /**
     * Sets the Route Data Length value.
     *
     * @param[in]  aLength  The Route Data Length value.
     */
    void SetRouteDataLength(uint8_t aLength) { SetLength(sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) + aLength); }

    /**
     * Returns the Route Cost value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Route Cost value for a given Router index.
     */
    uint8_t GetRouteCost(uint8_t aRouterIndex) const { return mRouteData[aRouterIndex] & kRouteCostMask; }

    /**
     * Returns the Link Quality In value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Link Quality In value for a given Router index.
     */
    LinkQuality GetLinkQualityIn(uint8_t aRouterIndex) const
    {
        return static_cast<LinkQuality>((mRouteData[aRouterIndex] & kLinkQualityInMask) >> kLinkQualityInOffset);
    }

    /**
     * Returns the Link Quality Out value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Link Quality Out value for a given Router index.
     */
    LinkQuality GetLinkQualityOut(uint8_t aRouterIndex) const
    {
        return static_cast<LinkQuality>((mRouteData[aRouterIndex] & kLinkQualityOutMask) >> kLinkQualityOutOffset);
    }

    /**
     * Sets the Route Data (Link Quality In/Out and Route Cost) for a given Router index.
     *
     * @param[in]  aRouterIndex    The Router index.
     * @param[in]  aLinkQualityIn  The Link Quality In value.
     * @param[in]  aLinkQualityOut The Link Quality Out value.
     * @param[in]  aRouteCost      The Route Cost value.
     */
    void SetRouteData(uint8_t aRouterIndex, LinkQuality aLinkQualityIn, LinkQuality aLinkQualityOut, uint8_t aRouteCost)
    {
        mRouteData[aRouterIndex] = (((aLinkQualityIn << kLinkQualityInOffset) & kLinkQualityInMask) |
                                    ((aLinkQualityOut << kLinkQualityOutOffset) & kLinkQualityOutMask) |
                                    ((aRouteCost << kRouteCostOffset) & kRouteCostMask));
    }

private:
    static constexpr uint8_t kLinkQualityOutOffset = 6;
    static constexpr uint8_t kLinkQualityOutMask   = 3 << kLinkQualityOutOffset;
    static constexpr uint8_t kLinkQualityInOffset  = 4;
    static constexpr uint8_t kLinkQualityInMask    = 3 << kLinkQualityInOffset;
    static constexpr uint8_t kRouteCostOffset      = 0;
    static constexpr uint8_t kRouteCostMask        = 0xf << kRouteCostOffset;

    uint8_t     mRouterIdSequence;
    RouterIdSet mRouterIdMask;
    uint8_t     mRouteData[kMaxRouterId + 1];
} OT_TOOL_PACKED_END;

#else // OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

/**
 * Implements Route TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Tlv, public TlvInfo<Tlv::kRoute>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void)
    {
        SetType(kRoute);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(mRouterIdSequence) + sizeof(mRouterIdMask); }

    /**
     * Returns the Router ID Sequence value.
     *
     * @returns The Router ID Sequence value.
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * Sets the Router ID Sequence value.
     *
     * @param[in]  aSequence  The Router ID Sequence value.
     */
    void SetRouterIdSequence(uint8_t aSequence) { mRouterIdSequence = aSequence; }

    /**
     * Gets the Router ID Mask.
     */
    const RouterIdSet &GetRouterIdMask(void) const { return mRouterIdMask; }

    /**
     * Sets the Router ID Mask.
     *
     * @param[in]  aRouterIdSet The Router ID Mask to set.
     */
    void SetRouterIdMask(const RouterIdSet &aRouterIdSet) { mRouterIdMask = aRouterIdSet; }

    /**
     * Indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     */
    bool IsRouterIdSet(uint8_t aRouterId) const { return mRouterIdMask.Contains(aRouterId); }

    /**
     * Indicates whether the `RouteTlv` is a singleton, i.e., only one router is allocated.
     *
     * @retval TRUE   It is a singleton.
     * @retval FALSE  It is not a singleton.
     */
    bool IsSingleton(void) const { return IsValid() && (mRouterIdMask.GetNumberOfAllocatedIds() <= 1); }

    /**
     * Sets the Router ID bit.
     *
     * @param[in]  aRouterId  The Router ID bit to set.
     */
    void SetRouterId(uint8_t aRouterId) { mRouterIdMask.Add(aRouterId); }

    /**
     * Returns the Route Data Length value.
     *
     * @returns The Route Data Length value in bytes
     */
    uint8_t GetRouteDataLength(void) const { return GetLength() - sizeof(mRouterIdSequence) - sizeof(mRouterIdMask); }

    /**
     * Sets the Route Data Length value.
     *
     * @param[in]  aLength  The Route Data Length value in number of router entries
     */
    void SetRouteDataLength(uint8_t aLength)
    {
        SetLength(sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) + aLength + (aLength + 1) / 2);
    }

    /**
     * Returns the Route Cost value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Route Cost value for a given Router index.
     */
    uint8_t GetRouteCost(uint8_t aRouterIndex) const
    {
        if (aRouterIndex & 1)
        {
            return mRouteData[aRouterIndex + aRouterIndex / 2 + 1];
        }
        else
        {
            return static_cast<uint8_t>((mRouteData[aRouterIndex + aRouterIndex / 2] & kRouteCostMask)
                                        << kOddEntryOffset) |
                   ((mRouteData[aRouterIndex + aRouterIndex / 2 + 1] &
                     static_cast<uint8_t>(kRouteCostMask << kOddEntryOffset)) >>
                    kOddEntryOffset);
        }
    }

    /**
     * Returns the Link Quality In value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Link Quality In value for a given Router index.
     */
    LinkQuality GetLinkQualityIn(uint8_t aRouterIndex) const
    {
        int offset = ((aRouterIndex & 1) ? kOddEntryOffset : 0);
        return static_cast<LinkQuality>(
            (mRouteData[aRouterIndex + aRouterIndex / 2] & (kLinkQualityInMask >> offset)) >>
            (kLinkQualityInOffset - offset));
    }

    /**
     * Returns the Link Quality Out value for a given Router index.
     *
     * @param[in]  aRouterIndex  The Router index.
     *
     * @returns The Link Quality Out value for a given Router index.
     */
    LinkQuality GetLinkQualityOut(uint8_t aRouterIndex) const
    {
        int offset = ((aRouterIndex & 1) ? kOddEntryOffset : 0);
        return static_cast<LinkQuality>(
            (mRouteData[aRouterIndex + aRouterIndex / 2] & (kLinkQualityOutMask >> offset)) >>
            (kLinkQualityOutOffset - offset));
    }

    /**
     * Sets the Route Data (Link Quality In/Out and Route Cost) for a given Router index.
     *
     * @param[in]  aRouterIndex    The Router index.
     * @param[in]  aLinkQualityIn  The Link Quality In value.
     * @param[in]  aLinkQualityOut The Link Quality Out value.
     * @param[in]  aRouteCost      The Route Cost value.
     */
    void SetRouteData(uint8_t aRouterIndex, LinkQuality aLinkQualityIn, LinkQuality aLinkQualityOut, uint8_t aRouteCost)
    {
        SetLinkQualityIn(aRouterIndex, aLinkQualityIn);
        SetLinkQualityOut(aRouterIndex, aLinkQualityOut);
        SetRouteCost(aRouterIndex, aRouteCost);
    }

private:
    static constexpr uint8_t kLinkQualityOutOffset = 6;
    static constexpr uint8_t kLinkQualityOutMask   = 3 << kLinkQualityOutOffset;
    static constexpr uint8_t kLinkQualityInOffset  = 4;
    static constexpr uint8_t kLinkQualityInMask    = 3 << kLinkQualityInOffset;
    static constexpr uint8_t kRouteCostOffset      = 0;
    static constexpr uint8_t kRouteCostMask        = 0xf << kRouteCostOffset;
    static constexpr uint8_t kOddEntryOffset       = 4;

    void SetRouteCost(uint8_t aRouterIndex, uint8_t aRouteCost)
    {
        if (aRouterIndex & 1)
        {
            mRouteData[aRouterIndex + aRouterIndex / 2 + 1] = aRouteCost;
        }
        else
        {
            mRouteData[aRouterIndex + aRouterIndex / 2] =
                (mRouteData[aRouterIndex + aRouterIndex / 2] & ~kRouteCostMask) |
                ((aRouteCost >> kOddEntryOffset) & kRouteCostMask);
            mRouteData[aRouterIndex + aRouterIndex / 2 + 1] = static_cast<uint8_t>(
                (mRouteData[aRouterIndex + aRouterIndex / 2 + 1] & ~(kRouteCostMask << kOddEntryOffset)) |
                ((aRouteCost & kRouteCostMask) << kOddEntryOffset));
        }
    }

    void SetLinkQualityIn(uint8_t aRouterIndex, uint8_t aLinkQuality)
    {
        int offset = ((aRouterIndex & 1) ? kOddEntryOffset : 0);
        mRouteData[aRouterIndex + aRouterIndex / 2] =
            (mRouteData[aRouterIndex + aRouterIndex / 2] & ~(kLinkQualityInMask >> offset)) |
            ((aLinkQuality << (kLinkQualityInOffset - offset)) & (kLinkQualityInMask >> offset));
    }

    void SetLinkQualityOut(uint8_t aRouterIndex, LinkQuality aLinkQuality)
    {
        int offset = ((aRouterIndex & 1) ? kOddEntryOffset : 0);
        mRouteData[aRouterIndex + aRouterIndex / 2] =
            (mRouteData[aRouterIndex + aRouterIndex / 2] & ~(kLinkQualityOutMask >> offset)) |
            ((aLinkQuality << (kLinkQualityOutOffset - offset)) & (kLinkQualityOutMask >> offset));
    }

    uint8_t     mRouterIdSequence;
    RouterIdSet mRouterIdMask;
    // Since we do hold 12 (compressible to 11) bits of data per router, each entry occupies 1.5 bytes,
    // consecutively. First 4 bits are link qualities, remaining 8 bits are route cost.
    uint8_t mRouteData[kMaxRouterId + 1 + kMaxRouterId / 2 + 1];
} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

/**
 * Implements Leader Data TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class LeaderDataTlv : public Tlv, public TlvInfo<Tlv::kLeaderData>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void)
    {
        SetType(kLeaderData);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Gets the Leader Data info from TLV.
     *
     * @param[out] aLeaderData   A reference to output Leader Data info.
     */
    void Get(LeaderData &aLeaderData) const
    {
        aLeaderData.SetPartitionId(BigEndian::HostSwap32(mPartitionId));
        aLeaderData.SetWeighting(mWeighting);
        aLeaderData.SetDataVersion(mDataVersion);
        aLeaderData.SetStableDataVersion(mStableDataVersion);
        aLeaderData.SetLeaderRouterId(mLeaderRouterId);
    }

    /**
     * Sets the Leader Data.
     *
     * @param[in] aLeaderData   A Leader Data.
     */
    void Set(const LeaderData &aLeaderData)
    {
        mPartitionId       = BigEndian::HostSwap32(aLeaderData.GetPartitionId());
        mWeighting         = aLeaderData.GetWeighting();
        mDataVersion       = aLeaderData.GetDataVersion(NetworkData::kFullSet);
        mStableDataVersion = aLeaderData.GetDataVersion(NetworkData::kStableSubset);
        mLeaderRouterId    = aLeaderData.GetLeaderRouterId();
    }

private:
    uint32_t mPartitionId;
    uint8_t  mWeighting;
    uint8_t  mDataVersion;
    uint8_t  mStableDataVersion;
    uint8_t  mLeaderRouterId;
} OT_TOOL_PACKED_END;

/**
 * Implements Scan Mask TLV generation and parsing.
 */
class ScanMaskTlv : public UintTlvInfo<Tlv::kScanMask, uint8_t>
{
public:
    static constexpr uint8_t kRouterFlag    = 1 << 7; ///< Scan Mask Router Flag.
    static constexpr uint8_t kEndDeviceFlag = 1 << 6; ///< Scan Mask End Device Flag.

    /**
     * Indicates whether or not the Router flag is set.
     *
     * @param[in] aMask   A scan mask value.
     *
     * @retval TRUE   If the Router flag is set.
     * @retval FALSE  If the Router flag is not set.
     */
    static bool IsRouterFlagSet(uint8_t aMask) { return (aMask & kRouterFlag) != 0; }

    /**
     * Indicates whether or not the End Device flag is set.
     *
     * @param[in] aMask   A scan mask value.
     *
     * @retval TRUE   If the End Device flag is set.
     * @retval FALSE  If the End Device flag is not set.
     */
    static bool IsEndDeviceFlagSet(uint8_t aMask) { return (aMask & kEndDeviceFlag) != 0; }
};

/**
 * Implements Connectivity TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlv : public Tlv, public TlvInfo<Tlv::kConnectivity>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void)
    {
        SetType(kConnectivity);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const
    {
        return IsSedBufferingIncluded() ||
               (GetLength() == sizeof(*this) - sizeof(Tlv) - sizeof(mSedBufferSize) - sizeof(mSedDatagramCount));
    }

    /**
     * Indicates whether or not the sed buffer size and datagram count are included.
     *
     * @retval TRUE   If the sed buffer size and datagram count are included.
     * @retval FALSE  If the sed buffer size and datagram count are not included.
     */
    bool IsSedBufferingIncluded(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Parent Priority value.
     *
     * @returns The Parent Priority value.
     */
    int8_t GetParentPriority(void) const;

    /**
     * Sets the Parent Priority value.
     *
     * @param[in] aParentPriority  The Parent Priority value.
     */
    void SetParentPriority(int8_t aParentPriority);

    /**
     * Returns the Link Quality 3 value.
     *
     * @returns The Link Quality 3 value.
     */
    uint8_t GetLinkQuality3(void) const { return mLinkQuality3; }

    /**
     * Sets the Link Quality 3 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 3 value.
     */
    void SetLinkQuality3(uint8_t aLinkQuality) { mLinkQuality3 = aLinkQuality; }

    /**
     * Returns the Link Quality 2 value.
     *
     * @returns The Link Quality 2 value.
     */
    uint8_t GetLinkQuality2(void) const { return mLinkQuality2; }

    /**
     * Sets the Link Quality 2 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 2 value.
     */
    void SetLinkQuality2(uint8_t aLinkQuality) { mLinkQuality2 = aLinkQuality; }

    /**
     * Sets the Link Quality 1 value.
     *
     * @returns The Link Quality 1 value.
     */
    uint8_t GetLinkQuality1(void) const { return mLinkQuality1; }

    /**
     * Sets the Link Quality 1 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 1 value.
     */
    void SetLinkQuality1(uint8_t aLinkQuality) { mLinkQuality1 = aLinkQuality; }

    /**
     * Increments the Link Quality N field in TLV for a given Link Quality N (1,2,3).
     *
     * The Link Quality N field specifies the number of neighboring router devices with which the sender shares a link
     * of quality N.
     *
     * @param[in] aLinkQuality  The Link Quality N (1,2,3) field to update.
     */
    void IncrementLinkQuality(LinkQuality aLinkQuality);

    /**
     * Sets the Active Routers value.
     *
     * @returns The Active Routers value.
     */
    uint8_t GetActiveRouters(void) const { return mActiveRouters; }

    /**
     * Indicates whether or not the partition is a singleton based on Active Routers value.
     *
     * @retval TRUE   The partition is a singleton.
     * @retval FALSE  The partition is not a singleton.
     */
    bool IsSingleton(void) const { return (mActiveRouters <= 1); }

    /**
     * Sets the Active Routers value.
     *
     * @param[in]  aActiveRouters  The Active Routers value.
     */
    void SetActiveRouters(uint8_t aActiveRouters) { mActiveRouters = aActiveRouters; }

    /**
     * Returns the Leader Cost value.
     *
     * @returns The Leader Cost value.
     */
    uint8_t GetLeaderCost(void) const { return mLeaderCost; }

    /**
     * Sets the Leader Cost value.
     *
     * @param[in]  aCost  The Leader Cost value.
     */
    void SetLeaderCost(uint8_t aCost) { mLeaderCost = aCost; }

    /**
     * Returns the ID Sequence value.
     *
     * @returns The ID Sequence value.
     */
    uint8_t GetIdSequence(void) const { return mIdSequence; }

    /**
     * Sets the ID Sequence value.
     *
     * @param[in]  aSequence  The ID Sequence value.
     */
    void SetIdSequence(uint8_t aSequence) { mIdSequence = aSequence; }

    /**
     * Returns the SED Buffer Size value.
     *
     * @returns The SED Buffer Size value.
     */
    uint16_t GetSedBufferSize(void) const
    {
        uint16_t buffersize = OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE;

        if (IsSedBufferingIncluded())
        {
            buffersize = BigEndian::HostSwap16(mSedBufferSize);
        }
        return buffersize;
    }

    /**
     * Sets the SED Buffer Size value.
     *
     * @param[in]  aSedBufferSize  The SED Buffer Size value.
     */
    void SetSedBufferSize(uint16_t aSedBufferSize) { mSedBufferSize = BigEndian::HostSwap16(aSedBufferSize); }

    /**
     * Returns the SED Datagram Count value.
     *
     * @returns The SED Datagram Count value.
     */
    uint8_t GetSedDatagramCount(void) const
    {
        uint8_t count = OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT;

        if (IsSedBufferingIncluded())
        {
            count = mSedDatagramCount;
        }
        return count;
    }

    /**
     * Sets the SED Datagram Count value.
     *
     * @param[in]  aSedDatagramCount  The SED Datagram Count value.
     */
    void SetSedDatagramCount(uint8_t aSedDatagramCount) { mSedDatagramCount = aSedDatagramCount; }

private:
    static constexpr uint8_t kFlagsParentPriorityOffset = 6;
    static constexpr uint8_t kFlagsParentPriorityMask   = (3 << kFlagsParentPriorityOffset);

    uint8_t  mFlags;
    uint8_t  mLinkQuality3;
    uint8_t  mLinkQuality2;
    uint8_t  mLinkQuality1;
    uint8_t  mLeaderCost;
    uint8_t  mIdSequence;
    uint8_t  mActiveRouters;
    uint16_t mSedBufferSize;
    uint8_t  mSedDatagramCount;
} OT_TOOL_PACKED_END;

/**
 * Specifies Status TLV status values.
 */
struct StatusTlv : public UintTlvInfo<Tlv::kStatus, uint8_t>
{
    /**
     * Status values.
     */
    enum Status : uint8_t
    {
        kError = 1, ///< Error.
    };
};

/**
 * Provides constants and methods for generation and parsing of Address Registration TLV.
 */
class AddressRegistrationTlv : public TlvInfo<Tlv::kAddressRegistration>
{
public:
    /**
     * This constant defines the control byte to use in an uncompressed entry where the full IPv6 address is included in
     * the TLV.
     */
    static constexpr uint8_t kControlByteUncompressed = 0;

    /**
     * Returns the control byte to use in a compressed entry where the 64-prefix is replaced with a
     * 6LoWPAN context identifier.
     *
     * @param[in] aContextId   The 6LoWPAN context ID.
     *
     * @returns The control byte associated with compressed entry with @p aContextId.
     */
    static uint8_t ControlByteFor(uint8_t aContextId) { return kCompressed | (aContextId & kContextIdMask); }

    /**
     * Indicates whether or not an address entry is using compressed format.
     *
     * @param[in] aControlByte  The control byte (the first byte in the entry).
     *
     * @retval TRUE   If the entry uses compressed format.
     * @retval FALSE  If the entry uses uncompressed format.
     */
    static bool IsEntryCompressed(uint8_t aControlByte) { return (aControlByte & kCompressed); }

    /**
     * Gets the context ID in a compressed entry.
     *
     * @param[in] aControlByte  The control byte (the first byte in the entry).
     *
     * @returns The 6LoWPAN context ID.
     */
    static uint8_t GetContextId(uint8_t aControlByte) { return (aControlByte & kContextIdMask); }

    AddressRegistrationTlv(void) = delete;

private:
    static constexpr uint8_t kCompressed    = 1 << 7;
    static constexpr uint8_t kContextIdMask = 0xf;
};

/**
 * Implements Channel TLV value format.
 *
 * This is used by both Channel TLV and CSL Channel TLV.
 */
OT_TOOL_PACKED_BEGIN
class ChannelTlvValue
{
public:
    /**
     * Default constructor.
     */
    ChannelTlvValue(void) = default;

    /**
     * Initializes the `ChannelTlvValue` with a given channel page and channel values.
     *
     * @param[in] aChannelPage   The channel page.
     * @param[in] aChannel       The channel.
     */
    ChannelTlvValue(uint8_t aChannelPage, uint16_t aChannel)
        : mChannelPage(aChannelPage)
        , mChannel(BigEndian::HostSwap16(aChannel))
    {
    }

    /**
     * Initializes the `ChannelTlvValue` with zero channel page and a given channel value.
     *
     * @param[in] aChannel       The channel.
     */
    ChannelTlvValue(uint16_t aChannel)
        : ChannelTlvValue(0, aChannel)
    {
    }

    /**
     * Returns the Channel Page value.
     *
     * @returns The Channel Page value.
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * Sets the Channel Page value.
     *
     * @param[in]  aChannelPage  The Channel Page value.
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * Returns the Channel value.
     *
     * @returns The Channel value.
     */
    uint16_t GetChannel(void) const { return BigEndian::HostSwap16(mChannel); }

    /**
     * Sets the Channel value.
     *
     * @param[in]  aChannel  The Channel value.
     */
    void SetChannel(uint16_t aChannel) { mChannel = BigEndian::HostSwap16(aChannel); }

    /**
     * Sets the Channel and determines and sets the Channel Page from the given channel.
     *
     * @param[in]  aChannel  The Channel value.
     */
    void SetChannelAndPage(uint16_t aChannel);

    /**
     * Indicates whether or not the Channel and Channel Page values are valid.
     *
     * @retval TRUE   If the Channel and Channel Page values are valid.
     * @retval FALSE  If the Channel and Channel Page values are not valid.
     */
    bool IsValid(void) const;

private:
    uint8_t  mChannelPage;
    uint16_t mChannel;
} OT_TOOL_PACKED_END;

/**
 * Defines Channel TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kChannel, ChannelTlvValue> ChannelTlv;

/**
 * Defines CSL Channel TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kCslChannel, ChannelTlvValue> CslChannelTlv;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
/**
 * Defines Time Request TLV constants and types.
 */
typedef TlvInfo<Tlv::kTimeRequest> TimeRequestTlv;

/**
 * Implements Time Parameter TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class TimeParameterTlv : public Tlv, public TlvInfo<Tlv::kTimeParameter>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void)
    {
        SetType(kTimeParameter);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the time sync period.
     *
     * @returns The time sync period.
     */
    uint16_t GetTimeSyncPeriod(void) const { return BigEndian::HostSwap16(mTimeSyncPeriod); }

    /**
     * Sets the time sync period.
     *
     * @param[in]  aTimeSyncPeriod  The time sync period.
     */
    void SetTimeSyncPeriod(uint16_t aTimeSyncPeriod) { mTimeSyncPeriod = BigEndian::HostSwap16(aTimeSyncPeriod); }

    /**
     * Returns the XTAL accuracy threshold.
     *
     * @returns The XTAL accuracy threshold.
     */
    uint16_t GetXtalThreshold(void) const { return BigEndian::HostSwap16(mXtalThreshold); }

    /**
     * Sets the XTAL accuracy threshold.
     *
     * @param[in]  aXTALThreshold  The XTAL accuracy threshold.
     */
    void SetXtalThreshold(uint16_t aXtalThreshold) { mXtalThreshold = BigEndian::HostSwap16(aXtalThreshold); }

private:
    uint16_t mTimeSyncPeriod;
    uint16_t mXtalThreshold;
} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
/**
 * Implements CSL Clock Accuracy TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class CslClockAccuracyTlv : public Tlv, public TlvInfo<Tlv::kCslClockAccuracy>
{
public:
    /**
     * Initializes the TLV.
     */
    void Init(void)
    {
        SetType(kCslClockAccuracy);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the CSL Clock Accuracy value.
     *
     * @returns The CSL Clock Accuracy value.
     */
    uint8_t GetCslClockAccuracy(void) const { return mCslClockAccuracy; }

    /**
     * Sets the CSL Clock Accuracy value.
     *
     * @param[in]  aCslClockAccuracy  The CSL Clock Accuracy value.
     */
    void SetCslClockAccuracy(uint8_t aCslClockAccuracy) { mCslClockAccuracy = aCslClockAccuracy; }

    /**
     * Returns the Clock Uncertainty value.
     *
     * @returns The Clock Uncertainty value.
     */
    uint8_t GetCslUncertainty(void) const { return mCslUncertainty; }

    /**
     * Sets the CSL Uncertainty value.
     *
     * @param[in]  aCslUncertainty  The CSL Uncertainty value.
     */
    void SetCslUncertainty(uint8_t aCslUncertainty) { mCslUncertainty = aCslUncertainty; }

private:
    uint8_t mCslClockAccuracy;
    uint8_t mCslUncertainty;
} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
/**
 * @}
 */

} // namespace Mle

} // namespace ot

#endif // MLE_TLVS_HPP_
