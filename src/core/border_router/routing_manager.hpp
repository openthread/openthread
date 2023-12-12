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
 *
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

#include <openthread/nat64.h>
#include <openthread/netdata.h>

#include "border_router/infra_if.hpp"
#include "common/array.hpp"
#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_array.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/notifier.hpp"
#include "common/pool.hpp"
#include "common/string.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/nat64_translator.hpp"
#include "net/nd6.hpp"
#include "thread/network_data.hpp"

namespace ot {

namespace BorderRouter {

/**
 * Implements bi-directional routing between Thread and Infrastructure networks.
 *
 * The Border Routing manager works on both Thread interface and infrastructure interface.
 * All ICMPv6 messages are sent/received on the infrastructure interface.
 *
 */
class RoutingManager : public InstanceLocator
{
    friend class ot::Notifier;
    friend class ot::Instance;

public:
    typedef NetworkData::RoutePreference       RoutePreference;     ///< Route preference (high, medium, low).
    typedef otBorderRoutingPrefixTableIterator PrefixTableIterator; ///< Prefix Table Iterator.
    typedef otBorderRoutingPrefixTableEntry    PrefixTableEntry;    ///< Prefix Table Entry.
    typedef otBorderRoutingRouterEntry         RouterEntry;         ///< Router Entry.
    typedef otPdProcessedRaInfo                PdProcessedRaInfo;   ///< Data of PdProcessedRaInfo.

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
     *
     */
    static constexpr uint16_t kMaxPublishedPrefixes = 3;

    /**
     * Represents the states of `RoutingManager`.
     *
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
     *
     */
    enum Dhcp6PdState : uint8_t
    {
        kDhcp6PdStateDisabled = OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED, ///< Disabled.
        kDhcp6PdStateStopped  = OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED,  ///< Enabled, but currently stopped.
        kDhcp6PdStateRunning  = OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING,  ///< Enabled, and running.
    };

    /**
     * Initializes the routing manager.
     *
     * @param[in]  aInstance  A OpenThread instance.
     *
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
     *
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
     *
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
     *
     */
    bool IsRunning(void) const { return mIsRunning; }

    /**
     * Gets the state of `RoutingManager`.
     *
     * @returns The current state of `RoutingManager`.
     *
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
     *
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
     *
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
     *
     */
    void SetRouteInfoOptionPreference(RoutePreference aPreference) { mRioAdvertiser.SetPreference(aPreference); }

    /**
     * Clears a previously set preference value for advertised Route Info Options.
     *
     * After a call to this method, BR will use device role to determine the RIO preference: Medium preference when
     * in router/leader role and low preference when in child role.
     *
     */
    void ClearRouteInfoOptionPreference(void) { mRioAdvertiser.ClearPreference(); }

    /**
     * Gets the current preference used for published routes in Network Data.
     *
     * The preference is determined as follows:
     *
     * - If explicitly set by user by calling `SetRoutePreference()`, the given preference is used.
     * - Otherwise, it is determined automatically by `RoutingManager` based on the device's role and link quality.
     *
     * @returns The current published route preference.
     *
     */
    RoutePreference GetRoutePreference(void) const { return mRoutePublisher.GetPreference(); }

    /**
     * Explicitly sets the preference of published routes in Network Data.
     *
     * After a call to this method, BR will use the given preference. The preference can be cleared by calling
     * `ClearRoutePreference`()`.
     *
     * @param[in] aPreference   The route preference to use.
     *
     */
    void SetRoutePreference(RoutePreference aPreference) { mRoutePublisher.SetPreference(aPreference); }

    /**
     * Clears a previously set preference value for published routes in Network Data.
     *
     * After a call to this method, BR will determine the preference automatically based on the device's role and
     * link quality (to the parent when acting as end-device).
     *
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
     *
     */
    Error GetOmrPrefix(Ip6::Prefix &aPrefix) const;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    /**
     * Returns the platform provided off-mesh-routable (OMR) prefix.
     *
     * The prefix is extracted from the platform generated RA messages handled by `ProcessPlatformGeneratedNd()`.
     *
     * @param[out] aPrefixInfo      A reference to where the prefix info will be output to.
     *
     * @retval kErrorNone           Successfully retrieved the OMR prefix.
     * @retval kErrorNotFound       There are no valid PD prefix on this BR.
     * @retval kErrorInvalidState   The Border Routing Manager is not initialized yet.
     *
     */
    Error GetPdOmrPrefix(PrefixTableEntry &aPrefixInfo) const;

    /**
     * Returns platform generated RA message processed information.
     *
     * @param[out] aPdProcessedRaInfo      A reference to where the PD processed RA info will be output to.
     *
     * @retval kErrorNone           Successfully retrieved the Info.
     * @retval kErrorNotFound       There are no valid RA process info on this BR.
     * @retval kErrorInvalidState   The Border Routing Manager is not initialized yet.
     *
     */
    Error GetPdProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo);
#endif

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
     *
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
     *
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
     *
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
     *
     */
    Nat64::State GetNat64PrefixManagerState(void) const { return mNat64PrefixManager.GetState(); }

    /**
     * Enable or disable NAT64 prefix publishing.
     *
     * @param[in]  aEnabled   A boolean to enable/disable NAT64 prefix publishing.
     *
     */
    void SetNat64PrefixManagerEnabled(bool aEnabled);

    /**
     * Returns the local NAT64 prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     *
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
     *
     */
    Error GetFavoredNat64Prefix(Ip6::Prefix &aPrefix, RoutePreference &aRoutePreference);

    /**
     * Informs `RoutingManager` of the result of the discovery request of NAT64 prefix on infrastructure
     * interface (`InfraIf::DiscoverNat64Prefix()`).
     *
     * @param[in]  aPrefix  The discovered NAT64 prefix on `InfraIf`.
     *
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
     *
     */
    void HandleReceived(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);

    /**
     * Handles infrastructure interface state changes.
     *
     */
    void HandleInfraIfStateChanged(void) { EvaluateState(); }

    /**
     * Checks whether the on-mesh prefix configuration is a valid OMR prefix.
     *
     * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALSE   The prefix is not a valid OMR prefix.
     *
     */
    static bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

    /**
     * Checks whether a given prefix is a valid OMR prefix.
     *
     * @param[in]  aPrefix  The prefix to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALSE   The prefix is not a valid OMR prefix.
     *
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
     *
     */
    void InitPrefixTableIterator(PrefixTableIterator &aIterator) const
    {
        mDiscoveredPrefixTable.InitIterator(aIterator);
    }

    /**
     * Iterates over entries in the discovered prefix table.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next entry, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more entries in the table.
     *
     */
    Error GetNextPrefixTableEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const
    {
        return mDiscoveredPrefixTable.GetNextEntry(aIterator, aEntry);
    }

    /**
     * Iterates over discovered router entries on infrastructure link.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next router info, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more routers.
     *
     */
    Error GetNextRouterEntry(PrefixTableIterator &aIterator, RouterEntry &aEntry) const
    {
        return mDiscoveredPrefixTable.GetNextRouter(aIterator, aEntry);
    }

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    /**
     * Determines whether to enable/disable SRP server when the auto-enable mode is changed on SRP server.
     *
     * This should be called from `Srp::Server` when auto-enable mode is changed.
     *
     */
    void HandleSrpServerAutoEnableMode(void);
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    /**
     * Handles a router advertisement message received on platform's Thread interface.
     *
     * Note: This method is a part of DHCPv6 PD support on Thread border routers. The message should be generated by the
     * software like dnamasq, radvd, systemd-networkd on the platform as a part of the DHCPv6 prefix delegation process
     * for distributing the prefix to the interfaces (links).
     *
     * @param[in] aRouterAdvert A pointer to the buffer of the router advertisement message.
     * @param[in] aLength       The length of the router advertisement message.
     *
     */
    void ProcessPlatformGeneratedRa(const uint8_t *aRouterAdvert, uint16_t aLength)
    {
        mPdPrefixManager.ProcessPlatformGeneratedRa(aRouterAdvert, aLength);
    }

    /**
     * Enables / Disables the functions for DHCPv6 PD.
     *
     * @param[in] aEnabled  Whether to accept platform generated RA messages.
     *
     */
    void SetDhcp6PdEnabled(bool aEnabled) { return mPdPrefixManager.SetEnabled(aEnabled); }

    /**
     * Returns the state of accpeting RouterAdvertisement messages on platform interface.
     *
     * @retval kDhcp6PdStateRunning  DHCPv6 PD should be enabled and running on this border router.
     * @retval kDhcp6PdStateDisabled DHCPv6 PD should be disabled on this border router..
     *
     */
    Dhcp6PdState GetDhcp6PdState(void) const { return mPdPrefixManager.GetState(); }
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

private:
    static constexpr uint8_t kMaxOnMeshPrefixes = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES;

    static constexpr uint8_t kOmrPrefixLength    = OT_IP6_PREFIX_BITSIZE; // The length of an OMR prefix. In bits.
    static constexpr uint8_t kOnLinkPrefixLength = OT_IP6_PREFIX_BITSIZE; // The length of an On-link prefix. In bits.
    static constexpr uint8_t kBrUlaPrefixLength  = 48;                    // The length of a BR ULA prefix. In bits.
    static constexpr uint8_t kNat64PrefixLength  = 96;                    // The length of a NAT64 prefix. In bits.

    static constexpr uint16_t kOmrPrefixSubnetId   = 1; // The subnet ID of an OMR prefix within a BR ULA prefix.
    static constexpr uint16_t kNat64PrefixSubnetId = 2; // The subnet ID of a NAT64 prefix within a BR ULA prefix.

    // The maximum number of initial Router Advertisements.
    static constexpr uint32_t kMaxInitRtrAdvertisements = 3;

    static constexpr uint32_t kDefaultOmrPrefixLifetime    = 1800; // The default OMR prefix valid lifetime. In sec.
    static constexpr uint32_t kDefaultOnLinkPrefixLifetime = 1800; // The default on-link prefix valid lifetime. In sec.
    static constexpr uint32_t kDefaultNat64PrefixLifetime  = 300;  // The default NAT64 prefix valid lifetime. In sec.
    static constexpr uint32_t kMaxRtrAdvInterval           = 600;  // Max Router Advertisement Interval. In sec.
    static constexpr uint32_t kMinRtrAdvInterval           = kMaxRtrAdvInterval / 3; // Min RA Interval. In sec.
    static constexpr uint32_t kMaxInitRtrAdvInterval       = 16;                     // Max Initial RA Interval. In sec.
    static constexpr uint32_t kRaReplyJitter               = 500;  // Jitter for sending RA after rx RS. In msec.
    static constexpr uint32_t kPolicyEvaluationMinDelay    = 2000; // Min delay for policy evaluation. In msec.
    static constexpr uint32_t kPolicyEvaluationMaxDelay    = 4000; // Max delay for policy evaluation. In msec.
    static constexpr uint32_t kMinDelayBetweenRtrAdvs      = 3000; // Min delay (msec) between consecutive RAs.

    // The STALE_RA_TIME in seconds. The Routing Manager will consider the prefixes
    // and learned RA parameters STALE when they are not refreshed in STALE_RA_TIME
    // seconds. The Routing Manager will then start Router Solicitation to verify
    // that the STALE prefix is not being advertised anymore and remove the STALE
    // prefix.
    // The value is chosen in range of [`kMaxRtrAdvInterval` upper bound (1800s), `kDefaultOnLinkPrefixLifetime`].
    static constexpr uint32_t kRtrAdvStaleTime = 1800;

    static_assert(kMinRtrAdvInterval <= 3 * kMaxRtrAdvInterval / 4, "invalid RA intervals");
    static_assert(kDefaultOmrPrefixLifetime >= kMaxRtrAdvInterval, "invalid default OMR prefix lifetime");
    static_assert(kDefaultOnLinkPrefixLifetime >= kMaxRtrAdvInterval, "invalid default on-link prefix lifetime");
    static_assert(kRtrAdvStaleTime >= 1800 && kRtrAdvStaleTime <= kDefaultOnLinkPrefixLifetime,
                  "invalid RA STALE time");
    static_assert(kPolicyEvaluationMaxDelay > kPolicyEvaluationMinDelay,
                  "kPolicyEvaluationMaxDelay must be larger than kPolicyEvaluationMinDelay");

    enum RouterAdvTxMode : uint8_t // Used in `SendRouterAdvertisement()`
    {
        kInvalidateAllPrevPrefixes,
        kAdvPrefixesFromNetData,
    };

    enum ScheduleMode : uint8_t // Used in `ScheduleRoutingPolicyEvaluation()`
    {
        kImmediately,
        kForNextRa,
        kAfterRandomDelay,
        kToReplyToRs,
    };

    void HandleDiscoveredPrefixTableChanged(void); // Declare early so we can use in `mSignalTask`
    void HandleDiscoveredPrefixTableEntryTimer(void) { mDiscoveredPrefixTable.HandleEntryTimer(); }
    void HandleDiscoveredPrefixTableRouterTimer(void) { mDiscoveredPrefixTable.HandleRouterTimer(); }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    class PdPrefixManager; // For DiscoveredPrefixTable::Entry
#endif

    class DiscoveredPrefixTable : public InstanceLocator
    {
        // This class maintains the discovered on-link and route prefixes
        // from the received RA messages by processing PIO and RIO options
        // from the message. It takes care of processing the RA message but
        // delegates the decision whether to include or exclude a prefix to
        // `RoutingManager` by calling its `ShouldProcessPrefixInfoOption()`
        // and `ShouldProcessRouteInfoOption()` methods.
        //
        // It manages the lifetime of the discovered entries and publishes
        // and unpublishes the prefixes in the Network Data (as external
        // route) as they are added or removed.
        //
        // When there is any change in the table (an entry is added, removed,
        // or modified), it signals the change to `RoutingManager` by calling
        // `HandleDiscoveredPrefixTableChanged()` callback. A `Tasklet` is
        // used for signalling which ensures that if there are multiple
        // changes within the same flow of execution, the callback is
        // invoked after all the changes are processed.

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
        friend class PdPrefixManager; // For DiscoveredPrefixTable::Entry
#endif

    public:
        explicit DiscoveredPrefixTable(Instance &aInstance);

        void ProcessRouterAdvertMessage(const Ip6::Nd::RouterAdvertMessage &aRaMessage,
                                        const Ip6::Address                 &aSrcAddress);
        void ProcessNeighborAdvertMessage(const Ip6::Nd::NeighborAdvertMessage &aNaMessage);

        bool ContainsDefaultOrNonUlaRoutePrefix(void) const;
        bool ContainsNonUlaOnLinkPrefix(void) const;
        bool ContainsUlaOnLinkPrefix(void) const;

        void FindFavoredOnLinkPrefix(Ip6::Prefix &aPrefix) const;

        void RemoveOnLinkPrefix(const Ip6::Prefix &aPrefix);
        void RemoveRoutePrefix(const Ip6::Prefix &aPrefix);

        void RemoveAllEntries(void);
        void RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold);

        TimeMilli CalculateNextStaleTime(TimeMilli aNow) const;

        void DetermineAndSetFlags(Ip6::Nd::RouterAdvertMessage &aRaMessage) const;

        void  InitIterator(PrefixTableIterator &aIterator) const;
        Error GetNextEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const;
        Error GetNextRouter(PrefixTableIterator &aIterator, RouterEntry &aEntry) const;

        void HandleEntryTimer(void);
        void HandleRouterTimer(void);

    private:
#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        static constexpr uint16_t kMaxRouters = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS;
        static constexpr uint16_t kMaxEntries = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES;
#endif

        class Entry : public LinkedListEntry<Entry>,
                      public Unequatable<Entry>,
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
                      public Heap::Allocatable<Entry>,
#endif
                      private Clearable<Entry>
        {
            friend class LinkedListEntry<Entry>;
            friend class Clearable<Entry>;
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
            friend class PdPrefixManager;
#endif

        public:
            enum Type : uint8_t
            {
                kTypeOnLink,
                kTypeRoute,
            };

            struct Matcher
            {
                Matcher(const Ip6::Prefix &aPrefix, Type aType)
                    : mPrefix(aPrefix)
                    , mType(aType)
                {
                }

                const Ip6::Prefix &mPrefix;
                Type               mType;
            };

            struct Checker
            {
                enum Mode : uint8_t
                {
                    kIsUla,
                    kIsNotUla,
                };

                Checker(Mode aMode, Type aType)
                    : mMode(aMode)
                    , mType(aType)

                {
                }

                Mode mMode;
                Type mType;
            };

            struct ExpirationChecker
            {
                explicit ExpirationChecker(TimeMilli aNow)
                    : mNow(aNow)
                {
                }

                TimeMilli mNow;
            };

            void               SetFrom(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader);
            void               SetFrom(const Ip6::Nd::PrefixInfoOption &aPio);
            void               SetFrom(const Ip6::Nd::RouteInfoOption &aRio);
            Type               GetType(void) const { return mType; }
            bool               IsOnLinkPrefix(void) const { return (mType == kTypeOnLink); }
            bool               IsRoutePrefix(void) const { return (mType == kTypeRoute); }
            const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
            const TimeMilli   &GetLastUpdateTime(void) const { return mLastUpdateTime; }
            uint32_t           GetValidLifetime(void) const { return mValidLifetime; }
            void               ClearValidLifetime(void) { mValidLifetime = 0; }
            TimeMilli          GetExpireTime(void) const;
            TimeMilli          GetStaleTime(void) const;
            RoutePreference    GetPreference(void) const;
            bool               operator==(const Entry &aOther) const;
            bool               Matches(const Matcher &aMatcher) const;
            bool               Matches(const Checker &aChecker) const;
            bool               Matches(const ExpirationChecker &aChecker) const;

            // Methods to use when `IsOnLinkPrefix()`
            uint32_t GetPreferredLifetime(void) const { return mShared.mPreferredLifetime; }
            void     ClearPreferredLifetime(void) { mShared.mPreferredLifetime = 0; }
            bool     IsDeprecated(void) const;
            void     AdoptValidAndPreferredLifetimesFrom(const Entry &aEntry);

            // Method to use when `!IsOnlinkPrefix()`
            RoutePreference GetRoutePreference(void) const { return mShared.mRoutePreference; }

        private:
            static uint32_t CalculateExpireDelay(uint32_t aValidLifetime);

            Entry      *mNext;
            Ip6::Prefix mPrefix;
            Type        mType;
            TimeMilli   mLastUpdateTime;
            uint32_t    mValidLifetime;
            union
            {
                uint32_t        mPreferredLifetime; // Applicable when prefix is on-link.
                RoutePreference mRoutePreference;   // Applicable when prefix is not on-link
            } mShared;
        };

        struct Router : public LinkedListEntry<Router>,
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
                        public Heap::Allocatable<Router>,
#endif
                        public Clearable<Router>
        {
            // The timeout (in msec) for router staying in active state
            // before starting the Neighbor Solicitation (NS) probes.
            static constexpr uint32_t kActiveTimeout = OPENTHREAD_CONFIG_BORDER_ROUTING_ROUTER_ACTIVE_CHECK_TIMEOUT;

            static constexpr uint8_t  kMaxNsProbes          = 5;    // Max number of NS probe attempts.
            static constexpr uint32_t kNsProbeRetryInterval = 1000; // In msec. Time between NS probe attempts.
            static constexpr uint32_t kNsProbeTimeout       = 2000; // In msec. Max Wait time after last NS probe.
            static constexpr uint32_t kJitter               = 2000; // In msec. Jitter to randomize probe starts.

            static_assert(kMaxNsProbes < 255, "kMaxNsProbes MUST not be 255");

            enum EmptyChecker : uint8_t
            {
                kContainsNoEntriesOrFlags
            };

            bool Matches(const Ip6::Address &aAddress) const { return aAddress == mAddress; }
            bool Matches(EmptyChecker aChecker) const;
            void CopyInfoTo(RouterEntry &aEntry) const;

            Router           *mNext;
            Ip6::Address      mAddress;
            LinkedList<Entry> mEntries;
            TimeMilli         mTimeout;
            uint8_t           mNsProbeCount;
            bool              mManagedAddressConfigFlag : 1;
            bool              mOtherConfigFlag : 1;
            bool              mStubRouterFlag : 1;
        };

        class Iterator : public PrefixTableIterator
        {
        public:
            enum AdvanceMode : uint8_t
            {
                kToNextEntry,
                kToNextRouter,
            };

            void Init(const LinkedList<Router> &aRouters);
            void Advance(AdvanceMode aMode);

            const Router *GetRouter(void) const { return static_cast<const Router *>(mPtr1); }
            const Entry  *GetEntry(void) const { return static_cast<const Entry *>(mPtr2); }
            TimeMilli     GetInitTime(void) const { return TimeMilli(mData32); }

        private:
            void SetRouter(const Router *aRouter) { mPtr1 = aRouter; }
            void SetEntry(const Entry *aEntry) { mPtr2 = aEntry; }
            void SetInitTime(void) { mData32 = TimerMilli::GetNow().GetValue(); }
        };

        void         ProcessRaHeader(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader, Router &aRouter);
        void         ProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio, Router &aRouter);
        void         ProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio, Router &aRouter);
        void         ProcessRaFlagsExtOption(const Ip6::Nd::RaFlagsExtOption &aFlagsOption, Router &aRouter);
        bool         Contains(const Entry::Checker &aChecker) const;
        void         RemovePrefix(const Entry::Matcher &aMatcher);
        void         RemoveOrDeprecateEntriesFromInactiveRouters(void);
        void         RemoveRoutersWithNoEntriesOrFlags(void);
        void         FreeRouters(LinkedList<Router> &aRouters);
        void         FreeEntries(LinkedList<Entry> &aEntries);
        void         UpdateNetworkDataOnChangeTo(Entry &aEntry);
        const Entry *FindFavoredEntryToPublish(const Ip6::Prefix &aPrefix) const;
        void         RemoveExpiredEntries(void);
        void         SignalTableChanged(void);
        void         UpdateRouterOnRx(Router &aRouter);
        void         SendNeighborSolicitToRouter(const Router &aRouter);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        Router *AllocateRouter(void) { return Router::Allocate(); }
        Entry  *AllocateEntry(void) { return Entry::Allocate(); }
        void    FreeRouter(Router &aRouter) { aRouter.Free(); }
        void    FreeEntry(Entry &aEntry) { aEntry.Free(); }
#else
        Router *AllocateRouter(void) { return mRouterPool.Allocate(); }
        Entry  *AllocateEntry(void) { return mEntryPool.Allocate(); }
        void    FreeRouter(Router &aRouter) { mRouterPool.Free(aRouter); }
        void    FreeEntry(Entry &aEntry) { mEntryPool.Free(aEntry); }
#endif

        using SignalTask  = TaskletIn<RoutingManager, &RoutingManager::HandleDiscoveredPrefixTableChanged>;
        using EntryTimer  = TimerMilliIn<RoutingManager, &RoutingManager::HandleDiscoveredPrefixTableEntryTimer>;
        using RouterTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleDiscoveredPrefixTableRouterTimer>;

        LinkedList<Router> mRouters;
        EntryTimer         mEntryTimer;
        RouterTimer        mRouterTimer;
        SignalTask         mSignalTask;
#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
        Pool<Entry, kMaxEntries>  mEntryPool;
        Pool<Router, kMaxRouters> mRouterPool;
#endif
    };

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
        bool               IsInitalEvaluationDone(void) const;
        void               HandleDiscoveredPrefixTableChanged(void);
        bool               ShouldPublishUlaRoute(void) const;
        void               AppendAsPiosTo(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        bool               IsPublishingOrAdvertising(void) const;
        void               HandleNetDataChange(void);
        void               HandleExtPanIdChange(void);
        void               HandleTimer(void);

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
        void  GenerateLocalPrefix(void);
        void  PublishAndAdvertise(void);
        void  Deprecate(void);
        void  ResetExpireTime(TimeMilli aNow);
        void  AppendCurPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        void  AppendOldPrefixes(Ip6::Nd::RouterAdvertMessage &aRaMessage);
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
        void            AppendRios(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        void            InvalidatPrevRios(Ip6::Nd::RouterAdvertMessage &aRaMessage);
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

        void SetPreferenceBasedOnRole(void);
        void UpdatePreference(RoutePreference aPreference);
        void AppendRio(const Ip6::Prefix &aPrefix, uint32_t aRouteLifetime, Ip6::Nd::RouterAdvertMessage &aRaMessage);

        using RioTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleRioAdvertiserimer>;

        RioPrefixArray  mPrefixes;
        RioTimer        mTimer;
        RoutePreference mPreference;
        bool            mUserSetPreference;
    };

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

    struct RaInfo
    {
        // Tracks info about emitted RA messages: Number of RAs sent,
        // last tx time, header to use and whether the header is
        // discovered from receiving RAs from the host itself. This
        // ensures that if an entity on host is advertising certain
        // info in its RA header (e.g., a default route), the RAs we
        // emit from `RoutingManager` also include the same header.

        RaInfo(void)
            : mHeaderUpdateTime(TimerMilli::GetNow())
            , mIsHeaderFromHost(false)
            , mTxCount(0)
            , mLastTxTime(TimerMilli::GetNow() - kMinDelayBetweenRtrAdvs)
        {
        }

        Ip6::Nd::RouterAdvertMessage::Header mHeader;
        TimeMilli                            mHeaderUpdateTime;
        bool                                 mIsHeaderFromHost;
        uint32_t                             mTxCount;
        TimeMilli                            mLastTxTime;
    };

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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    void HandlePdPrefixManagerTimer(void) { mPdPrefixManager.HandleTimer(); }

    class PdPrefixManager : public InstanceLocator
    {
    public:
        // This class implements handling (including management of the lifetime) of the prefix obtained from platform's
        // DHCPv6 PD client. We expect the platform will send ICMP6 RA messages to the platform's interface for the
        // information of the prefix.
        // This class manages the state of the PD prefix in OmrPrefixManager

        explicit PdPrefixManager(Instance &aInstance);

        void               SetEnabled(bool aEnabled);
        void               Start(void) { StartStop(/* aStart= */ true); }
        void               Stop(void) { StartStop(/* aStart= */ false); }
        bool               IsRunning(void) const { return GetState() == Dhcp6PdState::kDhcp6PdStateRunning; }
        bool               HasPrefix(void) const { return IsValidOmrPrefix(mPrefix.GetPrefix()); }
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix.GetPrefix(); }
        Dhcp6PdState       GetState(void) const;

        void  ProcessPlatformGeneratedRa(const uint8_t *aRouterAdvert, uint16_t aLength);
        Error GetPrefixInfo(PrefixTableEntry &aInfo) const;
        Error GetProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo) const;
        void  HandleTimer(void) { WithdrawPrefix(); }

        static const char *StateToString(Dhcp6PdState aState);

        static bool IsValidPdPrefix(const Ip6::Prefix &aPrefix)
        {
            // We should accept ULA prefix since it could be used by the internet infrastructure like NAT64.
            return aPrefix.GetLength() != 0 && aPrefix.GetLength() <= kOmrPrefixLength && !aPrefix.IsLinkLocal() &&
                   !aPrefix.IsMulticast();
        }

    private:
        Error Process(const Ip6::Nd::RouterAdvertMessage &aMessage);
        void  EvaluateStateChange(Dhcp6PdState aOldState);
        void  WithdrawPrefix(void);
        void  StartStop(bool aStart);

        using PlatformOmrPrefixTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandlePdPrefixManagerTimer>;

        bool                         mEnabled;
        bool                         mIsRunning;
        uint32_t                     mNumPlatformPioProcessed;
        uint32_t                     mNumPlatformRaReceived;
        TimeMilli                    mLastPlatformRaTime;
        PlatformOmrPrefixTimer       mTimer;
        DiscoveredPrefixTable::Entry mPrefix;
    };
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

    void  EvaluateState(void);
    void  Start(void);
    void  Stop(void);
    void  HandleNotifierEvents(Events aEvents);
    bool  IsInitialized(void) const { return mInfraIf.IsInitialized(); }
    bool  IsEnabled(void) const { return mIsEnabled; }
    Error LoadOrGenerateRandomBrUlaPrefix(void);

    void EvaluateRoutingPolicy(void);
    bool IsInitalPolicyEvaluationDone(void) const;
    void ScheduleRoutingPolicyEvaluation(ScheduleMode aMode);
    void HandleRsSenderFinished(TimeMilli aStartTime);
    void SendRouterAdvertisement(RouterAdvTxMode aRaTxMode);

    void HandleDiscoveredPrefixStaleTimer(void);

    void HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleNeighborAdvertisement(const InfraIf::Icmp6Packet &aPacket);
    bool ShouldProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio, const Ip6::Prefix &aPrefix);
    bool ShouldProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio, const Ip6::Prefix &aPrefix);
    void UpdateDiscoveredPrefixTableOnNetDataChange(void);
    bool NetworkDataContainsOmrPrefix(const Ip6::Prefix &aPrefix) const;
    bool NetworkDataContainsUlaRoute(void) const;
    void UpdateRouterAdvertHeader(const Ip6::Nd::RouterAdvertMessage *aRouterAdvertMessage);
    bool IsReceivedRouterAdvertFromManager(const Ip6::Nd::RouterAdvertMessage &aRaMessage) const;
    void ResetDiscoveredPrefixStaleTimer(void);

    static bool IsValidBrUlaPrefix(const Ip6::Prefix &aBrUlaPrefix);
    static bool IsValidOnLinkPrefix(const Ip6::Nd::PrefixInfoOption &aPio);
    static bool IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    static void LogPrefixInfoOption(const Ip6::Prefix &aPrefix, uint32_t aValidLifetime, uint32_t aPreferredLifetime);
    static void LogRouteInfoOption(const Ip6::Prefix &aPrefix, uint32_t aLifetime, RoutePreference aPreference);

    using RoutingPolicyTimer         = TimerMilliIn<RoutingManager, &RoutingManager::EvaluateRoutingPolicy>;
    using DiscoveredPrefixStaleTimer = TimerMilliIn<RoutingManager, &RoutingManager::HandleDiscoveredPrefixStaleTimer>;

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

    DiscoveredPrefixTable mDiscoveredPrefixTable;

    RoutePublisher mRoutePublisher;

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    Nat64PrefixManager mNat64PrefixManager;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    PdPrefixManager mPdPrefixManager;
#endif

    RaInfo   mRaInfo;
    RsSender mRsSender;

    DiscoveredPrefixStaleTimer mDiscoveredPrefixStaleTimer;
    RoutingPolicyTimer         mRoutingPolicyTimer;
};

} // namespace BorderRouter

DefineMapEnum(otBorderRoutingState, BorderRouter::RoutingManager::State);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTING_MANAGER_HPP_
