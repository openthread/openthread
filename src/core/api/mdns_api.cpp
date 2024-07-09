/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the OpenThread mDNS API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE

#include "instance/instance.hpp"

using namespace ot;

otError otMdnsSetEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().SetEnabled(aEnable, aInfraIfIndex);
}

bool otMdnsIsEnabled(otInstance *aInstance) { return AsCoreType(aInstance).Get<Dns::Multicast::Core>().IsEnabled(); }

void otMdnsSetQuestionUnicastAllowed(otInstance *aInstance, bool aAllow)
{
    AsCoreType(aInstance).Get<Dns::Multicast::Core>().SetQuestionUnicastAllowed(aAllow);
}

bool otMdnsIsQuestionUnicastAllowed(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().IsQuestionUnicastAllowed();
}

void otMdnsSetConflictCallback(otInstance *aInstance, otMdnsConflictCallback aCallback)
{
    AsCoreType(aInstance).Get<Dns::Multicast::Core>().SetConflictCallback(aCallback);
}

otError otMdnsRegisterHost(otInstance            *aInstance,
                           const otMdnsHost      *aHost,
                           otMdnsRequestId        aRequestId,
                           otMdnsRegisterCallback aCallback)
{
    AssertPointerIsNotNull(aHost);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().RegisterHost(*aHost, aRequestId, aCallback);
}

otError otMdnsUnregisterHost(otInstance *aInstance, const otMdnsHost *aHost)
{
    AssertPointerIsNotNull(aHost);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().UnregisterHost(*aHost);
}

otError otMdnsRegisterService(otInstance            *aInstance,
                              const otMdnsService   *aService,
                              otMdnsRequestId        aRequestId,
                              otMdnsRegisterCallback aCallback)
{
    AssertPointerIsNotNull(aService);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().RegisterService(*aService, aRequestId, aCallback);
}

otError otMdnsUnregisterService(otInstance *aInstance, const otMdnsService *aService)
{
    AssertPointerIsNotNull(aService);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().UnregisterService(*aService);
}

otError otMdnsRegisterKey(otInstance            *aInstance,
                          const otMdnsKey       *aKey,
                          otMdnsRequestId        aRequestId,
                          otMdnsRegisterCallback aCallback)
{
    AssertPointerIsNotNull(aKey);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().RegisterKey(*aKey, aRequestId, aCallback);
}

otError otMdnsUnregisterKey(otInstance *aInstance, const otMdnsKey *aKey)
{
    AssertPointerIsNotNull(aKey);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().UnregisterKey(*aKey);
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

otMdnsIterator *otMdnsAllocateIterator(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().AllocateIterator();
}

void otMdnsFreeIterator(otInstance *aInstance, otMdnsIterator *aIterator)
{
    AssertPointerIsNotNull(aIterator);

    AsCoreType(aInstance).Get<Dns::Multicast::Core>().FreeIterator(*aIterator);
}

otError otMdnsGetNextHost(otInstance *aInstance, otMdnsIterator *aIterator, otMdnsHost *aHost, otMdnsEntryState *aState)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aHost);
    AssertPointerIsNotNull(aState);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextHost(*aIterator, *aHost, *aState);
}

otError otMdnsGetNextService(otInstance       *aInstance,
                             otMdnsIterator   *aIterator,
                             otMdnsService    *aService,
                             otMdnsEntryState *aState)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aService);
    AssertPointerIsNotNull(aState);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextService(*aIterator, *aService, *aState);
}

otError otMdnsGetNextKey(otInstance *aInstance, otMdnsIterator *aIterator, otMdnsKey *aKey, otMdnsEntryState *aState)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aKey);
    AssertPointerIsNotNull(aState);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextKey(*aIterator, *aKey, *aState);
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

otError otMdnsStartBrowser(otInstance *aInstance, const otMdnsBrowser *aBroswer)
{
    AssertPointerIsNotNull(aBroswer);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StartBrowser(*aBroswer);
}

otError otMdnsStopBrowser(otInstance *aInstance, const otMdnsBrowser *aBroswer)
{
    AssertPointerIsNotNull(aBroswer);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StopBrowser(*aBroswer);
}

otError otMdnsStartSrvResolver(otInstance *aInstance, const otMdnsSrvResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StartSrvResolver(*aResolver);
}

otError otMdnsStopSrvResolver(otInstance *aInstance, const otMdnsSrvResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StopSrvResolver(*aResolver);
}

otError otMdnsStartTxtResolver(otInstance *aInstance, const otMdnsTxtResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StartTxtResolver(*aResolver);
}

otError otMdnsStopTxtResolver(otInstance *aInstance, const otMdnsTxtResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StopTxtResolver(*aResolver);
}

otError otMdnsStartIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StartIp6AddressResolver(*aResolver);
}

otError otMdnsStopIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StopIp6AddressResolver(*aResolver);
}

otError otMdnsStartIp4AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StartIp4AddressResolver(*aResolver);
}

otError otMdnsStopIp4AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver)
{
    AssertPointerIsNotNull(aResolver);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().StopIp4AddressResolver(*aResolver);
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

otError otMdnsGetNextBrowser(otInstance      *aInstance,
                             otMdnsIterator  *aIterator,
                             otMdnsBrowser   *aBrowser,
                             otMdnsCacheInfo *aInfo)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aBrowser);
    AssertPointerIsNotNull(aInfo);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextBrowser(*aIterator, *aBrowser, *aInfo);
}

otError otMdnsGetNextSrvResolver(otInstance        *aInstance,
                                 otMdnsIterator    *aIterator,
                                 otMdnsSrvResolver *aResolver,
                                 otMdnsCacheInfo   *aInfo)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aResolver);
    AssertPointerIsNotNull(aInfo);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextSrvResolver(*aIterator, *aResolver, *aInfo);
}

otError otMdnsGetNextTxtResolver(otInstance        *aInstance,
                                 otMdnsIterator    *aIterator,
                                 otMdnsTxtResolver *aResolver,
                                 otMdnsCacheInfo   *aInfo)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aResolver);
    AssertPointerIsNotNull(aInfo);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextTxtResolver(*aIterator, *aResolver, *aInfo);
}

otError otMdnsGetNextIp6AddressResolver(otInstance            *aInstance,
                                        otMdnsIterator        *aIterator,
                                        otMdnsAddressResolver *aResolver,
                                        otMdnsCacheInfo       *aInfo)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aResolver);
    AssertPointerIsNotNull(aInfo);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextIp6AddressResolver(*aIterator, *aResolver, *aInfo);
}

otError otMdnsGetNextIp4AddressResolver(otInstance            *aInstance,
                                        otMdnsIterator        *aIterator,
                                        otMdnsAddressResolver *aResolver,
                                        otMdnsCacheInfo       *aInfo)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aResolver);
    AssertPointerIsNotNull(aInfo);

    return AsCoreType(aInstance).Get<Dns::Multicast::Core>().GetNextIp4AddressResolver(*aIterator, *aResolver, *aInfo);
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
