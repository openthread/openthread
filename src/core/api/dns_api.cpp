/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread DNSv6 API.
 */

#include "openthread-core-config.h"

#include <openthread/dns_client.h>

#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "net/dns_client.hpp"
#include "net/dns_types.hpp"

using namespace ot;

void otDnsInitTxtEntryIterator(otDnsTxtEntryIterator *aIterator, const uint8_t *aTxtData, uint16_t aTxtDataLength)
{
    static_cast<Dns::TxtEntry::Iterator *>(aIterator)->Init(aTxtData, aTxtDataLength);
}

otError otDnsGetNextTxtEntry(otDnsTxtEntryIterator *aIterator, otDnsTxtEntry *aEntry)
{
    return static_cast<Dns::TxtEntry::Iterator *>(aIterator)->GetNextEntry(*static_cast<Dns::TxtEntry *>(aEntry));
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void otDnsSetNameCompressionEnabled(bool aEnabled)
{
    Instance::SetDnsNameCompressionEnabled(aEnabled);
}

bool otDnsIsNameCompressionEnabled(void)
{
    return Instance::IsDnsNameCompressionEnabled();
}
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

const otDnsQueryConfig *otDnsClientGetDefaultConfig(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Dns::Client>().GetDefaultConfig();
}

void otDnsClientSetDefaultConfig(otInstance *aInstance, const otDnsQueryConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aConfig != nullptr)
    {
        instance.Get<Dns::Client>().SetDefaultConfig(*static_cast<const Dns::Client::QueryConfig *>(aConfig));
    }
    else
    {
        instance.Get<Dns::Client>().ResetDefaultConfig();
    }
}

otError otDnsClientResolveAddress(otInstance *            aInstance,
                                  const char *            aHostName,
                                  otDnsAddressCallback    aCallback,
                                  void *                  aContext,
                                  const otDnsQueryConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Dns::Client>().ResolveAddress(aHostName, aCallback, aContext,
                                                      static_cast<const Dns::Client::QueryConfig *>(aConfig));
}

otError otDnsAddressResponseGetHostName(const otDnsAddressResponse *aResponse,
                                        char *                      aNameBuffer,
                                        uint16_t                    aNameBufferSize)
{
    const Dns::Client::AddressResponse &response = *static_cast<const Dns::Client::AddressResponse *>(aResponse);

    return response.GetHostName(aNameBuffer, aNameBufferSize);
}

otError otDnsAddressResponseGetAddress(const otDnsAddressResponse *aResponse,
                                       uint16_t                    aIndex,
                                       otIp6Address *              aAddress,
                                       uint32_t *                  aTtl)
{
    const Dns::Client::AddressResponse &response = *static_cast<const Dns::Client::AddressResponse *>(aResponse);
    uint32_t                            ttl;

    return response.GetAddress(aIndex, *static_cast<Ip6::Address *>(aAddress), (aTtl != nullptr) ? *aTtl : ttl);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

otError otDnsClientBrowse(otInstance *            aInstance,
                          const char *            aServiceName,
                          otDnsBrowseCallback     aCallback,
                          void *                  aContext,
                          const otDnsQueryConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Dns::Client>().Browse(aServiceName, aCallback, aContext,
                                              static_cast<const Dns::Client::QueryConfig *>(aConfig));
}

otError otDnsBrowseResponseGetServiceName(const otDnsBrowseResponse *aResponse,
                                          char *                     aNameBuffer,
                                          uint16_t                   aNameBufferSize)
{
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);

    return response.GetServiceName(aNameBuffer, aNameBufferSize);
}

otError otDnsBrowseResponseGetServiceInstance(const otDnsBrowseResponse *aResponse,
                                              uint16_t                   aIndex,
                                              char *                     aLabelBuffer,
                                              uint8_t                    aLabelBufferSize)
{
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);

    return response.GetServiceInstance(aIndex, aLabelBuffer, aLabelBufferSize);
}

otError otDnsBrowseResponseGetServiceInfo(const otDnsBrowseResponse *aResponse,
                                          const char *               aInstanceLabel,
                                          otDnsServiceInfo *         aServiceInfo)
{
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);

    return response.GetServiceInfo(aInstanceLabel, *static_cast<Dns::Client::ServiceInfo *>(aServiceInfo));
}

otError otDnsBrowseResponseGetHostAddress(const otDnsBrowseResponse *aResponse,
                                          const char *               aHostName,
                                          uint16_t                   aIndex,
                                          otIp6Address *             aAddress,
                                          uint32_t *                 aTtl)
{
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);
    uint32_t                           ttl;

    return response.GetHostAddress(aHostName, aIndex, *static_cast<Ip6::Address *>(aAddress),
                                   aTtl != nullptr ? *aTtl : ttl);
}

otError otDnsClientResolveService(otInstance *            aInstance,
                                  const char *            aInstanceLabel,
                                  const char *            aServiceName,
                                  otDnsServiceCallback    aCallback,
                                  void *                  aContext,
                                  const otDnsQueryConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Dns::Client>().ResolveService(aInstanceLabel, aServiceName, aCallback, aContext,
                                                      static_cast<const Dns::Client::QueryConfig *>(aConfig));
}

otError otDnsServiceResponseGetServiceName(const otDnsServiceResponse *aResponse,
                                           char *                      aLabelBuffer,
                                           uint8_t                     aLabelBufferSize,
                                           char *                      aNameBuffer,
                                           uint16_t                    aNameBufferSize)
{
    const Dns::Client::ServiceResponse &response = *static_cast<const Dns::Client::ServiceResponse *>(aResponse);

    return response.GetServiceName(aLabelBuffer, aLabelBufferSize, aNameBuffer, aNameBufferSize);
}

otError otDnsServiceResponseGetServiceInfo(const otDnsServiceResponse *aResponse, otDnsServiceInfo *aServiceInfo)
{
    const Dns::Client::ServiceResponse &response = *static_cast<const Dns::Client::ServiceResponse *>(aResponse);

    return response.GetServiceInfo(*static_cast<Dns::Client::ServiceInfo *>(aServiceInfo));
}

otError otDnsServiceResponseGetHostAddress(const otDnsServiceResponse *aResponse,
                                           const char *                aHostName,
                                           uint16_t                    aIndex,
                                           otIp6Address *              aAddress,
                                           uint32_t *                  aTtl)
{
    const Dns::Client::ServiceResponse &response = *static_cast<const Dns::Client::ServiceResponse *>(aResponse);
    uint32_t                            ttl;

    return response.GetHostAddress(aHostName, aIndex, *static_cast<Ip6::Address *>(aAddress),
                                   (aTtl != nullptr) ? *aTtl : ttl);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
