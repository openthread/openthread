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
 *   NCP mDNS platform implementation.
 *
 *   Sends mDNS packets to OTBR host via UDP datagrams on port 5353.
 *   Host forwards packets to infrastructure network.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE

#include <string.h>

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/logging.h>
#include <openthread/message.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/platform/mdns_socket.h>

#include "common/as_core_type.hpp"
#include "common/message.hpp"

namespace {
constexpr uint16_t kMdnsPort               = 5353;
constexpr char     kMdnsMulticastAddress[] = "ff02::fb";
} // namespace

extern "C" otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    otLogInfoPlat("mDNS listening %s on NCP", aEnable ? "enabled" : "disabled");

    return OT_ERROR_NONE;
}

extern "C" void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    otError           error = OT_ERROR_NONE;
    otMessageInfo     messageInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    otMessage        *udpMessage      = nullptr;
    uint16_t          length          = 0;

    OT_UNUSED_VARIABLE(aInfraIfIndex);

    VerifyOrExit(aMessage != nullptr);

    length = otMessageGetLength(aMessage);

    memset(&messageInfo, 0, sizeof(messageInfo));

    SuccessOrExit(error = otIp6AddressFromString(kMdnsMulticastAddress, &messageInfo.mPeerAddr));
    messageInfo.mPeerPort        = kMdnsPort;
    messageInfo.mSockPort        = kMdnsPort;
    messageInfo.mSockAddr        = *otThreadGetLinkLocalIp6Address(aInstance);
    messageInfo.mIsHostInterface = true;

    udpMessage = otUdpNewMessage(aInstance, &messageSettings);
    VerifyOrExit(udpMessage != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = ot::AsCoreType(udpMessage).AppendBytesFromMessage(ot::AsCoreType(aMessage), 0, length));

    otLogInfoPlat("Sending multicast mDNS packet (%u bytes)", length);

    error = otUdpSendDatagram(aInstance, udpMessage, &messageInfo);
    if (error == OT_ERROR_NONE)
    {
        udpMessage = nullptr;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("Failed to send multicast: %s", otThreadErrorToString(error));
    }

    if (udpMessage != nullptr)
    {
        otMessageFree(udpMessage);
    }

    if (aMessage != nullptr)
    {
        otMessageFree(aMessage);
    }
}

extern "C" void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    otError           error = OT_ERROR_NONE;
    otMessageInfo     messageInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    otMessage        *udpMessage      = nullptr;
    uint16_t          length          = 0;

    VerifyOrExit(aMessage != nullptr && aAddress != nullptr);

    length = otMessageGetLength(aMessage);

    memset(&messageInfo, 0, sizeof(messageInfo));

    messageInfo.mPeerAddr        = aAddress->mAddress;
    messageInfo.mPeerPort        = aAddress->mPort;
    messageInfo.mSockPort        = kMdnsPort;
    messageInfo.mSockAddr        = *otThreadGetLinkLocalIp6Address(aInstance);
    messageInfo.mIsHostInterface = true;

    udpMessage = otUdpNewMessage(aInstance, &messageSettings);
    VerifyOrExit(udpMessage != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = ot::AsCoreType(udpMessage).AppendBytesFromMessage(ot::AsCoreType(aMessage), 0, length));

    otLogInfoPlat("Sending unicast mDNS packet (%u bytes) to port %u", length, aAddress->mPort);

    error = otUdpSendDatagram(aInstance, udpMessage, &messageInfo);
    if (error == OT_ERROR_NONE)
    {
        udpMessage = nullptr;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("Failed to send unicast: %s", otThreadErrorToString(error));
    }

    if (udpMessage != nullptr)
    {
        otMessageFree(udpMessage);
    }

    if (aMessage != nullptr)
    {
        otMessageFree(aMessage);
    }
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
