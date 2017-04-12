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
 *   This file implements a BorderAgentProxy role.
 */

#define WPP_NAME "commissioner.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "openthread/platform/random.h"

#include <common/logging.hpp>
#include <coap/coap_header.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <openthread/types.h>

using Thread::Encoding::BigEndian::HostSwap64;

namespace Thread {
namespace MeshCoP {

class BorderAgentProxyMeta
{
public:
    BorderAgentProxyMeta(const otIp6Address &aAddress, uint16_t aPort) {
        mAddress = aAddress;
        mPort = HostSwap16(aPort);
    }

    BorderAgentProxyMeta(const Message &aMessage) {
        aMessage.Read(aMessage.GetLength() - sizeof(mAddress) - sizeof(mPort), sizeof(mAddress), mAddress.mFields.m8);
        aMessage.Read(aMessage.GetLength() - sizeof(mPort), sizeof(mPort), &mPort);
    }

    const Ip6::Address &GetAddress() {
        return *static_cast<Ip6::Address *>(&mAddress);
    }

    uint16_t GetPort() {
        return HostSwap16(mPort);
    }

    const uint8_t *GetBytes() {
        return (uint8_t *)this;
    }

private:
    otIp6Address mAddress;
    uint16_t     mPort;
};

BorderAgentProxy::BorderAgentProxy(otInstance *aInstance, Coap::Server &aCoapServer, Coap::Client &aCoapClient):
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &BorderAgentProxy::HandleRelayReceive, this),
    mBorderAgentProxyCallback(NULL), mContext(NULL),
    mInstance(aInstance),
    mCoapServer(aCoapServer), mCoapClient(aCoapClient)
{
}

ThreadError BorderAgentProxy::Start(otBorderAgentProxyCallback aBorderAgentProxyCallback, void *aContext)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!mBorderAgentProxyCallback, error = kThreadError_InvalidState);

    mCoapServer.AddResource(mRelayReceive);
    mBorderAgentProxyCallback = aBorderAgentProxyCallback;
    mContext = aContext;

exit:
    otLogInfoMle(mInstance, "border agent proxy started. error=%d", error);
    return error;
}

ThreadError BorderAgentProxy::Stop(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mBorderAgentProxyCallback != NULL, error = kThreadError_InvalidState);

    mCoapServer.RemoveResource(mRelayReceive);

    mBorderAgentProxyCallback = NULL;

exit:
    return error;
}

bool BorderAgentProxy::IsEnabled(void) const
{
    return mBorderAgentProxyCallback != NULL;
}

void BorderAgentProxy::HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                          const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgentProxy *>(aContext)->HandleRelayReceive(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgentProxy::HandleRelayReceive(Coap::Header &aHeader, Message &aMessage,
                                          const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    Message *message;

    VerifyOrExit(mBorderAgentProxyCallback != NULL, error = kThreadError_InvalidState);

    VerifyOrExit((message = aMessage.Clone()) != NULL, error = kThreadError_NoBufs);

    {
        message->RemoveHeader(message->GetOffset() - aHeader.GetLength());
        BorderAgentProxyMeta meta(aMessageInfo.GetPeerAddr(), aMessageInfo.GetPeerPort());
        SuccessOrExit(error = message->Append(meta.GetBytes(), sizeof(meta)));
        mBorderAgentProxyCallback(message, mContext);
    }

exit:

    (void)aHeader;

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogInfoMle(mInstance, "deliver to host error=%d", error);
    return;
}

void BorderAgentProxy::HandleResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<BorderAgentProxy *>(aContext)->HandleResponse(*static_cast<Coap::Header *>(aHeader),
                                                              *static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void BorderAgentProxy::HandleResponse(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                      ThreadError aResult)
{
    otLogInfoMle(mInstance, "response wrong error=%d", aResult);

    if (aResult == kThreadError_None)
    {
        HandleRelayReceive(aHeader, aMessage, aMessageInfo);
    }
}

ThreadError BorderAgentProxy::Send(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;

    (void)mInstance;
    otLogInfoMle(mInstance, "-------------%s", __func__);

    BorderAgentProxyMeta meta(aMessage);
    aMessage.SetLength(aMessage.GetLength() - sizeof(meta));

    messageInfo.SetPeerAddr(meta.GetAddress());
    messageInfo.SetPeerPort(meta.GetPort());

    if (meta.GetPort() == kCoapUdpPort)
    {
        // this is request to server, send with client
        error = mCoapClient.SendMessage(aMessage, messageInfo, BorderAgentProxy::HandleResponse, this);
    }
    else
    {
        error = mCoapServer.SendMessage(aMessage, messageInfo);
    }

    return error;
}

}  // namespace MeshCoP
}  // namespace Thread
