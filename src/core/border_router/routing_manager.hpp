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

#include <openthread/netdata.h>

#include "border_router/infra_if.hpp"
#include "border_router/router_advertisement.hpp"
#include "common/array.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "thread/network_data.hpp"

namespace ot {

namespace BorderRouter {

/**
 * This class implements bi-directional routing between Thread and Infrastructure networks.
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
    /**
     * This constructor initializes the routing manager.
     *
     * @param[in]  aInstance  A OpenThread instance.
     *
     */
    explicit RoutingManager(Instance &aInstance);

    /**
     * This method initializes the routing manager on given infrastructure interface.
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
     * This method enables/disables the Border Routing Manager.
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
     * This method returns the off-mesh-routable (OMR) prefix.
     *
     * The randomly generated 64-bit prefix will be published
     * in the Thread network if there isn't already an OMR prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the OMR prefix.
     *
     */
    Error GetOmrPrefix(Ip6::Prefix &aPrefix);

    /**
     * This method returns the on-link prefix for the adjacent  infrastructure link.
     *
     * The randomly generated 64-bit prefix will be advertised
     * on the infrastructure link if there isn't already a usable
     * on-link prefix being advertised on the link.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the on-link prefix.
     *
     */
    Error GetOnLinkPrefix(Ip6::Prefix &aPrefix);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    /**
     * This method returns the local NAT64 prefix.
     *
     * The local NAT64 prefix will be published in the Thread network
     * if none exists.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     *
     */
    Error GetNat64Prefix(Ip6::Prefix &aPrefix);
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE

    /**
     * This method processes a received ICMPv6 message from the infrastructure interface.
     *
     * Malformed or undesired messages are dropped silently.
     *
     * @param[in]  aPacket        The received ICMPv6 packet.
     * @param[in]  aSrcAddress    The source address this message is sent from.
     *
     */
    void HandleReceived(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);

    /**
     * This method handles infrastructure interface state changes.
     *
     */
    void HandleInfraIfStateChanged(void) { EvaluateState(); }

    /**
     * This method checks if the on-mesh prefix configuration is a valid OMR prefix.
     *
     * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to check.
     *
     * @returns  Whether the on-mesh prefix configuration is a valid OMR prefix.
     *
     */
    static bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

    /**
     * This method checks if the OMR prefix is valid (i.e. GUA/ULA prefix with length being 64).
     *
     * @param[in]  aOmrPrefix  The OMR prefix to check.
     * @returns    Whether the OMR prefix is valid.
     *
     */
    static bool IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix);

private:
    typedef NetworkData::RoutePreference RoutePreference;

    static constexpr uint16_t kMaxRouterAdvMessageLength = 256; // The maximum RA message length we can handle.

    // The maximum number of the OMR prefixes to advertise.
    static constexpr uint8_t kMaxOmrPrefixNum = OPENTHREAD_CONFIG_IP6_SLAAC_NUM_ADDRESSES;

    // The maximum number of prefixes to discover on the infra link.
    static constexpr uint8_t kMaxDiscoveredPrefixNum = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES;

    static constexpr uint8_t kOmrPrefixLength    = OT_IP6_PREFIX_BITSIZE; // The length of an OMR prefix. In bits.
    static constexpr uint8_t kOnLinkPrefixLength = OT_IP6_PREFIX_BITSIZE; // The length of an On-link prefix. In bits.
    static constexpr uint8_t kBrUlaPrefixLength  = 48;                    // The length of a BR ULA prefix. In bits.
    static constexpr uint8_t kNat64PrefixLength  = 96;                    // The length of a NAT64 prefix. In bits.

    static constexpr uint16_t kOmrPrefixSubnetId   = 1; // The subnet ID of an OMR prefix within a BR ULA prefix.
    static constexpr uint16_t kNat64PrefixSubnetId = 2; // The subnet ID of a NAT64 prefix within a BR ULA prefix.

    // The maximum number of initial Router Advertisements.
    static constexpr uint32_t kMaxInitRtrAdvertisements = 3;

    // The maximum number of Router Solicitations before sending Router Advertisements.
    static constexpr uint32_t kMaxRtrSolicitations = 3;

    static constexpr uint32_t kDefaultOmrPrefixLifetime    = 1800; // The default OMR prefix valid lifetime. In sec.
    static constexpr uint32_t kDefaultOnLinkPrefixLifetime = 1800; // The default on-link prefix valid lifetime. In sec.
    static constexpr uint32_t kMaxRtrAdvInterval           = 600;  // Max Router Advertisement Interval. In sec.
    static constexpr uint32_t kMinRtrAdvInterval           = kMaxRtrAdvInterval / 3; // Min RA Interval. In sec.
    static constexpr uint32_t kMaxInitRtrAdvInterval       = 16;                     // Max Initial RA Interval. In sec.
    static constexpr uint32_t kRaReplyJitter               = 500;    // Jitter for sending RA after rx RS. In msec.
    static constexpr uint32_t kRtrSolicitationInterval     = 4;      // Interval between RSs. In sec.
    static constexpr uint32_t kMaxRtrSolicitationDelay     = 1;      // Max delay for initial solicitation. In sec.
    static constexpr uint32_t kRoutingPolicyEvaluationJitter = 1000; // Jitter for routing policy evaluation. In msec.
    static constexpr uint32_t kRtrSolicitationRetryDelay =
        kRtrSolicitationInterval;                             // The delay before retrying failed RS tx. In Sec.
    static constexpr uint32_t kMinDelayBetweenRtrAdvs = 3000; // Min delay (msec) between consecutive RAs.

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

    // A prefix discovered from Router Advert msg from infra netif
    class ExternalPrefix : private Clearable<ExternalPrefix>, public Unequatable<ExternalPrefix>
    {
    public:
        void               InitFrom(const RouterAdv::PrefixInfoOption &aPio);
        void               InitFrom(const RouterAdv::RouteInfoOption &aRio);
        bool               IsOnLinkPrefix(void) const { return mIsOnLinkPrefix; }
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        const TimeMilli &  GetLastUpdateTime(void) const { return mLastUpdateTime; }
        uint32_t           GetValidLifetime(void) const { return mValidLifetime; }
        void               ClearValidLifetime(void) { mValidLifetime = 0; }
        TimeMilli          GetExpireTime(void) const { return mLastUpdateTime + GetPrefixExpireDelay(mValidLifetime); }
        TimeMilli          GetStaleTime(void) const;
        bool               operator==(const ExternalPrefix &aPrefix) const;

        // Methods to use when `IsOnLinkPrefix()`
        uint32_t GetPreferredLifetime(void) const { return mPreferredLifetime; }
        void     ClearPreferredLifetime(void) { mPreferredLifetime = 0; }
        bool     IsDeprecated(void) const;
        void     AdoptValidAndPreferredLiftimesFrom(const ExternalPrefix &Prefix);

        // Method to use when `!IsOnlinkPrefix()`
        RoutePreference GetRoutePreference(void) const { return mRoutePreference; }

    private:
        static uint32_t GetPrefixExpireDelay(uint32_t aValidLifetime);

        Ip6::Prefix     mPrefix;
        TimeMilli       mLastUpdateTime;
        uint32_t        mValidLifetime;
        uint32_t        mPreferredLifetime; // Applicable when prefix is on-link.
        RoutePreference mRoutePreference;   // Applicable when prefix is not on-link
        bool            mIsOnLinkPrefix;
    };

    typedef Array<Ip6::Prefix, kMaxOmrPrefixNum>           OmrPrefixArray;
    typedef Array<ExternalPrefix, kMaxDiscoveredPrefixNum> ExternalPrefixArray;

    void  EvaluateState(void);
    void  Start(void);
    void  Stop(void);
    void  HandleNotifierEvents(Events aEvents);
    bool  IsInitialized(void) const { return mInfraIf.IsInitialized(); }
    bool  IsEnabled(void) const { return mIsEnabled; }
    Error LoadOrGenerateRandomBrUlaPrefix(void);
    void  GenerateOmrPrefix(void);
    void  GenerateOnLinkPrefix(void);

    void EvaluateOnLinkPrefix(void);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    void GenerateNat64Prefix(void);
    void EvaluateNat64Prefix(void);
#endif

    void  EvaluateRoutingPolicy(void);
    void  StartRoutingPolicyEvaluationJitter(uint32_t aJitterMilli);
    void  StartRoutingPolicyEvaluationDelay(uint32_t aDelayMilli);
    void  EvaluateOmrPrefix(OmrPrefixArray &aNewOmrPrefixes);
    Error PublishLocalOmrPrefix(void);
    void  UnpublishLocalOmrPrefix(void);
    bool  IsOmrPrefixAddedToLocalNetworkData(void) const;
    Error PublishExternalRoute(const Ip6::Prefix &aPrefix, RoutePreference aRoutePreference, bool aNat64 = false);
    void  UnpublishExternalRoute(const Ip6::Prefix &aPrefix);
    void  StartRouterSolicitationDelay(void);
    Error SendRouterSolicitation(void);
    void  SendRouterAdvertisement(const OmrPrefixArray &aNewOmrPrefixes);
    bool  IsRouterSolicitationInProgress(void) const;

    static void HandleRouterSolicitTimer(Timer &aTimer);
    void        HandleRouterSolicitTimer(void);
    static void HandleDiscoveredPrefixInvalidTimer(Timer &aTimer);
    void        HandleDiscoveredPrefixInvalidTimer(void);
    static void HandleDiscoveredPrefixStaleTimer(Timer &aTimer);
    void        HandleDiscoveredPrefixStaleTimer(void);
    static void HandleRoutingPolicyTimer(Timer &aTimer);
    void        HandleOnLinkPrefixDeprecateTimer(void);
    static void HandleOnLinkPrefixDeprecateTimer(Timer &aTimer);

    void DeprecateOnLinkPrefix(void);
    void HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    bool UpdateDiscoveredOnLinkPrefix(const RouterAdv::PrefixInfoOption &aPio);
    void UpdateDiscoveredOmrPrefix(const RouterAdv::RouteInfoOption &aRio);
    void InvalidateDiscoveredPrefixes(void);
    void InvalidateAllDiscoveredPrefixes(void);
    bool NetworkDataContainsOmrPrefix(const Ip6::Prefix &aPrefix) const;
    bool UpdateRouterAdvMessage(const RouterAdv::RouterAdvMessage *aRouterAdvMessage);
    void ResetDiscoveredPrefixStaleTimer(void);

    static bool IsValidBrUlaPrefix(const Ip6::Prefix &aBrUlaPrefix);
    static bool IsValidOnLinkPrefix(const RouterAdv::PrefixInfoOption &aPio);
    static bool IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    // Indicates whether the Routing Manager is running (started).
    bool mIsRunning;

    // Indicates whether the Routing manager is enabled. The Routing
    // Manager will be stopped if we are disabled.
    bool mIsEnabled;

    InfraIf mInfraIf;

    // The /48 BR ULA prefix loaded from local persistent storage or
    // randomly generated if none is found in persistent storage.
    Ip6::Prefix mBrUlaPrefix;

    // The OMR prefix allocated from the /48 BR ULA prefix.
    Ip6::Prefix mLocalOmrPrefix;

    // The advertised OMR prefixes. For a stable Thread network without
    // manually configured OMR prefixes, there should be a single OMR
    // prefix that is being advertised because each BRs will converge to
    // the smallest OMR prefix sorted by method IsPrefixSmallerThan. If
    // manually configured OMR prefixes exist, they will also be
    // advertised on infra link.
    OmrPrefixArray mAdvertisedOmrPrefixes;

    // The on-link prefix loaded from local persistent storage or
    // randomly generated if non is found in persistent storage.
    Ip6::Prefix mLocalOnLinkPrefix;

    bool mIsAdvertisingLocalOnLinkPrefix;

    // The last time when the on-link prefix is advertised with
    // non-zero preferred lifetime.
    TimeMilli  mTimeAdvertisedOnLinkPrefix;
    TimerMilli mOnLinkPrefixDeprecateTimer;

    // The NAT64 prefix allocated from the /48 BR ULA prefix.
    Ip6::Prefix mLocalNat64Prefix;

    // True if the local NAT64 prefix is advertised in Thread network.
    bool mIsAdvertisingLocalNat64Prefix;

    // The array of prefixes discovered on the infra link. Those
    // prefixes consist of on-link prefix(es) and OMR prefixes
    // advertised by BRs in another Thread Network which is connected to
    // the same infra link.
    ExternalPrefixArray mDiscoveredPrefixes;

    // The RA header and parameters for the infra interface.
    // This value is initialized with `RouterAdvMessage::SetToDefault`
    // and updated with RA messages initiated from infra interface.
    RouterAdv::RouterAdvMessage mRouterAdvMessage;
    TimeMilli                   mTimeRouterAdvMessageLastUpdate;
    bool                        mLearntRouterAdvMessageFromHost;

    TimerMilli mDiscoveredPrefixInvalidTimer;
    TimerMilli mDiscoveredPrefixStaleTimer;

    uint32_t  mRouterAdvertisementCount;
    TimeMilli mLastRouterAdvertisementSendTime;

    TimerMilli mRouterSolicitTimer;
    TimeMilli  mTimeRouterSolicitStart;
    uint8_t    mRouterSolicitCount;

    TimerMilli mRoutingPolicyTimer;
};

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTING_MANAGER_HPP_
