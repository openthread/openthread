/*
 *  Copyright (c) 2019-26, The OpenThread Authors.
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

/**
 * @file
 *   This file implements a generic singly linked list.
 */

#include "linked_list.hpp"

#include "common/code_utils.hpp"

namespace ot {

LinkedListBase::LinkedListBase(const NextGetter aNextGetter, const NextSetter aNextSetter)
    : mHead(nullptr)
    , mNextGetter(aNextGetter)
    , mNextSetter(aNextSetter)
{
}

void *LinkedListBase::GetNext(void *aEntry) { return AsNonConst(mNextGetter(AsConst(aEntry))); }

const void *LinkedListBase::GetNext(const void *aEntry) const { return mNextGetter(AsNonConst(aEntry)); }

void LinkedListBase::SetNext(void *aEntry, void *aNext) { mNextSetter(aEntry, aNext); }

void LinkedListBase::Push(void *aEntry)
{
    SetNext(aEntry, mHead);
    mHead = aEntry;
}

const void *LinkedListBase::GetTail(void) const
{
    const void *tail = mHead;
    const void *next;

    VerifyOrExit(tail != nullptr);

    while (true)
    {
        next = GetNext(tail);
        VerifyOrExit(next != nullptr);
        tail = next;
    }

exit:
    return tail;
}

void LinkedListBase::PushAfter(void *aEntry, void *aPrev)
{
    SetNext(aEntry, GetNext(aPrev));
    SetNext(aPrev, aEntry);
}

void LinkedListBase::PushAfterTail(void *aEntry)
{
    void *tail = GetTail();

    if (tail == nullptr)
    {
        Push(aEntry);
    }
    else
    {
        PushAfter(aEntry, tail);
    }
}

void *LinkedListBase::Pop(void)
{
    void *entry = mHead;

    VerifyOrExit(entry != nullptr);
    mHead = GetNext(mHead);

exit:
    return entry;
}

void *LinkedListBase::PopAfter(void *aPrev)
{
    void *entry;

    if (aPrev == nullptr)
    {
        entry = Pop();
        ExitNow();
    }

    entry = GetNext(aPrev);

    VerifyOrExit(entry != nullptr);
    SetNext(aPrev, GetNext(entry));

exit:
    return entry;
}

bool LinkedListBase::Contains(const void *aEntry) const
{
    const void *prev;

    return Find(aEntry, prev) == kErrorNone;
}

Error LinkedListBase::Find(const void *aEntry, const void *&aPrev) const
{
    Error error = kErrorNotFound;

    aPrev = nullptr;

    for (const void *entry = mHead; entry != nullptr; aPrev = entry, entry = GetNext(entry))
    {
        if (entry == aEntry)
        {
            error = kErrorNone;
            break;
        }
    }

    return error;
}

Error LinkedListBase::Find(const void *aEntry, void *&aPrev)
{
    Error       error;
    const void *prev;

    error = AsConst(this)->Find(aEntry, prev);
    aPrev = AsNonConst(prev);

    return error;
}

Error LinkedListBase::Add(void *aEntry)
{
    Error error = kErrorNone;

    VerifyOrExit(!Contains(aEntry), error = kErrorAlready);
    Push(aEntry);

exit:
    return error;
}

Error LinkedListBase::Remove(const void *aEntry)
{
    void *prev;
    Error error;

    SuccessOrExit(error = Find(aEntry, prev));
    PopAfter(prev);

exit:
    return error;
}

uint32_t LinkedListBase::CountAllEntries(void) const
{
    uint32_t count = 0;

    for (const void *entry = mHead; entry != nullptr; entry = GetNext(entry))
    {
        count++;
    }

    return count;
}

const void *LinkedListBase::FindMatching(Matcher aMatcher, const void *aArg) const
{
    const void *prev;

    return FindMatchingWithPrev(prev, aMatcher, aArg);
}

const void *LinkedListBase::FindMatchingWithPrev(const void *&aPrev, Matcher aMatcher, const void *aArg) const
{
    const void *entry;

    aPrev = nullptr;

    for (entry = mHead; entry != nullptr; aPrev = entry, entry = GetNext(entry))
    {
        if (aMatcher(entry, aArg))
        {
            break;
        }
    }

    return entry;
}

void *LinkedListBase::RemoveMatching(Matcher aMatcher, const void *aArg)
{
    const void *entry;
    const void *prev;

    entry = FindMatchingWithPrev(prev, aMatcher, aArg);

    VerifyOrExit(entry != nullptr);
    PopAfter(AsNonConst(prev));

exit:
    return AsNonConst(entry);
}

void LinkedListBase::RemoveAllMatching(LinkedListBase &aRemovedList, Matcher aMatcher, const void *aArg)
{
    void *entry;
    void *prev;
    void *next;

    for (prev = nullptr, entry = mHead; entry != nullptr; entry = next)
    {
        next = GetNext(entry);

        if (aMatcher(entry, aArg))
        {
            PopAfter(prev);
            aRemovedList.Push(entry);

            // When the entry is removed from the list
            // we keep the `prev` pointer same as before.
        }
        else
        {
            prev = entry;
        }
    }
}

const void *LinkedListBase::FindMatching(Matcher2 aMatcher, const void *aArg1, const void *aArg2) const
{
    const void *prev;

    return FindMatchingWithPrev(prev, aMatcher, aArg1, aArg2);
}

const void *LinkedListBase::FindMatchingWithPrev(const void *&aPrev,
                                                 Matcher2     aMatcher,
                                                 const void  *aArg1,
                                                 const void  *aArg2) const
{
    const void *entry;

    aPrev = nullptr;

    for (entry = mHead; entry != nullptr; aPrev = entry, entry = GetNext(entry))
    {
        if (aMatcher(entry, aArg1, aArg2))
        {
            break;
        }
    }

    return entry;
}

void *LinkedListBase::RemoveMatching(Matcher2 aMatcher, const void *aArg1, const void *aArg2)
{
    const void *entry;
    const void *prev;

    entry = FindMatchingWithPrev(prev, aMatcher, aArg1, aArg2);

    VerifyOrExit(entry != nullptr);
    PopAfter(AsNonConst(prev));

exit:
    return AsNonConst(entry);
}

void LinkedListBase::RemoveAllMatching(LinkedListBase &aRemovedList,
                                       Matcher2        aMatcher,
                                       const void     *aArg1,
                                       const void     *aArg2)
{
    void *entry;
    void *prev;
    void *next;

    for (prev = nullptr, entry = mHead; entry != nullptr; entry = next)
    {
        next = GetNext(entry);

        if (aMatcher(entry, aArg1, aArg2))
        {
            PopAfter(prev);
            aRemovedList.Push(entry);

            // When the entry is removed from the list
            // we keep the `prev` pointer same as before.
        }
        else
        {
            prev = entry;
        }
    }
}

} // namespace ot
