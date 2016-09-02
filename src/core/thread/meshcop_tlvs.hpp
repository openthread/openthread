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
 *   This file includes definitions for generating and processing MeshCoP TLVs.
 *
 */

#ifndef MESHCOP_TLVS_HPP_
#define MESHCOP_TLVS_HPP_

#include <string.h>

#include <openthread-types.h>
#include <common/encoding.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {
namespace MeshCoP {

/**
 * This class implements MeshCoP TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Tlv
{
public:
    /**
     * MeshCoP TLV Types.
     *
     */
    enum Type
    {
        kChannel           = 0,   ///< Channel TLV
        kPanId             = 1,   ///< PAN ID TLV
        kExtendedPanId     = 2,   ///< Extended PAN ID TLV
        kNetworkName       = 3,   ///< Newtork Name TLV
        kPSKc              = 4,   ///< PSKc TLV
        kNetworkMasterKey  = 5,   ///< Network Master Key TLV
        kMeshLocalPrefix   = 7,   ///< Mesh Local Prefix TLV
        kSecurityPolicy    = 12,  ///< Security Policy TLV
        kGet               = 13,  ///< Get TLV
        kActiveTimestamp   = 14,  ///< Active Timestamp TLV
        kState             = 16,  ///< State TLV
        kPendingTimestamp  = 51,  ///< Pending Timestamp TLV
        kDelayTimer        = 52,  ///< Delay Timer TLV
        kChannelMask       = 53,  ///< Channel Mask TLV
        kDiscoveryRequest  = 128,  ///< Discovery Request TLV
        kDiscoveryResponse = 129,  ///< Discovery Response TLV
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
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    uint8_t *GetValue() { return reinterpret_cast<uint8_t *>(this) + sizeof(Tlv); }

    /**
     * This method returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext() {
        return reinterpret_cast<Tlv *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this) + mLength);
    }

    const Tlv *GetNext() const {
        return reinterpret_cast<const Tlv *>(reinterpret_cast<const uint8_t *>(this) + sizeof(*this) + mLength);
    }

private:
    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Channel TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kChannel); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the ChannelPage value.
     *
     * @returns The ChannelPage value.
     *
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * This method sets the ChannelPage value.
     *
     * @param[in]  aChannelPage  The ChannelPage value.
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
    uint8_t mChannelPage;
    uint16_t mChannel;
} OT_TOOL_PACKED_END;

/**
 * This class implements PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PanIdTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kPanId); SetLength(sizeof(*this) - sizeof(Tlv)); }

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

/**
 * This class implements Extended PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedPanIdTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kExtendedPanId); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Extended PAN ID value.
     *
     * @returns The Extended PAN ID value.
     *
     */
    const uint8_t *GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the Extended PAN ID value.
     *
     * @param[in]  aExtendedPanId  A pointer to the Extended PAN ID value.
     *
     */
    void SetExtendedPanId(const uint8_t *aExtendedPanId) {
        memcpy(mExtendedPanId, aExtendedPanId, OT_EXT_PAN_ID_SIZE);
    }

private:
    uint8_t mExtendedPanId[OT_EXT_PAN_ID_SIZE];
} OT_TOOL_PACKED_END;

/**
 * This class implements Network Name TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkNameTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kNetworkName); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Network Name value.
     *
     * @returns The Network Name value.
     *
     */
    const char *GetNetworkName(void) const { return mNetworkName; }

    /**
     * This method sets the Network Name value.
     *
     * @param[in]  aNetworkName  A pointer to the Network Name value.
     *
     */
    void SetNetworkName(const char *aNetworkName) {
        size_t length = strnlen(aNetworkName, sizeof(mNetworkName));
        memcpy(mNetworkName, aNetworkName, length);
        SetLength(static_cast<uint8_t>(length));
    }

private:
    char mNetworkName[OT_NETWORK_NAME_MAX_SIZE];
} OT_TOOL_PACKED_END;

/**
 * This class implements PSKc TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PSKcTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kPSKc); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the PSKc value.
     *
     * @returns The PSKc value.
     *
     */
    const uint8_t *GetPSKc(void) const { return mPSKc; }

    /**
     * This method sets the PSKc value.
     *
     * @param[in]  aPSKc  A pointer to the PSKc value.
     *
     */
    void SetPSKc(const uint8_t *aPSKc) {
        memcpy(mPSKc, aPSKc, sizeof(mPSKc));
    }

private:
    uint8_t mPSKc[16];
} OT_TOOL_PACKED_END;

/**
 * This class implements Network Master Key TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkMasterKeyTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kNetworkMasterKey); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Network Master Key value.
     *
     * @returns The Network Master Key value.
     *
     */
    const uint8_t *GetNetworkMasterKey(void) const { return mNetworkMasterKey; }

    /**
     * This method sets the Network Master Key value.
     *
     * @param[in]  aNetworkMasterKey  A pointer to the Network Master Key value.
     *
     */
    void SetNetworkMasterKey(const uint8_t *aNetworkMasterKey) {
        memcpy(mNetworkMasterKey, aNetworkMasterKey, sizeof(mNetworkMasterKey));
    }

private:
    uint8_t mNetworkMasterKey[16];
} OT_TOOL_PACKED_END;

/**
 * This class implements Mesh Local Prefix TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MeshLocalPrefixTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kMeshLocalPrefix); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Mesh Local Prefix value.
     *
     * @returns The Mesh Local Prefix value.
     *
     */
    const uint8_t *GetMeshLocalPrefix(void) const { return mMeshLocalPrefix; }

    /**
     * This method sets the Mesh Local Prefix value.
     *
     * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix value.
     *
     */
    void SetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix) {
        memcpy(mMeshLocalPrefix, aMeshLocalPrefix, sizeof(mMeshLocalPrefix));
    }

private:
    uint8_t mMeshLocalPrefix[8];
} OT_TOOL_PACKED_END;

/**
 * This class implements Security Policy TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class SecurityPolicyTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kSecurityPolicy); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Rotation Time value.
     *
     * @returns The Rotation Time value.
     *
     */
    uint16_t GetRotationTime(void) const { return HostSwap16(mRotationTime); }

    /**
     * This method sets the Rotation Time value.
     *
     * @param[in]  aRotationTime  The Rotation Time value.
     *
     */
    void SetRotationTime(uint16_t aRotationTime) { mRotationTime = HostSwap16(aRotationTime); }

    enum
    {
        kObtainMasterKeyFlag      = 1 << 7,  ///< Obtaining the Master Key
        kNativeCommissioningFlag  = 1 << 6,  ///< Native Commissioning
        kRoutersFlag              = 1 << 5,  ///< Routers enabled
        kExternalCommissionerFlag = 1 << 4,  ///< External Commissioner allowed
        kBeaconsFlag              = 1 << 3,  ///< Beacons enabled
    };

    /**
     * This method returns the Flags value.
     *
     * @returns The Flags value.
     *
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * This method sets the Flags value.
     *
     * @param[in]  aFlags  The Flags value.
     *
     */
    void SetFlags(uint8_t aFlags) { mFlags = aFlags; }

private:
    uint16_t mRotationTime;
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * This class implements Timestamp generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Timestamp
{
public:
    /**
     * This method initializes the Timestamp
     *
     */
    void Init(void) { memset(mSeconds, 0, sizeof(mSeconds)); mTicks = 0; }

    /**
     * This method compares this timestamp to another.
     *
     * @param[in]  aCompare  A reference to the timestamp to compare.
     *
     * @retval -1  if @p aCompare is less than this timestamp.
     * @retval  0  if @p aCompare is equal to this timestamp.
     * @retval  1  if @p aCompare is greater than this timestamp.
     *
     */
    int Compare(const Timestamp &aCompare) const;

    /**
     * This method returns the Seconds value.
     *
     * @returns The Seconds value.
     *
     */
    uint64_t GetSeconds(void) const {
        uint64_t seconds = 0;

        for (size_t i = 0; i < sizeof(mSeconds); i++) {
            seconds = (seconds << 8) | mSeconds[i];
        }

        return seconds;
    }

    /**
     * This method sets the Seconds value.
     *
     * @param[in]  aSeconds  The Seconds value.
     *
     */
    void SetSeconds(uint64_t aSeconds) {
        for (size_t i = 0; i < sizeof(mSeconds); i++, aSeconds >>= 8) {
            mSeconds[sizeof(mSeconds) - 1 - i] = aSeconds & 0xff;
        }
    }

    /**
     * This method returns the Ticks value.
     *
     * @returns The Ticks value.
     *
     */
    uint16_t GetTicks(void) const { return mTicks >> kTicksOffset; }

    /**
     * This method sets the Ticks value.
     *
     * @param[in]  aTicks  The Ticks value.
     *
     */
    void SetTicks(uint16_t aTicks) {
        mTicks = (mTicks & ~kTicksMask) | ((aTicks << kTicksOffset) & kTicksMask);
    }

    /**
     * This method returns the Authoritative value.
     *
     * @returns The Authoritative value.
     *
     */
    bool GetAuthoritative(void) const { return (mTicks & kAuthoritativeMask) != 0; }

    /**
     * This method sets the Authoritative value.
     *
     * @param[in]  aAuthoritative  The Authoritative value.
     *
     */
    void SetAuthoritative(bool aAuthoritative) {
        mTicks = (mTicks & kTicksMask) | ((aAuthoritative << kAuthoritativeOffset) & kAuthoritativeMask);
    }

private:
    uint8_t mSeconds[6];

    enum
    {
        kTicksOffset         = 1,
        kTicksMask           = 0x7fff << kTicksOffset,
        kAuthoritativeOffset = 0,
        kAuthoritativeMask   = 1 << kAuthoritativeOffset,
    };
    uint16_t mTicks;
} OT_TOOL_PACKED_END;

/**
 * This class implements Active Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ActiveTimestampTlv : public Tlv, public Timestamp
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kActiveTimestamp); SetLength(sizeof(*this) - sizeof(Tlv)); }

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
 * This class implements State TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class StateTlv: public Tlv
{
public:
    /**
      * State TLV values.
      */
    enum State
    {
        kReject  = -1,   ///< Reject
        kPending = 0,    ///< Pending
        kAccept  = 1,    ///< Accept
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kState); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the State value.
     *
     * @returns The State value.
     *
     */
    State GetState(void) const { return static_cast<State>(mState); }

    /**
     * This method sets the State value.
     *
     * @param[in]  aState  The State value.
     *
     */
    void SetState(State aState) { mState = static_cast<uint8_t>(aState); }

private:
    uint8_t mState;
} OT_TOOL_PACKED_END;

/**
 * This class implements Pending Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PendingTimestampTlv : public Tlv, public Timestamp
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kPendingTimestamp); SetLength(sizeof(*this) - sizeof(Tlv)); }

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
 * This class implements Delay Timer TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DelayTimerTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kDelayTimer); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Delay Timer value.
     *
     * @returns The Delay Timer value.
     *
     */
    uint32_t GetDelayTimer(void) const { return HostSwap32(mDelayTimer); }

    /**
     * This method sets the Delay Timer value.
     *
     * @param[in]  aDelayTimer  The Delay Timer value.
     *
     */
    void SetDelayTimer(uint32_t aDelayTimer) { mDelayTimer = HostSwap32(aDelayTimer); }

private:
    uint32_t mDelayTimer;
} OT_TOOL_PACKED_END;

/**
 * This class implements Channel Mask Entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskEntry
{
public:
    /**
     * This method gets the ChannelPage value.
     *
     * @returns The ChannelPage value.
     *
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * This method sets the ChannelPage value.
     *
     * @param[in]  aChannelPage  The ChannelPage value.
     *
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * This method gets the MaskLength value.
     *
     * @returns The MaskLength value.
     *
     */
    uint8_t GetMaskLength(void) const { return mMaskLength; }

    /**
     * This method sets the MaskLength value.
     *
     * @param[in]  aMaskLength  The MaskLength value.
     *
     */
    void SetMaskLength(uint8_t aMaskLength) { mMaskLength = aMaskLength; }

    /**
     * This method clears the bit corresponding to @p aChannel in ChannelMask.
     *
     * @param[in]  aChannel  The channel in ChannelMask to clear.
     *
     */
    void ClearChannel(uint8_t aChannel) {
        uint8_t *mask = reinterpret_cast<uint8_t *>(this) + sizeof(*this);
        mask[aChannel / 8] &= ~(1 << (aChannel % 8));
    }

    /**
     * This method sets the bit corresponding to @p aChannel in ChannelMask.
     *
     * @param[in]  aChannel  The channel in ChannelMask to set.
     *
     */
    void SetChannel(uint8_t aChannel) {
        uint8_t *mask = reinterpret_cast<uint8_t *>(this) + sizeof(*this);
        mask[aChannel / 8] |= 1 << (aChannel % 8);
    }

    /**
     * This method indicates whether or not the bit corresponding to @p aChannel in ChannelMask is set.
     *
     * @param[in]  aChannel  The channel in ChannelMask to get.
     *
     */
    bool IsChannelSet(uint8_t aChannel) const {
        const uint8_t *mask = reinterpret_cast<const uint8_t *>(this) + sizeof(*this);
        return (aChannel < (mMaskLength * 8)) ? mask[aChannel / 8] & (1 << (aChannel % 8)) : false;
    }

private:
    uint8_t mChannelPage;
    uint8_t mMaskLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Channel Mask TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kChannelMask); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return true; }
} OT_TOOL_PACKED_END;

/**
 * This class implements Discovery Request TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryRequestTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kDiscoveryRequest); SetLength(sizeof(*this) - sizeof(Tlv)); mReserved = 0; }

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
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) {
        mFlags = (mFlags & ~kVersionMask) | ((aVersion << kVersionOffset) & kVersionMask);
    }

    /**
     * This method indicates whether or not the Joiner flag is set.
     *
     * @retval TRUE   If the Joiner flag is set.
     * @retval FALSE  If the Joiner flag is not set.
     *
     */
    bool IsJoiner(void) { return mFlags & kJoinerMask; }

    /**
     * This method sets the Joiner flag.
     *
     * @param[in]  aJoiner  TRUE if set, FALSE otherwise.
     *
     */
    void SetJoiner(bool aJoiner) {
        if (aJoiner) {
            mFlags |= kJoinerMask;
        }
        else {
            mFlags &= ~kJoinerMask;
        }
    }

private:
    enum
    {
        kVersionOffset = 4,
        kVersionMask = 0xf << kVersionOffset,
        kJoinerOffset = 3,
        kJoinerMask = 1 << kJoinerOffset,
    };
    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

/**
 * This class implements Discovery Response TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryResponseTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kDiscoveryResponse); SetLength(sizeof(*this) - sizeof(Tlv)); mReserved = 0; }

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
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) {
        mFlags = (mFlags & ~kVersionMask) | ((aVersion << kVersionOffset) & kVersionMask);
    }

    /**
     * This method indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     *
     */
    bool IsNativeCommissioner(void) { return mFlags & kNativeMask; }

    /**
     * This method sets the Native Commissioner flag.
     *
     * @param[in]  aNativeCommissioner  TRUE if set, FALSE otherwise.
     *
     */
    void SetNativeCommissioner(bool aNativeCommissioner) {
        if (aNativeCommissioner) {
            mFlags |= kNativeMask;
        }
        else {
            mFlags &= ~kNativeMask;
        }
    }

private:
    enum
    {
        kVersionOffset = 4,
        kVersionMask = 0xf << kVersionOffset,
        kNativeOffset = 3,
        kNativeMask = 1 << kNativeOffset,
    };
    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

}  // namespace MeshCoP

}  // namespace Thread

#endif  // MESHCOP_TLVS_HPP_
