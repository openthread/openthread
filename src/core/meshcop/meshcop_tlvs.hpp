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

#include "openthread-core-config.h"

#include <openthread/commissioner.h>
#include <openthread/dataset.h>
#include <openthread/platform/radio.h>

#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/num_utils.hpp"
#include "common/string.hpp"
#include "common/tlvs.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "meshcop/network_name.hpp"
#include "meshcop/timestamp.hpp"
#include "net/ip6_address.hpp"
#include "radio/radio.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace MeshCoP {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;
using ot::Encoding::BigEndian::ReadUint24;
using ot::Encoding::BigEndian::WriteUint24;

/**
 * Implements MeshCoP TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * MeshCoP TLV Types.
     *
     */
    enum Type : uint8_t
    {
        kChannel                 = OT_MESHCOP_TLV_CHANNEL,                  ///< Channel TLV
        kPanId                   = OT_MESHCOP_TLV_PANID,                    ///< PAN ID TLV
        kExtendedPanId           = OT_MESHCOP_TLV_EXTPANID,                 ///< Extended PAN ID TLV
        kNetworkName             = OT_MESHCOP_TLV_NETWORKNAME,              ///< Network Name TLV
        kPskc                    = OT_MESHCOP_TLV_PSKC,                     ///< PSKc TLV
        kNetworkKey              = OT_MESHCOP_TLV_NETWORKKEY,               ///< Network Network Key TLV
        kNetworkKeySequence      = OT_MESHCOP_TLV_NETWORK_KEY_SEQUENCE,     ///< Network Key Sequence TLV
        kMeshLocalPrefix         = OT_MESHCOP_TLV_MESHLOCALPREFIX,          ///< Mesh Local Prefix TLV
        kSteeringData            = OT_MESHCOP_TLV_STEERING_DATA,            ///< Steering Data TLV
        kBorderAgentLocator      = OT_MESHCOP_TLV_BORDER_AGENT_RLOC,        ///< Border Agent Locator TLV
        kCommissionerId          = OT_MESHCOP_TLV_COMMISSIONER_ID,          ///< Commissioner ID TLV
        kCommissionerSessionId   = OT_MESHCOP_TLV_COMM_SESSION_ID,          ///< Commissioner Session ID TLV
        kSecurityPolicy          = OT_MESHCOP_TLV_SECURITYPOLICY,           ///< Security Policy TLV
        kGet                     = OT_MESHCOP_TLV_GET,                      ///< Get TLV
        kActiveTimestamp         = OT_MESHCOP_TLV_ACTIVETIMESTAMP,          ///< Active Timestamp TLV
        kCommissionerUdpPort     = OT_MESHCOP_TLV_COMMISSIONER_UDP_PORT,    ///< Commissioner UDP Port TLV
        kState                   = OT_MESHCOP_TLV_STATE,                    ///< State TLV
        kJoinerDtlsEncapsulation = OT_MESHCOP_TLV_JOINER_DTLS,              ///< Joiner DTLS Encapsulation TLV
        kJoinerUdpPort           = OT_MESHCOP_TLV_JOINER_UDP_PORT,          ///< Joiner UDP Port TLV
        kJoinerIid               = OT_MESHCOP_TLV_JOINER_IID,               ///< Joiner IID TLV
        kJoinerRouterLocator     = OT_MESHCOP_TLV_JOINER_RLOC,              ///< Joiner Router Locator TLV
        kJoinerRouterKek         = OT_MESHCOP_TLV_JOINER_ROUTER_KEK,        ///< Joiner Router KEK TLV
        kProvisioningUrl         = OT_MESHCOP_TLV_PROVISIONING_URL,         ///< Provisioning URL TLV
        kVendorName              = OT_MESHCOP_TLV_VENDOR_NAME_TLV,          ///< meshcop Vendor Name TLV
        kVendorModel             = OT_MESHCOP_TLV_VENDOR_MODEL_TLV,         ///< meshcop Vendor Model TLV
        kVendorSwVersion         = OT_MESHCOP_TLV_VENDOR_SW_VERSION_TLV,    ///< meshcop Vendor SW Version TLV
        kVendorData              = OT_MESHCOP_TLV_VENDOR_DATA_TLV,          ///< meshcop Vendor Data TLV
        kVendorStackVersion      = OT_MESHCOP_TLV_VENDOR_STACK_VERSION_TLV, ///< meshcop Vendor Stack Version TLV
        kUdpEncapsulation        = OT_MESHCOP_TLV_UDP_ENCAPSULATION_TLV,    ///< meshcop UDP encapsulation TLV
        kIp6Address              = OT_MESHCOP_TLV_IPV6_ADDRESS_TLV,         ///< meshcop IPv6 address TLV
        kPendingTimestamp        = OT_MESHCOP_TLV_PENDINGTIMESTAMP,         ///< Pending Timestamp TLV
        kDelayTimer              = OT_MESHCOP_TLV_DELAYTIMER,               ///< Delay Timer TLV
        kChannelMask             = OT_MESHCOP_TLV_CHANNELMASK,              ///< Channel Mask TLV
        kCount                   = OT_MESHCOP_TLV_COUNT,                    ///< Count TLV
        kPeriod                  = OT_MESHCOP_TLV_PERIOD,                   ///< Period TLV
        kScanDuration            = OT_MESHCOP_TLV_SCAN_DURATION,            ///< Scan Duration TLV
        kEnergyList              = OT_MESHCOP_TLV_ENERGY_LIST,              ///< Energy List TLV
        kDiscoveryRequest        = OT_MESHCOP_TLV_DISCOVERYREQUEST,         ///< Discovery Request TLV
        kDiscoveryResponse       = OT_MESHCOP_TLV_DISCOVERYRESPONSE,        ///< Discovery Response TLV
        kJoinerAdvertisement     = OT_MESHCOP_TLV_JOINERADVERTISEMENT,      ///< Joiner Advertisement TLV
    };

    /**
     * Max length of Provisioning URL TLV.
     *
     */
    static constexpr uint8_t kMaxProvisioningUrlLength = OT_PROVISIONING_URL_MAX_SIZE;

    static constexpr uint8_t kMaxCommissionerIdLength  = 64; ///< Max length of Commissioner ID TLV.
    static constexpr uint8_t kMaxVendorNameLength      = 32; ///< Max length of Vendor Name TLV.
    static constexpr uint8_t kMaxVendorModelLength     = 32; ///< Max length of Vendor Model TLV.
    static constexpr uint8_t kMaxVendorSwVersionLength = 16; ///< Max length of Vendor SW Version TLV.
    static constexpr uint8_t kMaxVendorDataLength      = 64; ///< Max length of Vendor Data TLV.

    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

    /**
     * Returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext(void) { return As<Tlv>(ot::Tlv::GetNext()); }

    /**
     * Returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    const Tlv *GetNext(void) const { return As<Tlv>(ot::Tlv::GetNext()); }

    /**
     * Indicates whether a TLV appears to be well-formed.
     *
     * @param[in]  aTlv  A reference to the TLV.
     *
     * @returns TRUE if the TLV appears to be well-formed, FALSE otherwise.
     *
     */
    static bool IsValid(const Tlv &aTlv);

} OT_TOOL_PACKED_END;

/**
 * Implements extended MeshCoP TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedTlv : public ot::ExtendedTlv
{
public:
    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     *
     */
    MeshCoP::Tlv::Type GetType(void) const { return static_cast<MeshCoP::Tlv::Type>(ot::ExtendedTlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(MeshCoP::Tlv::Type aType) { ot::ExtendedTlv::SetType(static_cast<uint8_t>(aType)); }
} OT_TOOL_PACKED_END;

/**
 * Defines Commissioner UDP Port TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kCommissionerUdpPort, uint16_t> CommissionerUdpPortTlv;

/**
 * Defines IPv6 Address TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kIp6Address, Ip6::Address> Ip6AddressTlv;

/**
 * Defines Joiner IID TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kJoinerIid, Ip6::InterfaceIdentifier> JoinerIidTlv;

/**
 * Defines Joiner Router Locator TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kJoinerRouterLocator, uint16_t> JoinerRouterLocatorTlv;

/**
 * Defines Joiner Router KEK TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kJoinerRouterKek, Kek> JoinerRouterKekTlv;

/**
 * Defines Count TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kCount, uint8_t> CountTlv;

/**
 * Defines Period TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kPeriod, uint16_t> PeriodTlv;

/**
 * Defines Scan Duration TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kScanDuration, uint16_t> ScanDurationTlv;

/**
 * Defines Commissioner ID TLV constants and type.s
 *
 */
typedef StringTlvInfo<Tlv::kCommissionerId, Tlv::kMaxCommissionerIdLength> CommissionerIdTlv;

/**
 * Implements Channel TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelTlv : public Tlv, public TlvInfo<Tlv::kChannel>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannel);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const;

    /**
     * Returns the ChannelPage value.
     *
     * @returns The ChannelPage value.
     *
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * Sets the ChannelPage value.
     *
     * @param[in]  aChannelPage  The ChannelPage value.
     *
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * Returns the Channel value.
     *
     * @returns The Channel value.
     *
     */
    uint16_t GetChannel(void) const { return HostSwap16(mChannel); }

    /**
     * Sets the Channel value.
     * Note: This method also sets the channel page according to the channel value.
     *
     * @param[in]  aChannel  The Channel value.
     *
     */
    void SetChannel(uint16_t aChannel);

private:
    uint8_t  mChannelPage;
    uint16_t mChannel;
} OT_TOOL_PACKED_END;

/**
 * Implements PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PanIdTlv : public Tlv, public UintTlvInfo<Tlv::kPanId, uint16_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kPanId);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the PAN ID value.
     *
     * @returns The PAN ID value.
     *
     */
    uint16_t GetPanId(void) const { return HostSwap16(mPanId); }

    /**
     * Sets the PAN ID value.
     *
     * @param[in]  aPanId  The PAN ID value.
     *
     */
    void SetPanId(uint16_t aPanId) { mPanId = HostSwap16(aPanId); }

private:
    uint16_t mPanId;
} OT_TOOL_PACKED_END;

/**
 * Implements Extended PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedPanIdTlv : public Tlv, public SimpleTlvInfo<Tlv::kExtendedPanId, ExtendedPanId>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kExtendedPanId);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Extended PAN ID value.
     *
     * @returns The Extended PAN ID value.
     *
     */
    const ExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * Sets the Extended PAN ID value.
     *
     * @param[in]  aExtendedPanId  An Extended PAN ID value.
     *
     */
    void SetExtendedPanId(const ExtendedPanId &aExtendedPanId) { mExtendedPanId = aExtendedPanId; }

private:
    ExtendedPanId mExtendedPanId;
} OT_TOOL_PACKED_END;

/**
 * Implements Network Name TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkNameTlv : public Tlv, public TlvInfo<Tlv::kNetworkName>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kNetworkName);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const;

    /**
     * Gets the Network Name value.
     *
     * @returns The Network Name value (as `NameData`).
     *
     */
    NameData GetNetworkName(void) const;

    /**
     * Sets the Network Name value.
     *
     * @param[in] aNameData   A Network Name value (as `NameData`).
     *
     */
    void SetNetworkName(const NameData &aNameData);

private:
    char mNetworkName[NetworkName::kMaxSize];
} OT_TOOL_PACKED_END;

/**
 * Implements PSKc TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PskcTlv : public Tlv, public SimpleTlvInfo<Tlv::kPskc, Pskc>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kPskc);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the PSKc value.
     *
     * @returns The PSKc value.
     *
     */
    const Pskc &GetPskc(void) const { return mPskc; }

    /**
     * Sets the PSKc value.
     *
     * @param[in]  aPskc  A pointer to the PSKc value.
     *
     */
    void SetPskc(const Pskc &aPskc) { mPskc = aPskc; }

private:
    Pskc mPskc;
} OT_TOOL_PACKED_END;

/**
 * Implements Network Network Key TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkKeyTlv : public Tlv, public SimpleTlvInfo<Tlv::kNetworkKey, NetworkKey>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kNetworkKey);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Network Network Key value.
     *
     * @returns The Network Network Key value.
     *
     */
    const NetworkKey &GetNetworkKey(void) const { return mNetworkKey; }

    /**
     * Sets the Network Network Key value.
     *
     * @param[in]  aNetworkKey  The Network Network Key.
     *
     */
    void SetNetworkKey(const NetworkKey &aNetworkKey) { mNetworkKey = aNetworkKey; }

private:
    NetworkKey mNetworkKey;
} OT_TOOL_PACKED_END;

/**
 * Implements Network Key Sequence TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkKeySequenceTlv : public Tlv, public UintTlvInfo<Tlv::kNetworkKeySequence, uint32_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kNetworkKeySequence);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Network Key Sequence value.
     *
     * @returns The Network Key Sequence value.
     *
     */
    uint32_t GetNetworkKeySequence(void) const { return HostSwap32(mNetworkKeySequence); }

    /**
     * Sets the Network Key Sequence value.
     *
     * @param[in]  aNetworkKeySequence  The Network Key Sequence value.
     *
     */
    void SetNetworkKeySequence(uint32_t aNetworkKeySequence) { mNetworkKeySequence = HostSwap32(aNetworkKeySequence); }

private:
    uint32_t mNetworkKeySequence;
} OT_TOOL_PACKED_END;

/**
 * Implements Mesh Local Prefix TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MeshLocalPrefixTlv : public Tlv, public SimpleTlvInfo<Tlv::kMeshLocalPrefix, Ip6::NetworkPrefix>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMeshLocalPrefix);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the size (in bytes) of the Mesh Local Prefix field.
     *
     * @returns The size (in bytes) of the Mesh Local Prefix field (8 bytes).
     *
     */
    uint8_t GetMeshLocalPrefixLength(void) const { return sizeof(mMeshLocalPrefix); }

    /**
     * Returns the Mesh Local Prefix value.
     *
     * @returns The Mesh Local Prefix value.
     *
     */
    const Ip6::NetworkPrefix &GetMeshLocalPrefix(void) const { return mMeshLocalPrefix; }

    /**
     * Sets the Mesh Local Prefix value.
     *
     * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix value.
     *
     */
    void SetMeshLocalPrefix(const Ip6::NetworkPrefix &aMeshLocalPrefix) { mMeshLocalPrefix = aMeshLocalPrefix; }

private:
    Ip6::NetworkPrefix mMeshLocalPrefix;
} OT_TOOL_PACKED_END;

class SteeringData;

/**
 * Implements Steering Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class SteeringDataTlv : public Tlv, public TlvInfo<Tlv::kSteeringData>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kSteeringData);
        SetLength(sizeof(*this) - sizeof(Tlv));
        Clear();
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() > 0; }

    /**
     * Returns the Steering Data length.
     *
     * @returns The Steering Data length.
     *
     */
    uint8_t GetSteeringDataLength(void) const
    {
        return GetLength() <= sizeof(mSteeringData) ? GetLength() : sizeof(mSteeringData);
    }

    /**
     * Sets all bits in the Bloom Filter to zero.
     *
     */
    void Clear(void) { memset(mSteeringData, 0, GetSteeringDataLength()); }

    /**
     * Copies the Steering Data from the TLV into a given `SteeringData` variable.
     *
     * @param[out]  aSteeringData   A reference to a `SteeringData` to copy into.
     *
     */
    void CopyTo(SteeringData &aSteeringData) const;

private:
    uint8_t mSteeringData[OT_STEERING_DATA_MAX_LENGTH];
} OT_TOOL_PACKED_END;

/**
 * Implements Border Agent Locator TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BorderAgentLocatorTlv : public Tlv, public UintTlvInfo<Tlv::kBorderAgentLocator, uint16_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kBorderAgentLocator);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Border Agent Locator value.
     *
     * @returns The Border Agent Locator value.
     *
     */
    uint16_t GetBorderAgentLocator(void) const { return HostSwap16(mLocator); }

    /**
     * Sets the Border Agent Locator value.
     *
     * @param[in]  aLocator  The Border Agent Locator value.
     *
     */
    void SetBorderAgentLocator(uint16_t aLocator) { mLocator = HostSwap16(aLocator); }

private:
    uint16_t mLocator;
} OT_TOOL_PACKED_END;

/**
 * Implements Commissioner Session ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class CommissionerSessionIdTlv : public Tlv, public UintTlvInfo<Tlv::kCommissionerSessionId, uint16_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kCommissionerSessionId);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Commissioner Session ID value.
     *
     * @returns The Commissioner Session ID value.
     *
     */
    uint16_t GetCommissionerSessionId(void) const { return HostSwap16(mSessionId); }

    /**
     * Sets the Commissioner Session ID value.
     *
     * @param[in]  aSessionId  The Commissioner Session ID value.
     *
     */
    void SetCommissionerSessionId(uint16_t aSessionId) { mSessionId = HostSwap16(aSessionId); }

private:
    uint16_t mSessionId;
} OT_TOOL_PACKED_END;

/**
 * Implements Security Policy TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class SecurityPolicyTlv : public Tlv, public TlvInfo<Tlv::kSecurityPolicy>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kSecurityPolicy);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const;

    /**
     * Returns the Security Policy.
     *
     * @returns  The Security Policy.
     *
     */
    SecurityPolicy GetSecurityPolicy(void) const;

    /**
     * Sets the Security Policy.
     *
     * @param[in]  aSecurityPolicy  The Security Policy which will be set.
     *
     */
    void SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy);

private:
    static constexpr uint8_t kThread11FlagsLength = 1; // The Thread 1.1 Security Policy Flags length.
    static constexpr uint8_t kThread12FlagsLength = 2; // The Thread 1.2 Security Policy Flags length.

    void     SetRotationTime(uint16_t aRotationTime) { mRotationTime = HostSwap16(aRotationTime); }
    uint16_t GetRotationTime(void) const { return HostSwap16(mRotationTime); }
    uint8_t  GetFlagsLength(void) const { return GetLength() - sizeof(mRotationTime); }

    uint16_t mRotationTime;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    uint8_t mFlags[kThread12FlagsLength];
#else
    uint8_t mFlags[kThread11FlagsLength];
#endif
} OT_TOOL_PACKED_END;

/**
 * Implements Active Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ActiveTimestampTlv : public Tlv, public SimpleTlvInfo<Tlv::kActiveTimestamp, Timestamp>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kActiveTimestamp);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mTimestamp.Clear();
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Gets the timestamp.
     *
     * @returns The timestamp.
     *
     */
    const Timestamp &GetTimestamp(void) const { return mTimestamp; }

    /**
     * Returns a reference to the timestamp.
     *
     * @returns A reference to the timestamp.
     *
     */
    Timestamp &GetTimestamp(void) { return mTimestamp; }

    /**
     * Sets the timestamp.
     *
     * @param[in] aTimestamp   The new timestamp.
     *
     */
    void SetTimestamp(const Timestamp &aTimestamp) { mTimestamp = aTimestamp; }

private:
    Timestamp mTimestamp;
} OT_TOOL_PACKED_END;

/**
 * Implements State TLV generation and parsing.
 *
 */
class StateTlv : public UintTlvInfo<Tlv::kState, uint8_t>
{
public:
    StateTlv(void) = delete;

    /**
     * State values.
     *
     */
    enum State : uint8_t
    {
        kReject  = 0xff, ///< Reject (-1)
        kPending = 0,    ///< Pending
        kAccept  = 1,    ///< Accept
    };

    /**
     * Converts a `State` to a string.
     *
     * @param[in] aState  An item state.
     *
     * @returns A string representation of @p aState.
     *
     */
    static const char *StateToString(State aState);
};

/**
 * Implements Joiner UDP Port TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerUdpPortTlv : public Tlv, public UintTlvInfo<Tlv::kJoinerUdpPort, uint16_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kJoinerUdpPort);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the UDP Port value.
     *
     * @returns The UDP Port value.
     *
     */
    uint16_t GetUdpPort(void) const { return HostSwap16(mUdpPort); }

    /**
     * Sets the UDP Port value.
     *
     * @param[in]  aUdpPort  The UDP Port value.
     *
     */
    void SetUdpPort(uint16_t aUdpPort) { mUdpPort = HostSwap16(aUdpPort); }

private:
    uint16_t mUdpPort;
} OT_TOOL_PACKED_END;

/**
 * Implements Pending Timestamp TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PendingTimestampTlv : public Tlv, public SimpleTlvInfo<Tlv::kPendingTimestamp, Timestamp>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kPendingTimestamp);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mTimestamp.Clear();
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Gets the timestamp.
     *
     * @returns The timestamp.
     *
     */
    const Timestamp &GetTimestamp(void) const { return mTimestamp; }

    /**
     * Returns a reference to the timestamp.
     *
     * @returns A reference to the timestamp.
     *
     */
    Timestamp &GetTimestamp(void) { return mTimestamp; }

    /**
     * Sets the timestamp.
     *
     * @param[in] aTimestamp   The new timestamp.
     *
     */
    void SetTimestamp(const Timestamp &aTimestamp) { mTimestamp = aTimestamp; }

private:
    Timestamp mTimestamp;
} OT_TOOL_PACKED_END;

/**
 * Implements Delay Timer TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DelayTimerTlv : public Tlv, public UintTlvInfo<Tlv::kDelayTimer, uint32_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kDelayTimer);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Delay Timer value.
     *
     * @returns The Delay Timer value.
     *
     */
    uint32_t GetDelayTimer(void) const { return HostSwap32(mDelayTimer); }

    /**
     * Sets the Delay Timer value.
     *
     * @param[in]  aDelayTimer  The Delay Timer value.
     *
     */
    void SetDelayTimer(uint32_t aDelayTimer) { mDelayTimer = HostSwap32(aDelayTimer); }

    static constexpr uint32_t kMaxDelayTimer = 259200; ///< Maximum delay timer value for a Pending Dataset in seconds

    /**
     * Minimum Delay Timer value for a Pending Operational Dataset (ms)
     *
     */
    static constexpr uint32_t kDelayTimerMinimal = OPENTHREAD_CONFIG_TMF_PENDING_DATASET_MINIMUM_DELAY;

    /**
     * Default Delay Timer value for a Pending Operational Dataset (ms)
     *
     */
    static constexpr uint32_t kDelayTimerDefault = OPENTHREAD_CONFIG_TMF_PENDING_DATASET_DEFAULT_DELAY;

private:
    uint32_t mDelayTimer;
} OT_TOOL_PACKED_END;

// forward declare ChannelMaskTlv
class ChannelMaskTlv;

/**
 * Implements Channel Mask Entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskEntryBase
{
public:
    /**
     * Gets the ChannelPage value.
     *
     * @returns The ChannelPage value.
     *
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * Sets the ChannelPage value.
     *
     * @param[in]  aChannelPage  The ChannelPage value.
     *
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * Gets the MaskLength value.
     *
     * @returns The MaskLength value.
     *
     */
    uint8_t GetMaskLength(void) const { return mMaskLength; }

    /**
     * Sets the MaskLength value.
     *
     * @param[in]  aMaskLength  The MaskLength value.
     *
     */
    void SetMaskLength(uint8_t aMaskLength) { mMaskLength = aMaskLength; }

    /**
     * Returns the total size of this Channel Mask Entry including the mask.
     *
     * @returns The total size of this entry (number of bytes).
     *
     */
    uint16_t GetEntrySize(void) const { return sizeof(ChannelMaskEntryBase) + mMaskLength; }

    /**
     * Clears the bit corresponding to @p aChannel in ChannelMask.
     *
     * @param[in]  aChannel  The channel in ChannelMask to clear.
     *
     */
    void ClearChannel(uint8_t aChannel)
    {
        uint8_t *mask = reinterpret_cast<uint8_t *>(this) + sizeof(*this);
        mask[aChannel / 8] &= ~(0x80 >> (aChannel % 8));
    }

    /**
     * Sets the bit corresponding to @p aChannel in ChannelMask.
     *
     * @param[in]  aChannel  The channel in ChannelMask to set.
     *
     */
    void SetChannel(uint8_t aChannel)
    {
        uint8_t *mask = reinterpret_cast<uint8_t *>(this) + sizeof(*this);
        mask[aChannel / 8] |= 0x80 >> (aChannel % 8);
    }

    /**
     * Indicates whether or not the bit corresponding to @p aChannel in ChannelMask is set.
     *
     * @param[in]  aChannel  The channel in ChannelMask to get.
     *
     */
    bool IsChannelSet(uint8_t aChannel) const
    {
        const uint8_t *mask = reinterpret_cast<const uint8_t *>(this) + sizeof(*this);
        return (aChannel < (mMaskLength * 8)) ? ((mask[aChannel / 8] & (0x80 >> (aChannel % 8))) != 0) : false;
    }

    /**
     * Gets the next Channel Mask Entry in a Channel Mask TLV.
     *
     * @returns A pointer to next Channel Mask Entry.
     *
     */
    const ChannelMaskEntryBase *GetNext(void) const
    {
        return reinterpret_cast<const ChannelMaskEntryBase *>(reinterpret_cast<const uint8_t *>(this) + GetEntrySize());
    }

    /**
     * Gets the next Channel Mask Entry in a Channel Mask TLV.
     *
     * @returns A pointer to next Channel Mask Entry.
     *
     */
    ChannelMaskEntryBase *GetNext(void) { return AsNonConst(AsConst(this)->GetNext()); }

private:
    uint8_t mChannelPage;
    uint8_t mMaskLength;
} OT_TOOL_PACKED_END;

/**
 * Implements Channel Mask Entry Page 0 generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskEntry : public ChannelMaskEntryBase
{
public:
    /**
     * Initializes the entry.
     *
     */
    void Init(void)
    {
        SetChannelPage(0);
        SetMaskLength(sizeof(mMask));
    }

    /**
     * Indicates whether or not the entry appears to be well-formed.
     *
     * @retval TRUE   If the entry appears to be well-formed.
     * @retval FALSE  If the entry does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetMaskLength() == sizeof(mMask); }

    /**
     * Returns the Channel Mask value as a `uint32_t` bit mask.
     *
     * @returns The Channel Mask value.
     *
     */
    uint32_t GetMask(void) const { return Encoding::Reverse32(HostSwap32(mMask)); }

    /**
     * Sets the Channel Mask value.
     *
     * @param[in]  aMask  The Channel Mask value.
     *
     */
    void SetMask(uint32_t aMask) { mMask = HostSwap32(Encoding::Reverse32(aMask)); }

private:
    uint32_t mMask;
} OT_TOOL_PACKED_END;

/**
 * Implements Channel Mask TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskBaseTlv : public Tlv, public TlvInfo<Tlv::kChannelMask>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannelMask);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const;

    /**
     * Gets the first Channel Mask Entry in the Channel Mask TLV.
     *
     * @returns A pointer to first Channel Mask Entry or `nullptr` if not found.
     *
     */
    const ChannelMaskEntryBase *GetFirstEntry(void) const;

    /**
     * Gets the first Channel Mask Entry in the Channel Mask TLV.
     *
     * @returns A pointer to first Channel Mask Entry or `nullptr` if not found.
     *
     */
    ChannelMaskEntryBase *GetFirstEntry(void);
} OT_TOOL_PACKED_END;

/**
 * Implements Channel Mask TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskTlv : public ChannelMaskBaseTlv
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannelMask);
        SetLength(sizeof(*this) - sizeof(Tlv));
        memset(mEntries, 0, sizeof(mEntries));
    }

    /**
     * Sets the Channel Mask Entries.
     *
     * @param[in]  aChannelMask  The Channel Mask value.
     *
     */
    void SetChannelMask(uint32_t aChannelMask);

    /**
     * Returns the Channel Mask value as a `uint32_t` bit mask.
     *
     * @returns The Channel Mask or 0 if not found.
     *
     */
    uint32_t GetChannelMask(void) const;

    /**
     * Reads message and returns the Channel Mask value as a `uint32_t` bit mask.
     *
     * @param[in]   aMessage     A reference to the message.
     *
     * @returns The Channel Mask or 0 if not found.
     *
     */
    static uint32_t GetChannelMask(const Message &aMessage);

private:
    static constexpr uint8_t kNumMaskEntries = Radio::kNumChannelPages;

    ChannelMaskEntry mEntries[kNumMaskEntries];
} OT_TOOL_PACKED_END;

/**
 * Implements Energy List TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class EnergyListTlv : public Tlv, public TlvInfo<Tlv::kEnergyList>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kEnergyList);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return true; }

    /**
     * Returns a pointer to the start of energy measurement list.
     *
     * @returns A pointer to the start start of energy energy measurement list.
     *
     */
    const uint8_t *GetEnergyList(void) const { return mEnergyList; }

    /**
     * Returns the length of energy measurement list.
     *
     * @returns The length of energy measurement list.
     *
     */
    uint8_t GetEnergyListLength(void) const { return Min(kMaxListLength, GetLength()); }

private:
    static constexpr uint8_t kMaxListLength = OPENTHREAD_CONFIG_TMF_ENERGY_SCAN_MAX_RESULTS;

    uint8_t mEnergyList[kMaxListLength];
} OT_TOOL_PACKED_END;

/**
 * Defines Provisioning TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kProvisioningUrl, Tlv::kMaxProvisioningUrlLength> ProvisioningUrlTlv;

/**
 * Defines Vendor Name TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kVendorName, Tlv::kMaxVendorNameLength> VendorNameTlv;

/**
 * Defines Vendor Model TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kVendorModel, Tlv::kMaxVendorModelLength> VendorModelTlv;

/**
 * Defines Vendor SW Version TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kVendorSwVersion, Tlv::kMaxVendorSwVersionLength> VendorSwVersionTlv;

/**
 * Defines Vendor Data TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kVendorData, Tlv::kMaxVendorDataLength> VendorDataTlv;

/**
 * Implements Vendor Stack Version TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class VendorStackVersionTlv : public Tlv, public TlvInfo<Tlv::kVendorStackVersion>
{
public:
    /**
     * Default constructor.
     *
     */
    VendorStackVersionTlv(void)
        : mBuildRevision(0)
        , mMinorMajor(0)
    {
    }

    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kVendorStackVersion);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Stack Vendor OUI value.
     *
     * @returns The Vendor Stack Vendor OUI value.
     *
     */
    uint32_t GetOui(void) const { return ReadUint24(mOui); }

    /**
     * Returns the Stack Vendor OUI value.
     *
     * @param[in]  aOui  The Vendor Stack Vendor OUI value.
     *
     */
    void SetOui(uint32_t aOui) { WriteUint24(aOui, mOui); }

    /**
     * Returns the Build value.
     *
     * @returns The Build value.
     *
     */
    uint16_t GetBuild(void) const { return (HostSwap16(mBuildRevision) & kBuildMask) >> kBuildOffset; }

    /**
     * Sets the Build value.
     *
     * @param[in]  aBuild  The Build value.
     *
     */
    void SetBuild(uint16_t aBuild)
    {
        mBuildRevision =
            HostSwap16((HostSwap16(mBuildRevision) & ~kBuildMask) | ((aBuild << kBuildOffset) & kBuildMask));
    }

    /**
     * Returns the Revision value.
     *
     * @returns The Revision value.
     *
     */
    uint8_t GetRevision(void) const { return (HostSwap16(mBuildRevision) & kRevMask) >> kRevOffset; }

    /**
     * Sets the Revision value.
     *
     * @param[in]  aRevision  The Revision value.
     *
     */
    void SetRevision(uint8_t aRevision)
    {
        mBuildRevision = HostSwap16((HostSwap16(mBuildRevision) & ~kRevMask) | ((aRevision << kRevOffset) & kRevMask));
    }

    /**
     * Returns the Minor value.
     *
     * @returns The Minor value.
     *
     */
    uint8_t GetMinor(void) const { return (mMinorMajor & kMinorMask) >> kMinorOffset; }

    /**
     * Sets the Minor value.
     *
     * @param[in]  aMinor  The Minor value.
     *
     */
    void SetMinor(uint8_t aMinor)
    {
        mMinorMajor = (mMinorMajor & ~kMinorMask) | ((aMinor << kMinorOffset) & kMinorMask);
    }

    /**
     * Returns the Major value.
     *
     * @returns The Major value.
     *
     */
    uint8_t GetMajor(void) const { return (mMinorMajor & kMajorMask) >> kMajorOffset; }

    /**
     * Sets the Major value.
     *
     * @param[in] aMajor  The Major value.
     *
     */
    void SetMajor(uint8_t aMajor)
    {
        mMinorMajor = (mMinorMajor & ~kMajorMask) | ((aMajor << kMajorOffset) & kMajorMask);
    }

private:
    // For `mBuildRevision`
    static constexpr uint8_t  kBuildOffset = 4;
    static constexpr uint16_t kBuildMask   = 0xfff << kBuildOffset;
    static constexpr uint8_t  kRevOffset   = 0;
    static constexpr uint16_t kRevMask     = 0xf << kBuildOffset;

    // For `mMinorMajor`
    static constexpr uint8_t kMinorOffset = 4;
    static constexpr uint8_t kMinorMask   = 0xf << kMinorOffset;
    static constexpr uint8_t kMajorOffset = 0;
    static constexpr uint8_t kMajorMask   = 0xf << kMajorOffset;

    uint8_t  mOui[3];
    uint16_t mBuildRevision;
    uint8_t  mMinorMajor;
} OT_TOOL_PACKED_END;

/**
 * Defines UDP Encapsulation TLV types and constants.
 *
 */
typedef TlvInfo<MeshCoP::Tlv::kUdpEncapsulation> UdpEncapsulationTlv;

/**
 * Represents UDP Encapsulation TLV value header (source and destination ports).
 *
 */
OT_TOOL_PACKED_BEGIN
class UdpEncapsulationTlvHeader
{
public:
    /**
     * Returns the source port.
     *
     * @returns The source port.
     *
     */
    uint16_t GetSourcePort(void) const { return HostSwap16(mSourcePort); }

    /**
     * Updates the source port.
     *
     * @param[in]   aSourcePort     The source port.
     *
     */
    void SetSourcePort(uint16_t aSourcePort) { mSourcePort = HostSwap16(aSourcePort); }

    /**
     * Returns the destination port.
     *
     * @returns The destination port.
     *
     */
    uint16_t GetDestinationPort(void) const { return HostSwap16(mDestinationPort); }

    /**
     * Updates the destination port.
     *
     * @param[in]   aDestinationPort    The destination port.
     *
     */
    void SetDestinationPort(uint16_t aDestinationPort) { mDestinationPort = HostSwap16(aDestinationPort); }

private:
    uint16_t mSourcePort;
    uint16_t mDestinationPort;
    // Followed by the UDP Payload.
} OT_TOOL_PACKED_END;

/**
 * Implements Discovery Request TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryRequestTlv : public Tlv, public TlvInfo<Tlv::kDiscoveryRequest>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kDiscoveryRequest);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mFlags    = 0;
        mReserved = 0;
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * Sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion)
    {
        mFlags = (mFlags & ~kVersionMask) | ((aVersion << kVersionOffset) & kVersionMask);
    }

    /**
     * Indicates whether or not the Joiner flag is set.
     *
     * @retval TRUE   If the Joiner flag is set.
     * @retval FALSE  If the Joiner flag is not set.
     *
     */
    bool IsJoiner(void) const { return (mFlags & kJoinerMask) != 0; }

    /**
     * Sets the Joiner flag.
     *
     * @param[in]  aJoiner  TRUE if set, FALSE otherwise.
     *
     */
    void SetJoiner(bool aJoiner)
    {
        if (aJoiner)
        {
            mFlags |= kJoinerMask;
        }
        else
        {
            mFlags &= ~kJoinerMask;
        }
    }

private:
    static constexpr uint8_t kVersionOffset = 4;
    static constexpr uint8_t kVersionMask   = 0xf << kVersionOffset;
    static constexpr uint8_t kJoinerOffset  = 3;
    static constexpr uint8_t kJoinerMask    = 1 << kJoinerOffset;

    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

/**
 * Implements Discovery Response TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryResponseTlv : public Tlv, public TlvInfo<Tlv::kDiscoveryResponse>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kDiscoveryResponse);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mFlags    = 0;
        mReserved = 0;
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * Sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion)
    {
        mFlags = (mFlags & ~kVersionMask) | ((aVersion << kVersionOffset) & kVersionMask);
    }

    /**
     * Indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     *
     */
    bool IsNativeCommissioner(void) const { return (mFlags & kNativeMask) != 0; }

    /**
     * Sets the Native Commissioner flag.
     *
     * @param[in]  aNativeCommissioner  TRUE if set, FALSE otherwise.
     *
     */
    void SetNativeCommissioner(bool aNativeCommissioner)
    {
        if (aNativeCommissioner)
        {
            mFlags |= kNativeMask;
        }
        else
        {
            mFlags &= ~kNativeMask;
        }
    }

    /**
     * Indicates whether or not the Commercial Commissioning Mode flag is set.
     *
     * @retval TRUE   If the Commercial Commissioning Mode flag is set.
     * @retval FALSE  If the Commercial Commissioning Mode flag is not set.
     *
     */
    bool IsCommercialCommissioningMode(void) const { return (mFlags & kCCMMask) != 0; }

    /**
     * Sets the Commercial Commissioning Mode flag.
     *
     * @param[in]  aCCM  TRUE if set, FALSE otherwise.
     *
     */
    void SetCommercialCommissioningMode(bool aCCM)
    {
        if (aCCM)
        {
            mFlags |= kCCMMask;
        }
        else
        {
            mFlags &= ~kCCMMask;
        }
    }

private:
    static constexpr uint8_t kVersionOffset = 4;
    static constexpr uint8_t kVersionMask   = 0xf << kVersionOffset;
    static constexpr uint8_t kNativeOffset  = 3;
    static constexpr uint8_t kNativeMask    = 1 << kNativeOffset;
    static constexpr uint8_t kCCMOffset     = 2;
    static constexpr uint8_t kCCMMask       = 1 << kCCMOffset;

    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

/**
 * Implements Joiner Advertisement TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class JoinerAdvertisementTlv : public Tlv, public TlvInfo<Tlv::kJoinerAdvertisement>
{
public:
    static constexpr uint8_t kAdvDataMaxLength = OT_JOINER_ADVDATA_MAX_LENGTH; ///< The Max Length of AdvData

    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kJoinerAdvertisement);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(mOui) && GetLength() <= sizeof(mOui) + sizeof(mAdvData); }

    /**
     * Returns the Vendor OUI value.
     *
     * @returns The Vendor OUI value.
     *
     */
    uint32_t GetOui(void) const { return ReadUint24(mOui); }

    /**
     * Sets the Vendor OUI value.
     *
     * @param[in]  aOui The Vendor OUI value.
     *
     */
    void SetOui(uint32_t aOui) { return WriteUint24(aOui, mOui); }

    /**
     * Returns the Adv Data length.
     *
     * @returns The AdvData length.
     *
     */
    uint8_t GetAdvDataLength(void) const { return GetLength() - sizeof(mOui); }

    /**
     * Returns the Adv Data value.
     *
     * @returns A pointer to the Adv Data value.
     *
     */
    const uint8_t *GetAdvData(void) const { return mAdvData; }

    /**
     * Sets the Adv Data value.
     *
     * @param[in]  aAdvData        A pointer to the AdvData value.
     * @param[in]  aAdvDataLength  The length of AdvData in bytes.
     *
     */
    void SetAdvData(const uint8_t *aAdvData, uint8_t aAdvDataLength)
    {
        OT_ASSERT((aAdvData != nullptr) && (aAdvDataLength > 0) && (aAdvDataLength <= kAdvDataMaxLength));

        SetLength(aAdvDataLength + sizeof(mOui));
        memcpy(mAdvData, aAdvData, aAdvDataLength);
    }

private:
    uint8_t mOui[3];
    uint8_t mAdvData[kAdvDataMaxLength];
} OT_TOOL_PACKED_END;

} // namespace MeshCoP

} // namespace ot

#endif // MESHCOP_TLVS_HPP_
