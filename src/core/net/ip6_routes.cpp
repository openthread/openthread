/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <net/ip6_routes.hpp>
#include <net/netif.hpp>
#include <common/code_utils.hpp>
#include <common/message.hpp>
#include <openthreadcontext.h>

namespace Thread {
namespace Ip6 {

ThreadError Routes::Add(otContext *aContext, Route &aRoute)
{
    ThreadError error = kThreadError_None;

    for (Route *cur = aContext->mRoutes; cur; cur = cur->mNext)
    {
        VerifyOrExit(cur != &aRoute, error = kThreadError_Busy);
    }

    aRoute.mNext = aContext->mRoutes;
    aContext->mRoutes = &aRoute;

exit:
    return error;
}

ThreadError Routes::Remove(otContext *aContext, Route &aRoute)
{
    if (&aRoute == aContext->mRoutes)
    {
        aContext->mRoutes = aRoute.mNext;
    }
    else
    {
        for (Route *cur = aContext->mRoutes; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aRoute)
            {
                cur->mNext = aRoute.mNext;
                break;
            }
        }
    }

    aRoute.mNext = NULL;

    return kThreadError_None;
}

int Routes::Lookup(otContext *aContext, const Address &aSource, const Address &aDestination)
{
    int maxPrefixMatch = -1;
    uint8_t prefixMatch;
    int rval = -1;

    for (Route *cur = aContext->mRoutes; cur; cur = cur->mNext)
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

        if (maxPrefixMatch > prefixMatch)
        {
            continue;
        }

        maxPrefixMatch = prefixMatch;
        rval = cur->mInterfaceId;
    }

    for (Netif *netif = Netif::GetNetifList(aContext); netif; netif = netif->GetNext())
    {
        if (netif->RouteLookup(aSource, aDestination, &prefixMatch) == kThreadError_None &&
            prefixMatch > maxPrefixMatch)
        {
            maxPrefixMatch = prefixMatch;
            rval = netif->GetInterfaceId();
        }
    }

    return rval;
}

}  // namespace Ip6
}  // namespace Thread
