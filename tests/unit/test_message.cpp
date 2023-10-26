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

#include "common/appender.hpp"
#include "common/debug.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

void TestMessage(void)
{
    enum : uint16_t
    {
        kMaxSize    = (kBufferSize * 3 + 24),
        kOffsetStep = 101,
        kLengthStep = 21,
    };

    Instance    *instance;
    MessagePool *messagePool;
    Message     *message;
    Message     *message2;
    Message     *messageCopy;
    uint8_t      writeBuffer[kMaxSize];
    uint8_t      readBuffer[kMaxSize];
    uint8_t      zeroBuffer[kMaxSize];

    printf("TestMessage\n");

    memset(zeroBuffer, 0, sizeof(zeroBuffer));

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    messagePool = &instance->Get<MessagePool>();

    Random::NonCrypto::FillBuffer(writeBuffer, kMaxSize);

    VerifyOrQuit((message = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
    message->SetLinkSecurityEnabled(Message::kWithLinkSecurity);
    SuccessOrQuit(message->SetPriority(Message::Priority::kPriorityNet));
    message->SetType(Message::Type::kType6lowpan);
    message->SetSubType(Message::SubType::kSubTypeMleChildIdRequest);
    message->SetLoopbackToHostAllowed(true);
    message->SetOrigin(Message::kOriginHostUntrusted);
    SuccessOrQuit(message->SetLength(kMaxSize));
    message->WriteBytes(0, writeBuffer, kMaxSize);
    SuccessOrQuit(message->Read(0, readBuffer, kMaxSize));
    VerifyOrQuit(memcmp(writeBuffer, readBuffer, kMaxSize) == 0);
    VerifyOrQuit(message->CompareBytes(0, readBuffer, kMaxSize));
    VerifyOrQuit(message->Compare(0, readBuffer));
    VerifyOrQuit(message->GetLength() == kMaxSize);

    // Verify `Clone()` behavior
    message->SetOffset(15);
    messageCopy = message->Clone();
    VerifyOrQuit(messageCopy->GetOffset() == message->GetOffset());
    SuccessOrQuit(messageCopy->Read(0, readBuffer, kMaxSize));
    VerifyOrQuit(memcmp(writeBuffer, readBuffer, kMaxSize) == 0);
    VerifyOrQuit(messageCopy->CompareBytes(0, readBuffer, kMaxSize));
    VerifyOrQuit(messageCopy->Compare(0, readBuffer));
    VerifyOrQuit(messageCopy->GetLength() == kMaxSize);
    VerifyOrQuit(messageCopy->GetType() == message->GetType());
    VerifyOrQuit(messageCopy->GetSubType() == message->GetSubType());
    VerifyOrQuit(messageCopy->IsLinkSecurityEnabled() == message->IsLinkSecurityEnabled());
    VerifyOrQuit(messageCopy->GetPriority() == message->GetPriority());
    VerifyOrQuit(messageCopy->IsLoopbackToHostAllowed() == message->IsLoopbackToHostAllowed());
    VerifyOrQuit(messageCopy->GetOrigin() == message->GetOrigin());
    VerifyOrQuit(messageCopy->Compare(0, readBuffer));
    message->SetOffset(0);

    messageCopy->Free();

    for (uint16_t offset = 0; offset < kMaxSize; offset++)
    {
        for (uint16_t length = 0; length <= kMaxSize - offset; length++)
        {
            for (uint16_t i = 0; i < length; i++)
            {
                writeBuffer[offset + i]++;
            }

            message->WriteBytes(offset, &writeBuffer[offset], length);

            SuccessOrQuit(message->Read(0, readBuffer, kMaxSize));
            VerifyOrQuit(memcmp(writeBuffer, readBuffer, kMaxSize) == 0);
            VerifyOrQuit(message->Compare(0, writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            SuccessOrQuit(message->Read(offset, readBuffer, length));
            VerifyOrQuit(memcmp(readBuffer, &writeBuffer[offset], length) == 0);
            VerifyOrQuit(memcmp(&readBuffer[length], zeroBuffer, kMaxSize - length) == 0, "read after length");

            VerifyOrQuit(message->CompareBytes(offset, &writeBuffer[offset], length));

            if (length == 0)
            {
                continue;
            }

            // Change the first byte, and then last byte, and verify that
            // `CompareBytes()` correctly fails.

            writeBuffer[offset]++;
            VerifyOrQuit(!message->CompareBytes(offset, &writeBuffer[offset], length));
            writeBuffer[offset]--;

            writeBuffer[offset + length - 1]++;
            VerifyOrQuit(!message->CompareBytes(offset, &writeBuffer[offset], length));
            writeBuffer[offset + length - 1]--;
        }

        // Verify `ReadBytes()` behavior when requested read length goes beyond available bytes in the message.

        for (uint16_t length = kMaxSize - offset + 1; length <= kMaxSize + 1; length++)
        {
            uint16_t readLength;

            memset(readBuffer, 0, sizeof(readBuffer));
            readLength = message->ReadBytes(offset, readBuffer, length);

            VerifyOrQuit(readLength < length, "Message::ReadBytes() returned longer length");
            VerifyOrQuit(readLength == kMaxSize - offset);
            VerifyOrQuit(memcmp(readBuffer, &writeBuffer[offset], readLength) == 0);
            VerifyOrQuit(memcmp(&readBuffer[readLength], zeroBuffer, kMaxSize - readLength) == 0, "read after length");

            VerifyOrQuit(!message->CompareBytes(offset, readBuffer, length));
            VerifyOrQuit(message->CompareBytes(offset, readBuffer, readLength));
        }
    }

    VerifyOrQuit(message->GetLength() == kMaxSize);

    // Test `WriteBytesFromMessage()` behavior copying between different
    // messages.

    VerifyOrQuit((message2 = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
    SuccessOrQuit(message2->SetLength(kMaxSize));

    for (uint16_t readOffset = 0; readOffset < kMaxSize; readOffset += kOffsetStep)
    {
        for (uint16_t writeOffset = 0; writeOffset < kMaxSize; writeOffset += kOffsetStep)
        {
            for (uint16_t length = 0; length <= kMaxSize - Max(writeOffset, readOffset); length += kLengthStep)
            {
                message2->WriteBytes(0, zeroBuffer, kMaxSize);

                message2->WriteBytesFromMessage(writeOffset, *message, readOffset, length);

                SuccessOrQuit(message2->Read(0, readBuffer, kMaxSize));

                VerifyOrQuit(memcmp(&readBuffer[0], zeroBuffer, writeOffset) == 0);
                VerifyOrQuit(memcmp(&readBuffer[writeOffset], &writeBuffer[readOffset], length) == 0);
                VerifyOrQuit(memcmp(&readBuffer[writeOffset + length], zeroBuffer, kMaxSize - length - writeOffset) ==
                             0);

                VerifyOrQuit(message->CompareBytes(readOffset, *message2, writeOffset, length));
                VerifyOrQuit(message2->CompareBytes(writeOffset, *message, readOffset, length));
            }
        }
    }

    // Verify `WriteBytesFromMessage()` behavior copying backwards within
    // same message.

    for (uint16_t readOffset = 0; readOffset < kMaxSize; readOffset++)
    {
        uint16_t length = kMaxSize - readOffset;

        message->WriteBytes(0, writeBuffer, kMaxSize);

        message->WriteBytesFromMessage(0, *message, readOffset, length);

        SuccessOrQuit(message->Read(0, readBuffer, kMaxSize));

        VerifyOrQuit(memcmp(&readBuffer[0], &writeBuffer[readOffset], length) == 0);
        VerifyOrQuit(memcmp(&readBuffer[length], &writeBuffer[length], kMaxSize - length) == 0);
    }

    // Verify `WriteBytesFromMessage()` behavior copying forward within
    // same message.

    for (uint16_t writeOffset = 0; writeOffset < kMaxSize; writeOffset++)
    {
        uint16_t length = kMaxSize - writeOffset;

        message->WriteBytes(0, writeBuffer, kMaxSize);

        message->WriteBytesFromMessage(writeOffset, *message, 0, length);

        SuccessOrQuit(message->Read(0, readBuffer, kMaxSize));

        VerifyOrQuit(memcmp(&readBuffer[0], &writeBuffer[0], writeOffset) == 0);
        VerifyOrQuit(memcmp(&readBuffer[writeOffset], &writeBuffer[0], length) == 0);
    }

    // Test `WriteBytesFromMessage()` behavior copying within same
    // message at different read and write offsets and lengths.

    for (uint16_t readOffset = 0; readOffset < kMaxSize; readOffset += kOffsetStep)
    {
        for (uint16_t writeOffset = 0; writeOffset < kMaxSize; writeOffset += kOffsetStep)
        {
            for (uint16_t length = 0; length <= kMaxSize - Max(writeOffset, readOffset); length += kLengthStep)
            {
                message->WriteBytes(0, writeBuffer, kMaxSize);

                message->WriteBytesFromMessage(writeOffset, *message, readOffset, length);

                SuccessOrQuit(message->Read(0, readBuffer, kMaxSize));

                VerifyOrQuit(memcmp(&readBuffer[0], writeBuffer, writeOffset) == 0);
                VerifyOrQuit(memcmp(&readBuffer[writeOffset], &writeBuffer[readOffset], length) == 0);
                VerifyOrQuit(memcmp(&readBuffer[writeOffset + length], &writeBuffer[writeOffset + length],
                                    kMaxSize - length - writeOffset) == 0);
            }
        }
    }

    // Verify `AppendBytesFromMessage()` with two different messages as source and destination.

    message->WriteBytes(0, writeBuffer, kMaxSize);

    for (uint16_t srcOffset = 0; srcOffset < kMaxSize; srcOffset += kOffsetStep)
    {
        for (uint16_t dstOffset = 0; dstOffset < kMaxSize; dstOffset += kOffsetStep)
        {
            for (uint16_t length = 0; length <= kMaxSize - srcOffset; length += kLengthStep)
            {
                IgnoreError(message2->SetLength(0));
                SuccessOrQuit(message2->AppendBytes(zeroBuffer, dstOffset));

                SuccessOrQuit(message2->AppendBytesFromMessage(*message, srcOffset, length));

                VerifyOrQuit(message2->CompareBytes(dstOffset, *message, srcOffset, length));
            }

            VerifyOrQuit(message2->AppendBytesFromMessage(*message, srcOffset, kMaxSize - srcOffset + 1) ==
                         kErrorParse);
        }
    }

    // Verify `AppendBytesFromMessage()` with the same message as source and destination.

    for (uint16_t srcOffset = 0; srcOffset < kMaxSize; srcOffset += kOffsetStep)
    {
        uint16_t size = kMaxSize;

        for (uint16_t length = 0; length <= kMaxSize - srcOffset; length++)
        {
            // Reset the `message` to its original size.
            IgnoreError(message->SetLength(size));

            SuccessOrQuit(message->AppendBytesFromMessage(*message, srcOffset, length));

            VerifyOrQuit(message->CompareBytes(size, *message, srcOffset, length));
        }
    }

    message->Free();
    message2->Free();

    // Verify `RemoveHeader()`

    for (uint16_t offset = 0; offset < kMaxSize; offset += kOffsetStep)
    {
        for (uint16_t length = 0; length <= kMaxSize - offset; length += kLengthStep)
        {
            VerifyOrQuit((message = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            SuccessOrQuit(message->AppendBytes(writeBuffer, kMaxSize));

            message->RemoveHeader(offset, length);

            VerifyOrQuit(message->GetLength() == kMaxSize - length);

            SuccessOrQuit(message->Read(0, readBuffer, kMaxSize - length));

            VerifyOrQuit(memcmp(&readBuffer[0], &writeBuffer[0], offset) == 0);
            VerifyOrQuit(memcmp(&readBuffer[offset], &writeBuffer[offset + length], kMaxSize - length - offset) == 0);
            message->Free();
        }
    }

    // Verify `InsertHeader()`

    for (uint16_t offset = 0; offset < kMaxSize; offset += kOffsetStep)
    {
        for (uint16_t length = 0; length <= kMaxSize; length += kLengthStep)
        {
            VerifyOrQuit((message = messagePool->Allocate(Message::kTypeIp6)) != nullptr);
            SuccessOrQuit(message->AppendBytes(writeBuffer, kMaxSize));

            SuccessOrQuit(message->InsertHeader(offset, length));

            VerifyOrQuit(message->GetLength() == kMaxSize + length);

            SuccessOrQuit(message->Read(0, readBuffer, offset));
            VerifyOrQuit(memcmp(&readBuffer[0], &writeBuffer[0], offset) == 0);

            SuccessOrQuit(message->Read(offset + length, readBuffer, kMaxSize - offset));
            VerifyOrQuit(memcmp(&readBuffer[0], &writeBuffer[offset], kMaxSize - offset) == 0);
            message->Free();
        }
    }

    testFreeInstance(instance);
}

void TestAppender(void)
{
    const uint8_t kData1[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t kData2[] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa};

    static constexpr uint16_t kMaxBufferSize = sizeof(kData1) * 2 + sizeof(kData2);

    Instance               *instance;
    Message                *message;
    uint8_t                 buffer[kMaxBufferSize];
    uint8_t                 zeroBuffer[kMaxBufferSize];
    Appender                bufAppender(buffer, sizeof(buffer));
    Data<kWithUint16Length> data;

    printf("TestAppender\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    memset(buffer, 0, sizeof(buffer));
    memset(zeroBuffer, 0, sizeof(zeroBuffer));

    // Test Buffer Appender
    VerifyOrQuit(bufAppender.GetType() == Appender::kBuffer);
    VerifyOrQuit(bufAppender.GetBufferStart() == buffer);
    VerifyOrQuit(bufAppender.GetAppendedLength() == 0);

    SuccessOrQuit(bufAppender.AppendBytes(kData1, sizeof(kData1)));
    DumpBuffer("Data1", buffer, sizeof(buffer));
    VerifyOrQuit(bufAppender.GetAppendedLength() == sizeof(kData1));
    VerifyOrQuit(bufAppender.GetBufferStart() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), zeroBuffer, sizeof(buffer) - sizeof(kData1)) == 0);

    SuccessOrQuit(bufAppender.AppendBytes(kData2, sizeof(kData2)));
    DumpBuffer("Data1+Data2", buffer, sizeof(buffer));
    VerifyOrQuit(bufAppender.GetAppendedLength() == sizeof(kData1) + sizeof(kData2));
    VerifyOrQuit(bufAppender.GetBufferStart() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1) + sizeof(kData2), zeroBuffer,
                        sizeof(buffer) - sizeof(kData1) - sizeof(kData2)) == 0);

    VerifyOrQuit(bufAppender.Append(kData2) == kErrorNoBufs);

    SuccessOrQuit(bufAppender.AppendBytes(kData1, sizeof(kData1)));
    DumpBuffer("Data1+Data2+Data1", buffer, sizeof(buffer));
    VerifyOrQuit(bufAppender.GetAppendedLength() == sizeof(kData1) + sizeof(kData2) + sizeof(kData1));
    VerifyOrQuit(bufAppender.GetBufferStart() == buffer);
    VerifyOrQuit(memcmp(buffer, kData1, sizeof(kData1)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1), kData2, sizeof(kData2)) == 0);
    VerifyOrQuit(memcmp(buffer + sizeof(kData1) + sizeof(kData2), kData1, sizeof(kData1)) == 0);

    VerifyOrQuit(bufAppender.Append<uint8_t>(0) == kErrorNoBufs);

    bufAppender.GetAsData(data);
    VerifyOrQuit(data.GetBytes() == buffer);
    VerifyOrQuit(data.GetLength() == sizeof(buffer));

    // Test Message Appender

    SuccessOrQuit(message->Append(kData2));
    VerifyOrQuit(message->Compare(0, kData2));

    {
        Appender msgAppender(*message);
        uint16_t offset = message->GetLength();

        VerifyOrQuit(msgAppender.GetType() == Appender::kMessage);

        SuccessOrQuit(msgAppender.AppendBytes(kData1, sizeof(kData1)));
        VerifyOrQuit(msgAppender.GetAppendedLength() == sizeof(kData1));

        VerifyOrQuit(message->GetLength() == sizeof(kData2) + sizeof(kData1));
        VerifyOrQuit(message->Compare(offset, kData1));

        SuccessOrQuit(msgAppender.AppendBytes(kData2, sizeof(kData2)));
        VerifyOrQuit(msgAppender.GetAppendedLength() == sizeof(kData1) + sizeof(kData2));
        VerifyOrQuit(message->Compare(offset, kData1));
        VerifyOrQuit(message->Compare(offset + sizeof(kData1), kData2));
    }

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestMessage();
    ot::TestAppender();
    printf("All tests passed\n");
    return 0;
}
