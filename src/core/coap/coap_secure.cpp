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

#include "coap_secure.hpp"

#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/dtls.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DTLS

/**
 * @file
 *   This file implements the secure CoAP agent.
 */

namespace ot {
namespace Coap {

CoapSecure::CoapSecure(Instance &aInstance)
    : CoapBase(aInstance, &CoapSecure::HandleRetransmissionTimer, &CoapSecure::HandleResponsesQueueTimer)
    , mConnectedCallback(NULL)
    , mConnectedContext(NULL)
    , mTransportCallback(NULL)
    , mTransportContext(NULL)
    , mTransmitQueue()
    , mTransmitTask(aInstance, &CoapSecure::HandleTransmit, this)
    , mLayerTwoSecurity(false)
{
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
CoapSecure::CoapSecure(Instance &       aInstance,
                       Tasklet::Handler aHandleTransmit,
                       Timer::Handler   aRetransmissionTimer,
                       Timer::Handler   aResponsesQueueTimer)
    : CoapBase(aInstance, aRetransmissionTimer, aResponsesQueueTimer)
    , mConnectedCallback(NULL)
    , mConnectedContext(NULL)
    , mTransportCallback(NULL)
    , mTransportContext(NULL)
    , mTransmitQueue()
    , mTransmitTask(aInstance, aHandleTransmit, this)
    , mLayerTwoSecurity(true)
{
}
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

otError CoapSecure::Start(uint16_t aPort, TransportCallback aCallback, void *aContext)
{
    otError error      = OT_ERROR_NONE;
    mTransportCallback = aCallback;
    mTransportContext  = aContext;
    mConnectedCallback = NULL;
    mConnectedContext  = NULL;

    // Passing mTransportCallback means that we do not want to use socket
    // to transmit/receive messages, so do not open it in that case.
    if (mTransportCallback == NULL)
    {
        error = CoapBase::Start(aPort);
    }

    return error;
}

void CoapSecure::SetConnectedCallback(ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;
}

otError CoapSecure::Stop(void)
{
    if (IsConnectionActive())
    {
        Disconnect();
    }

    for (Message *message = mTransmitQueue.GetHead(); message != NULL; message = message->GetNext())
    {
        mTransmitQueue.Dequeue(*message);
        message->Free();
    }

    mTransportCallback = NULL;
    mTransportContext  = NULL;

    return CoapBase::Stop();
}

otError CoapSecure::Connect(const Ip6::SockAddr &aSockAddr, ConnectedCallback aCallback, void *aContext)
{
    memcpy(&mPeerAddress.mPeerAddr, &aSockAddr.mAddress, sizeof(mPeerAddress.mPeerAddr));
    mPeerAddress.mPeerPort = aSockAddr.mPort;

    if (aSockAddr.GetAddress().IsLinkLocal() || aSockAddr.GetAddress().IsMulticast())
    {
        mPeerAddress.mInterfaceId = aSockAddr.mScopeId;
    }
    else
    {
        mPeerAddress.mInterfaceId = 0;
    }

    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;

    return GetNetif().GetDtls().Start(true, &CoapSecure::HandleDtlsConnected, &CoapSecure::HandleDtlsReceive,
                                      &CoapSecure::HandleDtlsSend, this);
}

bool CoapSecure::IsConnectionActive(void)
{
    return GetNetif().GetDtls().IsStarted();
}

bool CoapSecure::IsConnected(void)
{
    return GetNetif().GetDtls().IsConnected();
}

otError CoapSecure::Disconnect(void)
{
    Ip6::SockAddr sockAddr;
    otError       error = OT_ERROR_NONE;

    SuccessOrExit(error = GetNetif().GetDtls().Stop());
    // Disconnect from previous peer by connecting to any address
    SuccessOrExit(error = mSocket.Connect(sockAddr));

exit:
    return error;
}

MeshCoP::Dtls &CoapSecure::GetDtls(void)
{
    return GetNetif().GetDtls();
}

otError CoapSecure::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    return GetNetif().GetDtls().SetPsk(aPsk, aPskLength);
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
otError CoapSecure::SetCertificate(const uint8_t *aX509Cert,
                                   uint32_t       aX509Length,
                                   const uint8_t *aPrivateKey,
                                   uint32_t       aPrivateKeyLength)
{
    return GetNetif().GetDtls().SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
}

otError CoapSecure::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLenth)
{
    return GetNetif().GetDtls().SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLenth);
}
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
otError CoapSecure::SetPreSharedKey(const uint8_t *aPsk,
                                    uint16_t       aPskLength,
                                    const uint8_t *aPskIdentity,
                                    uint16_t       aPskIdLength)
{
    return GetNetif().GetDtls().SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_BASE64_C
otError CoapSecure::GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
{
    return GetNetif().GetDtls().GetPeerCertificateBase64(aPeerCert, aCertLength, aCertBufferSize);
}
#endif // MBEDTLS_BASE64_C

void CoapSecure::SetClientConnectedCallback(ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;
}

void CoapSecure::SetSslAuthMode(bool aVerifyPeerCertificate)
{
    GetNetif().GetDtls().SetSslAuthMode(aVerifyPeerCertificate);
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

otError CoapSecure::SendMessage(Message &aMessage, otCoapResponseHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsConnected(), error = OT_ERROR_INVALID_STATE);

    error = CoapBase::SendMessage(aMessage, mPeerAddress, aHandler, aContext);

exit:
    return error;
}

otError CoapSecure::SendMessage(Message &               aMessage,
                                const Ip6::MessageInfo &aMessageInfo,
                                otCoapResponseHandler   aHandler,
                                void *                  aContext)
{
    return CoapBase::SendMessage(aMessage, aMessageInfo, aHandler, aContext);
}

otError CoapSecure::Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    mTransmitQueue.Enqueue(aMessage);
    mTransmitTask.Post();
    return OT_ERROR_NONE;
}

void CoapSecure::Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();

    if (!netif.GetDtls().IsStarted())
    {
        Ip6::SockAddr sockAddr;
        sockAddr.mAddress = aMessageInfo.GetPeerAddr();
        sockAddr.mPort    = aMessageInfo.GetPeerPort();
        mSocket.Connect(sockAddr);

        mPeerAddress.SetPeerAddr(aMessageInfo.GetPeerAddr());
        mPeerAddress.SetPeerPort(aMessageInfo.GetPeerPort());
        mPeerAddress.SetInterfaceId(aMessageInfo.GetInterfaceId());

        if (netif.IsUnicastAddress(aMessageInfo.GetSockAddr()))
        {
            mPeerAddress.SetSockAddr(aMessageInfo.GetSockAddr());
        }

        mPeerAddress.SetSockPort(aMessageInfo.GetSockPort());

        VerifyOrExit(netif.GetDtls().Start(false, &CoapSecure::HandleDtlsConnected, &CoapSecure::HandleDtlsReceive,
                                           CoapSecure::HandleDtlsSend, this) == OT_ERROR_NONE);
    }
    else
    {
        // Once DTLS session is started, communicate only with a peer.
        VerifyOrExit((mPeerAddress.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                     (mPeerAddress.GetPeerPort() == aMessageInfo.GetPeerPort()));
    }

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER
    netif.GetDtls().SetClientId(mPeerAddress.GetPeerAddr().mFields.m8, sizeof(mPeerAddress.GetPeerAddr().mFields));
#endif

    netif.GetDtls().Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

exit:
    return;
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

    VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kTypeIp6, 0)) != NULL);
    SuccessOrExit(message->Append(aBuf, aLength));

    CoapBase::Receive(*message, mPeerAddress);

exit:

    if (message != NULL)
    {
        message->Free();
    }
}

otError CoapSecure::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsSend(aBuf, aLength, aMessageSubType);
}

otError CoapSecure::HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = NULL;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(aMessageSubType);
    message->SetLinkSecurityEnabled(mLayerTwoSecurity);

    SuccessOrExit(error = message->Append(aBuf, aLength));

    // Set message sub type in case Joiner Finalize Response is appended to the message.
    if (aMessageSubType != Message::kSubTypeNone)
    {
        message->SetSubType(aMessageSubType);
    }

    if (mTransportCallback)
    {
        SuccessOrExit(error = mTransportCallback(mTransportContext, *message, mPeerAddress));
    }
    else
    {
        SuccessOrExit(error = mSocket.SendTo(*message, mPeerAddress));
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void CoapSecure::HandleTransmit(Tasklet &aTasklet)
{
    aTasklet.GetOwner<CoapSecure>().HandleTransmit();
}

void CoapSecure::HandleTransmit(void)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = mTransmitQueue.GetHead();

    VerifyOrExit(message != NULL);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != NULL)
    {
        mTransmitTask.Post();
    }

    SuccessOrExit(error = GetDtls().Send(*message, message->GetLength()));

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogNoteMeshCoP("CoapSecure Transmit: %s", otThreadErrorToString(error));
        message->Free();
    }
    else
    {
        otLogDebgMeshCoP("CoapSecure Transmit: %s", otThreadErrorToString(error));
    }
}

void CoapSecure::HandleRetransmissionTimer(Timer &aTimer)
{
    aTimer.GetOwner<CoapSecure>().CoapBase::HandleRetransmissionTimer();
}

void CoapSecure::HandleResponsesQueueTimer(Timer &aTimer)
{
    aTimer.GetOwner<CoapSecure>().CoapBase::HandleResponsesQueueTimer();
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

ApplicationCoapSecure::ApplicationCoapSecure(Instance &aInstance)
    : CoapSecure(aInstance,
                 &ApplicationCoapSecure::HandleTransmit,
                 &ApplicationCoapSecure::HandleRetransmissionTimer,
                 &ApplicationCoapSecure::HandleResponsesQueueTimer)
{
}

void ApplicationCoapSecure::HandleTransmit(Tasklet &aTasklet)
{
    aTasklet.GetOwner<ApplicationCoapSecure>().CoapSecure::HandleTransmit();
}

void ApplicationCoapSecure::HandleRetransmissionTimer(Timer &aTimer)
{
    aTimer.GetOwner<ApplicationCoapSecure>().CoapBase::HandleRetransmissionTimer();
}

void ApplicationCoapSecure::HandleResponsesQueueTimer(Timer &aTimer)
{
    aTimer.GetOwner<ApplicationCoapSecure>().CoapBase::HandleResponsesQueueTimer();
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_ENABLE_DTLS
