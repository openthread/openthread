/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <string.h>

#include <openthread-types.h>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/ip6_address.hpp>
#include <thread/mle_constants.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

namespace Mle {

/**
 * @addtogroup core-mle-tlvs
 *
 * @brief
 *   This module includes definitions for generating and processing MLE TLVs.
 *
 * @{
 *
 */

/**
 * This class implements MLE TLV generation and parsing.
 *
 */
class Tlv
{
public:
    /**
     * MLE TLV Types.
     *
     */
    enum Type
    {
        kSourceAddress       = 0,    ///< Source Address TLV
        kMode                = 1,    ///< Mode TLV
        kTimeout             = 2,    ///< Timeout TLV
        kChallenge           = 3,    ///< Challenge TLV
        kResponse            = 4,    ///< Response TLV
        kLinkFrameCounter    = 5,    ///< Link-Layer Frame Counter TLV
        kLinkQuality         = 6,    ///< Link Quality TLV
        kNetworkParameter    = 7,    ///< Network Parameter TLV
        kMleFrameCounter     = 8,    ///< MLE Frame Counter TLV
        kRoute               = 9,    ///< Route64 TLV
        kAddress16           = 10,   ///< Address16 TLV
        kLeaderData          = 11,   ///< Leader Data TLV
        kNetworkData         = 12,   ///< Network Data TLV
        kTlvRequest          = 13,   ///< TLV Request TLV
        kScanMask            = 14,   ///< Scan Mask TLV
        kConnectivity        = 15,   ///< Connectivity TLV
        kLinkMargin          = 16,   ///< Link Margin TLV
        kStatus              = 17,   ///< Status TLV
        kVersion             = 18,   ///< Version TLV
        kAddressRegistration = 19,   ///< Address Registration TLV
        kInvalid             = 255,
    };

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(mType); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { mType = static_cast<uint8_t>(aType); }

    /**
     * This method returns the Length value.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * This static method reads the requested TLV out of @p aMessage.
     *
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aType       The Type value to search for.
     * @param[in]   aMaxLength  Maximum number of bytes to read.
     * @param[out]  aTlv        A reference to the TLV that will be copied to.
     *
     * @retval kThreadError_None      Successfully copied the TLV.
     * @retval kThreadError_NotFound  Could not find the TLV with Type @p aType.
     *
     */
    static ThreadError GetTlv(const Message &aMessage, Type aType, uint16_t aMaxLength, Tlv &aTlv);

private:
    uint8_t mType;
    uint8_t mLength;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class SourceAddressTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kSourceAddress); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     *
     */
    uint16_t GetRloc16(void) const { return HostSwap16(mRloc16); }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = HostSwap16(aRloc16); }

private:
    uint16_t mRloc16;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class ModeTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kMode); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    enum
    {
        kModeRxOnWhenIdle      = 1 << 3,
        kModeSecureDataRequest = 1 << 2,
        kModeFFD               = 1 << 1,
        kModeFullNetworkData   = 1 << 0,
    };

    /**
     * This method returns the Mode value.
     *
     * @returns The Mode value.
     *
     */
    uint8_t GetMode(void) const { return mMode; }

    /**
     * This method sets the Mode value.
     *
     * @param[in]  aMode  The Mode value.
     *
     */
    void SetMode(uint8_t aMode) { mMode = aMode; }

private:
    uint8_t mMode;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class TimeoutTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kTimeout); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Timeout value.
     *
     * @returns The Timeout value.
     *
     */
    uint32_t GetTimeout(void) const { return HostSwap32(mTimeout); }

    /**
     * This method sets the Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value.
     *
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = HostSwap32(aTimeout); }

private:
    uint32_t mTimeout;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class ChallengeTlv: public Tlv
{
public:
    enum
    {
        kMaxSize = 8,  ///< Maximum size in bytes (Thread Specification).
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kChallenge); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= 4 && GetLength() <= 8; }

    /**
     * This method returns a pointer to the Challenge value.
     *
     * @returns A pointer to the Challenge value.
     *
     */
    const uint8_t *GetChallenge(void) const { return mChallenge; }

    /**
     * This method sets the Challenge value.
     *
     * @param[in]  aChallenge  A pointer to the Challenge value.
     *
     */
    void SetChallenge(const uint8_t *aChallenge) { memcpy(mChallenge, aChallenge, GetLength()); }

private:
    uint8_t mChallenge[kMaxSize];
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class ResponseTlv: public Tlv
{
public:
    enum
    {
        kMaxSize = 8,  ///< Maximum size in bytes (Thread Specification).
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kResponse); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the Response value.
     *
     * @returns A pointer to the Response value.
     *
     */
    const uint8_t *GetResponse(void) const { return mResponse; }

    /**
     * This method sets the Response value.
     *
     * @param[in]  aResponse  A pointer to the Respones value.
     *
     */
    void SetResponse(const uint8_t *aResponse) { memcpy(mResponse, aResponse, GetLength()); }

private:
    uint8_t mResponse[kMaxSize];
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class LinkFrameCounterTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kLinkFrameCounter); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Frame Counter value.
     *
     * @returns The Frame Counter value.
     *
     */
    uint32_t GetFrameCounter(void) const { return HostSwap32(mFrameCounter); }

    /**
     * This method sets the Frame Counter value.
     *
     * @param[in]  aFrameCounter  The Frame Counter value.
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter) { mFrameCounter = HostSwap32(aFrameCounter); }

private:
    uint32_t mFrameCounter;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class RouteTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kRoute); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const {
        return GetLength() >= sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) &&
               GetLength() <= sizeof(*this) - sizeof(Tlv);
    }

    /**
     * This method returns the Router ID Sequence value.
     *
     * @returns The Router ID Sequence value.
     *
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * This method sets the Router ID Sequence value.
     *
     * @param[in]  aSequence  The Router ID Sequence value.
     *
     */
    void SetRouterIdSequence(uint8_t aSequence) { mRouterIdSequence = aSequence; }

    /**
     * This method clears the Router ID Mask.
     *
     */
    void ClearRouterIdMask(void) { memset(mRouterIdMask, 0, sizeof(mRouterIdMask)); }

    /**
     * This method indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     *
     */
    bool IsRouterIdSet(uint8_t aRouterId) const {
        return (mRouterIdMask[aRouterId / 8] & (0x80 >> (aRouterId % 8))) != 0;
    }

    /**
     * This method sets the Router ID bit.
     *
     * @param[in]  aRouterId  The Router ID bit to set.
     *
     */
    void SetRouterId(uint8_t aRouterId) { mRouterIdMask[aRouterId / 8] |= 0x80 >> (aRouterId % 8); }

    /**
     * This method returns the Route Data Length value.
     *
     * @returns The Route Data Length value.
     *
     */
    uint8_t GetRouteDataLength(void) const {
        return GetLength() - sizeof(mRouterIdSequence) - sizeof(mRouterIdMask);
    }

    /**
     * This method sets the Route Data Length value.
     *
     * @param[in]  aLength  The Route Data Length value.
     *
     */
    void SetRouteDataLength(uint8_t aLength) {
        SetLength(sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) + aLength);
    }

    /**
     * This method returns the Route Cost value for a given Router ID.
     *
     * @returns The Route Cost value for a given Router ID.
     *
     */
    uint8_t GetRouteCost(uint8_t aRouterId) const {
        return mRouteData[aRouterId] & kRouteCostMask;
    }

    /**
     * This method sets the Route Cost value for a given Router ID.
     *
     * @param[in]  aRouterId   The Router ID.
     * @param[in]  aRouteCost  The Route Cost value.
     *
     */
    void SetRouteCost(uint8_t aRouterId, uint8_t aRouteCost) {
        mRouteData[aRouterId] = (mRouteData[aRouterId] & ~kRouteCostMask) | aRouteCost;
    }

    /**
     * This method returns the Link Quality In value for a given Router ID.
     *
     * @returns The Link Quality In value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityIn(uint8_t aRouterId) const {
        return (mRouteData[aRouterId] & kLinkQualityInMask) >> kLinkQualityInOffset;
    }

    /**
     * This method sets the Link Quality In value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality In value for a given Router ID.
     *
     */
    void SetLinkQualityIn(uint8_t aRouterId, uint8_t aLinkQuality) {
        mRouteData[aRouterId] =
            (mRouteData[aRouterId] & ~kLinkQualityInMask) | (aLinkQuality << kLinkQualityInOffset);
    }

    /**
     * This method returns the Link Quality Out value for a given Router ID.
     *
     * @returns The Link Quality Out value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityOut(uint8_t aRouterId) const {
        return (mRouteData[aRouterId] & kLinkQualityOutMask) >> kLinkQualityOutOffset;
    }

    /**
     * This method sets the Link Quality Out value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality Out value for a given Router ID.
     *
     */
    void SetLinkQualityOut(uint8_t aRouterId, uint8_t aLinkQuality) {
        mRouteData[aRouterId] =
            (mRouteData[aRouterId] & ~kLinkQualityOutMask) | (aLinkQuality << kLinkQualityOutOffset);
    }

private:
    enum
    {
        kLinkQualityOutOffset = 6,
        kLinkQualityOutMask = 3 << kLinkQualityOutOffset,
        kLinkQualityInOffset = 4,
        kLinkQualityInMask = 3 << kLinkQualityInOffset,
        kRouteCostOffset = 0,
        kRouteCostMask = 0xf << kRouteCostOffset,
    };
    uint8_t mRouterIdSequence;
    uint8_t mRouterIdMask[BitVectorBytes(kMaxRouterId)];
    uint8_t mRouteData[kMaxRouters];
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class MleFrameCounterTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kMleFrameCounter); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Frame Counter value.
     *
     * @returns The Frame Counter value.
     *
     */
    uint32_t GetFrameCounter(void) const { return HostSwap32(mFrameCounter); }

    /**
     * This method sets the Frame Counter value.
     *
     * @param[in]  aFrameCounter  The Frame Counter value.
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter) { mFrameCounter = HostSwap32(aFrameCounter); }

private:
    uint32_t mFrameCounter;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class Address16Tlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kAddress16); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     *
     */
    uint16_t GetRloc16(void) const { return HostSwap16(mRloc16); }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = HostSwap16(aRloc16); }

private:
    uint16_t mRloc16;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class LeaderDataTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kLeaderData); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Partition ID value.
     *
     * @returns The Partition ID value.
     *
     */
    uint32_t GetPartitionId(void) const { return HostSwap32(mPartitionId); }

    /**
     * This method sets the Partition ID value.
     *
     * @param[in]  aPartitionId  The Partition ID value.
     *
     */
    void SetPartitionId(uint32_t aPartitionId) { mPartitionId = HostSwap32(aPartitionId); }

    /**
     * This method returns the Weighting value.
     *
     * @returns The Weighting value.
     *
     */
    uint8_t GetWeighting(void) const { return mWeighting; }

    /**
     * This method sets the Weighting value.
     *
     * @param[in]  aWeighting  The Weighting value.
     *
     */
    void SetWeighting(uint8_t aWeighting) { mWeighting = aWeighting; }

    /**
     * This method returns the Data Version value.
     *
     * @returns The Data Version value.
     *
     */
    uint8_t GetDataVersion(void) const { return mDataVersion; }

    /**
     * This method sets the Data Version value.
     *
     * @param[in]  aVersion  The Data Version value.
     *
     */
    void SetDataVersion(uint8_t aVersion)  { mDataVersion = aVersion; }

    /**
     * This method returns the Stable Data Version value.
     *
     * @returns The Stable Data Version value.
     *
     */
    uint8_t GetStableDataVersion(void) const { return mStableDataVersion; }

    /**
     * This method sets the Stable Data Version value.
     *
     * @param[in]  aVersion  The Stable Data Version value.
     *
     */
    void SetStableDataVersion(uint8_t aVersion) { mStableDataVersion = aVersion; }

    /**
     * This method returns the Leader Router ID value.
     *
     * @returns The Leader Router ID value.
     *
     */
    uint8_t GetLeaderRouterId(void) const { return mLeaderRouterId; }

    /**
     * This method sets the Leader Router ID value.
     *
     * @param[in]  aRouterId  The Leader Router ID value.
     *
     */
    void SetLeaderRouterId(uint8_t aRouterId) { mLeaderRouterId = aRouterId; }

private:
    uint32_t mPartitionId;
    uint8_t mWeighting;
    uint8_t mDataVersion;
    uint8_t mStableDataVersion;
    uint8_t mLeaderRouterId;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class NetworkDataTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kNetworkData); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the Network Data.
     *
     * @returns A pointer to the Network Data.
     *
     */
    uint8_t *GetNetworkData(void) { return mNetworkData; }

    /**
     * This method sets the Network Data.
     *
     * @param[in]  aNetworkData  A pointer to the Network Data.
     *
     */
    void SetNetworkData(const uint8_t *aNetworkData) { memcpy(mNetworkData, aNetworkData, GetLength()); }

private:
    uint8_t mNetworkData[255];
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class TlvRequestTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kTlvRequest); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the TLV list.
     *
     * @returns A pointer to the TLV list.
     *
     */
    const uint8_t *GetTlvs(void) const { return mTlvs; }

    /**
     * This method sets the list of TLVs.
     *
     * @param[in]  aTlvs  A pointer to the TLV list.
     *
     */
    void SetTlvs(const uint8_t *aTlvs) { memcpy(mTlvs, aTlvs, GetLength()); }

private:
    enum
    {
        kMaxTlvs = 8,
    };
    uint8_t mTlvs[kMaxTlvs];
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class ScanMaskTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kScanMask); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    enum
    {
        kRouterFlag = 1 << 7,
        kEndDeviceFlag = 1 << 6,
    };

    /**
     * This method clears Router flag.
     *
     */
    void ClearRouterFlag(void) { mMask &= ~kRouterFlag; }

    /**
     * This method sets the Router flag.
     *
     */
    void SetRouterFlag(void) { mMask |= kRouterFlag; }

    /**
     * This method indicates whether or not the Router flag is set.
     *
     * @retval TRUE   If the Router flag is set.
     * @retval FALSE  If the Router flag is not set.
     */
    bool IsRouterFlagSet(void) { return (mMask & kRouterFlag) != 0; }

    /**
     * This method clears the End Device flag.
     *
     */
    void ClearEndDeviceFlag(void) { mMask &= ~kEndDeviceFlag; }

    /**
     * This method sets the End Device flag.
     *
     */
    void SetEndDeviceFlag(void) { mMask |= kEndDeviceFlag; }

    /**
     * This method indicates whether or not the End Device flag is set.
     *
     * @retval TRUE   If the End Device flag is set.
     * @retval FALSE  If the End Device flag is not set.
     */
    bool IsEndDeviceFlagSet(void) { return (mMask & kEndDeviceFlag) != 0; }

    /**
     * This method sets the Mask byte value.
     *
     * @param[in]  aMask  The Mask byte value.
     *
     */
    void SetMask(uint8_t aMask) { mMask = aMask; }

private:
    uint8_t mMask;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class ConnectivityTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kConnectivity); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Max Child Count value.
     *
     * @returns The Max Child Count value.
     *
     */
    uint8_t GetMaxChildCount(void) const { return mMaxChildCount; }

    /**
     * This method sets the Max Child Count value.
     *
     * @param[in]  aCount  The Max Child Count value.
     *
     */
    void SetMaxChildCount(uint8_t aCount) { mMaxChildCount = aCount; }

    /**
     * This method returns the Child Count value.
     *
     * @returns The Child Count value.
     *
     */
    uint8_t GetChildCount(void) const { return mChildCount; }

    /**
     * This method sets the Child Count value.
     *
     * @param[in]  aCount  The Child Count value.
     *
     */
    void SetChildCount(uint8_t aCount) { mChildCount = aCount; }

    /**
     * This method returns the Link Quality 3 value.
     *
     * @returns The Link Quality 3 value.
     *
     */
    uint8_t GetLinkQuality3(void) const { return mLinkQuality3; }

    /**
     * This method sets the Link Quality 3 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 3 value.
     *
     */
    void SetLinkQuality3(uint8_t aLinkQuality) { mLinkQuality3 = aLinkQuality; }

    /**
     * This method returns the Link Quality 2 value.
     *
     * @returns The Link Quality 2 value.
     *
     */
    uint8_t GetLinkQuality2(void) const { return mLinkQuality2; }

    /**
     * This method sets the Link Quality 2 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 2 value.
     *
     */
    void SetLinkQuality2(uint8_t aLinkQuality) { mLinkQuality2 = aLinkQuality; }

    /**
     * This method sets the Link Quality 1 value.
     *
     * @returns The Link Quality 1 value.
     *
     */
    uint8_t GetLinkQuality1(void) const { return mLinkQuality1; }

    /**
     * This method sets the Link Quality 1 value.
     *
     * @param[in]  aLinkQuality  The Link Quality 1 value.
     *
     */
    void SetLinkQuality1(uint8_t aLinkQuality) { mLinkQuality1 = aLinkQuality; }

    /**
     * This method returns the Leader Cost value.
     *
     * @returns The Leader Cost value.
     *
     */
    uint8_t GetLeaderCost(void) const { return mLeaderCost; }

    /**
     * This method sets the Leader Cost value.
     *
     * @param[in]  aCost  The Leader Cost value.
     *
     */
    void SetLeaderCost(uint8_t aCost) { mLeaderCost = aCost; }

    /**
     * This method returns the ID Sequence value.
     *
     * @returns The ID Sequence value.
     *
     */
    uint8_t GetIdSequence(void) const { return mIdSequence; }

    /**
     * This method sets the ID Sequence value.
     *
     * @param[in]  aSequence  The ID Sequence value.
     *
     */
    void SetIdSequence(uint8_t aSequence) { mIdSequence = aSequence; }

private:
    uint8_t mMaxChildCount;
    uint8_t mChildCount;
    uint8_t mLinkQuality3;
    uint8_t mLinkQuality2;
    uint8_t mLinkQuality1;
    uint8_t mLeaderCost;
    uint8_t mIdSequence;
}  __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class LinkMarginTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kLinkMargin); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Link Margin value.
     *
     * @returns The Link Margin value.
     *
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * This method sets the Link Margin value.
     *
     * @param[in]  aLinkMargin  The Link Margin value.
     *
     */
    void SetLinkMargin(uint8_t aLinkMargin) { mLinkMargin = aLinkMargin; }

private:
    uint8_t mLinkMargin;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class StatusTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kStatus); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * Status values.
     */
    enum Status
    {
        kError = 1,   ///< Error.
    };

    /**
     * This method returns the Status value.
     *
     * @returns The Status value.
     *
     */
    Status GetStatus(void) const { return static_cast<Status>(mStatus); }

    /**
     * This method sets the Status value.
     *
     * @param[in]  aStatus  The Status value.
     *
     */
    void SetStatus(Status aStatus) { mStatus = static_cast<uint8_t>(aStatus); }

private:
    uint8_t mStatus;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class VersionTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kVersion); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint16_t GetVersion(void) const { return HostSwap16(mVersion); }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint16_t aVersion) { mVersion = HostSwap16(aVersion); }

private:
    uint16_t mVersion;
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class AddressRegistrationEntry
{
public:
    /**
     * This method returns the IPv6 address or IID length.
     *
     * @returns The IPv6 address length if the Compressed bit is clear, or the IID length if the Compressed bit is
     *          set.
     *
     */
    uint8_t GetLength(void) const { return sizeof(mControl) + (IsCompressed() ? sizeof(mIid) : sizeof(mIp6Address)); }

    /**
     * This method indicates whether or not the Compressed flag is set.
     *
     * @retval TRUE   If the Compressed flag is set.
     * @retval FALSE  If the Compressed flag is not set.
     *
     */
    bool IsCompressed(void) const { return mControl & kCompressed; }

    /**
     * This method sets the Uncompressed flag.
     *
     */
    void SetUncompressed(void) { mControl = 0; }

    /**
     * This method returns the Context ID for the compressed form.
     *
     * @returns The Context ID value.
     *
     */
    uint8_t GetContextId(void) const { return mControl & kCidMask; }

    /**
     * This method sets the Context ID value.
     *
     * @param[in]  aContextId  The Context ID value.
     *
     */
    void SetContextId(uint8_t aContextId) { mControl = kCompressed | aContextId; }

    /**
     * This method returns a pointer to the IID value.
     *
     * @returns A pointer to the IID value.
     *
     */
    const uint8_t *GetIid(void) const { return mIid; }

    /**
     * This method sets the IID value.
     *
     * @param[in]  aIid  A pointer to the IID value.
     *
     */
    void SetIid(const uint8_t *aIid) { memcpy(mIid, aIid, sizeof(mIid)); }

    /**
     * This method returns a pointer to the IPv6 Address value.
     *
     * @returns A pointer to the IPv6 Address value.
     *
     */
    const Ip6::Address *GetIp6Address(void) const { return &mIp6Address; }

    /**
     * This method sets the IPv6 Address value.
     *
     * @param[in]  aAddress  A reference to the IPv6 Address value.
     *
     */
    void SetIp6Address(const Ip6::Address &aAddress) { mIp6Address = aAddress; }

private:
    enum
    {
        kCompressed = 1 << 7,
        kCidMask = 0xf,
    };

    uint8_t mControl;
    union
    {
        uint8_t mIid[Ip6::Address::kInterfaceIdentifierSize];
        Ip6::Address mIp6Address;
    } __attribute__((packed));
} __attribute__((packed));

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
class AddressRegistrationTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kAddressRegistration); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the i'th Address Entry.
     *
     * @param[in]  aIndex  The index.
     *
     * @returns A pointer to the i'th Address Entry.
     *
     */
    const AddressRegistrationEntry *GetAddressEntry(uint8_t aIndex) const {
        const AddressRegistrationEntry *entry = NULL;
        const uint8_t *cur = reinterpret_cast<const uint8_t *>(mAddresses);
        const uint8_t *end = cur + GetLength();

        while (cur < end) {
            entry = reinterpret_cast<const AddressRegistrationEntry *>(cur);

            if (aIndex == 0) {
                break;
            }

            cur += entry->GetLength();
            aIndex--;
        }

        return (cur < end) ? entry : NULL;
    }

private:
    AddressRegistrationEntry mAddresses[4];
} __attribute__((packed));

/**
 * @}
 *
 */

}  // namespace Mle


}  // namespace Thread

#endif  // MLE_TLVS_HPP_
