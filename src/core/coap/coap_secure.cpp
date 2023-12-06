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

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/new.hpp"
#include "instance/instance.hpp"
#include "meshcop/secure_transport.hpp"

#include "thread/thread_netif.hpp"

/**
 * @file
 *   This file implements the secure CoAP agent.
 */

namespace ot {
namespace Coap {

RegisterLogModule("CoapSecure");

CoapSecure::CoapSecure(Instance &aInstance, bool aLayerTwoSecurity)
    : CoapBase(aInstance, &CoapSecure::Send)
    , mDtls(aInstance, aLayerTwoSecurity)
    , mTransmitTask(aInstance, CoapSecure::HandleTransmit, this)
{
}

Error CoapSecure::Start(uint16_t aPort) { return Start(aPort, /* aMaxAttempts */ 0, nullptr, nullptr); }

Error CoapSecure::Start(uint16_t aPort, uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext)
{
    Error error;

    SuccessOrExit(error = Open(aMaxAttempts, aCallback, aContext));
    error = mDtls.Bind(aPort);

exit:
    return error;
}

Error CoapSecure::Start(MeshCoP::SecureTransport::TransportCallback aCallback, void *aContext)
{
    Error error;

    SuccessOrExit(error = Open(/* aMaxAttemps */ 0, nullptr, nullptr));
    error = mDtls.Bind(aCallback, aContext);

exit:
    return error;
}

Error CoapSecure::Open(uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext)
{
    Error error = kErrorAlready;

    SuccessOrExit(mDtls.SetMaxConnectionAttempts(aMaxAttempts, HandleDtlsAutoClose, this));
    mAutoStopCallback.Set(aCallback, aContext);
    mConnectedCallback.Clear();
    SuccessOrExit(mDtls.Open(HandleDtlsReceive, HandleDtlsConnected, this));

    error = kErrorNone;

exit:
    return error;
}

void CoapSecure::Stop(void)
{
    mDtls.Close();

    mTransmitQueue.DequeueAndFreeAll();
    ClearRequestsAndResponses();
}

Error CoapSecure::Connect(const Ip6::SockAddr &aSockAddr, ConnectedCallback aCallback, void *aContext)
{
    mConnectedCallback.Set(aCallback, aContext);

    return mDtls.Connect(aSockAddr);
}

void CoapSecure::SetPsk(const MeshCoP::JoinerPskd &aPskd)
{
    static_assert(static_cast<uint16_t>(MeshCoP::JoinerPskd::kMaxLength) <=
                      static_cast<uint16_t>(MeshCoP::SecureTransport::kPskMaxLength),
                  "The maximum length of DTLS PSK is smaller than joiner PSKd");

    SuccessOrAssert(mDtls.SetPsk(reinterpret_cast<const uint8_t *>(aPskd.GetAsCString()), aPskd.GetLength()));
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
Error CoapSecure::SendMessage(Message                    &aMessage,
                              ResponseHandler             aHandler,
                              void                       *aContext,
                              otCoapBlockwiseTransmitHook aTransmitHook,
                              otCoapBlockwiseReceiveHook  aReceiveHook)
{
    Error error = kErrorNone;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);

    error = CoapBase::SendMessage(aMessage, mDtls.GetMessageInfo(), TxParameters::GetDefault(), aHandler, aContext,
                                  aTransmitHook, aReceiveHook);

exit:
    return error;
}

Error CoapSecure::SendMessage(Message                    &aMessage,
                              const Ip6::MessageInfo     &aMessageInfo,
                              ResponseHandler             aHandler,
                              void                       *aContext,
                              otCoapBlockwiseTransmitHook aTransmitHook,
                              otCoapBlockwiseReceiveHook  aReceiveHook)
{
    return CoapBase::SendMessage(aMessage, aMessageInfo, TxParameters::GetDefault(), aHandler, aContext, aTransmitHook,
                                 aReceiveHook);
}
#else  // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
Error CoapSecure::SendMessage(Message &aMessage, ResponseHandler aHandler, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);

    error = CoapBase::SendMessage(aMessage, mDtls.GetMessageInfo(), aHandler, aContext);

exit:
    return error;
}

Error CoapSecure::SendMessage(Message                &aMessage,
                              const Ip6::MessageInfo &aMessageInfo,
                              ResponseHandler         aHandler,
                              void                   *aContext)
{
    return CoapBase::SendMessage(aMessage, aMessageInfo, aHandler, aContext);
}
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error CoapSecure::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    mTransmitQueue.Enqueue(aMessage);
    mTransmitTask.Post();

    return kErrorNone;
}

void CoapSecure::HandleDtlsConnected(void *aContext, bool aConnected)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsConnected(aConnected);
}

void CoapSecure::HandleDtlsConnected(bool aConnected) { mConnectedCallback.InvokeIfSet(aConnected); }

void CoapSecure::HandleDtlsAutoClose(void *aContext)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsAutoClose();
}

void CoapSecure::HandleDtlsAutoClose(void)
{
    Stop();
    mAutoStopCallback.InvokeIfSet();
}

void CoapSecure::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<CoapSecure *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void CoapSecure::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    ot::Message *message = nullptr;

    VerifyOrExit((message = Get<MessagePool>().Allocate(Message::kTypeIp6, Message::GetHelpDataReserved())) != nullptr);
    SuccessOrExit(message->AppendBytes(aBuf, aLength));

    CoapBase::Receive(*message, mDtls.GetMessageInfo());

exit:
    FreeMessage(message);
}

void CoapSecure::HandleTransmit(Tasklet &aTasklet)
{
    static_cast<CoapSecure *>(static_cast<TaskletContext &>(aTasklet).GetContext())->HandleTransmit();
}

void CoapSecure::HandleTransmit(void)
{
    Error        error   = kErrorNone;
    ot::Message *message = mTransmitQueue.GetHead();

    VerifyOrExit(message != nullptr);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != nullptr)
    {
        mTransmitTask.Post();
    }

    SuccessOrExit(error = mDtls.Send(*message, message->GetLength()));

exit:
    if (error != kErrorNone)
    {
        LogNote("Transmit: %s", ErrorToString(error));
        message->Free();
    }
    else
    {
        LogDebg("Transmit: %s", ErrorToString(error));
    }
}

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
