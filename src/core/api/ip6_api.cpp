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
 *   This file implements the OpenThread IPv6 API.
 */

#include "openthread-core-config.h"

#include <openthread/ip6.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
#include "utils/slaac_address.hpp"
#endif

using namespace ot;

otError otIp6SetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(!instance.Get<Mac::LinkRaw>().IsEnabled(), error = OT_ERROR_INVALID_STATE);
#endif

    if (aEnabled)
    {
        instance.Get<ThreadNetif>().Up();
    }
    else
    {
        instance.Get<ThreadNetif>().Down();
    }

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
exit:
#endif
    return error;
}

bool otIp6IsEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().IsUp();
}

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().GetUnicastAddresses();
}

otError otIp6AddUnicastAddress(otInstance *aInstance, const otNetifAddress *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().AddExternalUnicastAddress(
        *static_cast<const Ip6::NetifUnicastAddress *>(aAddress));
}

otError otIp6RemoveUnicastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().RemoveExternalUnicastAddress(*static_cast<const Ip6::Address *>(aAddress));
}

const otNetifMulticastAddress *otIp6GetMulticastAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().GetMulticastAddresses();
}

otError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().SubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

otError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().UnsubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

bool otIp6IsMulticastPromiscuousEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ThreadNetif>().IsMulticastPromiscuousEnabled();
}

void otIp6SetMulticastPromiscuousEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<ThreadNetif>().SetMulticastPromiscuous(aEnabled);
}

void otIp6SetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Ip6>().SetReceiveDatagramCallback(aCallback, aCallbackContext);
}

void otIp6SetAddressCallback(otInstance *aInstance, otIp6AddressCallback aCallback, void *aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<ThreadNetif>().SetAddressCallback(aCallback, aCallbackContext);
}

bool otIp6IsReceiveFilterEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Ip6>().IsReceiveIp6FilterEnabled();
}

void otIp6SetReceiveFilterEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Ip6>().SetReceiveIp6FilterEnabled(aEnabled);
}

otError otIp6Send(otInstance *aInstance, otMessage *aMessage)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Ip6>().SendRaw(*static_cast<Message *>(aMessage));
}

otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Ip6>().NewMessage(0, Message::Settings(aSettings));
}

otMessage *otIp6NewMessageFromBuffer(otInstance *             aInstance,
                                     const uint8_t *          aData,
                                     uint16_t                 aDataLength,
                                     const otMessageSettings *aSettings)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Message * message;

    if (aSettings != nullptr)
    {
        message = instance.Get<Ip6::Ip6>().NewMessage(aData, aDataLength, Message::Settings(aSettings));
    }
    else
    {
        message = instance.Get<Ip6::Ip6>().NewMessage(aData, aDataLength);
    }

    return message;
}

otError otIp6AddUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Filter>().AddUnsecurePort(aPort);
}

otError otIp6RemoveUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Filter>().RemoveUnsecurePort(aPort);
}

void otIp6RemoveAllUnsecurePorts(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Filter>().RemoveAllUnsecurePorts();
}

const uint16_t *otIp6GetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Filter>().GetUnsecurePorts(*aNumEntries);
}

bool otIp6IsAddressEqual(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    return *static_cast<const Ip6::Address *>(aFirst) == *static_cast<const Ip6::Address *>(aSecond);
}

otError otIp6AddressFromString(const char *aString, otIp6Address *aAddress)
{
    return static_cast<Ip6::Address *>(aAddress)->FromString(aString);
}

uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    OT_ASSERT(aFirst != nullptr && aSecond != nullptr);

    return static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));
}

bool otIp6IsAddressUnspecified(const otIp6Address *aAddress)
{
    return static_cast<const Ip6::Address *>(aAddress)->IsUnspecified();
}

otError otIp6SelectSourceAddress(otInstance *aInstance, otMessageInfo *aMessageInfo)
{
    otError                         error    = OT_ERROR_NONE;
    Instance &                      instance = *static_cast<Instance *>(aInstance);
    const Ip6::NetifUnicastAddress *netifAddr;

    netifAddr = instance.Get<Ip6::Ip6>().SelectSourceAddress(*static_cast<Ip6::MessageInfo *>(aMessageInfo));
    VerifyOrExit(netifAddr != nullptr, error = OT_ERROR_NOT_FOUND);
    aMessageInfo->mSockAddr = netifAddr->GetAddress();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

bool otIp6IsSlaacEnabled(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->Get<Utils::Slaac>().IsEnabled();
}

void otIp6SetSlaacEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aEnabled)
    {
        instance.Get<Utils::Slaac>().Enable();
    }
    else
    {
        instance.Get<Utils::Slaac>().Disable();
    }
}

void otIp6SetSlaacPrefixFilter(otInstance *aInstance, otIp6SlaacPrefixFilter aFilter)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Utils::Slaac>().SetFilter(aFilter);
}

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
otError otIp6RegisterMulticastListeners(otInstance *                            aInstance,
                                        const otIp6Address *                    aAddresses,
                                        uint8_t                                 aAddressNum,
                                        const uint32_t *                        aTimeout,
                                        otIp6RegisterMulticastListenersCallback aCallback,
                                        void *                                  aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MlrManager>().RegisterMulticastListeners(aAddresses, aAddressNum, aTimeout, aCallback,
                                                                 aContext);
}
#endif

#endif // OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
