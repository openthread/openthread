/*
 *  Copyright (c) 2021-22, The OpenThread Authors.
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
 *   This file implements the OpenThread Border Routing Manager API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <openthread/border_routing.h>

#include "border_router/routing_manager.hpp"
#include "common/instance.hpp"

using namespace ot;

otError otBorderRoutingInit(otInstance *aInstance, uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().Init(aInfraIfIndex, aInfraIfIsRunning);
}

otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetEnabled(aEnabled);
}

otRoutePreference otBorderRoutingGetRouteInfoOptionPreference(otInstance *aInstance)
{
    return static_cast<otRoutePreference>(
        AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetRouteInfoOptionPreference());
}

void otBorderRoutingSetRouteInfoOptionPreference(otInstance *aInstance, otRoutePreference aPreference)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetRouteInfoOptionPreference(
        static_cast<NetworkData::RoutePreference>(aPreference));
}

otError otBorderRoutingGetOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetOmrPrefix(AsCoreType(aPrefix));
}

otError otBorderRoutingGetFavoredOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix, otRoutePreference *aPreference)
{
    otError                                       error;
    BorderRouter::RoutingManager::RoutePreference preference;

    AssertPointerIsNotNull(aPreference);

    SuccessOrExit(error = AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetFavoredOmrPrefix(
                      AsCoreType(aPrefix), preference));
    *aPreference = static_cast<otRoutePreference>(preference);

exit:
    return error;
}

otError otBorderRoutingGetOnLinkPrefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(AsCoreType(aPrefix));
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
otError otBorderRoutingGetNat64Prefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNat64Prefix(AsCoreType(aPrefix));
}

otError otBorderRoutingGetFavoredNat64Prefix(otInstance *       aInstance,
                                             otIp6Prefix *      aPrefix,
                                             otRoutePreference *aPreference)
{
    otError                                       error;
    BorderRouter::RoutingManager::RoutePreference preference;

    AssertPointerIsNotNull(aPreference);

    SuccessOrExit(error = AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetFavoredNat64Prefix(
                      AsCoreType(aPrefix), preference));
    *aPreference = static_cast<otRoutePreference>(preference);

exit:
    return error;
}
#endif

void otBorderRoutingPrefixTableInitIterator(otInstance *aInstance, otBorderRoutingPrefixTableIterator *aIterator)
{
    AssertPointerIsNotNull(aIterator);

    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().InitPrefixTableIterator(*aIterator);
}

otError otBorderRoutingGetNextPrefixTableEntry(otInstance *                        aInstance,
                                               otBorderRoutingPrefixTableIterator *aIterator,
                                               otBorderRoutingPrefixTableEntry *   aEntry)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aEntry);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNextPrefixTableEntry(*aIterator, *aEntry);
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
