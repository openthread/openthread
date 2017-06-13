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

#define WPP_NAME "coap_secure.tmh"

#include <openthread/config.h>

#include "coap_secure.hpp"

#include "common/logging.hpp"
#include "meshcop/dtls.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DTLS

/**
 * @file
 *   This file implements the secure CoAP agent.
 */

namespace ot {
namespace Coap {

CoapSecure::CoapSecure(ThreadNetif &aNetif) :
    Coap(aNetif),
    mConnectedCallback(NULL),
    mConnectedContext(NULL),
    mTransportCallback(NULL),
    mTransportContext(NULL),
    mTransmitMessage(NULL),
    mTransmitTask(aNetif.GetIp6().mTaskletScheduler, &CoapSecure::HandleUdpTransmit, this)
{
}

otError CoapSecure::Start(uint16_t aPort, TransportCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;

    mTransportCallback = aCallback;
    mTransportContext = aContext;

    // Passing mTransportCallback means that we do not want to use socket
    // to transmit/receive messages, so do not open it in that case.
    if (mTransportCallback == NULL)
    {
        error = Coap::Start(aPort);
    }

    return error;
}

otError CoapSecure::Stop(void)
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

    mTransportCallback = NULL;
    mTransportContext = NULL;

    return Coap::Stop();
}

otError CoapSecure::Connect(const Ip6::MessageInfo &aMessageInfo, ConnectedCallback aCallback, void *aContext)
{
    mPeerAddress = aMessageInfo;
    mConnectedCallback = aCallback;
    mConnectedContext = aContext;

    return mNetif.GetDtls().Start(true, &CoapSecure::HandleDtlsConnected, &CoapSecure::HandleDtlsReceive,
                                  &CoapSecure::HandleDtlsSend, this);
}

bool CoapSecure::IsConnectionActive(void)
{
    return mNetif.GetDtls().IsStarted();
}

bool CoapSecure::IsConnected(void)
{
    return mNetif.GetDtls().IsConnected();
}

otError CoapSecure::Disconnect(void)
{
    return mNetif.GetDtls().Stop();
}

MeshCoP::Dtls &CoapSecure::GetDtls(void)
{
    return mNetif.GetDtls();
}

otError CoapSecure::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    return mNetif.GetDtls().SetPsk(aPsk, aPskLength);
}

otError CoapSecure::SendMessage(Message &aMessage, otCoapResponseHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    VerifyOrExit(IsConnected(), error = OT_ERROR_INVALID_STATE);

    error = Coap::SendMessage(aMessage, mPeerAddress, aHandler, aContext);

exit:
    otLogFuncExitErr(error);
    return error;
}

otError CoapSecure::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                otCoapResponseHandler aHandler, void *aContext)
{
    return Coap::SendMessage(aMessage, aMessageInfo, aHandler, aContext);
}

otError CoapSecure::Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    (void)aMessageInfo;
    return mNetif.GetDtls().Send(aMessage, aMessage.GetLength());
}

void CoapSecure::Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otLogFuncEntry();

    if (!mNetif.GetDtls().IsStarted())
    {
        Ip6::SockAddr sockAddr;
        sockAddr.mAddress = aMessageInfo.GetPeerAddr();
        sockAddr.mPort = aMessageInfo.GetPeerPort();
        mSocket.Connect(sockAddr);

        mPeerAddress.SetPeerAddr(aMessageInfo.GetPeerAddr());
        mPeerAddress.SetPeerPort(aMessageInfo.GetPeerPort());

        if (mNetif.IsUnicastAddress(aMessageInfo.GetSockAddr()))
        {
            mPeerAddress.SetSockAddr(aMessageInfo.GetSockAddr());
        }

        mPeerAddress.SetSockPort(aMessageInfo.GetSockPort());

        mNetif.GetDtls().Start(false, HandleDtlsConnected, HandleDtlsReceive, HandleDtlsSend, this);
    }
    else
    {
        // Once DTLS session is started, communicate only with a peer.
        VerifyOrExit((mPeerAddress.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                     (mPeerAddress.GetPeerPort() == aMessageInfo.GetPeerPort()));
    }

    mNetif.GetDtls().SetClientId(mPeerAddress.GetPeerAddr().mFields.m8,
                                 sizeof(mPeerAddress.GetPeerAddr().mFields));
    mNetif.GetDtls().Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

exit:
    otLogFuncExit();
}

void CoapSecure::HandleDtlsConnected(void *aContext, bool aConnected)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsConnected(aConnected);
}

void CoapSecure::HandleDtlsConnected(bool aConnected)
{
    if (mConnectedCallback != NULL)
    {
        mConnectedCallback(aConnected, mConnectedContext);
    }
}

void CoapSecure::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void CoapSecure::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    Message *message = NULL;

    otLogFuncEntry();

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL);
    SuccessOrExit(message->Append(aBuf, aLength));

    Coap::Receive(*message, mPeerAddress);

exit:

    if (message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

otError CoapSecure::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsSend(aBuf, aLength, aMessageSubType);
}

otError CoapSecure::HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    if (mTransmitMessage == NULL)
    {
        VerifyOrExit((mTransmitMessage = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
        mTransmitMessage->SetSubType(aMessageSubType);
        mTransmitMessage->SetLinkSecurityEnabled(false);
    }

    // Set message sub type in case Joiner Finalize Response is appended to the message.
    if (aMessageSubType != Message::kSubTypeNone)
    {
        mTransmitMessage->SetSubType(aMessageSubType);
    }

    VerifyOrExit(mTransmitMessage->Append(aBuf, aLength) == OT_ERROR_NONE, error = OT_ERROR_NO_BUFS);

    mTransmitTask.Post();

exit:

    if ((error != OT_ERROR_NONE) && (mTransmitMessage != NULL))
    {
        mTransmitMessage->Free();
    }

    otLogFuncExitErr(error);

    return error;
}

void CoapSecure::HandleUdpTransmit(void *aContext)
{
    return static_cast<CoapSecure *>(aContext)->HandleUdpTransmit();
}

void CoapSecure::HandleUdpTransmit(void)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    VerifyOrExit(mTransmitMessage != NULL, error = OT_ERROR_NO_BUFS);

    if (mTransportCallback)
    {
        SuccessOrExit(error = mTransportCallback(mTransportContext, *mTransmitMessage, mPeerAddress));
    }
    else
    {
        SuccessOrExit(error = mSocket.SendTo(*mTransmitMessage, mPeerAddress));
    }

exit:

    if ((error != OT_ERROR_NONE) && (mTransmitMessage != NULL))
    {
        mTransmitMessage->Free();
    }

    mTransmitMessage = NULL;

    otLogFuncExit();
}

} // namespace Coap
} // namespace ot

#endif  // OPENTHREAD_ENABLE_DTLS
