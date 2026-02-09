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

    bool Matches(bool aFlag) const { return mFlag == aFlag; }
    bool Matches(uint16_t aValue) const { return mValue == aValue; }
    bool Matches(bool aFlag, uint16_t aValue) const { return Matches(aFlag) && Matches(aValue); }
    bool Matches(const char *aString) const { return StringMatch(mString, aString); }

    bool     mFlag;
    uint32_t mValue;
    char     mString[kStringSize];
};

void TestMsgBackedArray(void)
{
    static constexpr uint16_t kMaxSize = 4;

    using EntryArray = MessageBackedArray<Entry, kMaxSize>;

    Instance                *instance = testInitInstance();
    EntryArray               array(*instance);
    EntryArray::IndexedEntry entry;
    Entry                    entry0(false, 0x1234, "Entry A");
    Entry                    entry1(false, 0x5678, "Second Entry");
    Entry                    entry2(true, 0xfedc, "");
    Entry                    entry3(true, 0x1234, "4");
    Entry                    entry4(true, 0x9876, "Replace");

    // Initial state when array is empty

    VerifyOrQuit(array.GetLength() == 0);
    VerifyOrQuit(array.ReadAt(0, entry) == kErrorNotFound);
    VerifyOrQuit(array.WriteAt(0, entry0) == kErrorInvalidArgs);

    VerifyOrQuit(array.FindMatching(entry, true) == kErrorNotFound);

    entry.InitForIteration();
    VerifyOrQuit(array.ReadNext(entry) == kErrorNotFound);

    // Array with one entry

    SuccessOrQuit(array.Push(entry0));
    VerifyOrQuit(array.GetLength() == 1);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry0);
    VerifyOrQuit(array.ReadAt(1, entry) == kErrorNotFound);

    VerifyOrQuit(array.FindMatching(entry, true) == kErrorNotFound);

    SuccessOrQuit(array.FindMatching(entry, false));
    VerifyOrQuit(entry.GetIndex() == 0);
    VerifyOrQuit(entry == entry0);

    SuccessOrQuit(array.FindMatching(entry, "Entry A"));
    VerifyOrQuit(entry.GetIndex() == 0);
    VerifyOrQuit(entry == entry0);

    entry.InitForIteration();
    SuccessOrQuit(array.ReadNext(entry));
    VerifyOrQuit(entry.GetIndex() == 0);
    VerifyOrQuit(entry == entry0);
    VerifyOrQuit(array.ReadNext(entry) == kErrorNotFound);

    // Array with four entries.

    SuccessOrQuit(array.Push(entry1));
    SuccessOrQuit(array.Push(entry2));
    SuccessOrQuit(array.Push(entry3));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry0);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry1);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry2);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry3);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    SuccessOrQuit(array.FindMatching(entry, true));
    VerifyOrQuit(entry.GetIndex() == 2);
    VerifyOrQuit(entry == entry2);

    SuccessOrQuit(array.FindMatching(entry, false));
    VerifyOrQuit(entry.GetIndex() == 0);
    VerifyOrQuit(entry == entry0);

    SuccessOrQuit(array.FindMatching(entry, true, 0x1234));
    VerifyOrQuit(entry.GetIndex() == 3);
    VerifyOrQuit(entry == entry3);

    entry.InitForIteration();
    SuccessOrQuit(array.ReadNext(entry));
    VerifyOrQuit(entry.GetIndex() == 0);
    VerifyOrQuit(entry == entry0);
    SuccessOrQuit(array.ReadNext(entry));
    VerifyOrQuit(entry.GetIndex() == 1);
    VerifyOrQuit(entry == entry1);
    SuccessOrQuit(array.ReadNext(entry));
    VerifyOrQuit(entry.GetIndex() == 2);
    VerifyOrQuit(entry == entry2);
    SuccessOrQuit(array.ReadNext(entry));
    VerifyOrQuit(entry.GetIndex() == 3);
    VerifyOrQuit(entry == entry3);
    VerifyOrQuit(array.ReadNext(entry) == kErrorNotFound);

    // Overwrite entry at index 1

    SuccessOrQuit(array.WriteAt(1, entry4));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry0);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry4);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry2);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry3);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    // Overwrite last entry

    SuccessOrQuit(array.WriteAt(3, entry1));

    VerifyOrQuit(array.GetLength() == 4);
    SuccessOrQuit(array.ReadAt(0, entry));
    VerifyOrQuit(entry == entry0);
    SuccessOrQuit(array.ReadAt(1, entry));
    VerifyOrQuit(entry == entry4);
    SuccessOrQuit(array.ReadAt(2, entry));
    VerifyOrQuit(entry == entry2);
    SuccessOrQuit(array.ReadAt(3, entry));
    VerifyOrQuit(entry == entry1);
    VerifyOrQuit(array.ReadAt(4, entry) == kErrorNotFound);

    // Overwrite out of bound

    VerifyOrQuit(array.WriteAt(4, entry4) == kErrorInvalidArgs);

    // Array at its max size

    VerifyOrQuit(array.Push(entry4) == kErrorNoBufs);

    // Clearing array

    array.Clear();

    VerifyOrQuit(array.GetLength() == 0);
    VerifyOrQuit(array.ReadAt(0, entry) == kErrorNotFound);
    VerifyOrQuit(array.WriteAt(0, entry0) == kErrorInvalidArgs);

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestMsgBackedArray();

    printf("All tests passed\n");
    return 0;
}
