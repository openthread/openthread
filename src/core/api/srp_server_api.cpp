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

#include "instance/instance.hpp"

using namespace ot;

const char *otSrpServerGetDomain(otInstance *aInstance) { return AsCoreType(aInstance).Get<Srp::Server>().GetDomain(); }

otError otSrpServerSetDomain(otInstance *aInstance, const char *aDomain)
{
    return AsCoreType(aInstance).Get<Srp::Server>().SetDomain(aDomain);
}

otSrpServerState otSrpServerGetState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<Srp::Server>().GetState());
}

uint16_t otSrpServerGetPort(otInstance *aInstance) { return AsCoreType(aInstance).Get<Srp::Server>().GetPort(); }

otSrpServerAddressMode otSrpServerGetAddressMode(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<Srp::Server>().GetAddressMode());
}

otError otSrpServerSetAddressMode(otInstance *aInstance, otSrpServerAddressMode aMode)
{
    return AsCoreType(aInstance).Get<Srp::Server>().SetAddressMode(MapEnum(aMode));
}

uint8_t otSrpServerGetAnycastModeSequenceNumber(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Srp::Server>().GetAnycastModeSequenceNumber();
}

otError otSrpServerSetAnycastModeSequenceNumber(otInstance *aInstance, uint8_t aSequenceNumber)
{
    return AsCoreType(aInstance).Get<Srp::Server>().SetAnycastModeSequenceNumber(aSequenceNumber);
}

void otSrpServerSetEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<Srp::Server>().SetEnabled(aEnabled);
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void otSrpServerSetAutoEnableMode(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<Srp::Server>().SetAutoEnableMode(aEnabled);
}

bool otSrpServerIsAutoEnableMode(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Srp::Server>().IsAutoEnableMode();
}
#endif

void otSrpServerGetTtlConfig(otInstance *aInstance, otSrpServerTtlConfig *aTtlConfig)
{
    AsCoreType(aInstance).Get<Srp::Server>().GetTtlConfig(AsCoreType(aTtlConfig));
}

otError otSrpServerSetTtlConfig(otInstance *aInstance, const otSrpServerTtlConfig *aTtlConfig)
{
    return AsCoreType(aInstance).Get<Srp::Server>().SetTtlConfig(AsCoreType(aTtlConfig));
}

void otSrpServerGetLeaseConfig(otInstance *aInstance, otSrpServerLeaseConfig *aLeaseConfig)
{
    AsCoreType(aInstance).Get<Srp::Server>().GetLeaseConfig(AsCoreType(aLeaseConfig));
}

otError otSrpServerSetLeaseConfig(otInstance *aInstance, const otSrpServerLeaseConfig *aLeaseConfig)
{
    return AsCoreType(aInstance).Get<Srp::Server>().SetLeaseConfig(AsCoreType(aLeaseConfig));
}

void otSrpServerSetServiceUpdateHandler(otInstance                     *aInstance,
                                        otSrpServerServiceUpdateHandler aServiceHandler,
                                        void                           *aContext)
{
    AsCoreType(aInstance).Get<Srp::Server>().SetServiceHandler(aServiceHandler, aContext);
}

void otSrpServerHandleServiceUpdateResult(otInstance *aInstance, otSrpServerServiceUpdateId aId, otError aError)
{
    AsCoreType(aInstance).Get<Srp::Server>().HandleServiceUpdateResult(aId, aError);
}

const otSrpServerHost *otSrpServerGetNextHost(otInstance *aInstance, const otSrpServerHost *aHost)
{
    return AsCoreType(aInstance).Get<Srp::Server>().GetNextHost(AsCoreTypePtr(aHost));
}

const otSrpServerResponseCounters *otSrpServerGetResponseCounters(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Srp::Server>().GetResponseCounters();
}

bool otSrpServerHostIsDeleted(const otSrpServerHost *aHost) { return AsCoreType(aHost).IsDeleted(); }

const char *otSrpServerHostGetFullName(const otSrpServerHost *aHost) { return AsCoreType(aHost).GetFullName(); }

bool otSrpServerHostMatchesFullName(const otSrpServerHost *aHost, const char *aFullName)
{
    return AsCoreType(aHost).Matches(aFullName);
}

const otIp6Address *otSrpServerHostGetAddresses(const otSrpServerHost *aHost, uint8_t *aAddressesNum)
{
    return AsCoreType(aHost).GetAddresses(*aAddressesNum);
}

void otSrpServerHostGetLeaseInfo(const otSrpServerHost *aHost, otSrpServerLeaseInfo *aLeaseInfo)
{
    AsCoreType(aHost).GetLeaseInfo(*aLeaseInfo);
}

uint32_t otSrpServerHostGetKeyLease(const otSrpServerHost *aHost) { return AsCoreType(aHost).GetKeyLease(); }

const otSrpServerService *otSrpServerHostGetNextService(const otSrpServerHost    *aHost,
                                                        const otSrpServerService *aService)
{
    return AsCoreType(aHost).GetNextService(AsCoreTypePtr(aService));
}

bool otSrpServerServiceIsDeleted(const otSrpServerService *aService) { return AsCoreType(aService).IsDeleted(); }

const char *otSrpServerServiceGetInstanceName(const otSrpServerService *aService)
{
    return AsCoreType(aService).GetInstanceName();
}

bool otSrpServerServiceMatchesInstanceName(const otSrpServerService *aService, const char *aInstanceName)
{
    return AsCoreType(aService).MatchesInstanceName(aInstanceName);
}

const char *otSrpServerServiceGetInstanceLabel(const otSrpServerService *aService)
{
    return AsCoreType(aService).GetInstanceLabel();
}

const char *otSrpServerServiceGetServiceName(const otSrpServerService *aService)
{
    return AsCoreType(aService).GetServiceName();
}

bool otSrpServerServiceMatchesServiceName(const otSrpServerService *aService, const char *aServiceName)
{
    return AsCoreType(aService).MatchesServiceName(aServiceName);
}

uint16_t otSrpServerServiceGetNumberOfSubTypes(const otSrpServerService *aService)
{
    return AsCoreType(aService).GetNumberOfSubTypes();
}

const char *otSrpServerServiceGetSubTypeServiceNameAt(const otSrpServerService *aService, uint16_t aIndex)
{
    return AsCoreType(aService).GetSubTypeServiceNameAt(aIndex);
}

bool otSrpServerServiceHasSubTypeServiceName(const otSrpServerService *aService, const char *aSubTypeServiceName)
{
    return AsCoreType(aService).HasSubTypeServiceName(aSubTypeServiceName);
}

otError otSrpServerParseSubTypeServiceName(const char *aSubTypeServiceName, char *aLabel, uint8_t aLabelSize)
{
    return Srp::Server::Service::ParseSubTypeServiceName(aSubTypeServiceName, aLabel, aLabelSize);
}

uint16_t otSrpServerServiceGetPort(const otSrpServerService *aService) { return AsCoreType(aService).GetPort(); }

uint16_t otSrpServerServiceGetWeight(const otSrpServerService *aService) { return AsCoreType(aService).GetWeight(); }

uint16_t otSrpServerServiceGetPriority(const otSrpServerService *aService)
{
    return AsCoreType(aService).GetPriority();
}

uint32_t otSrpServerServiceGetTtl(const otSrpServerService *aService) { return AsCoreType(aService).GetTtl(); }

const uint8_t *otSrpServerServiceGetTxtData(const otSrpServerService *aService, uint16_t *aDataLength)
{
    *aDataLength = AsCoreType(aService).GetTxtDataLength();

    return AsCoreType(aService).GetTxtData();
}

const otSrpServerHost *otSrpServerServiceGetHost(const otSrpServerService *aService)
{
    return &AsCoreType(aService).GetHost();
}

void otSrpServerServiceGetLeaseInfo(const otSrpServerService *aService, otSrpServerLeaseInfo *aLeaseInfo)
{
    AsCoreType(aService).GetLeaseInfo(*aLeaseInfo);
}

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
