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

#include <openthread/instance.h>
#include <openthread/platform/dnssd.h>

#include "ncp/ncp_base.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE && OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
otPlatDnssdState otPlatDnssdGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return ot::Ncp::NcpBase::GetNcpInstance()->DnssdGetState();
}

void otPlatDnssdRegisterService(otInstance                 *aInstance,
                                const otPlatDnssdService   *aService,
                                otPlatDnssdRequestId        aRequestId,
                                otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdRegisterService(aService, aRequestId, aCallback);
}

void otPlatDnssdUnregisterService(otInstance                 *aInstance,
                                  const otPlatDnssdService   *aService,
                                  otPlatDnssdRequestId        aRequestId,
                                  otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdUnregisterService(aService, aRequestId, aCallback);
}

void otPlatDnssdRegisterHost(otInstance                 *aInstance,
                             const otPlatDnssdHost      *aHost,
                             otPlatDnssdRequestId        aRequestId,
                             otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdRegisterHost(aHost, aRequestId, aCallback);
}

void otPlatDnssdUnregisterHost(otInstance                 *aInstance,
                               const otPlatDnssdHost      *aHost,
                               otPlatDnssdRequestId        aRequestId,
                               otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdUnregisterHost(aHost, aRequestId, aCallback);
}

void otPlatDnssdRegisterKey(otInstance                 *aInstance,
                            const otPlatDnssdKey       *aKey,
                            otPlatDnssdRequestId        aRequestId,
                            otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdRegisterKey(aKey, aRequestId, aCallback);
}

void otPlatDnssdUnregisterKey(otInstance                 *aInstance,
                              const otPlatDnssdKey       *aKey,
                              otPlatDnssdRequestId        aRequestId,
                              otPlatDnssdRegisterCallback aCallback)
{
    OT_UNUSED_VARIABLE(aInstance);
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    ncp->DnssdUnregisterKey(aKey, aRequestId, aCallback);
}

void otPlatDnssdStartBrowser(otInstance *aInstance, const otPlatDnssdBrowser *aBrowser)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aBrowser);
}

void otPlatDnssdStopBrowser(otInstance *aInstance, const otPlatDnssdBrowser *aBrowser)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aBrowser);
}

void otPlatDnssdStartSrvResolver(otInstance *aInstance, const otPlatDnssdSrvResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStopSrvResolver(otInstance *aInstance, const otPlatDnssdSrvResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStartTxtResolver(otInstance *aInstance, const otPlatDnssdTxtResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStopTxtResolver(otInstance *aInstance, const otPlatDnssdTxtResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStartIp6AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStopIp6AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStartIp4AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStopIp4AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResolver);
}

void otPlatDnssdStartRecordQuerier(otInstance *aInstance, const otPlatDnssdRecordQuerier *aQuerier)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aQuerier);
}

void otPlatDnssdStopRecordQuerier(otInstance *aInstance, const otPlatDnssdRecordQuerier *aQuerier)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aQuerier);
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE && OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
