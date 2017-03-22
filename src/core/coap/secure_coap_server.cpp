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

#define WPP_NAME "secure_coap_server.tmh"

#include <coap/secure_coap_server.hpp>
#include <common/logging.hpp>
#include <meshcop/dtls.hpp>
#include <thread/thread_netif.hpp>

/**
 * @file
 *   This file implements the secure CoAP server.
 */

namespace Thread {
namespace Coap {

SecureServer::SecureServer(ThreadNetif &aNetif, uint16_t aPort):
    Server(aNetif.GetIp6().mUdp, aPort, &SecureServer::Send, &SecureServer::Receive),
    mTransmitCallback(NULL),
    mContext(NULL),
    mNetif(aNetif),
    mTransmitMessage(NULL),
    mTransmitTask(aNetif.GetIp6().mTaskletScheduler, &SecureServer::HandleUdpTransmit, this)
{
}

ThreadError SecureServer::Start(TransportCallback aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    mTransmitCallback = aCallback;
    mContext = aContext;

    // Passing mTransmitCallback means that we do not want to use socket
    // to transmit/receive messages, so do not open it in that case.
    if (mTransmitCallback == NULL)
    {
        error = Server::Start();
    }

    return error;
}

ThreadError SecureServer::Stop()
{
    if (mNetif.GetDtls().IsStarted())
    {
        mNetif.GetDtls().Stop();
    }

    if (mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
        mTransmitMessage = NULL;
    }

    mTransmitCallback = NULL;
    mContext = NULL;

    otLogFuncExit();

    return Server::Stop();
}

bool SecureServer::IsConnectionActive(void)
{
    return mNetif.GetDtls().IsStarted();
};

ThreadError SecureServer::Send(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureServer *>(aContext)->Send(aMessage, aMessageInfo);
}

ThreadError SecureServer::Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    (void)aMessageInfo;
    return mNetif.GetDtls().Send(aMessage, aMessage.GetLength());
}

void SecureServer::Receive(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureServer *>(aContext)->Receive(aMessage, aMessageInfo);
}

void SecureServer::Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otLogFuncEntry();
    
    if (!mNetif.GetDtls().IsStarted())
    {
        mPeerAddress.SetPeerAddr(aMessageInfo.GetPeerAddr());
        mPeerAddress.SetPeerPort(aMessageInfo.GetPeerPort());
        mPeerAddress.SetSockAddr(aMessageInfo.GetSockAddr());
        mPeerAddress.SetSockPort(aMessageInfo.GetSockPort());
        mNetif.GetDtls().Start(false, HandleDtlsConnected, HandleDtlsReceive, HandleDtlsSend, this);
    }
    else
    {
        // Once DTLS session is started, communicate only with a peer.
        VerifyOrExit((mPeerAddress.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                     (mPeerAddress.GetPeerPort() == aMessageInfo.GetPeerPort()), ;);
    }

    mNetif.GetDtls().SetClientId(mPeerAddress.GetPeerAddr().mFields.m8,
                                 sizeof(mPeerAddress.GetPeerAddr().mFields));
    mNetif.GetDtls().Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

exit:
    otLogFuncExit();
}

ThreadError SecureServer::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    return mNetif.GetDtls().SetPsk(aPsk, aPskLength);
}

void SecureServer::HandleDtlsConnected(void *aContext, bool aConnected)
{
    return static_cast<SecureServer *>(aContext)->HandleDtlsConnected(aConnected);
}

void SecureServer::HandleDtlsConnected(bool aConnected)
{
    (void)aConnected;
}

void SecureServer::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<SecureServer *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void SecureServer::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    Message *message;

    otLogFuncEntry();

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL, ;);
    SuccessOrExit(message->Append(aBuf, aLength));

    ProcessReceivedMessage(*message, mPeerAddress);

exit:

    if (message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

ThreadError SecureServer::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<SecureServer *>(aContext)->HandleDtlsSend(aBuf, aLength);
}

ThreadError SecureServer::HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    if (mTransmitMessage == NULL)
    {
        VerifyOrExit((mTransmitMessage = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
#ifdef OPENTHREAD_ENABLE_BORDER_AGENT
        mTransmitMessage->SetLinkSecurityEnabled(true);
#else
        mTransmitMessage->SetLinkSecurityEnabled(false);
#endif
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

void SecureServer::HandleUdpTransmit(void *aContext)
{
    return static_cast<SecureServer *>(aContext)->HandleUdpTransmit();
}

void SecureServer::HandleUdpTransmit(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(mTransmitMessage != NULL, error = kThreadError_NoBufs);

    if (mTransmitCallback)
    {
        SuccessOrExit(error = mTransmitCallback(mContext, *mTransmitMessage, mPeerAddress));
    }
    else
    {
        SuccessOrExit(error = mSocket.SendTo(*mTransmitMessage, mPeerAddress));
    }

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    mTransmitMessage = NULL;

    otLogFuncExit();
}

}  // namespace Coap
}  // namespace Thread
