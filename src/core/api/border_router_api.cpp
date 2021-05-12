/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the OpenThread Border Router API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#include <openthread/border_router.h>

#include "border_router/routing_manager.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError otBorderRoutingInit(otInstance *aInstance, uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<BorderRouter::RoutingManager>().Init(aInfraIfIndex, aInfraIfIsRunning);
}

otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<BorderRouter::RoutingManager>().SetEnabled(aEnabled);
}
#endif

otError otBorderRouterGetNetData(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aData != nullptr && aDataLength != nullptr);

    return instance.Get<NetworkData::Local>().GetNetworkData(aStable, aData, *aDataLength);
}

otError otBorderRouterAddOnMeshPrefix(otInstance *aInstance, const otBorderRouterConfig *aConfig)
{
    Error                                  error    = kErrorNone;
    Instance &                             instance = *static_cast<Instance *>(aInstance);
    const NetworkData::OnMeshPrefixConfig *config   = static_cast<const NetworkData::OnMeshPrefixConfig *>(aConfig);

    OT_ASSERT(aConfig != nullptr);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (aConfig->mDp)
    {
        error = instance.Get<BackboneRouter::Local>().SetDomainPrefix(*config);
    }
    else
#endif
    {
        error = instance.Get<NetworkData::Local>().AddOnMeshPrefix(*config);
    }

    return error;
}

otError otBorderRouterRemoveOnMeshPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    Error              error    = kErrorNone;
    Instance &         instance = *static_cast<Instance *>(aInstance);
    const Ip6::Prefix *prefix   = static_cast<const Ip6::Prefix *>(aPrefix);

    OT_ASSERT(aPrefix != nullptr);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    error = instance.Get<BackboneRouter::Local>().RemoveDomainPrefix(*prefix);

    if (error == kErrorNotFound)
#endif
    {
        error = instance.Get<NetworkData::Local>().RemoveOnMeshPrefix(*prefix);
    }

    return error;
}

otError otBorderRouterGetNextOnMeshPrefix(otInstance *           aInstance,
                                          otNetworkDataIterator *aIterator,
                                          otBorderRouterConfig * aConfig)
{
    Instance &                       instance = *static_cast<Instance *>(aInstance);
    NetworkData::OnMeshPrefixConfig *config   = static_cast<NetworkData::OnMeshPrefixConfig *>(aConfig);

    OT_ASSERT(aIterator != nullptr && aConfig != nullptr);

    return instance.Get<NetworkData::Local>().GetNextOnMeshPrefix(*aIterator, *config);
}

otError otBorderRouterAddRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aConfig != nullptr);

    return instance.Get<NetworkData::Local>().AddHasRoutePrefix(
        *static_cast<const NetworkData::ExternalRouteConfig *>(aConfig));
}

otError otBorderRouterRemoveRoute(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aPrefix != nullptr);

    return instance.Get<NetworkData::Local>().RemoveHasRoutePrefix(*static_cast<const Ip6::Prefix *>(aPrefix));
}

otError otBorderRouterGetNextRoute(otInstance *           aInstance,
                                   otNetworkDataIterator *aIterator,
                                   otExternalRouteConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aIterator != nullptr && aConfig != nullptr);

    return instance.Get<NetworkData::Local>().GetNextExternalRoute(
        *aIterator, *static_cast<NetworkData::ExternalRouteConfig *>(aConfig));
}

otError otBorderRouterRegister(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    return kErrorNone;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
