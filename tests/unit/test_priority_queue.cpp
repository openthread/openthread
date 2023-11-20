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
#include "common/message.hpp"
#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.h"

namespace ot {

#define kNumNewPriorityTestMessages 2
#define kNumSetPriorityTestMessages 2
#define kNumTestMessages (kNumNewPriorityTestMessages + kNumSetPriorityTestMessages)

// This function verifies the content of the priority queue to match the passed in messages
void VerifyPriorityQueueContent(PriorityQueue &aPriorityQueue, int aExpectedLength, ...)
{
    const PriorityQueue &constQueue = aPriorityQueue;
    va_list              args;
    Message             *message;
    Message             *msgArg;
    int8_t               curPriority = Message::kNumPriorities;
    PriorityQueue::Info  info;

    // Check the `GetInfo`
    memset(&info, 0, sizeof(info));
    aPriorityQueue.GetInfo(info);
    VerifyOrQuit(info.mNumMessages == aExpectedLength, "GetInfo() result does not match expected len.");

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aPriorityQueue.GetHead();
        VerifyOrQuit(message == nullptr, "PriorityQueue is not empty when expected len is zero.");

        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(Message::kPriorityLow) == nullptr);
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(Message::kPriorityNormal) == nullptr);
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(Message::kPriorityHigh) == nullptr);
        VerifyOrQuit(aPriorityQueue.GetHeadForPriority(Message::kPriorityNet) == nullptr);
    }
    else
    {
        // Go through all messages in the queue and verify they match the passed-in messages
        for (message = aPriorityQueue.GetHead(); message != nullptr; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "PriorityQueue contains more entries than expected.");

            msgArg = va_arg(args, Message *);

            if (msgArg->GetPriority() != curPriority)
            {
                for (curPriority--; curPriority != msgArg->GetPriority(); curPriority--)
                {
                    // Check the `GetHeadForPriority` is nullptr if there are no expected message for this priority
                    // level.
                    VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<Message::Priority>(curPriority)) ==
                                     nullptr,
                                 "is non-nullptr when no expected msg for this priority.");
                }

                // Check the `GetHeadForPriority`.
                VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<Message::Priority>(curPriority)) == msgArg);
            }

            // Check the queued message to match the one from argument list
            VerifyOrQuit(msgArg == message, "PriorityQueue content does not match what is expected.");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "PriorityQueue contains less entries than expected.");

        // Check the `GetHeadForPriority` is nullptr if there are no expected message for any remaining priority level.
        for (curPriority--; curPriority >= 0; curPriority--)
        {
            VerifyOrQuit(aPriorityQueue.GetHeadForPriority(static_cast<Message::Priority>(curPriority)) == nullptr,
                         "is non-nullptr when no expected msg for this priority.");
        }
    }

    va_end(args);

    // Check range-based `for` loop iteration using non-const iterator

    message = aPriorityQueue.GetHead();

    for (Message &msg : aPriorityQueue)
    {
        VerifyOrQuit(message == &msg, "`for` loop iteration does not match expected");
        message = message->GetNext();
    }

    VerifyOrQuit(message == nullptr, "`for` loop iteration resulted in fewer entries than expected");

    // Check  range-base `for` iteration using const iterator

    message = aPriorityQueue.GetHead();

    for (const Message &constMsg : constQueue)
    {
        VerifyOrQuit(message == &constMsg, "`for` loop iteration does not match expected");
        message = message->GetNext();
    }

    VerifyOrQuit(message == nullptr, "`for` loop iteration resulted in fewer entries than expected");
}

// This function verifies the content of the message queue to match the passed in messages
void VerifyMsgQueueContent(MessageQueue &aMessageQueue, int aExpectedLength, ...)
{
    va_list  args;
    Message *message;
    Message *msgArg;

    va_start(args, aExpectedLength);

    if (aExpectedLength == 0)
    {
        message = aMessageQueue.GetHead();
        VerifyOrQuit(message == nullptr, "is not empty when expected len is zero.");
    }
    else
    {
        for (message = aMessageQueue.GetHead(); message != nullptr; message = message->GetNext())
        {
            VerifyOrQuit(aExpectedLength != 0, "contains more entries than expected");

            msgArg = va_arg(args, Message *);
            VerifyOrQuit(msgArg == message, "content does not match what is expected.");

            aExpectedLength--;
        }

        VerifyOrQuit(aExpectedLength == 0, "contains less entries than expected");
    }

    va_end(args);
}

void TestPriorityQueue(void)
{
    Instance     *instance;
    MessagePool  *messagePool;
    PriorityQueue queue;
    MessageQueue  messageQueue;
    Message      *msgNet[kNumTestMessages];
    Message      *msgHigh[kNumTestMessages];
    Message      *msgNor[kNumTestMessages];
    Message      *msgLow[kNumTestMessages];

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();

    // Use the function "Allocate()" to allocate messages with different priorities
    for (int i = 0; i < kNumNewPriorityTestMessages; i++)
    {
        msgNet[i] = messagePool->Allocate(Message::kTypeIp6, 0, Message::Settings(Message::kPriorityNet));
        VerifyOrQuit(msgNet[i] != nullptr);
        msgHigh[i] = messagePool->Allocate(Message::kTypeIp6, 0, Message::Settings(Message::kPriorityHigh));
        VerifyOrQuit(msgHigh[i] != nullptr);
        msgNor[i] = messagePool->Allocate(Message::kTypeIp6, 0, Message::Settings(Message::kPriorityNormal));
        VerifyOrQuit(msgNor[i] != nullptr);
        msgLow[i] = messagePool->Allocate(Message::kTypeIp6, 0, Message::Settings(Message::kPriorityLow));
        VerifyOrQuit(msgLow[i] != nullptr);
    }

    // Use the function "SetPriority()" to allocate messages with different priorities
    for (int i = kNumNewPriorityTestMessages; i < kNumTestMessages; i++)
    {
        msgNet[i] = messagePool->Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgNet[i] != nullptr);
        SuccessOrQuit(msgNet[i]->SetPriority(Message::kPriorityNet));
        msgHigh[i] = messagePool->Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgHigh[i] != nullptr);
        SuccessOrQuit(msgHigh[i]->SetPriority(Message::kPriorityHigh));
        msgNor[i] = messagePool->Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgNor[i] != nullptr);
        SuccessOrQuit(msgNor[i]->SetPriority(Message::kPriorityNormal));
        msgLow[i] = messagePool->Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgLow[i] != nullptr);
        SuccessOrQuit(msgLow[i]->SetPriority(Message::kPriorityLow));
    }

    // Check the `GetPriority()`
    for (int i = 0; i < kNumTestMessages; i++)
    {
        VerifyOrQuit(msgLow[i]->GetPriority() == Message::kPriorityLow);
        VerifyOrQuit(msgNor[i]->GetPriority() == Message::kPriorityNormal);
        VerifyOrQuit(msgHigh[i]->GetPriority() == Message::kPriorityHigh);
        VerifyOrQuit(msgNet[i]->GetPriority() == Message::kPriorityNet);
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

    SuccessOrQuit(msgNor[0]->SetPriority(Message::kPriorityNet));
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityLow));
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityNormal));
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityHigh));
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgHigh[0], msgLow[0]);
    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityNet));
    VerifyPriorityQueueContent(queue, 3, msgNor[0], msgLow[0], msgHigh[0]);
    SuccessOrQuit(msgNor[0]->SetPriority(Message::kPriorityNormal));
    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityLow));
    VerifyPriorityQueueContent(queue, 3, msgHigh[0], msgNor[0], msgLow[0]);

    messageQueue.Enqueue(*msgNor[1]);
    messageQueue.Enqueue(*msgHigh[1]);
    messageQueue.Enqueue(*msgNet[1]);
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    // Change priority of message and check for not in messageQueue.
    SuccessOrQuit(msgNor[1]->SetPriority(Message::kPriorityNet));
    VerifyMsgQueueContent(messageQueue, 3, msgNor[1], msgHigh[1], msgNet[1]);

    SuccessOrQuit(msgLow[0]->SetPriority(Message::kPriorityHigh));
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

    queue.Dequeue(*msgNor[0]);
    VerifyPriorityQueueContent(queue, 0);
    messageQueue.Dequeue(*msgNor[1]);
    VerifyMsgQueueContent(messageQueue, 0);

    for (Message *message : msgNor)
    {
        SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));
    }

    // Range-based `for` and dequeue during iteration

    for (uint16_t removeIndex = 0; removeIndex < 4; removeIndex++)
    {
        uint16_t index = 0;

        queue.Enqueue(*msgNor[0]);
        queue.Enqueue(*msgNor[1]);
        queue.Enqueue(*msgNor[2]);
        queue.Enqueue(*msgNor[3]);
        VerifyPriorityQueueContent(queue, 4, msgNor[0], msgNor[1], msgNor[2], msgNor[3]);

        // While iterating over the queue remove the entry at `removeIndex`
        for (Message &message : queue)
        {
            if (index == removeIndex)
            {
                queue.Dequeue(message);
            }

            VerifyOrQuit(&message == msgNor[index++]);
        }

        index = 0;

        // Iterate over the queue and remove all
        for (Message &message : queue)
        {
            if (index == removeIndex)
            {
                index++;
            }

            VerifyOrQuit(&message == msgNor[index++]);
            queue.Dequeue(message);
        }

        VerifyPriorityQueueContent(queue, 0);
    }

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestPriorityQueue();
    printf("All tests passed\n");
    return 0;
}
