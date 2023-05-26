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
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace NetworkDiagnostic {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

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
        kExtMacAddress      = OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS,
        kAddress16          = OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS,
        kMode               = OT_NETWORK_DIAGNOSTIC_TLV_MODE,
        kTimeout            = OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT,
        kConnectivity       = OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY,
        kRoute              = OT_NETWORK_DIAGNOSTIC_TLV_ROUTE,
        kLeaderData         = OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA,
        kNetworkData        = OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA,
        kIp6AddressList     = OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST,
        kMacCounters        = OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS,
        kBatteryLevel       = OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL,
        kSupplyVoltage      = OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE,
        kChildTable         = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE,
        kChannelPages       = OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES,
        kTypeList           = OT_NETWORK_DIAGNOSTIC_TLV_TYPE_LIST,
        kMaxChildTimeout    = OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT,
        kVersion            = OT_NETWORK_DIAGNOSTIC_TLV_VERSION,
        kVendorName         = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME,
        kVendorModel        = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL,
        kVendorSwVersion    = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION,
        kThreadStackVersion = OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION,
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
    uint32_t GetIfInUnknownProtos(void) const { return HostSwap32(mIfInUnknownProtos); }

    /**
     * Sets the IfInUnknownProtos counter.
     *
     * @param[in]  aIfInUnknownProtos The IfInUnknownProtos counter
     *
     */
    void SetIfInUnknownProtos(const uint32_t aIfInUnknownProtos)
    {
        mIfInUnknownProtos = HostSwap32(aIfInUnknownProtos);
    }

    /**
     * Returns the IfInErrors counter.
     *
     * @returns The IfInErrors counter
     *
     */
    uint32_t GetIfInErrors(void) const { return HostSwap32(mIfInErrors); }

    /**
     * Sets the IfInErrors counter.
     *
     * @param[in]  aIfInErrors The IfInErrors counter
     *
     */
    void SetIfInErrors(const uint32_t aIfInErrors) { mIfInErrors = HostSwap32(aIfInErrors); }

    /**
     * Returns the IfOutErrors counter.
     *
     * @returns The IfOutErrors counter
     *
     */
    uint32_t GetIfOutErrors(void) const { return HostSwap32(mIfOutErrors); }

    /**
     * Sets the IfOutErrors counter.
     *
     * @param[in]  aIfOutErrors The IfOutErrors counter.
     *
     */
    void SetIfOutErrors(const uint32_t aIfOutErrors) { mIfOutErrors = HostSwap32(aIfOutErrors); }

    /**
     * Returns the IfInUcastPkts counter.
     *
     * @returns The IfInUcastPkts counter
     *
     */
    uint32_t GetIfInUcastPkts(void) const { return HostSwap32(mIfInUcastPkts); }

    /**
     * Sets the IfInUcastPkts counter.
     *
     * @param[in]  aIfInUcastPkts The IfInUcastPkts counter.
     *
     */
    void SetIfInUcastPkts(const uint32_t aIfInUcastPkts) { mIfInUcastPkts = HostSwap32(aIfInUcastPkts); }
    /**
     * Returns the IfInBroadcastPkts counter.
     *
     * @returns The IfInBroadcastPkts counter
     *
     */
    uint32_t GetIfInBroadcastPkts(void) const { return HostSwap32(mIfInBroadcastPkts); }

    /**
     * Sets the IfInBroadcastPkts counter.
     *
     * @param[in]  aIfInBroadcastPkts The IfInBroadcastPkts counter.
     *
     */
    void SetIfInBroadcastPkts(const uint32_t aIfInBroadcastPkts)
    {
        mIfInBroadcastPkts = HostSwap32(aIfInBroadcastPkts);
    }

    /**
     * Returns the IfInDiscards counter.
     *
     * @returns The IfInDiscards counter
     *
     */
    uint32_t GetIfInDiscards(void) const { return HostSwap32(mIfInDiscards); }

    /**
     * Sets the IfInDiscards counter.
     *
     * @param[in]  aIfInDiscards The IfInDiscards counter.
     *
     */
    void SetIfInDiscards(const uint32_t aIfInDiscards) { mIfInDiscards = HostSwap32(aIfInDiscards); }

    /**
     * Returns the IfOutUcastPkts counter.
     *
     * @returns The IfOutUcastPkts counter
     *
     */
    uint32_t GetIfOutUcastPkts(void) const { return HostSwap32(mIfOutUcastPkts); }

    /**
     * Sets the IfOutUcastPkts counter.
     *
     * @param[in]  aIfOutUcastPkts The IfOutUcastPkts counter.
     *
     */
    void SetIfOutUcastPkts(const uint32_t aIfOutUcastPkts) { mIfOutUcastPkts = HostSwap32(aIfOutUcastPkts); }

    /**
     * Returns the IfOutBroadcastPkts counter.
     *
     * @returns The IfOutBroadcastPkts counter
     *
     */
    uint32_t GetIfOutBroadcastPkts(void) const { return HostSwap32(mIfOutBroadcastPkts); }

    /**
     * Sets the IfOutBroadcastPkts counter.
     *
     * @param[in]  aIfOutBroadcastPkts The IfOutBroadcastPkts counter.
     *
     */
    void SetIfOutBroadcastPkts(const uint32_t aIfOutBroadcastPkts)
    {
        mIfOutBroadcastPkts = HostSwap32(aIfOutBroadcastPkts);
    }

    /**
     * Returns the IfOutDiscards counter.
     *
     * @returns The IfOutDiscards counter
     *
     */
    uint32_t GetIfOutDiscards(void) const { return HostSwap32(mIfOutDiscards); }

    /**
     * Sets the IfOutDiscards counter.
     *
     * @param[in]  aIfOutDiscards The IfOutDiscards counter.
     *
     */
    void SetIfOutDiscards(const uint32_t aIfOutDiscards) { mIfOutDiscards = HostSwap32(aIfOutDiscards); }

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

    uint16_t GetTimeoutChildId(void) const { return HostSwap16(mTimeoutChildId); }
    void     SetTimeoutChildId(uint16_t aTimeoutChildIf) { mTimeoutChildId = HostSwap16(aTimeoutChildIf); }

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

} // namespace NetworkDiagnostic
} // namespace ot

#endif // NETWORK_DIAGNOSTIC_TLVS_HPP_
