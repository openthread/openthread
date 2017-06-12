/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the TMF proxy.
 */

#define WPP_NAME "tmf_proxy.tmh"

#include <openthread/config.h>

#include "tmf_proxy.hpp"

#include <openthread/types.h>

#include "coap/coap_header.hpp"
#include "net/ip6_address.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_TMF_PROXY

namespace ot {

TmfProxy::TmfProxy(const Ip6::Address &aMeshLocal16, Coap::Coap &aCoap):
    mRelayReceive(OT_URI_PATH_RELAY_RX, &TmfProxy::HandleRelayReceive, this),
    mStreamHandler(NULL),
    mContext(NULL),
    mMeshLocal16(aMeshLocal16),
    mCoap(aCoap)
{
}

otError TmfProxy::Start(otTmfProxyStreamHandler aStreamHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mStreamHandler, error = OT_ERROR_ALREADY);

    mCoap.AddResource(mRelayReceive);
    mStreamHandler = aStreamHandler;
    mContext = aContext;

exit:
    return error;
}

otError TmfProxy::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mStreamHandler != NULL, error = OT_ERROR_ALREADY);

    mCoap.RemoveResource(mRelayReceive);

    mStreamHandler = NULL;

exit:
    return error;
}

bool TmfProxy::IsEnabled(void) const
{
    return mStreamHandler != NULL;
}

void TmfProxy::HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                  const otMessageInfo *aMessageInfo)
{
    static_cast<TmfProxy *>(aContext)->DeliverMessage(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void TmfProxy::HandleResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                              const otMessageInfo *aMessageInfo, otError aResult)
{
    VerifyOrExit(aResult == OT_ERROR_NONE);

    static_cast<TmfProxy *>(aContext)->DeliverMessage(*static_cast<Coap::Header *>(aHeader),
                                                      *static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
exit:
    return;
}

void TmfProxy::DeliverMessage(Coap::Header &aHeader, Message &aMessage,
                              const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    uint16_t rloc;
    uint16_t port;
    Message *message = NULL;

    VerifyOrExit(mStreamHandler != NULL, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit((message = aMessage.Clone()) != NULL, error = OT_ERROR_NO_BUFS);
    message->RemoveHeader(message->GetOffset() - aHeader.GetLength());

    rloc = HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]);
    port = aMessageInfo.GetPeerPort();
    mStreamHandler(message, rloc, port, mContext);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError TmfProxy::Send(Message &aMessage, uint16_t aLocator, uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(mStreamHandler != NULL, error = OT_ERROR_INVALID_STATE);

    messageInfo.SetSockAddr(mMeshLocal16);
    messageInfo.SetPeerAddr(mMeshLocal16);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aLocator);
    messageInfo.SetPeerPort(aPort);

    if (aPort == kCoapUdpPort)
    {
        // this is request to server, send with client
        error = mCoap.SendMessage(aMessage, messageInfo, TmfProxy::HandleResponse, this);
    }
    else
    {
        error = mCoap.SendMessage(aMessage, messageInfo);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        aMessage.Free();
    }

    return error;
}

}  // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_ENABLE_TMF_PROXY
