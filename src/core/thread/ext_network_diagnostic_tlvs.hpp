/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_TLVS_HPP_
#define OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_TLVS_HPP_

#include <net/ip6_address.hpp>
#include <thread/ext_network_diagnostic_types.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/network_diagnostic_tlvs.hpp>
#include "openthread/platform/toolchain.h"

namespace ot {
namespace ExtNetworkDiagnostic {

/**
 * Defines TLV structures and type aliases for extended network diagnostic server messages.
 *
 * This file provides type-safe TLV (Type-Length-Value) definitions used in diagnostic
 * server protocol messages. Each TLV type corresponds to a specific diagnostic data element
 * that can be exchanged between routers, children, and clients.
 */

/**
 * Extended MAC Address TLV (Type 0).
 *
 * Carries the IEEE 802.15.4 Extended Address (EUI-64) of a device.
 * Used to uniquely identify devices in diagnostic reports.
 */
typedef SimpleTlvInfo<Tlv::kMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

/**
 * Mode TLV (Type 1).
 *
 * Carries the Thread device mode byte, indicating device capabilities:
 * - RxOnWhenIdle
 * - SecureDataRequests (Deprecated)
 * - FullThreadDevice
 * - FullNetworkData
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * Timeout TLV (Type 2).
 *
 * For children: Child timeout value in seconds (time until parent considers child detached).
 * For routers: Not applicable.
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * Last Heard TLV (Type 3).
 *
 * Time in milliseconds since the last frame was received from this device.
 * Used to track communication freshness for children and neighbors.
 */
typedef UintTlvInfo<Tlv::kLastHeard, uint32_t> LastHeardTlv;

/**
 * Connection Time TLV (Type 4).
 *
 * Duration in seconds that the device has been connected as a child or neighbor.
 * Resets to zero when the relationship is re-established.
 */
typedef UintTlvInfo<Tlv::kConnectionTime, uint32_t> ConnectionTimeTlv;

/**
 * CSL (Coordinated Sampled Listening) TLV (Type 5).
 *
 * Carries CSL parameters for Sleepy End Devices (SEDs):
 * - Timeout: CSL timeout in seconds (time until CSL becomes inactive)
 * - Period: CSL sample period in units of 10 symbols (0 = CSL not synchronized)
 * - Channel: CSL channel number
 *
 * CSL allows SEDs to sleep most of the time while coordinating wake times with the parent.
 */
OT_TOOL_PACKED_BEGIN
class CslTlv : public Tlv, public TlvInfo<Tlv::kCsl>
{
public:
    /**
     * Initializes the CSL TLV with default values (all zeros).
     */
    void Init(void)
    {
        SetType(Tlv::kCsl);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mTimeout = 0;
        mPeriod  = 0;
        mChannel = 0;
    }

    /**
     * Checks if CSL is synchronized (period is non-zero).
     *
     * @retval true   CSL is synchronized (child has negotiated CSL parameters).
     * @retval false  CSL is not synchronized (period is zero).
     */
    bool IsCslSynchronized(void) const { return mPeriod != 0; }

    /**
     * Gets the CSL timeout value.
     *
     * @returns The CSL timeout in seconds.
     */
    uint32_t GetTimeout(void) const { return BigEndian::HostSwap32(mTimeout); }

    /**
     * Sets the CSL timeout value.
     *
     * @param[in] aTimeout  The CSL timeout in seconds.
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = BigEndian::HostSwap32(aTimeout); }

    /**
     * Gets the CSL period value.
     *
     * @returns The CSL period in units of 10 symbols (0 = not synchronized).
     */
    uint16_t GetPeriod(void) const { return BigEndian::HostSwap16(mPeriod); }

    /**
     * Sets the CSL period value.
     *
     * @param[in] aPeriod  The CSL period in units of 10 symbols.
     */
    void SetPeriod(uint16_t aPeriod) { mPeriod = BigEndian::HostSwap16(aPeriod); }

    /**
     * Gets the CSL channel number.
     *
     * @returns The CSL channel number.
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * Sets the CSL channel number.
     *
     * @param[in] aChannel  The CSL channel number.
     */
    void SetChannel(uint8_t aChannel) { mChannel = aChannel; }

private:
    uint32_t mTimeout;
    uint16_t mPeriod;
    uint8_t  mChannel;
} OT_TOOL_PACKED_END;

/**
 * Route64 TLV (Type 9).
 *
 * Carries the Router ID Sequence and Router ID Mask for the Thread network.
 * Extends the MLE RouteTlv with diagnostic server TLV type.
 * Used to report routing table state to diagnostic clients.
 */
OT_TOOL_PACKED_BEGIN
class Route64Tlv : public Mle::RouteTlv
{
public:
    static constexpr uint8_t kType = ot::ExtNetworkDiagnostic::Tlv::kRoute64;

    /**
     * Initializes the Route64 TLV.
     */
    void Init(void)
    {
        Mle::RouteTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * Common fields for Link Margin TLVs.
 *
 * Link margin represents the difference between received signal strength and
 * the minimum required signal strength for successful reception.
 * Includes both instantaneous (last RSSI) and averaged metrics.
 */
OT_TOOL_PACKED_BEGIN
class LinkMarginTlvFields
{
public:
    /**
     * Gets the link margin value.
     *
     * @returns The link margin in dB (0-130, where 130 = unused/unknown).
     */
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    /**
     * Sets the link margin value.
     *
     * @param[in] aLinkMargin  The link margin in dB.
     */
    void SetLinkMargin(uint8_t aLinkMargin) { mLinkMargin = aLinkMargin; }

    /**
     * Gets the average RSSI value.
     *
     * @returns The average RSSI in dBm.
     */
    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    /**
     * Sets the average RSSI value.
     *
     * @param[in] aRssi  The average RSSI in dBm.
     */
    void SetAverageRssi(int8_t aRssi) { mAverageRssi = aRssi; }

    /**
     * Gets the last RSSI value.
     *
     * @returns The last RSSI in dBm.
     */
    int8_t GetLastRssi(void) const { return mLastRssi; }

    /**
     * Sets the last RSSI value.
     *
     * @param[in] aRssi  The last RSSI in dBm.
     */
    void SetLastRssi(int8_t aRssi) { mLastRssi = aRssi; }

private:
    uint8_t mLinkMargin;
    int8_t  mAverageRssi;
    int8_t  mLastRssi;
} OT_TOOL_PACKED_END;

/**
 * Link Margin In TLV (Type 7).
 *
 * Carries inbound link quality metrics from the perspective of the reporting device.
 * "In" refers to frames received by this device from a child or neighbor.
 * Includes link margin, average RSSI, and last RSSI.
 */
OT_TOOL_PACKED_BEGIN
class LinkMarginInTlv : public Tlv, public TlvInfo<Tlv::kLinkMarginIn>, public LinkMarginTlvFields
{
public:
    /**
     * Initializes the Link Margin In TLV.
     */
    void Init(void)
    {
        SetType(Tlv::kLinkMarginIn);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }
} OT_TOOL_PACKED_END;

/**
 * Common fields for MAC Link Error Rates TLVs.
 *
 * Error rates are expressed as fixed-point values where 0xFFFF = 100%.
 * - Message error rate: Percentage of MAC-level messages that failed delivery
 * - Frame error rate: Percentage of MAC frames that failed (includes retries)
 */
OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesTlvFields
{
public:
    /**
     * Gets the message error rate.
     *
     * @returns The message error rate (0x0000-0xFFFF, where 0xFFFF = 100%).
     */
    uint16_t GetMessageErrorRates(void) const { return mMessageErrorRates; }

    /**
     * Sets the message error rate.
     *
     * @param[in] aMessageErrorRates  The message error rate.
     */
    void SetMessageErrorRates(uint16_t aMessageErrorRates) { mMessageErrorRates = aMessageErrorRates; }

    /**
     * Gets the frame error rate.
     *
     * @returns The frame error rate (0x0000-0xFFFF, where 0xFFFF = 100%).
     */
    uint16_t GetFrameErrorRates(void) const { return mFrameErrorRates; }

    /**
     * Sets the frame error rate.
     *
     * @param[in] aFrameErrorRates  The frame error rate.
     */
    void SetFrameErrorRates(uint16_t aFrameErrorRates) { mFrameErrorRates = aFrameErrorRates; }

private:
    uint16_t mMessageErrorRates;
    uint16_t mFrameErrorRates;
} OT_TOOL_PACKED_END;

/**
 * MAC Link Error Rates Out TLV (Type 8).
 *
 * Carries outbound MAC layer error rates for a child or neighbor.
 * "Out" refers to frames transmitted by this device to the child/neighbor.
 * Includes message error rate and frame error rate.
 */
OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesOutTlv : public Tlv,
                                public TlvInfo<Tlv::kMacLinkErrorRatesOut>,
                                public MacLinkErrorRatesTlvFields
{
public:
    /**
     * Initializes the MAC Link Error Rates Out TLV.
     */
    void Init(void)
    {
        SetType(Tlv::kMacLinkErrorRatesOut);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }
} OT_TOOL_PACKED_END;

/**
 * ML-EID (Mesh-Local Endpoint Identifier) TLV (Type 10).
 *
 * Carries the Interface Identifier (IID) portion of a device's Mesh-Local EID.
 * The full ML-EID is formed by combining the mesh-local prefix with this IID.
 * Used for FTD children to report their stable mesh-local identifier.
 */
typedef SimpleTlvInfo<Tlv::kMlEid, Ip6::InterfaceIdentifier> MlEidTlv;

/**
 * IPv6 Address List TLV (Type 11).
 *
 * Carries a list of IPv6 addresses assigned to a device.
 * Excludes mesh-local, link-local, anycast locator addresses (reported separately).
 * Variable length TLV containing concatenated 16-byte IPv6 addresses.
 */
typedef TlvInfo<Tlv::kIp6AddressList> Ip6AddressListTlv;

/**
 * ALOC (Anycast Locator) List TLV (Type 12).
 *
 * Carries a list of ALOC (Anycast Locator) values for a device.
 * Each entry is a single byte representing the locator field of an anycast address.
 * Variable length TLV containing one byte per ALOC.
 */
typedef TlvInfo<Tlv::kAlocList> AlocListTlv;

/**
 * Thread Spec Version TLV (Type 16).
 *
 * Carries the Thread specification version supported by the device.
 * Encoded as a 16-bit value (e.g., 4 for Thread 1.4).
 */
typedef UintTlvInfo<Tlv::kThreadSpecVersion, uint16_t> ThreadSpecVersionTlv;

/**
 * Thread Stack Version TLV (Type 17).
 *
 * Carries a human-readable string identifying the Thread stack implementation and version.
 * Maximum length defined by kMaxThreadStackTlvLength.
 */
typedef StringTlvInfo<Tlv::kThreadStackVersion, Tlv::kMaxThreadStackTlvLength> ThreadStackVersionTlv;

/**
 * Vendor Name TLV (Type 18).
 *
 * Carries a human-readable vendor/manufacturer name string.
 * Maximum length defined by kMaxVendorNameTlvLength.
 */
typedef StringTlvInfo<Tlv::kVendorName, Tlv::kMaxVendorNameTlvLength> VendorNameTlv;

/**
 * Vendor Model TLV (Type 19).
 *
 * Carries a human-readable product model identifier string.
 * Maximum length defined by kMaxVendorModelTlvLength.
 */
typedef StringTlvInfo<Tlv::kVendorModel, Tlv::kMaxVendorModelTlvLength> VendorModelTlv;

/**
 * Vendor Software Version TLV (Type 20).
 *
 * Carries a human-readable firmware/software version string.
 * Maximum length defined by kMaxVendorSwVersionTlvLength.
 */
typedef StringTlvInfo<Tlv::kVendorSwVersion, Tlv::kMaxVendorSwVersionTlvLength> VendorSwVersionTlv;

/**
 * Vendor App URL TLV (Type 21).
 *
 * Carries a URL string pointing to vendor application or product information.
 * Maximum length defined by kMaxVendorAppUrlTlvLength.
 */
typedef StringTlvInfo<Tlv::kVendorAppUrl, Tlv::kMaxVendorAppUrlTlvLength> VendorAppUrlTlv;

/**
 * IPv6 Link-Local Address List TLV (Type 22).
 *
 * Carries a list of link-local IPv6 addresses assigned to a device.
 * Excludes well-known link-local multicast addresses (all-nodes, all-routers).
 * Variable length TLV containing concatenated 16-byte IPv6 addresses.
 */
typedef TlvInfo<Tlv::kIp6LinkLocalAddressList> Ip6LinkLocalAddressListTlv;

/**
 * EUI-64 TLV (Type 23).
 *
 * Carries the IEEE EUI-64 identifier of a device.
 * This is a child-provided TLV reported by end devices to their parent router.
 * May differ from the MAC Extended Address (Type 0) on some platforms.
 */
typedef SimpleTlvInfo<Tlv::kEui64, Mac::ExtAddress> Eui64Tlv;

/**
 * MAC Counters TLV (Type 24).
 *
 * Carries MAC layer statistics counters including:
 * - TxTotal, TxUnicast, TxBroadcast, TxAckRequested, TxAcked
 * - TxNoAckRequested, TxData, TxDataPoll, TxBeacon, TxBeaconRequest
 * - RxTotal, RxUnicast, RxBroadcast, RxData, RxDataPoll
 * - RxBeacon, RxBeaconRequest, RxOther, RxAddressFiltered, RxDestAddrFiltered
 * - TxErrCca, TxErrAbort, TxErrBusyChannel
 *
 * Extends NetworkDiagnostic::MacCountersTlv with diagnostic server TLV type.
 */
OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public NetworkDiagnostic::MacCountersTlv
{
public:
    static constexpr uint8_t kType = ot::ExtNetworkDiagnostic::Tlv::kMacCounters;

    /**
     * Initializes the MAC Counters TLV.
     */
    void Init(void)
    {
        NetworkDiagnostic::MacCountersTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

/**
 * MAC Link Error Rates In TLV (Type 25).
 *
 * Carries inbound MAC layer error rates from the perspective of a child device.
 * "In" refers to frames received by the child from its parent.
 * This is a child-provided TLV reported by end devices.
 * Includes message error rate and frame error rate.
 */
OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesInTlv : public Tlv, public TlvInfo<Tlv::kMacLinkErrorRatesIn>, public MacLinkErrorRatesTlvFields
{
public:
    /**
     * Initializes the MAC Link Error Rates In TLV.
     */
    void Init(void)
    {
        SetType(Tlv::kMacLinkErrorRatesIn);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }
} OT_TOOL_PACKED_END;

/**
 * MLE Counters TLV (Type 26).
 *
 * Carries MLE (Mesh Link Establishment) layer statistics counters including:
 * - DisabledRole, DetachedRole, ChildRole, RouterRole, LeaderRole
 * - AttachAttempts, PartitionIdChanges, BetterPartitionAttachAttempts
 * - ParentChanges, TrackedTime, DisabledTime, DetachedTime, ChildTime, RouterTime, LeaderTime
 *
 */
OT_TOOL_PACKED_BEGIN
class MleCountersTlv : public Tlv, public TlvInfo<Tlv::kMleCounters>
{
public:
    static constexpr uint8_t kType = ot::ExtNetworkDiagnostic::Tlv::kMleCounters;

    /**
     * Initializes the MLE Counters TLV with counter values.
     *
     * @param[in] aCounters  Reference to MLE counters to copy into the TLV.
     */
    void Init(const Mle::Counters &aCounters)
    {
        SetType(kType);
        SetLength(sizeof(*this) - sizeof(Tlv));

        mDisabledRole                  = BigEndian::HostSwap16(aCounters.mDisabledRole);
        mDetachedRole                  = BigEndian::HostSwap16(aCounters.mDetachedRole);
        mChildRole                     = BigEndian::HostSwap16(aCounters.mChildRole);
        mRouterRole                    = BigEndian::HostSwap16(aCounters.mRouterRole);
        mLeaderRole                    = BigEndian::HostSwap16(aCounters.mLeaderRole);
        mAttachAttempts                = BigEndian::HostSwap16(aCounters.mAttachAttempts);
        mPartitionIdChanges            = BigEndian::HostSwap16(aCounters.mPartitionIdChanges);
        mBetterPartitionAttachAttempts = BigEndian::HostSwap16(aCounters.mBetterPartitionAttachAttempts);
        mParentChanges                 = BigEndian::HostSwap16(aCounters.mParentChanges);
        mTrackedTime                   = BigEndian::HostSwap64(aCounters.mTrackedTime);
        mDisabledTime                  = BigEndian::HostSwap64(aCounters.mDisabledTime);
        mDetachedTime                  = BigEndian::HostSwap64(aCounters.mDetachedTime);
        mChildTime                     = BigEndian::HostSwap64(aCounters.mChildTime);
        mRouterTime                    = BigEndian::HostSwap64(aCounters.mRouterTime);
        mLeaderTime                    = BigEndian::HostSwap64(aCounters.mLeaderTime);
    }

    uint16_t GetDisabledRole(void) const { return BigEndian::HostSwap16(mDisabledRole); }
    uint16_t GetDetachedRole(void) const { return BigEndian::HostSwap16(mDetachedRole); }
    uint16_t GetChildRole(void) const { return BigEndian::HostSwap16(mChildRole); }
    uint16_t GetRouterRole(void) const { return BigEndian::HostSwap16(mRouterRole); }
    uint16_t GetLeaderRole(void) const { return BigEndian::HostSwap16(mLeaderRole); }
    uint16_t GetAttachAttempts(void) const { return BigEndian::HostSwap16(mAttachAttempts); }
    uint16_t GetPartitionIdChanges(void) const { return BigEndian::HostSwap16(mPartitionIdChanges); }
    uint16_t GetBetterPartitionAttachAttempts(void) const
    {
        return BigEndian::HostSwap16(mBetterPartitionAttachAttempts);
    }
    uint16_t GetParentChanges(void) const { return BigEndian::HostSwap16(mParentChanges); }
    uint64_t GetTrackedTime(void) const { return BigEndian::HostSwap64(mTrackedTime); }
    uint64_t GetDisabledTime(void) const { return BigEndian::HostSwap64(mDisabledTime); }
    uint64_t GetDetachedTime(void) const { return BigEndian::HostSwap64(mDetachedTime); }
    uint64_t GetChildTime(void) const { return BigEndian::HostSwap64(mChildTime); }
    uint64_t GetRouterTime(void) const { return BigEndian::HostSwap64(mRouterTime); }
    uint64_t GetLeaderTime(void) const { return BigEndian::HostSwap64(mLeaderTime); }

private:
    uint16_t mDisabledRole;
    uint16_t mDetachedRole;
    uint16_t mChildRole;
    uint16_t mRouterRole;
    uint16_t mLeaderRole;
    uint16_t mAttachAttempts;
    uint16_t mPartitionIdChanges;
    uint16_t mBetterPartitionAttachAttempts;
    uint16_t mParentChanges;
    uint64_t mTrackedTime;
    uint64_t mDisabledTime;
    uint64_t mDetachedTime;
    uint64_t mChildTime;
    uint64_t mRouterTime;
    uint64_t mLeaderTime;
} OT_TOOL_PACKED_END;

/**
 * Link Margin Out TLV (Type 27).
 *
 * Carries outbound link quality metrics from the perspective of a child device.
 * "Out" refers to frames transmitted by the child to its parent.
 * This is a child-provided TLV reported by end devices.
 * Includes link margin, average RSSI, and last RSSI.
 */
OT_TOOL_PACKED_BEGIN
class LinkMarginOutTlv : public Tlv, public TlvInfo<Tlv::kLinkMarginOut>, public LinkMarginTlvFields
{
public:
    /**
     * Initializes the Link Margin Out TLV.
     */
    void Init(void)
    {
        SetType(Tlv::kLinkMarginOut);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }
} OT_TOOL_PACKED_END;

} // namespace ExtNetworkDiagnostic
} // namespace ot

#endif // OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_TLVS_HPP_
