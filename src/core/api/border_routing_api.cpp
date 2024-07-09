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

#include "instance/instance.hpp"

using namespace ot;

otError otBorderRoutingInit(otInstance *aInstance, uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().Init(aInfraIfIndex, aInfraIfIsRunning);
}

otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetEnabled(aEnabled);
}

otBorderRoutingState otBorderRoutingGetState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetState());
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

void otBorderRoutingClearRouteInfoOptionPreference(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().ClearRouteInfoOptionPreference();
}

otError otBorderRoutingSetExtraRouterAdvertOptions(otInstance *aInstance, const uint8_t *aOptions, uint16_t aLength)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetExtraRouterAdvertOptions(aOptions, aLength);
}

otRoutePreference otBorderRoutingGetRoutePreference(otInstance *aInstance)
{
    return static_cast<otRoutePreference>(
        AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetRoutePreference());
}

void otBorderRoutingSetRoutePreference(otInstance *aInstance, otRoutePreference aPreference)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetRoutePreference(
        static_cast<NetworkData::RoutePreference>(aPreference));
}

void otBorderRoutingClearRoutePreference(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().ClearRoutePreference();
}

otError otBorderRoutingGetOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetOmrPrefix(AsCoreType(aPrefix));
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
otError otBorderRoutingGetPdOmrPrefix(otInstance *aInstance, otBorderRoutingPrefixTableEntry *aPrefixInfo)
{
    AssertPointerIsNotNull(aPrefixInfo);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetPdOmrPrefix(*aPrefixInfo);
}

otError otBorderRoutingGetPdProcessedRaInfo(otInstance *aInstance, otPdProcessedRaInfo *aPdProcessedRaInfo)
{
    AssertPointerIsNotNull(aPdProcessedRaInfo);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetPdProcessedRaInfo(*aPdProcessedRaInfo);
}
#endif

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

otError otBorderRoutingGetFavoredOnLinkPrefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetFavoredOnLinkPrefix(AsCoreType(aPrefix));
}

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
otError otBorderRoutingGetNat64Prefix(otInstance *aInstance, otIp6Prefix *aPrefix)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNat64Prefix(AsCoreType(aPrefix));
}

otError otBorderRoutingGetFavoredNat64Prefix(otInstance        *aInstance,
                                             otIp6Prefix       *aPrefix,
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

otError otBorderRoutingGetNextPrefixTableEntry(otInstance                         *aInstance,
                                               otBorderRoutingPrefixTableIterator *aIterator,
                                               otBorderRoutingPrefixTableEntry    *aEntry)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aEntry);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNextPrefixTableEntry(*aIterator, *aEntry);
}

otError otBorderRoutingGetNextRouterEntry(otInstance                         *aInstance,
                                          otBorderRoutingPrefixTableIterator *aIterator,
                                          otBorderRoutingRouterEntry         *aEntry)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aEntry);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNextRouterEntry(*aIterator, *aEntry);
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

otError otBorderRoutingGetNextPeerBrEntry(otInstance                           *aInstance,
                                          otBorderRoutingPrefixTableIterator   *aIterator,
                                          otBorderRoutingPeerBorderRouterEntry *aEntry)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aEntry);

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNextPeerBrEntry(*aIterator, *aEntry);
}

uint16_t otBorderRoutingCountPeerBrs(otInstance *aInstance, uint32_t *aMinAge)
{
    uint32_t minAge;

    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().CountPeerBrs((aMinAge != nullptr) ? *aMinAge
                                                                                                       : minAge);
}

#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

void otBorderRoutingDhcp6PdSetEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetDhcp6PdEnabled(aEnabled);
}

otBorderRoutingDhcp6PdState otBorderRoutingDhcp6PdGetState(otInstance *aInstance)
{
    return static_cast<otBorderRoutingDhcp6PdState>(
        AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetDhcp6PdState());
}

void otBorderRoutingDhcp6PdSetRequestCallback(otInstance                           *aInstance,
                                              otBorderRoutingRequestDhcp6PdCallback aCallback,
                                              void                                 *aContext)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetRequestDhcp6PdCallback(aCallback, aContext);
}

#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TESTING_API_ENABLE

void otBorderRoutingSetOnLinkPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetOnLinkPrefix(AsCoreType(aPrefix));
}

#endif

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
