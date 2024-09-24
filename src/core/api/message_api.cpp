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

#include "instance/instance.hpp"

using namespace ot;

void otMessageFree(otMessage *aMessage) { AsCoreType(aMessage).Free(); }

uint16_t otMessageGetLength(const otMessage *aMessage) { return AsCoreType(aMessage).GetLength(); }

otError otMessageSetLength(otMessage *aMessage, uint16_t aLength) { return AsCoreType(aMessage).SetLength(aLength); }

uint16_t otMessageGetOffset(const otMessage *aMessage) { return AsCoreType(aMessage).GetOffset(); }

void otMessageSetOffset(otMessage *aMessage, uint16_t aOffset) { AsCoreType(aMessage).SetOffset(aOffset); }

bool otMessageIsLinkSecurityEnabled(const otMessage *aMessage) { return AsCoreType(aMessage).IsLinkSecurityEnabled(); }

bool otMessageIsLoopbackToHostAllowed(const otMessage *aMessage)
{
    return AsCoreType(aMessage).IsLoopbackToHostAllowed();
}

void otMessageSetLoopbackToHostAllowed(otMessage *aMessage, bool aAllowLoopbackToHost)
{
    return AsCoreType(aMessage).SetLoopbackToHostAllowed(aAllowLoopbackToHost);
}

bool otMessageIsMulticastLoopEnabled(otMessage *aMessage) { return AsCoreType(aMessage).GetMulticastLoop(); }

void otMessageSetMulticastLoopEnabled(otMessage *aMessage, bool aEnabled)
{
    AsCoreType(aMessage).SetMulticastLoop(aEnabled);
}

otMessageOrigin otMessageGetOrigin(const otMessage *aMessage) { return MapEnum(AsCoreType(aMessage).GetOrigin()); }

void otMessageSetOrigin(otMessage *aMessage, otMessageOrigin aOrigin)
{
    AsCoreType(aMessage).SetOrigin(MapEnum(aOrigin));
}

void otMessageSetDirectTransmission(otMessage *aMessage, bool aEnabled)
{
    if (aEnabled)
    {
        AsCoreType(aMessage).SetDirectTransmission();
    }
    else
    {
        AsCoreType(aMessage).ClearDirectTransmission();
    }
}

int8_t otMessageGetRss(const otMessage *aMessage) { return AsCoreType(aMessage).GetAverageRss(); }

otError otMessageGetThreadLinkInfo(const otMessage *aMessage, otThreadLinkInfo *aLinkInfo)
{
    return AsCoreType(aMessage).GetLinkInfo(AsCoreType(aLinkInfo));
}

otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
    AssertPointerIsNotNull(aBuf);

    return AsCoreType(aMessage).AppendBytes(aBuf, aLength);
}

uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    AssertPointerIsNotNull(aBuf);

    return AsCoreType(aMessage).ReadBytes(aOffset, aBuf, aLength);
}

int otMessageWrite(otMessage *aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    AssertPointerIsNotNull(aBuf);

    AsCoreType(aMessage).WriteBytes(aOffset, aBuf, aLength);

    return aLength;
}

void otMessageQueueInit(otMessageQueue *aQueue)
{
    AssertPointerIsNotNull(aQueue);

    aQueue->mData = nullptr;
}

void otMessageQueueEnqueue(otMessageQueue *aQueue, otMessage *aMessage)
{
    AsCoreType(aQueue).Enqueue(AsCoreType(aMessage));
}

void otMessageQueueEnqueueAtHead(otMessageQueue *aQueue, otMessage *aMessage)
{
    AsCoreType(aQueue).Enqueue(AsCoreType(aMessage), MessageQueue::kQueuePositionHead);
}

void otMessageQueueDequeue(otMessageQueue *aQueue, otMessage *aMessage)
{
    AsCoreType(aQueue).Dequeue(AsCoreType(aMessage));
}

otMessage *otMessageQueueGetHead(otMessageQueue *aQueue) { return AsCoreType(aQueue).GetHead(); }

otMessage *otMessageQueueGetNext(otMessageQueue *aQueue, const otMessage *aMessage)
{
    Message *next;

    VerifyOrExit(aMessage != nullptr, next = nullptr);

    VerifyOrExit(AsCoreType(aMessage).GetMessageQueue() == aQueue, next = nullptr);
    next = AsCoreType(aMessage).GetNext();

exit:
    return next;
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void otMessageGetBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo)
{
    AsCoreType(aInstance).GetBufferInfo(AsCoreType(aBufferInfo));
}

void otMessageResetBufferInfo(otInstance *aInstance) { AsCoreType(aInstance).ResetBufferInfo(); }
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
