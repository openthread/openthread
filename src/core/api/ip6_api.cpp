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

#define WPP_NAME "ip6_api.tmh"

#include "openthread-core-config.h"

#include <openthread/ip6.h>

#include "common/instance.hpp"
#include "common/logging.hpp"
#include "utils/slaac_address.hpp"

using namespace ot;

otError otIp6SetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    VerifyOrExit(!instance.GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);
#endif

    if (aEnabled)
    {
        instance.GetThreadNetif().Up();
    }
    else
    {
        instance.GetThreadNetif().Down();
    }

#if OPENTHREAD_ENABLE_RAW_LINK_API
exit:
#endif
    return error;
}

bool otIp6IsEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().IsUp();
}

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetUnicastAddresses();
}

otError otIp6AddUnicastAddress(otInstance *aInstance, const otNetifAddress *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().AddExternalUnicastAddress(
        *static_cast<const Ip6::NetifUnicastAddress *>(aAddress));
}

otError otIp6RemoveUnicastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().RemoveExternalUnicastAddress(*static_cast<const Ip6::Address *>(aAddress));
}

const otNetifMulticastAddress *otIp6GetMulticastAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMulticastAddresses();
}

otError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().SubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

otError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().UnsubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

bool otIp6IsMulticastPromiscuousEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().IsMulticastPromiscuousEnabled();
}

void otIp6SetMulticastPromiscuousEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().SetMulticastPromiscuous(aEnabled);
}

otError otIp6CreateRandomIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return Utils::Slaac::CreateRandomIid(aInstance, aAddress, aContext);
}

otError otIp6CreateMacIid(otInstance *aInstance, otNetifAddress *aAddress, void *)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    memcpy(&aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE],
           &instance.GetThreadNetif().GetMac().GetExtAddress(), OT_IP6_IID_SIZE);
    aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE] ^= 0x02;

    return OT_ERROR_NONE;
}

otError otIp6CreateSemanticallyOpaqueIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return static_cast<Utils::SemanticallyOpaqueIidGenerator *>(aContext)->CreateIid(aInstance, aAddress);
}

void otIp6SetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetIp6().SetReceiveDatagramCallback(aCallback, aCallbackContext);
}

void otIp6SetAddressCallback(otInstance *aInstance, otIp6AddressCallback aCallback, void *aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().SetAddressCallback(aCallback, aCallbackContext);
}

bool otIp6IsReceiveFilterEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetIp6().IsReceiveIp6FilterEnabled();
}

void otIp6SetReceiveFilterEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetIp6().SetReceiveIp6FilterEnabled(aEnabled);
}

otError otIp6Send(otInstance *aInstance, otMessage *aMessage)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    error = instance.GetIp6().SendRaw(*static_cast<Message *>(aMessage), instance.GetThreadNetif().GetInterfaceId());

    return error;
}

otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Message * message;

    if (aSettings != NULL)
    {
        VerifyOrExit(aSettings->mPriority <= OT_MESSAGE_PRIORITY_HIGH, message = NULL);
    }

    message = instance.GetIp6().NewMessage(0, aSettings);

exit:
    return message;
}

otMessage *otIp6NewMessageFromBuffer(otInstance *             aInstance,
                                     const uint8_t *          aData,
                                     uint16_t                 aDataLength,
                                     const otMessageSettings *aSettings)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Message * message;

    VerifyOrExit((message = instance.GetIp6().NewMessage(aData, aDataLength, aSettings)) != NULL);

exit:
    return message;
}

otError otIp6AddUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetIp6Filter().AddUnsecurePort(aPort);
}

otError otIp6RemoveUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetIp6Filter().RemoveUnsecurePort(aPort);
}

void otIp6RemoveAllUnsecurePorts(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetIp6Filter().RemoveAllUnsecurePorts();
}

const uint16_t *otIp6GetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetIp6Filter().GetUnsecurePorts(*aNumEntries);
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
    uint8_t rval;

    VerifyOrExit(aFirst != NULL && aSecond != NULL, rval = 0);

    rval = static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));

exit:
    return rval;
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

    netifAddr = instance.GetIp6().SelectSourceAddress(*static_cast<Ip6::MessageInfo *>(aMessageInfo));
    VerifyOrExit(netifAddr != NULL, error = OT_ERROR_NOT_FOUND);
    memcpy(&aMessageInfo->mSockAddr, &netifAddr->mAddress, sizeof(aMessageInfo->mSockAddr));

exit:
    return error;
}
