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

#include "test_util.h"
#include "openthread/openthread.h"
#include <openthread-instance.h>
#include <common/debug.hpp>
#include <common/message.hpp>
#include "utils/wrap_string.h"
#include <stdarg.h>

#define kNumTestMessages      3

// This function verifies the content of the priority queue to match the passed in messages
void VerifyPriorityQueueContent(ot::PriorityQueue &aPriorityQueue, int aExpectedLength, ...)
{
    va_list args;
    ot::Message *message;
    ot::Message *msgArg;
    uint8_t curPriority = 0xff;
    uint16_t msgCount, bufCount;

    // Check the `GetInfo`
    aPriorityQueue.GetInfo(msgCount, bufCount);
    VerifyOrQuit(msgCount == aExpectedLength, "GetInfo() result does not match expected len.\n");

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aPriorityQueue.GetHead();
        VerifyOrQuit(message == NULL, "PriorityQueue is not empty when expected len is zero.\n");

        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(0) == NULL, "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(1) == NULL, "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(2) == NULL, "GetHeadForPriority() non-NULL when empty\n");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(3) == NULL, "GetHeadForPriority() non-NULL when empty\n");
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
                for (curPriority++; curPriority != msgArg->GetPriority(); curPriority++)
                {
                    // Check the `GetHeadForPriority` is NULL if there are no expected message for this priority level.
                    VerifyOrQuit(
                        aPriorityQueue.GetHeadForPriority(curPriority) == NULL,
                        "PriorityQueue::GetHeadForPriority is non-NULL when no expected msg for this priority.\n"
                    );
                }

                // Check the `GetHeadForPriority`.
                VerifyOrQuit(
                    aPriorityQueue.GetHeadForPriority(curPriority) == msgArg,
                    "PriorityQueue::GetHeadForPriority failed.\n"
                );
            }

            // Check the queued message to match the one from argument list
            VerifyOrQuit(msgArg == message, "PriorityQueue content does not match what is expected.\n");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "PriorityQueue contains less entries than expected.\n");

        // Check the `GetHeadForPriority` is NULL if there are no expected message for any remaining priority level.
        for (curPriority++; curPriority < 4; curPriority++)
        {
            VerifyOrQuit(
                aPriorityQueue.GetHeadForPriority(curPriority) == NULL,
                "PriorityQueue::GetHeadForPriority is non-NULL when no expected msg for this priority.\n"
            );
        }
    }

    va_end(args);
}

// This function verifies the content of the all message queue to match the passed in messages
void VerifyAllMessagesContent(ot::MessagePool &aMessagePool, int aExpectedLength, ...)
{
    va_list args;
    ot::MessagePool::Iterator it;
    ot::Message *msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        VerifyOrQuit(aMessagePool.GetAllMessagesHead().IsEmpty(), "Head is not empty when expected len is zero.\n");
        VerifyOrQuit(aMessagePool.GetAllMessagesTail().IsEmpty(), "Tail is not empty when expected len is zero.\n");
    }
    else
    {
        for (it = aMessagePool.GetAllMessagesHead(); !it.HasEnded() ; it.GoToNext())
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
void VerifyAllMessagesContentInReverse(ot::MessagePool &aMessagePool, int aExpectedLength, ...)
{
    va_list args;
    ot::MessagePool::Iterator it;
    ot::Message *msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        VerifyOrQuit(aMessagePool.GetAllMessagesHead().IsEmpty(), "Head is not empty when expected len is zero.\n");
        VerifyOrQuit(aMessagePool.GetAllMessagesTail().IsEmpty(), "Tail is not empty when expected len is zero.\n");
    }
    else
    {
        for (it = aMessagePool.GetAllMessagesTail(); !it.HasEnded() ; it.GoToPrev())
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
    va_list args;
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
    otInstance instance;
    ot::MessagePool messagePool(&instance);
    ot::PriorityQueue queue;
    ot::MessageQueue messageQueue;
    ot::Message *msgHigh    [kNumTestMessages];
    ot::Message *msgMed     [kNumTestMessages];
    ot::Message *msgLow     [kNumTestMessages];
    ot::Message *msgVeryLow [kNumTestMessages];
    ot::MessagePool::Iterator it;

    // Allocate messages with different priorities.
    for (int i = 0; i < kNumTestMessages; i++)
    {
        msgHigh[i] = messagePool.New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgHigh[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgHigh[i]->SetPriority(0), "Message:SetPriority failed\n");
        msgMed[i] = messagePool.New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgMed[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgMed[i]->SetPriority(1), "Message:SetPriority failed\n");
        msgLow[i] = messagePool.New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgLow[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgLow[i]->SetPriority(2), "Message:SetPriority failed\n");
        msgVeryLow[i] = messagePool.New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgVeryLow[i] != NULL, "Message::New failed\n");
        SuccessOrQuit(msgVeryLow[i]->SetPriority(3), "Message:SetPriority failed\n");
    }

    // Check the failure case for `SetPriority` for invalid argument.
    VerifyOrQuit(
        msgHigh[2]->SetPriority(ot::Message::kNumPriorities) == kThreadError_InvalidArgs,
        "Message::SetPriority() with out of range value did not fail as expected.\n"
    );

    // Check the `GetPriority()`
    for (int i = 0; i < kNumTestMessages; i++)
    {
        VerifyOrQuit(msgHigh[i]->GetPriority() == 0, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgMed[i]->GetPriority() == 1, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgLow[i]->GetPriority() == 2, "Message::GetPriority failed.\n");
        VerifyOrQuit(msgVeryLow[i]->GetPriority() == 3, "Message::GetPriority failed.\n");
    }

    // Verify case of an empty queue.
    VerifyPriorityQueueContent(queue, 0);

    // Add msgs in different orders and check the content of queue.
    SuccessOrQuit(queue.Enqueue(*msgMed[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgMed[0]);
    SuccessOrQuit(queue.Enqueue(*msgMed[1]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgMed[0], msgMed[1]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgMed[0], msgMed[1]);
    SuccessOrQuit(queue.Enqueue(*msgHigh[1]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 4, msgHigh[0], msgHigh[1], msgMed[0], msgMed[1]);
    SuccessOrQuit(queue.Enqueue(*msgMed[2]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 5, msgHigh[0], msgHigh[1], msgMed[0], msgMed[1], msgMed[2]);
    SuccessOrQuit(queue.Enqueue(*msgVeryLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 6, msgHigh[0], msgHigh[1], msgMed[0], msgMed[1], msgMed[2], msgVeryLow[0]);
    SuccessOrQuit(queue.Enqueue(*msgLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 7, msgHigh[0], msgHigh[1], msgMed[0], msgMed[1], msgMed[2], msgLow[0],
                               msgVeryLow[0]);

    // Check the MessagePool::Iterator methods.
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    VerifyOrQuit(it.GetNext().IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    VerifyOrQuit(it.GetPrev().IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    it.GoToNext();
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");
    it.GoToNext();
    VerifyOrQuit(it.IsEmpty(), "Iterator::IsEmpty() failed to return `true` for an empty iterator.\n");

    it = messagePool.GetAllMessagesHead();
    VerifyOrQuit(!it.IsEmpty(), "Iterator::IsEmpty() failed to return `false` when it is not empty.\n");
    VerifyOrQuit(it.GetMessage() == msgHigh[0], "Iterator::GetMessage() failed.\n");
    it = it.GetNext();
    VerifyOrQuit(!it.IsEmpty(), "Iterator::IsEmpty() failed to return `false` when it is not empty.\n");
    VerifyOrQuit(it.GetMessage() == msgHigh[1], "Iterator::GetNext() failed.\n");
    it = it.GetPrev();
    VerifyOrQuit(it.GetMessage() == msgHigh[0], "Iterator::GetPrev() failed.\n");
    it = it.GetPrev();
    VerifyOrQuit(it.HasEnded(), "Iterator::GetPrev() failed to return empty at head.\n");
    it = messagePool.GetAllMessagesTail();
    it = it.GetNext();
    VerifyOrQuit(it.HasEnded(), "Iterator::GetNext() failed to return empty at tail.\n");

    // Check the AllMessage queue contents (should match the content of priority queue).
    VerifyAllMessagesContent(messagePool, 7, msgHigh[0], msgHigh[1], msgMed[0], msgMed[1], msgMed[2], msgLow[0],
                             msgVeryLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 7, msgVeryLow[0], msgLow[0], msgMed[2], msgMed[1], msgMed[0],
                                      msgHigh[1], msgHigh[0]);

    // Remove messages in different order and check the content of queue in each step.
    SuccessOrQuit(queue.Dequeue(*msgHigh[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 6, msgHigh[1], msgMed[0], msgMed[1], msgMed[2], msgLow[0], msgVeryLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgMed[2]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 5, msgHigh[1], msgMed[0], msgMed[1], msgLow[0], msgVeryLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgLow[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 4, msgHigh[1], msgMed[0], msgMed[1], msgVeryLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgMed[1]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgHigh[1], msgMed[0], msgVeryLow[0]);
    SuccessOrQuit(queue.Dequeue(*msgVeryLow[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgHigh[1], msgMed[0]);
    SuccessOrQuit(queue.Dequeue(*msgHigh[1]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgMed[0]);
    SuccessOrQuit(queue.Dequeue(*msgMed[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 0);
    VerifyAllMessagesContent(messagePool, 0);

    // Check the failure cases: Enqueuing an already queued message, or dequeuing a message not queued.
    SuccessOrQuit(queue.Enqueue(*msgHigh[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgHigh[0]);
    VerifyOrQuit(
        queue.Enqueue(*msgHigh[0]) == kThreadError_Already,
        "Enqueuing an already queued message did not fail as expected.\n"
    );
    VerifyOrQuit(
        queue.Dequeue(*msgMed[0]) == kThreadError_NotFound,
        "Dequeuing a message not queued, did not fail as expected.\n"
    );
    SuccessOrQuit(queue.Dequeue(*msgHigh[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyPriorityQueueContent(queue, 0);

    // Change the priority of an already queued message and check the order change in the queue.
    SuccessOrQuit(queue.Enqueue(*msgLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 1, msgLow[0]);
    SuccessOrQuit(queue.Enqueue(*msgMed[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 2, msgMed[0], msgLow[0]);
    SuccessOrQuit(queue.Enqueue(*msgVeryLow[0]), "PriorityQueue::Enqueue() failed.\n");
    VerifyPriorityQueueContent(queue, 3, msgMed[0], msgLow[0], msgVeryLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgMed[0], msgLow[0], msgVeryLow[0]);

    SuccessOrQuit(msgLow[0]->SetPriority(0), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgLow[0], msgMed[0], msgVeryLow[0]);
    SuccessOrQuit(msgVeryLow[0]->SetPriority(3), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgLow[0], msgMed[0], msgVeryLow[0]);
    SuccessOrQuit(msgVeryLow[0]->SetPriority(2), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgLow[0], msgMed[0], msgVeryLow[0]);
    SuccessOrQuit(msgVeryLow[0]->SetPriority(1), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgLow[0], msgMed[0], msgVeryLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgLow[0], msgMed[0], msgVeryLow[0]);
    SuccessOrQuit(msgVeryLow[0]->SetPriority(0), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgLow[0], msgVeryLow[0], msgMed[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(2), "SetPriority failed for an already queued message.\n");
    SuccessOrQuit(msgVeryLow[0]->SetPriority(3), "SetPriority failed for an already queued message.\n");
    VerifyPriorityQueueContent(queue, 3, msgMed[0], msgLow[0], msgVeryLow[0]);
    VerifyAllMessagesContent(messagePool, 3, msgMed[0], msgLow[0], msgVeryLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 3, msgVeryLow[0], msgLow[0], msgMed[0]);

    // Checking the AllMessages queue when adding messages from same pool to another queue.
    SuccessOrQuit(messageQueue.Enqueue(*msgLow[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 4, msgMed[0], msgLow[0], msgLow[1], msgVeryLow[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msgMed[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 5, msgMed[0], msgMed[1], msgLow[0], msgLow[1], msgVeryLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 5, msgVeryLow[0], msgLow[1], msgLow[0], msgMed[1], msgMed[0]);
    SuccessOrQuit(messageQueue.Enqueue(*msgHigh[1]), "MessageQueue::Enqueue() failed.\n");
    VerifyAllMessagesContent(messagePool, 6, msgHigh[1], msgMed[0], msgMed[1], msgLow[0], msgLow[1], msgVeryLow[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgLow[1], msgMed[1], msgHigh[1]);

    // Change priority of message and check that order changes in the AllMessage queue and not in messageQueue.
    SuccessOrQuit(msgLow[1]->SetPriority(0), "SetPriority failed for an already queued message.\n");
    VerifyAllMessagesContent(messagePool, 6, msgHigh[1], msgLow[1], msgMed[0], msgMed[1], msgLow[0], msgVeryLow[0]);
    VerifyAllMessagesContentInReverse(messagePool, 6, msgVeryLow[0], msgLow[0], msgMed[1], msgMed[0], msgLow[1],
                                      msgHigh[1]);
    VerifyMsgQueueContent(messageQueue, 3, msgLow[1], msgMed[1], msgHigh[1]);

    SuccessOrQuit(msgVeryLow[0]->SetPriority(1), "SetPriority failed for an already queued message.\n");
    VerifyAllMessagesContent(messagePool, 6, msgHigh[1], msgLow[1], msgMed[0], msgMed[1], msgVeryLow[0], msgLow[0]);
    VerifyPriorityQueueContent(queue, 3, msgMed[0], msgVeryLow[0], msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgLow[1], msgMed[1], msgHigh[1]);

    // Remove messages from the two queues and verify that AllMessage queue is updated correctly.
    SuccessOrQuit(queue.Dequeue(*msgMed[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 5, msgHigh[1], msgLow[1], msgMed[1], msgVeryLow[0], msgLow[0]);
    VerifyPriorityQueueContent(queue, 2, msgVeryLow[0], msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgLow[1], msgMed[1], msgHigh[1]);

    SuccessOrQuit(messageQueue.Dequeue(*msgHigh[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 4, msgLow[1], msgMed[1], msgVeryLow[0], msgLow[0]);
    VerifyPriorityQueueContent(queue, 2, msgVeryLow[0], msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 2, msgLow[1], msgMed[1]);

    SuccessOrQuit(messageQueue.Dequeue(*msgMed[1]), "MessageQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 3, msgLow[1], msgVeryLow[0], msgLow[0]);
    VerifyPriorityQueueContent(queue, 2, msgVeryLow[0], msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgLow[1]);

    SuccessOrQuit(queue.Dequeue(*msgVeryLow[0]), "PriorityQueue::Dequeue() failed.\n");
    VerifyAllMessagesContent(messagePool, 2, msgLow[1], msgLow[0]);
    VerifyPriorityQueueContent(queue, 1, msgLow[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgLow[1]);
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestPriorityQueue();
    printf("All tests passed\n");
    return 0;
}
#endif
