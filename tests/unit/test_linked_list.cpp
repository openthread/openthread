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
#include "common/linked_list.hpp"
#include "common/owning_list.hpp"
#include "instance/instance.hpp"

#include "test_util.h"

namespace ot {

struct EntryBase
{
    EntryBase *mNext;
};

struct Entry : public EntryBase, LinkedListEntry<Entry>
{
public:
    enum class Type : uint8_t
    {
        kAlpha,
        kBeta,
    };

    Entry(const char *aName, uint16_t aId, Type aType = Type::kAlpha)
        : mName(aName)
        , mId(aId)
        , mType(aType)
        , mWasFreed(false)
    {
    }

    const char *GetName(void) const { return mName; }
    uint16_t    GetId(void) const { return mId; }
    bool        Matches(const char *aName) const { return strcmp(mName, aName) == 0; }
    bool        Matches(uint16_t aId) const { return mId == aId; }
    bool        Matches(Type aType) const { return mType == aType; }
    void        Free(void) { mWasFreed = true; }

    void ResetTestFlags(void) { mWasFreed = false; }
    bool WasFreed(void) const { return mWasFreed; }

private:
    const char *mName;
    uint16_t    mId;
    Type        mType;
    bool        mWasFreed;
};

constexpr Entry::Type kAlphaType = Entry::Type::kAlpha;
constexpr Entry::Type kBetaType  = Entry::Type::kBeta;

// This function verifies the content of the linked list matches a given list of entries.
void VerifyLinkedListContent(const LinkedList<Entry> *aList, ...)
{
    va_list      args;
    Entry       *argEntry;
    Entry       *argPrev = nullptr;
    const Entry *prev;
    uint16_t     unusedId = 100;

    va_start(args, aList);

    for (const Entry &entry : *aList)
    {
        argEntry = va_arg(args, Entry *);
        VerifyOrQuit(argEntry != nullptr, "List contains more entries than expected");
        VerifyOrQuit(argEntry == &entry, "List does not contain the same entry");
        VerifyOrQuit(aList->Contains(*argEntry));
        VerifyOrQuit(aList->ContainsMatching(argEntry->GetName()));
        VerifyOrQuit(aList->ContainsMatching(argEntry->GetId()));

        SuccessOrQuit(aList->Find(*argEntry, prev));
        VerifyOrQuit(prev == argPrev, "List::Find() returned prev entry is incorrect");

        VerifyOrQuit(aList->FindMatching(argEntry->GetName(), prev) == argEntry);
        VerifyOrQuit(prev == argPrev, "List::FindMatching() returned prev entry is incorrect");

        VerifyOrQuit(aList->FindMatching(argEntry->GetId(), prev) == argEntry);
        VerifyOrQuit(prev == argPrev, "List::FindMatching() returned prev entry is incorrect");

        VerifyOrQuit(!argEntry->WasFreed());

        argPrev = argEntry;
    }

    argEntry = va_arg(args, Entry *);
    VerifyOrQuit(argEntry == nullptr, "List contains less entries than expected");

    VerifyOrQuit(aList->GetTail() == argPrev);

    VerifyOrQuit(!aList->ContainsMatching("none"), "succeeded for a missing entry");
    VerifyOrQuit(!aList->ContainsMatching(unusedId), "succeeded for a missing entry");

    VerifyOrQuit(aList->FindMatching("none", prev) == nullptr, "succeeded for a missing entry");
    VerifyOrQuit(aList->FindMatching(unusedId, prev) == nullptr, "succeeded for a missing entry");
}

void TestLinkedList(void)
{
    Entry             a("a", 1, kAlphaType), b("b", 2, kAlphaType), c("c", 3, kBetaType);
    Entry             d("d", 4, kBetaType), e("e", 5, kAlphaType), f("f", 6, kBetaType);
    Entry            *prev;
    LinkedList<Entry> list;
    LinkedList<Entry> removedList;

    printf("TestLinkedList\n");

    VerifyOrQuit(list.IsEmpty(), "failed after init");
    VerifyOrQuit(list.GetHead() == nullptr, "failed after init");
    VerifyOrQuit(list.Pop() == nullptr, "failed when empty");
    VerifyOrQuit(list.Find(a, prev) == kErrorNotFound, "succeeded when empty");

    VerifyLinkedListContent(&list, nullptr);

    list.Push(a);
    VerifyOrQuit(!list.IsEmpty());
    VerifyLinkedListContent(&list, &a, nullptr);
    VerifyOrQuit(list.Find(b, prev) == kErrorNotFound, "succeeded for a missing entry");

    SuccessOrQuit(list.Add(b));
    VerifyLinkedListContent(&list, &b, &a, nullptr);
    VerifyOrQuit(list.Find(c, prev) == kErrorNotFound, "succeeded for a missing entry");

    list.Push(c);
    VerifyLinkedListContent(&list, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Add(d));
    VerifyLinkedListContent(&list, &d, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Add(e));
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    VerifyOrQuit(list.Add(a) == kErrorAlready, "did not detect duplicate");
    VerifyOrQuit(list.Add(b) == kErrorAlready, "did not detect duplicate");
    VerifyOrQuit(list.Add(d) == kErrorAlready, "did not detect duplicate");
    VerifyOrQuit(list.Add(e) == kErrorAlready, "did not detect duplicate");

    VerifyOrQuit(list.Pop() == &e);
    VerifyLinkedListContent(&list, &d, &c, &b, &a, nullptr);
    VerifyOrQuit(list.Find(e, prev) == kErrorNotFound, "succeeded for a missing entry");

    VerifyOrQuit(list.FindMatching(d.GetName(), prev) == &d);
    VerifyOrQuit(prev == nullptr);
    VerifyOrQuit(list.FindMatching(c.GetId(), prev) == &c);
    VerifyOrQuit(prev == &d);
    VerifyOrQuit(list.FindMatching(b.GetName(), prev) == &b);
    VerifyOrQuit(prev == &c);
    VerifyOrQuit(list.FindMatching(a.GetId(), prev) == &a);
    VerifyOrQuit(prev == &b);
    VerifyOrQuit(list.FindMatching(e.GetId(), prev) == nullptr, "succeeded for a missing entry");
    VerifyOrQuit(list.FindMatching(e.GetName(), prev) == nullptr, "succeeded for a missing entry");

    list.SetHead(&e);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    SuccessOrQuit(list.Remove(c));
    VerifyLinkedListContent(&list, &e, &d, &b, &a, nullptr);

    VerifyOrQuit(list.Remove(c) == kErrorNotFound);
    VerifyLinkedListContent(&list, &e, &d, &b, &a, nullptr);
    VerifyOrQuit(list.Find(c, prev) == kErrorNotFound, "succeeded for a missing entry");

    SuccessOrQuit(list.Remove(e));
    VerifyLinkedListContent(&list, &d, &b, &a, nullptr);
    VerifyOrQuit(list.Find(e, prev) == kErrorNotFound, "succeeded for a missing entry");

    SuccessOrQuit(list.Remove(a));
    VerifyLinkedListContent(&list, &d, &b, nullptr);
    VerifyOrQuit(list.Find(a, prev) == kErrorNotFound, "succeeded for a missing entry");

    list.Push(a);
    list.Push(c);
    list.Push(e);
    VerifyLinkedListContent(&list, &e, &c, &a, &d, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&a) == &d);
    VerifyLinkedListContent(&list, &e, &c, &a, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&b) == nullptr);
    VerifyLinkedListContent(&list, &e, &c, &a, &b, nullptr);

    VerifyOrQuit(list.PopAfter(&e) == &c);
    VerifyLinkedListContent(&list, &e, &a, &b, nullptr);

    list.PushAfter(c, b);
    VerifyLinkedListContent(&list, &e, &a, &b, &c, nullptr);

    list.PushAfter(d, a);
    VerifyLinkedListContent(&list, &e, &a, &d, &b, &c, nullptr);

    VerifyOrQuit(list.PopAfter(nullptr) == &e);
    VerifyLinkedListContent(&list, &a, &d, &b, &c, nullptr);

    VerifyOrQuit(list.PopAfter(nullptr) == &a);
    VerifyLinkedListContent(&list, &d, &b, &c, nullptr);

    list.Push(e);
    list.Push(a);
    VerifyLinkedListContent(&list, &a, &e, &d, &b, &c, nullptr);

    VerifyOrQuit(list.RemoveMatching(a.GetName()) == &a);
    VerifyLinkedListContent(&list, &e, &d, &b, &c, nullptr);

    VerifyOrQuit(list.RemoveMatching(c.GetId()) == &c);
    VerifyLinkedListContent(&list, &e, &d, &b, nullptr);

    VerifyOrQuit(list.RemoveMatching(c.GetId()) == nullptr, "succeeded for missing entry");
    VerifyOrQuit(list.RemoveMatching(a.GetName()) == nullptr, "succeeded for missing entry");

    VerifyOrQuit(list.RemoveMatching(d.GetId()) == &d);
    VerifyLinkedListContent(&list, &e, &b, nullptr);

    list.Clear();
    VerifyOrQuit(list.IsEmpty(), "failed after Clear()");
    VerifyOrQuit(list.PopAfter(nullptr) == nullptr);
    VerifyLinkedListContent(&list, nullptr);
    VerifyOrQuit(list.Find(a, prev) == kErrorNotFound, "succeeded for a missing entry");
    VerifyOrQuit(list.FindMatching(b.GetName(), prev) == nullptr, "succeeded when empty");
    VerifyOrQuit(list.FindMatching(c.GetId(), prev) == nullptr, "succeeded when empty");
    VerifyOrQuit(list.RemoveMatching(a.GetName()) == nullptr, "succeeded when empty");
    VerifyOrQuit(list.Remove(a) == kErrorNotFound, "succeeded when empty");

    list.Clear();
    removedList.Clear();
    list.Push(f);
    list.Push(e);
    list.Push(d);
    list.Push(c);
    list.Push(b);
    list.Push(a);
    VerifyLinkedListContent(&list, &a, &b, &c, &d, &e, &f, nullptr);

    list.RemoveAllMatching(kAlphaType, removedList);
    VerifyLinkedListContent(&list, &c, &d, &f, nullptr);
    VerifyLinkedListContent(&removedList, &e, &b, &a, nullptr);

    removedList.Clear();
    list.RemoveAllMatching(kAlphaType, removedList);
    VerifyLinkedListContent(&list, &c, &d, &f, nullptr);
    VerifyOrQuit(removedList.IsEmpty());

    list.RemoveAllMatching(kBetaType, removedList);
    VerifyOrQuit(list.IsEmpty());
    VerifyLinkedListContent(&removedList, &f, &d, &c, nullptr);

    removedList.Clear();
    list.RemoveAllMatching(kAlphaType, removedList);
    VerifyOrQuit(list.IsEmpty());
    VerifyOrQuit(removedList.IsEmpty());

    list.Push(f);
    list.Push(e);
    list.Push(d);
    list.Push(c);
    list.Push(b);
    list.Push(a);
    VerifyLinkedListContent(&list, &a, &b, &c, &d, &e, &f, nullptr);

    list.RemoveAllMatching(kBetaType, removedList);
    VerifyLinkedListContent(&list, &a, &b, &e, nullptr);
    VerifyLinkedListContent(&removedList, &f, &d, &c, nullptr);

    list.Clear();
    list.PushAfterTail(a);
    VerifyLinkedListContent(&list, &a, nullptr);
    list.PushAfterTail(b);
    VerifyLinkedListContent(&list, &a, &b, nullptr);
    list.PushAfterTail(c);
    VerifyLinkedListContent(&list, &a, &b, &c, nullptr);
    list.PushAfterTail(d);
    VerifyLinkedListContent(&list, &a, &b, &c, &d, nullptr);
}

void TestOwningList(void)
{
    Entry             a("a", 1, kAlphaType), b("b", 2, kAlphaType), c("c", 3, kBetaType);
    Entry             d("d", 4, kBetaType), e("e", 5, kAlphaType), f("f", 6, kBetaType);
    OwningList<Entry> list;
    OwningList<Entry> removedList;
    OwnedPtr<Entry>   ptr;

    printf("TestOwningList\n");

    VerifyOrQuit(list.IsEmpty());
    VerifyOrQuit(list.GetHead() == nullptr);
    VerifyOrQuit(list.Pop().IsNull());

    list.Free();
    VerifyOrQuit(list.IsEmpty());
    VerifyOrQuit(list.GetHead() == nullptr);
    VerifyOrQuit(list.Pop().IsNull());

    // Clear()

    list.Push(a);
    VerifyLinkedListContent(&list, &a, nullptr);
    list.Free();
    VerifyOrQuit(list.IsEmpty());
    VerifyOrQuit(a.WasFreed());

    // Test removing entry without taking back the ownership

    a.ResetTestFlags();
    list.Push(a);
    list.Push(b);
    list.Push(c);
    list.Push(d);
    list.Push(e);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    list.Pop();
    VerifyLinkedListContent(&list, &d, &c, &b, &a, nullptr);
    VerifyOrQuit(e.WasFreed());

    list.PopAfter(&c);
    VerifyLinkedListContent(&list, &d, &c, &a, nullptr);
    VerifyOrQuit(b.WasFreed());

    list.RemoveMatching("c");
    VerifyLinkedListContent(&list, &d, &a, nullptr);
    VerifyOrQuit(c.WasFreed());

    list.Free();
    VerifyLinkedListContent(&list, nullptr);
    VerifyOrQuit(d.WasFreed());
    VerifyOrQuit(a.WasFreed());

    // Test removing entry and taking ownership

    a.ResetTestFlags();
    b.ResetTestFlags();
    c.ResetTestFlags();
    d.ResetTestFlags();
    e.ResetTestFlags();
    list.Push(a);
    list.Push(b);
    list.Push(c);
    list.Push(d);
    list.Push(e);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);

    ptr = list.PopAfter(&a);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, nullptr);
    VerifyOrQuit(ptr.IsNull());

    ptr = list.PopAfter(&e);
    VerifyLinkedListContent(&list, &e, &c, &b, &a, nullptr);
    VerifyOrQuit(ptr.Get() == &d);
    VerifyOrQuit(!d.WasFreed());

    ptr = list.Pop();
    VerifyLinkedListContent(&list, &c, &b, &a, nullptr);
    VerifyOrQuit(ptr.Get() == &e);
    VerifyOrQuit(!e.WasFreed());
    VerifyOrQuit(d.WasFreed());

    ptr = list.RemoveMatching<uint8_t>(1);
    VerifyLinkedListContent(&list, &c, &b, nullptr);
    VerifyOrQuit(ptr.Get() == &a);
    VerifyOrQuit(!a.WasFreed());
    VerifyOrQuit(e.WasFreed());

    list.Clear();
    VerifyOrQuit(c.WasFreed());
    VerifyOrQuit(b.WasFreed());
    VerifyOrQuit(!a.WasFreed());
    a.Free();
    VerifyOrQuit(a.WasFreed());

    // Test `RemoveAllMatching()`

    a.ResetTestFlags();
    b.ResetTestFlags();
    c.ResetTestFlags();
    d.ResetTestFlags();
    e.ResetTestFlags();
    f.ResetTestFlags();
    list.Push(a);
    list.Push(b);
    list.Push(c);
    list.Push(d);
    list.Push(e);
    list.Push(f);
    VerifyLinkedListContent(&list, &f, &e, &d, &c, &b, &a, nullptr);

    list.RemoveAllMatching(kAlphaType, removedList);
    VerifyLinkedListContent(&list, &f, &d, &c, nullptr);
    VerifyLinkedListContent(&removedList, &a, &b, &e, nullptr);
    VerifyOrQuit(!a.WasFreed());
    VerifyOrQuit(!c.WasFreed());

    removedList.Clear();
    list.RemoveAllMatching(kAlphaType, removedList);
    VerifyOrQuit(removedList.IsEmpty());
    VerifyLinkedListContent(&list, &f, &d, &c, nullptr);

    list.RemoveAllMatching(kBetaType, removedList);
    VerifyOrQuit(list.IsEmpty());
    VerifyLinkedListContent(&removedList, &c, &d, &f, nullptr);
    VerifyOrQuit(!c.WasFreed());
}

} // namespace ot

int main(void)
{
    ot::TestLinkedList();
    ot::TestOwningList();
    printf("All tests passed\n");
    return 0;
}
