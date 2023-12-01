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
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace MeshCoP {

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
 * Defines Commissioner ID TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kCommissionerId, Tlv::kMaxCommissionerIdLength> CommissionerIdTlv;

/**
 * Implements Channel TLV value format.
 *
 */
typedef Mle::ChannelTlvValue ChannelTlvValue;

/**
 * Defines Channel TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kChannel, ChannelTlvValue> ChannelTlv;

/**
 * Defines PAN ID TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kPanId, uint16_t> PanIdTlv;

/**
 * Defines Extended PAN ID TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kExtendedPanId, ExtendedPanId> ExtendedPanIdTlv;

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
 * Defines PSKc TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kPskc, Pskc> PskcTlv;

/**
 * Defines Network Key TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kNetworkKey, NetworkKey> NetworkKeyTlv;

/**
 * Defines Network Key Sequence TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kNetworkKeySequence, uint32_t> NetworkKeySequenceTlv;

/**
 * Defines Mesh Local Prefix TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kMeshLocalPrefix, Ip6::NetworkPrefix> MeshLocalPrefixTlv;

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
    uint16_t GetBorderAgentLocator(void) const { return BigEndian::HostSwap16(mLocator); }

    /**
     * Sets the Border Agent Locator value.
     *
     * @param[in]  aLocator  The Border Agent Locator value.
     *
     */
    void SetBorderAgentLocator(uint16_t aLocator) { mLocator = BigEndian::HostSwap16(aLocator); }

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
    uint16_t GetCommissionerSessionId(void) const { return BigEndian::HostSwap16(mSessionId); }

    /**
     * Sets the Commissioner Session ID value.
     *
     * @param[in]  aSessionId  The Commissioner Session ID value.
     *
     */
    void SetCommissionerSessionId(uint16_t aSessionId) { mSessionId = BigEndian::HostSwap16(aSessionId); }

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

    void     SetRotationTime(uint16_t aRotationTime) { mRotationTime = BigEndian::HostSwap16(aRotationTime); }
    uint16_t GetRotationTime(void) const { return BigEndian::HostSwap16(mRotationTime); }
    uint8_t  GetFlagsLength(void) const { return GetLength() - sizeof(mRotationTime); }

    uint16_t mRotationTime;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    uint8_t mFlags[kThread12FlagsLength];
#else
    uint8_t mFlags[kThread11FlagsLength];
#endif
} OT_TOOL_PACKED_END;

/**
 * Defines Active Timestamp TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kActiveTimestamp, Timestamp> ActiveTimestampTlv;

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
 * Defines Joiner UDP Port TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kJoinerUdpPort, uint16_t> JoinerUdpPortTlv;

/**
 * Defines Pending Timestamp TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kPendingTimestamp, Timestamp> PendingTimestampTlv;

/**
 * Defines Delay Timer TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kDelayTimer, uint32_t> DelayTimerTlv;

/**
 * Implements Channel Mask TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelMaskTlv : public Tlv, public TlvInfo<Tlv::kChannelMask>
{
    static constexpr uint8_t kEntryHeaderSize = 2; // Two bytes: mChannelPage and mMaskLength
    static constexpr uint8_t kEntrySize       = kEntryHeaderSize + sizeof(uint32_t);

public:
    /**
     * Represents Channel Mask TLV value to append.
     *
     */
    struct Value
    {
        static constexpr uint16_t kMaxLength = (kEntrySize * Radio::kNumChannelPages); ///< Max value length.

        uint8_t mData[kMaxLength]; ///< Array to store TLV value (encoded as one or more Channel Mask TLV Entry)
        uint8_t mLength;           ///< Value length in bytes.
    };

    ChannelMaskTlv(void) = delete;

    /**
     * Parses the Channel Mask TLV value and validates that all the included entries are well-formed.
     *
     * @returns TRUE if the TLV is well-formed, FALSE otherwise.
     *
     */
    bool IsValid(void) const;

    /**
     * Parses and retrieves the combined channel mask for all supported channel pages from entries in the TLV.
     *
     * @param[out] aChannelMask  A reference to return the channel mask.
     *
     * @retval kErrorNone   Successfully parsed the TLV value, @p aChannelMask is updated.
     * @retval kErrorParse  TLV value is not well-formed.
     *
     */
    Error ReadChannelMask(uint32_t &aChannelMask) const;

    /**
     * Searches within a given message for Channel Mask TLV, parses and validates the TLV value and returns the
     * combined channel mask for all supported channel pages included in the TLV.
     *
     * @param[in]  aMessage      The message to search in.
     * @param[out] aChannelMask  A reference to return the channel mask.
     *
     * @retval kErrorNone       Found the TLV, successfully parsed its value, @p aChannelMask is updated.
     * @retval kErrorNotFound   No Channel Mask TLV found in the @p aMessage.
     * @retval kErrorParse      Found the TLV, but failed to parse it.
     *
     */
    static Error FindIn(const Message &aMessage, uint32_t &aChannelMask);

    /**
     * Prepares Channel Mask TLV value for appending/writing.
     *
     * @param[out] aValue        A reference to `Value` structure to populate.
     * @param[in]  aChannelMask  The combined channel mask for all supported channel pages.
     *
     */
    static void PrepareValue(Value &aValue, uint32_t aChannelMask);

    /**
     * Prepares a Channel Mask TLV value and appends the TLV to a given message.
     *
     * @param[in] aMessage       The message to append to.
     * @param[in] aChannelMask   The combined channel mask for all supported channel pages.
     *
     * @retval kErrorNone    Successfully prepared the Channel Mask TLV and appended it to @p aMessage.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     *
     */
    static Error AppendTo(Message &aMessage, uint32_t aChannelMask);

private:
    static constexpr uint8_t kMaskLength = sizeof(uint32_t);

    OT_TOOL_PACKED_BEGIN
    class Entry
    {
    public:
        uint8_t  GetChannelPage(void) const { return mChannelPage; }
        void     SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }
        uint8_t  GetMaskLength(void) const { return mMaskLength; }
        void     SetMaskLength(uint8_t aMaskLength) { mMaskLength = aMaskLength; }
        uint32_t GetMask(void) const { return Reverse32(BigEndian::HostSwap32(mMask)); }
        void     SetMask(uint32_t aMask) { mMask = BigEndian::HostSwap32(Reverse32(aMask)); }

    private:
        uint8_t  mChannelPage;
        uint8_t  mMaskLength;
        uint32_t mMask;
    } OT_TOOL_PACKED_END;

    struct EntriesData : public Clearable<EntriesData>
    {
        // Represents received Channel Mask TLV Entries data which
        // is either contained in `mData` buffer, or in `mMessage`
        // at `mOffset`.

        Error Parse(uint32_t &aChannelMask);

        const uint8_t *mData;
        const Message *mMessage;
        uint16_t       mOffset;
        uint16_t       mLength;
    };

    uint8_t mEntriesStart;
} OT_TOOL_PACKED_BEGIN;

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
    uint32_t GetOui(void) const { return BigEndian::ReadUint24(mOui); }

    /**
     * Returns the Stack Vendor OUI value.
     *
     * @param[in]  aOui  The Vendor Stack Vendor OUI value.
     *
     */
    void SetOui(uint32_t aOui) { BigEndian::WriteUint24(aOui, mOui); }

    /**
     * Returns the Build value.
     *
     * @returns The Build value.
     *
     */
    uint16_t GetBuild(void) const { return (BigEndian::HostSwap16(mBuildRevision) & kBuildMask) >> kBuildOffset; }

    /**
     * Sets the Build value.
     *
     * @param[in]  aBuild  The Build value.
     *
     */
    void SetBuild(uint16_t aBuild)
    {
        mBuildRevision = BigEndian::HostSwap16((BigEndian::HostSwap16(mBuildRevision) & ~kBuildMask) |
                                               ((aBuild << kBuildOffset) & kBuildMask));
    }

    /**
     * Returns the Revision value.
     *
     * @returns The Revision value.
     *
     */
    uint8_t GetRevision(void) const { return (BigEndian::HostSwap16(mBuildRevision) & kRevMask) >> kRevOffset; }

    /**
     * Sets the Revision value.
     *
     * @param[in]  aRevision  The Revision value.
     *
     */
    void SetRevision(uint8_t aRevision)
    {
        mBuildRevision = BigEndian::HostSwap16((BigEndian::HostSwap16(mBuildRevision) & ~kRevMask) |
                                               ((aRevision << kRevOffset) & kRevMask));
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
    uint16_t GetSourcePort(void) const { return BigEndian::HostSwap16(mSourcePort); }

    /**
     * Updates the source port.
     *
     * @param[in]   aSourcePort     The source port.
     *
     */
    void SetSourcePort(uint16_t aSourcePort) { mSourcePort = BigEndian::HostSwap16(aSourcePort); }

    /**
     * Returns the destination port.
     *
     * @returns The destination port.
     *
     */
    uint16_t GetDestinationPort(void) const { return BigEndian::HostSwap16(mDestinationPort); }

    /**
     * Updates the destination port.
     *
     * @param[in]   aDestinationPort    The destination port.
     *
     */
    void SetDestinationPort(uint16_t aDestinationPort) { mDestinationPort = BigEndian::HostSwap16(aDestinationPort); }

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
    bool IsCommercialCommissioningMode(void) const { return (mFlags & kCcmMask) != 0; }

    /**
     * Sets the Commercial Commissioning Mode flag.
     *
     * @param[in]  aCcm  TRUE if set, FALSE otherwise.
     *
     */
    void SetCommercialCommissioningMode(bool aCcm)
    {
        if (aCcm)
        {
            mFlags |= kCcmMask;
        }
        else
        {
            mFlags &= ~kCcmMask;
        }
    }

private:
    static constexpr uint8_t kVersionOffset = 4;
    static constexpr uint8_t kVersionMask   = 0xf << kVersionOffset;
    static constexpr uint8_t kNativeOffset  = 3;
    static constexpr uint8_t kNativeMask    = 1 << kNativeOffset;
    static constexpr uint8_t kCcmOffset     = 2;
    static constexpr uint8_t kCcmMask       = 1 << kCcmOffset;

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
    uint32_t GetOui(void) const { return BigEndian::ReadUint24(mOui); }

    /**
     * Sets the Vendor OUI value.
     *
     * @param[in]  aOui The Vendor OUI value.
     *
     */
    void SetOui(uint32_t aOui) { return BigEndian::WriteUint24(aOui, mOui); }

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
