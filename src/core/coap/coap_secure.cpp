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

#include "instance/instance.hpp"

/**
 * @file
 *   This file implements the secure CoAP session.
 */

namespace ot {
namespace Coap {

RegisterLogModule("CoapSecure");

SecureSession::SecureSession(Instance &aInstance, Dtls::Transport &aDtlsTransport)
    : CoapBase(aInstance, Transmit)
    , Dtls::Session(aDtlsTransport)
    , mTransmitTask(aInstance, HandleTransmitTask, this)
{
    Dtls::Session::SetConnectCallback(HandleDtlsConnectEvent, this);
    Dtls::Session::SetReceiveCallback(HandleDtlsReceive, this);
}

void SecureSession::Cleanup(void)
{
    ClearAllRequestsAndResponses();
    mTransmitQueue.DequeueAndFreeAll();
    mTransmitTask.Unpost();
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error SecureSession::SendMessage(Message                    &aMessage,
                                 ResponseHandler             aHandler,
                                 void                       *aContext,
                                 otCoapBlockwiseTransmitHook aTransmitHook,
                                 otCoapBlockwiseReceiveHook  aReceiveHook)
{
    return IsConnected() ? CoapBase::SendMessage(aMessage, GetMessageInfo(), TxParameters::GetDefault(), aHandler,
                                                 aContext, aTransmitHook, aReceiveHook)
                         : kErrorInvalidState;
}

#else

Error SecureSession::SendMessage(Message &aMessage, ResponseHandler aHandler, void *aContext)
{
    return IsConnected() ? CoapBase::SendMessage(aMessage, GetMessageInfo(), aHandler, aContext) : kErrorInvalidState;
}

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error SecureSession::Transmit(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureSession &>(aCoapBase).Transmit(aMessage, aMessageInfo);
}

Error SecureSession::Transmit(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error error = kErrorNone;

    VerifyOrExit(!GetTransport().IsClosed(), error = kErrorInvalidState);

    mTransmitQueue.Enqueue(aMessage);
    mTransmitTask.Post();

exit:
    return error;
}

void SecureSession::HandleDtlsConnectEvent(ConnectEvent aEvent, void *aContext)
{
    static_cast<SecureSession *>(aContext)->HandleDtlsConnectEvent(aEvent);
}

void SecureSession::HandleDtlsConnectEvent(ConnectEvent aEvent)
{
    if (aEvent != kConnected)
    {
        mTransmitQueue.DequeueAndFreeAll();
        ClearAllRequestsAndResponses();
    }

    mConnectCallback.InvokeIfSet(aEvent);
}

void SecureSession::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    static_cast<SecureSession *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void SecureSession::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    ot::Message *message = nullptr;

    VerifyOrExit((message = Get<MessagePool>().Allocate(Message::kTypeIp6, Message::GetHelpDataReserved())) != nullptr);
    SuccessOrExit(message->AppendBytes(aBuf, aLength));

    CoapBase::Receive(*message, GetMessageInfo());

exit:
    FreeMessage(message);
}

void SecureSession::HandleTransmitTask(Tasklet &aTasklet)
{
    static_cast<SecureSession *>(static_cast<TaskletContext &>(aTasklet).GetContext())->HandleTransmitTask();
}

void SecureSession::HandleTransmitTask(void)
{
    Error        error   = kErrorNone;
    ot::Message *message = mTransmitQueue.GetHead();

    VerifyOrExit(message != nullptr);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != nullptr)
    {
        mTransmitTask.Post();
    }

    error = Dtls::Session::Send(*message);

exit:
    FreeMessageOnError(message, error);
}

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

MeshCoP::SecureSession *ApplicationCoapSecure::HandleDtlsAccept(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<ApplicationCoapSecure *>(aContext)->HandleDtlsAccept();
}

SecureSession *ApplicationCoapSecure::HandleDtlsAccept(void)
{
    return IsSessionInUse() ? nullptr : static_cast<SecureSession *>(this);
}

#endif

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
