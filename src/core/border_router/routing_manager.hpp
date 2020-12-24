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

#include <openthread/error.h>
#include <openthread/netdata.h>
#include <openthread/platform/infra_if.h>

#include "border_router/router_advertisement.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"

namespace ot {

namespace BorderRouter {

/**
 * This class implements bi-directional routing between Thread and
 * Infrastructure networks.
 *
 * The Border Routing manager works on both Thread interface and
 * infrastructure interface. All ICMPv6 messages are sent/recv
 * on the infrastructure interface.
 *
 */
class RoutingManager : public InstanceLocator
{
    friend class ot::Notifier;

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
     * @param[in]  aInfraIfIndex  An infrastructure network interface index.
     *
     * @retval  OT_ERROR_NONE          Successfully started the routing manager.
     * @retval  OT_ERROR_INVALID_ARGS  The index of the infra interface is not valid.
     *
     */
    otError Init(uint32_t aInfraIfIndex);

    /**
     * This method enables/disables the Border Routing Manager.
     *
     * @param[in]  aEnabled   A boolean to enable/disable the Border Routing Manager.
     *
     * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
     * @retval  OT_ERROR_NONE           Successfully enabled the Border Routing Manager.
     *
     * @note  The Border Routing Manager is enabled by default.
     *
     */
    otError SetEnabled(bool aEnabled);

    /**
     * This method receives an ICMPv6 message on the infrastructure interface.
     *
     * Malformed or undesired messages are dropped silently.
     *
     * @param[in]  aInfraIfIndex  The infrastructure interface index.
     * @param[in]  aSrcAddress    The source address this message is sent from.
     * @param[in]  aBuffer        THe ICMPv6 message buffer.
     * @param[in]  aLength        The length of the ICMPv6 message buffer.
     *
     */
    void RecvIcmp6Message(uint32_t            aInfraIfIndex,
                          const Ip6::Address &aSrcAddress,
                          const uint8_t *     aBuffer,
                          uint16_t            aBufferLength);

private:
    enum : uint16_t
    {
        kMaxRouterAdvMessageLength = 256u, // The maximum RA message length we can handle.
    };

    enum : uint8_t
    {
        kMaxOmrPrefixNum =
            OPENTHREAD_CONFIG_IP6_SLAAC_NUM_ADDRESSES, // The maximum number of the OMR prefixes to advertise.
        kMaxDiscoveredPrefixNum = 8u,                  // The maximum number of prefixes to discover on the infra link.
        kOmrPrefixLength        = OT_IP6_PREFIX_BITSIZE, // The length of an OMR prefix. In bits.
        kOnLinkPrefixLength     = OT_IP6_PREFIX_BITSIZE, // The length of an On-link prefix. In bits.
    };

    enum : uint32_t
    {
        kDefaultOmrPrefixLifetime    = 1800u,                  // The default OMR prefix valid lifetime. In seconds.
        kDefaultOnLinkPrefixLifetime = 1800u,                  // The default on-link prefix valid lifetime. In seconds.
        kMaxRtrAdvInterval           = 600,                    // Maximum Router Advertisement Interval. In seconds.
        kMinRtrAdvInterval           = kMaxRtrAdvInterval / 3, // Minimum Router Advertisement Interval. In seconds.
        kMaxInitRtrAdvInterval       = 16,  // Maximum Initial Router Advertisement Interval. In seconds.
        kMaxRaDelayTime              = 500, // The maximum delay of sending RA after receiving RS. In milliseconds.
        kRtrSolicitationInterval     = 4,   // The interval between Router Solicitations. In seconds.
        kMaxRtrSolicitationDelay     = 1,   // The maximum delay for initial solicitation. In seconds.
    };

    static_assert(kMinRtrAdvInterval <= 3 * kMaxRtrAdvInterval / 4, "invalid RA intervals");
    static_assert(kDefaultOmrPrefixLifetime >= kMaxRtrAdvInterval, "invalid default OMR prefix lifetime");
    static_assert(kDefaultOnLinkPrefixLifetime >= kMaxRtrAdvInterval, "invalid default on-link prefix lifetime");

    enum : uint32_t
    {
        kMaxInitRtrAdvertisements = 3, // The maximum number of initial Router Advertisements.
        kMaxRtrSolicitations = 3, // The Maximum number of Router Solicitations before sending Router Advertisements.
    };

    /**
     * This struct represents an external prefix which is discovered on the infrastructure interface.
     *
     */
    struct ExternalPrefix : public Clearable<ExternalPrefix>
    {
        Ip6::Prefix mPrefix;
        TimeMilli   mExpireTime;
        bool        mIsOnLinkPrefix;
    };

    void    Start(void);
    void    Stop(void);
    void    HandleNotifierEvents(Events aEvents);
    bool    IsInitialized(void) const { return mInfraIfIndex != 0; }
    bool    IsEnabled(void) const { return mEnabled; }
    otError LoadOrGenerateRandomOmrPrefix(void);
    otError LoadOrGenerateRandomOnLinkPrefix(void);

    /**
     * This method tells whether the first prefix is numerically smaller than the second one.
     *
     * @note  The caller must guarantee that the two prefix has the same length.
     *
     */
    static bool IsPrefixSmallerThan(const Ip6::Prefix &aFirstPrefix, const Ip6::Prefix &aSecondPrefix);

    static bool IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix);
    static bool IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    /**
     * This method evaluate the routing policy depends on prefix and route
     * information on Thread Network and infra link. As a result, this
     * method May send RA messages on infra link and publish/unpublish
     * OMR prefix in the Thread network.
     *
     * @sa EvaluateOmrPrefix
     * @sa EvaluateOnLinkPrefix
     * @sa PublishLocalOmrPrefix
     * @sa UnpublishLocalOmrPrefix
     *
     */
    void EvaluateRoutingPolicy(void);

    /**
     * This method evaluates the OMR prefix for the Thread Network.
     *
     * @param[out]  aNewOmrPrefixes   An array of the new OMR prefixes should be advertised.
     *                                MUST not be nullptr.
     * @param[in]   aMaxOmrPrefixNum  The maximum number of OMR prefixes that @p aNewOmrPrefixes can hold.
     *
     * @returns  The number of the new OMR prefixes that should be advertised after this evaluation.
     *
     */
    uint8_t EvaluateOmrPrefix(Ip6::Prefix *aNewOmrPrefixes, uint8_t aMaxOmrPrefixNum);

    /**
     * This method evaluates the on-link prefix for the infra link.
     *
     * @returns  A pointer to the new on-link prefix should be advertised.
     *           nullptr if we should no longer advertise an on-link prefix.
     *
     */
    const Ip6::Prefix *EvaluateOnLinkPrefix(void);

    /**
     * This method publishes the local OMR prefix in Thread network.
     *
     */
    otError PublishLocalOmrPrefix(void);

    /**
     * This method unpublishes the local OMR prefix.
     *
     */
    void UnpublishLocalOmrPrefix(void);

    /**
     * This method adds external route for the prefix.
     *
     */
    otError AddExternalRoute(const Ip6::Prefix &aPrefix, otRoutePreference aRoutePreference);

    /**
     * This method removes the external route added for the prefix.
     *
     */
    void RemoveExternalRoute(const Ip6::Prefix &aPrefix);

    /**
     * This method starts sending Router Solicitations in random delay
     * between 0 and kMaxRtrSolicitationDelay.
     *
     */
    void StartRouterSolicitation(void);

    /**
     * This method sends Router Solicitation messages to discover on-link
     * prefix on infra links.
     *
     * @sa HandleRouterAdvertisement
     *
     * @retval  OT_ERROR_NONE    Successfully sent the message.
     * @retval  OT_ERROR_FAILED  Failed to send the message.
     *
     */
    otError SendRouterSolicitation(void);

    /**
     * This method sends Router Advertisement messages to advertise on-link prefix and route for OMR prefix.
     *
     * @param[in]  aNewOmrPrefixes   A pointer to an array of the new OMR prefixes to be advertised.
     *                               @p aNewOmrPrefixNum must be zero if this argument is nullptr.
     * @param[in]  aNewOmrPrefixNum  The number of the new OMR prefixes to be advertised.
     *                               Zero means we should stop advertising OMR prefixes.
     * @param[in]  aOnLinkPrefix     A pointer to the new on-link prefix to be advertised.
     *                               nullptr means we should stop advertising on-link prefix.
     *
     */
    void SendRouterAdvertisement(const Ip6::Prefix *aNewOmrPrefixes,
                                 uint8_t            aNewOmrPrefixNum,
                                 const Ip6::Prefix *aNewOnLinkPrefix);

    static void HandleRouterAdvertisementTimer(Timer &aTimer);
    void        HandleRouterAdvertisementTimer(void);

    static void HandleRouterSolicitTimer(Timer &aTimer);
    void        HandleRouterSolicitTimer(void);

    static void HandleDiscoveredPrefixInvalidTimer(Timer &aTimer);
    void        HandleDiscoveredPrefixInvalidTimer(void);

    void HandleRouterSolicit(const Ip6::Address &aSrcAddress, const uint8_t *aBuffer, uint16_t aBufferLength);
    void HandleRouterAdvertisement(const Ip6::Address &aSrcAddress, const uint8_t *aBuffer, uint16_t aBufferLength);

    /**
     * This method updates the discovered prefix list with given Prefix Information Option.
     *
     * @param[in]  aPio  A reference to the Prefix Information Option.
     *
     * @returns  A boolean indicates whether we need to re-evaluate our routing policy.
     *
     */
    bool UpdateDiscoveredPrefixes(const RouterAdv::PrefixInfoOption &aPio);

    /**
     * This method updates the discovered prefix list with given Route Information Option.
     *
     * @param[in]  aPio  A reference to the Route Information Option.
     *
     * @returns  A boolean indicates whether we need to re-evaluate our routing policy.
     *
     */
    bool UpdateDiscoveredPrefixes(const RouterAdv::RouteInfoOption &aRio);

    /**
     * This method removes expired or specific discovered prefixes.
     *
     * @param[in]  aPrefix          A pointer to a specific prefix to be removed.
     * @param[in]  aIsOnLinkPrefix  A boolean which indicates whether the prefix is an on-link prefix.
     *
     * @returns  The number of removed prefixes.
     *
     */
    uint8_t InvalidateDiscoveredPrefixes(const Ip6::Prefix *aPrefix = nullptr, bool aIsOnLinkPrefix = true);

    /**
     * This method invalidates all discovered prefixes.
     *
     */
    void InvalidateAllDiscoveredPrefixes(void);

    /**
     * This methods adds a prefix discovered in a RA message.
     *
     * If the same prefix already exists, only the lifetime will be updated.
     *
     * @param[in]  aPrefix           The prefix to be added.
     * @param[in]  aIsOnLinkPrefix   A boolean which indicates whether the prefix is an on-link prefix.
     * @param[in]  aLifetime         The lifetime of the prefix. In seconds.
     * @param[in]  aRoutePreference  The preference of the route which is associated with the prefix.
     *
     * @returns  A boolean indicates whether a new prefix is added.
     *
     */
    bool AddDiscoveredPrefix(const Ip6::Prefix &aPrefix,
                             bool               aIsOnLinkPrefix,
                             uint32_t           aLifetime,
                             otRoutePreference  aRoutePreference = OT_ROUTE_PREFERENCE_MED);

    static bool ContainsPrefix(const Ip6::Prefix &aPrefix, const Ip6::Prefix *aPrefixList, uint8_t aPrefixNum);

    static uint32_t GetPrefixExpireDelay(uint32_t aValidLifetime);

    bool     mIsRunning;
    uint32_t mInfraIfIndex;
    bool     mEnabled;

    /**
     * The OMR prefix loaded from local persistent storage or randomly generated
     * if non is found in persistent storage.
     *
     */
    Ip6::Prefix mLocalOmrPrefix;

    /**
     * The advertised OMR prefixes. For a stable Thread network without
     * manually configured OMR prefixes, there should be a single OMR prefix
     * that being advertised because each BRs will converge to the smallest
     * OMR prefix sorted by method IsPrefixSmallerThan. If manually configured
     * OMR prefixes exist, they will also be advertised on infra link.
     *
     */
    Ip6::Prefix mAdvertisedOmrPrefixes[kMaxOmrPrefixNum];
    uint8_t     mAdvertisedOmrPrefixNum;

    /**
     * The on-link prefix loaded from local persistent storage or randomly generated
     * if non is found in persistent storage.
     *
     */
    Ip6::Prefix mLocalOnLinkPrefix;

    /**
     * The advertised on-link prefix.
     *
     * Could only be nullptr or a pointer to mLocalOnLinkPrefix.
     *
     */
    const Ip6::Prefix *mAdvertisedOnLinkPrefix;

    /**
     * The array of prefixes discovered on the infra link. Those prefixes consist of
     * on-link prefix(es) and OMR prefixes advertised by BRs in another Thread Network
     * which is connected to the same infra link.
     *
     */
    ExternalPrefix mDiscoveredPrefixes[kMaxDiscoveredPrefixNum];
    uint8_t        mDiscoveredPrefixNum;

    TimerMilli mDiscoveredPrefixInvalidTimer;

    TimerMilli mRouterAdvertisementTimer;
    uint32_t   mRouterAdvertisementCount;

    TimerMilli mRouterSolicitTimer;
    uint8_t    mRouterSolicitCount;
};

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTING_MANAGER_HPP_
