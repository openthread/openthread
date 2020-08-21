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

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "common/random.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

void TestMessage(void)
{
    enum : uint16_t
    {
        kMaxSize = (kBufferSize * 3 + 24),
    };

    Instance *   instance;
    MessagePool *messagePool;
    Message *    message;
    uint8_t      writeBuffer[kMaxSize];
    uint8_t      readBuffer[kMaxSize];
    uint8_t      zeroBuffer[kMaxSize];

    memset(zeroBuffer, 0, sizeof(zeroBuffer));

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance\n");

    messagePool = &instance->Get<MessagePool>();

    Random::NonCrypto::FillBuffer(writeBuffer, kMaxSize);

    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");
    SuccessOrQuit(message->SetLength(kMaxSize), "Message::SetLength failed");
    message->Write(0, kMaxSize, writeBuffer);
    VerifyOrQuit(message->Read(0, kMaxSize, readBuffer) == kMaxSize, "Message::Read failed");
    VerifyOrQuit(memcmp(writeBuffer, readBuffer, kMaxSize) == 0, "Message compare failed");
    VerifyOrQuit(message->GetLength() == kMaxSize, "Message::GetLength failed");

    for (uint16_t offset = 0; offset < kMaxSize; offset++)
    {
        for (uint16_t length = 0; length < kMaxSize - offset; length++)
        {
            for (uint16_t i = 0; i < length; i++)
            {
                writeBuffer[offset + i]++;
            }

            message->Write(offset, length, &writeBuffer[offset]);

            VerifyOrQuit(message->Read(0, kMaxSize, readBuffer) == kMaxSize, "Message::Read failed");
            VerifyOrQuit(memcmp(writeBuffer, readBuffer, kMaxSize) == 0, "Message compare failed");

            memset(readBuffer, 0, sizeof(readBuffer));
            VerifyOrQuit(message->Read(offset, length, readBuffer) == length, "Message::Read failed");
            VerifyOrQuit(memcmp(readBuffer, &writeBuffer[offset], length) == 0, "Message compare failed");
            VerifyOrQuit(memcmp(&readBuffer[length], zeroBuffer, kMaxSize - length) == 0, "Message read after length");
        }
    }

    VerifyOrQuit(message->GetLength() == kMaxSize, "Message::GetLength failed");
    message->Free();

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestMessage();
    printf("All tests passed\n");
    return 0;
}
