/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "test_util.h"
#include "common/code_utils.hpp"
#include "common/set.hpp"

namespace ot {

// First 100 prime numbers.
const uint16_t kPrimes[] = {2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,
                            61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137, 139,
                            149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
                            239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337,
                            347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439,
                            443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541};

template <uint16_t kMaxSize>
void VerifySetContent(const Set<kMaxSize> &aSet, const uint16_t *aElements, uint16_t aLength)
{
    uint16_t index = 0;
    uint16_t element;

    // Verify that the set element matches with the entries in `aElements` list.

    for (element = 0; element < aSet.GetMaxSize(); element++)
    {
        bool shouldContain = false;

        if ((index < aLength) && (element == aElements[index]))
        {
            index++;
            shouldContain = true;
        }

        VerifyOrQuit(aSet.Contains(element) == shouldContain, "Contains() does not match expected value");
    }

    // Iterate through set elements and verify that they match the `aElements` array (in same order).

    index   = 0;
    element = Set<kMaxSize>::kSetIteratorFirst;

    while (aSet.GetNextElement(element) == OT_ERROR_NONE)
    {
        VerifyOrQuit(element == aElements[index++], "GetNextElement() failed\n");
    }

    VerifyOrQuit(index == aLength, "GetNextElement() did not return all expects elements\n");

    // Check IsEmpty() and GetNumberOfElements().

    VerifyOrQuit(aSet.IsEmpty() == (aLength == 0), "IsEmpty() failed");
    VerifyOrQuit(aSet.GetNumberOfElements() == aLength, "GetNumberOfElements() failed");
}

template <uint16_t kMaxSize, typename MaskType> void VerifySetContent(const Set<kMaxSize> &aSet, MaskType aBitMask)
{
    // Verify that only the elements from `aBitMask` are contained the set.

    printf("bitmask = 0x%x -- set = %s  \n", static_cast<uint32_t>(aBitMask), aSet.ToString().AsCString());

    for (uint16_t element = 0; element < aSet.GetMaxSize(); element++)
    {
        bool shouldContain = ((aBitMask & (1UL << element)) != 0);

        VerifyOrQuit(aSet.Contains(element) == shouldContain, "Contains() does not match expected value");
    }
}

template <uint16_t kMaxSize> void PopulateSet(Set<kMaxSize> &aSet, const uint16_t *aList, uint16_t aListLength)
{
    aSet.Clear();

    for (uint16_t i = 0; i < aListLength; i++)
    {
        aSet.Add(aList[i]);
    }
}

template <uint16_t kMaxSize> void TestSetAddRemoveFlip(Set<kMaxSize> &aSet, const uint16_t *aList, uint16_t aListLength)
{
    for (uint16_t i = 0; i < aListLength; i++)
    {
        aSet.Add(aList[i]);
        VerifySetContent(aSet, &aList[0], i + 1);
    }

    printf("  set = %s\n", aSet.ToString().AsCString());

    for (uint16_t i = 0; i < aListLength; i++)
    {
        aSet.Remove(aList[i]);
        VerifySetContent(aSet, &aList[i + 1], aListLength - i - 1);
    }

    for (uint16_t i = aListLength; i > 0; i--)
    {
        aSet.Flip(aList[i - 1]);
        VerifySetContent(aSet, &aList[i - 1], aListLength - i + 1);
    }

    for (uint16_t i = aListLength; i > 0; i--)
    {
        aSet.Flip(aList[i - 1]);
        VerifySetContent(aSet, &aList[0], i - 1);
    }
}

template <uint16_t kMaxSize> void TestSet(void)
{
    Set<kMaxSize> set1;
    Set<kMaxSize> set2;

    uint16_t all[kMaxSize];
    uint16_t odds[kMaxSize / 2];
    uint16_t evens[(kMaxSize + 1) / 2];
    uint16_t primesLength;

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Testing Set with size %d\n\n", kMaxSize);

    for (uint16_t i = 0; i < kMaxSize; i++)
    {
        all[i] = i;
    }

    for (uint16_t i = 0; i < OT_ARRAY_LENGTH(odds); i++)
    {
        odds[i] = 2 * i + 1;
    }

    for (uint16_t i = 0; i < OT_ARRAY_LENGTH(evens); i++)
    {
        evens[i] = 2 * i;
    }

    for (primesLength = 0; kPrimes[primesLength] < kMaxSize; primesLength++)
        ;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // After constructor

    VerifyOrQuit(set1.GetMaxSize() == kMaxSize, "GetMaxSize() failed");
    VerifyOrQuit(set2.GetMaxSize() == kMaxSize, "GetMaxSize() failed");

    VerifySetContent(set1, NULL, 0);
    printf("Empty set = %s\n", set1.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Add/Remove/Flip

    printf("all\n");
    TestSetAddRemoveFlip(set1, all, OT_ARRAY_LENGTH(all));
    printf("odds\n");
    TestSetAddRemoveFlip(set1, odds, OT_ARRAY_LENGTH(odds));
    printf("evens\n");
    TestSetAddRemoveFlip(set1, evens, OT_ARRAY_LENGTH(evens));
    printf("primes\n");
    TestSetAddRemoveFlip(set1, kPrimes, primesLength);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify Clear() behavior

    PopulateSet(set1, all, OT_ARRAY_LENGTH(all));
    VerifySetContent(set1, all, OT_ARRAY_LENGTH(all));
    set1.Clear();
    VerifySetContent(set1, all, 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify Intersect() behavior

    PopulateSet(set1, all, OT_ARRAY_LENGTH(all));
    PopulateSet(set2, evens, OT_ARRAY_LENGTH(evens));

    VerifySetContent(set1, all, OT_ARRAY_LENGTH(all));
    VerifySetContent(set2, evens, OT_ARRAY_LENGTH(evens));

    // Intersecting with itself
    set1.Intersect(set1);
    VerifySetContent(set1, all, OT_ARRAY_LENGTH(all));

    // Intersecting all with evens should give us back evens.
    printf("Intersecting %s with %s", set1.ToString().AsCString(), set2.ToString().AsCString());
    set1.Intersect(set2);
    printf(" gives %s\n", set1.ToString().AsCString());
    VerifySetContent(set1, evens, OT_ARRAY_LENGTH(evens));

    // Intersecting primes with evens should give us only {2}.
    PopulateSet(set2, kPrimes, primesLength);
    printf("Intersecting %s with %s", set1.ToString().AsCString(), set2.ToString().AsCString());
    set1.Intersect(set2);
    printf(" gives %s\n", set1.ToString().AsCString());
    VerifyOrQuit(set1.GetNumberOfElements() == 1, "Intersect() failed");
    VerifyOrQuit(set1.Contains(2) == true, "Intersect() failed");

    // Intersecting primes with odds should gives all primes except 2.
    PopulateSet(set1, odds, OT_ARRAY_LENGTH(odds));
    printf("Intersecting %s with %s", set1.ToString().AsCString(), set2.ToString().AsCString());
    set1.Intersect(set2);
    printf(" gives %s\n", set1.ToString().AsCString());
    VerifySetContent(set1, &kPrimes[1], primesLength - 1);

    // Intersecting odd primes with evens should gives the empty set.
    PopulateSet(set2, evens, OT_ARRAY_LENGTH(evens));
    printf("Intersecting %s with %s", set1.ToString().AsCString(), set2.ToString().AsCString());
    set1.Intersect(set2);
    printf(" gives %s\n", set1.ToString().AsCString());
    VerifySetContent(set1, NULL, 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify Union() behavior

    PopulateSet(set1, evens, OT_ARRAY_LENGTH(evens));
    PopulateSet(set2, odds, OT_ARRAY_LENGTH(odds));

    // Union with itself
    set1.Union(set1);
    VerifySetContent(set1, evens, OT_ARRAY_LENGTH(evens));

    // Union of odds and evens should be all.
    printf("Union of %s and %s", set1.ToString().AsCString(), set2.ToString().AsCString());
    set1.Union(set2);
    printf(" gives %s\n", set1.ToString().AsCString());
    VerifySetContent(set1, all, OT_ARRAY_LENGTH(all));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify operators ==, !=, and = (assignment)

    PopulateSet(set1, evens, OT_ARRAY_LENGTH(evens));
    PopulateSet(set2, odds, OT_ARRAY_LENGTH(odds));

    VerifyOrQuit(set1 == set1, "operator== failed");
    VerifyOrQuit(set2 == set2, "operator== failed");
    VerifyOrQuit(set1 != set2, "operator!= failed");

    set2 = set1;
    VerifyOrQuit(set1 == set2, "operator== failed");

    set1.Clear();
    set2.Clear();
    VerifyOrQuit(set1 == set2, "operator== failed");

    for (uint16_t i = 0; i < kMaxSize; i++)
    {
        set1.Flip(i);
        VerifyOrQuit(set1 != set2, "operator== failed");
        set1.Flip(i);
        VerifyOrQuit(set1 == set2, "operator== failed");
    }

    printf(" -- PASS\n");
}

void TestSet16(void)
{
    const uint16_t kMask1 = 0xba42;
    const uint16_t kMask2 = 0xfedb;
    const uint16_t kMask3 = 0xffff;
    const uint16_t kMask4 = 0xa5a5;

    Set<16> set1(kMask1);
    Set<16> set2(kMask2);
    Set<16> set3;

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Testing Set<16> specific methods\n\n");

    VerifyOrQuit(set1.GetAsMask() == kMask1, "GetAsMask() failed");
    VerifyOrQuit(set2.GetAsMask() == kMask2, "GetAsMask() failed");
    VerifyOrQuit(set3.GetAsMask() == 0, "GetAsMask() failed");

    VerifySetContent(set1, kMask1);
    VerifySetContent(set2, kMask2);

    set1.SetFromMask(kMask3);
    VerifyOrQuit(set1.GetAsMask() == kMask3, "SetFromMask() failed");
    VerifySetContent(set1, kMask3);

    set2.SetFromMask(kMask4);
    VerifyOrQuit(set2.GetAsMask() == kMask4, "SetFromMask() failed");
    VerifySetContent(set2, kMask4);

    set1.SetFromMask(0);
    VerifyOrQuit(set1.GetAsMask() == 0, "SetFromMask() failed");
    VerifyOrQuit(set1.IsEmpty(), "SetFromMask() failed");

    printf(" -- PASS\n");
}

void TestSet32(void)
{
    const uint32_t kMask1 = 0xfedcba98;
    const uint32_t kMask2 = 0x12345670;
    const uint32_t kMask3 = 0xffffffff;
    const uint32_t kMask4 = 0xa5a5a5a5;

    Set<32> set1(kMask1);
    Set<32> set2(kMask2);
    Set<32> set3;

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Testing Set<32> specific methods\n\n");

    VerifyOrQuit(set1.GetAsMask() == kMask1, "GetAsMask() failed");
    VerifyOrQuit(set2.GetAsMask() == kMask2, "GetAsMask() failed");
    VerifyOrQuit(set3.GetAsMask() == 0, "GetAsMask() failed");

    VerifySetContent(set1, kMask1);
    VerifySetContent(set2, kMask2);

    set1.SetFromMask(kMask3);
    VerifyOrQuit(set1.GetAsMask() == kMask3, "SetFromMask() failed");
    VerifySetContent(set1, kMask3);

    set2.SetFromMask(kMask4);
    VerifyOrQuit(set2.GetAsMask() == kMask4, "SetFromMask() failed");
    VerifySetContent(set2, kMask4);

    set1.SetFromMask(0);
    VerifyOrQuit(set1.GetAsMask() == 0, "SetFromMask() failed");
    VerifyOrQuit(set1.IsEmpty(), "SetFromMask() failed");

    printf(" -- PASS\n");
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestSet<3>();
    ot::TestSet<9>();
    ot::TestSet<16>();
    ot::TestSet<20>();
    ot::TestSet<32>();
    ot::TestSet<77>();
    ot::TestSet<500>();

    ot::TestSet16();
    ot::TestSet32();

    printf("\nAll tests passed.\n");
    return 0;
}
#endif
