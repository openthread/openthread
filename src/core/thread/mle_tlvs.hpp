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

#include "utils/wrap_string.h"

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "meshcop/timestamp.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_constants.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Mle {

#define TLVREQUESTTLV_ITERATOR_INIT 0 ///< Initializer for TlvRequestTlvIterator.

typedef uint8_t TlvRequestIterator; ///< Used to iterate through TlvRequestTlv.

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
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * MLE TLV Types.
     *
     */
    enum Type
    {
        kSourceAddress       = 0,  ///< Source Address TLV
        kMode                = 1,  ///< Mode TLV
        kTimeout             = 2,  ///< Timeout TLV
        kChallenge           = 3,  ///< Challenge TLV
        kResponse            = 4,  ///< Response TLV
        kLinkFrameCounter    = 5,  ///< Link-Layer Frame Counter TLV
        kLinkQuality         = 6,  ///< Link Quality TLV
        kNetworkParameter    = 7,  ///< Network Parameter TLV
        kMleFrameCounter     = 8,  ///< MLE Frame Counter TLV
        kRoute               = 9,  ///< Route64 TLV
        kAddress16           = 10, ///< Address16 TLV
        kLeaderData          = 11, ///< Leader Data TLV
        kNetworkData         = 12, ///< Network Data TLV
        kTlvRequest          = 13, ///< TLV Request TLV
        kScanMask            = 14, ///< Scan Mask TLV
        kConnectivity        = 15, ///< Connectivity TLV
        kLinkMargin          = 16, ///< Link Margin TLV
        kStatus              = 17, ///< Status TLV
        kVersion             = 18, ///< Version TLV
        kAddressRegistration = 19, ///< Address Registration TLV
        kChannel             = 20, ///< Channel TLV
        kPanId               = 21, ///< PAN ID TLV
        kActiveTimestamp     = 22, ///< Active Timestamp TLV
        kPendingTimestamp    = 23, ///< Pending Timestamp TLV
        kActiveDataset       = 24, ///< Active Operational Dataset TLV
        kPendingDataset      = 25, ///< Pending Operational Dataset TLV
        kDiscovery           = 26, ///< Thread Discovery TLV

        /**
         * Applicable/Required only when time synchronization service
         * (`OPENTHREAD_CONFIG_ENABLE_TIME_SYNC`) is enabled.
         *
         */
        kTimeRequest   = 252, ///< Time Request TLV
        kTimeParameter = 253, ///< Time Parameter TLV
        kXtalAccuracy  = 254, ///< XTAL Accuracy TLV

        kInvalid = 255,
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
     * This static method obtains the offset of a TLV within @p aMessage.
     *
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aType       The Type value to search for.
     * @param[out]  aOffset     A reference to the offset of the TLV.
     *
     * @retval OT_ERROR_NONE       Successfully copied the TLV.
     * @retval OT_ERROR_NOT_FOUND  Could not find the TLV with Type @p aType.
     *
     */
    static otError GetOffset(const Message &aMessage, Type aType, uint16_t &aOffset)
    {
        return ot::Tlv::GetOffset(aMessage, static_cast<uint8_t>(aType), aOffset);
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class SourceAddressTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kSourceAddress);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ModeTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMode);
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

    enum
    {
        kModeRxOnWhenIdle      = 1 << 3,
        kModeSecureDataRequest = 1 << 2,
        kModeFullThreadDevice  = 1 << 1,
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TimeoutTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTimeout);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChallengeTlv : public Tlv
{
public:
    enum
    {
        kMaxSize = 8, ///< Maximum size in bytes (Thread Specification).
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChallenge);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ResponseTlv : public Tlv
{
public:
    enum
    {
        kMaxSize = 8, ///< Maximum size in bytes (Thread Specification).
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kResponse);
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
     * This method returns a pointer to the Response value.
     *
     * @returns A pointer to the Response value.
     *
     */
    const uint8_t *GetResponse(void) const { return mResponse; }

    /**
     * This method sets the Response value.
     *
     * @param[in]  aResponse  A pointer to the Response value.
     *
     */
    void SetResponse(const uint8_t *aResponse) { memcpy(mResponse, aResponse, GetLength()); }

private:
    uint8_t mResponse[kMaxSize];
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkFrameCounterTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkFrameCounter);
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
} OT_TOOL_PACKED_END;

#if !OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kRoute);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
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
    bool IsRouterIdSet(uint8_t aRouterId) const
    {
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
    uint8_t GetRouteDataLength(void) const { return GetLength() - sizeof(mRouterIdSequence) - sizeof(mRouterIdMask); }

    /**
     * This method sets the Route Data Length value.
     *
     * @param[in]  aLength  The Route Data Length value.
     *
     */
    void SetRouteDataLength(uint8_t aLength) { SetLength(sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) + aLength); }

    /**
     * This method returns the Route Cost value for a given Router ID.
     *
     * @returns The Route Cost value for a given Router ID.
     *
     */
    uint8_t GetRouteCost(uint8_t aRouterId) const { return mRouteData[aRouterId] & kRouteCostMask; }

    /**
     * This method sets the Route Cost value for a given Router ID.
     *
     * @param[in]  aRouterId   The Router ID.
     * @param[in]  aRouteCost  The Route Cost value.
     *
     */
    void SetRouteCost(uint8_t aRouterId, uint8_t aRouteCost)
    {
        mRouteData[aRouterId] = (mRouteData[aRouterId] & ~kRouteCostMask) | aRouteCost;
    }

    /**
     * This method returns the Link Quality In value for a given Router ID.
     *
     * @returns The Link Quality In value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityIn(uint8_t aRouterId) const
    {
        return (mRouteData[aRouterId] & kLinkQualityInMask) >> kLinkQualityInOffset;
    }

    /**
     * This method sets the Link Quality In value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality In value for a given Router ID.
     *
     */
    void SetLinkQualityIn(uint8_t aRouterId, uint8_t aLinkQuality)
    {
        mRouteData[aRouterId] = (mRouteData[aRouterId] & ~kLinkQualityInMask) |
                                ((aLinkQuality << kLinkQualityInOffset) & kLinkQualityInMask);
    }

    /**
     * This method returns the Link Quality Out value for a given Router ID.
     *
     * @returns The Link Quality Out value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityOut(uint8_t aRouterId) const
    {
        return (mRouteData[aRouterId] & kLinkQualityOutMask) >> kLinkQualityOutOffset;
    }

    /**
     * This method sets the Link Quality Out value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality Out value for a given Router ID.
     *
     */
    void SetLinkQualityOut(uint8_t aRouterId, uint8_t aLinkQuality)
    {
        mRouteData[aRouterId] = (mRouteData[aRouterId] & ~kLinkQualityOutMask) |
                                ((aLinkQuality << kLinkQualityOutOffset) & kLinkQualityOutMask);
    }

private:
    enum
    {
        kLinkQualityOutOffset = 6,
        kLinkQualityOutMask   = 3 << kLinkQualityOutOffset,
        kLinkQualityInOffset  = 4,
        kLinkQualityInMask    = 3 << kLinkQualityInOffset,
        kRouteCostOffset      = 0,
        kRouteCostMask        = 0xf << kRouteCostOffset,
    };
    uint8_t mRouterIdSequence;
    uint8_t mRouterIdMask[BitVectorBytes(kMaxRouterId + 1)];
    uint8_t mRouteData[kMaxRouterId + 1];
} OT_TOOL_PACKED_END;

#else // OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kRoute);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
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
    bool IsRouterIdSet(uint8_t aRouterId) const
    {
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
     * @returns The Route Data Length value in bytes
     *
     */
    uint8_t GetRouteDataLength(void) const { return GetLength() - sizeof(mRouterIdSequence) - sizeof(mRouterIdMask); }

    /**
     * This method sets the Route Data Length value.
     *
     * @param[in]  aLength  The Route Data Length value in number of router entries
     *
     */
    void SetRouteDataLength(uint8_t aLength)
    {
        SetLength(sizeof(mRouterIdSequence) + sizeof(mRouterIdMask) + aLength + (aLength + 1) / 2);
    }

    /**
     * This method returns the Route Cost value for a given Router ID.
     *
     * @returns The Route Cost value for a given Router ID.
     *
     */
    uint8_t GetRouteCost(uint8_t aRouterId) const
    {
        if (aRouterId & 1)
        {
            return mRouteData[aRouterId + aRouterId / 2 + 1];
        }
        else
        {
            return static_cast<uint8_t>((mRouteData[aRouterId + aRouterId / 2] & kRouteCostMask) << kOddEntryOffset) |
                   ((mRouteData[aRouterId + aRouterId / 2 + 1] &
                     static_cast<uint8_t>(kRouteCostMask << kOddEntryOffset)) >>
                    kOddEntryOffset);
        }
    }

    /**
     * This method sets the Route Cost value for a given Router ID.
     *
     * @param[in]  aRouterId   The Router ID.
     * @param[in]  aRouteCost  The Route Cost value.
     *
     */
    void SetRouteCost(uint8_t aRouterId, uint8_t aRouteCost)
    {
        if (aRouterId & 1)
        {
            mRouteData[aRouterId + aRouterId / 2 + 1] = aRouteCost;
        }
        else
        {
            mRouteData[aRouterId + aRouterId / 2] = (mRouteData[aRouterId + aRouterId / 2] & ~kRouteCostMask) |
                                                    ((aRouteCost >> kOddEntryOffset) & kRouteCostMask);
            mRouteData[aRouterId + aRouterId / 2 + 1] = static_cast<uint8_t>(
                (mRouteData[aRouterId + aRouterId / 2 + 1] & ~(kRouteCostMask << kOddEntryOffset)) |
                ((aRouteCost & kRouteCostMask) << kOddEntryOffset));
        }
    }

    /**
     * This method returns the Link Quality In value for a given Router ID.
     *
     * @returns The Link Quality In value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityIn(uint8_t aRouterId) const
    {
        int offset = ((aRouterId & 1) ? kOddEntryOffset : 0);
        return (mRouteData[aRouterId + aRouterId / 2] & (kLinkQualityInMask >> offset)) >>
               (kLinkQualityInOffset - offset);
    }

    /**
     * This method sets the Link Quality In value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality In value for a given Router ID.
     *
     */
    void SetLinkQualityIn(uint8_t aRouterId, uint8_t aLinkQuality)
    {
        int offset = ((aRouterId & 1) ? kOddEntryOffset : 0);
        mRouteData[aRouterId + aRouterId / 2] =
            (mRouteData[aRouterId + aRouterId / 2] & ~(kLinkQualityInMask >> offset)) |
            ((aLinkQuality << (kLinkQualityInOffset - offset)) & (kLinkQualityInMask >> offset));
    }

    /**
     * This method returns the Link Quality Out value for a given Router ID.
     *
     * @returns The Link Quality Out value for a given Router ID.
     *
     */
    uint8_t GetLinkQualityOut(uint8_t aRouterId) const
    {
        int offset = ((aRouterId & 1) ? kOddEntryOffset : 0);
        return (mRouteData[aRouterId + aRouterId / 2] & (kLinkQualityOutMask >> offset)) >>
               (kLinkQualityOutOffset - offset);
    }

    /**
     * This method sets the Link Quality Out value for a given Router ID.
     *
     * @param[in]  aRouterId     The Router ID.
     * @param[in]  aLinkQuality  The Link Quality Out value for a given Router ID.
     *
     */
    void SetLinkQualityOut(uint8_t aRouterId, uint8_t aLinkQuality)
    {
        int offset = ((aRouterId & 1) ? kOddEntryOffset : 0);
        mRouteData[aRouterId + aRouterId / 2] =
            (mRouteData[aRouterId + aRouterId / 2] & ~(kLinkQualityOutMask >> offset)) |
            ((aLinkQuality << (kLinkQualityOutOffset - offset)) & (kLinkQualityOutMask >> offset));
    }

private:
    enum
    {
        kLinkQualityOutOffset = 6,
        kLinkQualityOutMask   = 3 << kLinkQualityOutOffset,
        kLinkQualityInOffset  = 4,
        kLinkQualityInMask    = 3 << kLinkQualityInOffset,
        kRouteCostOffset      = 0,
        kRouteCostMask        = 0xf << kRouteCostOffset,
        kOddEntryOffset       = 4,
    };
    uint8_t mRouterIdSequence;
    uint8_t mRouterIdMask[BitVectorBytes(kMaxRouterId + 1)];
    // Since we do hold 12 (compressible to 11) bits of data per router, each entry occupies 1.5 bytes, consecutively.
    // First 4 bits are link qualities, remaining 8 bits are route cost.
    uint8_t mRouteData[kMaxRouterId + 1 + kMaxRouterId / 2 + 1];
} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MleFrameCounterTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMleFrameCounter);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Address16Tlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kAddress16);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LeaderDataTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLeaderData);
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
    void SetDataVersion(uint8_t aVersion) { mDataVersion = aVersion; }

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
    uint8_t  mWeighting;
    uint8_t  mDataVersion;
    uint8_t  mStableDataVersion;
    uint8_t  mLeaderRouterId;
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkDataTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kNetworkData);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TlvRequestTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTlvRequest);
        SetLength(sizeof(*this) - sizeof(Tlv));
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
     * This method returns a pointer to the TLV list.
     *
     * @returns A pointer to the TLV list.
     *
     */
    const uint8_t *GetTlvs(void) const { return mTlvs; }

    /**
     * This method provides the next Tlv in the TlvRequestTlv.
     *
     * @retval OT_ERROR_NONE        Successfully found the next Tlv.
     * @retval OT_ERROR_NOT_FOUND   No subsequent Tlv exists in TlvRequestTlv.
     *
     */
    otError GetNextTlv(TlvRequestIterator &aIterator, uint8_t &aTlv)
    {
        otError error = OT_ERROR_NOT_FOUND;

        if (aIterator < GetLength())
        {
            aTlv      = mTlvs[aIterator];
            aIterator = static_cast<TlvRequestIterator>(aIterator + sizeof(uint8_t));
            error     = OT_ERROR_NONE;
        }

        return error;
    }

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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ScanMaskTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kScanMask);
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

    enum
    {
        kRouterFlag    = 1 << 7,
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kConnectivity);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
        return IsSedBufferingIncluded() ||
               (GetLength() == sizeof(*this) - sizeof(Tlv) - sizeof(mSedBufferSize) - sizeof(mSedDatagramCount));
    }

    /**
     * This method indicates whether or not the sed buffer size and datagram count are included.
     *
     * @retval TRUE   If the sed buffer size and datagram count are included.
     * @retval FALSE  If the sed buffer size and datagram count are not included.
     *
     */
    bool IsSedBufferingIncluded(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Parent Priority value.
     *
     * @returns The Parent Priority value.
     *
     */
    int8_t GetParentPriority(void) const
    {
        return (static_cast<int8_t>(mParentPriority & kParentPriorityMask)) >> kParentPriorityOffset;
    }

    /**
     * This method sets the Parent Priority value.
     *
     * @param[in] aParentPriority  The Parent Priority value.
     *
     */
    void SetParentPriority(int8_t aParentPriority)
    {
        mParentPriority = (aParentPriority << kParentPriorityOffset) & kParentPriorityMask;
    }

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
     * This method sets the Active Routers value.
     *
     * @returns The Active Routers value.
     *
     */
    uint8_t GetActiveRouters(void) const { return mActiveRouters; }

    /**
     * This method sets the Active Routers value.
     *
     * @param[in]  aActiveRouters  The Active Routers value.
     *
     */
    void SetActiveRouters(uint8_t aActiveRouters) { mActiveRouters = aActiveRouters; }

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

    /**
     * This method returns the SED Buffer Size value.
     *
     * @returns The SED Buffer Size value.
     *
     */
    uint16_t GetSedBufferSize(void) const
    {
        uint16_t buffersize = OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE;

        if (IsSedBufferingIncluded())
        {
            buffersize = HostSwap16(mSedBufferSize);
        }
        return buffersize;
    }

    /**
     * This method sets the SED Buffer Size value.
     *
     * @param[in]  aSedBufferSize  The SED Buffer Size value.
     *
     */
    void SetSedBufferSize(uint16_t aSedBufferSize) { mSedBufferSize = HostSwap16(aSedBufferSize); }

    /**
     * This method returns the SED Datagram Count value.
     *
     * @returns The SED Datagram Count value.
     *
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
     * This method sets the SED Datagram Count value.
     *
     * @param[in]  aSedDatagramCount  The SED Datagram Count value.
     *
     */
    void SetSedDatagramCount(uint8_t aSedDatagramCount) { mSedDatagramCount = aSedDatagramCount; }

private:
    enum
    {
        kParentPriorityOffset = 6,
        kParentPriorityMask   = 3 << kParentPriorityOffset,
    };

    uint8_t  mParentPriority;
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
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LinkMarginTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLinkMargin);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class StatusTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kStatus);
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
     * Status values.
     */
    enum Status
    {
        kError = 1, ///< Error.
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class VersionTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kVersion);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Source Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
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
    bool IsCompressed(void) const { return (mControl & kCompressed) != 0; }

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
        kCidMask    = 0xf,
    };

    uint8_t mControl;
    union
    {
        uint8_t      mIid[Ip6::Address::kInterfaceIdentifierSize];
        Ip6::Address mIp6Address;
    } OT_TOOL_PACKED_FIELD;
} OT_TOOL_PACKED_END;

/**
 * This class implements Channel TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannel);
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
     * This method returns the Channel Page value.
     *
     * @returns The Channel Page value.
     *
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * This method sets the Channel Page value.
     *
     * @param[in]  aChannelPage  The Channel Page value.
     *
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * This method returns the Channel value.
     *
     * @returns The Channel value.
     *
     */
    uint16_t GetChannel(void) const { return HostSwap16(mChannel); }

    /**
     * This method sets the Channel value.
     *
     * @param[in]  aChannel  The Channel value.
     *
     */
    void SetChannel(uint16_t aChannel) { mChannel = HostSwap16(aChannel); }

private:
    uint8_t  mChannelPage;
    uint16_t mChannel;
} OT_TOOL_PACKED_END;

/**
 * This class implements PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PanIdTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kPanId);
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
     * This method returns the PAN ID value.
     *
     * @returns The PAN ID value.
     *
     */
    uint16_t GetPanId(void) const { return HostSwap16(mPanId); }

    /**
     * This method sets the PAN ID value.
     *
     * @param[in]  aPanId  The PAN ID value.
     *
     */
    void SetPanId(uint16_t aPanId) { mPanId = HostSwap16(aPanId); }

private:
    uint16_t mPanId;
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
/**
 * This class implements Time Request TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TimeRequestTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTimeRequest);
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
} OT_TOOL_PACKED_END;

/**
 * This class implements Time Parameter TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TimeParameterTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTimeParameter);
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
     * This method returns the time sync period.
     *
     * @returns The time sync period.
     *
     */
    uint16_t GetTimeSyncPeriod(void) const { return HostSwap16(mTimeSyncPeriod); }

    /**
     * This method sets the time sync period.
     *
     * @param[in]  aTimeSyncPeriod  The time sync period.
     *
     */
    void SetTimeSyncPeriod(uint16_t aTimeSyncPeriod) { mTimeSyncPeriod = HostSwap16(aTimeSyncPeriod); }

    /**
     * This method returns the XTAL accuracy threshold.
     *
     * @returns The XTAL accuracy threshold.
     *
     */
    uint16_t GetXtalThreshold(void) const { return HostSwap16(mXtalThreshold); }

    /**
     * This method sets the XTAL accuracy threshold.
     *
     * @param[in]  aXTALThreshold  The XTAL accuracy threshold.
     *
     */
    void SetXtalThreshold(uint16_t aXtalThreshold) { mXtalThreshold = HostSwap16(aXtalThreshold); }

private:
    uint16_t mTimeSyncPeriod;
    uint16_t mXtalThreshold;
} OT_TOOL_PACKED_END;

/**
 * This class implements XTAL Accuracy TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class XtalAccuracyTlv : public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kXtalAccuracy);
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
     * This method returns the XTAL accuracy.
     *
     * @returns The XTAL accuracy.
     *
     */
    uint16_t GetXtalAccuracy(void) const { return HostSwap16(mXtalAccuracy); }

    /**
     * This method sets the XTAL accuracy.
     *
     * @param[in]  aXTALAccuracy  The XTAL accuracy.
     *
     */
    void SetXtalAccuracy(uint16_t aXtalAccuracy) { mXtalAccuracy = HostSwap16(aXtalAccuracy); }

private:
    uint16_t mXtalAccuracy;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

/**
 * This class implements Active Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ActiveTimestampTlv : public Tlv, public MeshCoP::Timestamp
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(Mle::Tlv::kActiveTimestamp);
        SetLength(sizeof(*this) - sizeof(Tlv));
        Timestamp::Init();
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }
} OT_TOOL_PACKED_END;

/**
 * This class implements Pending Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PendingTimestampTlv : public Tlv, public MeshCoP::Timestamp
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(Mle::Tlv::kPendingTimestamp);
        SetLength(sizeof(*this) - sizeof(Tlv));
        Timestamp::Init();
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Mle

} // namespace ot

#endif // MLE_TLVS_HPP_
