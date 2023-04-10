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
#include "thread/link_quality.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/topology.hpp"

namespace ot {
namespace NetworkDiagnostic {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

/**
 * This class implements Network Diagnostic TLV generation and parsing.
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
        kVersion             = OT_NETWORK_DIAGNOSTIC_TLV_VERSION,
        kChild               = 25,
        kChildIp6AddressList = 26,
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
typedef SimpleTlvInfo<Tlv::kExtMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

/**
 * This class defines Address16 TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kAddress16, uint16_t> Address16Tlv;

/**
 * This class defines Mode TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * This class defines Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * This class defines Network Data TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kNetworkData> NetworkDataTlv;

/**
 * This class defines IPv6 Address List TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kIp6AddressList> Ip6AddressListTlv;

/**
 * This class defines Battery Level TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kBatteryLevel, uint8_t> BatteryLevelTlv;

/**
 * This class defines Supply Voltage TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kSupplyVoltage, uint16_t> SupplyVoltageTlv;

/**
 * This class defines Child Table TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kChildTable> ChildTableTlv;

/**
 * This class defines Max Child Timeout TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kMaxChildTimeout, uint32_t> MaxChildTimeoutTlv;

/**
 * This class defines Version TLV constants and types.
 *
 */
typedef UintTlvInfo<Tlv::kVersion, uint16_t> VersionTlv;

/**
 * This class defines Child IPv6 Address List TLV constants and types.
 *
 */
typedef TlvInfo<Tlv::kChildIp6AddressList> ChildIp6AddressListTlv;

typedef otNetworkDiagConnectivity Connectivity; ///< Network Diagnostic Connectivity value.

/**
 * This class implements Connectivity TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlv : public Mle::ConnectivityTlv
{
public:
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kConnectivity; ///< The TLV Type value.

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
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kRoute; ///< The TLV Type value.

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
    static constexpr uint8_t kType = ot::NetworkDiagnostic::Tlv::kLeaderData; ///< The TLV Type value.

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
 * This class implements Mac Counters TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public Tlv, public TlvInfo<Tlv::kMacCounters>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kMacCounters);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

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
class ChildTableEntry : public Clearable<ChildTableEntry>
{
public:
    /**
     * This method returns the Timeout value.
     *
     * @returns The Timeout value.
     *
     */
    uint8_t GetTimeout(void) const { return (GetTimeoutChildId() & kTimeoutMask) >> kTimeoutOffset; }

    /**
     * This method sets the Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value.
     *
     */
    void SetTimeout(uint8_t aTimeout)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kTimeoutMask) | ((aTimeout << kTimeoutOffset) & kTimeoutMask));
    }

    /**
     * This method the Link Quality value.
     *
     * @returns The Link Quality value.
     *
     */
    LinkQuality GetLinkQuality(void) const
    {
        return static_cast<LinkQuality>((GetTimeoutChildId() & kLqiMask) >> kLqiOffset);
    }

    /**
     * This method set the Link Quality value.
     *
     * @param[in] aLinkQuality  The Link Quality value.
     *
     */
    void SetLinkQuality(LinkQuality aLinkQuality)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kLqiMask) | ((aLinkQuality << kLqiOffset) & kLqiMask));
    }

    /**
     * This method returns the Child ID value.
     *
     * @returns The Child ID value.
     *
     */
    uint16_t GetChildId(void) const { return (GetTimeoutChildId() & kChildIdMask) >> kChildIdOffset; }

    /**
     * This method sets the Child ID value.
     *
     * @param[in]  aChildId  The Child ID value.
     *
     */
    void SetChildId(uint16_t aChildId)
    {
        SetTimeoutChildId((GetTimeoutChildId() & ~kChildIdMask) | ((aChildId << kChildIdOffset) & kChildIdMask));
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
 * This class implements Channel Pages TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChannelPagesTlv : public Tlv, public TlvInfo<Tlv::kChannelPages>
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kChannelPages);
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
class TypeListTlv : public Tlv, public TlvInfo<Tlv::kTypeList>
{
public:
    /**
     * This method initializes the TLV.
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
 * This class implements Child TLV generation and parsing.
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
     * This method initializes the TLV using information from a given `Child`.
     *
     * @param[in] aChild   The child to initialize the TLV from.
     *
     */
    void InitFrom(const Child &aChild);

    /**
     * This method initializes the TLV as empty (zero length).
     *
     */
    void InitAsEmpty(void)
    {
        SetType(kChild);
        SetLength(0);
    }

    /**
     * This method returns the Flags field (`kFlags*` constants define bits in flags).
     *
     * @returns The Flags field.
     *
     */
    uint8_t GetFlags(void) const { return mFlags; }

    /**
     * This method returns the RLOC16 field.
     *
     * @returns The RLOC16 of the child.
     *
     */
    uint16_t GetRloc16(void) const { return HostSwap16(mRloc16); }

    /**
     * This method returns the Extended Address.
     *
     * @returns The Extended Address of the child.
     *
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * This method returns the Version field.
     *
     * @returns The Version of the child.
     *
     */
    uint16_t GetVersion(void) const { return HostSwap16(mVersion); }

    /**
     * This method returns the Timeout field
     *
     * @returns The Timeout value in seconds.
     *
     */
    uint32_t GetTimeout(void) const { return HostSwap32(mTimeout); }

    /**
     * This method returns the Age field.
     *
     * @returns The Age field (seconds since last heard from the child).
     *
     */
    uint32_t GetAge(void) const { return HostSwap32(mAge); }

    /**
     * This method returns the Supervision Interval field
     *
     * @returns The Supervision Interval in seconds. Zero indicates not used.
     *
     */
    uint16_t GetSupervisionInterval(void) const { return HostSwap16(mSupervisionInterval); }

    /**
     * This method returns the Average RSSI field.
     *
     * @returns The Average RSSI in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * This method returns the Last RSSI field (RSSI of last received frame from child).
     *
     * @returns The Last RSSI field in dBm. 127 if not available or unknown.
     *
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * This method returns the Frame Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Frame Error Rate (0xffff->100%).
     *
     */
    uint16_t GetFrameErrorRate(void) const { return HostSwap16(mFrameErrorRate); }

    /**
     * This method returns the Message Error Rate field.
     *
     * `kFlagsTrackErrRate` from `GetFlags()` indicates whether or not the implementation supports tracking of error
     * rates and whether or not the value in this field is valid.
     *
     * @returns The Message Error Rate (0xffff->100%). Zero if not known or supported.
     *
     */
    uint16_t GetMessageErrorRate(void) const { return HostSwap16(mMessageErrorRate); }

    /**
     * This method returns the Queued Message Count field.
     *
     * @returns The Queued Message Count (number of queued message for indirect tx to child).
     *
     */
    uint16_t GetQueuedMessageCount(void) const { return HostSwap16(mQueuedMessageCount); }

    /**
     * This method returns the CSL Period in unit of 10 symbols.
     *
     * @returns The CSL Period in unit of 10 symbols. Zero if CSL is not supported.
     *
     */
    uint16_t GetCslPeriod(void) const { return HostSwap16(mCslPeriod); }

    /**
     * This method returns the CSL Timeout in seconds.
     *
     * @returns The CSL Timeout in seconds. Zero if CSL is not supported.
     *
     */
    uint32_t GetCslTimeout(void) const { return HostSwap32(mCslTimeout); }

private:
    uint8_t         mFlags;               // Flags (`kFlags*` constants).
    uint16_t        mRloc16;              // RLOC16.
    Mac::ExtAddress mExtAddress;          // Extended Address.
    uint16_t        mVersion;             // Version.
    uint32_t        mTimeout;             // Timeout in seconds.
    uint32_t        mAge;                 // Seconds since last heard from the child.
    uint16_t        mSupervisionInterval; // Supervision interval in seconds. Zero to indicate not used.
    int8_t          mAverageRssi;         // Average RSSI. 127 if not available or unknown
    int8_t          mLastRssi;            // RSSI of last received frame. 127 if not available or unknown.
    uint16_t        mFrameErrorRate;      // Frame error rate (0xffff->100%). Zero if not known or supported.
    uint16_t        mMessageErrorRate;    // (IPv6) msg error rate (0xffff->100%). Zero if not known or supported.
    uint16_t        mQueuedMessageCount;  // Number of queued message for indirect tx to child.
    uint16_t        mCslPeriod;           // CSL Period in unit of 10 symbols. Zero indicates disabled.
    uint32_t        mCslTimeout;          // CSL Timeout in seconds. Zero if not suppurated.
} OT_TOOL_PACKED_END;

/**
 * This class implements Child IPv6 Address List Value generation and parsing.
 *
 * This TLV can use  extended or normal format depending on the number of IPv6 addresses.
 *
 */
OT_TOOL_PACKED_BEGIN
class ChildIp6AddressListTlvValue
{
public:
    /**
     * This method returns the RLOC16 of the child.
     *
     * @returns The RLOC16 of the child.
     *
     */
    uint16_t GetRloc16(void) const { return HostSwap16(mRloc16); }

    /**
     * This method sets the RLOC16.
     *
     * @param[in] aRloc16   The RLOC16 value.
     *
     */
    void SetRloc16(uint16_t aRloc16) { mRloc16 = HostSwap16(aRloc16); }

private:
    uint16_t mRloc16;
    // Followed by zero or more IPv6 address(es).

} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_FTD

} // namespace NetworkDiagnostic
} // namespace ot

#endif // NETWORK_DIAGNOSTIC_TLVS_HPP_
