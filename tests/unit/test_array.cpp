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

#include <stdarg.h>
#include <string.h>

#include "test_platform.h"

#include <openthread/config.h>

#include "common/array.hpp"
#include "common/debug.hpp"
#include "common/equatable.hpp"
#include "common/instance.hpp"
#include "common/type_traits.hpp"

#include "test_util.h"

namespace ot {

void TestArray(void)
{
    constexpr uint16_t kMaxSize    = 10;
    constexpr uint16_t kStartValue = 100;

    Array<uint16_t, kMaxSize> array;
    uint8_t                   index;
    uint16_t                  seed;

    // All methods after constructor
    VerifyOrQuit(array.IsEmpty());
    VerifyOrQuit(!array.IsFull());
    VerifyOrQuit(array.GetLength() == 0);
    VerifyOrQuit(array.GetMaxSize() == kMaxSize);
    VerifyOrQuit(array.At(0) == nullptr);
    VerifyOrQuit(array.Front() == nullptr);
    VerifyOrQuit(array.Back() == nullptr);
    VerifyOrQuit(array.PopBack() == nullptr);

    seed = kStartValue;

    for (uint8_t len = 1; len <= kMaxSize; len++)
    {
        for (uint8_t iter = 0; iter < 2; iter++)
        {
            // On `iter == 0` use `PushBack(aEntry)` and on `iter == 1`
            // `PushBack()` version which returns a pointer to the newly
            // added entry.

            if (iter == 0)
            {
                SuccessOrQuit(array.PushBack(seed + len));
            }
            else
            {
                uint16_t *entry = array.PushBack();

                VerifyOrQuit(entry != nullptr);
                *entry = seed + len;
            }

            VerifyOrQuit(!array.IsEmpty());
            VerifyOrQuit(array.IsFull() == (len == kMaxSize));
            VerifyOrQuit(array.GetLength() == len);

            VerifyOrQuit(array.Front() != nullptr);
            VerifyOrQuit(*array.Front() == seed + 1);
            VerifyOrQuit(array.Back() != nullptr);
            VerifyOrQuit(*array.Back() == seed + len);

            for (index = 0; index < len; index++)
            {
                VerifyOrQuit(array[index] == seed + index + 1);
                VerifyOrQuit(array.At(index) != nullptr);
                VerifyOrQuit(*array.At(index) == seed + index + 1);

                VerifyOrQuit(array.Contains(seed + index + 1));
                VerifyOrQuit(array.Find(seed + index + 1) == &array[index]);

                VerifyOrQuit(!array.Contains(seed));
                VerifyOrQuit(array.Find(seed) == nullptr);
            }

            index = 0;

            for (uint16_t value : array)
            {
                VerifyOrQuit(value == array[index]);
                index++;
            }

            index = 0;

            for (uint16_t &entry : array)
            {
                // Uddate the value stored at the entry
                entry++;

                VerifyOrQuit(entry == array[index]);
                VerifyOrQuit(array.IndexOf(entry) == index);

                index++;
            }

            seed++;

            // On `iter == 0` we verify `PopBack()` and remove the
            // last entry. It will be added again from next `iter`
            // loop (on `iter == 1`).

            if (iter == 0)
            {
                uint16_t *entry = array.PopBack();

                VerifyOrQuit(entry != nullptr);
                VerifyOrQuit(*entry == seed + len);
                VerifyOrQuit(array.GetLength() == len - 1);
            }
        }
    }

    VerifyOrQuit(array.IsFull());
    VerifyOrQuit(array.PushBack(0) == kErrorNoBufs);
    VerifyOrQuit(array.PushBack() == nullptr);

    for (uint8_t len = kMaxSize; len >= 1; len--)
    {
        uint16_t *entry;

        VerifyOrQuit(array.GetLength() == len);
        VerifyOrQuit(array.Back() == &array[len - 1]);

        entry = array.PopBack();
        VerifyOrQuit(entry != nullptr);
        VerifyOrQuit(*entry == seed + len);

        VerifyOrQuit(array.GetLength() == len - 1);
        VerifyOrQuit(!array.IsFull());
    }

    VerifyOrQuit(array.IsEmpty());

    SuccessOrQuit(array.PushBack(seed));
    VerifyOrQuit(!array.IsEmpty());

    array.Clear();
    VerifyOrQuit(array.IsEmpty());
}

void TestArrayCopyAndFindMatching(void)
{
    constexpr uint16_t kMaxSize = 10;

    enum MatchMode : uint8_t
    {
        kMatchAll,
        kMatchNone,
        kMatchOddYear,
        kMatchEvenYear,
    };

    struct Entry : public Unequatable<Entry>
    {
        Entry(void) = default;

        Entry(const char *aName, uint16_t aYear)
            : mName(aName)
            , mYear(aYear)
        {
        }

        bool operator==(const Entry &aOther) const { return (mName == aOther.mName) && (mYear == aOther.mYear); }
        bool Matches(const char *aName) const { return strcmp(aName, mName) == 0; }
        bool Matches(uint16_t aYear) const { return aYear == mYear; }

        bool Matches(MatchMode aMatchMode) const
        {
            bool matches = false;

            switch (aMatchMode)
            {
            case kMatchAll:
                matches = true;
                break;
            case kMatchNone:
                matches = false;
                break;
            case kMatchOddYear:
                matches = ((mYear % 2) != 0);
                break;
            case kMatchEvenYear:
                matches = ((mYear % 2) == 0);
                break;
            }

            return matches;
        }

        const char *mName;
        uint16_t    mYear;
    };

    static const MatchMode kMatchModes[] = {kMatchAll, kMatchNone, kMatchEvenYear, kMatchOddYear};

    static const char *kMatchModeStrings[] = {
        "kMatchAll",
        "kMatchNone",
        "kMatchOddYear",
        "kMatchEvenYear",
    };

    Entry ps1("PS", 1994);
    Entry ps2("PS2", 1999);
    Entry ps3("PS3", 2006);
    Entry ps4("PS4", 2013);
    Entry ps5("PS5", 2020);

    Array<Entry, kMaxSize> array1;
    Array<Entry, kMaxSize> array2;
    Array<Entry, kMaxSize> array3(array1);

    VerifyOrQuit(array1.IsEmpty());
    VerifyOrQuit(array2.IsEmpty());
    VerifyOrQuit(array3.IsEmpty());

    SuccessOrQuit(array1.PushBack(ps1));
    SuccessOrQuit(array1.PushBack(ps2));
    SuccessOrQuit(array1.PushBack(ps3));
    SuccessOrQuit(array1.PushBack(ps4));
    VerifyOrQuit(array1.GetLength() == 4);

    SuccessOrQuit(array2.PushBack(ps3));
    SuccessOrQuit(array2.PushBack(ps5));
    VerifyOrQuit(array2.GetLength() == 2);

    array3 = array2 = array1;

    VerifyOrQuit(array1.GetLength() == 4);
    VerifyOrQuit(array2.GetLength() == 4);
    VerifyOrQuit(array3.GetLength() == 4);

    for (uint8_t index = 0; index < array1.GetLength(); index++)
    {
        VerifyOrQuit(array1[index] == array2[index]);
        VerifyOrQuit(array1[index] == array3[index]);
    }

    array3.Clear();

    array1 = array3;
    VerifyOrQuit(array1.IsEmpty());
    VerifyOrQuit(array1.GetLength() == 0);

    {
        Array<Entry, kMaxSize> array4(array2);

        VerifyOrQuit(array4.GetLength() == 4);

        for (uint8_t index = 0; index < array1.GetLength(); index++)
        {
            VerifyOrQuit(array2[index] == array4[index]);
        }
    }

    SuccessOrQuit(array2.PushBack(ps5));
    VerifyOrQuit(array2.GetLength() == 5);

    for (const Entry &entry : array2)
    {
        Entry *match;

        printf("- Name:%-3s Year:%d\n", entry.mName, entry.mYear);

        match = array2.FindMatching(entry.mName);
        VerifyOrQuit(match != nullptr);
        VerifyOrQuit(match == &entry);
        VerifyOrQuit(array2.ContainsMatching(entry.mName));

        match = array2.FindMatching(entry.mYear);
        VerifyOrQuit(match != nullptr);
        VerifyOrQuit(match == &entry);
        VerifyOrQuit(array2.ContainsMatching(entry.mYear));
    }

    VerifyOrQuit(array2.FindMatching("PS6") == nullptr);
    VerifyOrQuit(!array2.ContainsMatching("PS6"));
    VerifyOrQuit(array2.FindMatching(static_cast<uint16_t>(2001)) == nullptr);
    VerifyOrQuit(!array2.ContainsMatching(static_cast<uint16_t>(2001)));

    // Test removing of entries at every index.

    array1 = array2;

    for (const Entry &entryToRemove : array1)
    {
        Entry *match;

        // Test `Remove()`

        array2 = array1;
        match  = array2.Find(entryToRemove);
        VerifyOrQuit(match != nullptr);
        array2.Remove(*match);

        VerifyOrQuit(array2.GetLength() == array1.GetLength() - 1);

        for (const Entry &entry : array2)
        {
            VerifyOrQuit(entry != entryToRemove);
            VerifyOrQuit(array1.Contains(entry));
        }

        // Test `RemoveMatching()`

        array2 = array1;
        array2.RemoveMatching(entryToRemove.mName);

        printf("\n- - - - - - - - - - - - - - - - - - - - - - - - ");
        printf("\nArray after `RemoveMatching()` on entry %s\n", entryToRemove.mName);

        VerifyOrQuit(array2.GetLength() == array1.GetLength() - 1);

        for (const Entry &entry : array2)
        {
            printf("- Name:%-3s Year:%d\n", entry.mName, entry.mYear);
            VerifyOrQuit(entry != entryToRemove);
            VerifyOrQuit(array1.Contains(entry));
        }

        // Test `RemoveMatchin()` with a non-existing match

        array2.RemoveMatching(entryToRemove.mName);
        VerifyOrQuit(array2.GetLength() == array1.GetLength() - 1);

        // Test `RemoveAllMatching()` removing a single matching entry.

        array2 = array1;
        array2.RemoveAllMatching(entryToRemove.mName);

        VerifyOrQuit(array2.GetLength() == array1.GetLength() - 1);

        for (const Entry &entry : array2)
        {
            VerifyOrQuit(entry != entryToRemove);
            VerifyOrQuit(array1.Contains(entry));
        }

        array2.RemoveAllMatching(entryToRemove.mName);
        VerifyOrQuit(array2.GetLength() == array1.GetLength() - 1);

        // Test `RemoveAllMatching()` using different match modes
        // removing different subsets.

        for (MatchMode matchMode : kMatchModes)
        {
            array3 = array2;

            array3.RemoveAllMatching(matchMode);

            printf("\nArray after `RemoveAllMatching(%s)\n", kMatchModeStrings[matchMode]);

            for (const Entry &entry : array3)
            {
                VerifyOrQuit(!entry.Matches(matchMode));
                VerifyOrQuit(array2.Contains(entry));
                printf("- Name:%-3s Year:%d\n", entry.mName, entry.mYear);
            }

            for (const Entry &entry : array2)
            {
                if (!entry.Matches(matchMode))
                {
                    VerifyOrQuit(array2.Contains(entry));
                }
            }

            array3.RemoveAllMatching(kMatchAll);
            VerifyOrQuit(array3.IsEmpty());
        }
    }

    printf("\n");
}

void TestArrayIndexType(void)
{
    typedef Array<uint16_t, 255>           Array1;
    typedef Array<uint16_t, 256>           Array2;
    typedef Array<uint16_t, 100, uint16_t> Array3;

    static_assert(TypeTraits::IsSame<Array1::IndexType, uint8_t>::kValue, "Array1::IndexType is incorrect");
    static_assert(TypeTraits::IsSame<Array2::IndexType, uint16_t>::kValue, "Array2::IndexType is incorrect");
    static_assert(TypeTraits::IsSame<Array3::IndexType, uint16_t>::kValue, "Array3::IndexType is incorrect");
}

} // namespace ot

int main(void)
{
    ot::TestArray();
    ot::TestArrayCopyAndFindMatching();
    ot::TestArrayIndexType();

    printf("All tests passed\n");
    return 0;
}
