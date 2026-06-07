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

/**
 * @file
 *   This file includes definitions for a generic singly linked list.
 */

#ifndef OT_CORE_COMMON_LINKED_LIST_HPP_
#define OT_CORE_COMMON_LINKED_LIST_HPP_

#include "openthread-core-config.h"

#include <stdio.h>

#include "common/const_cast.hpp"
#include "common/error.hpp"
#include "common/iterator_utils.hpp"

namespace ot {

/**
 * @addtogroup core-linked-list
 *
 * @brief
 *   This module includes definitions for OpenThread Singly Linked List.
 *
 * @{
 */

class LinkedListBase
{
public:
    /**
     * Clears the linked list.
     */
    void Clear(void) { mHead = nullptr; }

    /**
     * Indicates whether the linked list is empty or not.
     *
     * @retval TRUE   If the linked list is empty.
     * @retval FALSE  If the linked list is not empty.
     */
    bool IsEmpty(void) const { return (mHead == nullptr); }

    /**
     * Counts and returns the number of entries in the linked list.
     *
     * @returns The number of entries in the linked list.
     */
    uint32_t CountAllEntries(void) const;

protected:
    typedef const void *(*NextGetter)(const void *aEntry);
    typedef void (*NextSetter)(void *aEntry, void *aNext);

    typedef bool (*Matcher)(const void *aEntry, const void *aArg);
    typedef bool (*Matcher2)(const void *aEntry, const void *Arg1, const void *aArg2);

    LinkedListBase(NextGetter aNextGetter, NextSetter aNextSetter);

    void       *GetHead(void) { return mHead; }
    const void *GetHead(void) const { return mHead; }
    void        SetHead(void *aHead) { mHead = aHead; }
    void       *GetTail(void) { return AsNonConst(AsConst(this)->GetTail()); }
    const void *GetTail(void) const;
    void        Push(void *aEntry);
    void        PushAfter(void *aEntry, void *aPrev);
    void        PushAfterTail(void *aEntry);
    void       *Pop(void);
    void       *PopAfter(void *aPrev);
    bool        Contains(const void *aEntry) const;
    Error       Find(const void *aEntry, const void *&aPrev) const;
    Error       Find(const void *aEntry, void *&aPrev);
    Error       Add(void *aEntry);
    Error       Remove(const void *aEntry);
    const void *FindMatching(Matcher aMatcher, const void *aArg) const;
    const void *FindMatchingWithPrev(const void *&aPrev, Matcher aMatcher, const void *aArg) const;
    void       *RemoveMatching(Matcher aMatcher, const void *aArg);
    void        RemoveAllMatching(LinkedListBase &aRemoveList, Matcher aMatcher, const void *aArg);

    const void *FindMatching(Matcher2 aMatcher, const void *aArg1, const void *aArg2) const;
    const void *FindMatchingWithPrev(const void *&aPrev, Matcher2 aMatcher, const void *aArg1, const void *aArg2) const;
    void       *RemoveMatching(Matcher2 aMatcher, const void *aArg1, const void *aArg2);
    void        RemoveAllMatching(LinkedListBase &aRemoveList, Matcher2 aMatcher, const void *aArg1, const void *aArg2);

private:
    void       *GetNext(void *aEntry);
    const void *GetNext(const void *aEntry) const;
    void        SetNext(void *aEntry, void *aNext);

    void      *mHead;
    NextGetter mNextGetter;
    NextSetter mNextSetter;
};

/**
 * Represents a linked list entry.
 *
 * Provides methods to `GetNext()` and `SetNext()` in the linked list entry.
 *
 * Users of this class should follow CRTP-style inheritance, i.e., the `Type` class itself should publicly inherit
 * from `LinkedListEntry<Type>`.
 *
 * The template type `Type` should contain a `mNext` member variable. The `mNext` should be of a type that can be
 * down-casted to `Type` itself.
 */
template <class Type> class LinkedListEntry
{
public:
    /**
     * Gets the next entry in the linked list.
     *
     * @returns A pointer to the next entry in the linked list or `nullptr` if at the end of the list.
     */
    const Type *GetNext(void) const { return static_cast<const Type *>(static_cast<const Type *>(this)->mNext); }

    /**
     * Gets the next entry in the linked list.
     *
     * @returns A pointer to the next entry in the linked list or `nullptr` if at the end of the list.
     */
    Type *GetNext(void) { return static_cast<Type *>(static_cast<Type *>(this)->mNext); }

    /**
     * Sets the next pointer on the entry.
     *
     * @param[in] aNext  A pointer to the next entry.
     */
    void SetNext(Type *aNext) { static_cast<Type *>(this)->mNext = aNext; }
};

/**
 * Represents a singly linked list.
 *
 * The template type `Type` should provide `GetNext()` and `SetNext()` methods (which can be realized by `Type`
 * inheriting from `LinkedListEntry<Type>` class).
 */
template <typename Type> class LinkedList : public LinkedListBase
{
    class Iterator;
    class ConstIterator;

public:
    /**
     * Initializes the linked list.
     */
    LinkedList(void)
        : LinkedListBase(GetNextEntry, SetNextEntry)
    {
    }

    /**
     * Returns the entry at the head of the linked list
     *
     * @returns Pointer to the entry at the head of the linked list, or `nullptr` if the list is empty.
     */
    Type *GetHead(void) { return AsEntry(LinkedListBase::GetHead()); }

    /**
     * Returns the entry at the head of the linked list.
     *
     * @returns Pointer to the entry at the head of the linked list, or `nullptr` if the list is empty.
     */
    const Type *GetHead(void) const { return AsEntry(LinkedListBase::GetHead()); }

    /**
     * Sets the head of the linked list to a given entry.
     *
     * @param[in] aHead   A pointer to an entry to set as the head of the linked list.
     */
    void SetHead(Type *aHead) { LinkedListBase::SetHead(aHead); }

    /**
     * Pushes an entry at the head of the linked list.
     *
     * @param[in] aEntry   A reference to an entry to push at the head of linked list.
     */
    void Push(Type &aEntry) { LinkedListBase::Push(&aEntry); }

    /**
     * Pushes an entry after a given previous existing entry in the linked list.
     *
     * @param[in] aEntry       A reference to an entry to push into the list.
     * @param[in] aPrevEntry   A reference to a previous entry (new entry @p aEntry will be pushed after this).
     */
    void PushAfter(Type &aEntry, Type &aPrevEntry) { LinkedListBase::PushAfter(&aEntry, &aPrevEntry); }

    /**
     * Pushes an entry after the tail in the linked list.
     *
     * @param[in] aEntry       A reference to an entry to push into the list.
     */
    void PushAfterTail(Type &aEntry) { LinkedListBase::PushAfterTail(&aEntry); }

    /**
     * Pops an entry from head of the linked list.
     *
     * @note This method does not change the popped entry itself, i.e., the popped entry next pointer stays as before.
     *
     * @returns The entry that was popped if the list is not empty, or `nullptr` if the list is empty.
     */
    Type *Pop(void) { return AsEntry(LinkedListBase::Pop()); }

    /**
     * Pops an entry after a given previous entry.
     *
     * @note This method does not change the popped entry itself, i.e., the popped entry next pointer stays as before.
     *
     * @param[in] aPrevEntry  A pointer to a previous entry. If it is not `nullptr` the entry after this will be popped,
     *                        otherwise (if it is `nullptr`) the entry at the head of the list is popped.
     *
     * @returns Pointer to the entry that was popped, or `nullptr` if there is no entry to pop.
     */
    Type *PopAfter(Type *aPrevEntry) { return AsEntry(LinkedListBase::PopAfter(aPrevEntry)); }

    /**
     * Indicates whether the linked list contains a given entry.
     *
     * @param[in] aEntry   A reference to an entry.
     *
     * @retval TRUE   The linked list contains @p aEntry.
     * @retval FALSE  The linked list does not contain @p aEntry.
     */
    bool Contains(const Type &aEntry) const { return LinkedListBase::Contains(&aEntry); }

    /**
     * Indicates whether the linked list contains an entry matching a set of conditions.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[in]  aArgs       The args to pass to `Matches()`.
     *
     * @retval TRUE   The linked list contains a matching entry.
     * @retval FALSE  The linked list does not contain a matching entry.
     */
    template <typename... Args> bool ContainsMatching(const Args &...aArgs) const
    {
        return FindMatching(aArgs...) != nullptr;
    }

    /**
     * Adds an entry (at the head of the linked list) if it is not already in the list.
     *
     * @param[in] aEntry   A reference to an entry to add.
     *
     * @retval kErrorNone     The entry was successfully added at the head of the list.
     * @retval kErrorAlready  The entry is already in the list.
     */
    Error Add(Type &aEntry) { return LinkedListBase::Add(&aEntry); }

    /**
     * Removes an entry from the linked list.
     *
     * @note This method does not change the removed entry @p aEntry itself (it is `const`), i.e., the entry next
     * pointer of @p aEntry stays as before.
     *
     * @param[in] aEntry   A reference to an entry to remove.
     *
     * @retval kErrorNone      The entry was successfully removed from the list.
     * @retval kErrorNotFound  Could not find the entry in the list.
     */
    Error Remove(const Type &aEntry) { return LinkedListBase::Remove(&aEntry); }

    /**
     * Removes an entry matching a given set of conditions from the linked list.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @note This method does not change the removed entry itself (which is returned in case of success), i.e., the
     * entry next pointer stays as before.
     *
     * @param[in]  aArgs       The args to pass to `Matches()`.
     *
     * @returns A pointer to the removed matching entry if one could be found, or `nullptr` if no matching entry is
     *          found.
     */
    template <typename Arg> Type *RemoveMatching(const Arg &aArg)
    {
        return AsEntry(LinkedListBase::RemoveMatching(MatchEntry<Arg>, &aArg));
    }

    template <typename Arg1, typename Arg2> Type *RemoveMatching(const Arg1 &aArg1, const Arg2 &aArg2)
    {
        return AsEntry(LinkedListBase::RemoveMatching(MatchEntry<Arg1, Arg2>, &aArg1, &aArg2));
    }

    /**
     * Removes all entries in the list matching a given entry indicator from the list and adds
     * them to a new list.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[in] aRemovedList The list to add the removed entries to.
     * @param[in]  aArgs       The args to pass to `Matches()`.
     */
    template <typename Arg> void RemoveAllMatching(LinkedList &aRemovedList, const Arg &aArg)
    {
        LinkedListBase::RemoveAllMatching(aRemovedList, MatchEntry<Arg>, &aArg);
    }

    template <typename Arg1, typename Arg2>
    void RemoveAllMatching(LinkedList &aRemovedList, const Arg1 &aArg1, const Arg2 &aArg2)
    {
        LinkedListBase::RemoveAllMatching(aRemovedList, MatchEntry<Arg1, Arg2>, &aArg1, &aArg2);
    }

    /**
     * Searches within the linked list to find an entry and if found returns a pointer to previous entry.
     *
     * @param[in]  aEntry      A reference to an entry to find.
     * @param[out] aPrevEntry  A pointer to output the previous entry on success (when @p aEntry is found in the list).
     *                         @p aPrevEntry is set to `nullptr` if @p aEntry is the head of the list. Otherwise it is
     *                         updated to point to the previous entry before @p aEntry in the list.
     *
     * @retval kErrorNone      The entry was found in the list and @p aPrevEntry was updated successfully.
     * @retval kErrorNotFound  The entry was not found in the list.
     */
    Error Find(const Type &aEntry, const Type *&aPrevEntry) const
    {
        Error       error;
        const void *prev;

        error      = LinkedListBase::Find(&aEntry, prev);
        aPrevEntry = AsEntry(prev);

        return error;
    }

    /**
     * Searches within the linked list to find an entry and if found returns a pointer to previous entry.
     *
     * @param[in]  aEntry      A reference to an entry to find.
     * @param[out] aPrevEntry  A pointer to output the previous entry on success (when @p aEntry is found in the list).
     *                         @p aPrevEntry is set to `nullptr` if @p aEntry is the head of the list. Otherwise it is
     *                         updated to point to the previous entry before @p aEntry in the list.
     *
     * @retval kErrorNone      The entry was found in the list and @p aPrevEntry was updated successfully.
     * @retval kErrorNotFound  The entry was not found in the list.
     */
    Error Find(const Type &aEntry, Type *&aPrevEntry)
    {
        return AsConst(this)->Find(aEntry, const_cast<const Type *&>(aPrevEntry));
    }

    /**
     * Searches within the linked list to find an entry matching a set of conditions, and if found also returns a
     * pointer to its previous entry in the list.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[out] aPrevEntry  A pointer to output the previous entry on success (when a match is found in the list).
     *                         @p aPrevEntry is set to `nullptr` if the matching entry is the head of the list.
     *                         Otherwise it is updated to point to the previous entry before the matching entry in the
     *                         list.
     * @param[in]  aArgs       The args to pass to `Matches()`.
     *
     * @returns A pointer to the matching entry if one is found, or `nullptr` if no matching entry was found.
     */
    template <typename Arg> const Type *FindMatchingWithPrev(const Type *&aPrevEntry, const Arg &aArg) const
    {
        const void *prev;
        const void *entry = LinkedListBase::FindMatchingWithPrev(prev, MatchEntry<Arg>, &aArg);

        aPrevEntry = AsEntry(prev);
        return AsEntry(entry);
    }

    template <typename Arg1, typename Arg2>
    const Type *FindMatchingWithPrev(const Type *&aPrevEntry, const Arg1 &aArg1, const Arg2 &aArg2) const
    {
        const void *prev;
        const void *entry = LinkedListBase::FindMatchingWithPrev(prev, MatchEntry<Arg1, Arg2>, &aArg1, &aArg2);

        aPrevEntry = AsEntry(prev);
        return AsEntry(entry);
    }

    /**
     * Searches within the linked list to find an entry matching a set of conditions, and if found also returns a
     * pointer to its previous entry in the list.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[out] aPrevEntry  A pointer to output the previous entry on success (when a match is found in the list).
     *                         @p aPrevEntry is set to `nullptr` if the matching entry is the head of the list.
     *                         Otherwise it is updated to point to the previous entry before the matching entry in the
     *                         list.
     * @param[in]  aArgs       The args to pass to `Matches()`.
     *
     * @returns A pointer to the matching entry if one is found, or `nullptr` if no matching entry was found.
     */
    template <typename... Args> Type *FindMatchingWithPrev(Type *&aPrevEntry, Args &&...aArgs)
    {
        return AsNonConst(AsConst(this)->FindMatchingWithPrev(const_cast<const Type *&>(aPrevEntry), aArgs...));
    }

    /**
     * Searches within the linked list to find an entry matching a set of conditions.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[in]  aArgs  The args to pass to `Matches()`.
     *
     * @returns A pointer to the matching entry if one is found, or `nullptr` if no matching entry was found.
     */
    template <typename Arg> const Type *FindMatching(const Arg &aArg) const
    {
        return AsEntry(LinkedListBase::FindMatching(MatchEntry<Arg>, &aArg));
    }

    template <typename Arg1, typename Arg2> const Type *FindMatching(const Arg1 &aArg1, const Arg2 &aArg2) const
    {
        return AsEntry(LinkedListBase::FindMatching(MatchEntry<Arg1, Arg2>, &aArg1, &aArg2));
    }

    /**
     * Searches within the linked list to find an entry matching a set of conditions.
     *
     * To check that an entry matches, the `Matches()` method is invoked on each `Type` entry in the list. The
     * `Matches()` method with the same set of `Args` input types should be provided by the `Type` class accordingly:
     *
     *      bool Type::Matches(const Args &...) const
     *
     * @param[in]  aArgs  The args to pass to `Matches()`.
     *
     * @returns A pointer to the matching entry if one is found, or `nullptr` if no matching entry was found.
     */
    template <typename... Args> Type *FindMatching(const Args &...aArgs)
    {
        return AsNonConst(AsConst(this)->FindMatching(aArgs...));
    }

    /**
     * Returns the tail of the linked list (i.e., the last entry in the list).
     *
     * @returns A pointer to the tail entry in the linked list or `nullptr` if the list is empty.
     */
    const Type *GetTail(void) const { return AsEntry(LinkedListBase::GetTail()); }

    /**
     * Returns the tail of the linked list (i.e., the last entry in the list).
     *
     * @returns A pointer to the tail entry in the linked list or `nullptr` if the list is empty.
     */
    Type *GetTail(void) { return AsNonConst(AsConst(this)->GetTail()); }

    // The following methods are intended to support range-based `for`
    // loop iteration over the linked-list entries and should not be
    // used directly.

    Iterator begin(void) { return Iterator(GetHead()); }
    Iterator end(void) { return Iterator(nullptr); }

    ConstIterator begin(void) const { return ConstIterator(GetHead()); }
    ConstIterator end(void) const { return ConstIterator(nullptr); }

private:
    class Iterator : public ItemPtrIterator<Type, Iterator>
    {
        friend class LinkedList;
        friend class ItemPtrIterator<Type, Iterator>;

        using ItemPtrIterator<Type, Iterator>::mItem;

        explicit Iterator(Type *aItem)
            : ItemPtrIterator<Type, Iterator>(aItem)
        {
        }

        void Advance(void) { mItem = mItem->GetNext(); }
    };

    class ConstIterator : public ItemPtrIterator<const Type, ConstIterator>
    {
        friend class LinkedList;
        friend class ItemPtrIterator<const Type, ConstIterator>;

        using ItemPtrIterator<const Type, ConstIterator>::mItem;

        explicit ConstIterator(const Type *aItem)
            : ItemPtrIterator<const Type, ConstIterator>(aItem)
        {
        }

        void Advance(void) { mItem = mItem->GetNext(); }
    };

    static Type       *AsEntry(void *aEntry) { return static_cast<Type *>(aEntry); }
    static const Type *AsEntry(const void *aEntry) { return static_cast<const Type *>(aEntry); }
    static const void *GetNextEntry(const void *aEntry) { return AsEntry(aEntry)->GetNext(); }
    static void        SetNextEntry(void *aEntry, void *aNext) { AsEntry(aEntry)->SetNext(AsEntry(aNext)); }

    template <typename Arg> static bool MatchEntry(const void *aEntry, const void *aArg)
    {
        return AsEntry(aEntry)->Matches(*static_cast<const Arg *>(aArg));
    }

    template <typename Arg1, typename Arg2>
    static bool MatchEntry(const void *aEntry, const void *aArg1, const void *aArg2)
    {
        return AsEntry(aEntry)->Matches(*static_cast<const Arg1 *>(aArg1), *static_cast<const Arg2 *>(aArg2));
    }
};

/**
 * @}
 */

} // namespace ot

#endif // OT_CORE_COMMON_LINKED_LIST_HPP_
