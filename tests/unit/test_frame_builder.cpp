/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#include "test_platform.h"
#include "test_util.hpp"

#include "common/frame_builder.hpp"

namespace ot {

void TestFrameBuilder(void)
{
    const uint8_t kData1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    const uint8_t kData2[] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa};
    const uint8_t kData3[] = {0xca, 0xfe, 0xbe, 0xef};

    static constexpr uint16_t kMaxBufferSize = sizeof(kData1) * 2 + sizeof(kData2);

    Instance    *instance;
    Message     *message;
    uint16_t     offset;
    uint8_t      buffer[kMaxBufferSize];
    uint8_t      zeroBuffer[kMaxBufferSize];
    FrameBuilder frameBuilder;

    printf("TestFrameBuilder\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Append(kData1));
    SuccessOrQuit(message->Append(kData2));

    memset(buffer, 0, sizeof(buffer));
    memset(zeroBuffer, 0, sizeof(zeroBuffer));

    frameBuilder.Init(buffer, sizeof(buffer));

    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(frameBuilder.GetLength() == 0);
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(buffer));
    VerifyOrQuit(memcmp(buffer, zeroBuffer, sizeof(buffer)) == 0);
    VerifyOrQuit(frameBuilder.CanAppend(sizeof(buffer)));
    VerifyOrQuit(!frameBuilder.CanAppend(sizeof(buffer) + 1));

    frameBuilder.SetMaxLength(sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(frameBuilder.GetLength() == 0);
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(kData1));
    VerifyOrQuit(memcmp(buffer, zeroBuffer, sizeof(buffer)) == 0);
    VerifyOrQuit(frameBuilder.CanAppend(sizeof(kData1)));
    VerifyOrQuit(!frameBuilder.CanAppend(sizeof(kData1) + 1));

    SuccessOrQuit(frameBuilder.Append(kData1));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), zeroBuffer, sizeof(buffer) - sizeof(kData1)) == 0);

    frameBuilder.SetMaxLength(sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(buffer));
    VerifyOrQuit(frameBuilder.CanAppend(sizeof(buffer) - sizeof(kData1)));
    VerifyOrQuit(!frameBuilder.CanAppend(sizeof(buffer) - sizeof(kData1) + 1));

    SuccessOrQuit(frameBuilder.AppendUint8(0x01));
    SuccessOrQuit(frameBuilder.AppendBigEndianUint16(0x0203));
    SuccessOrQuit(frameBuilder.AppendLittleEndianUint16(0x0504));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) * 2);
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), kData1, sizeof(kData1)) == 0);

    SuccessOrQuit(frameBuilder.AppendBigEndianUint32(0x01020304));
    SuccessOrQuit(frameBuilder.AppendUint8(0x05));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) * 3);
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + 2 * sizeof(kData1), kData1, sizeof(kData1)) == 0);

    frameBuilder.Init(buffer, sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(frameBuilder.GetLength() == 0);
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(buffer));

    offset = sizeof(kData1);
    SuccessOrQuit(frameBuilder.AppendBytesFromMessage(*message, offset, sizeof(kData2)));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData2));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);

    frameBuilder.Init(buffer, sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(frameBuilder.GetLength() == 0);
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(buffer));

    SuccessOrQuit(frameBuilder.AppendLittleEndianUint32(0x04030201));
    SuccessOrQuit(frameBuilder.AppendUint8(0x05));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);

    SuccessOrQuit(frameBuilder.AppendBytes(zeroBuffer, sizeof(kData2)));
    SuccessOrQuit(frameBuilder.Append(kData1));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), zeroBuffer, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1) + sizeof(kData2), kData1, sizeof(kData1)) == 0);

    VerifyOrQuit(!frameBuilder.CanAppend(1));
    VerifyOrQuit(frameBuilder.AppendUint8(0x00) == kErrorNoBufs);

    offset = sizeof(kData1);
    frameBuilder.Write(offset, kData2);
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1) + sizeof(kData2), kData1, sizeof(kData1)) == 0);

    frameBuilder.Init(buffer, sizeof(buffer));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(frameBuilder.GetLength() == 0);
    VerifyOrQuit(frameBuilder.GetMaxLength() == sizeof(buffer));

    offset = 0;
    SuccessOrQuit(frameBuilder.Insert(offset, kData1));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);

    offset = 0;
    SuccessOrQuit(frameBuilder.Insert(offset, kData2));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) + sizeof(kData2));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2), kData1, sizeof(kData1)) == 0);

    offset = sizeof(kData2);
    SuccessOrQuit(frameBuilder.InsertBytes(offset, kData3, sizeof(kData3)));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) + sizeof(kData2) + sizeof(kData3));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2), kData3, sizeof(kData3)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2) + sizeof(kData3), kData1, sizeof(kData1)) == 0);

    offset = frameBuilder.GetLength();
    SuccessOrQuit(frameBuilder.Insert<uint8_t>(offset, 0x77));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) + sizeof(kData2) + sizeof(kData3) + sizeof(uint8_t));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2), kData3, sizeof(kData3)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2) + sizeof(kData3), kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(buffer[sizeof(kData2) + sizeof(kData3) + sizeof(kData1)] == 0x77);

    offset = frameBuilder.GetLength() - 1;
    frameBuilder.RemoveBytes(offset, sizeof(uint8_t));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) + sizeof(kData2) + sizeof(kData3));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2), kData3, sizeof(kData3)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2) + sizeof(kData3), kData1, sizeof(kData1)) == 0);

    offset = sizeof(kData2);
    frameBuilder.RemoveBytes(offset, sizeof(kData3));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1) + sizeof(kData2));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData2), kData1, sizeof(kData1)) == 0);

    offset = 0;
    frameBuilder.RemoveBytes(offset, sizeof(kData2));
    VerifyOrQuit(frameBuilder.GetLength() == sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetBytes() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);

    offset = 0;
    frameBuilder.RemoveBytes(offset, sizeof(kData1));
    VerifyOrQuit(frameBuilder.GetLength() == 0);

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestFrameBuilder();

    printf("All tests passed\n");
    return 0;
}
