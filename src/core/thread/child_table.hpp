/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file includes definitions for Thread child table.
 */

#ifndef CHILD_TABLE_HPP_
#define CHILD_TABLE_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "thread/topology.hpp"

namespace ot {

#if OPENTHREAD_FTD

/**
 * This class represents the Thread child table.
 *
 */
class ChildTable : public InstanceLocator
{
public:
    /**
     * This enumeration defines child state filters used for finding a child or iterating through the child table.
     *
     * Each filter definition accepts a subset of `Child:State` values.
     *
     */
    enum StateFilter
    {
        kInStateValid,                     ///< Accept child only in `Child::kStateValid`.
        kInStateValidOrRestoring,          ///< Accept child with `Child::IsStateValidOrRestoring()` being `true`.
        kInStateChildIdRequest,            ///< Accept child only in `Child:kStateChildIdRequest`.
        kInStateValidOrAttaching,          ///< Accept child with `Child::IsStateValidOrAttaching()` being `true`.
        kInStateAnyExceptInvalid,          ///< Accept child in any state except `Child:kStateInvalid`.
        kInStateAnyExceptValidOrRestoring, ///< Accept child in any state except `Child::IsStateValidOrRestoring()`.
    };

    /**
     * This class represents an iterator for iterating through the child entries in the child table.
     *
     */
    class Iterator : public InstanceLocator
    {
    public:
        /**
         * This constructor initializes an `Iterator` instance to start from beginning of the child table.
         *
         * @param[in] aInstance  A reference to the OpenThread instance.
         * @param[in] aFilter    A child state filter.
         *
         */
        Iterator(Instance &aInstance, StateFilter aFilter);

        /**
         * This constructor initializes an `Iterator` instance to start from a given child.
         *
         * This constructor allows the iterator to start from a given `Child` entry. The iterator will start from the
         * given child and will go through all entries in the child table (matching the filter) till it gets back to
         * the starting `Child` entry.
         *
         * If the given starting `Child` pointer is `NULL`, then the iterator starts from beginning of the child table.
         *
         * @param[in] aInstance        A reference to the OpenThread instance.
         * @param[in] aFilter          A child state filter.
         * @param[in] aStartingChild   A pointer to a child. If non-NULL, the iterator starts from the given entry.
         *
         */
        Iterator(Instance &aInstance, StateFilter aFilter, Child *aStartingChild);

        /**
         * This method resets the iterator to start over.
         *
         */
        void Reset(void);

        /**
         * This method indicates whether there are no more `Child` entries in the list (iterator has reached end of
         * the list).
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) const { return (mChild == NULL); }

        /**
         * This method advances the iterator.
         *
         * The iterator is moved to point to the next `Child` entry matching the given state filter in the constructor.
         * If there are no more `Child` entries matching the given filter, the iterator becomes empty (i.e.,
         * `GetChild()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void Advance(void);

        /**
         * This method overloads `++` operator (pre-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Child` entry matching the given state filter in the constructor.
         * If there are no more `Child` entries matching the given filter, the iterator becomes empty (i.e.,
         * `GetChild()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void operator++(void) { Advance(); }

        /**
         * This method overloads `++` operator (post-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Child` entry matching the given state filter in the constructor.
         * If there are no more `Child` entries matching the given filter, the iterator becomes empty (i.e.,
         * `GetChild()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void operator++(int) { Advance(); }

        /**
         * This method gets the `Child` entry to which the iterator is currently pointing.
         *
         * @returns A pointer to the `Child` entry, or `NULL` if the iterator is done and/or empty.
         *
         */
        Child *GetChild(void) { return mChild; }

    private:
        StateFilter mFilter;
        Child *     mStart;
        Child *     mChild;
    };

    /**
     * This constructor initializes a `ChildTable` instance.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit ChildTable(Instance &aInstance);

    /**
     * This method clears the child table.
     *
     */
    void Clear(void) { memset(mChildren, 0, sizeof(mChildren)); }

    /**
     * This method returns the child table index for a given `Child` instance.
     *
     * @param[in]  aChild  A reference to a `Child`
     *
     * @returns The index corresponding to @p aChild.
     *
     */
    uint8_t GetChildIndex(const Child &aChild) const { return static_cast<uint8_t>(&aChild - mChildren); }

    /**
     * This method returns a pointer to a `Child` entry at a given index, or `NULL` if the index is out of bounds,
     * i.e., index is larger or equal to maximum number of children allowed (@sa GetMaxChildrenAllowed()).
     *
     * @param[in]  aChildIndex  A child index.
     *
     * @returns A pointer to the `Child` corresponding to the given index, or `NULL` if the index is out of bounds.
     *
     */
    Child *GetChildAtIndex(uint8_t aChildIndex);

    /**
     * This method gets a new/unused `Child` entry from the child table.
     *
     * @note The returned child entry will be cleared (`memset` to zero).
     *
     * @returns A pointer to a new `Child` entry, or `NULL` if all `Child` entries are in use.
     *
     */
    Child *GetNewChild(void);

    /**
     * This method searches the child table for a `Child` with a given RLOC16 also matching a given state filter.
     *
     * @param[in]  aRloc16  A RLOC16 address.
     * @param[in]  aFilter  A child state filter.
     *
     * @returns  A pointer to the `Child` entry if one is found, or `NULL` otherwise.
     *
     */
    Child *FindChild(uint16_t aRloc16, StateFilter aFilter);

    /**
     * This method searches the child table for a `Child` with a given extended address also matching a given state
     * filter.
     *
     * @param[in]  aAddress A reference to an extended address.
     * @param[in]  aFilter  A child state filter.
     *
     * @returns  A pointer to the `Child` entry if one is found, or `NULL` otherwise.
     *
     */
    Child *FindChild(const Mac::ExtAddress &aAddress, StateFilter aFilter);

    /**
     * This method searches the child table for a `Child` with a given address also matching a given state filter.
     *
     * @param[in]  aAddress A reference to a MAC address.
     * @param[in]  aFilter  A child state filter.
     *
     * @returns  A pointer to the `Child` entry if one is found, or `NULL` otherwise.
     *
     */
    Child *FindChild(const Mac::Address &aAddress, StateFilter aFilter);

    /**
     * This method indicates whether the child table contains any child matching a given state filter.
     *
     * @param[in]  aFilter  A child state filter.
     *
     * @returns  TRUE if the table contains at least one child table matching the given filter, FALSE otherwise.
     *
     */
    bool HasChildren(StateFilter aFilter) const;

    /**
     * This method returns the number of children in the child table matching a given state filter.
     *
     * @param[in]  aFilter  A child state filter.
     *
     * @returns Number of children matching the given state filer.
     *
     */
    uint8_t GetNumChildren(StateFilter aFilter) const;

    /**
     * This method returns the maximum number of children that can be supported (build-time constant).
     *
     * @note Number of children allowed (from `GetMaxChildrenAllowed()`) can be less than maximum number of supported
     * children.
     *
     * @returns  The maximum number of children supported
     *
     */
    uint8_t GetMaxChildren(void) const { return kMaxChildren; }

    /**
     * This method get the maximum number of children allowed.
     *
     * @returns  The maximum number of children allowed.
     *
     */
    uint8_t GetMaxChildrenAllowed(void) const { return mMaxChildrenAllowed; }

    /**
     * This method sets the maximum number of children allowed.
     *
     * The number of children allowed must be at least one and at most same as maximum supported children (@sa
     * GetMaxChildren()). It can be changed only if the child table is empty.
     *
     * @param[in]  aMaxChildren  Maximum number of children allowed.
     *
     * @retval OT_ERROR_NONE          The number of allowed children changed successfully.
     * @retval OT_ERROR_INVALID_ARGS  If @p aMaxChildren is not in the range [1, Max supported children].
     * @retval OT_ERROR_INVALID_STATE The child table is not empty.
     *
     */
    otError SetMaxChildrenAllowed(uint8_t aMaxChildren);

private:
    enum
    {
        kMaxChildren = OPENTHREAD_CONFIG_MAX_CHILDREN,
    };

    static bool MatchesFilter(const Child &aChild, StateFilter aFilter);

    uint8_t mMaxChildrenAllowed;
    Child   mChildren[kMaxChildren];
};

#endif // OPENTHREAD_FTD

#if OPENTHREAD_MTD

class ChildTable : public InstanceLocator
{
public:
    enum StateFilter
    {
        kInStateValid,
        kInStateValidOrRestoring,
        kInStateChildIdRequest,
        kInStateValidOrAttaching,
        kInStateAnyExceptInvalid,
    };

    class Iterator
    {
    public:
        Iterator(Instance &, StateFilter) {}
        Iterator(Instance &, StateFilter, Child *) {}
        void   Reset(void) {}
        bool   IsDone(void) const { return true; }
        void   Advance(void) {}
        void   operator++(void) {}
        void   operator++(int) {}
        Child *GetChild(void) { return NULL; }
    };

    explicit ChildTable(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }
    void Clear(void) {}

    uint8_t GetChildIndex(const Child &) const { return 0; }
    Child * GetChildAtIndex(uint8_t) { return NULL; }

    Child *GetNewChild(void) { return NULL; }

    Child *FindChild(uint16_t, StateFilter) { return NULL; }
    Child *FindChild(const Mac::ExtAddress &, StateFilter) { return NULL; }
    Child *FindChild(const Mac::Address &, StateFilter) { return NULL; }

    bool    HasChildren(StateFilter) const { return false; }
    uint8_t GetNumChildren(StateFilter) const { return 0; }
    uint8_t GetMaxChildren(void) const { return 0; }
    uint8_t GetMaxChildrenAllowed(void) const { return 0; }
    otError SetMaxChildrenAllowed(uint8_t) { return OT_ERROR_INVALID_STATE; }
};

#endif // OPENTHREAD_MTD

} // namespace ot

#endif // CHILD_TABLE_HPP_
