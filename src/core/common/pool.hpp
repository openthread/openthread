/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for a generic object pool.
 */

#ifndef OT_CORE_COMMON_POOL_HPP_
#define OT_CORE_COMMON_POOL_HPP_

#include "openthread-core-config.h"

#include "common/array.hpp"
#include "common/linked_list.hpp"
#include "common/non_copyable.hpp"

namespace ot {

class Instance;

/**
 * @addtogroup core-pool
 *
 * @brief
 *   This module includes definitions for OpenThread object pool.
 *
 * @{
 */

/**
 * Represents a base class for an object pool.
 *
 * @tparam Type         The object type. Type should provide `GetNext()` and `SetNext()` so that it can be added to a
 *                      linked list.
 */
template <class Type> class PoolBase : private NonCopyable
{
public:
    /**
     * Allocates a new object from the pool.
     *
     * @returns A pointer to the newly allocated object, or `nullptr` if all entries from the pool are already
     *          allocated.
     */
    Type *Allocate(void) { return mFreeList.Pop(); }

    /**
     * Frees a previously allocated object.
     *
     * The @p aEntry MUST be an entry from the pool previously allocated using `Allocate()` method and not yet freed.
     * An already freed entry MUST not be freed again.
     *
     * @param[in]  aEntry   The pool object entry to free.
     */
    void Free(Type &aEntry) { mFreeList.Push(aEntry); }

protected:
    PoolBase(void) = default;

    LinkedList<Type> mFreeList;
};

/**
 * Represents an object pool.
 *
 * @tparam Type         The object type. Type should provide `GetNext() and `SetNext()` so that it can be added to a
 *                      linked list.
 * @tparam kPoolSize    Specifies the pool size (maximum number of objects in the pool).
 */
template <class Type, uint16_t kPoolSize> class Pool : public PoolBase<Type>
{
public:
    /**
     * Initializes the pool.
     */
    Pool(void)
    {
        for (Type &entry : mPool)
        {
            PoolBase<Type>::Free(entry);
        }
    }

    /**
     * Initializes the pool.
     *
     * Version requires the `Type` class to provide method `void Init(Instance &)` to initialize
     * each `Type` entry object. This can be realized by the `Type` class inheriting from `InstanceLocatorInit()`.
     *
     * @param[in] aInstance   A reference to the OpenThread instance.
     */
    explicit Pool(Instance &aInstance)
    {
        for (Type &entry : mPool)
        {
            entry.Init(aInstance);
            PoolBase<Type>::Free(entry);
        }
    }

    /**
     * Frees all previously allocated objects.
     */
    void FreeAll(void)
    {
        PoolBase<Type>::mFreeList.Clear();

        for (Type &entry : mPool)
        {
            PoolBase<Type>::Free(entry);
        }
    }

    /**
     * Returns the pool size.
     *
     * @returns The pool size (maximum number of objects in the pool).
     */
    uint16_t GetSize(void) const { return kPoolSize; }

    /**
     * Indicates whether or not a given `Type` object is from the pool.
     *
     * @param[in]  aObject   A reference to a `Type` object.
     *
     * @retval TRUE   If @p aObject is from the pool.
     * @retval FALSE  If @p aObject is not from the pool.
     */
    bool IsPoolEntry(const Type &aObject) const { return (&mPool[0] <= &aObject) && (&aObject < GetArrayEnd(mPool)); }

    /**
     * Returns the associated index of a given entry from the pool.
     *
     * The @p aEntry MUST be from the pool, otherwise the behavior of this method is undefined.
     *
     * @param[in] aEntry  A reference to an entry from the pool.
     *
     * @returns The associated index of @p aEntry.
     */
    uint16_t GetIndexOf(const Type &aEntry) const { return static_cast<uint16_t>(&aEntry - mPool); }

    /**
     * Retrieves a pool entry at a given index.
     *
     * The @p aIndex MUST be from an earlier call to `GetIndexOf()`.
     *
     * @param[in] aIndex  An index.
     *
     * @returns A reference to entry at index @p aIndex.
     */
    Type &GetEntryAt(uint16_t aIndex) { return mPool[aIndex]; }

    /**
     * Retrieves a pool entry at a given index.
     *
     * The @p aIndex MUST be from an earlier call to `GetIndexOf()`.
     *
     * @param[in] aIndex  An index.
     *
     * @returns A reference to entry at index @p aIndex.
     */
    const Type &GetEntryAt(uint16_t aIndex) const { return mPool[aIndex]; }

private:
    Type mPool[kPoolSize];
};

/**
 * Represents an object pool with run-time configurable storage.
 *
 * @tparam Type         The object type. Type should provide `GetNext()` and `SetNext()` so that it can be added to a
 *                      linked list.
 */
template <class Type> class ConfigPool : public PoolBase<Type>
{
public:
    /**
     * Initializes the pool as empty and unlinked to any buffer.
     */
    ConfigPool(void)
        : mEntryArray(nullptr)
        , mNumEntries(0)
    {
    }

    /**
     * Initializes the pool with a given array of entries.
     *
     * @param[in] aEntryArray  A pointer to an array of entries.
     * @param[in] aNumEntries  The number of entries in the array.
     *
     * @retval kErrorNone     Successfully initialized the pool.
     * @retval kErrorAlready  The pool is already initialized.
     */
    Error Init(Type *aEntryArray, uint16_t aNumEntries)
    {
        Error error = kErrorNone;

        VerifyOrExit(mEntryArray == nullptr, error = kErrorAlready);

        mEntryArray = aEntryArray;
        mNumEntries = aNumEntries;

        for (uint16_t i = 0; i < aNumEntries; i++)
        {
            PoolBase<Type>::Free(mEntryArray[i]);
        }

    exit:
        return error;
    }

    /**
     * Returns the pool size.
     *
     * @returns The pool size (maximum number of objects in the pool).
     */
    uint16_t GetSize(void) const { return mNumEntries; }

    /**
     * Indicates whether or not a given `Type` object is from the pool.
     *
     * @param[in]  aObject   A reference to a `Type` object.
     *
     * @retval TRUE   If @p aObject is from the pool.
     * @retval FALSE  If @p aObject is not from the pool.
     */
    bool IsPoolEntry(const Type &aObject) const
    {
        return (mNumEntries > 0) && (mEntryArray <= &aObject) && (&aObject < &mEntryArray[mNumEntries]);
    }

private:
    Type    *mEntryArray;
    uint16_t mNumEntries;
};

/**
 * @}
 */

} // namespace ot

#endif // OT_CORE_COMMON_POOL_HPP_
