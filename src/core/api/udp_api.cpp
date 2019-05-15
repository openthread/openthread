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
 *   This file implements the OpenThread UDP API.
 */

#include "openthread-core-config.h"

#include <openthread/udp.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/new.hpp"
#include "net/udp6.hpp"

using namespace ot;

otMessage *otUdpNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    Message * message;

    if (aSettings != NULL)
    {
        VerifyOrExit(aSettings->mPriority <= OT_MESSAGE_PRIORITY_HIGH, message = NULL);
    }

    message = instance.Get<Ip6::Udp>().NewMessage(0, aSettings);

exit:
    return message;
}

otError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext)
{
    otError         error    = OT_ERROR_INVALID_ARGS;
    Instance &      instance = *static_cast<Instance *>(aInstance);
    Ip6::UdpSocket &socket   = *new (aSocket) Ip6::UdpSocket(instance.Get<Ip6::Udp>());

    error = socket.Open(aCallback, aContext);

    return error;
}

otError otUdpClose(otUdpSocket *aSocket)
{
    otError         error  = OT_ERROR_INVALID_STATE;
    Ip6::UdpSocket &socket = *static_cast<Ip6::UdpSocket *>(aSocket);

    error = socket.Close();

    return error;
}

otError otUdpBind(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket &socket = *static_cast<Ip6::UdpSocket *>(aSocket);
    return socket.Bind(*static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otUdpConnect(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket &socket = *static_cast<Ip6::UdpSocket *>(aSocket);
    return socket.Connect(*static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otUdpSend(otUdpSocket *aSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Ip6::UdpSocket &socket = *static_cast<Ip6::UdpSocket *>(aSocket);
    return socket.SendTo(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

#if OPENTHREAD_ENABLE_UDP_FORWARD
void otUdpForwardSetForwarder(otInstance *aInstance, otUdpForwarder aForwarder, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Udp>().SetUdpForwarder(aForwarder, aContext);
}

void otUdpForwardReceive(otInstance *        aInstance,
                         otMessage *         aMessage,
                         uint16_t            aPeerPort,
                         const otIp6Address *aPeerAddr,
                         uint16_t            aSockPort)
{
    Ip6::MessageInfo messageInfo;
    Instance &       instance = *static_cast<Instance *>(aInstance);

    assert(aMessage != NULL && aPeerAddr != NULL);

    messageInfo.SetSockAddr(instance.Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetSockPort(aSockPort);
    messageInfo.SetPeerAddr(*static_cast<const ot::Ip6::Address *>(aPeerAddr));
    messageInfo.SetPeerPort(aPeerPort);
    messageInfo.SetInterfaceId(OT_NETIF_INTERFACE_ID_HOST);

    instance.Get<Ip6::Udp>().HandlePayload(*static_cast<ot::Message *>(aMessage), messageInfo);

    static_cast<ot::Message *>(aMessage)->Free();
}
#endif // OPENTHREAD_ENABLE_UDP_FORWARD

#if OPENTHREAD_ENABLE_PLATFORM_UDP
otUdpSocket *otUdpGetSockets(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().GetUdpSockets();
}
#endif

otError otUdpAddReceiver(otInstance *aInstance, otUdpReceiver *aUdpReceiver)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().AddReceiver(*static_cast<Ip6::UdpReceiver *>(aUdpReceiver));
}

otError otUdpRemoveReceiver(otInstance *aInstance, otUdpReceiver *aUdpReceiver)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().RemoveReceiver(*static_cast<Ip6::UdpReceiver *>(aUdpReceiver));
}

otError otUdpSendDatagram(otInstance *aInstance, otMessage *aMessage, otMessageInfo *aMessageInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().SendDatagram(*static_cast<ot::Message *>(aMessage),
                                                 *static_cast<Ip6::MessageInfo *>(aMessageInfo), Ip6::kProtoUdp);
}
