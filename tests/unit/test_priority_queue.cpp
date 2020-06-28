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
    VerifyOrQuit(msgCount == aExpectedLength, "GetInfo() result does not match expected len.");

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aPriorityQueue.GetHead();
        VerifyOrQuit(message == nullptr, "PriorityQueue is not empty when expected len is zero.");

        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityLow) == nullptr,
                     "GetHeadForPriority() non-nullptr when empty");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityNormal) == nullptr,
                     "GetHeadForPriority() non-nullptr when empty");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityHigh) == nullptr,
                     "GetHeadForPriority() non-nullptr when empty");
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(ot::Message::kPriorityNet) == nullptr,
                     "GetHeadForPriority() non-nullptr when empty");
    }
    else
    {
        // Go through all messages in the queue and verify they match the passed-in messages
        for (message = aPriorityQueue.GetHead(); message != nullptr; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "PriorityQueue contains more entries than expected.");

            msgArg = va_arg(args, ot::Message *);

            if (msgArg->GetPriority() != curPriority)
            {
                for (curPriority--; curPriority != msgArg->GetPriority(); curPriority--)
                {
                    // Check the `GetHeadForPriority` is nullptr if there are no expected message for this priority
                    // level.
                    VerifyOrQuit(
                        aPriorityQueue.GetHeadForPriority(static_cast<ot::Message::Priority>(curPriority)) == nullptr,
                        "PriorityQueue::GetHeadForPriority is non-nullptr when no expected msg for this priority.");
                }

                // Check the `GetHeadForPriority`.
                VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<ot::Message::Priority>(curPriority)) ==
                                 msgArg,
                             "PriorityQueue::GetHeadForPriority failed.");
            }

            // Check the queued message to match the one from argument list
            VerifyOrQuit(msgArg == message, "PriorityQueue content does not match what is expected.");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "PriorityQueue contains less entries than expected.");

        // Check the `GetHeadForPriority` is nullptr if there are no expected message for any remaining priority level.
        for (curPriority--; curPriority >= 0; curPriority--)
        {
            VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<ot::Message::Priority>(curPriority)) == nullptr,
                         "PriorityQueue::GetHeadForPriority is non-nullptr when no expected msg for this priority.");
        }
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

void TestPriorityQueue(void)
{
    ot::Instance *    instance;
    ot::MessagePool * messagePool;
    ot::PriorityQueue queue;
    ot::MessageQueue  messageQueue;
    ot::Message *     msgNet[kNumTestMessages];
    ot::Message *     msgHigh[kNumTestMessages];
    ot::Message *     msgNor[kNumTestMessages];
    ot::Message *     msgLow[kNumTestMessages];

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<ot::MessagePool>();

    // Use the function "New()" to allocate messages with different priorities
    for (int i = 0; i < kNumNewPriorityTestMessages; i++)
    {
        msgNet[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityNet);
        VerifyOrQuit(msgNet[i] != nullptr, "Message::New failed");
        msgHigh[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityHigh);
        VerifyOrQuit(msgHigh[i] != nullptr, "Message::New failed");
        msgNor[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityNormal);
        VerifyOrQuit(msgNor[i] != nullptr, "Message::New failed");
        msgLow[i] = messagePool->New(ot::Message::kTypeIp6, 0, ot::Message::kPriorityLow);
        VerifyOrQuit(msgLow[i] != nullptr, "Message::New failed");
    }

    // Use the function "SetPriority()" to allocate messages with different priorities
    for (int i = kNumNewPriorityTestMessages; i < kNumTestMessages; i++)
    {
        msgNet[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgNet[i] != nullptr, "Message::New failed");
        SuccessOrQuit(msgNet[i]->SetPriority(ot::Message::kPriorityNet), "Message:SetPriority failed");
        msgHigh[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgHigh[i] != nullptr, "Message::New failed");
        SuccessOrQuit(msgHigh[i]->SetPriority(ot::Message::kPriorityHigh), "Message:SetPriority failed");
        msgNor[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgNor[i] != nullptr, "Message::New failed");
        SuccessOrQuit(msgNor[i]->SetPriority(ot::Message::kPriorityNormal), "Message:SetPriority failed");
        msgLow[i] = messagePool->New(ot::Message::kTypeIp6, 0);
        VerifyOrQuit(msgLow[i] != nullptr, "Message::New failed");
        SuccessOrQuit(msgLow[i]->SetPriority(ot::Message::kPriorityLow), "Message:SetPriority failed");
    }

    // Check the `GetPriority()`
    for (int i = 0; i < kNumTestMessages; i++)
    {
        VerifyOrQuit(msgLow[i]->GetPriority() == ot::Message::kPriorityLow, "Message::GetPriority failed.");
        VerifyOrQuit(msgNor[i]->GetPriority() == ot::Message::kPriorityNormal, "Message::GetPriority failed.");
        VerifyOrQuit(msgHigh[i]->GetPriority() == ot::Message::kPriorityHigh, "Message::GetPriority failed.");
        VerifyOrQuit(msgNet[i]->GetPriority() == ot::Message::kPriorityNet, "Message::GetPriority failed.");
    }

    // Verify case of an empty queue.
    VerifyPriorityQueueContent(queue, 0);

    // Add msgs in different orders and check the content of queue.
    queue.Enqueue(*msgHigh[0]);
    VerifyPriorityQueueContent(queue, 1, msgHigh[0]);
    queue.Enqueue(*msgHigh[1]);
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgHigh[1]);
    queue.Enqueue(*msgNet[0]);
    VerifyPriorityQueueContent(queue, 3, msgNet[0], msgHigh[0], msgHigh[1]);
    queue.Enqueue(*msgNet[1]);
    VerifyPriorityQueueContent(queue, 4, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1]);
    queue.Enqueue(*msgHigh[2]);
    VerifyPriorityQueueContent(queue, 5, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2]);
    queue.Enqueue(*msgLow[0]);
    VerifyPriorityQueueContent(queue, 6, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgLow[0]);
    queue.Enqueue(*msgNor[0]);
    VerifyPriorityQueueContent(queue, 7, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgNor[0],
                               msgLow[0]);
    queue.Enqueue(*msgHigh[3]);
    VerifyPriorityQueueContent(queue, 8, msgNet[0], msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgHigh[3],
                               msgNor[0], msgLow[0]);

    // Remove messages in different order and check the content of queue in each step.
    queue.Dequeue(*msgNet[0]);
    VerifyPriorityQueueContent(queue, 7, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[2], msgHigh[3], msgNor[0],
                               msgLow[0]);
    queue.Dequeue(*msgHigh[2]);
    VerifyPriorityQueueContent(queue, 6, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[3], msgNor[0], msgLow[0]);
    queue.Dequeue(*msgNor[0]);
    VerifyPriorityQueueContent(queue, 5, msgNet[1], msgHigh[0], msgHigh[1], msgHigh[3], msgLow[0]);
    queue.Dequeue(*msgHigh[1]);
    VerifyPriorityQueueContent(queue, 4, msgNet[1], msgHigh[0], msgHigh[3], msgLow[0]);
    queue.Dequeue(*msgLow[0]);
    VerifyPriorityQueueContent(queue, 3, msgNet[1], msgHigh[0], msgHigh[3]);
    queue.Dequeue(*msgNet[1]);
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgHigh[3]);
    queue.Dequeue(*msgHigh[0]);
    VerifyPriorityQueueContent(queue, 1, msgHigh[3]);
    queue.Dequeue(*msgHigh[3]);
    VerifyPriorityQueueContent(queue, 0);

    // Change the priority of an already queued message and check the order change in the queue.
    queue.Enqueue(*msgNor[0]);
    VerifyPriorityQueueContent(queue, 1, msgNor[0]);
    queue.Enqueue(*msgHigh[0]);
    VerifyPriorityQueueContent(queue, 2, msgHigh[0], msgNor[0]);
    queue.Enqueue(*msgLow[0]);
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgNor[0], msgLow[0]);

    SuccessOrQuit(msgNor[0]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityLow),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityNormal),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityHigh),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgLow[0], msgHigh[0]);
    SuccessOrQuit(msgNor[0]->SetPriority(ot::Message::kPriorityNormal),
                  "SetPriority failed for an already queued message.");
    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityLow),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgNor[0], msgLow[0]);

    messageQueue.Enqueue(*msgNor[1]);
    messageQueue.Enqueue(*msgHigh[1]);
    messageQueue.Enqueue(*msgNet[1]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    // Change priority of message and check for not in messageQueue.
    SuccessOrQuit(msgNor[1]->SetPriority(ot::Message::kPriorityNet),
                  "SetPriority failed for an already queued message.");
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    SuccessOrQuit(msgLow[0]->SetPriority(ot::Message::kPriorityHigh),
                  "SetPriority failed for an already queued message.");
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    // Remove messages from the two queues
    queue.Dequeue(*msgHigh[0]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    messageQueue.Dequeue(*msgNet[1]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 2, msgNor[1], msgHigh[1]);

    messageQueue.Dequeue(*msgHigh[1]);
    VerifyPriorityQueueContent(queue, 2, msgLow[0], msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgNor[1]);

    queue.Dequeue(*msgLow[0]);
    VerifyPriorityQueueContent(queue, 1, msgNor[0]);
    VerifyMsgQueueContent(messageQueue, 1, msgNor[1]);

    testFreeInstance(instance);
}

int main(void)
{
    TestPriorityQueue();
    printf("All tests passed\n");
    return 0;
}
