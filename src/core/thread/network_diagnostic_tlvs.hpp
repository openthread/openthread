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

#ifndef NETWORK_DIAGNOSTIC_TLVS_HPP_
#define NETWORK_DIAGNOSTIC_TLVS_HPP_

#include "openthread-core-config.h"

#include <openthread/netdiag.h>
#include <openthread/thread.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
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
 *
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * Network Diagnostic TLV Types.
     *
     */
    enum Type : uint8_t
    {
        kExtMacAddress       = OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS,
        kAddress16           = OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS,
        kMode                = OT_NETWORK_DIAGNOSTIC_TLV_MODE,
        kTimeout             = OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT,
        kConnectivity        = OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY,
        kRoute               = OT_NETWORK_DIAGNOSTIC_TLV_ROUTE,
        kLeaderData          = OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA,
        kNetworkData         = OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA,
        kIp6AddressList      = OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST,
        kMacCounters         = OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS,
        kBatteryLevel        = OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL,
        kSupplyVoltage       = OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE,
        kChildTable          = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE,
        kChannelPages        = OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES,
        kTypeList            = OT_NETWORK_DIAGNOSTIC_TLV_TYPE_LIST,
        kMaxChildTimeout     = OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT,
        kEui64               = OT_NETWORK_DIAGNOSTIC_TLV_EUI64,
        kVersion             = OT_NETWORK_DIAGNOSTIC_TLV_VERSION,
        kVendorName          = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME,
        kVendorModel         = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL,
        kVendorSwVersion     = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION,
        kThreadStackVersion  = OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION,
        kChild               = OT_NETWORK_DIAGNOSTIC_TLV_CHILD,
        kChildIp6AddressList = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_IP6_ADDR_LIST,
        kRouterNeighbor      = OT_NETWORK_DIAGNOSTIC_TLV_ROUTER_NEIGHBOR,
        kAnswer              = OT_NETWORK_DIAGNOSTIC_TLV_ANSWER,
        kQueryId             = OT_NETWORK_DIAGNOSTIC_TLV_QUERY_ID,
        kMleCounters         = OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS,
    };

    /**
     * Maximum length of Vendor Name TLV.
     *
     */
    static constexpr uint8_t kMaxVendorNameLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH;

    /**
     * Maximum length of Vendor Model TLV.
     *
     */
    static constexpr uint8_t kMaxVendorModelLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH;

    /**
     * Maximum length of Vendor SW Version TLV.
     *
     */
    static constexpr uint8_t kMaxVendorSwVersionLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH;

    /**
     * Maximum length of Vendor SW Version TLV.
     *
     */
    static constexpr uint8_t kMaxThreadStackVersionLength = OT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH;

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

} OT_TOOL_PACKED_END;

/**
 * Defines Extended MAC Address TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kExtMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

/**
 * Defines Address16 TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kAddress16, uint16_t> Address16Tlv;

/**
 * Defines Mode TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * Defines Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * Defines Network Data TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kNetworkData> NetworkDataTlv;

/**
 * Defines IPv6 Address List TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kIp6AddressList> Ip6AddressListTlv;

/**
 * Defines Battery Level TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kBatteryLevel, uint8_t> BatteryLevelTlv;

/**
 * Defines Supply Voltage TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kSupplyVoltage, uint16_t> SupplyVoltageTlv;

/**
 * Defines Child Table TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kChildTable> ChildTableTlv;

/**
 * Defines Max Child Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kMaxChildTimeout, uint32_t> MaxChildTimeoutTlv;

/**
 * Defines Eui64 TLV constants and types.
 *
 */
typedef SimpleTlvInfo<Tlv::kEui64, Mac::ExtAddress> Eui64Tlv;

/**
 * Defines Version TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kVersion, uint16_t> VersionTlv;

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
 * Defines Thread Stack Version TLV constants and types.
 *
 */
typedef StringTlvInfo<Tlv::kThreadStackVersion, Tlv::kMaxThreadStackVersionLength> ThreadStackVersionTlv;

/**
 * Defines Child IPv6 Address List TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kChildIp6AddressList> ChildIp6AddressListTlv;

/**
 * Defines Query ID TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kQueryId, uint16_t> QueryIdTlv;

typedef otNetworkDiagConnectivity Connectivity; ///< Network Diagnostic Connectivity value.

/**
 * Implements Connectivity TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlv : public Mle::ConnectivityTlv
{
public:
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kConnectivity; ///< The TLV Type value.

    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::ConnectivityTlv::Init();
        ot::Tlv::SetType(kType);
    }

    /**
     * Retrieves the `Connectivity` value.
     *
     * @param[out] aConnectivity   A reference to `Connectivity` to populate.
     *
     */
    void GetConnectivity(Connectivity &aConnectivity) const
    {
        aConnectivity.mParentPriority   = GetParentPriority();
        aConnectivity.mLinkQuality3     = GetLinkQuality3();
        aConnectivity.mLinkQuality2     = GetLinkQuality2();
        aConnectivity.mLinkQuality1     = GetLinkQuality1();
        aConnectivity.mLeaderCost       = GetLeaderCost();
        aConnectivity.mIdSequence       = GetIdSequence();
        aConnectivity.mActiveRouters    = GetActiveRouters();
        aConnectivity.mSedBufferSize    = GetSedBufferSize();
        aConnectivity.mSedDatagramCount = GetSedDatagramCount();
    }

} OT_TOOL_PACKED_END;

/**
 * Implements Route TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Mle::RouteTlv
{
public:
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kRoute; ///< The TLV Type value.

    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::RouteTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * Implements Leader Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LeaderDataTlv : public Mle::LeaderDataTlv
{
public:
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kLeaderData; ///< The TLV Type value.

    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::LeaderDataTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * Implements Mac Counters TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public Tlv, public TlvInfo<Tlv::kMacCounters>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMacCounters);
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
     * Returns the IfInUnknownProtos counter.
     *
     * @returns The IfInUnknownProtos counter
     *
     */
    uint32_t GetIfInUnknownProtos(void) const { return BigEndian::HostSwap32(mIfInUnknownProtos); }

    /**
     * Sets the IfInUnknownProtos counter.
     *
     * @param[in]  aIfInUnknownProtos The IfInUnknownProtos counter
     *
     */
    void SetIfInUnknownProtos(const uint32_t aIfInUnknownProtos)
    {
        mIfInUnknownProtos = BigEndian::HostSwap32(aIfInUnknownProtos);
    }

    /**
     * Returns the IfInErrors counter.
     *
     * @returns The IfInErrors counter
     *
     */
    uint32_t GetIfInErrors(void) const { return BigEndian::HostSwap32(mIfInErrors); }

    /**
     * Sets the IfInErrors counter.
     *
     * @param[in]  aIfInErrors The IfInErrors counter
     *
     */
    void SetIfInErrors(const uint32_t aIfInErrors) { mIfInErrors = BigEndian::HostSwap32(aIfInErrors); }

    /**
     * Returns the IfOutErrors counter.
     *
     * @returns The IfOutErrors counter
     *
     */
    uint32_t GetIfOutErrors(void) const { return BigEndian::HostSwap32(mIfOutErrors); }

    /**
     * Sets the IfOutErrors counter.
     *
     * @param[in]  aIfOutErrors The IfOutErrors counter.
     *
     */
    void SetIfOutErrors(const uint32_t aIfOutErrors) { mIfOutErrors = BigEndian::HostSwap32(aIfOutErrors); }

    /**
     * Returns the IfInUcastPkts counter.
     *
     * @returns The IfInUcastPkts counter
     *
     */
    uint32_t GetIfInUcastPkts(void) const { return BigEndian::HostSwap32(mIfInUcastPkts); }

    /**
     * Sets the IfInUcastPkts counter.
     *
     * @param[in]  aIfInUcastPkts The IfInUcastPkts counter.
     *
     */
    void SetIfInUcastPkts(const uint32_t aIfInUcastPkts) { mIfInUcastPkts = BigEndian::HostSwap32(aIfInUcastPkts); }
    /**
     * Returns the IfInBroadcastPkts counter.
     *
     * @returns The IfInBroadcastPkts counter
     *
     */
    uint32_t GetIfInBroadcastPkts(void) const { return BigEndian::HostSwap32(mIfInBroadcastPkts); }

    /**
     * Sets the IfInBroadcastPkts counter.
     *
     * @param[in]  aIfInBroadcastPkts The IfInBroadcastPkts counter.
     *
     */
    void SetIfInBroadcastPkts(const uint32_t aIfInBroadcastPkts)
    {
        mIfInBroadcastPkts = BigEndian::HostSwap32(aIfInBroadcastPkts);
    }

    /**
     * Returns the IfInDiscards counter.
     *
     * @returns The IfInDiscards counter
     *
     */
    uint32_t GetIfInDiscards(void) const { return BigEndian::HostSwap32(mIfInDiscards); }

    /**
     * Sets the IfInDiscards counter.
     *
     * @param[in]  aIfInDiscards The IfInDiscards counter.
     *
     */
    void SetIfInDiscards(const uint32_t aIfInDiscards) { mIfInDiscards = BigEndian::HostSwap32(aIfInDiscards); }

    /**
     * Returns the IfOutUcastPkts counter.
     *
     * @returns The IfOutUcastPkts counter
     *
     */
    uint32_t GetIfOutUcastPkts(void) const { return BigEndian::HostSwap32(mIfOutUcastPkts); }

    /**
     * Sets the IfOutUcastPkts counter.
     *
     * @param[in]  aIfOutUcastPkts The IfOutUcastPkts counter.
     *
     */
    void SetIfOutUcastPkts(const uint32_t aIfOutUcastPkts) { mIfOutUcastPkts = BigEndian::HostSwap32(aIfOutUcastPkts); }

    /**
     * Returns the IfOutBroadcastPkts counter.
     *
     * @returns The IfOutBroadcastPkts counter
     *
     */
    uint32_t GetIfOutBroadcastPkts(void) const { return BigEndian::HostSwap32(mIfOutBroadcastPkts); }

    /**
     * Sets the IfOutBroadcastPkts counter.
     *
     * @param[in]  aIfOutBroadcastPkts The IfOutBroadcastPkts counter.
     *
     */
    void SetIfOutBroadcastPkts(const uint32_t aIfOutBroadcastPkts)
    {
        mIfOutBroadcastPkts = BigEndian::HostSwap32(aIfOutBroadcastPkts);
    }

    /**
     * Returns the IfOutDiscards counter.
     *
     * @returns The IfOutDiscards counter
     *
     */
    uint32_t GetIfOutDiscards(void) const { return BigEndian::HostSwap32(mIfOutDiscards); }

    /**
     * Sets the IfOutDiscards counter.
     *
     * @param[in]  aIfOutDiscards The IfOutDiscards counter.
     *
     */
    void SetIfOutDiscards(const uint32_t aIfOutDiscards) { mIfOutDiscards = BigEndian::HostSwap32(aIfOutDiscards); }

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
 * Implements Child Table Entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildTableEntry : public Clearable<ChildTableEntry>
{
public:
    /**
     * Returns the Timeout value.
     *
     * @returns The Timeout value.
     *
     */
    uint8_t GetTimeout(void) const { return (GetTimeoutChildId() & kTimeoutMask) >> kTimeoutOffset; }

    /**
     * Sets the Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value.
     *
     */
    void SetTimeout(uint8_t aTimeout)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kTimeoutMask) | ((aTimeout << kTimeoutOffset) & kTimeoutMask));
    }

    /**
     * The Link Quality value.
     *
     * @returns The Link Quality value.
     *
     */
    LinkQuality GetLinkQuality(void) const
    {
        return static_cast<LinkQuality>((GetTimeoutChildId() & kLqiMask) >> kLqiOffset);
    }

    /**
     * Set the Link Quality value.
     *
     * @param[in] aLinkQuality  The Link Quality value.
     *
     */
    void SetLinkQuality(LinkQuality aLinkQuality)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kLqiMask) | ((aLinkQuality << kLqiOffset) & kLqiMask));
    }

    /**
     * Returns the Child ID value.
     *
     * @returns The Child ID value.
     *
     */
    uint16_t GetChildId(void) const { return (GetTimeoutChildId() & kChildIdMask) >> kChildIdOffset; }

    /**
     * Sets the Child ID value.
     *
     * @param[in]  aChildId  The Child ID value.
     *
     */
    void SetChildId(uint16_t aChildId)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kChildIdMask) | ((aChildId << kChildIdOffset) & kChildIdMask));
    }

    /**
     * Returns the Device Mode
     *
     * @returns The Device Mode
     *
     */
    Mle::DeviceMode GetMode(void) const { return Mle::DeviceMode(mMode); }

    /**
     * Sets the Device Mode.
     *
     * @param[in]  aMode  The Device Mode.
     *
     */
    void SetMode(Mle::DeviceMode aMode) { mMode = aMode.Get(); }

private:
    //             1                   0
    //   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  | Timeout |LQI|     Child ID    |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t  kTimeoutOffset = 11;
    static constexpr uint8_t  kLqiOffset     = 9;
    static constexpr uint8_t  kChildIdOffset = 0;
    static constexpr uint16_t kTimeoutMask   = 0x1f << kTimeoutOffset;
    static constexpr uint16_t kLqiMask       = 0x3 << kLqiOffset;
    static constexpr uint16_t kChildIdMask   = 0x1ff << kChildIdOffset;

    uint16_t GetTimeoutChildId(void) const { return BigEndian::HostSwap16(mTimeoutChildId); }
    void     SetTimeoutChildId(uint16_t aTimeoutChildIf) { mTimeoutChildId = BigEndian::HostSwap16(aTimeoutChildIf); }

    uint16_t mTimeoutChildId;
    uint8_t  mMode;
} OT_TOOL_PACKED_END;

/**
 * Implements Channel Pages TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelPagesTlv : public Tlv, public TlvInfo<Tlv::kChannelPages>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannelPages);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
        // At least one channel page must be included.
        return GetLength() >= 1;
    }

    /**
     * Returns a pointer to the list of Channel Pages.
     *
     * @returns A pointer to the list of Channel Pages.
     *
     */
    uint8_t *GetChannelPages(void) { return mChannelPages; }

private:
    uint8_t mChannelPages[Radio::kNumChannelPages];
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Address List TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TypeListTlv : public Tlv, public TlvInfo<Tlv::kTypeList>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTypeList);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }
} OT_TOOL_PACKED_END;

#if OPENTHREAD_FTD

/**
 * Implements Child TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildTlv : public Tlv, public TlvInfo<Tlv::kChild>, public Clearable<ChildTlv>
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
     *
     */
    void InitFrom(const Child &aChild);

    /**
     * Initializes the TLV as empty (zero length).
     *
     */
    void InitAsEmpty(void)
    {
        SetType(kChild);
        SetLength(0);
    }

    /**
     * Returns the Flags field (`kFlags*` constants define bits in flags).
     *
     * @returns The Flags field.
     *
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * Returns the RLOC16 field.
     *
     * @returns The RLOC16 of the child.
     *
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Returns the Extended Address.
     *
     * @returns The Extended Address of the child.
     *
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Returns the Version field.
     *
     * @returns The Version of the child.
     *
     */
    uint16_t GetVersion(void) const { return BigEndian::HostSwap16(mVersion); }

    /**
     * Returns the Timeout field
     *
     * @returns The Timeout value in seconds.
     *
     */
    uint32_t GetTimeout(void) const { return BigEndian::HostSwap32(mTimeout); }

    /**
     * Returns the Age field.
     *
     * @returns The Age field (seconds since last heard from the child).
     *
     */
    uint32_t GetAge(void) const { return BigEndian::HostSwap32(mAge); }

    /**
     * Returns the Connection Time field.
     *
     * @returns The Connection Time field (seconds since attach).
     *
     */
    uint32_t GetConnectionTime(void) const { return BigEndian::HostSwap32(mConnectionTime); }

    /**
     * Returns the Supervision Interval field
     *
     * @returns The Supervision Interval in seconds. Zero indicates not used.
     *
     */
    uint16_t GetSupervisionInterval(void) const { return BigEndian::HostSwap16(mSupervisionInterval); }

    /**
     * Returns the Link Margin field.
     *
     * @returns The Link Margin in dB.
     *
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * Returns the Average RSSI field.
     *
     * @returns The Average RSSI in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * Returns the Last RSSI field (RSSI of last received frame from child).
     *
     * @returns The Last RSSI field in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * Returns the Frame Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Frame Error Rate (0x0000->0%, 0xffff->100%).
     *
     */
    uint16_t GetFrameErrorRate(void) const { return BigEndian::HostSwap16(mFrameErrorRate); }

    /**
     * Returns the Message Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Message Error Rate (0x0000->0%, 0xffff->100%).
     *
     */
    uint16_t GetMessageErrorRate(void) const { return BigEndian::HostSwap16(mMessageErrorRate); }

    /**
     * Returns the Queued Message Count field.
     *
     * @returns The Queued Message Count (number of queued messages for indirect tx to child).
     *
     */
    uint16_t GetQueuedMessageCount(void) const { return BigEndian::HostSwap16(mQueuedMessageCount); }

    /**
     * Returns the CSL Period in unit of 10 symbols.
     *
     * @returns The CSL Period in unit of 10-symbols-time. Zero if CSL is not supported.
     *
     */
    uint16_t GetCslPeriod(void) const { return BigEndian::HostSwap16(mCslPeriod); }

    /**
     * Returns the CSL Timeout in seconds.
     *
     * @returns The CSL Timeout in seconds. Zero if unknown on parent of if CSL Is not supported.
     *
     */
    uint32_t GetCslTimeout(void) const { return BigEndian::HostSwap32(mCslTimeout); }

    /**
     * Returns the CSL Channel.
     *
     * @returns The CSL channel.
     *
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
 * Implements Child IPv6 Address List Value generation and parsing.
 *
 * This TLV can use extended or normal format depending on the number of IPv6 addresses.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildIp6AddressListTlvValue
{
public:
    /**
     * Returns the RLOC16 of the child.
     *
     * @returns The RLOC16 of the child.
     *
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Sets the RLOC16.
     *
     * @param[in] aRloc16   The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = BigEndian::HostSwap16(aRloc16); }

private:
    uint16_t mRloc16;
    // Followed by zero or more IPv6 address(es).

} OT_TOOL_PACKED_END;

/**
 * Implements Router Neighbor TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class RouterNeighborTlv : public Tlv, public TlvInfo<Tlv::kRouterNeighbor>, public Clearable<RouterNeighborTlv>
{
public:
    static constexpr uint8_t kFlagsTrackErrRate = 1 << 7; ///< Supports tracking error rates.

    /**
     * Initializes the TLV using information from a given `Router`.
     *
     * @param[in] aRouter   The router to initialize the TLV from.
     *
     */
    void InitFrom(const Router &aRouter);

    /**
     * Initializes the TLV as empty (zero length).
     *
     */
    void InitAsEmpty(void)
    {
        SetType(kRouterNeighbor);
        SetLength(0);
    }

    /**
     * Returns the Flags field (`kFlags*` constants define bits in flags).
     *
     * @returns The Flags field.
     *
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * Returns the RLOC16 field.
     *
     * @returns The RLOC16 of the router.
     *
     */
    uint16_t GetRloc16(void) const { return BigEndian::HostSwap16(mRloc16); }

    /**
     * Returns the Extended Address.
     *
     * @returns The Extended Address of the router.
     *
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Returns the Version field.
     *
     * @returns The Version of the router.
     *
     */
    uint16_t GetVersion(void) const { return BigEndian::HostSwap16(mVersion); }

    /**
     * Returns the Connection Time field.
     *
     * @returns The Connection Time field (seconds since link establishment).
     *
     */
    uint32_t GetConnectionTime(void) const { return BigEndian::HostSwap32(mConnectionTime); }

    /**
     * Returns the Link Margin field.
     *
     * @returns The Link Margin in dB.
     *
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * Returns the Average RSSI field.
     *
     * @returns The Average RSSI in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * Returns the Last RSSI field (RSSI of last received frame from router).
     *
     * @returns The Last RSSI field in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * Returns the Frame Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Frame Error Rate (0x0000->0%, 0xffff->100%).
     *
     */
    uint16_t GetFrameErrorRate(void) const { return BigEndian::HostSwap16(mFrameErrorRate); }

    /**
     * Returns the Message Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Message Error Rate (0x0000->0%, 0xffff->100%).
     *
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

#endif // OPENTHREAD_FTD

/**
 * Implements Answer TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class AnswerTlv : public Tlv, public TlvInfo<Tlv::kAnswer>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aIndex   The index value.
     * @param[in] aIsLast  The "IsLast" flag value.
     *
     */
    void Init(uint16_t aIndex, bool aIsLast);

    /**
     * Indicates whether or not the "IsLast" flag is set
     *
     * @retval TRUE   "IsLast" flag si set (this is the last answer for this query).
     * @retval FALSE  "IsLast" flag is not set (more answer messages are expected for this query).
     *
     */
    bool IsLast(void) const { return GetFlagsIndex() & kIsLastFlag; }

    /**
     * Gets the index.
     *
     * @returns The index.
     *
     */
    uint16_t GetIndex(void) const { return GetFlagsIndex() & kIndexMask; }

private:
    static constexpr uint16_t kIsLastFlag = 1 << 15;
    static constexpr uint16_t kIndexMask  = 0x7f;

    uint16_t GetFlagsIndex(void) const { return BigEndian::HostSwap16(mFlagsIndex); }
    void     SetFlagsIndex(uint16_t aFlagsIndex) { mFlagsIndex = BigEndian::HostSwap16(aFlagsIndex); }

    uint16_t mFlagsIndex;
} OT_TOOL_PACKED_END;

/**
 * Represents the MLE Counters.
 *
 */
typedef otNetworkDiagMleCounters MleCounters;

/**
 * Implements MLE Counters TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MleCountersTlv : public Tlv, public TlvInfo<Tlv::kMleCounters>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aMleCounters    The MLE counters to initialize the TLV with.
     *
     */
    void Init(const Mle::Counters &aMleCounters);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     *
     * Reads the counters from TLV.
     *
     * @param[out] aDiagMleCounters   A reference to `NetworkDiagnostic::MleCounters` to populate.
     *
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
} // namespace ot

#endif // NETWORK_DIAGNOSTIC_TLVS_HPP_
