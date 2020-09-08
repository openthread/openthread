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

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6_address.hpp"

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

enum
{
    kMaxChildren               = OPENTHREAD_CONFIG_MLE_MAX_CHILDREN,
    kMaxChildKeepAliveAttempts = 4, ///< Maximum keep alive attempts before attempting to reattach to a new Parent
    kFailedChildTransmissions  = OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS, ///< FAILED_CHILD_TRANSMISSIONS
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    // Extra one for core Backbone Router Service.
    kMaxServiceAlocs = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS + 1,
#else
    kMaxServiceAlocs      = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS,
#endif
};

/**
 * MLE Protocol Constants
 *
 */
enum
{
    kThreadVersion                  = OPENTHREAD_CONFIG_THREAD_VERSION, ///< Thread Version
    kUdpPort                        = 19788,                            ///< MLE UDP Port
    kParentRequestRouterTimeout     = 750,                              ///< Router Parent Request timeout
    kParentRequestDuplicateMargin   = 50,                               ///< Margin for duplicate parent request
    kParentRequestReedTimeout       = 1250,                             ///< Router and REEDs Parent Request timeout
    kAttachStartJitter              = 50,   ///< Maximum jitter time added to start of attach.
    kAnnounceProcessTimeout         = 250,  ///< Timeout after receiving Announcement before channel/pan-id change
    kAnnounceTimeout                = 1400, ///< Total timeout used for sending Announcement messages
    kMinAnnounceDelay               = 80,   ///< Minimum delay between Announcement messages
    kParentResponseMaxDelayRouters  = 500,  ///< Maximum delay for response for Parent Request sent to routers only
    kParentResponseMaxDelayAll      = 1000, ///< Maximum delay for response for Parent Request sent to all devices
    kUnicastRetransmissionDelay     = 1000, ///< Base delay before retransmitting an MLE unicast.
    kChildUpdateRequestPendingDelay = 100,  ///< Delay (in ms) for aggregating Child Update Request.
    kMaxTransmissionCount           = 3,    ///< Maximum number of times an MLE message may be transmitted.
    kMaxResponseDelay               = 1000, ///< Maximum delay before responding to a multicast request
    kMaxChildIdRequestTimeout       = 5000, ///< Maximum delay for receiving a Child ID Request
    kMaxChildUpdateResponseTimeout  = 2000, ///< Maximum delay for receiving a Child Update Response
    kMaxLinkRequestTimeout          = 2000, ///< Maximum delay for receiving a Link Accept
    kMinTimeoutKeepAlive            = (((kMaxChildKeepAliveAttempts + 1) * kUnicastRetransmissionDelay) /
                            1000), ///< Minimum timeout(in seconds) for keep alive
    kMinTimeoutDataPoll             = (OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD +
                           OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS * OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD) /
                          1000, ///< Minimum timeout(in seconds) for data poll
    kMinTimeout = (kMinTimeoutKeepAlive >= kMinTimeoutDataPoll ? kMinTimeoutKeepAlive
                                                               : kMinTimeoutDataPoll), ///< Minimum timeout(in seconds)
};

enum
{
    kMinChildId       = 1,   ///< Minimum Child ID
    kMaxChildId       = 511, ///< Maximum Child ID
    kRouterIdOffset   = 10,  ///< Bit offset of Router ID in RLOC16
    kRlocPrefixLength = 14,  ///< Prefix length of RLOC in bytes
};

/**
 *  MLE TLV Constants
 */
enum
{
    kMinChallengeSize = 4, ///< Minimum Challenge size in bytes.
    kMaxChallengeSize = 8, ///< Maximum Challenge size in bytes.
};

/**
 * Routing Protocol Constants
 *
 */
enum
{
    kAdvertiseIntervalMin = 1, ///< ADVERTISEMENT_I_MIN (sec)
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    kAdvertiseIntervalMax = 5, ///< ADVERTISEMENT_I_MAX (sec) proposal
#else
    kAdvertiseIntervalMax = 32, ///< ADVERTISEMENT_I_MAX (sec)
#endif
    kFailedRouterTransmissions = 4,   ///< FAILED_ROUTER_TRANSMISSIONS
    kRouterIdReuseDelay        = 100, ///< ID_REUSE_DELAY (sec)
    kRouterIdSequencePeriod    = 10,  ///< ID_SEQUENCE_PERIOD (sec)
    kMaxNeighborAge            = 100, ///< MAX_NEIGHBOR_AGE (sec)
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    kMaxRouteCost = 127, ///< MAX_ROUTE_COST proposal
#else
    kMaxRouteCost         = 16, ///< MAX_ROUTE_COST
#endif
    kMaxRouterId                = OT_NETWORK_MAX_ROUTER_ID,                    ///< MAX_ROUTER_ID
    kInvalidRouterId            = kMaxRouterId + 1,                            ///< Value indicating incorrect Router Id
    kMaxRouters                 = OPENTHREAD_CONFIG_MLE_MAX_ROUTERS,           ///< MAX_ROUTERS
    kMinDowngradeNeighbors      = 7,                                           ///< MIN_DOWNGRADE_NEIGHBORS
    kNetworkIdTimeout           = 120,                                         ///< NETWORK_ID_TIMEOUT (sec)
    kParentRouteToLeaderTimeout = 20,                                          ///< PARENT_ROUTE_TO_LEADER_TIMEOUT (sec)
    kRouterSelectionJitter      = 120,                                         ///< ROUTER_SELECTION_JITTER (sec)
    kRouterDowngradeThreshold   = 23,                                          ///< ROUTER_DOWNGRADE_THRESHOLD (routers)
    kRouterUpgradeThreshold     = 16,                                          ///< ROUTER_UPGRADE_THRESHOLD (routers)
    kMaxLeaderToRouterTimeout   = 90,                                          ///< INFINITE_COST_TIMEOUT (sec)
    kReedAdvertiseInterval      = 570,                                         ///< REED_ADVERTISEMENT_INTERVAL (sec)
    kReedAdvertiseJitter        = 60,                                          ///< REED_ADVERTISEMENT_JITTER (sec)
    kLeaderWeight               = 64,                                          ///< Default leader weight
    kMleEndDeviceTimeout        = OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT, ///< MLE_END_DEVICE_TIMEOUT (sec)
    kMeshLocalPrefixContextId   = 0,                                           ///< 0 is reserved for Mesh Local Prefix
};

/**
 * Parent Priority values
 *
 */
enum
{
    kParentPriorityHigh        = 1,  // Parent Priority High
    kParentPriorityMedium      = 0,  // Parent Priority Medium (default)
    kParentPriorityLow         = -1, // Parent Priority Low
    kParentPriorityUnspecified = -2, // Parent Priority Unspecified
};

enum
{
    kLinkQuality3LinkCost = 1,             ///< Link Cost for Link Quality 3
    kLinkQuality2LinkCost = 2,             ///< Link Cost for Link Quality 2
    kLinkQuality1LinkCost = 4,             ///< Link Cost for Link Quality 1
    kLinkQuality0LinkCost = kMaxRouteCost, ///< Link Cost for Link Quality 0
};

/**
 * Multicast Forwarding Constants
 *
 */
enum
{
    kMplChildDataMessageTimerExpirations  = 0, ///< Number of MPL retransmissions for Children.
    kMplRouterDataMessageTimerExpirations = 2, ///< Number of MPL retransmissions for Routers.
};

/**
 * This type represents a Thread device role.
 *
 */
enum DeviceRole
{
    kRoleDisabled = OT_DEVICE_ROLE_DISABLED, ///< The Thread stack is disabled.
    kRoleDetached = OT_DEVICE_ROLE_DETACHED, ///< Not currently participating in a Thread network/partition.
    kRoleChild    = OT_DEVICE_ROLE_CHILD,    ///< The Thread Child role.
    kRoleRouter   = OT_DEVICE_ROLE_ROUTER,   ///< The Thread Router role.
    kRoleLeader   = OT_DEVICE_ROLE_LEADER,   ///< The Thread Leader role.
};

/**
 * MLE Attach modes
 *
 */
enum AttachMode
{
    kAttachAny           = 0, ///< Attach to any Thread partition.
    kAttachSame1         = 1, ///< Attach to the same Thread partition (attempt 1 when losing connectivity).
    kAttachSame2         = 2, ///< Attach to the same Thread partition (attempt 2 when losing connectivity).
    kAttachBetter        = 3, ///< Attach to a better (i.e. higher weight/partition id) Thread partition.
    kAttachSameDowngrade = 4, ///< Attach to the same Thread partition during downgrade process.
};

/**
 * This enumeration represents the allocation of the ALOC Space
 *
 */
enum AlocAllocation
{
    kAloc16Leader                      = 0xfc00,
    kAloc16DhcpAgentStart              = 0xfc01,
    kAloc16DhcpAgentEnd                = 0xfc0f,
    kAloc16DhcpAgentMask               = 0x000f,
    kAloc16ServiceStart                = 0xfc10,
    kAloc16ServiceEnd                  = 0xfc2f,
    kAloc16CommissionerStart           = 0xfc30,
    kAloc16CommissionerEnd             = 0xfc37,
    kAloc16BackboneRouterPrimary       = 0xfc38,
    kAloc16CommissionerMask            = 0x0007,
    kAloc16NeighborDiscoveryAgentStart = 0xfc40,
    kAloc16NeighborDiscoveryAgentEnd   = 0xfc4e,
};

/**
 * Service IDs
 *
 */
enum
{
    kServiceMinId = 0x00, ///< Minimal Service ID.
    kServiceMaxId = 0x0f, ///< Maximal Service ID.
};

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

/**
 * Backbone Router / MLR constants
 *
 */
enum
{
    kRegistrationDelayDefault         = 1200,              ///< In seconds.
    kMlrTimeoutDefault                = 3600,              ///< In seconds.
    kMlrTimeoutMin                    = 300,               ///< In seconds.
    kMlrTimeoutMax                    = 0x7fffffff / 1000, ///< In seconds (about 24 days).
    kBackboneRouterRegistrationJitter = 5,                 ///< In seconds.
    kParentAggregateDelay             = 5,                 ///< In seconds.
    kNoBufDelay                       = 5,                 ///< In seconds.
    kImmediateReRegisterDelay         = 1,                 ///< In seconds.
    KResponseTimeoutDelay             = 30,                ///< In seconds.
    kDuaDadPeriod                     = 100,               ///< In seconds. Time period after which the address
                                                           ///< becomes "Preferred" if no duplicate address error.
    kTimeSinceLastTransactionMax = 10 * 86400,             ///< In seconds (10 days).
};

static_assert(kMlrTimeoutDefault >= kMlrTimeoutMin && kMlrTimeoutDefault <= kMlrTimeoutMax,
              "kMlrTimeoutDefault must be larger than or equal to kMlrTimeoutMin");

static_assert(Mle::kParentAggregateDelay > 1, "kParentAggregateDelay should be larger than 1 second");
static_assert(kMlrTimeoutMax * 1000 > kMlrTimeoutMax, "SecToMsec(kMlrTimeoutMax) will overflow");

static_assert(kTimeSinceLastTransactionMax * 1000 > kTimeSinceLastTransactionMax,
              "SecToMsec(kTimeSinceLastTransactionMax) will overflow");

/**
 * State change of Child's DUA
 *
 */
enum class ChildDuaState : uint8_t
{
    kAdded,   ///< A new DUA registered by the Child via Address Registration.
    kChanged, ///< A different DUA registered by the Child via Address Registration.
    kRemoved, ///< DUA registered by the Child is removed and not in Address Registration.
};

#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

/**
 * This type represents a MLE device mode.
 *
 */
class DeviceMode : public Equatable<DeviceMode>
{
public:
    enum
    {
        kModeRxOnWhenIdle      = 1 << 3, ///< If the device has its receiver on when not transmitting.
        kModeSecureDataRequest = 1 << 2, ///< If the device uses link layer security for all data requests.
        kModeFullThreadDevice  = 1 << 1, ///< If the device is an FTD.
        kModeFullNetworkData   = 1 << 0, ///< If the device requires the full Network Data.

        kInfoStringSize = 45, ///< String buffer size used for `ToString()`.
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
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
    DeviceMode(void) {}

    /**
     * This constructor initializes a `DeviceMode` object from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(uint8_t aMode)
        : mMode(aMode)
    {
    }

    /**
     * This constructor initializes a `DeviceMode` object from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration to initialize the `DeviceMode` object.
     *
     */
    explicit DeviceMode(ModeConfig aModeConfig) { Set(aModeConfig); }

    /**
     * This method gets the device mode as a mode TLV bitmask.
     *
     * @returns The device mode as a mode TLV bitmask.
     *
     */
    uint8_t Get(void) const { return mMode; }

    /**
     * This method sets the device mode from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask.
     *
     */
    void Set(uint8_t aMode) { mMode = aMode; }

    /**
     * This method gets the device mode as a mode configuration structure.
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
     * This method indicates whether or not the device is rx-on-when-idle.
     *
     * @retval TRUE   If the device is rx-on-when-idle (non-sleepy).
     * @retval FALSE  If the device is not rx-on-when-idle (sleepy).
     *
     */
    bool IsRxOnWhenIdle(void) const { return (mMode & kModeRxOnWhenIdle) != 0; }

    /**
     * This method indicates whether or not the device uses secure IEEE 802.15.4 Data Request messages.
     *
     * @retval TRUE   If the device uses secure IEEE 802.15.4 Data Request (data poll) messages.
     * @retval FALSE  If the device uses any IEEE 802.15.4 Data Request (data poll) messages.
     *
     */
    bool IsSecureDataRequest(void) const { return (mMode & kModeSecureDataRequest) != 0; }

    /**
     * This method indicates whether or not the device is a Full Thread Device.
     *
     * @retval TRUE   If the device is Full Thread Device.
     * @retval FALSE  If the device if not Full Thread Device.
     *
     */
    bool IsFullThreadDevice(void) const { return (mMode & kModeFullThreadDevice) != 0; }

    /**
     * This method indicates whether or not the device requests Full Network Data.
     *
     * @retval TRUE   If the device requests Full Network Data.
     * @retval FALSE  If the device does not request Full Network Data (only stable Network Data).
     *
     */
    bool IsFullNetworkData(void) const { return (mMode & kModeFullNetworkData) != 0; }

    /**
     * This method indicates whether or not the device is a Minimal End Device.
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
     * This method indicates whether or not the device mode flags are valid.
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
     * This method converts the device mode into a human-readable string.
     *
     * @returns An `InfoString` object representing the device mode.
     *
     */
    InfoString ToString(void) const;

private:
    uint8_t mMode;
};

/**
 * This class represents a Mesh Local Prefix.
 *
 */
OT_TOOL_PACKED_BEGIN
class MeshLocalPrefix : public Ip6::NetworkPrefix
{
public:
    /**
     * This method derives and sets the Mesh Local Prefix from an Extended PAN ID.
     *
     * @param[in] aExtendedPanId   An Extended PAN ID.
     *
     */
    void SetFromExtendedPanId(const Mac::ExtendedPanId &aExtendedPanId);

} OT_TOOL_PACKED_END;

/**
 * This class represents the Thread Leader Data.
 *
 */
class LeaderData : public otLeaderData, public Clearable<LeaderData>
{
public:
    /**
     * This method returns the Partition ID value.
     *
     * @returns The Partition ID value.
     *
     */
    uint32_t GetPartitionId(void) const { return mPartitionId; }

    /**
     * This method sets the Partition ID value.
     *
     * @param[in]  aPartitionId  The Partition ID value.
     *
     */
    void SetPartitionId(uint32_t aPartitionId) { mPartitionId = aPartitionId; }

    /**
     * This method returns the Weighting value.
     *
     * @returns The Weighting value.
     *
     */
    uint8_t GetWeighting(void) const { return mWeighting; }

    /**
     * This method sets the Weighting value.
     *
     * @param[in]  aWeighting  The Weighting value.
     *
     */
    void SetWeighting(uint8_t aWeighting) { mWeighting = aWeighting; }

    /**
     * This method returns the Data Version value.
     *
     * @returns The Data Version value.
     *
     */
    uint8_t GetDataVersion(void) const { return mDataVersion; }

    /**
     * This method sets the Data Version value.
     *
     * @param[in]  aVersion  The Data Version value.
     *
     */
    void SetDataVersion(uint8_t aVersion) { mDataVersion = aVersion; }

    /**
     * This method returns the Stable Data Version value.
     *
     * @returns The Stable Data Version value.
     *
     */
    uint8_t GetStableDataVersion(void) const { return mStableDataVersion; }

    /**
     * This method sets the Stable Data Version value.
     *
     * @param[in]  aVersion  The Stable Data Version value.
     *
     */
    void SetStableDataVersion(uint8_t aVersion) { mStableDataVersion = aVersion; }

    /**
     * This method returns the Leader Router ID value.
     *
     * @returns The Leader Router ID value.
     *
     */
    uint8_t GetLeaderRouterId(void) const { return mLeaderRouterId; }

    /**
     * This method sets the Leader Router ID value.
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
     * This method clears the Router Id Set.
     *
     */
    void Clear(void) { memset(mRouterIdSet, 0, sizeof(mRouterIdSet)); }

    /**
     * This method indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     *
     */
    bool Contains(uint8_t aRouterId) const { return (mRouterIdSet[aRouterId / 8] & (0x80 >> (aRouterId % 8))) != 0; }

    /**
     * This method sets a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to set.
     *
     */
    void Add(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] |= 0x80 >> (aRouterId % 8); }

    /**
     * This method removes a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to remove.
     *
     */
    void Remove(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] &= ~(0x80 >> (aRouterId % 8)); }

private:
    uint8_t mRouterIdSet[BitVectorBytes(Mle::kMaxRouterId + 1)];
} OT_TOOL_PACKED_END;

/**
 * This class represents a MLE key.
 *
 */
typedef Mac::Key Key;

/**
 * @}
 *
 */

} // namespace Mle
} // namespace ot

#endif // MLE_TYPES_HPP_
