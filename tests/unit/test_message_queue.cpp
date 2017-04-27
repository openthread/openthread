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

#include "test_util.h"

#include "openthread/openthread.h"

#include <openthread-instance.h>
#include <common/debug.hpp>
#include <common/message.hpp>

#include "utils/wrap_string.h"
#include <stdarg.h>

#define kNumTestMessages      5

// This function verifies the content of the message queue to match the passed in messages
void VerifyMessageQueueContent(Thread::MessageQueue &aMessageQueue, int aExpectedLength, ...)
{
    va_list args;
    Thread::Message *message;
    Thread::Message *msgArg;

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

            msgArg = va_arg(args, Thread::Message *);
            VerifyOrQuit(msgArg == message, "MessageQueue content does not match what is expected.\n");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "MessageQueue contains less entries than expected\n");
    }

    va_end(args);
}

void TestMessageQueue(void)
{
    otInstance instance;
    Thread::MessagePool messagePool(&instance);
    Thread::MessageQueue messageQueue;
    Thread::Message *msg[kNumTestMessages];
    ThreadError error;
    uint16_t msgCount, bufferCount;

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = messagePool.New(Thread::Message::kTypeIp6, 0);
        VerifyOrQuit(msg[i] != NULL, "Message::New failed\n");
    }

    VerifyMessageQueueContent(messageQueue, 0);

    // Enqueue 1 message and remove it
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
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

    // Add after removes
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[2], msg[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msg[3]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 4, msg[1], msg[2], msg[0], msg[3]);

    // Remove all messages
    SuccessOrQuit(messageQueue.Dequeue(*msg[2]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 3, msg[1], msg[0], msg[3]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 2, msg[0], msg[3]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[3]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    SuccessOrQuit(messageQueue.Dequeue(*msg[0]), "MessageQueue::Dequeue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 0);

    // Check the failure cases: Enqueue an already queued message or dequeue a message not in the queue.
    SuccessOrQuit(messageQueue.Enqueue(*msg[0]), "MessageQueue::Enqueue() failed.\n");
    VerifyMessageQueueContent(messageQueue, 1, msg[0]);
    error = messageQueue.Enqueue(*msg[0]);
    VerifyOrQuit(error == kThreadError_Already, "Enqueuing an already queued message did not fail as expected.\n");
    error = messageQueue.Dequeue(*msg[1]);
    VerifyOrQuit(error == kThreadError_NotFound, "Dequeuing a message not in the queue did not fail as expected.\n");
}

// This function verifies the content of the message queue to match the passed in messages
void VerifyMessageQueueContentUsingOtApi(otMessageQueue *aQueue, int aExpectedLength, ...)
{
    va_list args;
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
        for (message = otMessageQueueGetHead(aQueue);
             message != NULL;
             message = otMessageQueueGetNext(aQueue, message)
            )
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
    otInstance *instance;
    otMessageQueue queue, queue2;

    otMessage *msg[kNumTestMessages];
    ThreadError error;
    otMessage *message;

#ifdef OPENTHREAD_MULTIPLE_INSTANCE
    size_t otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer = NULL;

    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    assert(otInstanceBuffer);

    // Initialize Openthread with the buffer
    instance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    instance = otInstanceInit();
#endif

    VerifyOrQuit(instance != NULL, "Failed to get and init an otInstance.\n");

    for (int i = 0; i < kNumTestMessages; i++)
    {
        msg[i] = otIp6NewMessage(instance, true);
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
    SuccessOrQuit(otMessageQueueEnqueue(&queue, msg[2]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[0], msg[1], msg[2]);
    SuccessOrQuit(otMessageQueueEnqueue(&queue, msg[3]), "Failed to enqueue a message to otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 4, msg[0], msg[1], msg[2], msg[3]);

    // Remove elements and check the content
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[1]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 3, msg[0], msg[2], msg[3]);
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[0]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 2, msg[2], msg[3]);
    SuccessOrQuit(otMessageQueueDequeue(&queue, msg[3]), "Failed to dequeue a message from otMessageQueue.\n");
    VerifyMessageQueueContentUsingOtApi(&queue, 1, msg[2]);

    // Check the expected failure cases for the enqueue and dequeue:
    error = otMessageQueueEnqueue(&queue, msg[2]);
    VerifyOrQuit(error == kThreadError_Already, "Enqueuing an already queued message did not fail as expected.\n");
    error = otMessageQueueDequeue(&queue, msg[0]);
    VerifyOrQuit(error == kThreadError_NotFound, "Dequeuing a message not in the queue did not fail as expected.\n");

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

    otInstanceFinalize(instance);
#ifdef OPENTHREAD_MULTIPLE_INSTANCE
    free(otInstanceBuffer);
#endif
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
