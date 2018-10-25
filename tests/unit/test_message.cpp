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
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

void TestMessage(void)
{
    ot::Instance *   instance;
    ot::MessagePool *messagePool;
    ot::Message *    message;
    uint8_t          writeBuffer[1024];
    uint8_t          readBuffer[1024];

    instance = static_cast<ot::Instance *>(testInitInstance());
    VerifyOrQuit(instance != NULL, "Null OpenThread instance\n");

    messagePool = &instance->GetMessagePool();

    for (unsigned i = 0; i < sizeof(writeBuffer); i++)
    {
        writeBuffer[i] = static_cast<uint8_t>(random());
    }

    VerifyOrQuit((message = messagePool->New(ot::Message::kTypeIp6, 0)) != NULL, "Message::New failed\n");
    SuccessOrQuit(message->SetLength(sizeof(writeBuffer)), "Message::SetLength failed\n");
    VerifyOrQuit(message->Write(0, sizeof(writeBuffer), writeBuffer) == sizeof(writeBuffer), "Message::Write failed\n");
    VerifyOrQuit(message->Read(0, sizeof(readBuffer), readBuffer) == sizeof(readBuffer), "Message::Read failed\n");
    VerifyOrQuit(memcmp(writeBuffer, readBuffer, sizeof(writeBuffer)) == 0, "Message compare failed\n");
    VerifyOrQuit(message->GetLength() == 1024, "Message::GetLength failed\n");
    message->Free();

    testFreeInstance(instance);
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestMessage();
    printf("All tests passed\n");
    return 0;
}
#endif
