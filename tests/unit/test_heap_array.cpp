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

#include <string.h>

#include <openthread/config.h>

#include "test_util.hpp"

#include "common/heap_array.hpp"
#include "common/type_traits.hpp"

namespace ot {

// Counters tracking number of times `Entry` constructor and
// destructor are invoked. These are used to verify that the `Array`
// properly calls constructor/destructor when allocating and copying
// array buffer.
static uint16_t sConstructorCalls = 0;
static uint16_t sDestructorCalls  = 0;

class Entry
{
public:
    Entry(void)
        : mValue(0)
        , mInitialized(true)
    {
        sConstructorCalls++;
    }

    explicit Entry(uint16_t aValue)
        : mValue(aValue)
        , mInitialized(true)
    {
        sConstructorCalls++;
    }

    Entry(const Entry &aEntry)
        : mValue(aEntry.mValue)
        , mInitialized(true)
    {
        sConstructorCalls++;
    }

    ~Entry(void) { sDestructorCalls++; }

    uint16_t GetValue(void) const { return mValue; }
    void     SetValue(uint16_t aValue) { mValue = aValue; }
    bool     IsInitialized(void) const { return mInitialized; }
    bool     operator==(const Entry &aOther) const { return mValue == aOther.mValue; }
    bool     Matches(uint16_t aValue) const { return mValue == aValue; }

private:
    uint16_t mValue;
    bool     mInitialized;
};

template <typename EntryType>
void VerifyEntry(const EntryType &aEntry, const Heap::Array<EntryType, 2> &aArray, int aExpectedValue)
{
    // Verify the entry in a given array with an expected value.
    // Specializations of this template are defined below for `EntryType`
    // being `uint16_t` or `Entry` class.

    OT_UNUSED_VARIABLE(aEntry);
    OT_UNUSED_VARIABLE(aArray);
    OT_UNUSED_VARIABLE(aExpectedValue);

    VerifyOrQuit(false, "Specializations of this template method MUST be used instead");
}

template <> void VerifyEntry(const uint16_t &aEntry, const Heap::Array<uint16_t, 2> &aArray, int aExpectedValue)
{
    OT_UNUSED_VARIABLE(aArray);
    VerifyOrQuit(aEntry == static_cast<uint16_t>(aExpectedValue));
}

template <> void VerifyEntry(const Entry &aEntry, const Heap::Array<Entry, 2> &aArray, int aExpectedValue)
{
    VerifyOrQuit(aEntry.IsInitialized());
    VerifyOrQuit(aEntry.GetValue() == static_cast<uint16_t>(aExpectedValue));

    VerifyOrQuit(aArray.ContainsMatching(aEntry.GetValue()));
    VerifyOrQuit(aArray.FindMatching(aEntry.GetValue()) == &aEntry);
}

template <typename EntryType, typename... Args> void VerifyArray(const Heap::Array<EntryType, 2> &aArray, Args... aArgs)
{
    // Verify that array content matches the `aArgs` sequence
    // (which can be empty).

    constexpr uint16_t kUnusedValue = 0xffff;

    int      values[] = {aArgs..., 0};
    uint16_t index    = 0;

    printf(" - Array (len:%u, capacity:%u) = { ", aArray.GetLength(), aArray.GetCapacity());

    VerifyOrQuit(aArray.GetLength() == sizeof...(aArgs));

    if (aArray.GetLength() == 0)
    {
        VerifyOrQuit(aArray.AsCArray() == nullptr);
        VerifyOrQuit(aArray.Front() == nullptr);
        VerifyOrQuit(aArray.Back() == nullptr);
    }
    else
    {
        VerifyOrQuit(aArray.AsCArray() != nullptr);
    }

    for (const EntryType &entry : aArray)
    {
        VerifyOrQuit(index < aArray.GetLength());

        VerifyEntry(entry, aArray, values[index]);

        VerifyOrQuit(aArray.Contains(entry));
        VerifyOrQuit(aArray.Find(entry) == &entry);
        VerifyOrQuit(aArray.IndexOf(entry) == index);

        if (index == 0)
        {
            VerifyOrQuit(aArray.Front() == &entry);
        }

        if (index == aArray.GetLength())
        {
            VerifyOrQuit(aArray.Back() == &entry);
        }

        printf("%u ", values[index]);

        index++;
    }

    VerifyOrQuit(index == aArray.GetLength());

    VerifyOrQuit(!aArray.Contains(EntryType(kUnusedValue)));
    VerifyOrQuit(aArray.Find(EntryType(kUnusedValue)) == nullptr);

    if (TypeTraits::IsSame<EntryType, Entry>::kValue)
    {
        printf("} (constructor-calls:%u, destructor-calls:%u)\n", sConstructorCalls, sDestructorCalls);
        VerifyOrQuit(sConstructorCalls - sDestructorCalls == aArray.GetLength());
    }
    else
    {
        printf("}\n");
    }
}

void TestHeapArrayOfUint16(void)
{
    Heap::Array<uint16_t, 2> array;
    Heap::Array<uint16_t, 2> array2;
    uint16_t                *entry;

    printf("\n\n====================================================================================\n");
    printf("TestHeapArrayOfUint16\n\n");

    printf("------------------------------------------------------------------------------------\n");
    printf("After constructor\n");
    VerifyOrQuit(array.GetCapacity() == 0);
    VerifyArray(array);

    printf("------------------------------------------------------------------------------------\n");
    printf("PushBack(aEntry)\n");

    SuccessOrQuit(array.PushBack(1));
    VerifyArray(array, 1);
    VerifyOrQuit(array.GetCapacity() == 2);

    SuccessOrQuit(array.PushBack(2));
    VerifyArray(array, 1, 2);
    VerifyOrQuit(array.GetCapacity() == 2);

    SuccessOrQuit(array.PushBack(3));
    VerifyArray(array, 1, 2, 3);
    VerifyOrQuit(array.GetCapacity() == 4);

    printf("------------------------------------------------------------------------------------\n");
    printf("entry = PushBack()\n");

    entry = array.PushBack();
    VerifyOrQuit(entry != nullptr);
    *entry = 4;
    VerifyArray(array, 1, 2, 3, 4);
    VerifyOrQuit(array.GetCapacity() == 4);

    entry = array.PushBack();
    VerifyOrQuit(entry != nullptr);
    *entry = 5;
    VerifyArray(array, 1, 2, 3, 4, 5);
    VerifyOrQuit(array.GetCapacity() == 6);

    printf("------------------------------------------------------------------------------------\n");
    printf("Clear()\n");

    array.Clear();
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 6);

    *array.PushBack() = 11;
    SuccessOrQuit(array.PushBack(22));
    SuccessOrQuit(array.PushBack(33));
    SuccessOrQuit(array.PushBack(44));
    *array.PushBack() = 55;

    VerifyArray(array, 11, 22, 33, 44, 55);
    VerifyOrQuit(array.GetCapacity() == 6);

    SuccessOrQuit(array.PushBack(66));
    SuccessOrQuit(array.PushBack(77));
    VerifyArray(array, 11, 22, 33, 44, 55, 66, 77);
    VerifyOrQuit(array.GetCapacity() == 8);

    printf("------------------------------------------------------------------------------------\n");
    printf("PopBack()\n");

    array.PopBack();
    VerifyArray(array, 11, 22, 33, 44, 55, 66);
    VerifyOrQuit(array.GetCapacity() == 8);

    array.PopBack();
    array.PopBack();
    array.PopBack();
    array.PopBack();
    array.PopBack();
    VerifyArray(array, 11);
    VerifyOrQuit(array.GetCapacity() == 8);

    array.PopBack();
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 8);

    array.PopBack();
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 8);

    for (uint16_t num = 0; num < 11; num++)
    {
        SuccessOrQuit(array.PushBack(num + 0x100));
    }

    VerifyArray(array, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, 0x109, 0x10a);
    VerifyOrQuit(array.GetCapacity() == 12);

    printf("------------------------------------------------------------------------------------\n");
    printf("Free()\n");

    array.Free();
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 0);

    array.Free();
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 0);

    printf("------------------------------------------------------------------------------------\n");
    printf("ReserveCapacity()\n");

    SuccessOrQuit(array.ReserveCapacity(5));
    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 5);

    SuccessOrQuit(array.PushBack(0));
    VerifyArray(array, 0);
    VerifyOrQuit(array.GetCapacity() == 5);

    for (uint16_t num = 1; num < 5; num++)
    {
        SuccessOrQuit(array.PushBack(num));
    }

    VerifyArray(array, 0, 1, 2, 3, 4);
    VerifyOrQuit(array.GetCapacity() == 5);

    SuccessOrQuit(array.PushBack(5));
    VerifyArray(array, 0, 1, 2, 3, 4, 5);
    VerifyOrQuit(array.GetCapacity() == 7);

    SuccessOrQuit(array.ReserveCapacity(3));
    VerifyArray(array, 0, 1, 2, 3, 4, 5);
    VerifyOrQuit(array.GetCapacity() == 7);

    SuccessOrQuit(array.ReserveCapacity(10));
    VerifyArray(array, 0, 1, 2, 3, 4, 5);
    VerifyOrQuit(array.GetCapacity() == 10);

    printf("------------------------------------------------------------------------------------\n");
    printf("TakeFrom()\n");

    for (uint16_t num = 0; num < 7; num++)
    {
        SuccessOrQuit(array2.PushBack(num + 0x20));
    }

    VerifyArray(array2, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26);

    array2.TakeFrom(static_cast<Heap::Array<uint16_t, 2> &&>(array));

    VerifyArray(array);
    VerifyOrQuit(array.GetCapacity() == 0);

    VerifyArray(array2, 0, 1, 2, 3, 4, 5);
    VerifyOrQuit(array2.GetCapacity() == 10);

    printf("\n -- PASS\n");
}

void TestHeapArray(void)
{
    VerifyOrQuit(sConstructorCalls == 0);
    VerifyOrQuit(sDestructorCalls == 0);

    printf("\n\n====================================================================================\n");
    printf("TestHeapArray\n\n");

    {
        Heap::Array<Entry, 2> array;
        Heap::Array<Entry, 2> array2;
        Entry                *entry;

        printf("------------------------------------------------------------------------------------\n");
        printf("After constructor\n");
        VerifyOrQuit(array.GetCapacity() == 0);
        VerifyArray(array);

        printf("------------------------------------------------------------------------------------\n");
        printf("PushBack(aEntry)\n");

        SuccessOrQuit(array.PushBack(Entry(1)));
        VerifyArray(array, 1);
        VerifyOrQuit(array.GetCapacity() == 2);

        SuccessOrQuit(array.PushBack(Entry(2)));
        VerifyArray(array, 1, 2);
        VerifyOrQuit(array.GetCapacity() == 2);

        SuccessOrQuit(array.PushBack(Entry(3)));
        VerifyArray(array, 1, 2, 3);
        VerifyOrQuit(array.GetCapacity() == 4);

        entry = array.PushBack();
        VerifyOrQuit(entry != nullptr);
        VerifyOrQuit(entry->IsInitialized());
        VerifyOrQuit(entry->GetValue() == 0);
        entry->SetValue(4);
        VerifyArray(array, 1, 2, 3, 4);
        VerifyOrQuit(array.GetCapacity() == 4);

        entry = array.PushBack();
        VerifyOrQuit(entry != nullptr);
        VerifyOrQuit(entry->IsInitialized());
        VerifyOrQuit(entry->GetValue() == 0);
        entry->SetValue(5);
        VerifyArray(array, 1, 2, 3, 4, 5);
        VerifyOrQuit(array.GetCapacity() == 6);

        printf("------------------------------------------------------------------------------------\n");
        printf("PopBack()\n");

        array.PopBack();
        VerifyArray(array, 1, 2, 3, 4);
        VerifyOrQuit(array.GetCapacity() == 6);

        array.PopBack();
        VerifyArray(array, 1, 2, 3);
        VerifyOrQuit(array.GetCapacity() == 6);

        SuccessOrQuit(array.PushBack(Entry(7)));
        VerifyArray(array, 1, 2, 3, 7);
        VerifyOrQuit(array.GetCapacity() == 6);

        array.PopBack();
        VerifyArray(array, 1, 2, 3);
        VerifyOrQuit(array.GetCapacity() == 6);

        printf("------------------------------------------------------------------------------------\n");
        printf("Clear()\n");

        array.Clear();
        VerifyArray(array);
        VerifyOrQuit(array.GetCapacity() == 6);

        for (uint16_t num = 0; num < 11; num++)
        {
            SuccessOrQuit(array.PushBack(Entry(num)));
        }

        VerifyArray(array, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        VerifyOrQuit(array.GetCapacity() == 12);

        printf("------------------------------------------------------------------------------------\n");
        printf("Free()\n");
        array.Free();
        VerifyArray(array);
        VerifyOrQuit(array.GetCapacity() == 0);

        printf("------------------------------------------------------------------------------------\n");
        printf("ReserveCapacity()\n");

        SuccessOrQuit(array.ReserveCapacity(5));
        VerifyArray(array);
        VerifyOrQuit(array.GetCapacity() == 5);

        SuccessOrQuit(array.PushBack(Entry(0)));
        VerifyArray(array, 0);
        VerifyOrQuit(array.GetCapacity() == 5);

        for (uint16_t num = 1; num < 5; num++)
        {
            SuccessOrQuit(array.PushBack(Entry(num)));
        }

        VerifyArray(array, 0, 1, 2, 3, 4);
        VerifyOrQuit(array.GetCapacity() == 5);

        SuccessOrQuit(array.PushBack(Entry(5)));
        VerifyArray(array, 0, 1, 2, 3, 4, 5);
        VerifyOrQuit(array.GetCapacity() == 7);

        SuccessOrQuit(array.ReserveCapacity(3));
        VerifyArray(array, 0, 1, 2, 3, 4, 5);
        VerifyOrQuit(array.GetCapacity() == 7);

        SuccessOrQuit(array.ReserveCapacity(10));
        VerifyArray(array, 0, 1, 2, 3, 4, 5);
        VerifyOrQuit(array.GetCapacity() == 10);

        printf("------------------------------------------------------------------------------------\n");
        printf("TakeFrom()\n");

        for (uint16_t num = 0; num < 7; num++)
        {
            SuccessOrQuit(array2.PushBack(Entry(num + 0x20)));
        }

        array2.TakeFrom(static_cast<Heap::Array<Entry, 2> &&>(array));

        VerifyOrQuit(array.GetLength() == 0);
        VerifyOrQuit(array.GetCapacity() == 0);

        VerifyArray(array2, 0, 1, 2, 3, 4, 5);
        VerifyOrQuit(array2.GetCapacity() == 10);
    }

    printf("------------------------------------------------------------------------------------\n");
    printf("Array destructor\n");
    printf(" - (constructor-calls:%u, destructor-calls:%u)\n", sConstructorCalls, sDestructorCalls);
    VerifyOrQuit(sConstructorCalls == sDestructorCalls,
                 "Array destructor failed to invoke destructor on all its existing entries");

    printf("\n -- PASS\n");
}

} // namespace ot

int main(void)
{
    ot::TestHeapArrayOfUint16();
    ot::TestHeapArray();
    printf("\nAll tests passed.\n");
    return 0;
}
