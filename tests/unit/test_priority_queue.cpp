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

// NOLINTBEGIN(modernize-loop-convert)
void TestPriorityQueue(void)
{
    static constexpr uint16_t kNumNewPriorityTestMessages = 2;
    static constexpr uint16_t kNumSetPriorityTestMessages = 2;
    static constexpr uint16_t kNumTestMessages            = (kNumNewPriorityTestMessages + kNumSetPriorityTestMessages);

    // Use two char abbreviated names for different priority levels;
    static constexpr uint8_t kNw            = Message::kPriorityNet;    // Network level (highest priority)
    static constexpr uint8_t kHi            = Message::kPriorityHigh;   // High priority
    static constexpr uint8_t kMd            = Message::kPriorityNormal; // Middle (or Normal) priority
    static constexpr uint8_t kLo            = Message::kPriorityLow;    // Low priority.
    static constexpr uint8_t kNumPriorities = Message::kNumPriorities;

    Instance     *instance;
    MessagePool  *messagePool;
    PriorityQueue queue;
    Message      *msg[kNumPriorities][kNumTestMessages];

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();

    for (uint8_t prio = 0; prio < kNumPriorities; prio++)
    {
        Message::Priority priority = static_cast<Message::Priority>(prio);

        // Use the function "Allocate()" to allocate messages with different priorities
        for (int i = 0; i < kNumNewPriorityTestMessages; i++)
        {
            msg[prio][i] = messagePool->Allocate(Message::kTypeIp6, 0, Message::Settings(priority));
            VerifyOrQuit(msg[prio][i] != nullptr);
        }

        // Use the function "SetPriority()" to allocate messages with different priorities
        for (int i = kNumNewPriorityTestMessages; i < kNumTestMessages; i++)
        {
            msg[prio][i] = messagePool->Allocate(Message::kTypeIp6);
            VerifyOrQuit(msg[prio][i] != nullptr);
            SuccessOrQuit(msg[prio][i]->SetPriority(priority));
        }
    }

    // Check the `GetPriority()`
    for (int i = 0; i < kNumTestMessages; i++)
    {
        VerifyOrQuit(msg[kLo][i]->GetPriority() == Message::kPriorityLow);
        VerifyOrQuit(msg[kMd][i]->GetPriority() == Message::kPriorityNormal);
        VerifyOrQuit(msg[kHi][i]->GetPriority() == Message::kPriorityHigh);
        VerifyOrQuit(msg[kNw][i]->GetPriority() == Message::kPriorityNet);
    }

    // Verify case of an empty queue.
    VerifyPriorityQueueContent(queue, 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Messages with the same priority only.

    for (uint8_t prio = 0; prio < kNumPriorities; prio++)
    {
        // Add and remove one message only
        queue.Enqueue(*msg[prio][0]);
        VerifyPriorityQueueContent(queue, 1, msg[prio][0]);
        queue.Dequeue(*msg[prio][0]);
        VerifyPriorityQueueContent(queue, 0);

        // Add three messages and then dequeue them in different orders.

        for (uint8_t test = 0; test < 3; test++)
        {
            queue.Enqueue(*msg[prio][0]);
            VerifyPriorityQueueContent(queue, 1, msg[prio][0]);
            queue.Enqueue(*msg[prio][1]);
            VerifyPriorityQueueContent(queue, 2, msg[prio][0], msg[prio][1]);
            queue.Enqueue(*msg[prio][2]);
            VerifyPriorityQueueContent(queue, 3, msg[prio][0], msg[prio][1], msg[prio][2]);

            switch (test)
            {
            case 0:
                // Remove the messages in the same order added.
                queue.Dequeue(*msg[prio][0]);
                VerifyPriorityQueueContent(queue, 2, msg[prio][1], msg[prio][2]);
                queue.Dequeue(*msg[prio][1]);
                VerifyPriorityQueueContent(queue, 1, msg[prio][2]);
                queue.Dequeue(*msg[prio][2]);
                break;

            case 1:
                // Remove the message in the reverse order added
                queue.Dequeue(*msg[prio][2]);
                VerifyPriorityQueueContent(queue, 2, msg[prio][0], msg[prio][1]);
                queue.Dequeue(*msg[prio][1]);
                VerifyPriorityQueueContent(queue, 1, msg[prio][0]);
                queue.Dequeue(*msg[prio][0]);
                break;

            case 2:
                // Remove the message in random order added.
                queue.Dequeue(*msg[prio][1]);
                VerifyPriorityQueueContent(queue, 2, msg[prio][0], msg[prio][2]);
                queue.Dequeue(*msg[prio][0]);
                VerifyPriorityQueueContent(queue, 1, msg[prio][2]);
                queue.Dequeue(*msg[prio][2]);
                break;
            }

            VerifyPriorityQueueContent(queue, 0);
        }
    }

    VerifyPriorityQueueContent(queue, 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Messages with the two different priorities (`prio1` lower than `prio2`)

    for (uint8_t prio1 = 0; prio1 < kNumPriorities - 1; prio1++)
    {
        for (uint8_t prio2 = prio1 + 1; prio2 < kNumPriorities; prio2++)
        {
            // Add one message with `prio1` and one with `prio2` and
            // then remove them. Cover all possible combinations
            // (order to add and remove).

            for (uint8_t test = 0; test < 4; test++)
            {
                switch (test)
                {
                case 0:
                case 1:
                    // Add lower priority first, then higher priority
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Enqueue(*msg[prio2][0]);
                    break;

                case 2:
                case 3:
                    // Add higher priority first, then lower priority
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Enqueue(*msg[prio1][0]);
                    break;
                }

                VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);

                switch (test)
                {
                case 0:
                case 2:
                    // Remove lower priority first, then higher priority.
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    break;

                case 1:
                case 3:
                    // Remove higher priority first, then lower priority
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    break;
                }

                VerifyPriorityQueueContent(queue, 0);
            }

            // Add two messages with `prio1` (lower) and one with
            // `prio2` (higher) and then remove them. Cover all
            // possible combinations to add and remove.

            for (uint8_t test = 0; test < 6; test++)
            {
                switch (test)
                {
                case 0:
                case 1:
                    // Add two lower priority messages first, then one higher priority
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Enqueue(*msg[prio1][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio1][0], msg[prio1][1]);
                    queue.Enqueue(*msg[prio2][0]);
                    break;

                case 2:
                case 3:
                    // Add one higher priority first, then two lower priority messages.
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Enqueue(*msg[prio1][1]);
                    break;

                case 4:
                case 5:
                    // Add one lower priority first, then a higher priority, finally one lower priority.
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Enqueue(*msg[prio1][1]);
                    break;
                }

                VerifyPriorityQueueContent(queue, 3, msg[prio2][0], msg[prio1][0], msg[prio1][1]);

                switch (test)
                {
                case 0:
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][1]);
                    queue.Dequeue(*msg[prio1][1]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    break;

                case 1:
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][1]);
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][1]);
                    queue.Dequeue(*msg[prio1][1]);
                    break;

                case 2:
                    queue.Dequeue(*msg[prio1][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    break;

                case 3:
                    queue.Dequeue(*msg[prio1][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    break;

                case 4:
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio1][0], msg[prio1][1]);
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][1]);
                    queue.Dequeue(*msg[prio1][1]);
                    break;

                case 5:
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio1][0], msg[prio1][1]);
                    queue.Dequeue(*msg[prio1][1]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    break;
                }
            }

            VerifyPriorityQueueContent(queue, 0);

            // Add two messages with `prio2` (higher) and one with
            // `prio1` (lower) and then remove them. Cover all
            // possible combinations to add and remove.

            for (uint8_t test = 0; test < 6; test++)
            {
                switch (test)
                {
                case 0:
                case 1:
                    // Add two higher priority messages first, then one lower priority
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Enqueue(*msg[prio2][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio2][1]);
                    queue.Enqueue(*msg[prio1][0]);
                    break;

                case 2:
                case 3:
                    // Add one lower priority first, then two higher priority messages.
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Enqueue(*msg[prio2][1]);
                    break;

                case 4:
                case 5:
                    // Add one higher priority first, then a lower priority, finally one higher priority.
                    queue.Enqueue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Enqueue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Enqueue(*msg[prio2][1]);
                    break;
                }

                VerifyPriorityQueueContent(queue, 3, msg[prio2][0], msg[prio2][1], msg[prio1][0]);

                switch (test)
                {
                case 0:
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][1], msg[prio1][0]);
                    queue.Dequeue(*msg[prio2][1]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    break;

                case 1:
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][1], msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][1]);
                    queue.Dequeue(*msg[prio2][1]);
                    break;

                case 2:
                    queue.Dequeue(*msg[prio2][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    break;

                case 3:
                    queue.Dequeue(*msg[prio2][1]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    break;

                case 4:
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio2][1]);
                    queue.Dequeue(*msg[prio2][0]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][1]);
                    queue.Dequeue(*msg[prio2][1]);
                    break;

                case 5:
                    queue.Dequeue(*msg[prio1][0]);
                    VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio2][1]);
                    queue.Dequeue(*msg[prio2][1]);
                    VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                    queue.Dequeue(*msg[prio2][0]);
                    break;
                }

                VerifyPriorityQueueContent(queue, 0);
            }
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Messages with the three different priorities (prio1 < prio2 < prio3)

    for (uint8_t prio1 = 0; prio1 < kNumPriorities - 2; prio1++)
    {
        for (uint8_t prio2 = prio1 + 1; prio2 < kNumPriorities - 1; prio2++)
        {
            for (uint8_t prio3 = prio2 + 1; prio3 < kNumPriorities; prio3++)
            {
                for (uint8_t test = 0; test < 6; test++)
                {
                    switch (test)
                    {
                    case 0:
                        queue.Enqueue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                        queue.Enqueue(*msg[prio2][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                        queue.Enqueue(*msg[prio3][0]);
                        break;

                    case 1:
                        queue.Enqueue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio1][0]);
                        queue.Enqueue(*msg[prio3][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio3][0], msg[prio1][0]);
                        queue.Enqueue(*msg[prio2][0]);
                        break;

                    case 2:
                        queue.Enqueue(*msg[prio2][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                        queue.Enqueue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                        queue.Enqueue(*msg[prio3][0]);
                        break;

                    case 3:
                        queue.Enqueue(*msg[prio2][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                        queue.Enqueue(*msg[prio3][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio3][0], msg[prio2][0]);
                        queue.Enqueue(*msg[prio1][0]);
                        break;

                    case 4:
                        queue.Enqueue(*msg[prio3][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio3][0]);
                        queue.Enqueue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio3][0], msg[prio1][0]);
                        queue.Enqueue(*msg[prio2][0]);
                        break;

                    case 5:
                        queue.Enqueue(*msg[prio3][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio3][0]);
                        queue.Enqueue(*msg[prio2][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio3][0], msg[prio2][0]);
                        queue.Enqueue(*msg[prio1][0]);
                        break;
                    }

                    VerifyPriorityQueueContent(queue, 3, msg[prio3][0], msg[prio2][0], msg[prio1][0]);

                    switch (test)
                    {
                    case 0:
                    case 1:
                    case 2:
                        queue.Dequeue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio3][0], msg[prio2][0]);
                        queue.Dequeue(*msg[prio2][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio3][0]);
                        queue.Dequeue(*msg[prio3][0]);
                        break;
                    case 3:
                    case 4:
                    case 5:
                        queue.Dequeue(*msg[prio3][0]);
                        VerifyPriorityQueueContent(queue, 2, msg[prio2][0], msg[prio1][0]);
                        queue.Dequeue(*msg[prio1][0]);
                        VerifyPriorityQueueContent(queue, 1, msg[prio2][0]);
                        queue.Dequeue(*msg[prio2][0]);
                        break;
                    }

                    VerifyPriorityQueueContent(queue, 0);
                }
            }
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add msgs in different orders and check the content of queue.

    queue.Enqueue(*msg[kHi][0]);
    VerifyPriorityQueueContent(queue, 1, msg[kHi][0]);
    queue.Enqueue(*msg[kHi][1]);
    VerifyPriorityQueueContent(queue, 2, msg[kHi][0], msg[kHi][1]);
    queue.Enqueue(*msg[kNw][0]);
    VerifyPriorityQueueContent(queue, 3, msg[kNw][0], msg[kHi][0], msg[kHi][1]);
    queue.Enqueue(*msg[kNw][1]);
    VerifyPriorityQueueContent(queue, 4, msg[kNw][0], msg[kNw][1], msg[kHi][0], msg[kHi][1]);
    queue.Enqueue(*msg[kHi][2]);
    VerifyPriorityQueueContent(queue, 5, msg[kNw][0], msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][2]);
    queue.Enqueue(*msg[kLo][0]);
    VerifyPriorityQueueContent(queue, 6, msg[kNw][0], msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][2], msg[kLo][0]);
    queue.Enqueue(*msg[kMd][0]);
    VerifyPriorityQueueContent(queue, 7, msg[kNw][0], msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][2], msg[kMd][0],
                               msg[kLo][0]);
    queue.Enqueue(*msg[kHi][3]);
    VerifyPriorityQueueContent(queue, 8, msg[kNw][0], msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][2], msg[kHi][3],
                               msg[kMd][0], msg[kLo][0]);

    // Remove messages in different order and check the content of queue in each step.
    queue.Dequeue(*msg[kNw][0]);
    VerifyPriorityQueueContent(queue, 7, msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][2], msg[kHi][3], msg[kMd][0],
                               msg[kLo][0]);
    queue.Dequeue(*msg[kHi][2]);
    VerifyPriorityQueueContent(queue, 6, msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][3], msg[kMd][0], msg[kLo][0]);
    queue.Dequeue(*msg[kMd][0]);
    VerifyPriorityQueueContent(queue, 5, msg[kNw][1], msg[kHi][0], msg[kHi][1], msg[kHi][3], msg[kLo][0]);
    queue.Dequeue(*msg[kHi][1]);
    VerifyPriorityQueueContent(queue, 4, msg[kNw][1], msg[kHi][0], msg[kHi][3], msg[kLo][0]);
    queue.Dequeue(*msg[kLo][0]);
    VerifyPriorityQueueContent(queue, 3, msg[kNw][1], msg[kHi][0], msg[kHi][3]);
    queue.Dequeue(*msg[kNw][1]);
    VerifyPriorityQueueContent(queue, 2, msg[kHi][0], msg[kHi][3]);
    queue.Dequeue(*msg[kHi][0]);
    VerifyPriorityQueueContent(queue, 1, msg[kHi][3]);
    queue.Dequeue(*msg[kHi][3]);
    VerifyPriorityQueueContent(queue, 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that priority of an already queued message in a priority queue cannot change
    queue.Enqueue(*msg[kMd][0]);
    VerifyPriorityQueueContent(queue, 1, msg[kMd][0]);
    queue.Enqueue(*msg[kHi][0]);
    VerifyPriorityQueueContent(queue, 2, msg[kHi][0], msg[kMd][0]);
    queue.Enqueue(*msg[kLo][0]);
    VerifyPriorityQueueContent(queue, 3, msg[kHi][0], msg[kMd][0], msg[kLo][0]);

    VerifyOrQuit(msg[kMd][0]->SetPriority(Message::kPriorityNet) == kErrorInvalidState);
    VerifyOrQuit(msg[kLo][0]->SetPriority(Message::kPriorityLow) == kErrorInvalidState);
    VerifyOrQuit(msg[kLo][0]->SetPriority(Message::kPriorityNormal) == kErrorInvalidState);
    VerifyOrQuit(msg[kHi][0]->SetPriority(Message::kPriorityNormal) == kErrorInvalidState);
    VerifyPriorityQueueContent(queue, 3, msg[kHi][0], msg[kMd][0], msg[kLo][0]);

    // Remove messages from the queue
    queue.Dequeue(*msg[kHi][0]);
    VerifyPriorityQueueContent(queue, 2, msg[kMd][0], msg[kLo][0]);
    queue.Dequeue(*msg[kLo][0]);
    VerifyPriorityQueueContent(queue, 1, msg[kMd][0]);
    queue.Dequeue(*msg[kMd][0]);
    VerifyPriorityQueueContent(queue, 0);

    for (Message *message : msg[kMd])
    {
        SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Range-based `for` and dequeue during iteration

    for (uint16_t removeIndex = 0; removeIndex < 4; removeIndex++)
    {
        uint16_t index = 0;

        queue.Enqueue(*msg[kMd][0]);
        queue.Enqueue(*msg[kMd][1]);
        queue.Enqueue(*msg[kMd][2]);
        queue.Enqueue(*msg[kMd][3]);
        VerifyPriorityQueueContent(queue, 4, msg[kMd][0], msg[kMd][1], msg[kMd][2], msg[kMd][3]);

        // While iterating over the queue remove the entry at `removeIndex`
        for (Message &message : queue)
        {
            if (index == removeIndex)
            {
                queue.Dequeue(message);
            }

            VerifyOrQuit(&message == msg[kMd][index++]);
        }

        index = 0;

        // Iterate over the queue and remove all
        for (Message &message : queue)
        {
            if (index == removeIndex)
            {
                index++;
            }

            VerifyOrQuit(&message == msg[kMd][index++]);
            queue.Dequeue(message);
        }

        VerifyPriorityQueueContent(queue, 0);
    }

    testFreeInstance(instance);
}
// NOLINTEND(modernize-loop-convert)

} // namespace ot

int main(void)
{
    ot::TestPriorityQueue();
    printf("All tests passed\n");
    return 0;
}
