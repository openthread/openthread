/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for MLE types and constants.
 */

#ifndef MLE_TYPES_HPP_
#define MLE_TYPES_HPP_

#include "openthread-core-config.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <openthread/thread.h>
#if OPENTHREAD_FTD
#include <openthread/thread_ftd.h>
#endif

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/ip6_address.hpp"
#include "thread/network_data_types.hpp"

namespace ot {
namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 * @brief
 *   This module includes definition for MLE types and constants.
 *
 * @{
 *
 */

constexpr uint16_t kMaxChildren               = OPENTHREAD_CONFIG_MLE_MAX_CHILDREN;
constexpr uint8_t  kMaxChildKeepAliveAttempts = 4; ///< Max keep alive attempts before reattach to a new Parent.
constexpr uint8_t  kFailedChildTransmissions  = OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
// Extra one for core Backbone Router Service.
constexpr uint8_t kMaxServiceAlocs = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS + 1;
#else
constexpr uint8_t  kMaxServiceAlocs      = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS;
#endif

constexpr uint16_t kUdpPort = 19788; ///< MLE UDP Port

/*
 * MLE Protocol delays and timeouts.
 *
 */
constexpr uint32_t kParentRequestRouterTimeout     = 750;  ///< Router Parent Request timeout (in msec)
constexpr uint32_t kParentRequestDuplicateMargin   = 50;   ///< Margin for duplicate parent request
constexpr uint32_t kParentRequestReedTimeout       = 1250; ///< Router and REEDs Parent Request timeout (in msec)
constexpr uint32_t kChildIdResponseTimeout         = 1250; ///< Wait time to receive Child ID Response (in msec)
constexpr uint32_t kAttachStartJitter              = 50;   ///< Max jitter time added to start of attach (in msec)
constexpr uint32_t kAnnounceProcessTimeout         = 250;  ///< Delay after Announce rx before channel/pan-id change
constexpr uint32_t kAnnounceTimeout                = 1400; ///< Total timeout for sending Announce messages (in msec)
constexpr uint16_t kMinAnnounceDelay               = 80;   ///< Min delay between Announcement messages (in msec)
constexpr uint32_t kParentResponseMaxDelayRouters  = 500;  ///< Max response delay for Parent Req to routers (in msec)
constexpr uint32_t kParentResponseMaxDelayAll      = 1000; ///< Max response delay for Parent Req to all (in msec)
constexpr uint32_t kUnicastRetransmissionDelay     = 1000; ///< Base delay before an MLE unicast retx (in msec)
constexpr uint32_t kChildUpdateRequestPendingDelay = 100;  ///< Delay for aggregating Child Update Req (in msec)
constexpr uint8_t  kMaxTransmissionCount           = 3;    ///< Max number of times an MLE message may be transmitted
constexpr uint32_t kMaxResponseDelay               = 1000; ///< Max response delay for a multicast request (in msec)
constexpr uint32_t kChildIdRequestTimeout          = 5000; ///< Max delay to rx a Child ID Request (in msec)
constexpr uint32_t kLinkRequestTimeout             = 2000; ///< Max delay to rx a Link Accept
constexpr uint8_t  kMulticastLinkRequestDelay      = 5;    ///< Max delay for sending a mcast Link Request (in sec)
constexpr uint8_t kMaxCriticalTransmissionCount = 6; ///< Max number of times an critical MLE message may be transmitted

constexpr uint32_t kMulticastTransmissionDelay = 5000; ///< Delay for retransmitting a multicast packet (in msec)
constexpr uint32_t kMulticastTransmissionDelayMin =
    kMulticastTransmissionDelay * 9 / 10; ///< Min delay for retransmitting a multicast packet (in msec)
constexpr uint32_t kMulticastTransmissionDelayMax =
    kMulticastTransmissionDelay * 11 / 10; ///< Max delay for retransmitting a multicast packet (in msec)

constexpr uint32_t kMinTimeoutKeepAlive = (((kMaxChildKeepAliveAttempts + 1) * kUnicastRetransmissionDelay) / 1000);
constexpr uint32_t kMinPollPeriod       = OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD;
constexpr uint32_t kRetxPollPeriod      = OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD;
constexpr uint32_t kMinTimeoutDataPoll  = (kMinPollPeriod + kFailedChildTransmissions * kRetxPollPeriod) / 1000;
constexpr uint32_t kMinTimeout          = OT_MAX(kMinTimeoutKeepAlive, kMinTimeoutDataPoll); ///< Min timeout (in sec)

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
constexpr uint8_t kLinkAcceptMaxRouters = 3; ///< Max Route TLV entries in a Link Accept message
#else
constexpr uint8_t  kLinkAcceptMaxRouters = 20; ///< Max Route TLV entries in a Link Accept message
#endif
constexpr uint8_t kLinkAcceptSequenceRollback = 64; ///< Route Sequence value rollback in a Link Accept message.

constexpr uint16_t kMinChildId = 1;   ///< Minimum Child ID
constexpr uint16_t kMaxChildId = 511; ///< Maximum Child ID

constexpr uint8_t kRouterIdOffset   = 10; ///< Bit offset of Router ID in RLOC16
constexpr uint8_t kRlocPrefixLength = 14; ///< Prefix length of RLOC in bytes

constexpr uint16_t kMinChallengeSize = 4; ///< Minimum Challenge size in bytes.
constexpr uint16_t kMaxChallengeSize = 8; ///< Maximum Challenge size in bytes.

/*
 * Routing Protocol Constants
 *
 */
constexpr uint32_t kAdvertiseIntervalMin = 1; ///< Min Advertise interval (in sec)
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
constexpr uint32_t kAdvertiseIntervalMax = 5; ///< Max Advertise interval (in sec)
#else
constexpr uint32_t kAdvertiseIntervalMax = 32; ///< Max Advertise interval (in sec)
#endif

constexpr uint8_t kFailedRouterTransmissions = 4;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
constexpr uint8_t kFailedCslDataPollTransmissions = 15;
#endif

constexpr uint8_t  kRouterIdReuseDelay     = 100; ///< (in sec)
constexpr uint32_t kRouterIdSequencePeriod = 10;  ///< (in sec)
constexpr uint32_t kMaxNeighborAge         = 100; ///< (in sec)

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
constexpr uint8_t kMaxRouteCost = 127;
#else
constexpr uint8_t  kMaxRouteCost         = 16;
#endif

constexpr uint8_t kMaxRouterId           = OT_NETWORK_MAX_ROUTER_ID; ///< Max Router ID
constexpr uint8_t kInvalidRouterId       = kMaxRouterId + 1;         ///< Value indicating incorrect Router ID
constexpr uint8_t kMaxRouters            = OPENTHREAD_CONFIG_MLE_MAX_ROUTERS;
constexpr uint8_t kMinDowngradeNeighbors = 7;

constexpr uint8_t kNetworkIdTimeout           = 120; ///< (in sec)
constexpr uint8_t kParentRouteToLeaderTimeout = 20;  ///< (in sec)
constexpr uint8_t kRouterSelectionJitter      = 120; ///< (in sec)

constexpr uint8_t kRouterDowngradeThreshold = 23;
constexpr uint8_t kRouterUpgradeThreshold   = 16;

constexpr uint16_t kInvalidRloc16 = Mac::kShortAddrInvalid; ///< Invalid RLOC16.

/**
 * Threshold to accept a router upgrade request with reason `kBorderRouterRequest` (number of BRs acting as router in
 * Network Data).
 *
 */
constexpr uint8_t kRouterUpgradeBorderRouterRequestThreshold = 2;

constexpr uint32_t kMaxLeaderToRouterTimeout = 90;  ///< (in sec)
constexpr uint32_t kReedAdvertiseInterval    = 570; ///< (in sec)
constexpr uint32_t kReedAdvertiseJitter      = 60;  ///< (in sec)

constexpr uint32_t kMleEndDeviceTimeout      = OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT; ///< (in sec)
constexpr uint8_t  kMeshLocalPrefixContextId = 0; ///< 0 is reserved for Mesh Local Prefix

constexpr int8_t kParentPriorityHigh        = 1;  ///< Parent Priority High
constexpr int8_t kParentPriorityMedium      = 0;  ///< Parent Priority Medium (default)
constexpr int8_t kParentPriorityLow         = -1; ///< Parent Priority Low
constexpr int8_t kParentPriorityUnspecified = -2; ///< Parent Priority Unspecified

/**
 * Represents a Thread device role.
 *
 */
enum DeviceRole : uint8_t
{
    kRoleDisabled = OT_DEVICE_ROLE_DISABLED, ///< The Thread stack is disabled.
    kRoleDetached = OT_DEVICE_ROLE_DETACHED, ///< Not currently participating in a Thread network/partition.
    kRoleChild    = OT_DEVICE_ROLE_CHILD,    ///< The Thread Child role.
    kRoleRouter   = OT_DEVICE_ROLE_ROUTER,   ///< The Thread Router role.
    kRoleLeader   = OT_DEVICE_ROLE_LEADER,   ///< The Thread Leader role.
};

constexpr uint16_t kAloc16Leader                      = 0xfc00;
constexpr uint16_t kAloc16DhcpAgentStart              = 0xfc01;
constexpr uint16_t kAloc16DhcpAgentEnd                = 0xfc0f;
constexpr uint16_t kAloc16ServiceStart                = 0xfc10;
constexpr uint16_t kAloc16ServiceEnd                  = 0xfc2f;
constexpr uint16_t kAloc16CommissionerStart           = 0xfc30;
constexpr uint16_t kAloc16CommissionerEnd             = 0xfc37;
constexpr uint16_t kAloc16BackboneRouterPrimary       = 0xfc38;
constexpr uint16_t kAloc16CommissionerMask            = 0x0007;
constexpr uint16_t kAloc16NeighborDiscoveryAgentStart = 0xfc40;
constexpr uint16_t kAloc16NeighborDiscoveryAgentEnd   = 0xfc4e;

constexpr uint8_t kServiceMinId = 0x00; ///< Minimal Service ID.
constexpr uint8_t kServiceMaxId = 0x0f; ///< Maximal Service ID.

/**
 * Specifies the leader role start mode.
 *
 * The start mode indicates whether device is starting normally as leader or restoring its role after reset.
 *
 */
enum LeaderStartMode : uint8_t
{
    kStartingAsLeader,              ///< Starting as leader normally.
    kRestoringLeaderRoleAfterReset, ///< Restoring leader role after reset.
};

/**
 * Represents a MLE device mode.
 *
 */
class DeviceMode : public Equatable<DeviceMode>
{
public:
    static constexpr uint8_t kModeRxOnWhenIdle     = 1 << 3; ///< If to keep receiver on when not transmitting.
    static constexpr uint8_t kModeReserved         = 1 << 2; ///< Set on transmission, ignore on reception.
    static constexpr uint8_t kModeFullThreadDevice = 1 << 1; ///< If the device is an FTD.
    static constexpr uint8_t kModeFullNetworkData  = 1 << 0; ///< If the device requires the full Network Data.

    static constexpr uint16_t kInfoStringSize = 45; ///< String buffer size used for `ToString()`.

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     *  This structure represents an MLE Mode configuration.
     *
     */
    typedef otLinkModeConfig ModeConfig;

    /**
     * This is the default constructor for `DeviceMode` object.
     *
     */
    DeviceMode(void) = default;

    /**
     * Initializes a `DeviceMode` object from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(uint8_t aMode) { Set(aMode); }

    /**
     * Initializes a `DeviceMode` object from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(ModeConfig aModeConfig) { Set(aModeConfig); }

    /**
     * Gets the device mode as a mode TLV bitmask.
     *
     * @returns The device mode as a mode TLV bitmask.
     *
     */
    uint8_t Get(void) const { return mMode; }

    /**
     * Sets the device mode from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask.
     *
     */
    void Set(uint8_t aMode) { mMode = aMode | kModeReserved; }

    /**
     * Gets the device mode as a mode configuration structure.
     *
     * @param[out] aModeConfig   A reference to a mode configuration structure to output the device mode.
     *
     */
    void Get(ModeConfig &aModeConfig) const;

    /**
     * this method sets the device mode from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration structure.
     *
     */
    void Set(const ModeConfig &aModeConfig);

    /**
     * Indicates whether or not the device is rx-on-when-idle.
     *
     * @retval TRUE   If the device is rx-on-when-idle (non-sleepy).
     * @retval FALSE  If the device is not rx-on-when-idle (sleepy).
     *
     */
    bool IsRxOnWhenIdle(void) const { return (mMode & kModeRxOnWhenIdle) != 0; }

    /**
     * Indicates whether or not the device is a Full Thread Device.
     *
     * @retval TRUE   If the device is Full Thread Device.
     * @retval FALSE  If the device if not Full Thread Device.
     *
     */
    bool IsFullThreadDevice(void) const { return (mMode & kModeFullThreadDevice) != 0; }

    /**
     * Gets the Network Data type (full set or stable subset) that the device requests.
     *
     * @returns The Network Data type requested by this device.
     *
     */
    NetworkData::Type GetNetworkDataType(void) const
    {
        return (mMode & kModeFullNetworkData) ? NetworkData::kFullSet : NetworkData::kStableSubset;
    }

    /**
     * Indicates whether or not the device is a Minimal End Device.
     *
     * @retval TRUE   If the device is a Minimal End Device.
     * @retval FALSE  If the device is not a Minimal End Device.
     *
     */
    bool IsMinimalEndDevice(void) const
    {
        return (mMode & (kModeFullThreadDevice | kModeRxOnWhenIdle)) != (kModeFullThreadDevice | kModeRxOnWhenIdle);
    }

    /**
     * Indicates whether or not the device mode flags are valid.
     *
     * An FTD which is not rx-on-when-idle (is sleepy) is considered invalid.
     *
     * @returns TRUE if , FALSE otherwise.
     * @retval TRUE   If the device mode flags are valid.
     * @retval FALSE  If the device mode flags are not valid.
     *
     */
    bool IsValid(void) const { return !IsFullThreadDevice() || IsRxOnWhenIdle(); }

    /**
     * Converts the device mode into a human-readable string.
     *
     * @returns An `InfoString` object representing the device mode.
     *
     */
    InfoString ToString(void) const;

private:
    uint8_t mMode;
};

#if OPENTHREAD_FTD && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3_1)
/**
 * Represents device properties.
 *
 * The device properties are used for calculating the local leader weight on the device.
 *
 */
class DeviceProperties : public otDeviceProperties, public Clearable<DeviceProperties>
{
public:
    /**
     * Represents the device's power supply property.
     *
     */
    enum PowerSupply : uint8_t
    {
        kPowerSupplyBattery          = OT_POWER_SUPPLY_BATTERY,           ///< Battery powered.
        kPowerSupplyExternal         = OT_POWER_SUPPLY_EXTERNAL,          ///< External powered.
        kPowerSupplyExternalStable   = OT_POWER_SUPPLY_EXTERNAL_STABLE,   ///< Stable external power with backup.
        kPowerSupplyExternalUnstable = OT_POWER_SUPPLY_EXTERNAL_UNSTABLE, ///< Unstable external power.
    };

    /**
     * Initializes `DeviceProperties` with default values.
     *
     */
    DeviceProperties(void);

    /**
     * Clamps the `mLeaderWeightAdjustment` value to the valid range.
     *
     */
    void ClampWeightAdjustment(void);

    /**
     * Calculates the leader weight based on the device properties.
     *
     * @returns The calculated leader weight.
     *
     */
    uint8_t CalculateLeaderWeight(void) const;

private:
    static constexpr int8_t  kDefaultAdjustment        = OPENTHREAD_CONFIG_MLE_DEFAULT_LEADER_WEIGHT_ADJUSTMENT;
    static constexpr uint8_t kBaseWeight               = 64;
    static constexpr int8_t  kBorderRouterInc          = +1;
    static constexpr int8_t  kCcmBorderRouterInc       = +8;
    static constexpr int8_t  kIsUnstableInc            = -4;
    static constexpr int8_t  kPowerBatteryInc          = -8;
    static constexpr int8_t  kPowerExternalInc         = 0;
    static constexpr int8_t  kPowerExternalStableInc   = +4;
    static constexpr int8_t  kPowerExternalUnstableInc = -4;
    static constexpr int8_t  kMinAdjustment            = -16;
    static constexpr int8_t  kMaxAdjustment            = +16;

    static_assert(kDefaultAdjustment >= kMinAdjustment, "Invalid default weight adjustment");
    static_assert(kDefaultAdjustment <= kMaxAdjustment, "Invalid default weight adjustment");
};

#endif // #if OPENTHREAD_FTD && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3_1)

/**
 * Represents the Thread Leader Data.
 *
 */
class LeaderData : public otLeaderData, public Clearable<LeaderData>
{
public:
    /**
     * Returns the Partition ID value.
     *
     * @returns The Partition ID value.
     *
     */
    uint32_t GetPartitionId(void) const { return mPartitionId; }

    /**
     * Sets the Partition ID value.
     *
     * @param[in]  aPartitionId  The Partition ID value.
     *
     */
    void SetPartitionId(uint32_t aPartitionId) { mPartitionId = aPartitionId; }

    /**
     * Returns the Weighting value.
     *
     * @returns The Weighting value.
     *
     */
    uint8_t GetWeighting(void) const { return mWeighting; }

    /**
     * Sets the Weighting value.
     *
     * @param[in]  aWeighting  The Weighting value.
     *
     */
    void SetWeighting(uint8_t aWeighting) { mWeighting = aWeighting; }

    /**
     * Returns the Data Version value for a type (full set or stable subset).
     *
     * @param[in] aType   The Network Data type (full set or stable subset).
     *
     * @returns The Data Version value for @p aType.
     *
     */
    uint8_t GetDataVersion(NetworkData::Type aType) const
    {
        return (aType == NetworkData::kFullSet) ? mDataVersion : mStableDataVersion;
    }

    /**
     * Sets the Data Version value.
     *
     * @param[in]  aVersion  The Data Version value.
     *
     */
    void SetDataVersion(uint8_t aVersion) { mDataVersion = aVersion; }

    /**
     * Sets the Stable Data Version value.
     *
     * @param[in]  aVersion  The Stable Data Version value.
     *
     */
    void SetStableDataVersion(uint8_t aVersion) { mStableDataVersion = aVersion; }

    /**
     * Returns the Leader Router ID value.
     *
     * @returns The Leader Router ID value.
     *
     */
    uint8_t GetLeaderRouterId(void) const { return mLeaderRouterId; }

    /**
     * Sets the Leader Router ID value.
     *
     * @param[in]  aRouterId  The Leader Router ID value.
     *
     */
    void SetLeaderRouterId(uint8_t aRouterId) { mLeaderRouterId = aRouterId; }
};

OT_TOOL_PACKED_BEGIN
class RouterIdSet : public Equatable<RouterIdSet>
{
public:
    /**
     * Clears the Router Id Set.
     *
     */
    void Clear(void) { memset(mRouterIdSet, 0, sizeof(mRouterIdSet)); }

    /**
     * Indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     *
     */
    bool Contains(uint8_t aRouterId) const { return (mRouterIdSet[aRouterId / 8] & MaskFor(aRouterId)) != 0; }

    /**
     * Sets a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to set.
     *
     */
    void Add(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] |= MaskFor(aRouterId); }

    /**
     * Removes a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to remove.
     *
     */
    void Remove(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] &= ~MaskFor(aRouterId); }

    /**
     * Calculates the number of allocated Router IDs in the set.
     *
     * @returns The number of allocated Router IDs in the set.
     *
     */
    uint8_t GetNumberOfAllocatedIds(void) const;

private:
    static uint8_t MaskFor(uint8_t aRouterId) { return (0x80 >> (aRouterId % 8)); }

    uint8_t mRouterIdSet[BitVectorBytes(Mle::kMaxRouterId + 1)];
} OT_TOOL_PACKED_END;

/**
 * Represents a MLE Key Material
 *
 */
typedef Mac::KeyMaterial KeyMaterial;

/**
 * Represents a MLE Key.
 *
 */
typedef Mac::Key Key;

/**
 * Represents the Thread MLE counters.
 *
 */
typedef otMleCounters Counters;

/**
 * Derives the Child ID from a given RLOC16.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @returns The Child ID portion of an RLOC16.
 *
 */
inline uint16_t ChildIdFromRloc16(uint16_t aRloc16) { return aRloc16 & kMaxChildId; }

/**
 * Derives the Router ID portion from a given RLOC16.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @returns The Router ID portion of an RLOC16.
 *
 */
inline uint8_t RouterIdFromRloc16(uint16_t aRloc16) { return aRloc16 >> kRouterIdOffset; }

/**
 * Returns whether the two RLOC16 have the same Router ID.
 *
 * @param[in]  aRloc16A  The first RLOC16 value.
 * @param[in]  aRloc16B  The second RLOC16 value.
 *
 * @returns true if the two RLOC16 have the same Router ID, false otherwise.
 *
 */
inline bool RouterIdMatch(uint16_t aRloc16A, uint16_t aRloc16B)
{
    return RouterIdFromRloc16(aRloc16A) == RouterIdFromRloc16(aRloc16B);
}

/**
 * Returns the Service ID corresponding to a Service ALOC16.
 *
 * @param[in]  aAloc16  The Service ALOC16 value.
 *
 * @returns The Service ID corresponding to given ALOC16.
 *
 */
inline uint8_t ServiceIdFromAloc(uint16_t aAloc16) { return static_cast<uint8_t>(aAloc16 - kAloc16ServiceStart); }

/**
 * Returns the Service ALOC16 corresponding to a Service ID.
 *
 * @param[in]  aServiceId  The Service ID value.
 *
 * @returns The Service ALOC16 corresponding to given ID.
 *
 */
inline uint16_t ServiceAlocFromId(uint8_t aServiceId)
{
    return static_cast<uint16_t>(aServiceId + kAloc16ServiceStart);
}

/**
 * Returns the Commissioner Aloc corresponding to a Commissioner Session ID.
 *
 * @param[in]  aSessionId   The Commissioner Session ID value.
 *
 * @returns The Commissioner ALOC16 corresponding to given ID.
 *
 */
inline uint16_t CommissionerAloc16FromId(uint16_t aSessionId)
{
    return static_cast<uint16_t>((aSessionId & kAloc16CommissionerMask) + kAloc16CommissionerStart);
}

/**
 * Derives RLOC16 from a given Router ID.
 *
 * @param[in]  aRouterId  The Router ID value.
 *
 * @returns The RLOC16 corresponding to the given Router ID.
 *
 */
inline uint16_t Rloc16FromRouterId(uint8_t aRouterId) { return static_cast<uint16_t>(aRouterId << kRouterIdOffset); }

/**
 * Indicates whether or not @p aRloc16 refers to an active router.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @retval TRUE   If @p aRloc16 refers to an active router.
 * @retval FALSE  If @p aRloc16 does not refer to an active router.
 *
 */
inline bool IsActiveRouter(uint16_t aRloc16) { return ChildIdFromRloc16(aRloc16) == 0; }

/**
 * Converts a device role into a human-readable string.
 *
 * @param[in] aRole  The device role to convert.
 *
 * @returns The string representation of @p aRole.
 *
 */
const char *RoleToString(DeviceRole aRole);

/**
 * @}
 *
 */

} // namespace Mle

DefineCoreType(otLeaderData, Mle::LeaderData);
DefineMapEnum(otDeviceRole, Mle::DeviceRole);
#if OPENTHREAD_FTD && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3_1)
DefineCoreType(otDeviceProperties, Mle::DeviceProperties);
DefineMapEnum(otPowerSupply, Mle::DeviceProperties::PowerSupply);
#endif

} // namespace ot

#endif // MLE_TYPES_HPP_
