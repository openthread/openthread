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
 *   This file implements the OpenThread SRP client APIs.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#include <openthread/srp_client.h>

#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "net/srp_client.hpp"

using namespace ot;

otError otSrpClientStart(otInstance *aInstance, const otSockAddr *aServerSockAddr)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().Start(*static_cast<const Ip6::SockAddr *>(aServerSockAddr));
}

void otSrpClientStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().Stop();
}

bool otSrpClientIsRunning(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().IsRunning();
}

const otSockAddr *otSrpClientGetServerAddress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Srp::Client>().GetServerAddress();
}

void otSrpClientSetCallback(otInstance *aInstance, otSrpClientCallback aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Client>().SetCallback(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
void otSrpClientEnableAutoStartMode(otInstance *aInstance, otSrpClientAutoStartCallback aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Client>().EnableAutoStartMode(aCallback, aContext);
}

void otSrpClientDisableAutoStartMode(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Client>().DisableAutoStartMode();
}

bool otSrpClientIsAutoStartModeEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().IsAutoStartModeEnabled();
}
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

uint32_t otSrpClientGetLeaseInterval(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().GetLeaseInterval();
}

void otSrpClientSetLeaseInterval(otInstance *aInstance, uint32_t aInterval)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().SetLeaseInterval(aInterval);
}

uint32_t otSrpClientGetKeyLeaseInterval(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().GetKeyLeaseInterval();
}

void otSrpClientSetKeyLeaseInterval(otInstance *aInstance, uint32_t aInterval)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().SetKeyLeaseInterval(aInterval);
}

const otSrpClientHostInfo *otSrpClientGetHostInfo(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Srp::Client>().GetHostInfo();
}

otError otSrpClientSetHostName(otInstance *aInstance, const char *aName)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().SetHostName(aName);
}

otError otSrpClientSetHostAddresses(otInstance *aInstance, const otIp6Address *aIp6Addresses, uint8_t aNumAddresses)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().SetHostAddresses(static_cast<const Ip6::Address *>(aIp6Addresses),
                                                        aNumAddresses);
}

otError otSrpClientAddService(otInstance *aInstance, otSrpClientService *aService)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().AddService(*static_cast<Srp::Client::Service *>(aService));
}

otError otSrpClientRemoveService(otInstance *aInstance, otSrpClientService *aService)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().RemoveService(*static_cast<Srp::Client::Service *>(aService));
}

otError otSrpClientClearService(otInstance *aInstance, otSrpClientService *aService)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().ClearService(*static_cast<Srp::Client::Service *>(aService));
}

const otSrpClientService *otSrpClientGetServices(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().GetServices().GetHead();
}

otError otSrpClientRemoveHostAndServices(otInstance *aInstance, bool aRemoveKeyLease)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().RemoveHostAndServices(aRemoveKeyLease);
}

void otSrpClientClearHostAndServices(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Client>().ClearHostAndServices();
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
const char *otSrpClientGetDomainName(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().GetDomainName();
}

otError otSrpClientSetDomainName(otInstance *aInstance, const char *aDomainName)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().SetDomainName(aDomainName);
}
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE

const char *otSrpClientItemStateToString(otSrpClientItemState aItemState)
{
    OT_ASSERT(aItemState <= OT_SRP_CLIENT_ITEM_STATE_REMOVED);

    return Srp::Client::ItemStateToString(static_cast<Srp::Client::ItemState>(aItemState));
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void otSrpClientSetServiceKeyRecordEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Srp::Client>().SetServiceKeyRecordEnabled(aEnabled);
}

bool otSrpClientIsServiceKeyRecordEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Srp::Client>().IsServiceKeyRecordEnabled();
}
#endif

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
