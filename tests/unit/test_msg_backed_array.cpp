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

#include <stdarg.h>
#include <string.h>

#include "test_platform.h"

#include <openthread/config.h>

#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/msg_backed_array.hpp"
#include "instance/instance.hpp"

#include "test_util.hpp"

namespace ot {

struct Entry : public Clearable<Entry>, public Equatable<Entry>
{
    static constexpr uint8_t kStringSize = 32;

    Entry(void) { Clear(); }

    Entry(bool aFlag, uint32_t aValue, const char *aString)
    {
        Clear();
        mFlag  = aFlag;
        mValue = aValue;
        SuccessOrQuit(StringCopy(mString, aString));
    }

    bool     mFlag;
    uint32_t mValue;
    char     mString[kStringSize];
};

constexpr uint16_t kInvalidIndex = NumericLimits<uint16_t>::kMax;

struct Context
{
    void Reset(void)
    {
        mIndex = kInvalidIndex;
        mEntry.Clear();
    }

    uint16_t mIndex;
    Entry    mEntry;
};

bool FindFirstTrueFlag(uint16_t aIndex, const Entry &aEntry, void *aContext)
{
    Context *context        = static_cast<Context *>(aContext);
    bool     shouldContinue = true;

    VerifyOrQuit(context != nullptr);

    if (aEntry.mFlag)
    {
        context->mIndex = aIndex;
        context->mEntry = aEntry;
        shouldContinue  = false;
    }

    return shouldContinue;
}

bool FindLastTrueFlag(uint16_t aIndex, const Entry &aEntry, void *aContext)
{
    Context *context = static_cast<Context *>(aContext);

    VerifyOrQuit(context != nullptr);

    if (aEntry.mFlag)
    {
        context->mIndex = aIndex;
        context->mEntry = aEntry;
    }

    return true; /* shouldContinue */
}

bool FindMaxValue(uint16_t aIndex, const Entry &aEntry, void *aContext)
{
    OT_UNUSED_VARIABLE(aIndex);

    uint32_t *maxValue = static_cast<uint32_t *>(aContext);

    VerifyOrQuit(maxValue != nullptr);
    *maxValue = Max<uint32_t>(*maxValue, aEntry.mValue);

    return true; /* shouldContinue */
}

void TestMsgBackedArray(void)
{
    static constexpr uint16_t kMaxSize = 4;

    Instance                           *instance = testInitInstance();
    MessageBackedArray<Entry, kMaxSize> array(*instance);
    Entry                               entry;
    Entry                               entry1(false, 0x1234, "Entry A");
    Entry                               entry2(false, 0x5678, "Second Entry");
    Entry                               entry3(true, 0xfedc, "");
    Entry                               entry4(true, 0x0, "4");
    Entry                               entry5(true, 0x9876, "Replace");
    Context                             context;
    uint32_t                            maxValue;

    // Initial state when array is empty

    VerifyOrQuit(array.GetLength() == 0);
    VerifyOrQuit(array.ReadAt(0, entry) == kErrorNotFound);
    VerifyOrQuit(array.WriteAt(0, entry1) == kErrorInvalidArgs);

    context.Reset();
    array.Iterate(FindFirstTrueFlag, &context);
    VerifyOrQuit(context.mIndex == kInvalidIndex);

    context.Reset();
    array.Iterate(FindLastTrueFlag, &context);
    VerifyOrQuit(context.mIndex == kInvalidIndex);

    // Array with one entry

    SuccessOrQuit(array.Push(entry1));
    VerifyOrQuit(array.GetLength() == 1);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry1);
    VerifyOrQuit(array.ReadAt(1, entry) == kErrorNotFound);

    context.Reset();
    array.Iterate(FindFirstTrueFlag, &context);
    VerifyOrQuit(context.mIndex == kInvalidIndex);

    context.Reset();
    array.Iterate(FindLastTrueFlag, &context);
    VerifyOrQuit(context.mIndex == kInvalidIndex);

    maxValue = 0;
    array.Iterate(FindMaxValue, &maxValue);
    VerifyOrQuit(maxValue == entry1.mValue);

    // Array with four entries.

    SuccessOrQuit(array.Push(entry2));
    SuccessOrQuit(array.Push(entry3));
    SuccessOrQuit(array.Push(entry4));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry1);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry2);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry3);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry4);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    context.Reset();
    array.Iterate(FindFirstTrueFlag, &context);
    VerifyOrQuit(context.mIndex == 2);
    VerifyOrQuit(context.mEntry == entry3);

    context.Reset();
    array.Iterate(FindLastTrueFlag, &context);
    VerifyOrQuit(context.mIndex == 3);
    VerifyOrQuit(context.mEntry == entry4);

    maxValue = 0;
    array.Iterate(FindMaxValue, &maxValue);
    VerifyOrQuit(maxValue == entry3.mValue);

    // Overwrite entry at index 1

    SuccessOrQuit(array.WriteAt(1, entry5));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry1);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry5);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry3);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry4);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    // Overwrite last entry

    SuccessOrQuit(array.WriteAt(3, entry2));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry1);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry5);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry3);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry2);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    // Overwrite out of bound

    VerifyOrQuit(array.WriteAt(4, entry5) == kErrorInvalidArgs);

    // Array at its max size

    VerifyOrQuit(array.Push(entry5) == kErrorNoBufs);

    // Clearing array

    array.Clear();

    VerifyOrQuit(array.GetLength() == 0);
    VerifyOrQuit(array.ReadAt(0, entry) == kErrorNotFound);
    VerifyOrQuit(array.WriteAt(0, entry1) == kErrorInvalidArgs);

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestMsgBackedArray();

    printf("All tests passed\n");
    return 0;
}
