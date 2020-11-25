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

#ifndef POSIX_ROUTING_MANAGER_HPP
#define POSIX_ROUTING_MANAGER_HPP

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

#include <openthread/openthread-system.h>

#include "router_advertisement.hpp"
#include "timer.hpp"

namespace ot {

namespace Posix {

/**
 * This class implements bi-directional routing between Thread and
 * Infrastructure networks.
 *
 */
class RoutingManager
{
public:
    /**
     * This constructor initializes the routing manager with a OpenThread instance.
     *
     * @param[in]  aInstance  A OpenThread instance.
     *
     */
    explicit RoutingManager(otInstance *aInstance);

    /**
     * this method initializes the routing manager with given infrastructure
     * network interface.
     *
     * @param[in]  aInfraNetifName  The infrastructure network interface
     *                              the routing manager will work on.
     *
     */
    void Init(const char *aInfraNetifName);

    /**
     * this method deinitializes the routing manager.
     *
     */
    void Deinit();

    /**
     * This method updates the mainloop context.
     *
     * @param[in]  aMainloop  The mainloop context to be updated.
     *
     */
    void Update(otSysMainloopContext *aMainloop);

    /**
     * This method processes events in the mainloop context.
     *
     * @param[in]  aMainloop  The mainloop context to be updated.
     *
     */
    void Process(const otSysMainloopContext *aMainloop);

    /**
     * This method handles Thread network events.
     *
     * @param[in]  aFlags  The OpenThread event flags.
     *
     */
    void HandleStateChanged(otChangedFlags aFlags);

private:
    static constexpr uint16_t kKeyOmrPrefix = 0xFF01; ///< The OMR prefix key in settings.

    static constexpr uint32_t kMinRtrAdvInterval        = 8;  // 30;     // In Seconds.
    static constexpr uint32_t kMaxRtrAdvInterval        = 18; // 1800;   // In Seconds.
    static constexpr uint32_t kMaxInitRtrAdvInterval    = 16; // In Seconds.
    static constexpr uint32_t kMaxInitRtrAdvertisements = 3;
    static constexpr uint32_t kRtrSolicitionInterval    = 4; // In Seconds.

    /**
     * This method starts the routing manager.
     *
     */
    void Start();

    /**
     * This method stops the routing manager.
     *
     */
    void Stop();

    /**
     * This method generates a random OMR prefix.
     *
     * @return  The random-generated OMR prefix.
     *
     */
    static otIp6Prefix GenerateRandomOmrPrefix();

    /**
     * This method decides if given prefix is a valid OMR prefix.
     *
     * @param[in]  aPrefix  An IPv6 prefix.
     *
     * @return  a boolean indicates whether the prefix is a valid OMR prefix.
     *
     */
    static bool IsValidOmrPrefix(const otIp6Prefix &aPrefix);

    /**
     * This method decides if given prefix is a valid on-link prefix.
     *
     * @param[in]  aPrefix  An IPv6 prefix.
     *
     * @return  a boolean indicates whether the prefix is a valid on-link prefix.
     *
     */
    static bool IsValidOnLinkPrefix(const otIp6Prefix &aPrefix);

    /**
     * This method evaluate the routing policy depends on prefix and route
     * information on Thread Network and infra link. As a result, this
     * method May send RA messages on infra link and publish/unpublish
     * OMR prefix in the Thread Network.
     *
     * @see EvaluateOmrPrefix
     * @see EvaluateOnLinkPrefix
     * @see PublishOmrPrefix
     * @see UnpublishOmrPrefix
     *
     */
    void EvaluateRoutingPolicy();

    /**
     * This method evaluates the OMR prefix for the Thread Network.
     *
     * @return  The new OMR prefix. An invalid OMR prefix means we should
     *          no longer publish a OMR in the Thread network.
     *
     */
    otIp6Prefix EvaluateOmrPrefix();

    /**
     * This method evaluates the on-link prefix for the infra link.
     *
     * @return  The new on-link prefix. An invalid on-link prefix means we
     *          should no longer advertise a on-link prefix on the infra link.
     *
     */
    otIp6Prefix EvaluateOnLinkPrefix();

    /**
     * This method publishes an OMR prefix in Thread network.
     *
     * @param[in]  aPrefix  An OMR prefix.
     *
     */
    void PublishOmrPrefix(const otIp6Prefix &aPrefix);

    /**
     * This method unpublishes an OMR prefix if we have done that.
     *
     * @param[in]  aPrefix  An OMR prefix.
     *
     */
    void UnpublishOmrPrefix(const otIp6Prefix &aPrefix);

    /**
     * This method sends Router Solicitation messages to discovery on-link
     * prefix on infra links.
     *
     * @see HandleRouterAdvertisement
     *
     */
    void SendRouterSolicit();

    /**
     * This method sends Router Advertisement messages to advertise
     * on-link prefix and route for OMR prefix.
     *
     * @param[in]  aOmrPrefix     An OMR prefix.
     * @param[in]  aOnLinkPrefix  An on-link prefix.
     *
     */
    void SendRouterAdvertisement(const otIp6Prefix &aOmrPrefix, const otIp6Prefix &aOnLinkPrefix);

    static void HandleRouterAdvertisementTimer(Timer &aTimer, void *aRouterManager);
    void        HandleRouterAdvertisementTimer(Timer &aTimer);

    static void HandleRouterSolicitTimer(Timer &aTimer, void *aRouterManager);
    void        HandleRouterSolicitTimer(Timer &aTimer);

    static void HandleDiscoveredOnLinkPrefixInvalidTimer(Timer &aTimer, void *aRouterManager);
    void        HandleDiscoveredOnLinkPrefixInvalidTimer(Timer &aTimer);

    static void HandleRouterSolicit(unsigned int aIfIndex, void *aRouterManager);
    void        HandleRouterSolicit(unsigned int aIfIndex);
    static void HandleRouterAdvertisement(const RouterAdvertisement::RouterAdvMessage &aRouterAdv,
                                          unsigned int                                 aIfIndex,
                                          void *                                       aRouterManager);
    void HandleRouterAdvertisement(const RouterAdvertisement::RouterAdvMessage &aRouterAdv, unsigned int aIfIndex);

    static uint32_t GenerateRandomNumber(uint32_t aBegin, uint32_t aEnd);

    otInstance *mInstance;

    /**
     * The OMR prefix loaded from local persistent storage.
     *
     */
    otIp6Prefix mLocalOmrPrefix;

    /**
     * The OMR prefix selected to be advertised.
     *
     */
    otIp6Prefix mAdvertisedOmrPrefix;

    /**
     * The on-link prefix created based on the local OMR prefix.
     *
     */
    otIp6Prefix mLocalOnLinkPrefix;

    /**
     * The on-link prefix selected to be advertised.
     *
     */
    otIp6Prefix mAdvertisedOnLinkPrefix;

    /**
     * The on-link prefix we discvered on the infra link by
     * sending Router Solicitations.
     *
     */
    otIp6Prefix mDiscoveredOnLinkPrefix;

    InfraNetif                            mInfraNetif;
    RouterAdvertisement::RouterAdvertiser mRouterAdvertiser;

    Timer    mRouterAdvertisementTimer;
    uint32_t mRouterAdvertisementCount;

    Timer mRouterSolicitTimer;
    Timer mDiscoveredOnLinkPrefixInvalidTimer;
};

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // POSIX_ROUTING_MANAGER_HPP
