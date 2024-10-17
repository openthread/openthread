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

#include <stdint.h>
#include <string.h>

#include <openthread/thread.h>
#if OPENTHREAD_FTD
#include <openthread/thread_ftd.h>
#endif

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/numeric_limits.hpp"
#include "common/offset_range.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/ip6_address.hpp"
#include "thread/network_data_types.hpp"

namespace ot {

class Message;

namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 * @brief
 *   This module includes definition for MLE types and constants.
 *
 * @{
 */

constexpr uint16_t kUdpPort = 19788; ///< MLE UDP Port

constexpr uint16_t kMaxChildren     = OPENTHREAD_CONFIG_MLE_MAX_CHILDREN; ///< Maximum number of children
constexpr uint16_t kMinChildId      = 1;                                  ///< Minimum Child ID
constexpr uint16_t kMaxChildId      = 511;                                ///< Maximum Child ID
constexpr uint8_t  kMaxRouters      = OPENTHREAD_CONFIG_MLE_MAX_ROUTERS;  ///< Maximum number of routers
constexpr uint8_t  kMaxRouterId     = OT_NETWORK_MAX_ROUTER_ID;           ///< Max Router ID
constexpr uint8_t  kInvalidRouterId = kMaxRouterId + 1;                   ///< Value indicating invalid Router ID
constexpr uint8_t  kRouterIdOffset  = 10;                                 ///< Bit offset of router ID in RLOC16
constexpr uint16_t kInvalidRloc16   = Mac::kShortAddrInvalid;             ///< Invalid RLOC16.

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
constexpr uint8_t kMaxRouteCost = 127; ///< Maximum path cost
#else
constexpr uint8_t kMaxRouteCost = 16; ///< Maximum path cost
#endif

constexpr uint8_t kMeshLocalPrefixContextId = 0; ///< Reserved 6lowpan context ID for Mesh Local Prefix

/**
 * Number of consecutive tx failures to child (with no-ack error) to consider child-parent link broken.
 */
constexpr uint8_t kFailedChildTransmissions = OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS;

/**
 * Threshold to accept a router upgrade request with reason `kBorderRouterRequest` (number of BRs acting as router in
 * Network Data).
 */
constexpr uint8_t kRouterUpgradeBorderRouterRequestThreshold = 2;

/**
 * Represents a Thread device role.
 */
enum DeviceRole : uint8_t
{
    kRoleDisabled = OT_DEVICE_ROLE_DISABLED, ///< The Thread stack is disabled.
    kRoleDetached = OT_DEVICE_ROLE_DETACHED, ///< Not currently participating in a Thread network/partition.
    kRoleChild    = OT_DEVICE_ROLE_CHILD,    ///< The Thread Child role.
    kRoleRouter   = OT_DEVICE_ROLE_ROUTER,   ///< The Thread Router role.
    kRoleLeader   = OT_DEVICE_ROLE_LEADER,   ///< The Thread Leader role.
};

/**
 * Represents MLE commands.
 */
enum Command : uint8_t
{
    kCommandLinkRequest                   = 0,  ///< Link Request command
    kCommandLinkAccept                    = 1,  ///< Link Accept command
    kCommandLinkAcceptAndRequest          = 2,  ///< Link Accept And Request command
    kCommandLinkReject                    = 3,  ///< Link Reject command
    kCommandAdvertisement                 = 4,  ///< Advertisement command
    kCommandUpdate                        = 5,  ///< Update command
    kCommandUpdateRequest                 = 6,  ///< Update Request command
    kCommandDataRequest                   = 7,  ///< Data Request command
    kCommandDataResponse                  = 8,  ///< Data Response command
    kCommandParentRequest                 = 9,  ///< Parent Request command
    kCommandParentResponse                = 10, ///< Parent Response command
    kCommandChildIdRequest                = 11, ///< Child ID Request command
    kCommandChildIdResponse               = 12, ///< Child ID Response command
    kCommandChildUpdateRequest            = 13, ///< Child Update Request command
    kCommandChildUpdateResponse           = 14, ///< Child Update Response command
    kCommandAnnounce                      = 15, ///< Announce command
    kCommandDiscoveryRequest              = 16, ///< Discovery Request command
    kCommandDiscoveryResponse             = 17, ///< Discovery Response command
    kCommandLinkMetricsManagementRequest  = 18, ///< Link Metrics Management Request command
    kCommandLinkMetricsManagementResponse = 19, ///< Link Metrics Management Response command
    kCommandLinkProbe                     = 20, ///< Link Probe command
    kCommandTimeSync                      = 99, ///< Time Sync command
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

/**
 * Specifies the leader role start mode.
 *
 * The start mode indicates whether device is starting normally as leader or restoring its role after reset.
 */
enum LeaderStartMode : uint8_t
{
    kStartingAsLeader,              ///< Starting as leader normally.
    kRestoringLeaderRoleAfterReset, ///< Restoring leader role after reset.
};

/**
 * Represents a MLE device mode.
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
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     *  This structure represents an MLE Mode configuration.
     */
    typedef otLinkModeConfig ModeConfig;

    /**
     * This is the default constructor for `DeviceMode` object.
     */
    DeviceMode(void) = default;

    /**
     * Initializes a `DeviceMode` object from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask to initialize the `DeviceMode` object.
     */
    explicit DeviceMode(uint8_t aMode) { Set(aMode); }

    /**
     * Initializes a `DeviceMode` object from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration to initialize the `DeviceMode` object.
     */
    explicit DeviceMode(ModeConfig aModeConfig) { Set(aModeConfig); }

    /**
     * Gets the device mode as a mode TLV bitmask.
     *
     * @returns The device mode as a mode TLV bitmask.
     */
    uint8_t Get(void) const { return mMode; }

    /**
     * Sets the device mode from a given mode TLV bitmask.
     *
     * @param[in] aMode   A mode TLV bitmask.
     */
    void Set(uint8_t aMode) { mMode = aMode | kModeReserved; }

    /**
     * Gets the device mode as a mode configuration structure.
     *
     * @param[out] aModeConfig   A reference to a mode configuration structure to output the device mode.
     */
    void Get(ModeConfig &aModeConfig) const;

    /**
     * this method sets the device mode from a given mode configuration structure.
     *
     * @param[in] aModeConfig   A mode configuration structure.
     */
    void Set(const ModeConfig &aModeConfig);

    /**
     * Indicates whether or not the device is rx-on-when-idle.
     *
     * @retval TRUE   If the device is rx-on-when-idle (non-sleepy).
     * @retval FALSE  If the device is not rx-on-when-idle (sleepy).
     */
    bool IsRxOnWhenIdle(void) const { return (mMode & kModeRxOnWhenIdle) != 0; }

    /**
     * Indicates whether or not the device is a Full Thread Device.
     *
     * @retval TRUE   If the device is Full Thread Device.
     * @retval FALSE  If the device if not Full Thread Device.
     */
    bool IsFullThreadDevice(void) const { return (mMode & kModeFullThreadDevice) != 0; }

    /**
     * Gets the Network Data type (full set or stable subset) that the device requests.
     *
     * @returns The Network Data type requested by this device.
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
     */
    bool IsValid(void) const { return !IsFullThreadDevice() || IsRxOnWhenIdle(); }

    /**
     * Converts the device mode into a human-readable string.
     *
     * @returns An `InfoString` object representing the device mode.
     */
    InfoString ToString(void) const;

private:
    uint8_t mMode;
};

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
/**
 * Represents device properties.
 *
 * The device properties are used for calculating the local leader weight on the device.
 */
class DeviceProperties : public otDeviceProperties, public Clearable<DeviceProperties>
{
public:
    /**
     * Represents the device's power supply property.
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
     */
    DeviceProperties(void);

    /**
     * Clamps the `mLeaderWeightAdjustment` value to the valid range.
     */
    void ClampWeightAdjustment(void);

    /**
     * Calculates the leader weight based on the device properties.
     *
     * @returns The calculated leader weight.
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

#endif // #if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE

/**
 * Represents the Thread Leader Data.
 */
class LeaderData : public otLeaderData, public Clearable<LeaderData>
{
public:
    /**
     * Returns the Partition ID value.
     *
     * @returns The Partition ID value.
     */
    uint32_t GetPartitionId(void) const { return mPartitionId; }

    /**
     * Sets the Partition ID value.
     *
     * @param[in]  aPartitionId  The Partition ID value.
     */
    void SetPartitionId(uint32_t aPartitionId) { mPartitionId = aPartitionId; }

    /**
     * Returns the Weighting value.
     *
     * @returns The Weighting value.
     */
    uint8_t GetWeighting(void) const { return mWeighting; }

    /**
     * Sets the Weighting value.
     *
     * @param[in]  aWeighting  The Weighting value.
     */
    void SetWeighting(uint8_t aWeighting) { mWeighting = aWeighting; }

    /**
     * Returns the Data Version value for a type (full set or stable subset).
     *
     * @param[in] aType   The Network Data type (full set or stable subset).
     *
     * @returns The Data Version value for @p aType.
     */
    uint8_t GetDataVersion(NetworkData::Type aType) const
    {
        return (aType == NetworkData::kFullSet) ? mDataVersion : mStableDataVersion;
    }

    /**
     * Sets the Data Version value.
     *
     * @param[in]  aVersion  The Data Version value.
     */
    void SetDataVersion(uint8_t aVersion) { mDataVersion = aVersion; }

    /**
     * Sets the Stable Data Version value.
     *
     * @param[in]  aVersion  The Stable Data Version value.
     */
    void SetStableDataVersion(uint8_t aVersion) { mStableDataVersion = aVersion; }

    /**
     * Returns the Leader Router ID value.
     *
     * @returns The Leader Router ID value.
     */
    uint8_t GetLeaderRouterId(void) const { return mLeaderRouterId; }

    /**
     * Sets the Leader Router ID value.
     *
     * @param[in]  aRouterId  The Leader Router ID value.
     */
    void SetLeaderRouterId(uint8_t aRouterId) { mLeaderRouterId = aRouterId; }
};

OT_TOOL_PACKED_BEGIN
class RouterIdSet : public Equatable<RouterIdSet>, public Clearable<RouterIdSet>
{
public:
    /**
     * Indicates whether or not a Router ID bit is set.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval TRUE   If the Router ID bit is set.
     * @retval FALSE  If the Router ID bit is not set.
     */
    bool Contains(uint8_t aRouterId) const { return (mRouterIdSet[aRouterId / 8] & MaskFor(aRouterId)) != 0; }

    /**
     * Sets a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to set.
     */
    void Add(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] |= MaskFor(aRouterId); }

    /**
     * Removes a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to remove.
     */
    void Remove(uint8_t aRouterId) { mRouterIdSet[aRouterId / 8] &= ~MaskFor(aRouterId); }

    /**
     * Calculates the number of allocated Router IDs in the set.
     *
     * @returns The number of allocated Router IDs in the set.
     */
    uint8_t GetNumberOfAllocatedIds(void) const;

private:
    static uint8_t MaskFor(uint8_t aRouterId) { return (0x80 >> (aRouterId % 8)); }

    uint8_t mRouterIdSet[BytesForBitSize(Mle::kMaxRouterId + 1)];
} OT_TOOL_PACKED_END;

class TxChallenge;

/**
 * Represents a received Challenge data from an MLE message.
 */
class RxChallenge
{
public:
    static constexpr uint8_t kMinSize = 4; ///< Minimum Challenge size in bytes.
    static constexpr uint8_t kMaxSize = 8; ///< Maximum Challenge size in bytes.

    /**
     * Clears the challenge.
     */
    void Clear(void) { mArray.Clear(); }

    /**
     * Indicates whether or not the challenge data is empty.
     *
     * @retval TRUE  The challenge is empty.
     * @retval FALSE The challenge is not empty.
     */
    bool IsEmpty(void) const { return mArray.GetLength() == 0; }

    /**
     * Gets a pointer to challenge data bytes.
     *
     * @return A pointer to the challenge data bytes.
     */
    const uint8_t *GetBytes(void) const { return mArray.GetArrayBuffer(); }

    /**
     * Gets the length of challenge data.
     *
     * @returns The length of challenge data in bytes.
     */
    uint8_t GetLength(void) const { return mArray.GetLength(); }

    /**
     * Reads the challenge bytes from given message.
     *
     * If the given @p aLength is longer than `kMaxSize`, only `kMaxSize` bytes will be read.
     *
     * @param[in] aMessage     The message to read the challenge from.
     * @param[in] aOffsetRange The offset range in @p aMessage to read from.
     *
     * @retval kErrorNone     Successfully read the challenge data from @p aMessage.
     * @retval kErrorParse    Not enough bytes to read, or invalid length (smaller than `kMinSize`).
     */
    Error ReadFrom(const Message &aMessage, const OffsetRange &aOffsetRange);

    /**
     * Compares the `RxChallenge` with a given `TxChallenge`.
     *
     * @param[in] aTxChallenge  The `TxChallenge` to compare with.
     *
     * @retval TRUE  The two challenges are equal.
     * @retval FALSE The two challenges are not equal.
     */
    bool operator==(const TxChallenge &aTxChallenge) const;

private:
    Array<uint8_t, kMaxSize> mArray;
};

/**
 * Represents a max-sized challenge data to send in MLE message.
 *
 * OpenThread always uses max size challenge when sending MLE messages.
 */
class TxChallenge : public Clearable<TxChallenge>
{
    friend class RxChallenge;

public:
    /**
     * Generates a cryptographically secure random sequence to populate the challenge data.
     */
    void GenerateRandom(void);

private:
    uint8_t m8[RxChallenge::kMaxSize];
};

/**
 * Represents a MLE Key Material
 */
typedef Mac::KeyMaterial KeyMaterial;

/**
 * Represents a MLE Key.
 */
typedef Mac::Key Key;

/**
 * Represents the Thread MLE counters.
 */
typedef otMleCounters Counters;

/**
 * Derives the Child ID from a given RLOC16.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @returns The Child ID portion of an RLOC16.
 */
inline uint16_t ChildIdFromRloc16(uint16_t aRloc16) { return aRloc16 & kMaxChildId; }

/**
 * Derives the Router ID portion from a given RLOC16.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @returns The Router ID portion of an RLOC16.
 */
inline uint8_t RouterIdFromRloc16(uint16_t aRloc16) { return aRloc16 >> kRouterIdOffset; }

/**
 * Indicates whether or not a given Router ID is valid.
 *
 * @param[in]  aRouterId  The Router ID value to check.
 *
 * @retval TRUE   If @p aRouterId is in correct range [0..62].
 * @retval FALSE  If @p aRouterId is not a valid Router ID.
 */
inline bool IsRouterIdValid(uint8_t aRouterId) { return aRouterId <= kMaxRouterId; }

/**
 * Returns whether the two RLOC16 have the same Router ID.
 *
 * @param[in]  aRloc16A  The first RLOC16 value.
 * @param[in]  aRloc16B  The second RLOC16 value.
 *
 * @returns true if the two RLOC16 have the same Router ID, false otherwise.
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
 */
inline uint8_t ServiceIdFromAloc(uint16_t aAloc16) { return static_cast<uint8_t>(aAloc16 - kAloc16ServiceStart); }

/**
 * Returns the Service ALOC16 corresponding to a Service ID.
 *
 * @param[in]  aServiceId  The Service ID value.
 *
 * @returns The Service ALOC16 corresponding to given ID.
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
 */
inline uint16_t Rloc16FromRouterId(uint8_t aRouterId) { return static_cast<uint16_t>(aRouterId << kRouterIdOffset); }

/**
 * Derives the router RLOC16 corresponding to the parent of a given (child) RLOC16.
 *
 * If @p aRloc16 itself refers to a router, then the same RLOC16 value is returned.
 *
 * @param[in] aRloc16   An RLOC16.
 *
 * @returns The router RLOC16 corresponding to the parent associated with @p aRloc16.
 */
inline uint16_t ParentRloc16ForRloc16(uint16_t aRloc16) { return Rloc16FromRouterId(RouterIdFromRloc16(aRloc16)); }

/**
 * Indicates whether or not @p aRloc16 refers to a router.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @retval TRUE   If @p aRloc16 refers to a router.
 * @retval FALSE  If @p aRloc16 does not refer to a router.
 */
inline bool IsRouterRloc16(uint16_t aRloc16) { return ChildIdFromRloc16(aRloc16) == 0; }

/**
 * Indicates whether or not @p aRloc16 refers to a child.
 *
 * @param[in]  aRloc16  The RLOC16 value.
 *
 * @retval TRUE   If @p aRloc16 refers to a child.
 * @retval FALSE  If @p aRloc16 does not refer to a child.
 */
inline bool IsChildRloc16(uint16_t aRloc16) { return ChildIdFromRloc16(aRloc16) != 0; }

/**
 * Converts a device role into a human-readable string.
 *
 * @param[in] aRole  The device role to convert.
 *
 * @returns The string representation of @p aRole.
 */
const char *RoleToString(DeviceRole aRole);

/**
 * @}
 */

} // namespace Mle

DefineCoreType(otLeaderData, Mle::LeaderData);
DefineMapEnum(otDeviceRole, Mle::DeviceRole);
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
DefineCoreType(otDeviceProperties, Mle::DeviceProperties);
DefineMapEnum(otPowerSupply, Mle::DeviceProperties::PowerSupply);
#endif

} // namespace ot

#endif // MLE_TYPES_HPP_
