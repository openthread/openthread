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

#define WPP_NAME "secure_coap_client.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "secure_coap_client.hpp"

#include "common/logging.hpp"
#include "meshcop/dtls.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_JOINER

/**
 * @file
 *   This file implements the secure CoAP client.
 */

namespace ot {
namespace Coap {

SecureClient::SecureClient(ThreadNetif &aNetif):
    Client(aNetif, &SecureClient::Send, &SecureClient::Receive),
    mConnectedCallback(NULL),
    mContext(NULL),
    mNetif(aNetif),
    mTransmitMessage(NULL),
    mTransmitTask(aNetif.GetIp6().mTaskletScheduler, &SecureClient::HandleUdpTransmit, this)
{
}

ThreadError SecureClient::Stop(void)
{
    if (IsConnectionActive())
    {
        Disconnect();
    }

    if (mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
        mTransmitMessage = NULL;
    }

    return Client::Stop();
}

ThreadError SecureClient::Connect(const Ip6::MessageInfo &aMessageInfo, ConnectedCallback aCallback, void *aContext)
{
    mPeerAddress = aMessageInfo;
    mConnectedCallback = aCallback;
    mContext = aContext;

    return mNetif.GetDtls().Start(true, &SecureClient::HandleDtlsConnected, &SecureClient::HandleDtlsReceive,
                                  &SecureClient::HandleDtlsSend, this);
}

bool SecureClient::IsConnectionActive(void)
{
    return mNetif.GetDtls().IsStarted();
};

bool SecureClient::IsConnected(void)
{
    return mNetif.GetDtls().IsConnected();
};

ThreadError SecureClient::Disconnect(void)
{
    return mNetif.GetDtls().Stop();
}

MeshCoP::Dtls &SecureClient::GetDtls(void)
{
    return mNetif.GetDtls();
};

ThreadError SecureClient::SendMessage(Message &aMessage, otCoapResponseHandler aHandler, void *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(IsConnected(), error = kThreadError_InvalidState);

    error = Client::SendMessage(aMessage, mPeerAddress, aHandler, aContext);

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError SecureClient::Send(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureClient *>(aContext)->Send(aMessage, aMessageInfo);
}

ThreadError SecureClient::Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    (void)aMessageInfo;
    return mNetif.GetDtls().Send(aMessage, aMessage.GetLength());
}

void SecureClient::Receive(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureClient *>(aContext)->Receive(aMessage, aMessageInfo);
}

void SecureClient::Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otLogFuncEntry();

    VerifyOrExit((mPeerAddress.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                 (mPeerAddress.GetPeerPort() == aMessageInfo.GetPeerPort()));

    mNetif.GetDtls().Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

exit:
    otLogFuncExit();
}

void SecureClient::HandleDtlsConnected(void *aContext, bool aConnected)
{
    return static_cast<SecureClient *>(aContext)->HandleDtlsConnected(aConnected);
}

void SecureClient::HandleDtlsConnected(bool aConnected)
{
    if (mConnectedCallback != NULL)
    {
        mConnectedCallback(aConnected, mContext);
    }
}

void SecureClient::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<SecureClient *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void SecureClient::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    Message *message = NULL;

    otLogFuncEntry();

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL);
    SuccessOrExit(message->Append(aBuf, aLength));

    ProcessReceivedMessage(*message, mPeerAddress);

exit:

    if (message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

ThreadError SecureClient::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    return static_cast<SecureClient *>(aContext)->HandleDtlsSend(aBuf, aLength, aMessageSubType);
}

ThreadError SecureClient::HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    if (mTransmitMessage == NULL)
    {
        VerifyOrExit((mTransmitMessage = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
        mTransmitMessage->SetSubType(aMessageSubType);
        mTransmitMessage->SetLinkSecurityEnabled(false);
    }

    VerifyOrExit(mTransmitMessage->Append(aBuf, aLength) == kThreadError_None, error = kThreadError_NoBufs);

    mTransmitTask.Post();

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    otLogFuncExitErr(error);

    return error;
}

void SecureClient::HandleUdpTransmit(void *aContext)
{
    return static_cast<SecureClient *>(aContext)->HandleUdpTransmit();
}

void SecureClient::HandleUdpTransmit(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(mTransmitMessage != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = mSocket.SendTo(*mTransmitMessage, mPeerAddress));

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    mTransmitMessage = NULL;

    otLogFuncExit();
}

}  // namespace Coap
}  // namespace ot

#endif // OPENTHREAD_ENABLE_JOINER
