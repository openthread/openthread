/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements infrastructure DNS-SD module.
 */

#include "dnssd.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#include "instance/instance.hpp"

namespace ot {

//---------------------------------------------------------------------------------------------------------------------
// Dnssd::RequestIdRange

void Dnssd::RequestIdRange::Add(RequestId aId)
{
    if (IsEmpty())
    {
        mStart = aId;
        mEnd   = aId + 1;
    }
    else if (SerialNumber::IsLess(aId, mStart)) // Equivalent to (aId < mStart)
    {
        mStart = aId;
    }
    else if (!SerialNumber::IsLess(aId, mEnd)) // Equivalent to !(aId < mEd) -> (aId >= mEnd)
    {
        mEnd = aId + 1;
    }
}

void Dnssd::RequestIdRange::Remove(RequestId aId)
{
    if (!IsEmpty())
    {
        if (aId == mStart)
        {
            mStart++;
        }
        else if (aId + 1 == mEnd)
        {
            mEnd--;
        }
    }
}

bool Dnssd::RequestIdRange::Contains(RequestId aId) const
{
    // Equivalent to `(aId >= mStart) && (aId < mEnd)`

    return (!SerialNumber::IsLess(aId, mStart) && SerialNumber::IsLess(aId, mEnd));
}

//---------------------------------------------------------------------------------------------------------------------
// Dnssd

Dnssd::Dnssd(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    , mUseNativeMdns(true)
#endif
{
}

Dnssd::State Dnssd::GetState(void) const
{
    State state;

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        state = Get<Dns::Multicast::Core>().IsEnabled() ? kReady : kStopped;
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    state = MapEnum(otPlatDnssdGetState(&GetInstance()));
    ExitNow();
#endif

exit:
    return state;
}

void Dnssd::RegisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().RegisterService(aService, aRequestId, aCallback));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdRegisterService(&GetInstance(), &aService, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::UnregisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().UnregisterService(aService));
        VerifyOrExit(aCallback != nullptr);
        aCallback(&GetInstance(), aRequestId, kErrorNone);
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdUnregisterService(&GetInstance(), &aService, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::RegisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().RegisterHost(aHost, aRequestId, aCallback));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdRegisterHost(&GetInstance(), &aHost, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::UnregisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().UnregisterHost(aHost));
        VerifyOrExit(aCallback != nullptr);
        aCallback(&GetInstance(), aRequestId, kErrorNone);
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdUnregisterHost(&GetInstance(), &aHost, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::RegisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().RegisterKey(aKey, aRequestId, aCallback));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdRegisterKey(&GetInstance(), &aKey, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::UnregisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().UnregisterKey(aKey));
        VerifyOrExit(aCallback != nullptr);
        aCallback(&GetInstance(), aRequestId, kErrorNone);
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdUnregisterKey(&GetInstance(), &aKey, aRequestId, aCallback);
#endif

exit:
    return;
}

void Dnssd::StartBrowser(const Browser &aBrowser)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StartBrowser(aBrowser));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStartBrowser(&GetInstance(), &aBrowser);
#endif

exit:
    return;
}

void Dnssd::StopBrowser(const Browser &aBrowser)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StopBrowser(aBrowser));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStopBrowser(&GetInstance(), &aBrowser);
#endif

exit:
    return;
}

void Dnssd::StartSrvResolver(const SrvResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StartSrvResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStartSrvResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StopSrvResolver(const SrvResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StopSrvResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStopSrvResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StartTxtResolver(const TxtResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StartTxtResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStartTxtResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StopTxtResolver(const TxtResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StopTxtResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStopTxtResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StartIp6AddressResolver(const AddressResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StartIp6AddressResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStartIp6AddressResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StopIp6AddressResolver(const AddressResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StopIp6AddressResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStopIp6AddressResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StartIp4AddressResolver(const AddressResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StartIp4AddressResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStartIp4AddressResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::StopIp4AddressResolver(const AddressResolver &aResolver)
{
    VerifyOrExit(IsReady());

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    {
        IgnoreError(Get<Dns::Multicast::Core>().StopIp4AddressResolver(aResolver));
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    otPlatDnssdStopIp4AddressResolver(&GetInstance(), &aResolver);
#endif

exit:
    return;
}

void Dnssd::HandleStateChange(void)
{
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    Get<Srp::AdvertisingProxy>().HandleDnssdPlatformStateChange();
#endif

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE && OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    Get<Dns::ServiceDiscovery::Server>().HandleDnssdPlatformStateChange();
#endif
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
void Dnssd::HandleMdnsCoreStateChange(void)
{
#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    if (mUseNativeMdns)
#endif
    {
        HandleStateChange();
    }
}
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
extern "C" void otPlatDnssdStateHandleStateChange(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Dnssd>().HandleStateChange();
}
#endif

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
