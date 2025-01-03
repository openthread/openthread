/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for the RA-based routing management.
 */

#ifndef ROUTING_MANAGER_HPP_
#define ROUTING_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if !OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#error "OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE is required for OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
#error "OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE is required for OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_UPTIME_ENABLE
#error "OPENTHREAD_CONFIG_UPTIME_ENABLE is required for OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE"
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE && !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
#error "TRACK_PEER_BR_INFO_ENABLE feature requires OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE"
#endif

#include <openthread/border_routing.h>
#include <openthread/nat64.h>
#include <openthread/netdata.h>

#include "border_router/infra_if.hpp"
#include "common/array.hpp"
#include "common/callback.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_array.hpp"
#include "common/heap_data.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/notifier.hpp"
#include "common/owning_list.hpp"
#include "common/pool.hpp"
#include "common/string.hpp"
#include "common/timer.hpp"
#include "crypto/sha256.hpp"
#include "net/ip6.hpp"
#include "net/nat64_translator.hpp"
#include "net/nd6.hpp"
#include "thread/network_data.hpp"

namespace ot {
namespace BorderRouter {

extern "C" void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength);
extern "C" void otPlatBorderRoutingProcessDhcp6PdPrefix(otInstance                            *aInstance,
                                                        const otBorderRoutingPrefixTableEntry *aPrefixInfo);

/**
 * Implements bi-directional routing between Thread and Infrastructure networks.
 *
 * The Border Routing manager works on both Thread interface and infrastructure interface.
 * All ICMPv6 messages are sent/received on the infrastructure interface.
 */
class RoutingManager : public InstanceLocator
{
    friend class ot::Notifier;
    friend class ot::Instance;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    friend void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength);
    friend void otPlatBorderRoutingProcessDhcp6PdPrefix(otInstance                            *aInstance,
                                                        const otBorderRoutingPrefixTableEntry *aPrefixInfo);
#endif

public:
    typedef NetworkData::RoutePreference          RoutePreference;     ///< Route preference (high, medium, low).
    typedef otBorderRoutingPrefixTableIterator    PrefixTableIterator; ///< Prefix Table Iterator.
    typedef otBorderRoutingPrefixTableEntry       PrefixTableEntry;    ///< Prefix Table Entry.
    typedef otBorderRoutingRouterEntry            RouterEntry;         ///< Router Entry.
    typedef otBorderRoutingPeerBorderRouterEntry  PeerBrEntry;         ///< Peer Border Router Entry.
    typedef otPdProcessedRaInfo                   PdProcessedRaInfo;   ///< Data of PdProcessedRaInfo.
    typedef otBorderRoutingRequestDhcp6PdCallback PdCallback;          ///< DHCPv6 PD callback.

    /**
     * This constant specifies the maximum number of route prefixes that may be published by `RoutingManager`
     * in Thread Network Data.
     *
     * This is used by `NetworkData::Publisher` to reserve entries for use by `RoutingManager`.
     *
     * The number of published entries accounts for:
     * - Route prefix `fc00::/7` or `::/0`
     * - One entry for NAT64 published prefix.
     * - One extra entry for transitions.
     */
    static constexpr uint16_t kMaxPublishedPrefixes = 3;

    /**
     * Represents the states of `RoutingManager`.
     */
    enum State : uint8_t
    {
        kStateUninitialized = OT_BORDER_ROUTING_STATE_UNINITIALIZED, ///< Uninitialized.
        kStateDisabled      = OT_BORDER_ROUTING_STATE_DISABLED,      ///< Initialized but disabled.
        kStateStopped       = OT_BORDER_ROUTING_STATE_STOPPED,       ///< Initialized & enabled, but currently stopped.
        kStateRunning       = OT_BORDER_ROUTING_STATE_RUNNING,       ///< Initialized, enabled, and running.
    };

    /**
     * This enumeration represents the states of DHCPv6 PD in `RoutingManager`.
     */
    enum Dhcp6PdState : uint8_t
    {
        kDhcp6PdStateDisabled = OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED, ///< Disabled.
        kDhcp6PdStateStopped  = OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED,  ///< Enabled, but currently stopped.
        kDhcp6PdStateRunning  = OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING,  ///< Enabled, and running.
        kDhcp6PdStateIdle     = OT_BORDER_ROUTING_DHCP6_PD_STATE_IDLE,     ///< Enabled, but is not requesting prefix.
    };

    /**
     * Initializes the routing manager.
     *
     * @param[in]  aInstance  A OpenThread instance.
     */
    explicit RoutingManager(Instance &aInstance);

    /**
     * Initializes the routing manager on given infrastructure interface.
     *
     * @param[in]  aInfraIfIndex      An infrastructure network interface index.
     * @param[in]  aInfraIfIsRunning  A boolean that indicates whether the infrastructure
     *                                interface is running.
     *
     * @retval  kErrorNone         Successfully started the routing manager.
     * @retval  kErrorInvalidArgs  The index of the infra interface is not valid.
     */
    Error Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning);

    /**
     * Enables/disables the Border Routing Manager.
     *
     * @note  The Border Routing Manager is enabled by default.
     *
     * @param[in]  aEnabled   A boolean to enable/disable the Border Routing Manager.
     *
     * @retval  kErrorInvalidState   The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone           Successfully enabled/disabled the Border Routing Manager.
     */
    Error SetEnabled(bool aEnabled);

    /**
     * Indicates whether or not it is currently running.
     *
     * In order for the `RoutingManager` to be running it needs to be initialized and enabled, and device being
     * attached.
     *
     * @retval TRUE  The RoutingManager is currently running.
     * @retval FALSE The RoutingManager is not running.
     */
    bool IsRunning(void) const { return mIsRunning; }

    /**
     * Gets the state of `RoutingManager`.
     *
     * @returns The current state of `RoutingManager`.
     */
    State GetState(void) const;

    /**
     * Requests the Border Routing Manager to stop.
     *
     * If Border Routing Manager is running, calling this method immediately stops it and triggers the preparation
     * and sending of a final Router Advertisement (RA) message on infrastructure interface which deprecates and/or
     * removes any previously advertised PIO/RIO prefixes. If Routing Manager is not running (or not enabled), no
     * action is taken.
     *
     * Note that this method does not change whether the Routing Manager is enabled or disabled (see `SetEnabled()`).
     * It stops the Routing Manager temporarily. After calling this method if the device role gets changes (device
     * gets attached) and/or the infra interface state gets changed, the Routing Manager may be started again.
     */
    void RequestStop(void) { Stop(); }

    /**
     * Gets the current preference used when advertising Route Info Options (RIO) in Router Advertisement
     * messages sent over the infrastructure link.
     *
     * The RIO preference is determined as follows:
     *
     * - If explicitly set by user by calling `SetRouteInfoOptionPreference()`, the given preference is used.
     * - Otherwise, it is determined based on device's role: Medium preference when in router/leader role and low
     *   preference when in child role.
     *
     * @returns The current Route Info Option preference.
     */
    RoutePreference GetRouteInfoOptionPreference(void) const { return mRioAdvertiser.GetPreference(); }

    /**
     * Explicitly sets the preference to use when advertising Route Info Options (RIO) in Router
     * Advertisement messages sent over the infrastructure link.
     *
     * After a call to this method, BR will use the given preference for all its advertised RIOs. The preference can be
     * cleared by calling `ClearRouteInfoOptionPreference`()`.
     *
     * @param[in] aPreference   The route preference to use.
     */
    void SetRouteInfoOptionPreference(RoutePreference aPreference) { mRioAdvertiser.SetPreference(aPreference); }

    /**
     * Clears a previously set preference value for advertised Route Info Options.
     *
     * After a call to this method, BR will use device role to determine the RIO preference: Medium preference when
     * in router/leader role and low preference when in child role.
     */
    void ClearRouteInfoOptionPreference(void) { mRioAdvertiser.ClearPreference(); }

    /**
     * Sets additional options to append at the end of emitted Router Advertisement (RA) messages.
     *
     * The content of @p aOptions is copied internally, so can be a temporary stack variable.
     *
     * Subsequent calls to this method will overwrite the previously set value.
     *
     * @param[in] aOptions   A pointer to the encoded options. Can be `nullptr` to clear.
     * @param[in] aLength    Number of bytes in @p aOptions.
     *
     * @retval kErrorNone     Successfully set the extra option bytes.
     * @retval kErrorNoBufs   Could not allocate buffer to save the buffer.
     */
    Error SetExtraRouterAdvertOptions(const uint8_t *aOptions, uint16_t aLength);

    /**
     * Gets the current preference used for published routes in Network Data.
     *
     * The preference is determined as follows:
     *
     * - If explicitly set by user by calling `SetRoutePreference()`, the given preference is used.
     * - Otherwise, it is determined automatically by `RoutingManager` based on the device's role and link quality.
     *
     * @returns The current published route preference.
     */
    RoutePreference GetRoutePreference(void) const { return mRoutePublisher.GetPreference(); }

    /**
     * Explicitly sets the preference of published routes in Network Data.
     *
     * After a call to this method, BR will use the given preference. The preference can be cleared by calling
     * `ClearRoutePreference`()`.
     *
     * @param[in] aPreference   The route preference to use.
     */
    void SetRoutePreference(RoutePreference aPreference) { mRoutePublisher.SetPreference(aPreference); }

    /**
     * Clears a previously set preference value for published routes in Network Data.
     *
     * After a call to this method, BR will determine the preference automatically based on the device's role and
     * link quality (to the parent when acting as end-device).
     */
    void ClearRoutePreference(void) { mRoutePublisher.ClearPreference(); }

    /**
     * Returns the local generated off-mesh-routable (OMR) prefix.
     *
     * The randomly generated 64-bit prefix will be added to the Thread Network Data if there isn't already an OMR
     * prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the OMR prefix.
     */
    Error GetOmrPrefix(Ip6::Prefix &aPrefix) const;

    /**
     * Returns the currently favored off-mesh-routable (OMR) prefix.
     *
     * The favored OMR prefix can be discovered from Network Data or can be our local OMR prefix.
     *
     * An OMR prefix with higher preference is favored. If the preference is the same, then the smaller prefix (in the
     * sense defined by `Ip6::Prefix`) is favored.
     *
     * @param[out] aPrefix         A reference to output the favored prefix.
     * @param[out] aPreference     A reference to output the preference associated with the favored OMR prefix.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not running yet.
     * @retval  kErrorNone          Successfully retrieved the OMR prefix.
     */
    Error GetFavoredOmrPrefix(Ip6::Prefix &aPrefix, RoutePreference &aPreference) const;

    /**
     * Returns the on-link prefix for the adjacent infrastructure link.
     *
     * The randomly generated 64-bit prefix will be advertised
     * on the infrastructure link if there isn't already a usable
     * on-link prefix being advertised on the link.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the local on-link prefix.
     */
    Error GetOnLinkPrefix(Ip6::Prefix &aPrefix) const;

    /**
     * Returns the favored on-link prefix for the adjacent infrastructure link.
     *
     * The favored prefix is either a discovered prefix on the infrastructure link or the local on-link prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the favored on-link prefix.
     */
    Error GetFavoredOnLinkPrefix(Ip6::Prefix &aPrefix) const;

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    /**
     * Gets the state of NAT64 prefix publishing.
     *
     * @retval  kStateDisabled   NAT64 is disabled.
     * @retval  kStateNotRunning NAT64 is enabled, but is not running since routing manager is not running.
     * @retval  kStateIdle       NAT64 is enabled, but the border router is not publishing a NAT64 prefix. Usually
     *                           when there is another border router publishing a NAT64 prefix with higher
     *                           priority.
     * @retval  kStateActive     The Border router is publishing a NAT64 prefix.
     */
    Nat64::State GetNat64PrefixManagerState(void) const { return mNat64PrefixManager.GetState(); }

    /**
     * Enable or disable NAT64 prefix publishing.
     *
     * @param[in]  aEnabled   A boolean to enable/disable NAT64 prefix publishing.
     */
    void SetNat64PrefixManagerEnabled(bool aEnabled);

    /**
     * Returns the local NAT64 prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     */
    Error GetNat64Prefix(Ip6::Prefix &aPrefix);

    /**
     * Returns the currently favored NAT64 prefix.
     *
     * The favored NAT64 prefix can be discovered from infrastructure link or can be the local NAT64 prefix.
     *
     * @param[out] aPrefix           A reference to output the favored prefix.
     * @param[out] aRoutePreference  A reference to output the preference associated with the favored prefix.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     */
    Error GetFavoredNat64Prefix(Ip6::Prefix &aPrefix, RoutePreference &aRoutePreference);

    /**
     * Informs `RoutingManager` of the result of the discovery request of NAT64 prefix on infrastructure
     * interface (`InfraIf::DiscoverNat64Prefix()`).
     *
     * @param[in]  aPrefix  The discovered NAT64 prefix on `InfraIf`.
     */
    void HandleDiscoverNat64PrefixDone(const Ip6::Prefix &aPrefix) { mNat64PrefixManager.HandleDiscoverDone(aPrefix); }

#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

    /**
     * Processes a received ICMPv6 message from the infrastructure interface.
     *
     * Malformed or undesired messages are dropped silently.
     *
     * @param[in]  aPacket        The received ICMPv6 packet.
     * @param[in]  aSrcAddress    The source address this message is sent from.
     */
    void HandleReceived(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);

    /**
     * Handles infrastructure interface state changes.
     */
    void HandleInfraIfStateChanged(void) { EvaluateState(); }

    /**
     * Checks whether the on-mesh prefix configuration is a valid OMR prefix.
     *
     * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALSE   The prefix is not a valid OMR prefix.
     */
    static bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

    /**
     * Checks whether a given prefix is a valid OMR prefix.
     *
     * @param[in]  aPrefix  The prefix to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALSE   The prefix is not a valid OMR prefix.
     */
    static bool IsValidOmrPrefix(const Ip6::Prefix &aPrefix);

    /**
     * Initializes a `PrefixTableIterator`.
     *
     * An iterator can be initialized again to start from the beginning of the table.
     *
     * When iterating over entries in the table, to ensure the entry update times are consistent, they are given
     * relative to the time the iterator was initialized.
     *
     * @param[out] aIterator  The iterator to initialize.
     */
    void InitPrefixTableIterator(PrefixTableIterator &aIterator) const { mRxRaTracker.InitIterator(aIterator); }

    /**
     * Iterates over entries in the discovered prefix table.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next entry, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more entries in the table.
     */
    Error GetNextPrefixTableEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const
    {
        return mRxRaTracker.GetNextEntry(aIterator, aEntry);
    }

    /**
     * Iterates over discovered router entries on infrastructure link.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next router info, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more routers.
     */
    Error GetNextRouterEntry(PrefixTableIterator &aIterator, RouterEntry &aEntry) const
    {
        return mRxRaTracker.GetNextRouter(aIterator, aEntry);
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

    /**
     * Iterates over the peer BRs found in the Network Data.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next peer BR info, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more PR beers in the list.
     */
    Error GetNextPeerBrEntry(PrefixTableIterator &aIterator, PeerBrEntry &aEntry) const
    {
        return mNetDataPeerBrTracker.GetNext(aIterator, aEntry);
    }

    /**
     * Returns the number of peer BRs found in the Network Data.
     *
     * The count does not include this device itself (when it itself is acting as a BR).
     *
     * @param[out] aMinAge   Reference to an `uint32_t` to return the minimum age among all peer BRs.
     *                       Age is represented as seconds since appearance of the BR entry in the Network Data.
     *
     * @returns The number of peer BRs.
     */
    uint16_t CountPeerBrs(uint32_t &aMinAge) const { return mNetDataPeerBrTracker.CountPeerBrs(aMinAge); }

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    /**
     * Determines whether to enable/disable SRP server when the auto-enable mode is changed on SRP server.
     *
     * This should be called from `Srp::Server` when auto-enable mode is changed.
     */
    void HandleSrpServerAutoEnableMode(void);
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    /**
     * Enables / Disables the DHCPv6 Prefix Delegation.
     *
     * @param[in] aEnabled  Whether to enable or disable.
     */
    void SetDhcp6PdEnabled(bool aEnabled) { return mPdPrefixManager.SetEnabled(aEnabled); }

    /**
     * Returns the state DHCPv6 Prefix Delegation manager.
     *
     * @returns The DHCPv6 PD state.
     */
    Dhcp6PdState GetDhcp6PdState(void) const { return mPdPrefixManager.GetState(); }

    /**
     * Sets the callback to notify when DHCPv6 Prefix Delegation manager state gets changed.
     *
     * @param[in] aCallback  A pointer to a callback function
     * @param[in] aContext   A pointer to arbitrary context information.
     */
    void SetRequestDhcp6PdCallback(PdCallback aCallback, void *aContext)
    {
        mPdPrefixManager.SetStateCallback(aCallback, aContext);
    }

    /**
     * Returns the DHCPv6-PD based off-mesh-routable (OMR) prefix.
     *
     * @param[out] aPrefixInfo      A reference to where the prefix info will be output to.
     *
     * @retval kErrorNone           Successfully retrieved the OMR prefix.
     * @retval kErrorNotFound       There are no valid PD prefix on this BR.
     * @retval kErrorInvalidState   The Border Routing Manager is not initialized yet.
     */
    Error GetPdOmrPrefix(PrefixTableEntry &aPrefixInfo) const;

    /**
     * Returns platform generated RA message processed counters and information.
     *
     * @param[out] aPdProcessedRaInfo  A reference to where the PD processed RA info will be output to.
     *
     * @retval kErrorNone           Successfully retrieved the Info.
     * @retval kErrorNotFound       There are no valid RA process info on this BR.
     * @retval kErrorInvalidState   The Border Routing Manager is not initialized yet.
     */
    Error GetPdProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo);

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_REACHABILITY_CHECK_ICMP6_ERROR_ENABLE
    /**
     * Determines whether to send an ICMPv6 Destination Unreachable error to the sender based on reachability and
     * source address.
     *
     * Specifically, if the Border Router (BR) decides to forward a unicast IPv6 message outside the AIL and the
     * message's source address matches a BR-generated ULA OMR prefix (with low preference), and the destination is
     * unreachable using this source address, then an ICMPv6 Destination Unreachable message is sent back to the sender.
     *
     * @param[in] aMessage    The message.
     * @param[in] aIp6Header  The IPv6 header of @p aMessage.
     */
    void CheckReachabilityToSendIcmpError(const Message &aMessage, const Ip6::Header &aIp6Header);
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TESTING_API_ENABLE
    /**
     * Sets the local on-link prefix.
     *
     * This is intended for testing only and using it will make a device non-compliant with the Thread Specification.
     *
     * @param[in]  aPrefix      The on-link prefix to use.
     */
    void SetOnLinkPrefix(const Ip6::Prefix &aPrefix) { mOnLinkPrefixManager.SetLocalPrefix(aPrefix); }
#endif

private:
    //------------------------------------------------------------------------------------------------------------------
    // Constants

    static constexpr uint8_t kMaxOnMeshPrefixes = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES;

    // Prefix length in bits.
    static constexpr uint8_t kOmrPrefixLength    = 64;
    static constexpr uint8_t kOnLinkPrefixLength = 64;
    static constexpr uint8_t kBrUlaPrefixLength  = 48;
    static constexpr uint8_t kNat64PrefixLength  = 96;

    // Subnet IDs for OMR and NAT64 prefixes.
    static constexpr uint16_t kOmrPrefixSubnetId   = 1;
    static constexpr uint16_t kNat64PrefixSubnetId = 2;

    // Default valid lifetime. In seconds.
    static constexpr uint32_t kDefaultOmrPrefixLifetime    = 1800;
    static constexpr uint32_t kDefaultOnLinkPrefixLifetime = 1800;
    static constexpr uint32_t kDefaultNat64PrefixLifetime  = 300;

    // The entry stale time in seconds.
    //
    // The amount of time that can pass after the last time an RA from
    // a particular router has been received advertising an on-link
    // or route prefix before we assume the prefix entry is stale.
    //
    // If multiple routers advertise the same on-link or route prefix,
    // the stale time for the prefix is determined by the latest
    // stale time among all corresponding entries. Stale time
    // expiration triggers tx of Router Solicitation (RS) messages

    static constexpr uint32_t kStaleTime = 600; // 10 minutes.

    // RA transmission constants (in milliseconds). Initially, three
    // RAs are sent with a short interval of 16 seconds (± 2 seconds
    // jitter). Subsequently, a longer, regular RA beacon interval of
    // 3 minutes (± 15 seconds jitter) is used. The actual interval is
    // randomly selected within the range [interval - jitter,
    // interval + jitter].

    static constexpr uint32_t kInitalRaTxCount    = 3;
    static constexpr uint32_t kInitalRaInterval   = Time::kOneSecondInMsec * 16;
    static constexpr uint16_t kInitialRaJitter    = Time::kOneSecondInMsec * 2;
    static constexpr uint32_t kRaBeaconInterval   = Time::kOneSecondInMsec * 180; // 3 minutes
    static constexpr uint16_t kRaBeaconJitter     = Time::kOneSecondInMsec * 15;
    static constexpr uint32_t kMinDelayBetweenRas = Time::kOneSecondInMsec * 3;
    static constexpr uint32_t kRsReplyInterval    = 250;
    static constexpr uint16_t kRsReplyJitter      = 250;
    static constexpr uint32_t kEvaluationInterval = Time::kOneSecondInMsec * 3;
    static constexpr uint16_t kEvaluationJitter   = Time::kOneSecondInMsec * 1;

    //------------------------------------------------------------------------------------------------------------------
    // Typedefs

    using Option                = Ip6::Nd::Option;
    using PrefixInfoOption      = Ip6::Nd::PrefixInfoOption;
    using RouteInfoOption       = Ip6::Nd::RouteInfoOption;
    using RaFlagsExtOption      = Ip6::Nd::RaFlagsExtOption;
    using RouterAdvert          = Ip6::Nd::RouterAdvert;
    using NeighborAdvertMessage = Ip6::Nd::NeighborAdvertMessage;
    using TxMessage             = Ip6::Nd::TxMessage;
    using NeighborSolicitHeader = Ip6::Nd::NeighborSolicitHeader;
    using RouterSolicitHeader   = Ip6::Nd::RouterSolicitHeader;
    using LinkLayerAddress      = InfraIf::LinkLayerAddress;

    //------------------------------------------------------------------------------------------------------------------
    // Enumerations

    enum RouterAdvTxMode : uint8_t // Used in `SendRouterAdvertisement()`
    {
        kInvalidateAllPrevPrefixes,
        kAdvPrefixesFromNetData,
    };

    enum RouterAdvOrigin : uint8_t // Origin of a received Router Advert message.
    {
        kAnotherRouter,        // From another router on infra-if.
        kThisBrRoutingManager, // From this BR generated by `RoutingManager` itself.
        kThisBrOtherEntity,    // From this BR generated by another sw entity.
    };

    enum ScheduleMode : uint8_t // Used in `ScheduleRoutingPolicyEvaluation()`
    {
        kImmediately,
        kForNextRa,
        kAfterRandomDelay,
        kToReplyToRs,
    };

    //------------------------------------------------------------------------------------------------------------------
    // Nested types

    class LifetimedPrefix
    {
        // Represents an IPv6 prefix with its valid lifetime. Used as
        // base class for `OnLinkPrefix` or `RoutePrefix`.

    public:
        struct ExpirationChecker
        {
            explicit ExpirationChecker(TimeMilli aNow) { mNow = aNow; }
            TimeMilli mNow;
        };

        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        Ip6::Prefix       &GetPrefix(void) { return mPrefix; }
        const TimeMilli   &GetLastUpdateTime(void) const { return mLastUpdateTime; }
        uint32_t           GetValidLifetime(void) const { return mValidLifetime; }
        TimeMilli          GetExpireTime(void) const { return CalculateExpirationTime(mValidLifetime); }

        bool Matches(const Ip6::Prefix &aPrefix) const { return (mPrefix == aPrefix); }
        bool Matches(const ExpirationChecker &aChecker) const { return (GetExpireTime() <= aChecker.mNow); }

        void SetStaleTimeCalculated(bool aFlag) { mStaleTimeCalculated = aFlag; }
        bool IsStaleTimeCalculated(void) const { return mStaleTimeCalculated; }

        void SetDisregardFlag(bool aFlag) { mDisregard = aFlag; }
        bool ShouldDisregard(void) const { return mDisregard; }

    protected:
        LifetimedPrefix(void) = default;

        TimeMilli CalculateExpirationTime(uint32_t aLifetime) const;

        Ip6::Prefix mPrefix;
        bool        mDisregard : 1;
        bool        mStaleTimeCalculated : 1;
        uint32_t    mValidLifetime;
        TimeMilli   mLastUpdateTime;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class OnLinkPrefix : public LifetimedPrefix, public Clearable<OnLinkPrefix>
    {
    public:
        void      SetFrom(const PrefixInfoOption &aPio);
        void      SetFrom(const PrefixTableEntry &aPrefixTableEntry);
        uint32_t  GetPreferredLifetime(void) const { return mPreferredLifetime; }
        void      ClearPreferredLifetime(void) { mPreferredLifetime = 0; }
        bool      IsDeprecated(void) const;
        TimeMilli GetDeprecationTime(void) const;
        TimeMilli GetStaleTime(void) const;
        void      AdoptValidAndPreferredLifetimesFrom(const OnLinkPrefix &aPrefix);
        void      CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const;
        bool      IsFavoredOver(const Ip6::Prefix &aPrefix) const;

    private:
        static constexpr uint32_t kFavoredMinPreferredLifetime = 1800; // In sec.

        uint32_t mPreferredLifetime;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RoutePrefix : public LifetimedPrefix, public Clearable<RoutePrefix>
    {
    public:
        void            SetFrom(const RouteInfoOption &aRio);
        void            SetFrom(const RouterAdvert::Header &aRaHeader);
        void            ClearValidLifetime(void) { mValidLifetime = 0; }
        TimeMilli       GetStaleTime(void) const;
        RoutePreference GetRoutePreference(void) const { return mRoutePreference; }
        void            CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const;

    private:
        RoutePreference mRoutePreference;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

    class RxRaTracker;

    class NetDataPeerBrTracker : public InstanceLocator
    {
        friend class RxRaTracker;

    public:
        explicit NetDataPeerBrTracker(Instance &aInstance);

        uint16_t CountPeerBrs(uint32_t &aMinAge) const;
        Error    GetNext(PrefixTableIterator &aIterator, PeerBrEntry &aEntry) const;

        void HandleNotifierEvents(Events aEvents);

    private:
        struct PeerBr : LinkedListEntry<PeerBr>, Heap::Allocatable<PeerBr>
        {
            struct Filter
            {
                Filter(const NetworkData::Rlocs &aRlocs)
                    : mExcludeRlocs(aRlocs)
                {
                }

                const NetworkData::Rlocs &mExcludeRlocs;
            };

            uint32_t GetAge(uint32_t aUptime) const { return aUptime - mDiscoverTime; }
            bool     Matches(uint16_t aRloc16) const { return mRloc16 == aRloc16; }
            bool     Matches(const Filter &aFilter) const { return !aFilter.mExcludeRlocs.Contains(mRloc16); }

            PeerBr  *mNext;
            uint16_t mRloc16;
            uint32_t mDiscoverTime;
        };

        OwningList<PeerBr> mPeerBrs;
    };

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRxRaTrackerSignalTask(void) { mRxRaTracker.HandleSignalTask(); }
    void HandleRxRaTrackerExpirationTimer(void) { mRxRaTracker.HandleExpirationTimer(); }
    void HandleRxRaTrackerStaleTimer(void) { mRxRaTracker.HandleStaleTimer(); }
    void HandleRxRaTrackerRouterTimer(void) { mRxRaTracker.HandleRouterTimer(); }

    class RxRaTracker : public InstanceLocator
    {
        // Processes received RA and NA messages tracking a table of active
        // routers and their advertised on-link and route prefixes. Also
        // manages prefix lifetimes and router reachability (sending NS probes
        // as needed).
        //
        // When there is any change in the table (an entry is added, removed,
        // or modified), it signals the change to `RoutingManager` by calling
        // `HandleRaPrefixTableChanged()` callback. A `Tasklet` is used for
        // signalling which ensures that if there are multiple changes within
        // the same flow of execution, the callback is invoked after all the
        // changes are processed.

        friend class NetDataPeerBrTracker;

    public:
        explicit RxRaTracker(Instance &aInstance);

        void Start(void);
        void Stop(void);

        void ProcessRouterAdvertMessage(const RouterAdvert::RxMessage &aRaMessage,
                                        const Ip6::Address            &aSrcAddress,
                                        RouterAdvOrigin                aRaOrigin);
        void ProcessNeighborAdvertMessage(const NeighborAdvertMessage &aNaMessage);

        // Decision factors
        bool ContainsDefaultOrNonUlaRoutePrefix(void) const { return mDecisionFactors.mHasNonUlaRoute; }
        bool ContainsNonUlaOnLinkPrefix(void) const { return mDecisionFactors.mHasNonUlaOnLink; }
        bool ContainsUlaOnLinkPrefix(void) const { return mDecisionFactors.mHasUlaOnLink; }

        const Ip6::Prefix &GetFavoredOnLinkPrefix(void) const { return mDecisionFactors.mFavoredOnLinkPrefix; }
        void               SetHeaderFlagsOn(RouterAdvert::Header &aHeader) const;

        const RouterAdvert::Header &GetLocalRaHeaderToMirror(void) const { return mLocalRaHeader; }

        bool IsAddressOnLink(const Ip6::Address &aAddress) const;
        bool IsAddressReachableThroughExplicitRoute(const Ip6::Address &aAddress) const;

        // Iterating over discovered items
        void  InitIterator(PrefixTableIterator &aIterator) const;
        Error GetNextEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const;
        Error GetNextRouter(PrefixTableIterator &aIterator, RouterEntry &aEntry) const;

        // Callbacks notifying of changes
        void RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold);
        void HandleLocalOnLinkPrefixChanged(void);
        void HandleNetDataChange(void);

        // Tasklet or timer callbacks
        void HandleSignalTask(void);
        void HandleExpirationTimer(void);
        void HandleStaleTimer(void);
        void HandleRouterTimer(void);

    private:
        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

        template <class Type>
        struct Entry : public Type,
                       public LinkedListEntry<Entry<Type>>,
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
                       public Heap::Allocatable<Entry<Type>>
#else
                       public InstanceLocatorInit
#endif
        {
#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
            void Init(Instance &aInstance) { InstanceLocatorInit::Init(aInstance); }
            void Free(void);
#endif

            Entry<Type> *mNext;
        };

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

        struct Router : public Clearable<Router>
        {
            // Reachability timeout intervals before starting Neighbor
            // Solicitation (NS) probes. Generally 60 seconds (± 2
            // seconds jitter) is used. For peer BRs a longer timeout
            // of 200 seconds (3 min and 20 seconds) is used. This is
            // selected to be longer than regular RA beacon interval
            // of 3 minutes (± 15 seconds jitter).

            static constexpr uint32_t kReachableInterval = OPENTHREAD_CONFIG_BORDER_ROUTING_ROUTER_ACTIVE_CHECK_TIMEOUT;
            static constexpr uint32_t kPeerBrReachableInterval = Time::kOneSecondInMsec * 200;

            static constexpr uint8_t  kMaxNsProbes          = 5;    // Max number of NS probe attempts.
            static constexpr uint32_t kNsProbeRetryInterval = 1000; // In msec. Time between NS probe attempts.
            static constexpr uint32_t kNsProbeTimeout       = 2000; // In msec. Max Wait time after last NS probe.
            static constexpr uint32_t kJitter               = 2000; // In msec. Jitter to randomize probe starts.

            static_assert(kMaxNsProbes < 255, "kMaxNsProbes MUST not be 255");

            typedef LifetimedPrefix::ExpirationChecker EmptyChecker;

            bool IsReachable(void) const { return mNsProbeCount <= kMaxNsProbes; }
            bool ShouldCheckReachability(void) const;
            void ResetReachabilityState(void);
            void DetermineReachabilityTimeout(void);
            bool Matches(const Ip6::Address &aAddress) const { return aAddress == mAddress; }
            bool Matches(const EmptyChecker &aChecker);
            bool IsPeerBr(void) const;
            void CopyInfoTo(RouterEntry &aEntry, TimeMilli aNow, uint32_t aUptime) const;

            using OnLinkPrefixList = OwningList<Entry<OnLinkPrefix>>;
            using RoutePrefixList  = OwningList<Entry<RoutePrefix>>;

            // `mDiscoverTime` tracks the initial discovery time of
            // this router. To accommodate longer durations, the
            // `Uptime` is used, as `TimeMilli` (which uses `uint32_t`
            // intervals) would be limited to tracking ~ 49 days.
            //
            // `mLastUpdateTime` tracks the most recent time an RA or
            // NA was received from this router. It is bounded due to
            // the frequency of reachability checks, so we can safely
            // use `TimeMilli` for it.

            Ip6::Address     mAddress;
            OnLinkPrefixList mOnLinkPrefixes;
            RoutePrefixList  mRoutePrefixes;
            uint32_t         mDiscoverTime;
            TimeMilli        mLastUpdateTime;
            TimeMilli        mTimeoutTime;
            uint8_t          mNsProbeCount;
            bool             mManagedAddressConfigFlag : 1;
            bool             mOtherConfigFlag : 1;
            bool             mSnacRouterFlag : 1;
            bool             mIsLocalDevice : 1;
            bool             mAllEntriesDisregarded : 1;
        };

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

        class Iterator : public PrefixTableIterator
        {
        public:
            enum Type : uint8_t
            {
                kUnspecified,
                kRouterIterator,
                kPrefixIterator,
                kPeerBrIterator,
            };

            enum EntryType : uint8_t
            {
                kOnLinkPrefix,
                kRoutePrefix,
            };

            void                 Init(const Entry<Router> *aRoutersHead, uint32_t aUptime);
            Error                AdvanceToNextRouter(Type aType);
            Error                AdvanceToNextEntry(void);
            uint32_t             GetInitUptime(void) const { return mData0; }
            TimeMilli            GetInitTime(void) const { return TimeMilli(mData1); }
            Type                 GetType(void) const { return static_cast<Type>(mData2); }
            const Entry<Router> *GetRouter(void) const { return static_cast<const Entry<Router> *>(mPtr1); }
            EntryType            GetEntryType(void) const { return static_cast<EntryType>(mData3); }

            template <class PrefixType> const Entry<PrefixType> *GetEntry(void) const
            {
                return static_cast<const Entry<PrefixType> *>(mPtr2);
            }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
            using PeerBr = NetDataPeerBrTracker::PeerBr;

            Error         AdvanceToNextPeerBr(const PeerBr *aPeerBrsHead);
            const PeerBr *GetPeerBrEntry(void) const { return static_cast<const PeerBr *>(mPtr2); }
#endif

        private:
            void SetRouter(const Entry<Router> *aRouter) { mPtr1 = aRouter; }
            void SetInitUptime(uint32_t aUptime) { mData0 = aUptime; }
            void SetInitTime(void) { mData1 = TimerMilli::GetNow().GetValue(); }
            void SetEntry(const void *aEntry) { mPtr2 = aEntry; }
            bool HasEntry(void) const { return mPtr2 != nullptr; }
            void SetEntryType(EntryType aType) { mData3 = aType; }
            void SetType(Type aType) { mData2 = aType; }
        };

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        static constexpr uint16_t kMaxRouters = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS;
        static constexpr uint16_t kMaxEntries = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES;

        union SharedEntry
        {
            SharedEntry(void) { mNext = nullptr; }
            void               SetNext(SharedEntry *aNext) { mNext = aNext; }
            SharedEntry       *GetNext(void) { return mNext; }
            const SharedEntry *GetNext(void) const { return mNext; }

            template <class PrefixType> Entry<PrefixType> &GetEntry(void);

            SharedEntry        *mNext;
            Entry<OnLinkPrefix> mOnLinkEntry;
            Entry<RoutePrefix>  mRouteEntry;
        };
#endif

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

        struct DecisionFactors : public Clearable<DecisionFactors>, public Equatable<DecisionFactors>
        {
            DecisionFactors(void) { Clear(); }

            bool HasFavoredOnLink(void) const { return (mFavoredOnLinkPrefix.GetLength() != 0); }
            void UpdateFlagsFrom(const Router &aRouter);
            void UpdateFrom(const OnLinkPrefix &aOnLinkPrefix);
            void UpdateFrom(const RoutePrefix &aRoutePrefix);

            Ip6::Prefix mFavoredOnLinkPrefix;
            bool        mHasNonUlaRoute : 1;
            bool        mHasNonUlaOnLink : 1;
            bool        mHasUlaOnLink : 1;
            bool        mHeaderManagedAddressConfigFlag : 1;
            bool        mHeaderOtherConfigFlag : 1;
        };

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

        void ProcessRaHeader(const RouterAdvert::Header &aRaHeader, Router &aRouter, RouterAdvOrigin aRaOrigin);
        void ProcessPrefixInfoOption(const PrefixInfoOption &aPio, Router &aRouter);
        void ProcessRouteInfoOption(const RouteInfoOption &aRio, Router &aRouter);
        void Evaluate(void);
        void DetermineStaleTimeFor(const OnLinkPrefix &aPrefix, NextFireTime &aStaleTime);
        void DetermineStaleTimeFor(const RoutePrefix &aPrefix, NextFireTime &aStaleTime);
        void SendNeighborSolicitToRouter(const Router &aRouter);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        template <class Type> Entry<Type> *AllocateEntry(void) { return Entry<Type>::Allocate(); }
#else
        template <class Type> Entry<Type> *AllocateEntry(void);
#endif

        using SignalTask      = TaskletIn<RoutingManager, &RoutingManager::HandleRxRaTrackerSignalTask>;
        using ExpirationTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleRxRaTrackerExpirationTimer>;
        using StaleTimer      = TimerMilliIn<RoutingManager, &RoutingManager::HandleRxRaTrackerStaleTimer>;
        using RouterTimer     = TimerMilliIn<RoutingManager, &RoutingManager::HandleRxRaTrackerRouterTimer>;
        using RouterList      = OwningList<Entry<Router>>;

        DecisionFactors      mDecisionFactors;
        RouterList           mRouters;
        ExpirationTimer      mExpirationTimer;
        StaleTimer           mStaleTimer;
        RouterTimer          mRouterTimer;
        SignalTask           mSignalTask;
        RouterAdvert::Header mLocalRaHeader;
        TimeMilli            mLocalRaHeaderUpdateTime;

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        Pool<SharedEntry, kMaxEntries>   mEntryPool;
        Pool<Entry<Router>, kMaxRouters> mRouterPool;
#endif
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class OmrPrefixManager;

    class OmrPrefix : public Clearable<OmrPrefix>
    {
        friend class OmrPrefixManager;

    public:
        OmrPrefix(void) { Clear(); }

        bool               IsEmpty(void) const { return (mPrefix.GetLength() == 0); }
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        RoutePreference    GetPreference(void) const { return mPreference; }
        bool               IsDomainPrefix(void) const { return mIsDomainPrefix; }

    protected:
        Ip6::Prefix     mPrefix;
        RoutePreference mPreference;
        bool            mIsDomainPrefix;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class FavoredOmrPrefix : public OmrPrefix
    {
        friend class OmrPrefixManager;

    public:
        bool IsInfrastructureDerived(void) const;

    private:
        void SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);
        void SetFrom(const OmrPrefix &aOmrPrefix);
        bool IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class OmrPrefixManager : public InstanceLocator
    {
    public:
        explicit OmrPrefixManager(Instance &aInstance);

        void                    Init(const Ip6::Prefix &aBrUlaPrefix);
        void                    Start(void);
        void                    Stop(void);
        void                    Evaluate(void);
        void                    UpdateDefaultRouteFlag(bool aDefaultRoute);
        bool                    ShouldAdvertiseLocalAsRio(void) const;
        const Ip6::Prefix      &GetGeneratedPrefix(void) const { return mGeneratedPrefix; }
        const OmrPrefix        &GetLocalPrefix(void) const { return mLocalPrefix; }
        const FavoredOmrPrefix &GetFavoredPrefix(void) const { return mFavoredPrefix; }

    private:
        static constexpr uint16_t kInfoStringSize = 85;

        typedef String<kInfoStringSize> InfoString;

        void       DetermineFavoredPrefix(void);
        Error      AddLocalToNetData(void);
        Error      AddOrUpdateLocalInNetData(void);
        void       RemoveLocalFromNetData(void);
        InfoString LocalToString(void) const;

        OmrPrefix        mLocalPrefix;
        Ip6::Prefix      mGeneratedPrefix;
        FavoredOmrPrefix mFavoredPrefix;
        bool             mIsLocalAddedInNetData;
        bool             mDefaultRoute;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleOnLinkPrefixManagerTimer(void) { mOnLinkPrefixManager.HandleTimer(); }

    class OnLinkPrefixManager : public InstanceLocator
    {
    public:
        explicit OnLinkPrefixManager(Instance &aInstance);

        // Max number of old on-link prefixes to retain to deprecate.
        static constexpr uint16_t kMaxOldPrefixes = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_OLD_ON_LINK_PREFIXES;

        void               Init(void);
        void               Start(void);
        void               Stop(void);
        void               Evaluate(void);
        const Ip6::Prefix &GetLocalPrefix(void) const { return mLocalPrefix; }
        const Ip6::Prefix &GetFavoredDiscoveredPrefix(void) const { return mFavoredDiscoveredPrefix; }
        bool               AddressMatchesLocalPrefix(const Ip6::Address &aAddress) const;
        bool               IsInitalEvaluationDone(void) const;
        void               HandleRaPrefixTableChanged(void);
        bool               ShouldPublishUlaRoute(void) const;
        Error              AppendAsPiosTo(RouterAdvert::TxMessage &aRaMessage);
        void               HandleNetDataChange(void);
        void               HandleExtPanIdChange(void);
        void               HandleTimer(void);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_TESTING_API_ENABLE
        void SetLocalPrefix(const Ip6::Prefix &aPrefix) { mLocalPrefix = aPrefix; }
#endif

    private:
        enum State : uint8_t // State of `mLocalPrefix`
        {
            kIdle,
            kPublishing,
            kAdvertising,
            kDeprecating,
        };

        struct OldPrefix
        {
            bool Matches(const Ip6::Prefix &aPrefix) const { return mPrefix == aPrefix; }

            Ip6::Prefix mPrefix;
            TimeMilli   mExpireTime;
        };

        State GetState(void) const { return mState; }
        void  SetState(State aState);
        bool  IsPublishingOrAdvertising(void) const;
        void  GenerateLocalPrefix(void);
        void  PublishAndAdvertise(void);
        void  Deprecate(void);
        void  ResetExpireTime(TimeMilli aNow);
        Error AppendCurPrefix(RouterAdvert::TxMessage &aRaMessage);
        Error AppendOldPrefixes(RouterAdvert::TxMessage &aRaMessage);
        void  DeprecateOldPrefix(const Ip6::Prefix &aPrefix, TimeMilli aExpireTime);
        void  SavePrefix(const Ip6::Prefix &aPrefix, TimeMilli aExpireTime);

        static const char *StateToString(State aState);

        using ExpireTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleOnLinkPrefixManagerTimer>;

        Ip6::Prefix                       mLocalPrefix;
        State                             mState;
        TimeMilli                         mExpireTime;
        Ip6::Prefix                       mFavoredDiscoveredPrefix;
        Array<OldPrefix, kMaxOldPrefixes> mOldLocalPrefixes;
        ExpireTimer                       mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRioAdvertiserimer(void) { mRioAdvertiser.HandleTimer(); }

    class RioAdvertiser : public InstanceLocator
    {
        // Manages the list of prefixes advertised as RIO in emitted
        // RA. The RIO prefixes are discovered from on-mesh prefixes in
        // network data including OMR prefix from `OmrPrefixManager`.
        // It also handles deprecating removed prefixes.

    public:
        explicit RioAdvertiser(Instance &aInstance);

        RoutePreference GetPreference(void) const { return mPreference; }
        void            SetPreference(RoutePreference aPreference);
        void            ClearPreference(void);
        void            HandleRoleChanged(void);
        Error           AppendRios(RouterAdvert::TxMessage &aRaMessage);
        Error           InvalidatPrevRios(RouterAdvert::TxMessage &aRaMessage);
        bool            HasAdvertised(const Ip6::Prefix &aPrefix) const { return mPrefixes.ContainsMatching(aPrefix); }
        uint16_t        GetAdvertisedRioCount(void) const { return mPrefixes.GetLength(); }
        void            HandleTimer(void);

    private:
        static constexpr uint32_t kDeprecationTime = TimeMilli::SecToMsec(300);

        struct RioPrefix : public Clearable<RioPrefix>
        {
            bool Matches(const Ip6::Prefix &aPrefix) const { return (mPrefix == aPrefix); }

            Ip6::Prefix mPrefix;
            bool        mIsDeprecating;
            TimeMilli   mExpirationTime;
        };

        struct RioPrefixArray :
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
            public Heap::Array<RioPrefix>
#else
            public Array<RioPrefix, 2 * kMaxOnMeshPrefixes>
#endif
        {
            void Add(const Ip6::Prefix &aPrefix);
        };

        void  SetPreferenceBasedOnRole(void);
        void  UpdatePreference(RoutePreference aPreference);
        Error AppendRio(const Ip6::Prefix       &aPrefix,
                        uint32_t                 aRouteLifetime,
                        RoutePreference          aPreference,
                        RouterAdvert::TxMessage &aRaMessage);

        using RioTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleRioAdvertiserimer>;

        RioPrefixArray  mPrefixes;
        RioTimer        mTimer;
        RoutePreference mPreference;
        bool            mUserSetPreference;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

    void HandleNat64PrefixManagerTimer(void) { mNat64PrefixManager.HandleTimer(); }

    class Nat64PrefixManager : public InstanceLocator
    {
    public:
        // This class manages the NAT64 related functions including
        // generation of local NAT64 prefix, discovery of infra
        // interface prefix, maintaining the discovered prefix
        // lifetime, and selection of the NAT64 prefix to publish in
        // Network Data.
        //
        // Calling methods except GenerateLocalPrefix and SetEnabled
        // when disabled becomes no-op.

        explicit Nat64PrefixManager(Instance &aInstance);

        void         SetEnabled(bool aEnabled);
        Nat64::State GetState(void) const;

        void Start(void);
        void Stop(void);

        void               GenerateLocalPrefix(const Ip6::Prefix &aBrUlaPrefix);
        const Ip6::Prefix &GetLocalPrefix(void) const { return mLocalPrefix; }
        const Ip6::Prefix &GetFavoredPrefix(RoutePreference &aPreference) const;
        void               Evaluate(void);
        void               HandleDiscoverDone(const Ip6::Prefix &aPrefix);
        void               HandleTimer(void);

    private:
        void Discover(void);
        void Publish(void);

        using Nat64Timer = TimerMilliIn<RoutingManager, &RoutingManager::HandleNat64PrefixManagerTimer>;

        bool mEnabled;

        Ip6::Prefix     mInfraIfPrefix;       // The latest NAT64 prefix discovered on the infrastructure interface.
        Ip6::Prefix     mLocalPrefix;         // The local prefix (from BR ULA prefix).
        Ip6::Prefix     mPublishedPrefix;     // The prefix to publish in Net Data (empty or local or from infra-if).
        RoutePreference mPublishedPreference; // The published prefix preference.
        Nat64Timer      mTimer;
    };

#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRoutePublisherTimer(void) { mRoutePublisher.HandleTimer(); }

    class RoutePublisher : public InstanceLocator // Manages the routes that are published in net data
    {
    public:
        explicit RoutePublisher(Instance &aInstance);

        void Start(void) { Evaluate(); }
        void Stop(void) { Unpublish(); }
        void Evaluate(void);

        void UpdateAdvPioFlags(bool aAdvPioFlag);

        RoutePreference GetPreference(void) const { return mPreference; }
        void            SetPreference(RoutePreference aPreference);
        void            ClearPreference(void);

        void HandleNotifierEvents(Events aEvents);
        void HandleTimer(void);

        static const Ip6::Prefix &GetUlaPrefix(void) { return AsCoreType(&kUlaPrefix); }

    private:
        static constexpr uint32_t kDelayBeforePrfUpdateOnLinkQuality3 = TimeMilli::SecToMsec(5 * 60);

        static const otIp6Prefix kUlaPrefix;

        enum State : uint8_t
        {
            kDoNotPublish,   // Do not publish any routes in network data.
            kPublishDefault, // Publish "::/0" route in network data.
            kPublishUla,     // Publish "fc00::/7" route in network data.
        };

        void DeterminePrefixFor(State aState, Ip6::Prefix &aPrefix) const;
        void UpdatePublishedRoute(State aNewState);
        void Unpublish(void);
        void SetPreferenceBasedOnRole(void);
        void UpdatePreference(RoutePreference aPreference);

        static const char *StateToString(State aState);

        using DelayTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleRoutePublisherTimer>;

        State           mState;
        RoutePreference mPreference;
        bool            mUserSetPreference;
        bool            mAdvPioFlag;
        DelayTimer      mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct TxRaInfo
    {
        // Tracks info about emitted RA messages:
        //
        // - Number of RAs sent
        // - Last RA TX time
        // - Hashes of last TX RAs (to tell if a received RA is from
        //   `RoutingManager` itself).

        typedef Crypto::Sha256::Hash Hash;

        static constexpr uint16_t kNumHashEntries = 5;

        TxRaInfo(void)
            : mTxCount(0)
            , mLastTxTime(TimerMilli::GetNow() - kMinDelayBetweenRas)
            , mLastHashIndex(0)
        {
        }

        void        IncrementTxCountAndSaveHash(const InfraIf::Icmp6Packet &aRaMessage);
        bool        IsRaFromManager(const RouterAdvert::RxMessage &aRaMessage) const;
        static void CalculateHash(const RouterAdvert::RxMessage &aRaMessage, Hash &aHash);

        uint32_t  mTxCount;
        TimeMilli mLastTxTime;
        Hash      mHashes[kNumHashEntries];
        uint16_t  mLastHashIndex;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRsSenderTimer(void) { mRsSender.HandleTimer(); }

    class RsSender : public InstanceLocator
    {
    public:
        // This class implements tx of Router Solicitation (RS)
        // messages to discover other routers. `Start()` schedules
        // a cycle of RS transmissions of `kMaxTxCount` separated
        // by `kTxInterval`. At the end of cycle the callback
        // `HandleRsSenderFinished()` is invoked to inform end of
        // the cycle to `RoutingManager`.

        explicit RsSender(Instance &aInstance);

        bool IsInProgress(void) const { return mTimer.IsRunning(); }
        void Start(void);
        void Stop(void);
        void HandleTimer(void);

    private:
        // All time intervals are in msec.
        static constexpr uint32_t kMaxStartDelay     = 1000;        // Max random delay to send the first RS.
        static constexpr uint32_t kTxInterval        = 4000;        // Interval between RS tx.
        static constexpr uint32_t kRetryDelay        = kTxInterval; // Interval to wait to retry a failed RS tx.
        static constexpr uint32_t kWaitOnLastAttempt = 1000;        // Wait interval after last RS tx.
        static constexpr uint8_t  kMaxTxCount        = 3;           // Number of RS tx in one cycle.

        Error SendRs(void);

        using RsTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleRsSenderTimer>;

        uint8_t   mTxCount;
        RsTimer   mTimer;
        TimeMilli mStartTime;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

    void HandlePdPrefixManagerTimer(void) { mPdPrefixManager.HandleTimer(); }

    class PdPrefixManager : public InstanceLocator
    {
    public:
        // This class implements handling (including management of the
        // lifetime) of the prefix obtained from platform's DHCPv6 PD
        // client.

        typedef Dhcp6PdState State;

        static constexpr RoutePreference kPdRoutePreference = RoutePreference::kRoutePreferenceMedium;

        explicit PdPrefixManager(Instance &aInstance);

        void               SetEnabled(bool aEnabled);
        void               Start(void) { StartStop(/* aStart= */ true); }
        void               Stop(void) { StartStop(/* aStart= */ false); }
        bool               IsRunning(void) const { return GetState() == kDhcp6PdStateRunning; }
        bool               HasPrefix(void) const { return IsValidOmrPrefix(mPrefix.GetPrefix()); }
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix.GetPrefix(); }
        State              GetState(void) const;

        void  ProcessRa(const uint8_t *aRouterAdvert, uint16_t aLength);
        void  ProcessPrefix(const PrefixTableEntry &aPrefixTableEntry);
        Error GetPrefixInfo(PrefixTableEntry &aInfo) const;
        Error GetProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo) const;
        void  HandleTimer(void) { WithdrawPrefix(); }
        void  SetStateCallback(PdCallback aCallback, void *aContext) { mStateCallback.Set(aCallback, aContext); }
        void  Evaluate(void);

    private:
        class PrefixEntry : public OnLinkPrefix
        {
        public:
            PrefixEntry(void) { Clear(); }
            bool IsEmpty(void) const { return (GetPrefix().GetLength() == 0); }
            bool IsValidPdPrefix(void) const;
            bool IsFavoredOver(const PrefixEntry &aOther) const;
        };

        void Process(const InfraIf::Icmp6Packet *aRaPacket, const PrefixTableEntry *aPrefixTableEntry);
        bool ProcessPrefixEntry(PrefixEntry &aEntry, PrefixEntry &aFavoredEntry);
        void EvaluateStateChange(State aOldState);
        void WithdrawPrefix(void);
        void StartStop(bool aStart);
        void PauseResume(bool aPause);

        static const char *StateToString(State aState);

        using PrefixTimer   = TimerMilliIn<RoutingManager, &RoutingManager::HandlePdPrefixManagerTimer>;
        using StateCallback = Callback<PdCallback>;

        bool mEnabled;   // Whether PdPrefixManager is enabled. (guards the overall PdPrefixManager functions)
        bool mIsStarted; // Whether PdPrefixManager is started. (monitoring prefixes)
        bool mIsPaused;  // Whether PdPrefixManager is paused. (when there is another BR advertising another prefix)

        uint32_t      mNumPlatformPioProcessed;
        uint32_t      mNumPlatformRaReceived;
        TimeMilli     mLastPlatformRaTime;
        StateCallback mStateCallback;
        PrefixTimer   mTimer;
        PrefixEntry   mPrefix;
    };

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

    //------------------------------------------------------------------------------------------------------------------
    // Methods

    void  EvaluateState(void);
    void  Start(void);
    void  Stop(void);
    void  HandleNotifierEvents(Events aEvents);
    bool  IsInitialized(void) const { return mInfraIf.IsInitialized(); }
    bool  IsEnabled(void) const { return mIsEnabled; }
    Error LoadOrGenerateRandomBrUlaPrefix(void);

    void EvaluateRoutingPolicy(void);
    bool IsInitialPolicyEvaluationDone(void) const;
    void ScheduleRoutingPolicyEvaluation(ScheduleMode aMode);
    void HandleRsSenderFinished(TimeMilli aStartTime);
    void SendRouterAdvertisement(RouterAdvTxMode aRaTxMode);

    void HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleNeighborAdvertisement(const InfraIf::Icmp6Packet &aPacket);
    bool NetworkDataContainsUlaRoute(void) const;

    void HandleRaPrefixTableChanged(void);
    void HandleLocalOnLinkPrefixChanged(void);

    static TimeMilli CalculateExpirationTime(TimeMilli aUpdateTime, uint32_t aLifetime);

    static bool IsValidBrUlaPrefix(const Ip6::Prefix &aBrUlaPrefix);
    static bool IsValidOnLinkPrefix(const PrefixInfoOption &aPio);
    static bool IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    static void LogRaHeader(const RouterAdvert::Header &aRaHeader);
    static void LogPrefixInfoOption(const Ip6::Prefix &aPrefix, uint32_t aValidLifetime, uint32_t aPreferredLifetime);
    static void LogRouteInfoOption(const Ip6::Prefix &aPrefix, uint32_t aLifetime, RoutePreference aPreference);

    static const char *RouterAdvOriginToString(RouterAdvOrigin aRaOrigin);

    //------------------------------------------------------------------------------------------------------------------
    // Variables

    using RoutingPolicyTimer = TimerMilliIn<RoutingManager, &RoutingManager::EvaluateRoutingPolicy>;

    // Indicates whether the Routing Manager is running (started).
    bool mIsRunning;

    // Indicates whether the Routing manager is enabled. The Routing
    // Manager will be stopped if we are disabled.
    bool mIsEnabled;

    InfraIf mInfraIf;

    // The /48 BR ULA prefix loaded from local persistent storage or
    // randomly generated if none is found in persistent storage.
    Ip6::Prefix mBrUlaPrefix;

    OmrPrefixManager mOmrPrefixManager;

    RioAdvertiser   mRioAdvertiser;
    RoutePreference mRioPreference;
    bool            mUserSetRioPreference;

    OnLinkPrefixManager mOnLinkPrefixManager;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    NetDataPeerBrTracker mNetDataPeerBrTracker;
#endif

    RxRaTracker mRxRaTracker;

    RoutePublisher mRoutePublisher;

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    Nat64PrefixManager mNat64PrefixManager;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    PdPrefixManager mPdPrefixManager;
#endif

    TxRaInfo   mTxRaInfo;
    RsSender   mRsSender;
    Heap::Data mExtraRaOptions;

    RoutingPolicyTimer mRoutingPolicyTimer;
};

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// Template specializations and declarations

template <>
inline RoutingManager::RxRaTracker::Entry<RoutingManager::OnLinkPrefix>
    &RoutingManager::RxRaTracker::SharedEntry::GetEntry(void)
{
    return mOnLinkEntry;
}

template <>
inline RoutingManager::RxRaTracker::Entry<RoutingManager::RoutePrefix>
    &RoutingManager::RxRaTracker::SharedEntry::GetEntry(void)
{
    return mRouteEntry;
}

// Declare template (full) specializations for `Router` type.

template <>
RoutingManager::RxRaTracker::Entry<RoutingManager::RxRaTracker::Router> *RoutingManager::RxRaTracker::AllocateEntry(
    void);

template <> void RoutingManager::RxRaTracker::Entry<RoutingManager::RxRaTracker::Router>::Free(void);

#endif // #if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

} // namespace BorderRouter

DefineMapEnum(otBorderRoutingState, BorderRouter::RoutingManager::State);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTING_MANAGER_HPP_
