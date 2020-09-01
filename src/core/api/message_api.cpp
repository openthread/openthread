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

/**
 * @file
 *   This file implements the OpenThread Message API.
 */

#include "openthread-core-config.h"

#include <openthread/message.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"

using namespace ot;

void otMessageFree(otMessage *aMessage)
{
    static_cast<Message *>(aMessage)->Free();
}

uint16_t otMessageGetLength(const otMessage *aMessage)
{
    const Message &message = *static_cast<const Message *>(aMessage);
    return message.GetLength();
}

otError otMessageSetLength(otMessage *aMessage, uint16_t aLength)
{
    Message &message = *static_cast<Message *>(aMessage);
    return message.SetLength(aLength);
}

uint16_t otMessageGetOffset(const otMessage *aMessage)
{
    const Message &message = *static_cast<const Message *>(aMessage);
    return message.GetOffset();
}

void otMessageSetOffset(otMessage *aMessage, uint16_t aOffset)
{
    Message &message = *static_cast<Message *>(aMessage);
    message.SetOffset(aOffset);
}

bool otMessageIsLinkSecurityEnabled(const otMessage *aMessage)
{
    const Message &message = *static_cast<const Message *>(aMessage);
    return message.IsLinkSecurityEnabled();
}

void otMessageSetDirectTransmission(otMessage *aMessage, bool aEnabled)
{
    Message &message = *static_cast<Message *>(aMessage);

    if (aEnabled)
    {
        message.SetDirectTransmission();
    }
    else
    {
        message.ClearDirectTransmission();
    }
}

int8_t otMessageGetRss(const otMessage *aMessage)
{
    const Message &message = *static_cast<const Message *>(aMessage);
    return message.GetAverageRss();
}

otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
    Message &message = *static_cast<Message *>(aMessage);
    return message.Append(aBuf, aLength);
}

uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    const Message &message = *static_cast<const Message *>(aMessage);
    return message.Read(aOffset, aLength, aBuf);
}

int otMessageWrite(otMessage *aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    Message &message = *static_cast<Message *>(aMessage);
    message.Write(aOffset, aLength, aBuf);

    return aLength;
}

void otMessageQueueInit(otMessageQueue *aQueue)
{
    aQueue->mData = nullptr;
}

void otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage *aMessage)
{
    Message &     message = *static_cast<Message *>(aMessage);
    MessageQueue &queue   = *static_cast<MessageQueue *>(aQueue);

    queue.Enqueue(message);
}

void otMessageQueueEnqueueAtHead(otMessageQueue *aQueue, otMessage *aMessage)
{
    Message &     message = *static_cast<Message *>(aMessage);
    MessageQueue &queue   = *static_cast<MessageQueue *>(aQueue);

    queue.Enqueue(message, MessageQueue::kQueuePositionHead);
}

void otMessageQueueDequeue(otMessageQueue *aQueue, otMessage *aMessage)
{
    Message &     message = *static_cast<Message *>(aMessage);
    MessageQueue &queue   = *static_cast<MessageQueue *>(aQueue);

    queue.Dequeue(message);
}

otMessage *otMessageQueueGetHead(otMessageQueue *aQueue)
{
    MessageQueue &queue = *static_cast<MessageQueue *>(aQueue);
    return queue.GetHead();
}

otMessage *otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage *aMessage)
{
    Message *next;

    VerifyOrExit(aMessage != nullptr, next = nullptr);

    {
        const Message &message = *static_cast<const Message *>(aMessage);
        MessageQueue & queue   = *static_cast<MessageQueue *>(aQueue);

        VerifyOrExit(message.GetMessageQueue() == &queue, next = nullptr);
        next = message.GetNext();
    }

exit:
    return next;
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void otMessageGetBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo)
{
    uint16_t  messages, buffers;
    Instance &instance = *static_cast<Instance *>(aInstance);

    aBufferInfo->mTotalBuffers = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;

    aBufferInfo->mFreeBuffers = instance.Get<MessagePool>().GetFreeBufferCount();

    instance.Get<MeshForwarder>().GetSendQueue().GetInfo(aBufferInfo->m6loSendMessages, aBufferInfo->m6loSendBuffers);

    instance.Get<MeshForwarder>().GetReassemblyQueue().GetInfo(aBufferInfo->m6loReassemblyMessages,
                                                               aBufferInfo->m6loReassemblyBuffers);

#if OPENTHREAD_FTD
    instance.Get<MeshForwarder>().GetResolvingQueue().GetInfo(aBufferInfo->mArpMessages, aBufferInfo->mArpBuffers);
#else
    aBufferInfo->mArpMessages             = 0;
    aBufferInfo->mArpBuffers              = 0;
#endif

    instance.Get<Ip6::Ip6>().GetSendQueue().GetInfo(aBufferInfo->mIp6Messages, aBufferInfo->mIp6Buffers);

#if OPENTHREAD_FTD
    instance.Get<Ip6::Mpl>().GetBufferedMessageSet().GetInfo(aBufferInfo->mMplMessages, aBufferInfo->mMplBuffers);
#else
    aBufferInfo->mMplMessages             = 0;
    aBufferInfo->mMplBuffers              = 0;
#endif

    instance.Get<Mle::MleRouter>().GetMessageQueue().GetInfo(aBufferInfo->mMleMessages, aBufferInfo->mMleBuffers);

    instance.Get<Tmf::TmfAgent>().GetRequestMessages().GetInfo(aBufferInfo->mCoapMessages, aBufferInfo->mCoapBuffers);
    instance.Get<Tmf::TmfAgent>().GetCachedResponses().GetInfo(messages, buffers);
    aBufferInfo->mCoapMessages += messages;
    aBufferInfo->mCoapBuffers += buffers;

#if OPENTHREAD_CONFIG_DTLS_ENABLE
    instance.Get<Coap::CoapSecure>().GetRequestMessages().GetInfo(aBufferInfo->mCoapSecureMessages,
                                                                  aBufferInfo->mCoapSecureBuffers);
    instance.Get<Coap::CoapSecure>().GetCachedResponses().GetInfo(messages, buffers);
    aBufferInfo->mCoapSecureMessages += messages;
    aBufferInfo->mCoapSecureBuffers += buffers;
#else
    aBufferInfo->mCoapSecureMessages      = 0;
    aBufferInfo->mCoapSecureBuffers       = 0;
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    instance.GetApplicationCoap().GetRequestMessages().GetInfo(aBufferInfo->mApplicationCoapMessages,
                                                               aBufferInfo->mApplicationCoapBuffers);
    instance.GetApplicationCoap().GetCachedResponses().GetInfo(messages, buffers);
    aBufferInfo->mApplicationCoapMessages += messages;
    aBufferInfo->mApplicationCoapBuffers += buffers;
#else
    aBufferInfo->mApplicationCoapMessages = 0;
    aBufferInfo->mApplicationCoapBuffers  = 0;
#endif
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
