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

    return instance.Get<Ip6::Udp>().NewMessage(0, Message::Settings(aSettings));
}

otError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().Open(*static_cast<Ip6::Udp::SocketHandle *>(aSocket), aCallback, aContext);
}

otError otUdpClose(otInstance *aInstance, otUdpSocket *aSocket)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().Close(*static_cast<Ip6::Udp::SocketHandle *>(aSocket));
}

otError otUdpBind(otInstance *aInstance, otUdpSocket *aSocket, const otSockAddr *aSockName)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().Bind(*static_cast<Ip6::Udp::SocketHandle *>(aSocket),
                                         *static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otUdpConnect(otInstance *aInstance, otUdpSocket *aSocket, const otSockAddr *aSockName)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().Connect(*static_cast<Ip6::Udp::SocketHandle *>(aSocket),
                                            *static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otUdpSend(otInstance *aInstance, otUdpSocket *aSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().SendTo(*static_cast<Ip6::Udp::SocketHandle *>(aSocket),
                                           *static_cast<Message *>(aMessage),
                                           *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

otUdpSocket *otUdpGetSockets(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().GetUdpSockets();
}

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
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

    OT_ASSERT(aMessage != nullptr && aPeerAddr != nullptr);

    messageInfo.SetSockAddr(instance.Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetSockPort(aSockPort);
    messageInfo.SetPeerAddr(*static_cast<const ot::Ip6::Address *>(aPeerAddr));
    messageInfo.SetPeerPort(aPeerPort);
    messageInfo.SetIsHostInterface(true);

    instance.Get<Ip6::Udp>().HandlePayload(*static_cast<ot::Message *>(aMessage), messageInfo);

    static_cast<ot::Message *>(aMessage)->Free();
}
#endif // OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE

otError otUdpAddReceiver(otInstance *aInstance, otUdpReceiver *aUdpReceiver)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().AddReceiver(*static_cast<Ip6::Udp::Receiver *>(aUdpReceiver));
}

otError otUdpRemoveReceiver(otInstance *aInstance, otUdpReceiver *aUdpReceiver)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().RemoveReceiver(*static_cast<Ip6::Udp::Receiver *>(aUdpReceiver));
}

otError otUdpSendDatagram(otInstance *aInstance, otMessage *aMessage, otMessageInfo *aMessageInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Ip6::Udp>().SendDatagram(*static_cast<ot::Message *>(aMessage),
                                                 *static_cast<Ip6::MessageInfo *>(aMessageInfo), Ip6::kProtoUdp);
}
