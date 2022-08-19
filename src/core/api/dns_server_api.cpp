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
 *   This file implements the DNS-SD Server APIs.
 */

#include "openthread-core-config.h"

#include "common/instance.hpp"
#include "net/dns_types.hpp"
#include "net/dnssd_server.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

void otDnssdQuerySetCallbacks(otInstance *                    aInstance,
                              otDnssdQuerySubscribeCallback   aSubscribe,
                              otDnssdQueryUnsubscribeCallback aUnsubscribe,
                              void *                          aContext)
{
    AsCoreType(aInstance).Get<Dns::ServiceDiscovery::Server>().SetQueryCallbacks(aSubscribe, aUnsubscribe, aContext);
}

void otDnssdQueryHandleDiscoveredServiceInstance(otInstance *                aInstance,
                                                 const char *                aServiceFullName,
                                                 otDnssdServiceInstanceInfo *aInstanceInfo)
{
    AssertPointerIsNotNull(aServiceFullName);
    AssertPointerIsNotNull(aInstanceInfo);

    AsCoreType(aInstance).Get<Dns::ServiceDiscovery::Server>().HandleDiscoveredServiceInstance(aServiceFullName,
                                                                                               *aInstanceInfo);
}

void otDnssdQueryHandleDiscoveredHost(otInstance *aInstance, const char *aHostFullName, otDnssdHostInfo *aHostInfo)
{
    AssertPointerIsNotNull(aHostFullName);
    AssertPointerIsNotNull(aHostInfo);

    AsCoreType(aInstance).Get<Dns::ServiceDiscovery::Server>().HandleDiscoveredHost(aHostFullName, *aHostInfo);
}

const otDnssdQuery *otDnssdGetNextQuery(otInstance *aInstance, const otDnssdQuery *aQuery)
{
    return AsCoreType(aInstance).Get<Dns::ServiceDiscovery::Server>().GetNextQuery(aQuery);
}

otDnssdQueryType otDnssdGetQueryTypeAndName(const otDnssdQuery *aQuery, char (*aNameOutput)[OT_DNS_MAX_NAME_SIZE])
{
    AssertPointerIsNotNull(aQuery);
    AssertPointerIsNotNull(aNameOutput);

    return MapEnum(Dns::ServiceDiscovery::Server::GetQueryTypeAndName(aQuery, *aNameOutput));
}

const otDnssdCounters *otDnssdGetCounters(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Dns::ServiceDiscovery::Server>().GetCounters();
}

#endif // OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
