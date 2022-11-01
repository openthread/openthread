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

#include <openthread/thread.h>

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "net/ip6_address.hpp"
#include "radio/radio.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace NetworkDiagnostic {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

/**
 * This class implements Network Diagnostic TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkDiagnosticTlv : public ot::Tlv
{
public:
    /**
     * Network Diagnostic TLV Types.
     *
     */
    enum Type : uint8_t
    {
        kExtMacAddress   = OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS,
        kAddress16       = OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS,
        kMode            = OT_NETWORK_DIAGNOSTIC_TLV_MODE,
        kTimeout         = OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT,
        kConnectivity    = OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY,
        kRoute           = OT_NETWORK_DIAGNOSTIC_TLV_ROUTE,
        kLeaderData      = OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA,
        kNetworkData     = OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA,
        kIp6AddressList  = OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST,
        kMacCounters     = OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS,
        kBatteryLevel    = OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL,
        kSupplyVoltage   = OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE,
        kChildTable      = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE,
        kChannelPages    = OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES,
        kTypeList        = OT_NETWORK_DIAGNOSTIC_TLV_TYPE_LIST,
        kMaxChildTimeout = OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT,
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

} OT_TOOL_PACKED_END;

/**
 * This class defines Extended MAC Address TLV constants and types.
 *
 */
typedef SimpleTlvInfo<NetworkDiagnosticTlv::kExtMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

/**
 * This class defines Address16 TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kAddress16, uint16_t> Address16Tlv;

/**
 * This class defines Mode TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kMode, uint8_t> ModeTlv;

/**
 * This class defines Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * This class defines Battery Level TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kBatteryLevel, uint8_t> BatteryLevelTlv;

/**
 * This class defines Supply Voltage TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kSupplyVoltage, uint16_t> SupplyVoltageTlv;

/**
 * This class defines Max Child Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<NetworkDiagnosticTlv::kMaxChildTimeout, uint32_t> MaxChildTimeoutTlv;

typedef otNetworkDiagConnectivity Connectivity; ///< Network Diagnostic Connectivity value.

/**
 * This class implements Connectivity TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlv : public Mle::ConnectivityTlv
{
public:
    static constexpr uint8_t kType = NetworkDiagnosticTlv::kConnectivity; ///< The TLV Type value.

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::ConnectivityTlv::Init();
        ot::Tlv::SetType(kType);
    }

    /**
     * This method retrieves the `Connectivity` value.
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
 * This class implements Route TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteTlv : public Mle::RouteTlv
{
public:
    static constexpr uint8_t kType = NetworkDiagnosticTlv::kRoute; ///< The TLV Type value.

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::RouteTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * This class implements Leader Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class LeaderDataTlv : public Mle::LeaderDataTlv
{
public:
    static constexpr uint8_t kType = NetworkDiagnosticTlv::kLeaderData; ///< The TLV Type value.

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        Mle::LeaderDataTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * This class implements Network Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkDataTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kNetworkData>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kNetworkData);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() < sizeof(*this) - sizeof(NetworkDiagnosticTlv); }

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
 * This class implements IPv6 Address List TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Ip6AddressListTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kIp6AddressList>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kIp6AddressList);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return !IsExtended() && (GetLength() % sizeof(Ip6::Address) == 0); }

    /**
     * This method returns a pointer to the IPv6 address entry.
     *
     * @param[in]  aIndex  The index into the IPv6 address list.
     *
     * @returns A reference to the IPv6 address.
     *
     */
    const Ip6::Address &GetIp6Address(uint8_t aIndex) const
    {
        return *reinterpret_cast<const Ip6::Address *>(GetValue() + (aIndex * sizeof(Ip6::Address)));
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Mac Counters TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kMacCounters>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMacCounters);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(NetworkDiagnosticTlv); }

    /**
     * This method returns the IfInUnknownProtos counter.
     *
     * @returns The IfInUnknownProtos counter
     *
     */
    uint32_t GetIfInUnknownProtos(void) const { return HostSwap32(mIfInUnknownProtos); }

    /**
     * This method sets the IfInUnknownProtos counter.
     *
     * @param[in]  aIfInUnknownProtos The IfInUnknownProtos counter
     *
     */
    void SetIfInUnknownProtos(const uint32_t aIfInUnknownProtos)
    {
        mIfInUnknownProtos = HostSwap32(aIfInUnknownProtos);
    }

    /**
     * This method returns the IfInErrors counter.
     *
     * @returns The IfInErrors counter
     *
     */
    uint32_t GetIfInErrors(void) const { return HostSwap32(mIfInErrors); }

    /**
     * This method sets the IfInErrors counter.
     *
     * @param[in]  aIfInErrors The IfInErrors counter
     *
     */
    void SetIfInErrors(const uint32_t aIfInErrors) { mIfInErrors = HostSwap32(aIfInErrors); }

    /**
     * This method returns the IfOutErrors counter.
     *
     * @returns The IfOutErrors counter
     *
     */
    uint32_t GetIfOutErrors(void) const { return HostSwap32(mIfOutErrors); }

    /**
     * This method sets the IfOutErrors counter.
     *
     * @param[in]  aIfOutErrors The IfOutErrors counter.
     *
     */
    void SetIfOutErrors(const uint32_t aIfOutErrors) { mIfOutErrors = HostSwap32(aIfOutErrors); }

    /**
     * This method returns the IfInUcastPkts counter.
     *
     * @returns The IfInUcastPkts counter
     *
     */
    uint32_t GetIfInUcastPkts(void) const { return HostSwap32(mIfInUcastPkts); }

    /**
     * This method sets the IfInUcastPkts counter.
     *
     * @param[in]  aIfInUcastPkts The IfInUcastPkts counter.
     *
     */
    void SetIfInUcastPkts(const uint32_t aIfInUcastPkts) { mIfInUcastPkts = HostSwap32(aIfInUcastPkts); }
    /**
     * This method returns the IfInBroadcastPkts counter.
     *
     * @returns The IfInBroadcastPkts counter
     *
     */
    uint32_t GetIfInBroadcastPkts(void) const { return HostSwap32(mIfInBroadcastPkts); }

    /**
     * This method sets the IfInBroadcastPkts counter.
     *
     * @param[in]  aIfInBroadcastPkts The IfInBroadcastPkts counter.
     *
     */
    void SetIfInBroadcastPkts(const uint32_t aIfInBroadcastPkts)
    {
        mIfInBroadcastPkts = HostSwap32(aIfInBroadcastPkts);
    }

    /**
     * This method returns the IfInDiscards counter.
     *
     * @returns The IfInDiscards counter
     *
     */
    uint32_t GetIfInDiscards(void) const { return HostSwap32(mIfInDiscards); }

    /**
     * This method sets the IfInDiscards counter.
     *
     * @param[in]  aIfInDiscards The IfInDiscards counter.
     *
     */
    void SetIfInDiscards(const uint32_t aIfInDiscards) { mIfInDiscards = HostSwap32(aIfInDiscards); }

    /**
     * This method returns the IfOutUcastPkts counter.
     *
     * @returns The IfOutUcastPkts counter
     *
     */
    uint32_t GetIfOutUcastPkts(void) const { return HostSwap32(mIfOutUcastPkts); }

    /**
     * This method sets the IfOutUcastPkts counter.
     *
     * @param[in]  aIfOutUcastPkts The IfOutUcastPkts counter.
     *
     */
    void SetIfOutUcastPkts(const uint32_t aIfOutUcastPkts) { mIfOutUcastPkts = HostSwap32(aIfOutUcastPkts); }

    /**
     * This method returns the IfOutBroadcastPkts counter.
     *
     * @returns The IfOutBroadcastPkts counter
     *
     */
    uint32_t GetIfOutBroadcastPkts(void) const { return HostSwap32(mIfOutBroadcastPkts); }

    /**
     * This method sets the IfOutBroadcastPkts counter.
     *
     * @param[in]  aIfOutBroadcastPkts The IfOutBroadcastPkts counter.
     *
     */
    void SetIfOutBroadcastPkts(const uint32_t aIfOutBroadcastPkts)
    {
        mIfOutBroadcastPkts = HostSwap32(aIfOutBroadcastPkts);
    }

    /**
     * This method returns the IfOutDiscards counter.
     *
     * @returns The IfOutDiscards counter
     *
     */
    uint32_t GetIfOutDiscards(void) const { return HostSwap32(mIfOutDiscards); }

    /**
     * This method sets the IfOutDiscards counter.
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
 * This class implements Child Table Entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildTableEntry
{
public:
    /**
     * Default constructor.
     *
     */
    ChildTableEntry(void)
        : mTimeoutRsvChildId(0)
        , mMode(0)
    {
    }

    /**
     * This method returns the Timeout value.
     *
     * @returns The Timeout value.
     *
     */
    uint8_t GetTimeout(void) const { return (HostSwap16(mTimeoutRsvChildId) & kTimeoutMask) >> kTimeoutOffset; }

    /**
     * This method sets the Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value.
     *
     */
    void SetTimeout(uint8_t aTimeout)
    {
        mTimeoutRsvChildId = HostSwap16((HostSwap16(mTimeoutRsvChildId) & ~kTimeoutMask) |
                                        ((aTimeout << kTimeoutOffset) & kTimeoutMask));
    }

    /**
     * This method returns the Child ID value.
     *
     * @returns The Child ID value.
     *
     */
    uint16_t GetChildId(void) const { return HostSwap16(mTimeoutRsvChildId) & kChildIdMask; }

    /**
     * This method sets the Child ID value.
     *
     * @param[in]  aChildId  The Child ID value.
     *
     */
    void SetChildId(uint16_t aChildId)
    {
        mTimeoutRsvChildId = HostSwap16((HostSwap16(mTimeoutRsvChildId) & ~kChildIdMask) | (aChildId & kChildIdMask));
    }

    /**
     * This method returns the Device Mode
     *
     * @returns The Device Mode
     *
     */
    Mle::DeviceMode GetMode(void) const { return Mle::DeviceMode(mMode); }

    /**
     * This method sets the Device Mode.
     *
     * @param[in]  aMode  The Device Mode.
     *
     */
    void SetMode(Mle::DeviceMode aMode) { mMode = aMode.Get(); }

    /**
     * This method returns the Reserved value.
     *
     * @returns The Reserved value.
     *
     */
    uint8_t GetReserved(void) const { return (HostSwap16(mTimeoutRsvChildId) & kReservedMask) >> kReservedOffset; }

    /**
     * This method sets the Reserved value.
     *
     * @param[in]  aReserved  The Reserved value.
     *
     */
    void SetReserved(uint8_t aReserved)
    {
        mTimeoutRsvChildId = HostSwap16((HostSwap16(mTimeoutRsvChildId) & ~kReservedMask) |
                                        ((aReserved << kReservedOffset) & kReservedMask));
    }

private:
    static constexpr uint8_t  kTimeoutOffset  = 11;
    static constexpr uint8_t  kReservedOffset = 9;
    static constexpr uint16_t kTimeoutMask    = 0xf800;
    static constexpr uint16_t kReservedMask   = 0x0600;
    static constexpr uint16_t kChildIdMask    = 0x1ff;

    uint16_t mTimeoutRsvChildId;
    uint8_t  mMode;
} OT_TOOL_PACKED_END;

/**
 * This class implements Child Table TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildTableTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kChildTable>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChildTable);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return (GetLength() % sizeof(ChildTableEntry)) == 0; }

    /**
     * This method returns the number of Child Table entries.
     *
     * @returns The number of Child Table entries.
     *
     */
    uint8_t GetNumEntries(void) const { return GetLength() / sizeof(ChildTableEntry); }

    /**
     * This method returns the Child Table entry at @p aIndex.
     *
     * @param[in]  aIndex  The index into the Child Table list.
     *
     * @returns  A reference to the Child Table entry.
     *
     */
    ChildTableEntry &GetEntry(uint16_t aIndex)
    {
        return *reinterpret_cast<ChildTableEntry *>(GetValue() + (aIndex * sizeof(ChildTableEntry)));
    }

    /**
     * This method reads the Child Table entry at @p aIndex.
     *
     * @param[out]  aEntry      A reference to a ChildTableEntry.
     * @param[in]   aMessage    A reference to the message.
     * @param[in]   aOffset     The offset of the ChildTableTLV in aMessage.
     * @param[in]   aIndex      The index into the Child Table list.
     *
     * @retval  kErrorNotFound   No such entry is found.
     * @retval  kErrorNone       Successfully read the entry.
     *
     */
    Error ReadEntry(ChildTableEntry &aEntry, const Message &aMessage, uint16_t aOffset, uint8_t aIndex) const
    {
        return ((aIndex < GetNumEntries()) &&
                (aMessage.Read(aOffset + sizeof(ChildTableTlv) + (aIndex * sizeof(ChildTableEntry)), aEntry) ==
                 kErrorNone))
                   ? kErrorNone
                   : kErrorInvalidArgs;
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Channel Pages TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelPagesTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kChannelPages>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannelPages);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
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
        // At least one channel page must be included.
        return GetLength() >= 1;
    }

    /**
     * This method returns a pointer to the list of Channel Pages.
     *
     * @returns A pointer to the list of Channel Pages.
     *
     */
    uint8_t *GetChannelPages(void) { return mChannelPages; }

private:
    uint8_t mChannelPages[Radio::kNumChannelPages];
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Address List TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class TypeListTlv : public NetworkDiagnosticTlv, public TlvInfo<NetworkDiagnosticTlv::kTypeList>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTypeList);
        SetLength(sizeof(*this) - sizeof(NetworkDiagnosticTlv));
    }
} OT_TOOL_PACKED_END;

} // namespace NetworkDiagnostic
} // namespace ot

#endif // NETWORK_DIAGNOSTIC_TLVS_HPP_
