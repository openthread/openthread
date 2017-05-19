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

#include <openthread/ip6.h>

#include "openthread-instance.h"
#include "common/logging.hpp"
#include "utils/slaac_address.hpp"

using namespace ot;

ThreadError otIp6SetEnabled(otInstance *aInstance, bool aEnabled)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    if (aEnabled)
    {
#if OPENTHREAD_ENABLE_RAW_LINK_API
        VerifyOrExit(!aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
        error = aInstance->mThreadNetif.Up();
    }
    else
    {
#if OPENTHREAD_ENABLE_RAW_LINK_API
        VerifyOrExit(!aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
        error = aInstance->mThreadNetif.Down();
    }

#if OPENTHREAD_ENABLE_RAW_LINK_API
exit:
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    otLogFuncExitErr(error);
    return error;
}

bool otIp6IsEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.IsUp();
}

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetUnicastAddresses();
}

ThreadError otIp6AddUnicastAddress(otInstance *aInstance, const otNetifAddress *address)
{
    return aInstance->mThreadNetif.AddExternalUnicastAddress(*static_cast<const Ip6::NetifUnicastAddress *>(address));
}

ThreadError otIp6RemoveUnicastAddress(otInstance *aInstance, const otIp6Address *address)
{
    return aInstance->mThreadNetif.RemoveExternalUnicastAddress(*static_cast<const Ip6::Address *>(address));
}

const otNetifMulticastAddress *otIp6GetMulticastAddresses(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMulticastAddresses();
}

ThreadError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.SubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

ThreadError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.UnsubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

bool otIp6IsMulticastPromiscuousEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.IsMulticastPromiscuousEnabled();
}

void otIp6SetMulticastPromiscuousEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.SetMulticastPromiscuous(aEnabled);
}

void otIp6SlaacUpdate(otInstance *aInstance, otNetifAddress *aAddresses, uint32_t aNumAddresses,
                      otIp6SlaacIidCreate aIidCreate, void *aContext)
{
    Utils::Slaac::UpdateAddresses(aInstance, aAddresses, aNumAddresses, aIidCreate, aContext);
}

ThreadError otIp6CreateRandomIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return Utils::Slaac::CreateRandomIid(aInstance, aAddress, aContext);
}

ThreadError otIp6CreateMacIid(otInstance *aInstance, otNetifAddress *aAddress, void *)
{
    memcpy(&aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE],
           aInstance->mThreadNetif.GetMac().GetExtAddress(), OT_IP6_IID_SIZE);
    aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE] ^= 0x02;

    return kThreadError_None;
}

ThreadError otIp6CreateSemanticallyOpaqueIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return static_cast<Utils::SemanticallyOpaqueIidGenerator *>(aContext)->CreateIid(aInstance, aAddress);
}

void otIp6SetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    aInstance->mIp6.SetReceiveDatagramCallback(aCallback, aCallbackContext);
}

bool otIp6IsReceiveFilterEnabled(otInstance *aInstance)
{
    return aInstance->mIp6.IsReceiveIp6FilterEnabled();
}

void otIp6SetReceiveFilterEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mIp6.SetReceiveIp6FilterEnabled(aEnabled);
}

ThreadError otIp6Send(otInstance *aInstance, otMessage *aMessage)
{
    ThreadError error;

    otLogFuncEntry();

    error = aInstance->mIp6.HandleDatagram(*static_cast<Message *>(aMessage), NULL,
                                           aInstance->mThreadNetif.GetInterfaceId(), NULL, true);

    otLogFuncExitErr(error);

    return error;
}

otMessage *otIp6NewMessage(otInstance *aInstance, bool aLinkSecurityEnabled)
{
    Message *message = aInstance->mIp6.mMessagePool.New(Message::kTypeIp6, 0);

    if (message)
    {
        message->SetLinkSecurityEnabled(aLinkSecurityEnabled);
    }

    return message;
}

ThreadError otIp6AddUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    return aInstance->mThreadNetif.GetIp6Filter().AddUnsecurePort(aPort);
}

ThreadError otIp6RemoveUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    return aInstance->mThreadNetif.GetIp6Filter().RemoveUnsecurePort(aPort);
}

const uint16_t *otIp6GetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries)
{
    return aInstance->mThreadNetif.GetIp6Filter().GetUnsecurePorts(*aNumEntries);
}

bool otIp6IsAddressEqual(const otIp6Address *a, const otIp6Address *b)
{
    return *static_cast<const Ip6::Address *>(a) == *static_cast<const Ip6::Address *>(b);
}

ThreadError otIp6AddressFromString(const char *str, otIp6Address *address)
{
    return static_cast<Ip6::Address *>(address)->FromString(str);
}

uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    uint8_t rval;

    VerifyOrExit(aFirst != NULL && aSecond != NULL, rval = 0);

    rval = static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));

exit:
    return rval;
}
