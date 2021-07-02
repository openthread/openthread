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
 *  This file defines the OpenThread SRP server API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#include <openthread/srp_server.h>

#include "common/instance.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

const char *otSrpServerGetDomain(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Server>().GetDomain();
}

otError otSrpServerSetDomain(otInstance *aInstance, const char *aDomain)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Server>().SetDomain(aDomain);
}

void otSrpServerSetEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Server>().SetEnabled(aEnabled);
}

void otSrpServerGetLeaseConfig(otInstance *aInstance, otSrpServerLeaseConfig *aLeaseConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Server>().GetLeaseConfig(static_cast<Srp::Server::LeaseConfig &>(*aLeaseConfig));
}

otError otSrpServerSetLeaseConfig(otInstance *aInstance, const otSrpServerLeaseConfig *aLeaseConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Server>().SetLeaseConfig(static_cast<const Srp::Server::LeaseConfig &>(*aLeaseConfig));
}

void otSrpServerSetServiceUpdateHandler(otInstance *                    aInstance,
                                        otSrpServerServiceUpdateHandler aServiceHandler,
                                        void *                          aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Server>().SetServiceHandler(aServiceHandler, aContext);
}

void otSrpServerHandleServiceUpdateResult(otInstance *aInstance, otSrpServerServiceUpdateId aId, otError aError)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Server>().HandleServiceUpdateResult(aId, aError);
}

const otSrpServerHost *otSrpServerGetNextHost(otInstance *aInstance, const otSrpServerHost *aHost)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Server>().GetNextHost(static_cast<const Srp::Server::Host *>(aHost));
}

bool otSrpServerHostIsDeleted(const otSrpServerHost *aHost)
{
    return static_cast<const Srp::Server::Host *>(aHost)->IsDeleted();
}

const char *otSrpServerHostGetFullName(const otSrpServerHost *aHost)
{
    return static_cast<const Srp::Server::Host *>(aHost)->GetFullName();
}

const otIp6Address *otSrpServerHostGetAddresses(const otSrpServerHost *aHost, uint8_t *aAddressesNum)
{
    auto host = static_cast<const Srp::Server::Host *>(aHost);

    return host->GetAddresses(*aAddressesNum);
}

const otSrpServerService *otSrpServerHostGetNextService(const otSrpServerHost *   aHost,
                                                        const otSrpServerService *aService)
{
    auto host = static_cast<const Srp::Server::Host *>(aHost);

    return host->FindNextService(static_cast<const Srp::Server::Service *>(aService),
                                 Srp::Server::kFlagsBaseTypeServiceOnly);
}

const otSrpServerService *otSrpServerHostFindNextService(const otSrpServerHost *   aHost,
                                                         const otSrpServerService *aPrevService,
                                                         otSrpServerServiceFlags   aFlags,
                                                         const char *              aServiceName,
                                                         const char *              aInstanceName)
{
    auto host = static_cast<const Srp::Server::Host *>(aHost);

    return host->FindNextService(static_cast<const Srp::Server::Service *>(aPrevService), aFlags, aServiceName,
                                 aInstanceName);
}

bool otSrpServerServiceIsDeleted(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->IsDeleted();
}

bool otSrpServerServiceIsSubType(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->IsSubType();
}

const char *otSrpServerServiceGetFullName(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetInstanceName();
}

const char *otSrpServerServiceGetInstanceName(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetInstanceName();
}

const char *otSrpServerServiceGetServiceName(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetServiceName();
}

otError otSrpServerServiceGetServiceSubTypeLabel(const otSrpServerService *aService, char *aLabel, uint8_t aMaxSize)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetServiceSubTypeLabel(aLabel, aMaxSize);
}

uint16_t otSrpServerServiceGetPort(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetPort();
}

uint16_t otSrpServerServiceGetWeight(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetWeight();
}

uint16_t otSrpServerServiceGetPriority(const otSrpServerService *aService)
{
    return static_cast<const Srp::Server::Service *>(aService)->GetPriority();
}

const uint8_t *otSrpServerServiceGetTxtData(const otSrpServerService *aService, uint16_t *aDataLength)
{
    const Srp::Server::Service &service = *static_cast<const Srp::Server::Service *>(aService);

    *aDataLength = service.GetTxtDataLength();

    return service.GetTxtData();
}

const otSrpServerHost *otSrpServerServiceGetHost(const otSrpServerService *aService)
{
    return &static_cast<const Srp::Server::Service *>(aService)->GetHost();
}

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
