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
 *   This file includes definitions for generating and processing Network Diagnostics TLVs.
 */

#ifndef OT_CORE_THREAD_NETWORK_DIAGNOSTIC_TLVS_HPP_
#define OT_CORE_THREAD_NETWORK_DIAGNOSTIC_TLVS_HPP_

#include "openthread-core-config.h"

#include <openthread/netdiag.h>
#include <openthread/thread.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6_address.hpp"
#include "radio/radio.hpp"
#include "thread/child.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/router.hpp"

namespace ot {
namespace NetworkDiagnostic {

/**
 * Implements Network Diagnostic TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * Network Diagnostic TLV Types.
     */
    enum Type : uint8_t
    {
        kExtMacAddress         = OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS,
        kAddress16             = OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS,
        kMode                  = OT_NETWORK_DIAGNOSTIC_TLV_MODE,
        kTimeout               = OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT,
        kConnectivity          = OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY,
        kRoute                 = OT_NETWORK_DIAGNOSTIC_TLV_ROUTE,
        kLeaderData            = OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA,
        kNetworkData           = OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA,
        kIp6AddressList        = OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST,
        kMacCounters           = OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS,
        kBatteryLevel          = OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL,
        kSupplyVoltage         = OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE,
        kChildTable            = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE,
        kChannelPages          = OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES,
        kTypeList              = OT_NETWORK_DIAGNOSTIC_TLV_TYPE_LIST,
        kMaxChildTimeout       = OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT,
        kEui64                 = OT_NETWORK_DIAGNOSTIC_TLV_EUI64,
        kVersion               = OT_NETWORK_DIAGNOSTIC_TLV_VERSION,
        kVendorName            = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME,
        kVendorModel           = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL,
        kVendorSwVersion       = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION,
        kThreadStackVersion    = OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION,
        kChild                 = OT_NETWORK_DIAGNOSTIC_TLV_CHILD,
        kChildIp6AddressList   = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_IP6_ADDR_LIST,
        kRouterNeighbor        = OT_NETWORK_DIAGNOSTIC_TLV_ROUTER_NEIGHBOR,
        kAnswer                = OT_NETWORK_DIAGNOSTIC_TLV_ANSWER,
        kQueryId               = OT_NETWORK_DIAGNOSTIC_TLV_QUERY_ID,
        kMleCounters           = OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS,
        kVendorAppUrl          = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL,
        kNonPreferredChannels  = OT_NETWORK_DIAGNOSTIC_TLV_NON_PREFERRED_CHANNELS,
        kEnhancedRoute         = OT_NETWORK_DIAGNOSTIC_TLV_ENHANCED_ROUTE,
        kBrState               = OT_NETWORK_DIAGNOSTIC_TLV_BR_STATE,
        kBrIfAddrs             = OT_NETWORK_DIAGNOSTIC_TLV_BR_IF_ADDRS,
        kBrLocalOmrPrefix      = OT_NETWORK_DIAGNOSTIC_TLV_BR_LOCAL_OMR_PREFIX,
        kBrDhcp6PdOmrPrefix    = OT_NETWORK_DIAGNOSTIC_TLV_BR_DHCP6_PD_OMR_PREFIX,
        kBrLocalOnlinkPrefix   = OT_NETWORK_DIAGNOSTIC_TLV_BR_LOCAL_OL_PREFIX,
        kBrFavoredOnLinkPrefix = OT_NETWORK_DIAGNOSTIC_TLV_BR_FAVORED_OL_PREFIX,
    };

    /**
     * Maximum length of Vendor Name TLV.
     */
    static constexpr uint8_t kMaxVendorNameLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH;

    /**
     * Maximum length of Vendor Model TLV.
     */
    static constexpr uint8_t kMaxVendorModelLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH;

    /**
     * Maximum length of Vendor SW Version TLV.
     */
    static constexpr uint8_t kMaxVendorSwVersionLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH;

    /**
     * Maximum length of Vendor SW Version TLV.
     */
    static constexpr uint8_t kMaxThreadStackVersionLength = OT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH;

    /**
     * Maximum length of Vendor SW Version TLV.
     */
    static constexpr uint8_t kMaxVendorAppUrlLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH;

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
 * Defines Extended MAC Address TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kExtMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

/**
 * Defines Address16 TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kAddress16, uint16_t> Address16Tlv;

/**
 * Defines Mode TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * Defines Network Data TLV constants and types.
 */
typedef TlvInfo<Tlv::kNetworkData> NetworkDataTlv;

/**
 * Defines IPv6 Address List TLV constants and types.
 */
typedef TlvInfo<Tlv::kIp6AddressList> Ip6AddressListTlv;

/**
 * Defines Battery Level TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kBatteryLevel, uint8_t> BatteryLevelTlv;

/**
 * Defines Supply Voltage TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kSupplyVoltage, uint16_t> SupplyVoltageTlv;

/**
 * Defines Child Table TLV constants and types.
 */
typedef TlvInfo<Tlv::kChildTable> ChildTableTlv;

/**
 * Defines Max Child Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMaxChildTimeout, uint32_t> MaxChildTimeoutTlv;

/**
 * Defines Eui64 TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kEui64, Mac::ExtAddress> Eui64Tlv;

/**
 * Defines Version TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kVersion, uint16_t> VersionTlv;

/**
 * Defines Vendor Name TLV constants and types.
 */
typedef StringTlvInfo<Tlv::kVendorName, Tlv::kMaxVendorNameLength> VendorNameTlv;

/**
 * Defines Vendor Model TLV constants and types.
 */
typedef StringTlvInfo<Tlv::kVendorModel, Tlv::kMaxVendorModelLength> VendorModelTlv;

/**
 * Defines Vendor SW Version TLV constants and types.
 */
typedef StringTlvInfo<Tlv::kVendorSwVersion, Tlv::kMaxVendorSwVersionLength> VendorSwVersionTlv;

/**
 * Defines Thread Stack Version TLV constants and types.
 */
typedef StringTlvInfo<Tlv::kThreadStackVersion, Tlv::kMaxThreadStackVersionLength> ThreadStackVersionTlv;

/**
 * Defines Vendor App URL TLV constants and types.
 */
typedef StringTlvInfo<Tlv::kVendorAppUrl, Tlv::kMaxVendorAppUrlLength> VendorAppUrlTlv;

/**
 * Defines Child IPv6 Address List TLV constants and types.
 */
typedef TlvInfo<Tlv::kChildIp6AddressList> ChildIp6AddressListTlv;

/**
 * Defines Query ID TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kQueryId, uint16_t> QueryIdTlv;

/**
 * Defines BR State TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kBrState, uint8_t> BrStateTlv;

/**
 * Represents a BR State TLV value.
 */
typedef otNetworkDiagBrState BrState;

/**
 * Defines BR Infra-if Address List TLV constants and types.
 */
typedef TlvInfo<Tlv::kBrIfAddrs> BrIfAddrsTlv;

/**
 * Defines BR Local OMR Prefix TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kBrLocalOmrPrefix, Ip6::NetworkPrefix> BrLocalOmrPrefixTlv;

/**
 * Defines BR DHCPv6-PD Prefix TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kBrDhcp6PdOmrPrefix, Ip6::NetworkPrefix> BrDhcp6PdOmrPrefixTlv;

/**
 * Defines BR Local On-link Prefix TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kBrLocalOnlinkPrefix, Ip6::NetworkPrefix> BrLocalOnlinkPrefixTlv;

/**
 * Defines BR Favored On-link Prefix TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kBrFavoredOnLinkPrefix, Ip6::NetworkPrefix> BrFavoredOnLinkPrefixTlv;

/**
 * Represents information parsed from Connectivity TLV.
 */
typedef Mle::Connectivity Connectivity;

/**
 * Represents a Connectivity TLV value.
 */
typedef Mle::ConnectivityTlvValue ConnectivityTlvValue;

/**
 * Defines Connectivity TLV constants.
 */
typedef TlvInfo<Tlv::kConnectivity> ConnectivityTlv;

/**
 * Implements Route TLV generation and parsing.
 */
class RouteTlv : public Mle::RouteTlv
{
public:
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kRoute; ///< The TLV Type value.

    /**
     * Finds and parses a Route TLV in a given message.
     *
     * @param[in]  aMessage  The message to search within.
     * @param[out] aData     A reference to a `Data` to populate upon success.
     *
     * @retval kErrorNone       Successfully found and parsed the Route TLV.
     * @retval kErrorNotFound   Could not find a Route TLV in @p aMessage.
     * @retval kErrorParse      Found the Route TLV but failed to parse it.
     */
    static Error FindIn(const Message &aMessage, Data &aData);
};

/**
 * Represents a Leader Data TLV value.
 */
typedef Mle::LeaderDataTlvValue LeaderDataTlvValue;

/**
 * Defines Leader Data TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kLeaderData, LeaderDataTlvValue> LeaderDataTlv;

/**
 * Represents the Mac Counters.
 */
typedef otNetworkDiagMacCounters MacCounters;

/**
 * Implements Mac Counters TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public Tlv, public TlvInfo<Tlv::kMacCounters>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aMacCounters    The MAC counters to initialize the TLV with.
     */
    void Init(const Mac::Counters &aMacCounters);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Reads the counters from TLV.
     *
     * @param[out] aDiagMacCounters   A reference to `NetworkDiagnostic::MacCounters` to populate.
     */
    void Read(MacCounters &aDiagMacCounters) const;

private:
    uint32_t mIfInUnknownProtos;
    uint32_t mIfInErrors;
    uint32_t mIfOutErrors;
    uint32_t mIfInUcastPkts;
    uint32_t mIfInBroadcastPkts;
    uint32_t mIfInDiscards;
    uint32_t mIfOutUcastPkts;
    uint32_t mIfOutBroadcastPkts;
    uint32_t mIfOutDiscards;
} OT_TOOL_PACKED_END;

/**
 * Implements Child Table TLV Entry generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class ChildTableTlvEntry : public Clearable<ChildTableTlvEntry>
{
public:
    typedef otNetworkDiagChildEntry ParseInfo; ///< Parse entry info

#if OPENTHREAD_FTD
    /**
     * Initializes the `ChildTableTlvEntry` from a given `Child` object.
     *
     * @param[in] aChild  The `Child` to initialize from.
     */
    void InitFrom(const Child &aChild);
#endif

    /**
     * Parses the TLV entry and populates the information in a given `ParseInfo` struct.
     *
     * @param[out] aParseInfo   The `ParseInfo` structure to populate.
     */
    void Parse(ParseInfo &aParseInfo) const;

    /**
     * Determines the timeout value (in seconds) from a given exponent.
     *
     * The timeout is expressed as `2^(exponent - 4)` seconds.
     *
     * @param[in] aExponent  The exponent value.
     *
     * @returns The timeout value (in seconds).
     */
    static uint32_t DetermineTimeoutFromExponent(uint8_t aExponent);

#if OPENTHREAD_FTD
    /**
     * Determines the exponent to use for a given timeout value (in seconds).
     *
     * @param[in] aTimeout  The timeout value (in seconds).
     *
     * @returns The corresponding exponent.
     */
    static uint8_t DetermineExponentFromTimeout(uint32_t aTimeout);
#endif

private:
    //             1                   0
    //   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |TmoutExp |ILQ|     Child ID    |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t  kTimeoutOffset      = 11;
    static constexpr uint8_t  kIlqOffset          = 9;
    static constexpr uint8_t  kChildIdOffset      = 0;
    static constexpr uint16_t kTimeoutMask        = 0x1f << kTimeoutOffset;
    static constexpr uint16_t kIlqMask            = 0x3 << kIlqOffset;
    static constexpr uint16_t kChildIdMask        = 0x1ff << kChildIdOffset;
    static constexpr uint8_t  kTimeoutExponentMin = 4;
    static constexpr uint8_t  kTimeoutExponentMax = 0x1f;

    uint16_t mTimeoutIlqChildId;
    uint8_t  mMode;
} OT_TOOL_PACKED_END;

/**
 * Defines Channel Pages TLV constants and types.
 */
typedef TlvInfo<Tlv::kChannelPages> ChannelPagesTlv;

/**
 * Defines Type List TLV constants and types.
 */
typedef TlvInfo<Tlv::kTypeList> TypeListTlv;

#if OPENTHREAD_FTD

/**
 * Implements Child TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class ChildTlvValue : public Clearable<ChildTlvValue>
{
public:
    static constexpr uint8_t kFlagsRxOnWhenIdle = 1 << 7; ///< Device mode - Rx-on when idle.
    static constexpr uint8_t kFlagsFtd          = 1 << 6; ///< Device mode - Full Thread Device (FTD).
    static constexpr uint8_t kFlagsFullNetdta   = 1 << 5; ///< Device mode - Full Network Data.
    static constexpr uint8_t kFlagsCslSync      = 1 << 4; ///< Is CSL capable and CSL synchronized.
    static constexpr uint8_t kFlagsTrackErrRate = 1 << 3; ///< Supports tracking error rates.

    /**
     * Initializes the TLV using information from a given `Child`.
     *
     * @param[in] aChild   The child to initialize the TLV from.
     */
    void InitFrom(const Child &aChild);

    /**
     * Returns the Flags field (`kFlags*` constants define bits in flags).
     *
     * @returns The Flags field.
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * Returns the RLOC16 field.
     *
     * @returns The RLOC16 of the child.
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Returns the Extended Address.
     *
     * @returns The Extended Address of the child.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Returns the Version field.
     *
     * @returns The Version of the child.
     */
    uint16_t GetVersion(void) const { return BigEndian::HostSwap16(mVersion); }

    /**
     * Returns the Timeout field
     *
     * @returns The Timeout value in seconds.
     */
    uint32_t GetTimeout(void) const { return BigEndian::HostSwap32(mTimeout); }

    /**
     * Returns the Age field.
     *
     * @returns The Age field (seconds since last heard from the child).
     */
    uint32_t GetAge(void) const { return BigEndian::HostSwap32(mAge); }

    /**
     * Returns the Connection Time field.
     *
     * @returns The Connection Time field (seconds since attach).
     */
    uint32_t GetConnectionTime(void) const { return BigEndian::HostSwap32(mConnectionTime); }

    /**
     * Returns the Supervision Interval field
     *
     * @returns The Supervision Interval in seconds. Zero indicates not used.
     */
    uint16_t GetSupervisionInterval(void) const { return BigEndian::HostSwap16(mSupervisionInterval); }

    /**
     * Returns the Link Margin field.
     *
     * @returns The Link Margin in dB.
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * Returns the Average RSSI field.
     *
     * @returns The Average RSSI in dBm. 127 if not available or unknown.
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * Returns the Last RSSI field (RSSI of last received frame from child).
     *
     * @returns The Last RSSI field in dBm. 127 if not available or unknown.
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * Returns the Frame Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Frame Error Rate (0x0000->0%, 0xffff->100%).
     */
    uint16_t GetFrameErrorRate(void) const { return BigEndian::HostSwap16(mFrameErrorRate); }

    /**
     * Returns the Message Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Message Error Rate (0x0000->0%, 0xffff->100%).
     */
    uint16_t GetMessageErrorRate(void) const { return BigEndian::HostSwap16(mMessageErrorRate); }

    /**
     * Returns the Queued Message Count field.
     *
     * @returns The Queued Message Count (number of queued messages for indirect tx to child).
     */
    uint16_t GetQueuedMessageCount(void) const { return BigEndian::HostSwap16(mQueuedMessageCount); }

    /**
     * Returns the CSL Period in unit of 10 symbols.
     *
     * @returns The CSL Period in unit of 10-symbols-time. Zero if CSL is not supported.
     */
    uint16_t GetCslPeriod(void) const { return BigEndian::HostSwap16(mCslPeriod); }

    /**
     * Returns the CSL Timeout in seconds.
     *
     * @returns The CSL Timeout in seconds. Zero if unknown on parent of if CSL Is not supported.
     */
    uint32_t GetCslTimeout(void) const { return BigEndian::HostSwap32(mCslTimeout); }

    /**
     * Returns the CSL Channel.
     *
     * @returns The CSL channel.
     */
    uint8_t GetCslChannel(void) const { return mCslChannel; }

private:
    uint8_t         mFlags;               // Flags (`kFlags*` constants).
    uint16_t        mRloc16;              // RLOC16.
    Mac::ExtAddress mExtAddress;          // Extended Address.
    uint16_t        mVersion;             // Version.
    uint32_t        mTimeout;             // Timeout in seconds.
    uint32_t        mAge;                 // Seconds since last heard from the child.
    uint32_t        mConnectionTime;      // Seconds since attach.
    uint16_t        mSupervisionInterval; // Supervision interval in seconds. Zero to indicate not used.
    uint8_t         mLinkMargin;          // Link margin in dB.
    int8_t          mAverageRssi;         // Average RSSI. 127 if not available or unknown
    int8_t          mLastRssi;            // RSSI of last received frame. 127 if not available or unknown.
    uint16_t        mFrameErrorRate;      // Frame error rate (0x0000->0%, 0xffff->100%).
    uint16_t        mMessageErrorRate;    // (IPv6) msg error rate (0x0000->0%, 0xffff->100%)
    uint16_t        mQueuedMessageCount;  // Number of queued messages for indirect tx to child.
    uint16_t        mCslPeriod;           // CSL Period in unit of 10 symbols.
    uint32_t        mCslTimeout;          // CSL Timeout in seconds.
    uint8_t         mCslChannel;          // CSL channel.
} OT_TOOL_PACKED_END;

/**
 * Defines Child TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kChild, ChildTlvValue> ChildTlv;

/**
 * Implements Child IPv6 Address List Value generation and parsing.
 *
 * This TLV can use extended or normal format depending on the number of IPv6 addresses.
 */
OT_TOOL_PACKED_BEGIN
class ChildIp6AddressListTlvValue
{
public:
    /**
     * Returns the RLOC16 of the child.
     *
     * @returns The RLOC16 of the child.
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Sets the RLOC16.
     *
     * @param[in] aRloc16   The RLOC16 value.
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = BigEndian::HostSwap16(aRloc16); }

private:
    uint16_t mRloc16;
    // Followed by zero or more IPv6 address(es).

} OT_TOOL_PACKED_END;

/**
 * Implements Router Neighbor TLV Value generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class RouterNeighborTlvValue : public Clearable<RouterNeighborTlvValue>
{
public:
    static constexpr uint8_t kFlagsTrackErrRate = 1 << 7; ///< Supports tracking error rates.

    /**
     * Initializes the TLV using information from a given `Router`.
     *
     * @param[in] aRouter   The router to initialize the TLV from.
     */
    void InitFrom(const Router &aRouter);

    /**
     * Returns the Flags field (`kFlags*` constants define bits in flags).
     *
     * @returns The Flags field.
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * Returns the RLOC16 field.
     *
     * @returns The RLOC16 of the router.
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Returns the Extended Address.
     *
     * @returns The Extended Address of the router.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Returns the Version field.
     *
     * @returns The Version of the router.
     */
    uint16_t GetVersion(void) const { return BigEndian::HostSwap16(mVersion); }

    /**
     * Returns the Connection Time field.
     *
     * @returns The Connection Time field (seconds since link establishment).
     */
    uint32_t GetConnectionTime(void) const { return BigEndian::HostSwap32(mConnectionTime); }

    /**
     * Returns the Link Margin field.
     *
     * @returns The Link Margin in dB.
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * Returns the Average RSSI field.
     *
     * @returns The Average RSSI in dBm. 127 if not available or unknown.
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * Returns the Last RSSI field (RSSI of last received frame from router).
     *
     * @returns The Last RSSI field in dBm. 127 if not available or unknown.
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * Returns the Frame Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Frame Error Rate (0x0000->0%, 0xffff->100%).
     */
    uint16_t GetFrameErrorRate(void) const { return BigEndian::HostSwap16(mFrameErrorRate); }

    /**
     * Returns the Message Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Message Error Rate (0x0000->0%, 0xffff->100%).
     */
    uint16_t GetMessageErrorRate(void) const { return BigEndian::HostSwap16(mMessageErrorRate); }

private:
    uint8_t         mFlags;            // Flags (`kFlags*` constants).
    uint16_t        mRloc16;           // RLOC16.
    Mac::ExtAddress mExtAddress;       // Extended Address.
    uint16_t        mVersion;          // Version.
    uint32_t        mConnectionTime;   // Seconds since link establishment.
    uint8_t         mLinkMargin;       // Link Margin.
    int8_t          mAverageRssi;      // Average RSSI. 127 if not available or unknown
    int8_t          mLastRssi;         // RSSI of last received frame. 127 if not available or unknown.
    uint16_t        mFrameErrorRate;   // Frame error rate (0x0000->0%, 0xffff->100%).
    uint16_t        mMessageErrorRate; // (IPv6) msg error rate (0x0000->0%, 0xffff->100%)
} OT_TOOL_PACKED_END;

/**
 * Defines Router Neighbor TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kRouterNeighbor, RouterNeighborTlvValue> RouterNeighborTlv;

#endif // OPENTHREAD_FTD

/**
 * Represents an Enhanced Route TLV Entry
 */
OT_TOOL_PACKED_BEGIN
class EnhancedRouteTlvEntry
{
public:
    typedef otNetworkDiagEnhRouteData ParseInfo; ///< Parse entry info

    /**
     * Initializes the entry as self (associated with device itself).
     */
    void InitAsSelf(void) { SetRouteData(kSelfFlag); }

    /**
     * Initializes the entry from given `router`.
     *
     * @param[in] aRouter  Router entry to use for initialization.
     */
    void InitFrom(const Router &aRouter);

    /**
     * Parses the entry and populate the information in given `ParseInfo` struct.
     *
     * @parma[out] aParseInfo   The `ParseInfo` structure to populate.
     */
    void Parse(ParseInfo &aParseInfo) const;

private:
    // Format:
    //
    //  15  14  13   12  11  10  9   8   7   6   5   4   3   2   1   0
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | S | L | LQOut | LQIn  |     NextHop (6-bit)   | NHCost(4 bits)|
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

    static constexpr uint16_t kSelfFlag             = 1 << 15;
    static constexpr uint16_t kLinkFlag             = 1 << 14;
    static constexpr uint8_t  kLinkQualityOutOffset = 12;
    static constexpr uint8_t  kLinkQualityInOffset  = 10;
    static constexpr uint8_t  kNextHopOffset        = 4;
    static constexpr uint8_t  kNextHopCostOffset    = 0;

    static constexpr uint16_t kLinkQualityMask = 0x3;
    static constexpr uint16_t kNextHopMask     = 0x3f;
    static constexpr uint16_t kCostMask        = 0xf;

    uint16_t GetRouteData(void) const { return BigEndian::HostSwap16(mRouteData); }
    void     SetRouteData(uint16_t aRouteData) { mRouteData = BigEndian::HostSwap16(aRouteData); }

    uint16_t mRouteData;
} OT_TOOL_PACKED_END;

/**
 * Represents an Answer TLV value.
 */
OT_TOOL_PACKED_BEGIN
class AnswerTlvValue
{
public:
    enum IsLastFlag : uint8_t
    {
        kMoreToFollow, ///< More answer messages to follow.
        kIsLast,       ///< This is the last answer for this query.
    };

    /**
     * Initializes the TLV value.
     *
     * @param[in] aIndex       The index value.
     * @param[in] aIsLastFlag  Indicates the `IsLastFlag` value.
     */
    void Init(uint16_t aIndex, IsLastFlag aIsLastFlag);

    /**
     * Indicates whether or not the "IsLast" flag is set
     *
     * @retval TRUE   "IsLast" flag is set (this is the last answer for this query).
     * @retval FALSE  "IsLast" flag is not set (more answer messages are expected for this query).
     */
    bool IsLast(void) const { return GetFlagsIndex() & kIsLastFlag; }

    /**
     * Gets the index.
     *
     * @returns The index.
     */
    uint16_t GetIndex(void) const { return GetFlagsIndex() & kIndexMask; }

private:
    static constexpr uint16_t kIsLastFlag = 1 << 15;
    static constexpr uint16_t kIndexMask  = 0x7fff;

    uint16_t GetFlagsIndex(void) const { return BigEndian::HostSwap16(mFlagsIndex); }
    void     SetFlagsIndex(uint16_t aFlagsIndex) { mFlagsIndex = BigEndian::HostSwap16(aFlagsIndex); }

    uint16_t mFlagsIndex;
} OT_TOOL_PACKED_END;

/**
 * Defines Answer TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kAnswer, AnswerTlvValue> AnswerTlv;

/**
 * Represents the MLE Counters.
 */
typedef otNetworkDiagMleCounters MleCounters;

/**
 * Implements MLE Counters TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class MleCountersTlv : public Tlv, public TlvInfo<Tlv::kMleCounters>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aMleCounters    The MLE counters to initialize the TLV with.
     */
    void Init(const Mle::Counters &aMleCounters);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     *
     * Reads the counters from TLV.
     *
     * @param[out] aDiagMleCounters   A reference to `NetworkDiagnostic::MleCounters` to populate.
     */
    void Read(MleCounters &aDiagMleCounters) const;

private:
    uint16_t mDisabledRole;                  // Number of times device entered disabled role.
    uint16_t mDetachedRole;                  // Number of times device entered detached role.
    uint16_t mChildRole;                     // Number of times device entered child role.
    uint16_t mRouterRole;                    // Number of times device entered router role.
    uint16_t mLeaderRole;                    // Number of times device entered leader role.
    uint16_t mAttachAttempts;                // Number of attach attempts while device was detached.
    uint16_t mPartitionIdChanges;            // Number of changes to partition ID.
    uint16_t mBetterPartitionAttachAttempts; // Number of attempts to attach to a better partition.
    uint16_t mParentChanges;                 // Number of time device changed its parent.
    uint64_t mTrackedTime;                   // Milliseconds tracked by next counters.
    uint64_t mDisabledTime;                  // Milliseconds device has been in disabled role.
    uint64_t mDetachedTime;                  // Milliseconds device has been in detached role.
    uint64_t mChildTime;                     // Milliseconds device has been in child role.
    uint64_t mRouterTime;                    // Milliseconds device has been in router role.
    uint64_t mLeaderTime;                    // Milliseconds device has been in leader role.
} OT_TOOL_PACKED_END;

} // namespace NetworkDiagnostic

DefineCoreType(otNetworkDiagConnectivity, NetworkDiagnostic::Connectivity);

} // namespace ot

#endif // OT_CORE_THREAD_NETWORK_DIAGNOSTIC_TLVS_HPP_
