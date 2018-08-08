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
 *   This file includes definitions and methods for generating and processing Thread Network Layer TLVs.
 */

#ifndef THREAD_TLVS_HPP_
#define THREAD_TLVS_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

enum
{
    kCoapUdpPort = 61631,
};

/**
 * This class implements Network Layer TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadTlv : public ot::Tlv
{
public:
    /**
     * Network Layer TLV Types.
     *
     */
    enum Type
    {
        kTarget              = 0,  ///< Target EID TLV
        kExtMacAddress       = 1,  ///< Extended MAC Address TLV
        kRloc16              = 2,  ///< RLOC16 TLV
        kMeshLocalEid        = 3,  ///< ML-EID TLV
        kStatus              = 4,  ///< Status TLV
        kLastTransactionTime = 6,  ///< Time Since Last Transaction TLV
        kRouterMask          = 7,  ///< Router Mask TLV
        kNDOption            = 8,  ///< ND Option TLV
        kNDData              = 9,  ///< ND Data TLV
        kThreadNetworkData   = 10, ///< Thread Network Data TLV
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

} OT_TOOL_PACKED_END;

/**
 * This class implements Target EID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadTargetTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTarget);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

    /**
     * This method returns a reference to the Target EID.
     *
     * @returns A pointer to the Target EID.
     *
     */
    const Ip6::Address &GetTarget(void) const { return mTarget; }

    /**
     * This method sets the Target EID.
     *
     * @param[in]  aTarget  A reference to the Target EID.
     *
     */
    void SetTarget(const Ip6::Address &aTarget) { mTarget = aTarget; }

private:
    Ip6::Address mTarget;
} OT_TOOL_PACKED_END;

/**
 * This class implements Extended MAC Address TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadExtMacAddressTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kExtMacAddress);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

    /**
     * This method returns a reference to the Extended MAC Address.
     *
     * @returns A pointer to the Extended MAC Address.
     *
     */
    const Mac::ExtAddress &GetMacAddr(void) const { return mMacAddr; }

    /**
     * This method sets the Extended MAC Address.
     *
     * @param[in]  aAddress  A reference to the Extended MAC Address.
     *
     */
    void SetMacAddr(const Mac::ExtAddress &aAddress) { mMacAddr = aAddress; }

private:
    Mac::ExtAddress mMacAddr;
} OT_TOOL_PACKED_END;

/**
 * This class implements RLOC16 TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadRloc16Tlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kRloc16);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

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
 * This class implements ML-EID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadMeshLocalEidTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMeshLocalEid);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

    /**
     * This method returns a pointer to the ML-EID IID.
     *
     * @returns A pointer to the Extended MAC Address.
     *
     */
    const uint8_t *GetIid(void) const { return mIid; }

    /**
     * This method sets the ML-EID IID.
     *
     * @param[in]  aIid  A pointer to the ML-EID IID.
     *
     */
    void SetIid(const uint8_t *aIid) { memcpy(mIid, aIid, sizeof(mIid)); }

    /**
     * This method sets the ML-EID IID.
     *
     * @param[in]  aExtAddress  A reference to the MAC Extended Address.
     *
     */
    void SetIid(const Mac::ExtAddress &aExtAddress)
    {
        memcpy(mIid, aExtAddress.m8, sizeof(mIid));
        mIid[0] ^= 0x2;
    }

private:
    uint8_t mIid[8];
} OT_TOOL_PACKED_END;

/**
 * This class implements Status TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadStatusTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kStatus);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

    /**
     * Status values.
     *
     */
    enum Status
    {
        kSuccess               = 0, ///< Success.
        kNoAddressAvailable    = 1, ///< No address available.
        kTooFewRouters         = 2, ///< Address Solicit due to too few routers.
        kHaveChildIdRequest    = 3, ///< Address Solicit due to child ID request.
        kParentPartitionChange = 4, ///< Address Solicit due to parent partition change
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
 * This class implements Time Since Last Transaction TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadLastTransactionTimeTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kLastTransactionTime);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

    /**
     * This method returns the Last Transaction Time value.
     *
     * @returns The Last Transaction Time value.
     *
     */
    uint32_t GetTime(void) const { return HostSwap32(mTime); }

    /**
     * This method sets the Last Transaction Time value.
     *
     * @param[in]  aTime  The Last Transaction Time value.
     *
     */
    void SetTime(uint32_t aTime) { mTime = HostSwap32(aTime); }

private:
    uint32_t mTime;
} OT_TOOL_PACKED_END;

/**
 * This class implements Router Mask TLV generation and parsing.
 *
 */
class ThreadRouterMaskTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kRouterMask);
        SetLength(sizeof(*this) - sizeof(ThreadTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(ThreadTlv); }

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
     * This method clears the Assigned Router ID Mask.
     *
     */
    void ClearAssignedRouterIdMask(void) { memset(mAssignedRouterIdMask, 0, sizeof(mAssignedRouterIdMask)); }

    /**
     * This method indicates whether or not a given Router ID is set in the Assigned Router ID Mask.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the given Router ID is set in the Assigned Router ID Mask.
     * @retval FALSE  If the given Router ID is not set in the Assigned Router ID Mask.
     *
     */
    bool IsAssignedRouterIdSet(uint8_t aRouterId) const
    {
        return (mAssignedRouterIdMask[aRouterId / 8] & (0x80 >> (aRouterId % 8))) != 0;
    }

    /**
     * This method clears the Assigned Router ID Mask.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     */
    void SetAssignedRouterId(uint8_t aRouterId) { mAssignedRouterIdMask[aRouterId / 8] |= 0x80 >> (aRouterId % 8); }

private:
    uint8_t mIdSequence;
    uint8_t mAssignedRouterIdMask[BitVectorBytes(Mle::kMaxRouterId + 1)];
};

/**
 * This class implements Thread Network Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ThreadNetworkDataTlv : public ThreadTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kThreadNetworkData);
        SetLength(0);
    }

    /**
     * This method overrides same method of the base class
     *
     * @retval TRUE  the TLV appears to be well-formed.
     *
     */
    bool IsValid(void) const { return true; }

    /**
     * This method returns a pointer to the Network Data TLVs.
     *
     * @returns A pointer to the Network Data TLVs.
     *
     */
    uint8_t *GetTlvs(void) { return mTlvs; }

private:
    enum
    {
        kMaxSize = 255,
    };

    uint8_t mTlvs[kMaxSize];
} OT_TOOL_PACKED_END;

} // namespace ot

#endif // THREAD_TLVS_HPP_
