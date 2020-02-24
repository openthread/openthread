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
};

// This function verifies the content of the linked list matches a given list of entries.
void VerifyLinkedListContent(const ot::LinkedList<Entry> *aList, ...)
{
    va_list args;
    Entry * argEntry;
    Entry * argPrev = NULL;

    va_start(args, aList);

    for (const Entry *entry = aList->GetHead(); entry; entry = entry->GetNext())
    {
        Entry *prev;

        argEntry = va_arg(args, Entry *);
        VerifyOrQuit(argEntry != NULL, "List contains more entries than expected");
        VerifyOrQuit(argEntry == entry, "List does not contain the same entry");
        VerifyOrQuit(aList->Contains(*argEntry), "List::Contains() failed");

        SuccessOrQuit(aList->Find(*argEntry, prev), "List::Find() failed");
        VerifyOrQuit(prev == argPrev, "List::Find() returned prev entry is incorrect");

        argPrev = argEntry;
    }

    argEntry = va_arg(args, Entry *);
    VerifyOrQuit(argEntry == NULL, "List contains less entries than expected");

    VerifyOrQuit(aList->GetTail() == argPrev, "List::GetTail() failed");
}

void TestLinkedList(void)
{
    Entry                 a, b, c, d, e;
    ot::LinkedList<Entry> list;

    VerifyOrQuit(list.IsEmpty(), "LinkedList::IsEmpty() failed after init");
    VerifyOrQuit(list.GetHead() == NULL, "LinkedList::GetHead() failed after init");
    VerifyOrQuit(list.Pop() == NULL, "LinkedList::Pop() failed when empty");

    VerifyLinkedListContent(&list, NULL);

    list.Push(a);
    VerifyOrQuit(!list.IsEmpty(), "LinkedList::IsEmpty() failed");
    VerifyLinkedListContent(&list, &a, NULL);

    SuccessOrQuit(list.Add(b), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &b, &a, NULL);

    list.Push(c);
    VerifyLinkedListContent(&list, &c, &b, &a, NULL);

    SuccessOrQuit(list.Add(d), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &d, &c, &b, &a, NULL);

    SuccessOrQuit(list.Add(e), "LinkedList::Add() failed");
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, NULL);

    VerifyOrQuit(list.Add(a) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(b) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(d) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");
    VerifyOrQuit(list.Add(e) == OT_ERROR_ALREADY, "LinkedList::Add() did not detect duplicate");

    VerifyOrQuit(list.Pop() == &e, "LinkedList::Pop() failed");
    VerifyLinkedListContent(&list, &d, &c, &b, &a, NULL);

    list.SetHead(&e);
    VerifyLinkedListContent(&list, &e, &d, &c, &b, &a, NULL);

    SuccessOrQuit(list.Remove(c), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, &a, NULL);

    VerifyOrQuit(list.Remove(c) == OT_ERROR_NOT_FOUND, "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &e, &d, &b, &a, NULL);

    SuccessOrQuit(list.Remove(e), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &d, &b, &a, NULL);

    SuccessOrQuit(list.Remove(a), "LinkedList::Remove() failed");
    VerifyLinkedListContent(&list, &d, &b, NULL);

    list.Push(a);
    list.Push(c);
    list.Push(e);
    VerifyLinkedListContent(&list, &e, &c, &a, &d, &b, NULL);

    VerifyOrQuit(list.PopAfter(a) == &d, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &c, &a, &b, NULL);

    VerifyOrQuit(list.PopAfter(b) == NULL, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &c, &a, &b, NULL);

    VerifyOrQuit(list.PopAfter(e) == &c, "LinkedList::PopAfter() failed");
    VerifyLinkedListContent(&list, &e, &a, &b, NULL);

    list.PushAfter(c, b);
    VerifyLinkedListContent(&list, &e, &a, &b, &c, NULL);

    list.PushAfter(d, a);
    VerifyLinkedListContent(&list, &e, &a, &d, &b, &c, NULL);

    list.Clear();
    VerifyOrQuit(list.IsEmpty(), "LinkedList::IsEmpty() failed after Clear()");
    VerifyLinkedListContent(&list, NULL);
}

int main(void)
{
    TestLinkedList();
    printf("All tests passed\n");
    return 0;
}
