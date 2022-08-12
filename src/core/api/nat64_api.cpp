/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#include <openthread/border_router.h>
#include <openthread/ip6.h>
#include <openthread/nat64.h>

#include "border_router/routing_manager.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6_headers.hpp"
#include "net/nat64_translator.hpp"

using namespace ot;

otError otNat64SetIp4Cidr(otInstance *aInstance, const otIp4Cidr *aCidr)
{
    return AsCoreType(aInstance).Get<Nat64::Translator>().SetIp4Cidr(AsCoreType(aCidr));
}

otMessage *otIp4NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    return AsCoreType(aInstance).Get<Nat64::Translator>().NewIp4Message(Message::Settings::From(aSettings));
}

otError otNat64Send(otInstance *aInstance, otMessage *aMessage)
{
    return AsCoreType(aInstance).Get<Nat64::Translator>().SendMessage(AsCoreType(aMessage));
}

void otNat64SetReceiveIp4Callback(otInstance *aInstance, otNat64ReceiveIp4Callback aCallback, void *aContext)
{
    AsCoreType(aInstance).Get<Ip6::Ip6>().SetNat64ReceiveIp4DatagramCallback(aCallback, aContext);
}

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
