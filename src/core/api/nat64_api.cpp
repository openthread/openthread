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

#include <openthread/border_router.h>
#include <openthread/ip6.h>
#include <openthread/nat64.h>

#include "border_router/routing_manager.hpp"
#include "common/debug.hpp"
#include "instance/instance.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6_headers.hpp"
#include "net/nat64_translator.hpp"

using namespace ot;

// Note: We support the following scenarios:
// - Using OpenThread's routing manager, while using external NAT64 translator (like tayga).
// - Using OpenThread's NAT64 translator, while using external routing manager.
// So OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE translator and OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE are two
// separate build flags and they are not depending on each other.

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
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

void otNat64InitAddressMappingIterator(otInstance *aInstance, otNat64AddressMappingIterator *aIterator)
{
    AssertPointerIsNotNull(aIterator);

    AsCoreType(aInstance).Get<Nat64::Translator>().InitAddressMappingIterator(*aIterator);
}

otError otNat64GetNextAddressMapping(otInstance                    *aInstance,
                                     otNat64AddressMappingIterator *aIterator,
                                     otNat64AddressMapping         *aMapping)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aMapping);

    return AsCoreType(aInstance).Get<Nat64::Translator>().GetNextAddressMapping(*aIterator, *aMapping);
}

void otNat64GetCounters(otInstance *aInstance, otNat64ProtocolCounters *aCounters)
{
    AsCoreType(aInstance).Get<Nat64::Translator>().GetCounters(AsCoreType(aCounters));
}

void otNat64GetErrorCounters(otInstance *aInstance, otNat64ErrorCounters *aCounters)
{
    AsCoreType(aInstance).Get<Nat64::Translator>().GetErrorCounters(AsCoreType(aCounters));
}

otError otNat64GetCidr(otInstance *aInstance, otIp4Cidr *aCidr)
{
    return AsCoreType(aInstance).Get<Nat64::Translator>().GetIp4Cidr(AsCoreType(aCidr));
}

otNat64State otNat64GetTranslatorState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<Nat64::Translator>().GetState());
}
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
otNat64State otNat64GetPrefixManagerState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().GetNat64PrefixManagerState());
}
#endif

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE || OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
void otNat64SetEnabled(otInstance *aInstance, bool aEnabled)
{
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().SetNat64PrefixManagerEnabled(aEnabled);
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    AsCoreType(aInstance).Get<Nat64::Translator>().SetEnabled(aEnabled);
#endif
}
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE || OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

bool otIp4IsAddressEqual(const otIp4Address *aFirst, const otIp4Address *aSecond)
{
    return AsCoreType(aFirst) == AsCoreType(aSecond);
}

void otIp4ExtractFromIp6Address(uint8_t aPrefixLength, const otIp6Address *aIp6Address, otIp4Address *aIp4Address)
{
    AsCoreType(aIp4Address).ExtractFromIp6Address(aPrefixLength, AsCoreType(aIp6Address));
}

otError otIp4FromIp4MappedIp6Address(const otIp6Address *aIp6Address, otIp4Address *aIp4Address)
{
    return AsCoreType(aIp4Address).ExtractFromIp4MappedIp6Address(AsCoreType(aIp6Address));
}

void otIp4ToIp4MappedIp6Address(const otIp4Address *aIp4Address, otIp6Address *aIp6Address)
{
    AsCoreType(aIp6Address).SetToIp4Mapped(AsCoreType(aIp4Address));
}

otError otIp4AddressFromString(const char *aString, otIp4Address *aAddress)
{
    AssertPointerIsNotNull(aString);
    return AsCoreType(aAddress).FromString(aString);
}

otError otNat64SynthesizeIp6Address(otInstance *aInstance, const otIp4Address *aIp4Address, otIp6Address *aIp6Address)
{
    otError                          err = OT_ERROR_NONE;
    NetworkData::ExternalRouteConfig nat64Prefix;

    VerifyOrExit(AsCoreType(aInstance).Get<NetworkData::Leader>().GetPreferredNat64Prefix(nat64Prefix) == OT_ERROR_NONE,
                 err = OT_ERROR_INVALID_STATE);
    AsCoreType(aIp6Address).SynthesizeFromIp4Address(nat64Prefix.GetPrefix(), AsCoreType(aIp4Address));

exit:
    return err;
}

void otIp4AddressToString(const otIp4Address *aAddress, char *aBuffer, uint16_t aSize)
{
    AssertPointerIsNotNull(aBuffer);

    AsCoreType(aAddress).ToString(aBuffer, aSize);
}

otError otIp4CidrFromString(const char *aString, otIp4Cidr *aCidr) { return AsCoreType(aCidr).FromString(aString); }

void otIp4CidrToString(const otIp4Cidr *aCidr, char *aBuffer, uint16_t aSize)
{
    AssertPointerIsNotNull(aBuffer);

    AsCoreType(aCidr).ToString(aBuffer, aSize);
}
