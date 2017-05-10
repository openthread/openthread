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

#include  " openthread_enable_defines.h"

#include "openthread/udp.h"

#include "openthread-instance.h"

using namespace ot;

otMessage *otUdpNewMessage(otInstance *aInstance, bool aLinkSecurityEnabled)
{
    Message *message = aInstance->mIp6.mUdp.NewMessage(0);

    if (message)
    {
        message->SetLinkSecurityEnabled(aLinkSecurityEnabled);
    }

    return message;
}

ThreadError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aCallbackContext)
{
    ThreadError error = kThreadError_InvalidArgs;
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);

    if (socket->mTransport == NULL)
    {
        socket->mTransport = &aInstance->mIp6.mUdp;
        error = socket->Open(aCallback, aCallbackContext);
    }

    return error;
}

ThreadError otUdpClose(otUdpSocket *aSocket)
{
    ThreadError error = kThreadError_InvalidState;
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);

    if (socket->mTransport != NULL)
    {
        error = socket->Close();

        if (error == kThreadError_None)
        {
            socket->mTransport = NULL;
        }
    }

    return error;
}

ThreadError otUdpBind(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Bind(*static_cast<const Ip6::SockAddr *>(aSockName));
}

ThreadError otUdpConnect(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Connect(*static_cast<const Ip6::SockAddr *>(aSockName));
}

ThreadError otUdpSend(otUdpSocket *aSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);
    return socket->SendTo(*static_cast<Message *>(aMessage),
                          *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}
