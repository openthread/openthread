/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements IPv6 route tables.
 */

#include "ip6_routes.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"

namespace ot {
namespace Ip6 {

Routes::Routes(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRoutes(NULL)
{
}

otError Routes::Add(Route &aRoute)
{
    otError error = OT_ERROR_NONE;

    for (Route *cur = mRoutes; cur; cur = cur->mNext)
    {
        VerifyOrExit(cur != &aRoute, error = OT_ERROR_ALREADY);
    }

    aRoute.mNext = mRoutes;
    mRoutes      = &aRoute;

exit:
    return error;
}

otError Routes::Remove(Route &aRoute)
{
    if (&aRoute == mRoutes)
    {
        mRoutes = aRoute.mNext;
    }
    else
    {
        for (Route *cur = mRoutes; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aRoute)
            {
                cur->mNext = aRoute.mNext;
                break;
            }
        }
    }

    aRoute.mNext = NULL;

    return OT_ERROR_NONE;
}

int8_t Routes::Lookup(const Address &aSource, const Address &aDestination)
{
    int8_t  maxPrefixMatch = -1;
    uint8_t prefixMatch;
    int8_t  rval = -1;

    for (Route *cur = mRoutes; cur; cur = cur->mNext)
    {
        prefixMatch = cur->mPrefix.PrefixMatch(aDestination);

        if (prefixMatch < cur->mPrefixLength)
        {
            continue;
        }

        if (prefixMatch > cur->mPrefixLength)
        {
            prefixMatch = cur->mPrefixLength;
        }

        if (maxPrefixMatch > static_cast<int8_t>(prefixMatch))
        {
            continue;
        }

        maxPrefixMatch = static_cast<int8_t>(prefixMatch);
        rval           = cur->mInterfaceId;
    }

    for (Netif *netif = GetIp6().GetNetifList(); netif; netif = netif->GetNext())
    {
        if (netif->RouteLookup(aSource, aDestination, &prefixMatch) == OT_ERROR_NONE &&
            static_cast<int8_t>(prefixMatch) > maxPrefixMatch)
        {
            maxPrefixMatch = static_cast<int8_t>(prefixMatch);
            rval           = netif->GetInterfaceId();
        }
    }

    return rval;
}

} // namespace Ip6
} // namespace ot
