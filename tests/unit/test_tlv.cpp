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

namespace ot {

void TestTlv(void)
{
    Instance   *instance = testInitInstance();
    Message    *message;
    Tlv         tlv;
    ExtendedTlv extTlv;
    uint16_t    offset;
    uint16_t    valueOffset;
    uint16_t    length;
    uint8_t     buffer[4];

    VerifyOrQuit(instance != nullptr);

    VerifyOrQuit((message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6)) != nullptr);
    VerifyOrQuit(message != nullptr);

    VerifyOrQuit(message->GetOffset() == 0);
    VerifyOrQuit(message->GetLength() == 0);

    VerifyOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 1, valueOffset, length) == kErrorNotFound);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, 0, buffer, 1) == kErrorParse);

    // Add an empty TLV with type 1 and check that we can find it

    offset = message->GetLength();

    tlv.SetType(1);
    tlv.SetLength(0);
    SuccessOrQuit(message->Append(tlv));

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 1, valueOffset, length));
    VerifyOrQuit(valueOffset == sizeof(Tlv));
    VerifyOrQuit(length == 0);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0));
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    // Add an empty extended TLV (type 2), and check that we can find it.

    offset = message->GetLength();

    extTlv.SetType(2);
    extTlv.SetLength(0);
    SuccessOrQuit(message->Append(extTlv));

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 2, valueOffset, length));
    VerifyOrQuit(valueOffset == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(length == 0);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0));
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    // Add a TLV with type 3 with one byte value and check if we can find it.

    offset = message->GetLength();

    tlv.SetType(3);
    tlv.SetLength(1);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(message->Append<uint8_t>(0xff));

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 3, valueOffset, length));
    VerifyOrQuit(valueOffset == offset + sizeof(Tlv));
    VerifyOrQuit(length == 1);
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

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 4, valueOffset, length));
    VerifyOrQuit(valueOffset == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(length == 2);
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

    VerifyOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 5, valueOffset, length) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 0) == kErrorParse);

    // Add the missing value.
    SuccessOrQuit(message->Append<uint8_t>(0xaa));

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 5, valueOffset, length));
    VerifyOrQuit(valueOffset == offset + sizeof(Tlv));
    VerifyOrQuit(length == 1);
    SuccessOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1));
    VerifyOrQuit(buffer[0] == 0xaa);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 2) == kErrorParse);

    // Add an extended TLV with missing value.

    offset = message->GetLength();

    extTlv.SetType(6);
    extTlv.SetLength(2);
    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append<uint8_t>(0xbb));

    VerifyOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 6, valueOffset, length) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    SuccessOrQuit(message->Append<uint8_t>(0xcc));

    SuccessOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 6, valueOffset, length) != kErrorNone);
    VerifyOrQuit(valueOffset == offset + sizeof(ExtendedTlv));
    VerifyOrQuit(length == 2);
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

    VerifyOrQuit(Tlv::FindTlvValueOffset(*message, /* aType */ 7, valueOffset, length) != kErrorNone);
    VerifyOrQuit(Tlv::ReadTlvValue(*message, offset, buffer, 1) == kErrorParse);

    message->Free();

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestTlv();
    printf("All tests passed\n");
    return 0;
}
