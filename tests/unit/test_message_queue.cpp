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
#include "utils/wrap_string.h"

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
        VerifyOrQuit(message == NULL, "MessageQueue is not empty when expected len is zero.\n");
    }
    else
    {
        for (message = aMessageQueue.GetHead(); message != NULL; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "MessageQueue contains more entries than expected\n");

            msgArg = va_arg(args, ot::Message *);
            VerifyOrQuit(msgArg == message, "MessageQueue content does not match what is expected.\n");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "MessageQueue contains less entries than expected\n");
    }

    va_end(args);
}

void TestMessageQueue(void)
{
    ot::MessageQueue messageQueue;
    ot::Message *    msg[kNumTestMessages];
    otError          error;
    uint16_t         msgCount, bufferCount;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != NULL, "Null instance");

    sMessagePool = &sInstance->GetMessagePool();

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = sMessagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msg[i] != NULL, "Message::New failed\n");
    }

    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 1 message and remove it
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[0]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 1 message at head and remove it
    SuccessOrQuit(messageQueue.Enqueue(*msg[0], ot::MessageQueue::kQueuePositionHead),
                  "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[0]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 5 messages
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[1]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[2]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[0], msg[1], msg[2]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[3]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[0], msg[1], msg[2], msg[3]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[4]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 5, msg[0], msg[1], msg[2], msg[3], msg[4]);

    // Check the GetInfo()
    messageQueue.GetInfo(msgCount, bufferCount);
    VerifyOrQuit(msgCount == 5, "MessageQueue::GetInfo() failed.\n");

    // Remove from head
    SuccessOrQuit(messageQueue.Dequeue(*msg[0]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[2], msg[3], msg[4]);

    // Remove a message in middle
    SuccessOrQuit(messageQueue.Dequeue(*msg[3]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[2], msg[4]);

    // Remove from tail
    SuccessOrQuit(messageQueue.Dequeue(*msg[4]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 2, msg[1], msg[2]);

    // Add after remove
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[2], msg[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[3]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[2], msg[0], msg[3]);

    // Remove from middle
    SuccessOrQuit(messageQueue.Dequeue(*msg[2]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Add to head
    SuccessOrQuit(messageQueue.Enqueue(*msg[2], ot::MessageQueue::kQueuePositionHead),
                  "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[2], msg[1], msg[0], msg[3]);

    // Remove from head
    SuccessOrQuit(messageQueue.Dequeue(*msg[2]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Remove from head
    SuccessOrQuit(messageQueue.Dequeue(*msg[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[3]);

    // Add to head
    SuccessOrQuit(messageQueue.Enqueue(*msg[1], ot::MessageQueue::kQueuePositionHead),
                  "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);

    // Add to tail
    SuccessOrQuit(messageQueue.Enqueue(*msg[2], ot::MessageQueue::kQueuePositionTail),
                  "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[0], msg[3], msg[2]);

    // Remove all messages.
    SuccessOrQuit(messageQueue.Dequeue(*msg[3]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[2]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[2]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[2]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[0]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 0);

    // Check the failure cases: Enqueue an already queued message or dequeue a message not in the queue.
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    error = messageQueue.Enqueue(*msg[0]);
    VerifyOrQuit(error == OT_ERROR_ALREADY, "Enqueuing an already queued message did not fail as expected.\n");
    error = messageQueue.Dequeue(*msg[1]);
    VerifyOrQuit(error == OT_ERROR_NOT_FOUND, "Dequeuing a message not in the queue did not fail as expected.\n");

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
        VerifyOrQuit(message == NULL, "MessageQueue is not empty when expected len is zero.\n");
    }
    else
    {
        for (message = otMessageQueueGetHead(aQueue); message != NULL; message = otMessageQueueGetNext(aQueue, message))
        {
            VerifyOrQuit(aExpectedLength != 0, "MessageQueue contains more entries than expected\n");

            msgArg = va_arg(args, otMessage *);
            VerifyOrQuit(msgArg == message, "MessageQueue content does not match what is expected.\n");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "MessageQueue contains less entries than expected\n");
    }

    va_end(args);
}

// This test checks all the OpenThread C APIs for `otMessageQueue`
void TestMessageQueueOtApis(void)
{
    otMessage *    msg[kNumTestMessages];
    otError        error;
    otMessage *    message;
    otMessageQueue queue, queue2;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != NULL, "Null instance");

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = otIp6NewMessage(sInstance, NULL);
        VerifyOrQuit(msg[i] != NULL, "otIp6NewMessage() failed.\n");
    }

    otMessageQueueInit(&queue);
    otMessageQueueInit(&queue2);

    // Check an empty queue.
    VerifyMessageQueueContentUsingOtApi(&queue, 0);

    // Add message to the queue and check the content
    SuccessOrQuit(otMessageQueueEnqueue(&queue, msg[0]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 1, msg[0]);
    SuccessOrQuit(otMessageQueueEnqueue(&queue, msg[1]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 2, msg[0], msg[1]);
    SuccessOrQuit(otMessageQueueEnqueueAtHead(&queue, msg[2]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[2], msg[0], msg[1]);
    SuccessOrQuit(otMessageQueueEnqueue(&queue, msg[3]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 4, msg[2], msg[0], msg[1], msg[3]);

    // Remove elements and check the content
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[1]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[2], msg[0], msg[3]);
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[0]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 2, msg[2], msg[3]);
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[3]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 1, msg[2]);

    // Check the expected failure cases for the enqueue and dequeue:
    error = otMessageQueueEnqueue(&queue, msg[2]);
    VerifyOrQuit(error == OT_ERROR_ALREADY, "Enqueuing an already queued message did not fail as expected.\n");
    error = otMessageQueueDequeue(&queue, msg[0]);
    VerifyOrQuit(error == OT_ERROR_NOT_FOUND, "Dequeuing a message not in the queue did not fail as expected.\n");

    // Check the failure cases for otMessageQueueGetNext()
    message = otMessageQueueGetNext(&queue, NULL);
    VerifyOrQuit(message == NULL, "otMessageQueueGetNext(queue, NULL) did not return NULL.\n");
    message = otMessageQueueGetNext(&queue, msg[1]);
    VerifyOrQuit(message == NULL, "otMessageQueueGetNext() did not return NULL for a message not in the queue.\n");

    // Check the failure case when attempting to do otMessageQueueGetNext() but passing in a wrong queue pointer.
    SuccessOrQuit(otMessageQueueEnqueue(&queue2, msg[0]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue2, 1, msg[0]);
    SuccessOrQuit(otMessageQueueEnqueue(&queue2, msg[1]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue2, 2, msg[0], msg[1]);

    message = otMessageQueueGetNext(&queue2, msg[0]);
    VerifyOrQuit(message == msg[1], "otMessageQueueGetNext() failed\n");
    message = otMessageQueueGetNext(&queue, msg[0]);
    VerifyOrQuit(message == NULL, "otMessageQueueGetNext() did not return NULL for message not in  the queue.\n");

    // Remove all element and make sure queue is empty
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[2]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 0);

    testFreeInstance(sInstance);
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestMessageQueue();
    TestMessageQueueOtApis();
    printf("All tests passed\n");
    return 0;
}
#endif
