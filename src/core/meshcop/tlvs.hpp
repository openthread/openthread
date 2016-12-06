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
 *   This file includes definitions for generating and processing MeshCoP TLVs.
 *
 */

#ifndef MESHCOP_TLVS_HPP_
#define MESHCOP_TLVS_HPP_

#include <string.h>

#include <openthread-types.h>
#include <common/encoding.hpp>
#include <common/message.hpp>

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
        kChannel                 = OT_MESHCOP_TLV_CHANNEL,           ///< Channel TLV
        kPanId                   = OT_MESHCOP_TLV_PANID,             ///< PAN ID TLV
        kExtendedPanId           = OT_MESHCOP_TLV_EXTPANID,          ///< Extended PAN ID TLV
        kNetworkName             = OT_MESHCOP_TLV_NETWORKNAME,       ///< Newtork Name TLV
        kPSKc                    = OT_MESHCOP_TLV_PSKC,              ///< PSKc TLV
        kNetworkMasterKey        = OT_MESHCOP_TLV_MASTERKEY,         ///< Network Master Key TLV
        kMeshLocalPrefix         = OT_MESHCOP_TLV_MESHLOCALPREFIX,   ///< Mesh Local Prefix TLV
        kSteeringData            = OT_MESHCOP_TLV_STEERING_DATA,     ///< Steering Data TLV
        kBorderAgentLocator      = OT_MESHCOP_TLV_BORDER_AGENT_RLOC, ///< Border Agent Locator TLV
        kCommissionerId          = OT_MESHCOP_TLV_COMMISSIONER_ID,   ///< Commissioner ID TLV
        kCommissionerSessionId   = OT_MESHCOP_TLV_COMM_SESSION_ID,   ///< Commissioner Session ID TLV
        kSecurityPolicy          = OT_MESHCOP_TLV_SECURITYPOLICY,    ///< Security Policy TLV
        kGet                     = OT_MESHCOP_TLV_GET,               ///< Get TLV
        kActiveTimestamp         = OT_MESHCOP_TLV_ACTIVETIMESTAMP,   ///< Active Timestamp TLV
        kState                   = OT_MESHCOP_TLV_STATE,             ///< State TLV
        kJoinerDtlsEncapsulation = OT_MESHCOP_TLV_JOINER_DTLS,       ///< Joiner DTLS Encapsulation TLV
        kJoinerUdpPort           = OT_MESHCOP_TLV_JOINER_UDP_PORT,   ///< Joiner UDP Port TLV
        kJoinerIid               = OT_MESHCOP_TLV_JOINER_IID,        ///< Joiner IID TLV
        kJoinerRouterLocator     = OT_MESHCOP_TLV_JOINER_RLOC,       ///< Joiner Router Locator TLV
        kJoinerRouterKek         = OT_MESHCOP_TLV_JOINER_ROUTER_KEK, ///< Joiner Router KEK TLV
        kProvisioningUrl         = OT_MESHCOP_TLV_PROVISIONING_URL,  ///< Provisioning URL TLV
        kPendingTimestamp        = OT_MESHCOP_TLV_PENDINGTIMESTAMP,  ///< Pending Timestamp TLV
        kDelayTimer              = OT_MESHCOP_TLV_DELAYTIMER,        ///< Delay Timer TLV
        kChannelMask             = OT_MESHCOP_TLV_CHANNELMASK,       ///< Channel Mask TLV
        kCount                   = OT_MESHCOP_TLV_COUNT,             ///< Count TLV
        kPeriod                  = OT_MESHCOP_TLV_PERIOD,            ///< Period TLV
        kScanDuration            = OT_MESHCOP_TLV_SCAN_DURATION,     ///< Scan Duration TLV
        kEnergyList              = OT_MESHCOP_TLV_ENERGY_LIST,       ///< Energy List TLV
        kDiscoveryRequest        = OT_MESHCOP_TLV_DISCOVERYREQUEST,  ///< Discovery Request TLV
        kDiscoveryResponse       = OT_MESHCOP_TLV_DISCOVERYRESPONSE, ///< Discovery Response TLV
    };

    /**
     * Length values.
     *
     */
    enum
    {
        kExtendedLength          = 255, ///< Extended Length value
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
    uint8_t *GetValue(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(Tlv); }

    /**
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    const uint8_t *GetValue(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(Tlv); }

    /**
     * This method returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext(void) {
        return reinterpret_cast<Tlv *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this) + mLength);
    }

    const Tlv *GetNext(void) const {
        return reinterpret_cast<const Tlv *>(reinterpret_cast<const uint8_t *>(this) + sizeof(*this) + mLength);
    }

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

    /**
     * This static method finds the offset and length of a given TLV type.
     *
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aType       The Type value to search for.
     * @param[out]  aOffset     The offset where the value starts.
     * @param[out]  aLength     The length of the value.
     *
     * @retval kThreadError_None      Successfully found the TLV.
     * @retval kThreadError_NotFound  Could not find the TLV with Type @p aType.
     *
     */
    static ThreadError GetValueOffset(const Message &aMesasge, Type aType, uint16_t &aOffset, uint16_t &aLength);

private:
    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class ExtendedTlv: public Tlv
{
public:
    /**
     * This method returns the Length value.
     *
     */
    uint16_t GetLength(void) const { return HostSwap16(mLength); }

    /**
     * This method sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     *
     */
    void SetLength(uint16_t aLength) { Tlv::SetLength(kExtendedLength); mLength = HostSwap16(aLength); }

private:
    uint16_t mLength;
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
 * This class implements Steering Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class SteeringDataTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kSteeringData); SetLength(sizeof(*this) - sizeof(Tlv)); Clear(); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method sets all bits in the Bloom Filter to zero.
     *
     */
    void Clear(void) { memset(mSteeringData, 0, GetLength()); }

    /**
     * Ths method sets all bits in the Bloom Filter to one.
     *
     */
    void Set(void) { memset(mSteeringData, 0xff, GetLength()); }

    /**
     * This method returns the number of bits in the Bloom Filter.
     *
     * @returns The number of bits in the Bloom Filter.
     *
     */
    uint8_t GetNumBits(void) const { return GetLength() * 8; }

    /**
     * This method indicates whether or not bit @p aBit is set.
     *
     * @param[in]  aBit  The bit offset.
     *
     * @retval TRUE   If bit @p aBit is set.
     * @retval FALSE  If bit @p aBit is not set.
     *
     */
    bool GetBit(uint8_t aBit) const { return (mSteeringData[GetLength() - 1 - (aBit / 8)] & (1 << (aBit % 8))) != 0; }

    /**
     * This method clears bit @p aBit.
     *
     * @param[in]  aBit  The bit offset.
     *
     */
    void ClearBit(uint8_t aBit) { mSteeringData[GetLength() - 1 - (aBit / 8)] &= ~(1 << (aBit % 8)); }

    /**
     * This method sets bit @p aBit.
     *
     * @param[in]  aBit  The bit offset.
     *
     */
    void SetBit(uint8_t aBit) { mSteeringData[GetLength() - 1 - (aBit / 8)] |= 1 << (aBit % 8); }

private:
    uint8_t mSteeringData[OT_STEERING_DATA_MAX_LENGTH];
} OT_TOOL_PACKED_END;

/**
 * This class implements Border Agent Locator TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BorderAgentLocatorTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kBorderAgentLocator); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Border Agent Locator value.
     *
     * @returns The Border Agent Locator value.
     *
     */
    uint16_t GetBorderAgentLocator(void) const { return HostSwap16(mLocator); }

    /**
     * This method sets the Border Agent Locator value.
     *
     * @param[in]  aBorderAgentLocator  The Border Agent Locator value.
     *
     */
    void SetBorderAgentLocator(uint16_t aLocator) { mLocator = HostSwap16(aLocator); }

private:
    uint16_t mLocator;
} OT_TOOL_PACKED_END;

/**
 * This class implements the Commissioner ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class CommissionerIdTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kCommissionerId); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Commissioner ID value.
     *
     * @returns The Commissioner ID value.
     *
     */
    const char *GetCommissionerId(void) const { return mCommissionerId; }

    /**
     * This method sets the Commissioner ID value.
     *
     * @param[in]  aCommissionerId  A pointer to the Commissioner ID value.
     *
     */
    void SetCommissionerId(const char *aCommissionerId) {
        size_t length = strnlen(aCommissionerId, sizeof(mCommissionerId));
        memcpy(mCommissionerId, aCommissionerId, length);
    }

private:
    enum
    {
        kMaxLength = 64,
    };

    char mCommissionerId[kMaxLength];
} OT_TOOL_PACKED_END;

/**
 * This class implements Commissioner Session ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class CommissionerSessionIdTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kCommissionerSessionId); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Commissioner Session ID value.
     *
     * @returns The Commissioner Session ID value.
     *
     */
    uint16_t GetCommissionerSessionId(void) const { return HostSwap16(mSessionId); }

    /**
     * This method sets the Commissioner Session ID value.
     *
     * @param[in]  aCommissionerSessionId  The Commissioner Session ID value.
     *
     */
    void SetCommissionerSessionId(uint16_t aSessionId) { mSessionId = HostSwap16(aSessionId); }

private:
    uint16_t mSessionId;
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
        kObtainMasterKeyFlag      = OT_SECURITY_POLICY_OBTAIN_MASTER_KEY,      ///< Obtaining the Master Key
        kNativeCommissioningFlag  = OT_SECURITY_POLICY_NATIVE_COMMISSIONING,   ///< Native Commissioning
        kRoutersFlag              = OT_SECURITY_POLICY_ROUTERS,                ///< Routers enabled
        kExternalCommissionerFlag = OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER,  ///< External Commissioner allowed
        kBeaconsFlag              = OT_SECURITY_POLICY_BEACONS,                ///< Beacons enabled
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
    void Init(void) { SetType(kActiveTimestamp); SetLength(sizeof(*this) - sizeof(Tlv)); Timestamp::Init(); }

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
     * State values.
     *
     */
    enum State
    {
        kReject  = -1,  ///< Reject
        kPending = 0,   ///< Pending
        kAccept  = 1,   ///< Accept
    };

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
 * This class implements Joiner UDP Port TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerUdpPortTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kJoinerUdpPort); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the UDP Port value.
     *
     * @returns The UDP Port value.
     *
     */
    uint16_t GetUdpPort(void) const { return HostSwap16(mUdpPort); }

    /**
     * This method sets the UDP Port value.
     *
     * @param[in]  aUdpPort  The UDP Port value.
     *
     */
    void SetUdpPort(uint16_t aUdpPort) { mUdpPort = HostSwap16(aUdpPort); }

private:
    uint16_t mUdpPort;
} OT_TOOL_PACKED_END;

/**
 * This class implements Joiner IID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerIidTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kJoinerIid); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the Joiner IID.
     *
     * @returns A pointer to the Joiner IID.
     *
     */
    const uint8_t *GetIid(void) const { return mIid; }

    /**
     * This method sets the Joiner IID.
     *
     * @param[in]  aIid  A pointer to the Joiner IID.
     *
     */
    void SetIid(const uint8_t *aIid) { memcpy(mIid, aIid, sizeof(mIid)); }

private:
    uint8_t mIid[8];
} OT_TOOL_PACKED_END;

/**
 * This class implements Joiner Router Locator TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerRouterLocatorTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kJoinerRouterLocator); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Joiner Router Locator value.
     *
     * @returns The Joiner Router Locator value.
     *
     */
    uint16_t GetJoinerRouterLocator(void) const { return HostSwap16(mLocator); }

    /**
     * This method sets the Joiner Router Locator value.
     *
     * @param[in]  aJoinerRouterLocator  The Joiner Router Locator value.
     *
     */
    void SetJoinerRouterLocator(uint16_t aLocator) { mLocator = HostSwap16(aLocator); }

private:
    uint16_t mLocator;
} OT_TOOL_PACKED_END;

/**
 * This class implements Joiner Router KEK TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerRouterKekTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kJoinerRouterKek); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns a pointer to the Joiner Router KEK.
     *
     * @returns A pointer to the Joiner Router KEK.
     *
     */
    const uint8_t *GetKek(void) const { return mKek; }

    /**
     * This method sets the Joiner Router KEK.
     *
     * @param[in]  aIid  A pointer to the Joiner Router KEK.
     *
     */
    void SetKek(const uint8_t *aKek) { memcpy(mKek, aKek, sizeof(mKek)); }

private:
    uint8_t mKek[16];
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
    void Init(void) { SetType(kPendingTimestamp); SetLength(sizeof(*this) - sizeof(Tlv)); Timestamp::Init(); }

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

    enum
    {
        kMaxDelayTimer = 259200,  ///< maximum delay timer value for a Pending Dataset in seconds
        kMinDelayTimer = 28800,   ///< minimum delay timer value for a Pending Dataset in seconds
    };

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
        return (aChannel < (mMaskLength * 8)) ? ((mask[aChannel / 8] & (1 << (aChannel % 8))) != 0) : false;
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
 * This class implements Channel Mask TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMask0Tlv: public ChannelMaskTlv, public ChannelMaskEntry
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) {
        SetType(kChannelMask);
        SetLength(sizeof(*this) - sizeof(Tlv));
        SetChannelPage(0);
        SetMaskLength(sizeof(mMask));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const {
        return
            GetLength() == sizeof(*this) - sizeof(Tlv) &&
            GetChannelPage() == 0 &&
            GetMaskLength() == sizeof(mMask);
    }

    /**
     * This method returns the Channel Mask value.
     *
     * @returns The Channel Mask value.
     *
     */
    uint32_t GetMask(void) const { return Thread::Encoding::LittleEndian::HostSwap32(mMask); }

    /**
     * This method sets the Channel Mask value.
     *
     * @param[in]  aMask  The Channel Mask value.
     *
     */
    void SetMask(uint32_t aMask) { mMask = Thread::Encoding::LittleEndian::HostSwap32(aMask); }

private:
    uint32_t mMask;
} OT_TOOL_PACKED_END;

/**
 * This class implements Count TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class CountTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kCount); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Count value.
     *
     * @returns The Count value.
     *
     */
    uint8_t GetCount(void) const { return mCount; }

    /**
     * This method sets the Count value.
     *
     * @param[in]  aCount  The Count value.
     *
     */
    void SetCount(uint8_t aCount) { mCount = aCount; }

private:
    uint8_t mCount;
} OT_TOOL_PACKED_END;

/**
 * This class implements Period TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PeriodTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kPeriod); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Period value.
     *
     * @returns The Period value.
     *
     */
    uint16_t GetPeriod(void) const { return HostSwap16(mPeriod); }

    /**
     * This method sets the Period value.
     *
     * @param[in]  aPeriod  The Period value.
     *
     */
    void SetPeriod(uint16_t aPeriod) { mPeriod = HostSwap16(aPeriod); }

private:
    uint16_t mPeriod;
} OT_TOOL_PACKED_END;

/**
 * This class implements Scan Duration TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ScanDurationTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kScanDuration); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Scan Duration value.
     *
     * @returns The Scan Duration value.
     *
     */
    uint16_t GetScanDuration(void) const { return HostSwap16(mScanDuration); }

    /**
     * This method sets the Scan Duration value.
     *
     * @param[in]  aScanDuration  The Scan Duration value.
     *
     */
    void SetScanDuration(uint16_t aScanDuration) { mScanDuration = HostSwap16(aScanDuration); }

private:
    uint16_t mScanDuration;
} OT_TOOL_PACKED_END;

/**
 * This class implements Energy List TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class EnergyListTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kEnergyList); SetLength(sizeof(*this) - sizeof(Tlv)); }

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
 * This class implements Provisioning URL TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ProvisioningUrlTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kProvisioningUrl); SetLength(0); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Provisioning URL value.
     *
     * @returns The Provisioning URL value.
     *
     */
    const char *GetProvisioningUrl(void) const { return mProvisioningUrl; }

    /**
     * This method sets the Provisioning URL value.
     *
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL value.
     *
     */
    ThreadError SetProvisioningUrl(const char *aProvisioningUrl) {
        ThreadError error = kThreadError_None;
        size_t len = aProvisioningUrl ? strnlen(aProvisioningUrl, kMaxLength + 1) : 0;

        VerifyOrExit(len <= kMaxLength, error = kThreadError_InvalidArgs);
        SetLength(static_cast<uint8_t>(len));

        if (len > 0) {
            memcpy(mProvisioningUrl, aProvisioningUrl, len);
        }

exit:
        return error;
    }

private:
    enum
    {
        kMaxLength = 64,
    };

    char mProvisioningUrl[kMaxLength];
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
    void Init(void) { SetType(kDiscoveryRequest); SetLength(sizeof(*this) - sizeof(Tlv)); mFlags = 0; mReserved = 0; }

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
    bool IsJoiner(void) { return (mFlags & kJoinerMask) != 0; }

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
    void Init(void) { SetType(kDiscoveryResponse); SetLength(sizeof(*this) - sizeof(Tlv)); mFlags = 0; mReserved = 0; }

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
    bool IsNativeCommissioner(void) { return (mFlags & kNativeMask) != 0; }

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
