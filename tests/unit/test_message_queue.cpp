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

#include <stdarg.h>

#include "test_platform.h"

#include <openthread/config.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"

#include "test_util.h"

#define kNumTestMessages 5

static ot::Instance *   sInstance;
static ot::MessagePool *sMessagePool;

// This function verifies the content of the message queue to match the passed in messages
void VerifyMessageQueueContent(ot::MessageQueue &aMessageQueue, int aExpectedLength, ...)
{
    va_list      args;
    ot::Message *message;
    ot::Message *msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aMessageQueue.GetHead();
        VerifyOrQuit(message == nullptr, "MessageQueue is not empty when expected len is zero.");
    }
    else
    {
        for (message = aMessageQueue.GetHead(); message != nullptr; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "MessageQueue contains more entries than expected");

            msgArg = va_arg(args, ot::Message *);
            VerifyOrQuit(msgArg == message, "MessageQueue content does not match what is expected.");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "MessageQueue contains less entries than expected");
    }

    va_end(args);
}

void TestMessageQueue(void)
{
    ot::MessageQueue messageQueue;
    ot::Message *    msg[kNumTestMessages];
    uint16_t         msgCount, bufferCount;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != nullptr, "Null instance");

    sMessagePool = &sInstance->Get<ot::MessagePool>();

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = sMessagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msg[i] != nullptr, "Message::New failed");
    }

    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 1 message and remove it
    messageQueue.Enqueue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    messageQueue.Dequeue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 1 message at head and remove it
    messageQueue.Enqueue(*msg[0], ot::MessageQueue::kQueuePositionHead);
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    messageQueue.Dequeue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 5 messages
    messageQueue.Enqueue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    messageQueue.Enqueue(*msg[1]);
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[1]);
    messageQueue.Enqueue(*msg[2]);
    VerifyMessageQueueContent(messageQueue, 3, msg[0], msg[1], msg[2]);
    messageQueue.Enqueue(*msg[3]);
    VerifyMessageQueueContent(messageQueue, 4, msg[0], msg[1], msg[2], msg[3]);
    messageQueue.Enqueue(*msg[4]);
    VerifyMessageQueueContent(messageQueue, 5, msg[0], msg[1], msg[2], msg[3], msg[4]);

    // Check the GetInfo()
    messageQueue.GetInfo(msgCount, bufferCount);
    VerifyOrQuit(msgCount == 5, "MessageQueue::GetInfo() failed.");

    // Remove from head
    messageQueue.Dequeue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[2], msg[3], msg[4]);

    // Remove a message in middle
    messageQueue.Dequeue(*msg[3]);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[2], msg[4]);

    // Remove from tail
    messageQueue.Dequeue(*msg[4]);
    VerifyMessageQueueContent(messageQueue, 2, msg[1], msg[2]);

    // Add after remove
    messageQueue.Enqueue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[2], msg[0]);
    messageQueue.Enqueue(*msg[3]);
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[2], msg[0], msg[3]);

    // Remove from middle
    messageQueue.Dequeue(*msg[2]);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Add to head
    messageQueue.Enqueue(*msg[2], ot::MessageQueue::kQueuePositionHead);
    VerifyMessageQueueContent(messageQueue, 4, msg[2], msg[1], msg[0], msg[3]);

    // Remove from head
    messageQueue.Dequeue(*msg[2]);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Remove from head
    messageQueue.Dequeue(*msg[1]);
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[3]);

    // Add to head
    messageQueue.Enqueue(*msg[1], ot::MessageQueue::kQueuePositionHead);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Add to tail
    messageQueue.Enqueue(*msg[2], ot::MessageQueue::kQueuePositionTail);
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[0], msg[3], msg[2]);

    // Remove all messages.
    messageQueue.Dequeue(*msg[3]);
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[2]);
    messageQueue.Dequeue(*msg[1]);
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[2]);
    messageQueue.Dequeue(*msg[2]);
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    messageQueue.Dequeue(*msg[0]);
    VerifyMessageQueueContent(messageQueue, 0);

    testFreeInstance(sInstance);
}

// This function verifies the content of the message queue to match the passed in messages
void VerifyMessageQueueContentUsingOtApi(otMessageQueue *aQueue, int aExpectedLength, ...)
{
    va_list    args;
    otMessage *message;
    otMessage *msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = otMessageQueueGetHead(aQueue);
        VerifyOrQuit(message == nullptr, "MessageQueue is not empty when expected len is zero.");
    }
    else
    {
        for (message = otMessageQueueGetHead(aQueue); message != nullptr;
             message = otMessageQueueGetNext(aQueue, message))
        {
            VerifyOrQuit(aExpectedLength != 0, "MessageQueue contains more entries than expected");

            msgArg = va_arg(args, otMessage *);
            VerifyOrQuit(msgArg == message, "MessageQueue content does not match what is expected.");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "MessageQueue contains less entries than expected");
    }

    va_end(args);
}

// This test checks all the OpenThread C APIs for `otMessageQueue`
void TestMessageQueueOtApis(void)
{
    otMessage *    msg[kNumTestMessages];
    otMessage *    message;
    otMessageQueue queue, queue2;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != nullptr, "Null instance");

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = otIp6NewMessage(sInstance, nullptr);
        VerifyOrQuit(msg[i] != nullptr, "otIp6NewMessage() failed.");
    }

    otMessageQueueInit(&queue);
    otMessageQueueInit(&queue2);

    // Check an empty queue.
    VerifyMessageQueueContentUsingOtApi(&queue, 0);

    // Add message to the queue and check the content
    otMessageQueueEnqueue(&queue, msg[0]);
    VerifyMessageQueueContentUsingOtApi(&queue, 1, msg[0]);
    otMessageQueueEnqueue(&queue, msg[1]);
    VerifyMessageQueueContentUsingOtApi(&queue, 2, msg[0], msg[1]);
    otMessageQueueEnqueueAtHead(&queue, msg[2]);
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[2], msg[0], msg[1]);
    otMessageQueueEnqueue(&queue, msg[3]);
    VerifyMessageQueueContentUsingOtApi(&queue, 4, msg[2], msg[0], msg[1], msg[3]);

    // Remove elements and check the content
    otMessageQueueDequeue(&queue, msg[1]);
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[2], msg[0], msg[3]);
    otMessageQueueDequeue(&queue, msg[0]);
    VerifyMessageQueueContentUsingOtApi(&queue, 2, msg[2], msg[3]);
    otMessageQueueDequeue(&queue, msg[3]);
    VerifyMessageQueueContentUsingOtApi(&queue, 1, msg[2]);

    // Check the failure cases for otMessageQueueGetNext()
    message = otMessageQueueGetNext(&queue, nullptr);
    VerifyOrQuit(message == nullptr, "otMessageQueueGetNext(queue, nullptr) did not return nullptr.");
    message = otMessageQueueGetNext(&queue, msg[1]);
    VerifyOrQuit(message == nullptr, "otMessageQueueGetNext() did not return nullptr for a message not in the queue.");

    // Check the failure case when attempting to do otMessageQueueGetNext() but passing in a wrong queue pointer.
    otMessageQueueEnqueue(&queue2, msg[0]);
    VerifyMessageQueueContentUsingOtApi(&queue2, 1, msg[0]);
    otMessageQueueEnqueue(&queue2, msg[1]);
    VerifyMessageQueueContentUsingOtApi(&queue2, 2, msg[0], msg[1]);

    message = otMessageQueueGetNext(&queue2, msg[0]);
    VerifyOrQuit(message == msg[1], "otMessageQueueGetNext() failed");
    message = otMessageQueueGetNext(&queue, msg[0]);
    VerifyOrQuit(message == nullptr, "otMessageQueueGetNext() did not return nullptr for message not in  the queue.");

    // Remove all element and make sure queue is empty
    otMessageQueueDequeue(&queue, msg[2]);
    VerifyMessageQueueContentUsingOtApi(&queue, 0);

    testFreeInstance(sInstance);
}

int main(void)
{
    TestMessageQueue();
    TestMessageQueueOtApis();
    printf("All tests passed\n");
    return 0;
}
