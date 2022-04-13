/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "test_platform.h"

#include <string.h>

#include <openthread/config.h>

#include "test_util.hpp"
#include "common/code_utils.hpp"
#include "common/heap_data.hpp"
#include "common/heap_string.hpp"

namespace ot {

void PrintString(const char *aName, const Heap::String &aString)
{
    if (aString.IsNull())
    {
        printf("%s = (null)\n", aName);
    }
    else
    {
        printf("%s = [%zu] \"%s\"\n", aName, strlen(aString.AsCString()), aString.AsCString());
    }
}

void VerifyString(const char *aName, const Heap::String &aString, const char *aExpectedString)
{
    PrintString(aName, aString);

    if (aExpectedString == nullptr)
    {
        VerifyOrQuit(aString.IsNull());
        VerifyOrQuit(aString.AsCString() == nullptr);
        VerifyOrQuit(aString != "something");
    }
    else
    {
        VerifyOrQuit(!aString.IsNull());
        VerifyOrQuit(aString.AsCString() != nullptr);
        VerifyOrQuit(strcmp(aString.AsCString(), aExpectedString) == 0, "String content is incorrect");
        VerifyOrQuit(aString != nullptr);
    }

    VerifyOrQuit(aString == aExpectedString);
}

// Function returning a `Heap::String` by value.
Heap::String GetName(void)
{
    Heap::String name;

    SuccessOrQuit(name.Set("name"));

    return name;
}

void TestHeapString(void)
{
    Heap::String str1;
    Heap::String str2;
    const char * oldBuffer;

    printf("====================================================================================\n");
    printf("TestHeapString\n\n");

    printf("------------------------------------------------------------------------------------\n");
    printf("After constructor\n\n");
    VerifyString("str1", str1, nullptr);

    printf("------------------------------------------------------------------------------------\n");
    printf("Set(const char *aCstring)\n\n");
    SuccessOrQuit(str1.Set("hello"));
    VerifyString("str1", str1, "hello");
    oldBuffer = str1.AsCString();

    SuccessOrQuit(str1.Set("0123456789"));
    VerifyString("str1", str1, "0123456789");
    printf("\tDid reuse its old buffer: %s\n", str1.AsCString() == oldBuffer ? "yes" : "no");
    oldBuffer = str1.AsCString();

    SuccessOrQuit(str1.Set("9876543210"));
    VerifyString("str1", str1, "9876543210");
    printf("\tDid reuse its old buffer (same length): %s\n", str1.AsCString() == oldBuffer ? "yes" : "no");

    printf("------------------------------------------------------------------------------------\n");
    printf("Set(const Heap::String &)\n\n");
    SuccessOrQuit(str2.Set(str1));
    VerifyString("str2", str2, str1.AsCString());

    SuccessOrQuit(str1.Set(nullptr));
    VerifyString("str1", str1, nullptr);

    SuccessOrQuit(str2.Set(str1));
    VerifyString("str2", str2, nullptr);

    printf("------------------------------------------------------------------------------------\n");
    printf("Free()\n\n");
    str1.Free();
    VerifyString("str1", str1, nullptr);

    SuccessOrQuit(str1.Set("hello again"));
    VerifyString("str1", str1, "hello again");

    str1.Free();
    VerifyString("str1", str1, nullptr);

    printf("------------------------------------------------------------------------------------\n");
    printf("Set() move semantics\n\n");
    SuccessOrQuit(str1.Set("old name"));
    PrintString("str1", str1);
    SuccessOrQuit(str1.Set(GetName()), "Set() with move semantics failed");
    VerifyString("str1", str1, "name");

    printf("------------------------------------------------------------------------------------\n");
    printf("operator==() with two null string\n\n");
    str1.Free();
    str2.Free();
    VerifyString("str1", str1, nullptr);
    VerifyString("str2", str2, nullptr);
    VerifyOrQuit(str1 == str2, "operator==() failed with two null strings");

    printf("\n -- PASS\n");
}

void PrintData(const Heap::Data &aData)
{
    DumpBuffer("data", aData.GetBytes(), aData.GetLength());
}

static const uint8_t kTestValue = 0x77;

// Function returning a `Heap::Data` by value.
Heap::Data GetData(void)
{
    Heap::Data data;

    SuccessOrQuit(data.SetFrom(&kTestValue, sizeof(kTestValue)));

    return data;
}

void VerifyData(const Heap::Data &aData, const uint8_t *aBytes, uint16_t aLength)
{
    static constexpr uint16_t kMaxLength = 100;
    uint8_t                   buffer[kMaxLength];

    PrintData(aData);

    if (aLength == 0)
    {
        VerifyOrQuit(aData.IsNull());
        VerifyOrQuit(aData.GetBytes() == nullptr);
        VerifyOrQuit(aData.GetLength() == 0);
    }
    else
    {
        VerifyOrQuit(!aData.IsNull());
        VerifyOrQuit(aData.GetBytes() != nullptr);
        VerifyOrQuit(aData.GetLength() == aLength);
        VerifyOrQuit(memcmp(aData.GetBytes(), aBytes, aLength) == 0, "Data content is incorrect");

        aData.CopyBytesTo(buffer);
        VerifyOrQuit(memcmp(buffer, aBytes, aLength) == 0, "CopyBytesTo() failed");
    }
}

template <uint16_t kLength> void VerifyData(const Heap::Data &aData, const uint8_t (&aArray)[kLength])
{
    VerifyData(aData, &aArray[0], kLength);
}

void TestHeapData(void)
{
    Instance *     instance;
    MessagePool *  messagePool;
    Message *      message;
    Heap::Data     data;
    uint16_t       offset;
    const uint8_t *oldBuffer;

    static const uint8_t kData1[] = {10, 20, 3, 15, 100, 0, 60, 16};
    static const uint8_t kData2[] = "OpenThread HeapData";
    static const uint8_t kData3[] = {0xaa, 0xbb, 0xcc};
    static const uint8_t kData4[] = {0x11, 0x22, 0x33};

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->Allocate(Message::kTypeIp6)) != nullptr);

    message->SetOffset(0);

    printf("\n\n====================================================================================\n");
    printf("TestHeapData\n\n");

    printf("------------------------------------------------------------------------------------\n");
    printf("After constructor\n");
    VerifyData(data, nullptr, 0);

    printf("------------------------------------------------------------------------------------\n");
    printf("SetFrom(aBuffer, aLength)\n");

    SuccessOrQuit(data.SetFrom(kData1, sizeof(kData1)));
    VerifyData(data, kData1);

    SuccessOrQuit(data.SetFrom(kData2, sizeof(kData2)));
    VerifyData(data, kData2);

    SuccessOrQuit(data.SetFrom(kData3, sizeof(kData3)));
    VerifyData(data, kData3);
    oldBuffer = data.GetBytes();

    SuccessOrQuit(data.SetFrom(kData4, sizeof(kData4)));
    VerifyData(data, kData4);
    VerifyOrQuit(oldBuffer == data.GetBytes(), "did not reuse old buffer on same data length");

    SuccessOrQuit(data.SetFrom(kData4, 0));
    VerifyData(data, nullptr, 0);

    printf("------------------------------------------------------------------------------------\n");
    printf("SetFrom(aMessage)\n");

    SuccessOrQuit(message->Append(kData2));
    SuccessOrQuit(data.SetFrom(*message));
    VerifyData(data, kData2);

    SuccessOrQuit(message->Append(kData3));
    SuccessOrQuit(data.SetFrom(*message));
    PrintData(data);
    VerifyOrQuit(data.GetLength() == message->GetLength());

    message->SetOffset(sizeof(kData2));
    SuccessOrQuit(data.SetFrom(*message));
    VerifyData(data, kData3);

    SuccessOrQuit(message->Append(kData4));

    offset = 0;
    SuccessOrQuit(data.SetFrom(*message, offset, sizeof(kData2)));
    VerifyData(data, kData2);

    offset = sizeof(kData2);
    SuccessOrQuit(data.SetFrom(*message, offset, sizeof(kData3)));
    VerifyData(data, kData3);

    offset += sizeof(kData3);
    SuccessOrQuit(data.SetFrom(*message, offset, sizeof(kData4)));
    VerifyData(data, kData4);

    VerifyOrQuit(data.SetFrom(*message, offset, sizeof(kData4) + 1) == kErrorParse);
    VerifyOrQuit(data.SetFrom(*message, 0, message->GetLength() + 1) == kErrorParse);
    VerifyOrQuit(data.SetFrom(*message, 1, message->GetLength()) == kErrorParse);

    printf("------------------------------------------------------------------------------------\n");
    printf("Free()\n");

    data.Free();
    VerifyData(data, nullptr, 0);

    data.Free();
    VerifyData(data, nullptr, 0);

    printf("------------------------------------------------------------------------------------\n");
    printf("CopyBytesTo(aMessage)\n");

    SuccessOrQuit(message->SetLength(0));

    SuccessOrQuit(data.CopyBytesTo(*message));
    VerifyOrQuit(message->GetLength() == 0, "CopyBytesTo() failed");

    SuccessOrQuit(data.SetFrom(kData1, sizeof(kData1)));
    VerifyData(data, kData1);
    SuccessOrQuit(data.CopyBytesTo(*message));
    VerifyOrQuit(message->GetLength() == data.GetLength(), "CopyBytesTo() failed");
    VerifyOrQuit(message->Compare(0, kData1), "CopyBytesTo() failed");

    printf("------------------------------------------------------------------------------------\n");
    printf("SetFrom() move semantics\n\n");
    data.SetFrom(GetData());
    VerifyData(data, &kTestValue, sizeof(kTestValue));

    printf("\n -- PASS\n");
}

} // namespace ot

int main(void)
{
    ot::TestHeapString();
    ot::TestHeapData();
    printf("\nAll tests passed.\n");
    return 0;
}
