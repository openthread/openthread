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
 *   This file includes common Border Router logging helper functions.
 */

#ifndef BR_LOG_HPP_
#define BR_LOG_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "border_router/br_types.hpp"
#include "common/log.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace BorderRouter {

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

/**
 * Logs a Router Advertisement (RA) header at info log level.
 *
 * @param[in] aRaHeader  The RA header to log.
 */
void LogRaHeader(const RouterAdvert::Header &aRaHeader);

/**
 * Logs a Prefix Information Option (PIO) at info log level.
 *
 * @param[in] aPrefix             The IPv6 prefix.
 * @param[in] aValidLifetime      The valid lifetime in seconds.
 * @param[in] aPreferredLifetime  The preferred lifetime in seconds.
 * @param[in] aFlags              The PIO flags.
 */
void LogPrefixInfoOption(const Ip6::Prefix      &aPrefix,
                         uint32_t                aValidLifetime,
                         uint32_t                aPreferredLifetime,
                         PrefixInfoOption::Flags aFlags);

/**
 * Logs a Route Information Option (RIO) at info log level.
 *
 * @param[in] aPrefix      The IPv6 prefix.
 * @param[in] aLifetime    The route lifetime in seconds.
 * @param[in] aPreference  The route preference.
 */
void LogRouteInfoOption(const Ip6::Prefix &aPrefix, uint32_t aLifetime, RoutePreference aPreference);

/**
 * Logs a Recursive DNS Server (RDNSS) option at info log level.
 *
 * @param[in] aAddress   The IPv6 address of the DNS server.
 * @param[in] aLifetime  The lifetime of the DNS server address in seconds.
 */
void LogRecursiveDnsServerOption(const Ip6::Address &aAddress, uint32_t aLifetime);

#else

inline void LogRaHeader(const RouterAdvert::Header &) {}
inline void LogPrefixInfoOption(const Ip6::Prefix &, uint32_t, uint32_t, PrefixInfoOption::Flags) {}
inline void LogRouteInfoOption(const Ip6::Prefix &, uint32_t, RoutePreference) {}
inline void LogRecursiveDnsServerOption(const Ip6::Address &, uint32_t) {}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // BR_LOG_HPP_
