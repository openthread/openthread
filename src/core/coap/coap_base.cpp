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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <common/code_utils.hpp>
#include <coap/coap_base.hpp>

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace ot {
namespace Coap {

Message *CoapBase::NewMessage(const Header &aHeader, uint8_t aPriority)
{
    Message *message = NULL;

    // Ensure that header has minimum required length.
    VerifyOrExit(aHeader.GetLength() >= Header::kMinHeaderLength);

    VerifyOrExit((message = mSocket.NewMessage(aHeader.GetLength())) != NULL);
    message->Prepend(aHeader.GetBytes(), aHeader.GetLength());
    message->SetOffset(0);
    message->SetPriority(aPriority);

exit:
    return message;
}

ThreadError CoapBase::Start(const Ip6::SockAddr &aSockAddr)
{
    ThreadError error;

    SuccessOrExit(error = mSocket.Open(&CoapBase::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(aSockAddr));

exit:
    return error;
}

ThreadError CoapBase::Stop(void)
{
    return mSocket.Close();
}

void CoapBase::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapBase *>(aContext)->mReceiver(aContext, *static_cast<Message *>(aMessage),
                                                 *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

ThreadError CoapBase::SendEmptyMessage(Header::Type aType, const Header &aRequestHeader,
                                       const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message = NULL;

    VerifyOrExit(aRequestHeader.GetType() == kCoapTypeConfirmable, error = kThreadError_InvalidArgs);

    responseHeader.Init(aType, kCoapCodeEmpty);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());

    VerifyOrExit((message = NewMessage(responseHeader)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = mSender(this, *message, aMessageInfo));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

}  // namespace Coap
}  // namespace ot
