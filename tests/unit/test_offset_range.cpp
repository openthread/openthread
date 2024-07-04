/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "common/message.hpp"
#include "common/offset_range.hpp"
#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

void TestOffsetRange(void)
{
    Instance   *instance;
    Message    *message;
    OffsetRange offsetRange;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    // Empty `OffsetRange`

    offsetRange.Clear();
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 0);
    VerifyOrQuit(offsetRange.IsEmpty());

    offsetRange.ShrinkLength(10);
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 0);
    VerifyOrQuit(offsetRange.IsEmpty());

    offsetRange.AdvanceOffset(20);
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 0);
    VerifyOrQuit(offsetRange.IsEmpty());

    // Empty `OffsetRange` with non-zero starting offset

    offsetRange.Init(100, 0);
    VerifyOrQuit(offsetRange.GetOffset() == 100);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 100);
    VerifyOrQuit(offsetRange.IsEmpty());

    offsetRange.ShrinkLength(10);
    VerifyOrQuit(offsetRange.GetOffset() == 100);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 100);
    VerifyOrQuit(offsetRange.IsEmpty());

    offsetRange.AdvanceOffset(20);
    VerifyOrQuit(offsetRange.GetOffset() == 100);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 100);
    VerifyOrQuit(offsetRange.IsEmpty());

    // Non-empty `OffsetRange`

    offsetRange.Init(200, 10);
    VerifyOrQuit(offsetRange.GetOffset() == 200);
    VerifyOrQuit(offsetRange.GetLength() == 10);
    VerifyOrQuit(offsetRange.GetEndOffset() == 210);
    VerifyOrQuit(!offsetRange.IsEmpty());
    VerifyOrQuit(offsetRange.Contains(10));
    VerifyOrQuit(!offsetRange.Contains(11));

    offsetRange.ShrinkLength(10);
    VerifyOrQuit(offsetRange.GetOffset() == 200);
    VerifyOrQuit(offsetRange.GetLength() == 10);
    VerifyOrQuit(offsetRange.GetEndOffset() == 210);

    offsetRange.ShrinkLength(20);
    VerifyOrQuit(offsetRange.GetOffset() == 200);
    VerifyOrQuit(offsetRange.GetLength() == 10);
    VerifyOrQuit(offsetRange.GetEndOffset() == 210);

    offsetRange.ShrinkLength(5);
    VerifyOrQuit(offsetRange.GetOffset() == 200);
    VerifyOrQuit(offsetRange.GetLength() == 5);
    VerifyOrQuit(offsetRange.GetEndOffset() == 205);
    VerifyOrQuit(!offsetRange.Contains(10));
    VerifyOrQuit(!offsetRange.Contains(6));
    VerifyOrQuit(offsetRange.Contains(5));

    offsetRange.AdvanceOffset(4);
    VerifyOrQuit(offsetRange.GetOffset() == 204);
    VerifyOrQuit(offsetRange.GetLength() == 1);
    VerifyOrQuit(offsetRange.GetEndOffset() == 205);
    VerifyOrQuit(!offsetRange.IsEmpty());
    VerifyOrQuit(offsetRange.Contains(1));
    VerifyOrQuit(!offsetRange.Contains(2));

    offsetRange.AdvanceOffset(1);
    VerifyOrQuit(offsetRange.GetOffset() == 205);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 205);
    VerifyOrQuit(offsetRange.IsEmpty());

    // `InitFromRange()`

    offsetRange.InitFromRange(300, 400);
    VerifyOrQuit(offsetRange.GetOffset() == 300);
    VerifyOrQuit(offsetRange.GetLength() == 100);
    VerifyOrQuit(offsetRange.GetEndOffset() == 400);
    VerifyOrQuit(!offsetRange.IsEmpty());
    VerifyOrQuit(offsetRange.Contains(100));
    VerifyOrQuit(!offsetRange.Contains(101));

    offsetRange.AdvanceOffset(101);
    VerifyOrQuit(offsetRange.GetOffset() == 400);
    VerifyOrQuit(offsetRange.GetLength() == 0);
    VerifyOrQuit(offsetRange.GetEndOffset() == 400);
    VerifyOrQuit(offsetRange.IsEmpty());

    // Init from a `Message` from offset or full length

    message = instance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->SetLength(120));
    VerifyOrQuit(message->GetOffset() == 0);

    offsetRange.InitFromMessageOffsetToEnd(*message);
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 120);
    VerifyOrQuit(offsetRange.GetEndOffset() == 120);

    offsetRange.InitFromMessageFullLength(*message);
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 120);
    VerifyOrQuit(offsetRange.GetEndOffset() == 120);

    message->SetOffset(40);
    VerifyOrQuit(message->GetOffset() == 40);

    offsetRange.InitFromMessageOffsetToEnd(*message);
    VerifyOrQuit(offsetRange.GetOffset() == 40);
    VerifyOrQuit(offsetRange.GetLength() == 80);
    VerifyOrQuit(offsetRange.GetEndOffset() == 120);

    offsetRange.InitFromMessageFullLength(*message);
    VerifyOrQuit(offsetRange.GetOffset() == 0);
    VerifyOrQuit(offsetRange.GetLength() == 120);
    VerifyOrQuit(offsetRange.GetEndOffset() == 120);

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestOffsetRange();
    printf("All tests passed\n");
    return 0;
}
