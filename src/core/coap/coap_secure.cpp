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

#include "coap_secure.hpp"

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "meshcop/dtls.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_DTLS_ENABLE

/**
 * @file
 *   This file implements the secure CoAP agent.
 */

namespace ot {
namespace Coap {

CoapSecure::CoapSecure(Instance &aInstance, bool aLayerTwoSecurity)
    : CoapBase(aInstance, &CoapSecure::Send)
    , mDtls(aInstance, aLayerTwoSecurity)
    , mConnectedCallback(NULL)
    , mConnectedContext(NULL)
    , mTransmitQueue()
    , mTransmitTask(aInstance, &CoapSecure::HandleTransmit, this)
{
}

otError CoapSecure::Start(uint16_t aPort)
{
    otError error = OT_ERROR_NONE;

    mConnectedCallback = NULL;
    mConnectedContext  = NULL;

    SuccessOrExit(error = mDtls.Open(&CoapSecure::HandleDtlsReceive, &CoapSecure::HandleDtlsConnected, this));
    SuccessOrExit(error = mDtls.Bind(aPort));

exit:
    return error;
}

otError CoapSecure::Start(MeshCoP::Dtls::TransportCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;

    mConnectedCallback = NULL;
    mConnectedContext  = NULL;

    SuccessOrExit(error = mDtls.Open(&CoapSecure::HandleDtlsReceive, &CoapSecure::HandleDtlsConnected, this));
    SuccessOrExit(error = mDtls.Bind(aCallback, aContext));

exit:
    return error;
}

void CoapSecure::SetConnectedCallback(ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;
}

void CoapSecure::Stop(void)
{
    mDtls.Close();

    for (ot::Message *message = mTransmitQueue.GetHead(); message != NULL; message = message->GetNext())
    {
        mTransmitQueue.Dequeue(*message);
        message->Free();
    }

    ClearRequestsAndResponses();
}

otError CoapSecure::Connect(const Ip6::SockAddr &aSockAddr, ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;

    return mDtls.Connect(aSockAddr);
}

otError CoapSecure::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    return mDtls.SetPsk(aPsk, aPskLength);
}

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
void CoapSecure::SetCertificate(const uint8_t *aX509Cert,
                                uint32_t       aX509Length,
                                const uint8_t *aPrivateKey,
                                uint32_t       aPrivateKeyLength)
{
    mDtls.SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
}

void CoapSecure::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    mDtls.SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_BASE64_C
otError CoapSecure::GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
{
    return mDtls.GetPeerCertificateBase64(aPeerCert, aCertLength, aCertBufferSize);
}
#endif // MBEDTLS_BASE64_C

void CoapSecure::SetClientConnectedCallback(ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback = aCallback;
    mConnectedContext  = aContext;
}

void CoapSecure::SetSslAuthMode(bool aVerifyPeerCertificate)
{
    mDtls.SetSslAuthMode(aVerifyPeerCertificate);
}

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

otError CoapSecure::SendMessage(Message &aMessage, otCoapResponseHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsConnected(), error = OT_ERROR_INVALID_STATE);

    error = CoapBase::SendMessage(aMessage, mDtls.GetPeerAddress(), aHandler, aContext);

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

otError CoapSecure::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError error;

    SuccessOrExit(error = mTransmitQueue.Enqueue(aMessage));
    mTransmitTask.Post();

exit:
    return error;
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
    ot::Message *message = NULL;

    VerifyOrExit((message = Get<MessagePool>().New(Message::kTypeIp6, Message::GetHelpDataReserved())) != NULL);
    SuccessOrExit(message->Append(aBuf, aLength));

    CoapBase::Receive(*message, mDtls.GetPeerAddress());

exit:

    if (message != NULL)
    {
        message->Free();
    }
}

void CoapSecure::HandleTransmit(Tasklet &aTasklet)
{
    static_cast<CoapSecure *>(static_cast<TaskletContext &>(aTasklet).GetContext())->HandleTransmit();
}

void CoapSecure::HandleTransmit(void)
{
    otError      error   = OT_ERROR_NONE;
    ot::Message *message = mTransmitQueue.GetHead();

    VerifyOrExit(message != NULL);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != NULL)
    {
        mTransmitTask.Post();
    }

    SuccessOrExit(error = mDtls.Send(*message, message->GetLength()));

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

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_DTLS_ENABLE
