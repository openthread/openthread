/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <openthread/config.h>

#include "common/message.hpp"
#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.h"

#if OPENTHREAD_CONFIG_MESSAGE_BUFFER_THRESHOLD_ENABLE

namespace ot {

class UnitTester
{
public:
    struct Context
    {
        uint16_t mLowCalls;
        uint16_t mHighCalls;
        void    *mExpectedContext;
    };

    static void HandleLowThreshold(void *aContext)
    {
        Context *context = static_cast<Context *>(aContext);

        VerifyOrQuit(aContext == context->mExpectedContext);
        context->mLowCalls++;
    }

    static void HandleHighThreshold(void *aContext)
    {
        Context *context = static_cast<Context *>(aContext);

        VerifyOrQuit(aContext == context->mExpectedContext);
        context->mHighCalls++;
    }

    static void TestMessageBufferThreshold(void)
    {
        static constexpr uint16_t kMaxMessages = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;

        Instance    *instance;
        MessagePool *messagePool;
        Context      context;
        Message     *messages[kMaxMessages];
        uint16_t     numAllocated = 0;
        uint16_t     totalBuffers;
        uint16_t     highThreshold;
        uint16_t     lowThreshold;

        printf("TestMessageBufferThreshold()\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        messagePool = &instance->Get<MessagePool>();

        totalBuffers = messagePool->GetTotalBufferCount();
        VerifyOrQuit(totalBuffers == kMaxMessages);

        highThreshold = totalBuffers * OPENTHREAD_CONFIG_MESSAGE_BUFFER_HIGH_THRESHOLD / 100;
        lowThreshold  = totalBuffers * OPENTHREAD_CONFIG_MESSAGE_BUFFER_LOW_THRESHOLD / 100;

        // Ensure there is enough margin above the high threshold to
        // avoid exhausting the buffer pool during the test.
        VerifyOrQuit(highThreshold + 5 < totalBuffers);
        VerifyOrQuit(lowThreshold < highThreshold);

        context.mLowCalls        = 0;
        context.mHighCalls       = 0;
        context.mExpectedContext = &context;

        otMessageSetBufferThresholdCallback(instance, HandleLowThreshold, HandleHighThreshold, &context);

        // Allocate one message at a time and verify that each buffer
        // allocated consumes exactly one buffer (messages with zero
        // length only use their embedded head buffer).

        while (numAllocated < highThreshold)
        {
            VerifyOrQuit((messages[numAllocated] = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            numAllocated++;

            VerifyOrQuit(messagePool->GetFreeBufferCount() == totalBuffers - numAllocated);
            VerifyOrQuit(context.mHighCalls == 0, "High threshold callback fired too early");
        }

        // Allocating one more buffer should cross the high threshold
        // and invoke the high callback exactly once.

        VerifyOrQuit((messages[numAllocated] = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
        numAllocated++;
        VerifyOrQuit(context.mHighCalls == 1, "High threshold callback did not fire");
        VerifyOrQuit(context.mLowCalls == 0);

        // Further allocations above the high threshold should not
        // re-trigger the high callback (hysteresis/latch behavior).

        while (numAllocated < highThreshold + 5)
        {
            VerifyOrQuit((messages[numAllocated] = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            numAllocated++;
            VerifyOrQuit(context.mHighCalls == 1, "High threshold callback fired more than once");
        }

        // Now free messages one at a time. The low callback should
        // not fire until usage drops strictly below the low threshold.

        while (numAllocated > lowThreshold)
        {
            numAllocated--;
            messages[numAllocated]->Free();
            VerifyOrQuit(context.mLowCalls == 0, "Low threshold callback fired too early");
        }

        VerifyOrQuit(numAllocated == lowThreshold);
        numAllocated--;
        messages[numAllocated]->Free();
        VerifyOrQuit(context.mLowCalls == 1, "Low threshold callback did not fire");
        VerifyOrQuit(context.mHighCalls == 1);

        // Further frees below the low threshold should not re-trigger
        // the low callback.

        while (numAllocated > 0)
        {
            numAllocated--;
            messages[numAllocated]->Free();
            VerifyOrQuit(context.mLowCalls == 1, "Low threshold callback fired more than once");
        }

        VerifyOrQuit(messagePool->GetFreeBufferCount() == totalBuffers);

        // Verify that crossing the high threshold again re-triggers
        // the high callback (the latch was reset by the low crossing).

        while (numAllocated <= highThreshold)
        {
            VerifyOrQuit((messages[numAllocated] = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            numAllocated++;
        }

        VerifyOrQuit(context.mHighCalls == 2, "High threshold callback did not re-trigger");

        while (numAllocated > 0)
        {
            numAllocated--;
            messages[numAllocated]->Free();
        }

        VerifyOrQuit(context.mLowCalls == 2, "Low threshold callback did not re-trigger");
        VerifyOrQuit(messagePool->GetFreeBufferCount() == totalBuffers);

        // Unregister the callbacks and verify they are no longer invoked.

        otMessageSetBufferThresholdCallback(instance, nullptr, nullptr, nullptr);

        while (numAllocated <= highThreshold + 5)
        {
            VerifyOrQuit((messages[numAllocated] = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            numAllocated++;
        }

        VerifyOrQuit(context.mHighCalls == 2, "High threshold callback fired while unregistered");

        while (numAllocated > 0)
        {
            numAllocated--;
            messages[numAllocated]->Free();
        }

        VerifyOrQuit(context.mLowCalls == 2, "Low threshold callback fired while unregistered");
        VerifyOrQuit(messagePool->GetFreeBufferCount() == totalBuffers);

        testFreeInstance(instance);
    }
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_MESSAGE_BUFFER_THRESHOLD_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_MESSAGE_BUFFER_THRESHOLD_ENABLE
    ot::UnitTester::TestMessageBufferThreshold();
#endif

    printf("All tests passed\n");
    return 0;
}
