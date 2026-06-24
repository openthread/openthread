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

#include "test_platform.h"
#include "test_util.hpp"

#include "common/bit_set.hpp"

namespace ot {

template <uint16_t kNumBits> void TestBitSetProperties(void)
{
    BitSet<kNumBits> bitSet;
    uint16_t         i;

    printf("TestBitSetProperties<%u>\r\n", kNumBits);

    bitSet.Clear();
    VerifyOrQuit(bitSet.IsEmpty());
    VerifyOrQuit(bitSet.CountElements() == 0);
    VerifyOrQuit(bitSet.GetNumBits() == kNumBits);
    VerifyOrQuit(bitSet.GetMaskSize() == BytesForBitSize(kNumBits));

    for (i = 0; i < kNumBits; i++)
    {
        VerifyOrQuit(!bitSet.Has(i));
    }

    bitSet.Add(0);
    VerifyOrQuit(!bitSet.IsEmpty());
    VerifyOrQuit(bitSet.CountElements() == 1);
    VerifyOrQuit(bitSet.Has(0));

    bitSet.Remove(0);
    VerifyOrQuit(bitSet.IsEmpty());
    VerifyOrQuit(!bitSet.Has(0));

    for (i = 0; i < kNumBits; i++)
    {
        bitSet.Add(i);
        VerifyOrQuit(bitSet.Has(i));
        VerifyOrQuit(bitSet.CountElements() == i + 1);
    }

    bitSet.Clear();
    VerifyOrQuit(bitSet.IsEmpty());

    for (i = 0; i < kNumBits; i++)
    {
        VerifyOrQuit(!bitSet.Has(i));
    }

    bitSet.Update(0, true);
    VerifyOrQuit(bitSet.Has(0));
    bitSet.Update(0, false);
    VerifyOrQuit(!bitSet.Has(0));
}

void TestBitSetOperations(void)
{
    BitSet<10> set1;
    BitSet<10> set2;
    BitSet<10> unionSet;
    BitSet<10> intersectSet;
    BitSet<10> subtractSet;

    printf("TestBitSetOperations\r\n");

    set1.Clear();
    set2.Clear();

    VerifyOrQuit(set1.IsSubsetOf(set2));
    VerifyOrQuit(set1.IsSupersetOf(set2));
    VerifyOrQuit(set1 == set2);

    set1.Add(1);
    set1.Add(3);
    set1.Add(8);

    VerifyOrQuit(!set1.IsEmpty());
    VerifyOrQuit(set1.CountElements() == 3);

    VerifyOrQuit(!set1.IsSubsetOf(set2));
    VerifyOrQuit(set2.IsSubsetOf(set1));
    VerifyOrQuit(set1.IsSupersetOf(set2));
    VerifyOrQuit(set1 != set2);

    VerifyOrQuit(set1.IsSubsetOf(set1));
    VerifyOrQuit(set1.IsSupersetOf(set1));

    set2.Add(1);
    set2.Add(8);

    VerifyOrQuit(set2.IsSubsetOf(set1));
    VerifyOrQuit(!set1.IsSubsetOf(set2));
    VerifyOrQuit(set1.IsSupersetOf(set2));

    unionSet = set1;
    unionSet.UnionWith(set2);
    VerifyOrQuit(unionSet == set1);

    set2.Add(5);
    unionSet = set1;
    unionSet.UnionWith(set2);
    VerifyOrQuit(unionSet.CountElements() == 4);
    VerifyOrQuit(unionSet.Has(1) && unionSet.Has(3) && unionSet.Has(5) && unionSet.Has(8));

    intersectSet = set1;
    intersectSet.IntersectWith(set2);
    VerifyOrQuit(intersectSet.CountElements() == 2);
    VerifyOrQuit(intersectSet.Has(1) && intersectSet.Has(8));
    VerifyOrQuit(!intersectSet.Has(3) && !intersectSet.Has(5));

    subtractSet = set1;
    subtractSet.SubtractWith(set2);
    VerifyOrQuit(subtractSet.CountElements() == 1);
    VerifyOrQuit(subtractSet.Has(3));
}

void TestBitSetComplementAndTidy(void)
{
    BitSet<10>     bitSet;
    const uint8_t *bytes;
    uint16_t       i;
    uint8_t        dirtyMask[2];

    printf("TestBitSetComplementAndTidy\r\n");

    bitSet.Clear();
    bitSet.Complement();

    for (i = 0; i < 10; i++)
    {
        VerifyOrQuit(bitSet.Has(i));
    }

    bytes = bitSet.GetMaskBytes();
    VerifyOrQuit(bitSet.GetMaskSize() == 2);
    VerifyOrQuit(bytes[0] == 0xff);
    VerifyOrQuit(bytes[1] == 0xc0);

    bitSet.Complement();
    VerifyOrQuit(bitSet.IsEmpty());
    VerifyOrQuit(bytes[0] == 0x00);
    VerifyOrQuit(bytes[1] == 0x00);

    dirtyMask[0] = 0xff;
    dirtyMask[1] = 0xff;
    bitSet.SetMask(dirtyMask);

    for (i = 0; i < 10; i++)
    {
        VerifyOrQuit(bitSet.Has(i));
    }

    VerifyOrQuit(bitSet.CountElements() == 10);

    bytes = bitSet.GetMaskBytes();
    VerifyOrQuit(bytes[0] == 0xff);
    VerifyOrQuit(bytes[1] == 0xc0);
}

void TestBitSet(void)
{
    TestBitSetProperties<1>();
    TestBitSetProperties<7>();
    TestBitSetProperties<8>();
    TestBitSetProperties<9>();
    TestBitSetProperties<16>();
    TestBitSetProperties<17>();
    TestBitSetProperties<100>();

    TestBitSetOperations();
    TestBitSetComplementAndTidy();
}

} // namespace ot

int main(void)
{
    ot::TestBitSet();

    printf("All tests passed\n");
    return 0;
}
