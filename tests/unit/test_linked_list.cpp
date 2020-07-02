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

#include <stdarg.h>
#include <string.h>

#include "test_platform.h"

#include <openthread/config.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/linked_list.hpp"

#include "test_util.h"

struct EntryBase
{
    EntryBase *mNext;
};

struct Entry : public EntryBase, ot::LinkedListEntry<Entry>
{
public:
    Entry(const char *aName, uint16_t aId)
        : mName(aName)
        , mId(aId)
    {
    }

    const char *GetName(void) const { return mName; }
    uint16_t    GetId(void) const { return mId; }
    bool        Matches(const char *aName) const { return strcmp(mName, aName) == 0; }
    bool        Matches(uint16_t aId) const { return mId == aId; }

private:
    const char *mName;
    uint16_t    mId;
};

// This function verifies the content of the linked list matches a given list of entries.
void VerifyLinkedListContent(const ot::LinkedList<Entry> *aList, ...)
{
    va_list      args;
    Entry *      argEntry;
    Entry *      argPrev = nullptr;
    const Entry *prev;
    uint16_t     unusedId = 100;

    va_start(args, aList);

    for (const Entry *entry = aList->GetHead(); entry; entry = entry->GetNext())
    {
        argEntry = va_arg(args, Entry *);
        VerifyOrQuit(argEntry != nullptr, "List contains more entries than expected");
        VerifyOrQuit(argEntry == entry, "List does not contain the same entry");
        VerifyOrQuit(aList->Contains(*argEntry), "List::Contains() failed");
        VerifyOrQuit(aList->ContainsMatching(argEntry->GetName()), "List::ContainsMatching() failed");
        VerifyOrQuit(aList->ContainsMatching(argEntry->GetId()), "List::ContainsMatching() failed");

        SuccessOrQuit(aList->Find(*argEntry, prev), "List::Find() failed");
        VerifyOrQuit(prev == argPrev, "List::Find() returned prev entry is incorrect");

        VerifyOrQuit(aList->FindMatching(argEntry->GetName(), prev) == argEntry, "List::FindMatching() failed");
        VerifyOrQuit(prev == argPrev, "List::FindMatching() returned prev entry is incorrect");

        VerifyOrQuit(aList->FindMatching(argEntry->GetId(), prev) == argEntry, "List::FindMatching() failed");
        VerifyOrQuit(prev == argPrev, "List::FindMatching() returned prev entry is incorrect");

        argPrev = argEntry;
    }

    argEntry = va_arg(args, Entry *);
    VerifyOrQuit(argEntry == nullptr, "List contains less entries than expected");

    VerifyOrQuit(aList->GetTail() == argPrev, "List::GetTail() failed");

    VerifyOrQuit(!aList->ContainsMatching("none"), "List::ContainsMatching() succeeded for a missing entry");
    VerifyOrQuit(!aList->ContainsMatching(unusedId), "List::ContainsMatching() succeeded for a missing entry");

    VerifyOrQuit(aList->FindMatching("none", prev) == nullptr,
                 "LinkedList::FindMatching() succeeded for a missing entry");
    VerifyOrQuit(aList->FindMatching(unusedId, prev) == nullptr,
                 "LinkedList::FindMatching() succeeded for a missing entry");
}

void TestLinkedList(void)
{
    Entry                 a("a", 1), b("b", 2), c("c", 3), d("d", 4), e("e", 5);
    Entry *               prev;
    ot::LinkedList<Entry> list;

    VerifyOrQuit(list.IsEmpty(), "LinkedList::IsEmpty() failed after init");
    VerifyOrQuit(list.GetHead() == nullptr, "LinkedList::GetHead() failed after init");
    VerifyOrQuit(list.Pop() == nullptr, "LinkedList::Pop() failed when empty");
    VerifyOrQuit(list.Find(a, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    VerifyLinkedListContent(&list, nullptr);

    list.Push(a);
    VerifyOrQuit(!list.IsEmpty(), "LinkedList::IsEmpty() failed");
    VerifyLinkedListContent(&list, &a, nullptr);
    VerifyOrQuit(list.Find(b, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    SuccessOrQuit(list.Add(b), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &b, &a, nullptr);
    VerifyOrQuit(list.Find(c, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    list.Push(c);
    VerifyLinkedListContent(&list, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Add(d), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &d, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Add(e), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    VerifyOrQuit(list.Add(a) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(b) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(d) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(e) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");

    VerifyOrQuit(list.Pop() == &e, "LinkedList::Pop() failed");
    VerifyLinkedListContent(&list, &d, &c, &b, &a, nullptr);
    VerifyOrQuit(list.Find(e, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    VerifyOrQuit(list.FindMatching(d.GetName(), prev) == &d, "List::FindMatching() failed");
    VerifyOrQuit(prev == nullptr, "List::FindMatching() failed");
    VerifyOrQuit(list.FindMatching(c.GetId(), prev) == &c, "List::FindMatching() failed");
    VerifyOrQuit(prev == &d, "List::FindMatching() failed");
    VerifyOrQuit(list.FindMatching(b.GetName(), prev) == &b, "List::FindMatching() failed");
    VerifyOrQuit(prev == &c, "List::FindMatching() failed");
    VerifyOrQuit(list.FindMatching(a.GetId(), prev) == &a, "List::FindMatching() failed");
    VerifyOrQuit(prev == &b, "List::FindMatching() failed");
    VerifyOrQuit(list.FindMatching(e.GetId(), prev) == nullptr, "List::FindMatching() succeeded for a missing entry");
    VerifyOrQuit(list.FindMatching(e.GetName(), prev) == nullptr, "List::FindMatching() succeeded for a missing entry");

    list.SetHead(&e);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Remove(c), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, &a, nullptr);

    VerifyOrQuit(list.Remove(c) == OT_ERROR_NOT_FOUND, "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, &a, nullptr);
    VerifyOrQuit(list.Find(c, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    SuccessOrQuit(list.Remove(e), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &d, &b, &a, nullptr);
    VerifyOrQuit(list.Find(e, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    SuccessOrQuit(list.Remove(a), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &d, &b, nullptr);
    VerifyOrQuit(list.Find(a, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");

    list.Push(a);
    list.Push(c);
    list.Push(e);
    VerifyLinkedListContent(&list, &e, &c, &a, &d, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&a) == &d, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &c, &a, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&b) == nullptr, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &c, &a, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&e) == &c, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &a, &b, nullptr);

    list.PushAfter(c, b);
    VerifyLinkedListContent(&list, &e, &a, &b, &c, nullptr);

    list.PushAfter(d, a);
    VerifyLinkedListContent(&list, &e, &a, &d, &b, &c, nullptr);

    VerifyOrQuit(list.PopAfter(nullptr) == &e, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &a, &d, &b, &c, nullptr);

    VerifyOrQuit(list.PopAfter(nullptr) == &a, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &d, &b, &c, nullptr);

    list.Push(e);
    list.Push(a);
    VerifyLinkedListContent(&list, &a, &e, &d, &b, &c, nullptr);

    VerifyOrQuit(list.RemoveMatching(a.GetName()) == &a, "LinkedList::RemoveMatching() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, &c, nullptr);

    VerifyOrQuit(list.RemoveMatching(c.GetId()) == &c, "LinkedList::RemoveMatching() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, nullptr);

    VerifyOrQuit(list.RemoveMatching(c.GetId()) == nullptr, "LinkedList::RemoveMatching() succeeded for missing entry");
    VerifyOrQuit(list.RemoveMatching(a.GetName()) == nullptr,
                 "LinkedList::RemoveMatching() succeeded for missing entry");

    VerifyOrQuit(list.RemoveMatching(d.GetId()) == &d, "LinkedList::RemoveMatching() failed");
    VerifyLinkedListContent(&list, &e, &b, nullptr);

    list.Clear();
    VerifyOrQuit(list.IsEmpty(), "LinkedList::IsEmpty() failed after Clear()");
    VerifyOrQuit(list.PopAfter(nullptr) == nullptr, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, nullptr);
    VerifyOrQuit(list.Find(a, prev) == OT_ERROR_NOT_FOUND, "LinkedList::Find() succeeded for a missing entry");
    VerifyOrQuit(list.FindMatching(b.GetName(), prev) == nullptr, "LinkedList::FindMatching() succeeded when empty");
    VerifyOrQuit(list.FindMatching(c.GetId(), prev) == nullptr, "LinkedList::FindMatching() succeeded when empty");
    VerifyOrQuit(list.RemoveMatching(a.GetName()) == nullptr, "LinkedList::RemoveMatching() succeeded when empty");
    VerifyOrQuit(list.Remove(a) == OT_ERROR_NOT_FOUND, "LinkedList::Remove() succeeded when empty");
}

int main(void)
{
    TestLinkedList();
    printf("All tests passed\n");
    return 0;
}
