/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "common/ltvs.hpp"

#include "test_util.h"
#include "test_util.hpp"

namespace ot {

// LTV type constants used throughout the tests.
static constexpr uint8_t kTypeA = 0x01;
static constexpr uint8_t kTypeB = 0x02;
static constexpr uint8_t kTypeC = 0x03;

// A fixed-size value type for SimpleLtvInfo tests.
struct SimpleValue
{
    uint8_t  mField1;
    uint16_t mField2;
    uint8_t  mField3;
};

using SimpleValueLtv = SimpleLtvInfo<kTypeC, SimpleValue>;

// ---------------------------------------------------------------------------
// TestLtvAppendAndFind
//
// Appends a single LTV with a known value, then locates it by type and verifies
// the Length, Type, and Value bytes are correct.
// ---------------------------------------------------------------------------
void TestLtvAppendAndFind(void)
{
    static constexpr uint8_t kValue[]    = {0xAA, 0xBB, 0xCC};
    static constexpr uint8_t kBufSize    = Ltv::kHeaderSize + sizeof(kValue) + 4;
    static constexpr uint8_t kAbsentType = 0xFF;

    uint8_t      buffer[kBufSize];
    FrameBuilder fb;
    const Ltv   *ltv;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::Append(fb, kTypeA, kValue, sizeof(kValue)));

    VerifyOrQuit(fb.GetLength() == Ltv::kHeaderSize + sizeof(kValue));

    // Verify raw wire bytes: [Length][Type][Value...]
    VerifyOrQuit(buffer[0] == sizeof(kValue));
    VerifyOrQuit(buffer[1] == kTypeA);
    VerifyOrQuit(memcmp(buffer + Ltv::kHeaderSize, kValue, sizeof(kValue)) == 0);

    // FindLtv by matching type.
    ltv = Ltv::FindLtv(buffer, fb.GetLength(), kTypeA);
    VerifyOrQuit(ltv != nullptr);
    VerifyOrQuit(ltv->GetType() == kTypeA);
    VerifyOrQuit(ltv->GetLength() == sizeof(kValue));
    VerifyOrQuit(memcmp(ltv->GetValue(), kValue, sizeof(kValue)) == 0);

    // FindLtv returns nullptr for a type not present.
    VerifyOrQuit(Ltv::FindLtv(buffer, fb.GetLength(), kAbsentType) == nullptr);

    // FindLtv on an empty buffer returns nullptr.
    VerifyOrQuit(Ltv::FindLtv(buffer, 0, kTypeA) == nullptr);
}

// ---------------------------------------------------------------------------
// TestLtvEmptyValue
//
// An LTV with Length=0 carries no value bytes; this is the teardown pattern
// (e.g. SCA LTV with L=0 signals link teardown).
// ---------------------------------------------------------------------------
void TestLtvEmptyValue(void)
{
    static constexpr uint8_t kBufSize = Ltv::kHeaderSize + 4;

    uint8_t      buffer[kBufSize];
    FrameBuilder fb;
    const Ltv   *ltv;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::Append(fb, kTypeB, nullptr, 0));

    VerifyOrQuit(fb.GetLength() == Ltv::kHeaderSize);
    VerifyOrQuit(buffer[0] == 0);      // Length = 0
    VerifyOrQuit(buffer[1] == kTypeB); // Type

    ltv = Ltv::FindLtv(buffer, fb.GetLength(), kTypeB);
    VerifyOrQuit(ltv != nullptr);
    VerifyOrQuit(ltv->GetLength() == 0);
    VerifyOrQuit(ltv->GetTotalSize() == Ltv::kHeaderSize);
}

// ---------------------------------------------------------------------------
// TestLtvIterator
//
// Appends three LTVs of different types, then uses Ltv::Iterator to walk the
// sequence and verify each entry is visited exactly once in order.
// ---------------------------------------------------------------------------
void TestLtvIterator(void)
{
    static constexpr uint8_t kValA[]  = {0x01};
    static constexpr uint8_t kValB[]  = {0x02, 0x03};
    static constexpr uint8_t kValC[]  = {0x04, 0x05, 0x06};
    static constexpr uint8_t kBufSize = 3 * Ltv::kHeaderSize + sizeof(kValA) + sizeof(kValB) + sizeof(kValC);

    uint8_t       buffer[kBufSize];
    FrameBuilder  fb;
    Ltv::Iterator iter;
    uint8_t       count = 0;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::Append(fb, kTypeA, kValA, sizeof(kValA)));
    SuccessOrQuit(Ltv::Append(fb, kTypeB, kValB, sizeof(kValB)));
    SuccessOrQuit(Ltv::Append(fb, kTypeC, kValC, sizeof(kValC)));

    iter.Init(buffer, fb.GetLength());

    // First LTV: type A, 1-byte value.
    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.GetLtv().GetType() == kTypeA);
    VerifyOrQuit(iter.GetLtv().GetLength() == sizeof(kValA));
    VerifyOrQuit(memcmp(iter.GetLtv().GetValue(), kValA, sizeof(kValA)) == 0);
    SuccessOrQuit(iter.Advance());
    count++;

    // Second LTV: type B, 2-byte value.
    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.GetLtv().GetType() == kTypeB);
    VerifyOrQuit(iter.GetLtv().GetLength() == sizeof(kValB));
    VerifyOrQuit(memcmp(iter.GetLtv().GetValue(), kValB, sizeof(kValB)) == 0);
    SuccessOrQuit(iter.Advance());
    count++;

    // Third LTV: type C, 3-byte value.
    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.GetLtv().GetType() == kTypeC);
    VerifyOrQuit(iter.GetLtv().GetLength() == sizeof(kValC));
    VerifyOrQuit(memcmp(iter.GetLtv().GetValue(), kValC, sizeof(kValC)) == 0);
    SuccessOrQuit(iter.Advance());
    count++;

    // Iterator is exhausted.
    VerifyOrQuit(iter.IsDone());
    VerifyOrQuit(count == 3);

    // Advance on an exhausted iterator is a no-op.
    SuccessOrQuit(iter.Advance());
    VerifyOrQuit(iter.IsDone());
}

// ---------------------------------------------------------------------------
// TestLtvBufferBoundary
//
// Verifies that Append returns kErrorNoBufs when the buffer cannot accommodate
// another full LTV.
// ---------------------------------------------------------------------------
void TestLtvBufferBoundary(void)
{
    static constexpr uint8_t kValue[] = {0x10, 0x20, 0x30};

    // Buffer exactly fits one LTV.
    static constexpr uint8_t kExactSize = Ltv::kHeaderSize + sizeof(kValue);

    uint8_t      buffer[kExactSize];
    FrameBuilder fb;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::Append(fb, kTypeA, kValue, sizeof(kValue)));
    VerifyOrQuit(fb.GetLength() == kExactSize);

    // One more byte would overflow.
    VerifyOrQuit(Ltv::Append(fb, kTypeB, kValue, 1) == kErrorNoBufs);
}

// ---------------------------------------------------------------------------
// TestLtvTruncated
//
// An iterator over a truncated buffer — where the declared LTV value does not
// fit — returns kErrorParse from Advance().
// ---------------------------------------------------------------------------
void TestLtvTruncated(void)
{
    // Craft a buffer whose LTV header declares a 10-byte value but only 3 bytes follow.
    static constexpr uint8_t kDeclaredLen = 10;
    static constexpr uint8_t kActualBytes = 3;

    uint8_t buffer[Ltv::kHeaderSize + kActualBytes];
    buffer[0] = kDeclaredLen; // Length field claims 10 bytes.
    buffer[1] = kTypeA;
    buffer[2] = buffer[3] = buffer[4] = 0xFF; // Only 3 value bytes present.

    Ltv::Iterator iter;
    iter.Init(buffer, sizeof(buffer));

    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.Advance() == kErrorParse);

    // FindLtv on a truncated buffer returns nullptr.
    VerifyOrQuit(Ltv::FindLtv(buffer, sizeof(buffer), kTypeA) == nullptr);
}

// ---------------------------------------------------------------------------
// TestLtvOneByteBoundary
//
// A buffer with only one byte remaining (incomplete LTV header) is treated as
// done by IsDone() without triggering a parse error.
// ---------------------------------------------------------------------------
void TestLtvOneByteBoundary(void)
{
    uint8_t       oneByte = 0x00;
    Ltv::Iterator iter;

    iter.Init(&oneByte, 1);
    VerifyOrQuit(iter.IsDone());

    // Advance on a "done" iterator is a no-op.
    SuccessOrQuit(iter.Advance());
    VerifyOrQuit(iter.IsDone());
}

// ---------------------------------------------------------------------------
// TestLtvSimpleLtvInfo
//
// Exercises the SimpleLtvInfo<kType, ValueType> typed-append path, then reads
// the value back using FindLtv and casts the value pointer.
// ---------------------------------------------------------------------------
void TestLtvSimpleLtvInfo(void)
{
    static constexpr uint8_t kBufSize = Ltv::kHeaderSize + sizeof(SimpleValue) + 4;

    uint8_t      buffer[kBufSize];
    FrameBuilder fb;
    SimpleValue  original;
    const Ltv   *ltv;

    original.mField1 = 0xAB;
    original.mField2 = 0xCDEF;
    original.mField3 = 0x12;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit((Ltv::Append<SimpleValueLtv>(fb, original)));

    VerifyOrQuit(fb.GetLength() == Ltv::kHeaderSize + sizeof(SimpleValue));

    ltv = Ltv::FindLtv(buffer, fb.GetLength(), SimpleValueLtv::kType);
    VerifyOrQuit(ltv != nullptr);
    VerifyOrQuit(ltv->GetType() == kTypeC);
    VerifyOrQuit(ltv->GetLength() == sizeof(SimpleValue));

    // Verify value bytes round-trip correctly.
    SimpleValue readBack;
    memcpy(&readBack, ltv->GetValue(), sizeof(SimpleValue));
    VerifyOrQuit(readBack.mField1 == original.mField1);
    VerifyOrQuit(readBack.mField2 == original.mField2);
    VerifyOrQuit(readBack.mField3 == original.mField3);
}

// ---------------------------------------------------------------------------
// TestLtvFindSecond
//
// When two LTVs with different types are present, FindLtv returns the correct one.
// ---------------------------------------------------------------------------
void TestLtvFindSecond(void)
{
    static constexpr uint8_t kValA[]  = {0x11, 0x22};
    static constexpr uint8_t kValB[]  = {0x33, 0x44, 0x55};
    static constexpr uint8_t kBufSize = 2 * Ltv::kHeaderSize + sizeof(kValA) + sizeof(kValB);

    uint8_t      buffer[kBufSize];
    FrameBuilder fb;
    const Ltv   *ltv;

    fb.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::Append(fb, kTypeA, kValA, sizeof(kValA)));
    SuccessOrQuit(Ltv::Append(fb, kTypeB, kValB, sizeof(kValB)));

    ltv = Ltv::FindLtv(buffer, fb.GetLength(), kTypeB);
    VerifyOrQuit(ltv != nullptr);
    VerifyOrQuit(ltv->GetType() == kTypeB);
    VerifyOrQuit(ltv->GetLength() == sizeof(kValB));
    VerifyOrQuit(memcmp(ltv->GetValue(), kValB, sizeof(kValB)) == 0);

    // kTypeA is still found too.
    ltv = Ltv::FindLtv(buffer, fb.GetLength(), kTypeA);
    VerifyOrQuit(ltv != nullptr);
    VerifyOrQuit(ltv->GetLength() == sizeof(kValA));
}

// ---------------------------------------------------------------------------
// TestPackedEncodeDecode
//
// Verifies that PackedLtvStream::Encode produces output no larger than the
// plain input and that Decode recovers the original bytes exactly.
// ---------------------------------------------------------------------------
void TestPackedEncodeDecode(void)
{
    // Three entries: [L=1,T=0x01,V={0xAA}], [L=2,T=0x02,V={0xBB,0xCC}], [L=0,T=0x03]
    static constexpr uint8_t kPlain[] = {0x01, 0x01, 0xAA, 0x02, 0x02, 0xBB, 0xCC, 0x00, 0x03};

    uint8_t packed[sizeof(kPlain)];
    uint8_t plain2[sizeof(kPlain)];
    uint8_t packedLen;
    uint8_t plain2Len;

    packedLen = PackedLtvStream::Encode(kPlain, sizeof(kPlain), packed, sizeof(packed));
    VerifyOrQuit(packedLen > 0);
    VerifyOrQuit(packedLen <= sizeof(kPlain));

    VerifyOrQuit(PackedLtvStream::Decode(packed, packedLen, plain2, sizeof(plain2), plain2Len) == kErrorNone);
    VerifyOrQuit(plain2Len == sizeof(kPlain));
    VerifyOrQuit(memcmp(plain2, kPlain, sizeof(kPlain)) == 0);
}

// ---------------------------------------------------------------------------
// TestPackedIterator
//
// Walks a packed stream with PackedLtvStream::Iterator; verifies type, length,
// value, and GetValueOffset() for each entry.
// ---------------------------------------------------------------------------
void TestPackedIterator(void)
{
    static constexpr uint8_t kValA[] = {0xDE, 0xAD};
    static constexpr uint8_t kValB[] = {0xBE, 0xEF, 0xCA};

    uint8_t      plain[16];
    FrameBuilder fb;

    fb.Init(plain, sizeof(plain));
    SuccessOrQuit(Ltv::Append(fb, 0x01, kValA, sizeof(kValA)));
    SuccessOrQuit(Ltv::Append(fb, 0x02, kValB, sizeof(kValB)));

    uint8_t packed[sizeof(plain)];
    uint8_t packedLen = PackedLtvStream::Encode(plain, static_cast<uint8_t>(fb.GetLength()), packed, sizeof(packed));

    PackedLtvStream::Iterator iter;
    iter.Init(packed, packedLen);

    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.GetType() == 0x01);
    VerifyOrQuit(iter.GetLength() == sizeof(kValA));
    VerifyOrQuit(memcmp(iter.GetValue(), kValA, sizeof(kValA)) == 0);

    uint8_t off0 = iter.GetValueOffset();
    VerifyOrQuit(memcmp(packed + off0, kValA, sizeof(kValA)) == 0);

    SuccessOrQuit(iter.Advance());

    VerifyOrQuit(!iter.IsDone());
    VerifyOrQuit(iter.GetType() == 0x02);
    VerifyOrQuit(iter.GetLength() == sizeof(kValB));
    VerifyOrQuit(memcmp(iter.GetValue(), kValB, sizeof(kValB)) == 0);

    uint8_t off1 = iter.GetValueOffset();
    VerifyOrQuit(off0 + sizeof(kValA) <= off1);
    VerifyOrQuit(off1 + sizeof(kValB) <= packedLen);
    VerifyOrQuit(memcmp(packed + off1, kValB, sizeof(kValB)) == 0);

    SuccessOrQuit(iter.Advance());
    VerifyOrQuit(iter.IsDone());
}

// ---------------------------------------------------------------------------
// TestPackedNL0
//
// When L=1 the stream is a single zero-length LTV encoded as a bare type byte
// (N_L=0 case from the spec).
// ---------------------------------------------------------------------------
void TestPackedNL0(void)
{
    // Plain: [L=0, T=0xAA]
    static constexpr uint8_t kPlain[] = {0x00, 0xAA};

    uint8_t packed[4];
    uint8_t packedLen = PackedLtvStream::Encode(kPlain, sizeof(kPlain), packed, sizeof(packed));

    VerifyOrQuit(packedLen == 1);
    VerifyOrQuit(packed[0] == 0xAA);

    uint8_t plain2[4];
    uint8_t plain2Len;
    VerifyOrQuit(PackedLtvStream::Decode(packed, packedLen, plain2, sizeof(plain2), plain2Len) == kErrorNone);
    VerifyOrQuit(plain2Len == sizeof(kPlain));
    VerifyOrQuit(memcmp(plain2, kPlain, sizeof(kPlain)) == 0);
}

// ---------------------------------------------------------------------------
// TestPackedEscapeCode
//
// A type value equal to all-ones in the available bits triggers the base
// (escape) format: a second header byte carries the type.
//
// Entry A: len=0, type=0x01.  Entry B (last): len=1, type=0x7F, value={0xCC}.
//
// For entry B: L=2, N_L=1, shift=7, allOnes=127=0x7F.  type==allOnes, so
// compact fails.  Base format: L=3, N_L=2, shift=6, header={0x7F,0x7F}.
// For entry A: scratchLen=3, L=4, N_L=2, shift=6, allOnes=63.  type=1<63,
// compact: header={0x01}.
// Packed: {0x01, 0x7F, 0x7F, 0xCC}.
// ---------------------------------------------------------------------------
void TestPackedEscapeCode(void)
{
    // Plain: [L=0,T=0x01][L=1,T=0x7F,V={0xCC}]
    static constexpr uint8_t kPlain[] = {0x00, 0x01, 0x01, 0x7F, 0xCC};

    uint8_t packed[sizeof(kPlain)];
    uint8_t packedLen = PackedLtvStream::Encode(kPlain, sizeof(kPlain), packed, sizeof(packed));

    VerifyOrQuit(packedLen == 4);
    VerifyOrQuit(packed[0] == 0x01); // Entry A compact
    VerifyOrQuit(packed[1] == 0x7F); // Entry B: escape sentinel
    VerifyOrQuit(packed[2] == 0x7F); // Entry B: type byte
    VerifyOrQuit(packed[3] == 0xCC); // Entry B: value

    uint8_t plain2[sizeof(kPlain)];
    uint8_t plain2Len;
    VerifyOrQuit(PackedLtvStream::Decode(packed, packedLen, plain2, sizeof(plain2), plain2Len) == kErrorNone);
    VerifyOrQuit(plain2Len == sizeof(kPlain));
    VerifyOrQuit(memcmp(plain2, kPlain, sizeof(kPlain)) == 0);
}

} // namespace ot

int main(void)
{
    ot::TestLtvAppendAndFind();
    ot::TestLtvEmptyValue();
    ot::TestLtvIterator();
    ot::TestLtvBufferBoundary();
    ot::TestLtvTruncated();
    ot::TestLtvOneByteBoundary();
    ot::TestLtvSimpleLtvInfo();
    ot::TestLtvFindSecond();
    ot::TestPackedEncodeDecode();
    ot::TestPackedIterator();
    ot::TestPackedNL0();
    ot::TestPackedEscapeCode();

    printf("All tests passed\n");
    return 0;
}
