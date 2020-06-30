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

#ifndef LINKED_LIST_HPP_
#define LINKED_LIST_HPP_

#include "openthread-core-config.h"

#include <stdio.h>
#include <openthread/error.h>

namespace ot {

/**
 * @addtogroup core-linked-list
 *
 * @brief
 *   This module includes definitions for OpenThread Singly Linked List.
 *
 * @{
 *
 */

/**
 * This template class represents a linked list entry.
 *
 * This class provides methods to `GetNext()` and `SetNext()` in the linked list entry.
 *
 * Users of this class should follow CRTP-style inheritance, i.e., the `Type` class itself should publicly inherit
 * from `LinkedListEntry<Type>`.
 *
 * The template type `Type` should contain a `mNext` member variable. The `mNext` should be of a type that can be
 * down-casted to `Type` itself.
 *
 */
template <class Type> class LinkedListEntry
{
public:
    /**
     * This method gets the next entry in the linked list.
     *
     * @returns A pointer to the next entry in the linked list or nullptr if at the end of the list.
     *
     */
    const Type *GetNext(void) const { return static_cast<const Type *>(static_cast<const Type *>(this)->mNext); }

    /**
     * This method gets the next entry in the linked list.
     *
     * @returns A pointer to the next entry in the linked list or nullptr if at the end of the list.
     *
     */
    Type *GetNext(void) { return static_cast<Type *>(static_cast<Type *>(this)->mNext); }

    /**
     * This method sets the next pointer on the entry.
     *
     * @param[in] aNext  A pointer to the next entry.
     *
     */
    void SetNext(Type *aNext) { static_cast<Type *>(this)->mNext = aNext; }
};

/**
 * This template class represents a singly linked list.
 *
 * The template type `Type` should provide `GetNext()` and `SetNext()` methods (which can be realized by `Type`
 * inheriting from `LinkedListEntry<Type>` class).
 *
 */
template <typename Type> class LinkedList
{
public:
    /**
     * This constructor initializes the linked list.
     *
     */
    LinkedList(void)
        : mHead(nullptr)
    {
    }

    /**
     * This method returns the entry at the head of the linked list
     *
     * @returns Pointer to the entry at the head of the linked list, or nullptr if list is empty.
     *
     */
    Type *GetHead(void) { return mHead; }

    /**
     * This method returns the entry at the head of the linked list.
     *
     * @returns Pointer to the entry at the head of the linked list, or nullptr if list is empty.
     *
     */
    const Type *GetHead(void) const { return mHead; }

    /**
     * This method sets the head of the linked list to a given entry.
     *
     * @param[in] aHead   A pointer to an entry to set as the head of the linked list.
     *
     */
    void SetHead(Type *aHead) { mHead = aHead; }

    /**
     * This method clears the linked list.
     *
     */
    void Clear(void) { mHead = nullptr; }

    /**
     * This method indicates whether the linked list is empty or not.
     *
     * @retval TRUE   If the linked list is empty.
     * @retval FALSE  If the linked list is not empty.
     *
     */
    bool IsEmpty(void) const { return (mHead == nullptr); }

    /**
     * This method pushes an entry at the head of the linked list.
     *
     * @param[in] aEntry   A reference to an entry to push at the head of linked list.
     *
     */
    void Push(Type &aEntry)
    {
        aEntry.SetNext(mHead);
        mHead = &aEntry;
    }

    /**
     * This method pushes an entry after a given previous existing entry in the linked list.
     *
     * @param[in] aEntry       A reference to an entry to push into the list.
     * @param[in] aPrevEntry   A reference to a previous entry (new entry @p aEntry will be pushed after this).
     *
     */
    void PushAfter(Type &aEntry, Type &aPrevEntry)
    {
        aEntry.SetNext(aPrevEntry.GetNext());
        aPrevEntry.SetNext(&aEntry);
    }

    /**
     * This method pops an entry from head of the linked list.
     *
     * @note This method does not change the popped entry itself, i.e., the popped entry next pointer stays as before.
     *
     * @returns The entry that was popped if list is not empty, or nullptr if list is empty.
     *
     */
    Type *Pop(void)
    {
        Type *entry = mHead;

        if (mHead != nullptr)
        {
            mHead = mHead->GetNext();
        }

        return entry;
    }

    /**
     * This method pops an entry after a given previous entry.
     *
     * @note This method does not change the popped entry itself, i.e., the popped entry next pointer stays as before.
     *
     * @param[in] aPrevEntry  A pointer to a previous entry. If it is not nullptr the entry after this will be popped,
     *                        otherwise (if it is nullptr) the entry at head of the list is popped.
     *
     * @returns Pointer to the entry that was popped, or nullptr if there is no entry to pop.
     *
     */
    Type *PopAfter(Type *aPrevEntry)
    {
        Type *entry;

        if (aPrevEntry == nullptr)
        {
            entry = Pop();
        }
        else
        {
            entry = aPrevEntry->GetNext();

            if (entry != nullptr)
            {
                aPrevEntry->SetNext(entry->GetNext());
            }
        }

        return entry;
    }

    /**
     * This method indicates whether the linked list contains a given entry.
     *
     * @param[in] aEntry   A reference to an entry.
     *
     * @retval TRUE   The linked list contains @p aEntry.
     * @retval FALSE  The linked list does not contain @p aEntry.
     *
     */
    bool Contains(const Type &aEntry) const
    {
        bool contains = false;

        for (Type *cur = mHead; cur != nullptr; cur = cur->GetNext())
        {
            if (cur == &aEntry)
            {
                contains = true;
                break;
            }
        }

        return contains;
    }

    /**
     * This method adds an entry (at the head of the linked list) if it is not already in the list.
     *
     * @param[in] aEntry   A reference to an entry to add.
     *
     * @retval OT_ERROR_NONE     The entry was successfully added at the head of the list.
     * @retval OT_ERROR_ALREADY  The entry is already in the list.
     *
     */
    otError Add(Type &aEntry)
    {
        otError error = OT_ERROR_NONE;

        if (Contains(aEntry))
        {
            error = OT_ERROR_ALREADY;
        }
        else
        {
            Push(aEntry);
        }

        return error;
    }

    /**
     * This method removes an entry from the linked list.
     *
     * @note This method does not change the removed entry @p aEntry itself (it is `const`), i.e., the  entry next
     * pointer of @p aEntry stays as before.
     *
     * @param[in] aEntry   A reference to an entry to remove.
     *
     * @retval OT_ERROR_NONE       The entry was successfully removed from the list.
     * @retval OT_ERROR_NOT_FOUND  Could not find the entry in the list.
     *
     */
    otError Remove(const Type &aEntry)
    {
        otError error = OT_ERROR_NOT_FOUND;

        if (mHead == &aEntry)
        {
            Pop();
            error = OT_ERROR_NONE;
        }
        else if (mHead != nullptr)
        {
            for (Type *cur = mHead; cur->GetNext() != nullptr; cur = cur->GetNext())
            {
                if (cur->GetNext() == &aEntry)
                {
                    cur->SetNext(cur->GetNext()->GetNext());
                    error = OT_ERROR_NONE;
                    break;
                }
            }
        }

        return error;
    }

    /**
     * This method searches within the linked list to find an entry and if found returns a pointer to previous entry.
     *
     * @param[in]  aEntry      A reference to an entry to find.
     * @param[out] aPrevEntry  A pointer to output previous entry on success (when @p aEntry is found is the list).
     *                         @p aPrevEntry is set to nullptr if the @p aEntry is head of the list. Otherwise it
     *                         is updated to point to previous entry before @p aEntry in the list.
     *
     * @retval OT_ERROR_NONE       The entry was found in the list and @p aPrevEntry was updated successfully.
     * @retval OT_ERROR_NOT_FOUND  The entry was not found in the list.
     *
     */
    otError Find(const Type &aEntry, Type *&aPrevEntry) const
    {
        otError error = OT_ERROR_NOT_FOUND;

        if (mHead == &aEntry)
        {
            aPrevEntry = nullptr;
            error      = OT_ERROR_NONE;
        }
        else if (mHead != nullptr)
        {
            for (Type *cur = mHead; cur->GetNext() != nullptr; cur = cur->GetNext())
            {
                if (cur->GetNext() == &aEntry)
                {
                    aPrevEntry = cur;
                    error      = OT_ERROR_NONE;
                    break;
                }
            }
        }

        return error;
    }

    /**
     * This method returns the tail of the linked list (i.e., the last entry in the list).
     *
     * @returns A pointer to tail entry in the linked list or nullptr if list is empty.
     *
     */
    const Type *GetTail(void) const
    {
        const Type *tail = mHead;

        if (tail != nullptr)
        {
            while (tail->GetNext() != nullptr)
            {
                tail = tail->GetNext();
            }
        }

        return tail;
    }

    /**
     * This method returns the tail of the linked list (i.e., the last entry in the list).
     *
     * @returns A pointer to tail entry in the linked list or nullptr if list is empty.
     *
     */
    Type *GetTail(void) { return const_cast<Type *>(const_cast<const LinkedList<Type> *>(this)->GetTail()); }

private:
    Type *mHead;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // LINKED_LIST_HPP_
