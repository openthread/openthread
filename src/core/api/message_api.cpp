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

#include <openthread/message.h>

#include "openthread-instance.h"

using namespace ot;

otError otMessageFree(otMessage *aMessage)
{
    return static_cast<Message *>(aMessage)->Free();
}

uint16_t otMessageGetLength(otMessage *aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->GetLength();
}

otError otMessageSetLength(otMessage *aMessage, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->SetLength(aLength);
}

uint16_t otMessageGetOffset(otMessage *aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->GetOffset();
}

otError otMessageSetOffset(otMessage *aMessage, uint16_t aOffset)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->SetOffset(aOffset);
}

bool otMessageIsLinkSecurityEnabled(otMessage *aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->IsLinkSecurityEnabled();
}

void otMessageSetDirectTransmission(otMessage *aMessage, bool aEnabled)
{
    Message *message = static_cast<Message *>(aMessage);

    if (aEnabled)
    {
        message->SetDirectTransmission();
    }
    else
    {
        message->ClearDirectTransmission();
    }
}

otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Append(aBuf, aLength);
}

int otMessageRead(otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Read(aOffset, aLength, aBuf);
}

int otMessageWrite(otMessage *aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Write(aOffset, aLength, aBuf);
}

void otMessageQueueInit(otMessageQueue *aQueue)
{
    aQueue->mData = NULL;
}

otError otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage *aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    MessageQueue *queue = static_cast<MessageQueue *>(aQueue);
    return queue->Enqueue(*message);
}

otError otMessageQueueDequeue(otMessageQueue *aQueue, otMessage *aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    MessageQueue *queue = static_cast<MessageQueue *>(aQueue);
    return queue->Dequeue(*message);
}

otMessage *otMessageQueueGetHead(otMessageQueue *aQueue)
{
    MessageQueue *queue = static_cast<MessageQueue *>(aQueue);
    return queue->GetHead();
}

otMessage *otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage *aMessage)
{
    Message *next;
    const Message *message = static_cast<const Message *>(aMessage);
    MessageQueue *queue = static_cast<MessageQueue *>(aQueue);

    VerifyOrExit(message != NULL, next = NULL);
    VerifyOrExit(message->GetMessageQueue() == queue, next = NULL);
    next = message->GetNext();

exit:
    return next;
}

void otMessageGetBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo)
{
    aBufferInfo->mTotalBuffers = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;

    aBufferInfo->mFreeBuffers = aInstance->mThreadNetif.GetIp6().mMessagePool.GetFreeBufferCount();

    aInstance->mThreadNetif.GetMeshForwarder().GetSendQueue().GetInfo(aBufferInfo->m6loSendMessages,
                                                                      aBufferInfo->m6loSendBuffers);

    aInstance->mThreadNetif.GetMeshForwarder().GetReassemblyQueue().GetInfo(aBufferInfo->m6loReassemblyMessages,
                                                                            aBufferInfo->m6loReassemblyBuffers);

    aInstance->mThreadNetif.GetMeshForwarder().GetResolvingQueue().GetInfo(aBufferInfo->mArpMessages,
                                                                           aBufferInfo->mArpBuffers);

    aInstance->mThreadNetif.GetIp6().GetSendQueue().GetInfo(aBufferInfo->mIp6Messages,
                                                            aBufferInfo->mIp6Buffers);

    aInstance->mThreadNetif.GetIp6().mMpl.GetBufferedMessageSet().GetInfo(aBufferInfo->mMplMessages,
                                                                          aBufferInfo->mMplBuffers);

    aInstance->mThreadNetif.GetMle().GetMessageQueue().GetInfo(aBufferInfo->mMleMessages,
                                                               aBufferInfo->mMleBuffers);

    aInstance->mThreadNetif.GetCoap().GetRequestMessages().GetInfo(aBufferInfo->mCoapMessages,
                                                                   aBufferInfo->mCoapBuffers);
}
