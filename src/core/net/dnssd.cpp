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
 *   This file implements infrastructure DNS-SD (mDNS) platform APIs.
 */

#include "dnssd.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE

#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
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
{
}

Dnssd::State Dnssd::GetState(void) const { return MapEnum(otPlatDnssdGetState(&GetInstance())); }

void Dnssd::RegisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdRegisterService(&GetInstance(), &aService, aRequestId, aCallback);
    }
}

void Dnssd::UnregisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdUnregisterService(&GetInstance(), &aService, aRequestId, aCallback);
    }
}

void Dnssd::RegisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdRegisterHost(&GetInstance(), &aHost, aRequestId, aCallback);
    }
}

void Dnssd::UnregisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdUnregisterHost(&GetInstance(), &aHost, aRequestId, aCallback);
    }
}

void Dnssd::RegisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdRegisterKey(&GetInstance(), &aKey, aRequestId, aCallback);
    }
}

void Dnssd::UnregisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback)
{
    if (IsReady())
    {
        otPlatDnssdUnregisterKey(&GetInstance(), &aKey, aRequestId, aCallback);
    }
}

void Dnssd::HandleStateChange(void)
{
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    Get<Srp::AdvertisingProxy>().HandleDnssdPlatformStateChange();
#endif
}

extern "C" void otPlatDnssdStateHandleStateChange(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Dnssd>().HandleStateChange();
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
