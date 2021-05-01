/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the OpenThread Network Data Publisher API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE

#include <openthread/netdata_publisher.h>

#include "common/instance.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

void otNetDataPublishDnsSrpServiceAnycast(otInstance *aInstance, uint8_t aSequenceNumber)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(aSequenceNumber);
}

void otNetDataPublishDnsSrpServiceUnicast(otInstance *aInstance, const otIp6Address *aAddress, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(*static_cast<const Ip6::Address *>(aAddress),
                                                                       aPort);
}

void otNetDataPublishDnsSrpServiceUnicastMeshLocalEid(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(aPort);
}

bool otNetDataIsDnsSrpServiceAdded(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().IsDnsSrpServiceAdded();
}

void otNetDataSetDnsSrpServicePublisherCallback(otInstance *                            aInstance,
                                                otNetDataDnsSrpServicePublisherCallback aCallback,
                                                void *                                  aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Publisher>().SetDnsSrpServiceCallback(aCallback, aContext);
}

void otNetDataUnpublishDnsSrpService(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Publisher>().UnpublishDnsSrpService();
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

otError otNetDataPublishOnMeshPrefix(otInstance *aInstance, const otBorderRouterConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().PublishOnMeshPrefix(
        *static_cast<const NetworkData::OnMeshPrefixConfig *>(aConfig));
}

otError otNetDataPublishExternalRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().PublishExternalRoute(
        *static_cast<const NetworkData::ExternalRouteConfig *>(aConfig));
}

bool otNetDataIsPrefixAdded(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().IsPrefixAdded(*static_cast<const Ip6::Prefix *>(aPrefix));
}

void otNetDataSetPrefixPublisherCallback(otInstance *                     aInstance,
                                         otNetDataPrefixPublisherCallback aCallback,
                                         void *                           aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().SetPrefixCallback(aCallback, aContext);
}

otError otNetDataUnpublishPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Publisher>().UnpublishPrefix(*static_cast<const Ip6::Prefix *>(aPrefix));
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#endif // OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
