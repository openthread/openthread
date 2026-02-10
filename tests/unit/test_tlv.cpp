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

#include "test_platform.h"

#include <openthread/config.h>

#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "instance/instance.hpp"

#include "test_util.h"
#include "test_util.hpp"

namespace ot {

void TestTlv(void)
{
    static constexpr uint16_t kMaxBufferSize = 300;

    Instance     *instance = testInitInstance();
    Message      *message;
    Tlv           tlv;
    ExtendedTlv   extTlv;
    Tlv::Bookmark bookmark;
    uint16_t      offset;
    OffsetRange   offsetRange;
    uint16_t      length;
    uint16_t      prevLength;
    uint16_t      index;
    uint8_t       buffer[kMaxBufferSize];

    VerifyOrQuit(instance != nullptr);

    VerifyOrQuit((message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6)) != nullptr);
    VerifyOrQuit(message != nullptr);

    VerifyOrQuit(message->GetOffset() == 0);
    VerifyOrQuit(message->GetLength() == 0);

    VerifyOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 1, offsetRange) == kErrorNotFound);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, 0, buffer, 1) == kErrorParse);

    // Add an empty TLV with type 1 and check that we can find it

    offset = message->GetLength();

    tlv.SetType(1);
    tlv.SetLength(0);
    SuccessOrQuit(message->Append(tlv));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 1, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == sizeof(Tlv));
    VerifyOrQuit(offsetRange.GetLength() == 0);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0));
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    // Add an empty extended TLV (type 2), and check that we can find it.

    offset = message->GetLength();

    extTlv.SetType(2);
    extTlv.SetLength(0);
    SuccessOrQuit(message->Append(extTlv));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 2, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(offsetRange.GetLength() == 0);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0));
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    // Add a TLV with type 3 with one byte value and check if we can find it.

    offset = message->GetLength();

    tlv.SetType(3);
    tlv.SetLength(1);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(message->Append<uint8_t>(0xff));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 3, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(Tlv));
    VerifyOrQuit(offsetRange.GetLength() == 1);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1));
    VerifyOrQuit(buffer[0] == 0x0ff);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 2) == kErrorParse);

    // Add an extended TLV with type 4 with two byte value and check if we can find it.

    offset = message->GetLength();

    extTlv.SetType(4);
    extTlv.SetLength(2);
    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append<uint8_t>(0x12));
    SuccessOrQuit(message->Append<uint8_t>(0x34));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 4, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(offsetRange.GetLength() == 2);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1));
    VerifyOrQuit(buffer[0] == 0x12);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 2));
    VerifyOrQuit(buffer[0] == 0x12);
    VerifyOrQuit(buffer[1] == 0x34);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 3) == kErrorParse);

    // Add a TLV with missing value.

    offset = message->GetLength();

    tlv.SetType(5);
    tlv.SetLength(1);
    SuccessOrQuit(message->Append(tlv));

    VerifyOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 5, offsetRange) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0) == kErrorParse);

    // Add the missing value.
    SuccessOrQuit(message->Append<uint8_t>(0xaa));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 5, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(Tlv));
    VerifyOrQuit(offsetRange.GetLength() == 1);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1));
    VerifyOrQuit(buffer[0] == 0xaa);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 2) == kErrorParse);

    // Add an extended TLV with missing value.

    offset = message->GetLength();

    extTlv.SetType(6);
    extTlv.SetLength(2);
    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append<uint8_t>(0xbb));

    VerifyOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 6, offsetRange) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    SuccessOrQuit(message->Append<uint8_t>(0xcc));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 6, offsetRange) != kErrorNone);
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(offsetRange.GetLength() == 2);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 2));
    VerifyOrQuit(buffer[0] == 0xbb);
    VerifyOrQuit(buffer[1] == 0xcc);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 3) == kErrorParse);

    // Add an extended TLV with overflow length.

    offset = message->GetLength();

    extTlv.SetType(7);
    extTlv.SetLength(0xffff);
    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append<uint8_t>(0x11));

    VerifyOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 7, offsetRange) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    //- - - - - - - - - - - - - - - - - - - - - -
    // Validate `StartTlv()`, `AdjustTlv()`, `EndTlv()`

    SuccessOrQuit(message->SetLength(0));
    offset = 0;

    // Build a TLV with length 3

    SuccessOrQuit(Tlv::StartTlv(*message, /* aType */ 1, bookmark));
    SuccessOrQuit(message->Append<uint8_t>(0xab));
    SuccessOrQuit(message->Append<uint8_t>(0xcd));
    SuccessOrQuit(message->Append<uint8_t>(0xef));
    SuccessOrQuit(Tlv::EndTlv(*message, bookmark));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 1, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(Tlv));
    VerifyOrQuit(offsetRange.GetLength() == 3);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 3));
    VerifyOrQuit(buffer[0] == 0xab);
    VerifyOrQuit(buffer[1] == 0xcd);
    VerifyOrQuit(buffer[2] == 0xef);

    offset = offsetRange.GetEndOffset();
    VerifyOrQuit(offset == message->GetLength());

    for (index = 0; index < kMaxBufferSize; index++)
    {
        buffer[index] = static_cast<uint8_t>(index);
    }

    // Build a TLV with length 254 (max for a regular TLV).

    SuccessOrQuit(Tlv::StartTlv(*message, /* aType */ 2, bookmark));
    SuccessOrQuit(message->AppendBytes(buffer, Tlv::kBaseTlvMaxLength));
    SuccessOrQuit(Tlv::EndTlv(*message, bookmark));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 2, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(Tlv));
    VerifyOrQuit(offsetRange.GetLength() == Tlv::kBaseTlvMaxLength);
    VerifyOrQuit(message->CompareBytes(offsetRange, buffer));

    offset = offsetRange.GetEndOffset();
    VerifyOrQuit(offset == message->GetLength());

    // Build a TLV with length 255 (ensure it is written as Extended TLV).

    SuccessOrQuit(Tlv::StartTlv(*message, /* aType */ 3, bookmark));
    SuccessOrQuit(message->AppendBytes(buffer, Tlv::kBaseTlvMaxLength + 1));
    SuccessOrQuit(Tlv::EndTlv(*message, bookmark));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 3, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(offsetRange.GetLength() == Tlv::kBaseTlvMaxLength + 1);
    VerifyOrQuit(message->CompareBytes(offsetRange, buffer));

    offset = offsetRange.GetEndOffset();
    VerifyOrQuit(offset == message->GetLength());

    // Validate that `AdjustTlv()` copies the bytes only when we reach the
    // TLV length limit.

    SuccessOrQuit(Tlv::StartTlv(*message, /* aType */ 4, bookmark));

    for (index = 0; index < Tlv::kBaseTlvMaxLength; index++)
    {
        SuccessOrQuit(message->Append<uint8_t>(buffer[index]));

        prevLength = message->GetLength();
        SuccessOrQuit(Tlv::AdjustTlv(*message, bookmark));
        VerifyOrQuit(prevLength == message->GetLength());
    }

    SuccessOrQuit(message->Append<uint8_t>(buffer[index]));
    index++;

    prevLength = message->GetLength();
    SuccessOrQuit(Tlv::AdjustTlv(*message, bookmark));
    VerifyOrQuit(message->GetLength() == prevLength + sizeof(uint16_t));

    for (; index < kMaxBufferSize; index++)
    {
        SuccessOrQuit(message->Append<uint8_t>(buffer[index]));

        prevLength = message->GetLength();
        SuccessOrQuit(Tlv::AdjustTlv(*message, bookmark));
        VerifyOrQuit(prevLength == message->GetLength());
    }

    SuccessOrQuit(Tlv::EndTlv(*message, bookmark));

    SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, /* aType */ 4, offsetRange));
    VerifyOrQuit(offsetRange.GetOffset() == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(offsetRange.GetLength() == kMaxBufferSize);
    VerifyOrQuit(message->CompareBytes(offsetRange, buffer));

    offset = offsetRange.GetEndOffset();
    VerifyOrQuit(offset == message->GetLength());

    message->Free();

    testFreeInstance(instance);
}

void TestTlvInfo(void)
{
    Instance   *instance;
    Message    *message;
    uint16_t    offset;
    uint16_t    len;
    Tlv         tlv;
    ExtendedTlv extTlv;
    Tlv::Info   info;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);
    message = instance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    // Append TLV 1: Standard TLV with 1-byte value.
    tlv.SetType(1);
    tlv.SetLength(1);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(message->Append<uint8_t>(0xaa));

    // Append TLV 2: Extended TLV with 2-byte value.
    extTlv.SetType(2);
    extTlv.SetLength(2);
    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append<uint16_t>(0xcafe));

    // Append TLV 3: Standard empty TLV.
    tlv.SetType(3);
    tlv.SetLength(0);
    SuccessOrQuit(message->Append(tlv));

    // Append TLV 4: Extended empty TLV.
    extTlv.SetType(4);
    extTlv.SetLength(0);
    SuccessOrQuit(message->Append(extTlv));

    // Append TLV 5: Malformed standard TLV (claims length 2, but has only 1).
    tlv.SetType(5);
    tlv.SetLength(2);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(message->Append<uint8_t>(0x12));

    for (uint8_t testIter = 0; testIter <= 1; testIter++)
    {
        offset = 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // TLV 1 (standard, len=1)

        len = 1;

        if (testIter == 0)
        {
            SuccessOrQuit(info.ParseFrom(*message, offset));
        }
        else
        {
            SuccessOrQuit(info.FindIn(*message, 1));
        }

        VerifyOrQuit(info.GetType() == 1);
        VerifyOrQuit(info.GetLength() == len);
        VerifyOrQuit(!info.IsExtended());
        VerifyOrQuit(info.GetSize() == sizeof(Tlv) + len);
        VerifyOrQuit(info.GetTlvOffset() == offset);
        VerifyOrQuit(info.GetValueOffset() == offset + sizeof(Tlv));
        VerifyOrQuit(info.GetTlvOffsetRange().GetOffset() == offset);
        VerifyOrQuit(info.GetTlvOffsetRange().GetLength() == sizeof(Tlv) + len);
        VerifyOrQuit(info.GetValueOffsetRange().GetOffset() == offset + sizeof(Tlv));
        VerifyOrQuit(info.GetValueOffsetRange().GetLength() == len);

        offset = info.GetTlvOffsetRange().GetEndOffset();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // TLV 2 (extended, len=2)

        len = 2;

        if (testIter == 0)
        {
            SuccessOrQuit(info.ParseFrom(*message, offset));
        }
        else
        {
            SuccessOrQuit(info.FindIn(*message, 2));
        }

        VerifyOrQuit(info.GetType() == 2);
        VerifyOrQuit(info.GetLength() == len);
        VerifyOrQuit(info.IsExtended());
        VerifyOrQuit(info.GetSize() == sizeof(ExtendedTlv) + len);
        VerifyOrQuit(info.GetTlvOffset() == offset);
        VerifyOrQuit(info.GetValueOffset() == offset + sizeof(ExtendedTlv));
        VerifyOrQuit(info.GetTlvOffsetRange().GetOffset() == offset);
        VerifyOrQuit(info.GetTlvOffsetRange().GetLength() == sizeof(ExtendedTlv) + len);
        VerifyOrQuit(info.GetValueOffsetRange().GetOffset() == offset + sizeof(ExtendedTlv));
        VerifyOrQuit(info.GetValueOffsetRange().GetLength() == len);

        offset = info.GetTlvOffsetRange().GetEndOffset();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // TLV 3 (standard, empty)

        len = 0;

        if (testIter == 0)
        {
            SuccessOrQuit(info.ParseFrom(*message, offset));
        }
        else
        {
            SuccessOrQuit(info.FindIn(*message, 3));
        }

        VerifyOrQuit(info.GetType() == 3);
        VerifyOrQuit(info.GetLength() == len);
        VerifyOrQuit(!info.IsExtended());
        VerifyOrQuit(info.GetSize() == sizeof(Tlv) + len);
        VerifyOrQuit(info.GetTlvOffset() == offset);
        VerifyOrQuit(info.GetValueOffset() == offset + sizeof(Tlv));
        VerifyOrQuit(info.GetTlvOffsetRange().GetOffset() == offset);
        VerifyOrQuit(info.GetTlvOffsetRange().GetLength() == sizeof(Tlv) + len);
        VerifyOrQuit(info.GetValueOffsetRange().GetOffset() == offset + sizeof(Tlv));
        VerifyOrQuit(info.GetValueOffsetRange().GetLength() == len);

        offset = info.GetTlvOffsetRange().GetEndOffset();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // TLV 4 (extended, empty)

        len = 0;

        if (testIter == 0)
        {
            SuccessOrQuit(info.ParseFrom(*message, offset));
        }
        else
        {
            SuccessOrQuit(info.FindIn(*message, 4));
        }

        VerifyOrQuit(info.GetType() == 4);
        VerifyOrQuit(info.GetLength() == len);
        VerifyOrQuit(info.IsExtended());
        VerifyOrQuit(info.GetSize() == sizeof(ExtendedTlv) + len);
        VerifyOrQuit(info.GetTlvOffset() == offset);
        VerifyOrQuit(info.GetValueOffset() == offset + sizeof(ExtendedTlv));
        VerifyOrQuit(info.GetTlvOffsetRange().GetOffset() == offset);
        VerifyOrQuit(info.GetTlvOffsetRange().GetLength() == sizeof(ExtendedTlv) + len);
        VerifyOrQuit(info.GetValueOffsetRange().GetOffset() == offset + sizeof(ExtendedTlv));
        VerifyOrQuit(info.GetValueOffsetRange().GetLength() == len);

        offset = info.GetTlvOffsetRange().GetEndOffset();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Test TLV 5 (malformed)

        if (testIter == 0)
        {
            VerifyOrQuit(info.ParseFrom(*message, offset) == kErrorParse);
        }
        else
        {
            VerifyOrQuit(info.FindIn(*message, 5) != kErrorNone);
        }
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestTlv();
    ot::TestTlvInfo();
    printf("All tests passed\n");
    return 0;
}
