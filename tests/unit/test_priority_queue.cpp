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

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

#define kNumNewPriorityTestMessages 2
#define kNumSetPriorityTestMessages 2
#define kNumTestMessages (kNumNewPriorityTestMessages + kNumSetPriorityTestMessages)

// This function verifies the content of the priority queue to match the passed in messages
void VerifyPriorityQueueContent(ot::PriorityQueue &aPriorityQueue, int aExpectedLength, ...)
{
    va_list      args;
    ot::Message *message;
    ot::Message *msgArg;
    int8_t       curPriority = ot::Message::kNumPriorities;
    uint16_t     msgCount, bufCount;

    // Check the `GetInfo`
    aPriorityQueue.GetInfo(msgCount, bufCount);
    VerifyOrQuit(msgCount == aExpectedLength, "GetInfo() result does not match expected len.\n");

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aPriorityQueue.GetHead();
        VerifyOrQuit(message == NULL, "PriorityQueue is not empty when expected len is zero.\n");

        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityLow) == NULL,
                     "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityNormal) == NULL,
                     "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityHigh) == NULL,
                     "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityNet) == NULL,
                     "GetHeadForPriority() non-NULL when empty\n");
    }
    else
    {
        // Go through all messages in the queue and verify they match the passed-in messages
        for (message = aPriorityQueue.GetHead(); message != NULL; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "PriorityQueue contains more entries than expected.\n");

            msgArg = va_arg(args, ot::Message *);

            if (msgArg->GetPriority() != curPriority)
            {
                for (curPriority--; curPriority != msgArg->GetPriority(); curPriority--)
                {
                    // Check the `GetHeadForPriority` is NULL if there are no expected message for this priority level.
                    VerifyOrQuit(
                        aPriorityQueue.GetHeadForPriority(static_cast<uint8_t>(curPriority)) == NULL,
                        "PriorityQueue::GetHeadForPriority is non-NULL when no expected msg for this priority.\n");
                }

                // Check the `GetHeadForPriority`.
                VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<uint8_t>(curPriority)) == msgArg,
                             "PriorityQueue::GetHeadForPriority failed.\n");
            }

            // Check the queued message to match the one from argument list
            VerifyOrQuit(msgArg == message, "PriorityQueue content does not match what is expected.\n");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "PriorityQueue contains less entries than expected.\n");

        // Check the `GetHeadForPriority` is NULL if there are no expected message for any remaining priority level.
        for (curPriority--; curPriority >= 0; curPriority--)
        {
            VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<uint8_t>(curPriority)) == NULL,
                         "PriorityQueue::GetHeadForPriority is non-NULL when no expected msg for this priority.\n");
        }
    }

    va_end(args);
}

// This function verifies the content of the all message queue to match the passed in messages
void VerifyAllMessagesContent(ot::MessagePool *aMessagePool, int aExpectedLength, ...)
{
    va_list                   args;
    ot::MessagePool::Iterator it;
    ot::Message *             msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        VerifyOrQuit(aMessagePool->GetAllMessagesHead().IsEmpty(), "Head is not empty when expected len is zero.\n");
        VerifyOrQuit(aMessagePool->GetAllMessagesTail().IsEmpty(), "Tail is not empty when expected len is zero.\n");
    }
    else
    {
        for (it = aMessagePool->GetAllMessagesHead(); !it.HasEnded(); it.GoToNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "AllMessagesQueue contains more entries than expected.\n");
            msgArg = va_arg(args, ot::Message *);
            VerifyOrQuit(msgArg == it.GetMessage(), "AllMessagesQueue content does not match what is expected.\n");
            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "AllMessagesQueue contains less entries than expected.\n");
    }

    va_end(args);
}

// This function verifies the content of the all message queue to match the passed in messages. It goes
// through the AllMessages list in reverse.
void VerifyAllMessagesContentInReverse(ot::MessagePool *aMessagePool, int aExpectedLength, ...)
{
    va_list                   args;
    ot::MessagePool::Iterator it;
    ot::Message *             msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        VerifyOrQuit(aMessagePool->GetAllMessagesHead().IsEmpty(), "Head is not empty when expected len is zero.\n");
        VerifyOrQuit(aMessagePool->GetAllMessagesTail().IsEmpty(), "Tail is not empty when expected len is zero.\n");
    }
    else
    {
        for (it = aMessagePool->GetAllMessagesTail(); !it.HasEnded(); it.GoToPrev())
        {
            VerifyOrQuit(aExpectedLength != 0, "AllMessagesQueue contains more entries than expected.\n");
            msgArg = va_arg(args, ot::Message *);
            VerifyOrQuit(msgArg == it.GetMessage(), "AllMessagesQueue content does not match what is expected.\n");
            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "AllMessagesQueue contains less entries than expected.\n");
    }

    va_end(args);
}

// This function verifies the content of the message queue to match the passed in messages
void VerifyMsgQueueContent(ot::MessageQueue &aMessageQueue, int aExpectedLength, ...)
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

void TestPriorityQueue(void)
{
    ot::Instance *            instance;
    ot::MessagePool *         messagePool;
    ot::PriorityQueue         queue;
    ot::MessageQueue          messageQueue;
    ot::Message *             msgNet[kNumTestMessages];
    ot::Message *             msgHigh[kNumTestMessages];
    ot::Message *             msgNor[kNumTestMessages];
    ot::Message *             msgLow[kNumTestMessages];
    ot::MessagePool::Iterator it;

    instance = testInitInstance();
    VerifyOrQuit(instance != NULL, "Null OpenThread instance\n");

    messagePool = &instance->GetMessagePool();

    // Use the function "New()" to allocate messages with different priorities
    for (int i = 0; i < kNumNewPriorityTestMessages; i++)
    {
        msgNet[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityNet);
        VerifyOrQuit(msgNet[i] != NULL, "Message::New failed\n");
        msgHigh[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityHigh);
        VerifyOrQuit(msgHigh[i] != NULL, "Message::New failed\n");
        msgNor[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityNormal);
        VerifyOrQuit(msgNor[i] != NULL, "Message::New failed\n");
        msgLow[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityLow);
        VerifyOrQuit(msgLow[i] != NULL, "Message::New failed\n");
    }

    // Check the failure case for `New()` for invalid argument.
    VerifyOrQuit(messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kNumPriorities) == NULL,
                 "Message::New() with out of range value did not fail as expected.\n");

    // Use the function "SetPriority()" to allocate messages with different priorities
    for (int i = kNumNewPriorityTestMessages; i < kNumTestMessages; i++)
    {
        msgNet[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgNet[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgNet[i]->SetPriority(ot::Message::kPriorityNet), "Message:SetPriority failed\n");
        msgHigh[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgHigh[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgHigh[i]->SetPriority(ot::Message::kPriorityHigh), "Message:SetPriority failed\n");
        msgNor[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgNor[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgNor[i]->SetPriority(ot::Message::kPriorityNormal), "Message:SetPriority failed\n");
        msgLow[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgLow[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgLow[i]->SetPriority(ot::Message::kPriorityLow), "Message:SetPriority failed\n");
    }

    // Check the failure case for `SetPriority()` for invalid argument.
    VerifyOrQuit(msgNet[2]->SetPriority(ot::Message::kNumPriorities) == OT_ERROR_INVALID_ARGS,
                 "Message::SetPriority() with out of range value did not fail as expected.\n");

    // Check the `GetPriority()`
    for (int i = 0; i < kNumTestMessages; i++)
    {
        VerifyOrQuit(msgLow[i]->GetPriority() == ot::Message::kPriorityLow, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgNor[i]->GetPriority() == ot::Message::kPriorityNormal, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgHigh[i]->GetPriority() == ot::Message::kPriorityHigh, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgNet[i]->GetPriority() == ot::Message::kPriorityNet, "Message::GetPriority failed.\n");
    }

    // Verify case of an empty queue.
    VerifyPriorityQueueContent(queue, 0);

    // Add msgs in different orders and check the content of queue.
    SuccessOrQuit(queue.Enqueue(*msgHigh[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgHigh[0]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[1]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgHigh[1]);
    SuccessOrQuit(queue.Enqueue(*msgNet[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgNet[0], msgHigh[0], msgHigh[1]);
    SuccessOrQuit(queue.Enqueue(*msgNet[1]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 4, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[2]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 5, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2]);
    SuccessOrQuit(queue.Enqueue(*msgLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 6, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgLow[0]);
    SuccessOrQuit(queue.Enqueue(*msgNor[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 7, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgNor[0],
                               msgLow[0]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[3]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 8, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgHigh[3],
                               msgNor[0], msgLow[0]);

    // Check the MessagePool::Iterator methods.
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    VerifyOrQuit(it.GetNext().IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    VerifyOrQuit(it.GetPrev().IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    it.GoToNext();
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    it.GoToNext();
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");

    it = messagePool->GetAllMessagesHead();
    VerifyOrQuit(!it.IsEmpty(), "Iterator::IsEmpty() failed to return `false` when it is not empty.\n");
    VerifyOrQuit(it.GetMessage() == msgNet[0], "Iterator::GetMessage() failed.\n");
    it = it.GetNext();
    VerifyOrQuit(!it.IsEmpty(), "Iterator::IsEmpty() failed to return `false` when it is not empty.\n");
    VerifyOrQuit(it.GetMessage() == msgNet[1], "Iterator::GetNext() failed.\n");
    it = it.GetPrev();
    VerifyOrQuit(it.GetMessage() == msgNet[0], "Iterator::GetPrev() failed.\n");
    it = it.GetPrev();
    VerifyOrQuit(it.HasEnded(), "Iterator::GetPrev() failed to return empty at head.\n");
    it = messagePool->GetAllMessagesTail();
    it = it.GetNext();
    VerifyOrQuit(it.HasEnded(), "Iterator::GetNext() failed to return empty at tail.\n");

    // Check the AllMessage queue contents (should match the content of priority queue).
    VerifyAllMessagesContent(messagePool, 8, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgHigh[3],
                             msgNor[0], msgLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 8, msgLow[0], msgNor[0], msgHigh[3], msgHigh[2], msgHigh[1],
                                      msgHigh[0], msgNet[1], msgNet[0]);

    // Remove messages in different order and check the content of queue in each step.
    SuccessOrQuit(queue.Dequeue(*msgNet[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 7, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgHigh[3], msgNor[0],
                               msgLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgHigh[2]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 6, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[3], msgNor[0], msgLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgNor[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 5, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[3], msgLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgHigh[1]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 4, msgNet[1], msgHigh[0], msgHigh[3], msgLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgLow[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgNet[1], msgHigh[0], msgHigh[3]);
    SuccessOrQuit(queue.Dequeue(*msgNet[1]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgHigh[3]);
    SuccessOrQuit(queue.Dequeue(*msgHigh[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgHigh[3]);
    SuccessOrQuit(queue.Dequeue(*msgHigh[3]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 0);
    VerifyAllMessagesContent(messagePool, 0);

    // Check the failure cases: Enqueuing an already queued message, or dequeuing a message not queued.
    SuccessOrQuit(queue.Enqueue(*msgNet[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgNet[0]);
    VerifyOrQuit(queue.Enqueue(*msgNet[0]) == OT_ERROR_ALREADY,
                 "Enqueuing an already queued message did not fail as expected.\n");
    VerifyOrQuit(queue.Dequeue(*msgHigh[0]) == OT_ERROR_NOT_FOUND,
                 "Dequeuing a message not queued, did not fail as expected.\n");
    SuccessOrQuit(queue.Dequeue(*msgNet[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 0);

    // Change the priority of an already queued message and check the order change in the queue.
    SuccessOrQuit(queue.Enqueue(*msgNor[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgNor[0]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgNor[0]);
    SuccessOrQuit(queue.Enqueue(*msgLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgNor[0], msgLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgHigh[0], msgNor[0], msgLow[0]);

    SuccessOrQuit(msgNor[0]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityLow),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityNormal),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityHigh),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgLow[0], msgHigh[0]);
    SuccessOrQuit(msgNor[0]->SetPriority(ot::Message::kPriorityNormal),
                  "SetPriority failed for an already queued message.\n");
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityLow),
                  "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgNor[0], msgLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgHigh[0], msgNor[0], msgLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 3, msgLow[0], msgNor[0], msgHigh[0]);

    // Checking the AllMessages queue when adding messages from same pool to another queue.
    SuccessOrQuit(messageQueue.Enqueue(*msgNor[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 4, msgHigh[0], msgNor[0], msgNor[1], msgLow[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msgHigh[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 5, msgHigh[0], msgHigh[1], msgNor[0], msgNor[1], msgLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 5, msgLow[0], msgNor[1], msgNor[0], msgHigh[1], msgHigh[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msgNet[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 6, msgNet[1], msgHigh[0], msgHigh[1], msgNor[0], msgNor[1], msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    // Change priority of message and check that order changes in the AllMessage queue and not in messageQueue.
    SuccessOrQuit(msgNor[1]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.\n");
    VerifyAllMessagesContent(messagePool, 6, msgNet[1], msgNor[1], msgHigh[0], msgHigh[1], msgNor[0], msgLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 6, msgLow[0], msgNor[0], msgHigh[1], msgHigh[0], msgNor[1],
                                      msgNet[1]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityHigh),
                  "SetPriority failed for an already queued message.\n");
    VerifyAllMessagesContent(messagePool, 6, msgNet[1], msgNor[1], msgHigh[0], msgHigh[1], msgLow[0], msgNor[0]);
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    // Remove messages from the two queues and verify that AllMessage queue is updated correctly.
    SuccessOrQuit(queue.Dequeue(*msgHigh[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 5, msgNet[1], msgNor[1], msgHigh[1], msgLow[0], msgNor[0]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    SuccessOrQuit(messageQueue.Dequeue(*msgNet[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 4, msgNor[1], msgHigh[1], msgLow[0], msgNor[0]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 2, msgNor[1], msgHigh[1]);

    SuccessOrQuit(messageQueue.Dequeue(*msgHigh[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 3, msgNor[1], msgLow[0], msgNor[0]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgNor[1]);

    SuccessOrQuit(queue.Dequeue(*msgLow[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 2, msgNor[1], msgNor[0]);
    VerifyPriorityQueueContent(queue, 1, msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgNor[1]);

    testFreeInstance(instance);
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestPriorityQueue();
    printf("All tests passed\n");
    return 0;
}
#endif
