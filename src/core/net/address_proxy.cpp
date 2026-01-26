/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements IPv6 Address Proxy.
 */

#include "address_proxy.hpp"

#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "openthread/platform/toolchain.h"

namespace ot {
namespace Ip6 {

AddressProxy::AddressProxy(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void AddressProxy::AddAddress(ProxyAddress &aAddress)
{
    if (!mProxyAddresses.Contains(aAddress))
    {
        Error error = mProxyAddresses.Add(aAddress);

        OT_UNUSED_VARIABLE(error);
        OT_ASSERT(error == kErrorNone);
    }
}

void AddressProxy::RemoveAddress(ProxyAddress &aAddress) { IgnoreError(mProxyAddresses.Remove(aAddress)); }

bool AddressProxy::IsProxyAddress(const Address &aAddress) const
{
    bool matches = false;

    for (const ProxyAddress &entry : mProxyAddresses)
    {
        if (entry.GetAddress() == aAddress)
        {
            matches = true;
            break;
        }
    }

    return matches;
}

void AddressProxy::HandleDatagram(ot::Message &aMessage)
{
    Header ip6Header;

    SuccessOrExit(ip6Header.ParseFrom(aMessage));

    for (ProxyAddress &entry : mProxyAddresses)
    {
        if (entry.GetAddress() == ip6Header.GetDestination())
        {
            entry.InvokeCallback(aMessage);
            break;
        }
    }

exit:
    return;
}

void AddressProxy::ForEachAddress(otIp6ProxyAddressCallback aCallback, void *aContext) const
{
    for (const ProxyAddress &entry : mProxyAddresses)
    {
        aCallback(&entry.GetAddress(), aContext);
    }
}

} // namespace Ip6
} // namespace ot
