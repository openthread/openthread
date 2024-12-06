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
 *   This file implements the secure CoAP agent.
 */

namespace ot {
namespace Coap {

RegisterLogModule("CoapSecure");

CoapSecureBase::CoapSecureBase(Instance &aInstance, MeshCoP::Dtls &aDtls)
    : CoapBase(aInstance, Send)
    , mDtls(aDtls)
    , mTransmitTask(aInstance, HandleTransmit, this)
{
}

Error CoapSecureBase::Start(uint16_t aPort) { return Start(aPort, /* aMaxAttempts */ 0, nullptr, nullptr); }

Error CoapSecureBase::Start(uint16_t aPort, uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext)
{
    Error error;

    SuccessOrExit(error = Open(aMaxAttempts, aCallback, aContext));
    error = mDtls.Bind(aPort);

exit:
    return error;
}

Error CoapSecureBase::Start(MeshCoP::Dtls::TransportCallback aCallback, void *aContext)
{
    Error error;

    SuccessOrExit(error = Open(/* aMaxAttemps */ 0, nullptr, nullptr));
    error = mDtls.Bind(aCallback, aContext);

exit:
    return error;
}

Error CoapSecureBase::Open(uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext)
{
    Error error = kErrorAlready;

    SuccessOrExit(mDtls.SetMaxConnectionAttempts(aMaxAttempts, HandleDtlsAutoClose, this));
    mAutoStopCallback.Set(aCallback, aContext);
    mConnectEventCallback.Clear();
    SuccessOrExit(mDtls.Open(HandleDtlsReceive, HandleDtlsConnectEvent, this));

    error = kErrorNone;

exit:
    return error;
}

void CoapSecureBase::Stop(void)
{
    mDtls.Close();

    mTransmitQueue.DequeueAndFreeAll();
    ClearRequestsAndResponses();
}

Error CoapSecureBase::Connect(const Ip6::SockAddr &aSockAddr, ConnectEventCallback aCallback, void *aContext)
{
    mConnectEventCallback.Set(aCallback, aContext);

    return mDtls.Connect(aSockAddr);
}

void CoapSecureBase::SetPsk(const MeshCoP::JoinerPskd &aPskd)
{
    static_assert(static_cast<uint16_t>(MeshCoP::JoinerPskd::kMaxLength) <=
                      static_cast<uint16_t>(MeshCoP::Dtls::kPskMaxLength),
                  "The maximum length of DTLS PSK is smaller than joiner PSKd");

    SuccessOrAssert(mDtls.SetPsk(reinterpret_cast<const uint8_t *>(aPskd.GetAsCString()), aPskd.GetLength()));
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
Error CoapSecureBase::SendMessage(Message                    &aMessage,
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

Error CoapSecureBase::SendMessage(Message                    &aMessage,
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
Error CoapSecureBase::SendMessage(Message &aMessage, ResponseHandler aHandler, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);

    error = CoapBase::SendMessage(aMessage, mDtls.GetMessageInfo(), aHandler, aContext);

exit:
    return error;
}

Error CoapSecureBase::SendMessage(Message                &aMessage,
                                  const Ip6::MessageInfo &aMessageInfo,
                                  ResponseHandler         aHandler,
                                  void                   *aContext)
{
    return CoapBase::SendMessage(aMessage, aMessageInfo, aHandler, aContext);
}
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error CoapSecureBase::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    mTransmitQueue.Enqueue(aMessage);
    mTransmitTask.Post();

    return kErrorNone;
}

void CoapSecureBase::HandleDtlsConnectEvent(MeshCoP::Dtls::ConnectEvent aEvent, void *aContext)
{
    return static_cast<CoapSecureBase *>(aContext)->HandleDtlsConnectEvent(aEvent);
}

void CoapSecureBase::HandleDtlsConnectEvent(MeshCoP::Dtls::ConnectEvent aEvent)
{
    mConnectEventCallback.InvokeIfSet(aEvent);
}

void CoapSecureBase::HandleDtlsAutoClose(void *aContext)
{
    return static_cast<CoapSecureBase *>(aContext)->HandleDtlsAutoClose();
}

void CoapSecureBase::HandleDtlsAutoClose(void)
{
    Stop();
    mAutoStopCallback.InvokeIfSet();
}

void CoapSecureBase::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<CoapSecureBase *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void CoapSecureBase::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    ot::Message *message = nullptr;

    VerifyOrExit((message = Get<MessagePool>().Allocate(Message::kTypeIp6, Message::GetHelpDataReserved())) != nullptr);
    SuccessOrExit(message->AppendBytes(aBuf, aLength));

    CoapBase::Receive(*message, mDtls.GetMessageInfo());

exit:
    FreeMessage(message);
}

void CoapSecureBase::HandleTransmit(Tasklet &aTasklet)
{
    static_cast<CoapSecureBase *>(static_cast<TaskletContext &>(aTasklet).GetContext())->HandleTransmit();
}

void CoapSecureBase::HandleTransmit(void)
{
    Error        error   = kErrorNone;
    ot::Message *message = mTransmitQueue.GetHead();

    VerifyOrExit(message != nullptr);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != nullptr)
    {
        mTransmitTask.Post();
    }

    SuccessOrExit(error = mDtls.Send(*message));
    LogDebg("Transmit");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "transmit");
}

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
