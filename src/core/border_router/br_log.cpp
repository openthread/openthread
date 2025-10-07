/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements common Border Router logging helper functions.
 */

#include "br_log.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

RegisterLogModule("BorderRouting");

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void LogRaHeader(const RouterAdvert::Header &aRaHeader)
{
    LogInfo("- RA Header - flags - M:%u O:%u S:%u", aRaHeader.IsManagedAddressConfigFlagSet(),
            aRaHeader.IsOtherConfigFlagSet(), aRaHeader.IsSnacRouterFlagSet());
    LogInfo("- RA Header - default route - lifetime:%u", aRaHeader.GetRouterLifetime());
}

void LogPrefixInfoOption(const Ip6::Prefix      &aPrefix,
                         uint32_t                aValidLifetime,
                         uint32_t                aPreferredLifetime,
                         PrefixInfoOption::Flags aFlags)
{
    LogInfo("- PIO %s (valid:%lu, preferred:%lu, flags - L:%u A:%u P:%u)", aPrefix.ToString().AsCString(),
            ToUlong(aValidLifetime), ToUlong(aPreferredLifetime), (aFlags & PrefixInfoOption::kOnLinkFlag) ? 1 : 0,
            (aFlags & PrefixInfoOption::kAutoConfigFlag) ? 1 : 0,
            (aFlags & PrefixInfoOption::kDhcp6PdPreferredFlag) ? 1 : 0);
}

void LogRouteInfoOption(const Ip6::Prefix &aPrefix, uint32_t aLifetime, RoutePreference aPreference)
{
    LogInfo("- RIO %s (lifetime:%lu, prf:%s)", aPrefix.ToString().AsCString(), ToUlong(aLifetime),
            RoutePreferenceToString(aPreference));
}

void LogRecursiveDnsServerOption(const Ip6::Address &aAddress, uint32_t aLifetime)
{
    LogInfo("- RDNSS %s (lifetime:%lu)", aAddress.ToString().AsCString(), ToUlong(aLifetime));
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
