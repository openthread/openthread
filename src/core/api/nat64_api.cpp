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
 *   This file implements the OpenThread APIs for handling IPv4 (NAT64) messages
 */

#include "openthread-core-config.h"

#include <openthread/border_router.h>
#include <openthread/ip6.h>
#include <openthread/nat64.h>

#include "border_router/nat64.hpp"
#include "border_router/routing_manager.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6_headers.hpp"

using namespace ot;

// The following functions supports NAT64, but actual NAT64 handling is wrapped in RoutingManager::SendPacket,
// RoutingManager::SetInfraReceiveCallback
// When BORDER_ROUTING is disabled, otBorderRouterSend / otBorderRouterSetReceiveCallback will fallback to corresponding
// functions in Ip6 namespace so they can be always available.
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError otBorderRouterSend(otInstance *aInstance, otMessage *aMessage)
{
    return AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SendPacket(AsCoreType(aMessage));
}

void otBorderRouterSetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetInfraReceiveCallback(aCallback, aCallbackContext);
}
#else  // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError otBorderRouterSend(otInstance *aInstance, otMessage *aMessage)
{
    return otIp6Send(aInstance, aMessage);
}

void otBorderRouterSetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    return otIp6SetReceiveCallback(aInstance, aCallback, aCallbackContext);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE
otError otBorderRouterSetIp4CidrForNat64(otInstance *aInstance, otIp4Cidr *aCidr)
{
    return AsCoreType(aInstance).Get<BorderRouter::Nat64>().SetIp4Cidr(static_cast<ot::Ip4::Cidr &>(*aCidr));
}

otError otBorderRouterSetNat64TranslatorEnabled(otInstance *aInstance, bool aEnabled)
{
    return AsCoreType(aInstance).Get<BorderRouter::Nat64>().SetEnabled(aEnabled);
}

otMessage *otIp6NewMessageForNat64(otInstance *aInstance, const otMessageSettings *aSettings)
{
    return AsCoreType(aInstance).Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Header) - sizeof(Ip4::Header),
                                                            Message::Settings::From(aSettings));
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE
