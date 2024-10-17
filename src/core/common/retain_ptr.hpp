/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes definitions for a retain (reference counted) smart pointer.
 */

#ifndef RETAIN_PTR_HPP_
#define RETAIN_PTR_HPP_

#include "openthread-core-config.h"

#include "common/ptr_wrapper.hpp"

namespace ot {

/**
 * Represents a retain (reference counted) smart pointer.
 *
 * The `Type` class MUST provide mechanism to track its current retain count. It MUST provide the following three
 * methods:
 *
 *   - void      IncrementRetainCount(void);  (Increment the retain count).
 *   - uint16_t  DecrementRetainCount(void);  (Decrement the retain count and return the value after decrement).
 *   - void      Free(void);                  (Free the `Type` instance).
 *
 * The `Type` can inherit from `RetainCountable` which provides the retain counting methods.
 *
 * @tparam Type  The pointer type.
 */
template <class Type> class RetainPtr : public Ptr<Type>
{
    using Ptr<Type>::mPointer;

public:
    /**
     * This is the default constructor for `RetainPtr` initializing it as null.
     */
    RetainPtr(void) = default;

    /**
     * Initializes the `RetainPtr` with a given pointer.
     *
     * Upon construction the `RetainPtr` will increment the retain count on @p aPointer (if not null).
     *
     * @param[in] aPointer  A pointer to object to initialize with.
     */
    explicit RetainPtr(Type *aPointer)
        : Ptr<Type>(aPointer)
    {
        IncrementRetainCount();
    }

    /**
     * Initializes the `RetainPtr` from another `RetainPtr`.
     *
     * @param[in] aOther   Another `RetainPtr`.
     */
    RetainPtr(const RetainPtr &aOther)
        : Ptr<Type>(aOther.mPointer)
    {
        IncrementRetainCount();
    }

    /**
     * This is the destructor for `RetainPtr`.
     *
     * Upon destruction, the `RetainPtr` will decrement the retain count on the managed object (if not null) and
     * free the object if its retain count reaches zero.
     */
    ~RetainPtr(void) { DecrementRetainCount(); }

    /**
     * Replaces the managed object by `RetainPtr` with a new one.
     *
     * The method correctly handles a self `Reset()` (i.e., @p aPointer being the same pointer as the one currently
     * managed by `RetainPtr`).
     *
     * @param[in] aPointer   A pointer to a new object to replace with.
     */
    void Reset(Type *aPointer = nullptr)
    {
        if (aPointer != mPointer)
        {
            DecrementRetainCount();
            mPointer = aPointer;
            IncrementRetainCount();
        }
    }

    /**
     * Releases the ownership of the current pointer in `RetainPtr` (if any) without changing its retain
     * count.
     *
     * After this call, the `RetainPtr` will be null.
     *
     * @returns The pointer to the object managed by `RetainPtr` or `nullptr` if `RetainPtr` was null.
     */
    Type *Release(void)
    {
        Type *pointer = mPointer;
        mPointer      = nullptr;
        return pointer;
    }

    /**
     * Overloads the assignment operator `=`.
     *
     * The `RetainPtr` first frees its current managed object (if there is any and it is different from @p aOther)
     * before taking over the ownership of the object from @p aOther. This method correctly handles a self assignment
     * (i.e., assigning the pointer to itself).
     *
     * @param[in] aOther   A reference to another `RetainPtr`.
     *
     * @returns A reference to this `RetainPtr`.
     */
    RetainPtr &operator=(const RetainPtr &aOther)
    {
        Reset(aOther.mPointer);
        return *this;
    }

private:
    void IncrementRetainCount(void)
    {
        if (mPointer != nullptr)
        {
            mPointer->IncrementRetainCount();
        }
    }

    void DecrementRetainCount(void)
    {
        if ((mPointer != nullptr) && (mPointer->DecrementRetainCount() == 0))
        {
            mPointer->Free();
        }
    }
};

/**
 * Provides mechanism to track retain count.
 */
class RetainCountable
{
    template <class Type> friend class RetainPtr;

protected:
    /**
     * This constrictor initializes the object starting with retain count of zero.
     */
    RetainCountable(void)
        : mRetainCount(0)
    {
    }

    /**
     * Returns the current retain count.
     *
     * @returns The current retain count.
     */
    uint16_t GetRetainCount(void) const { return mRetainCount; }

    /**
     * Increments the retain count.
     */
    void IncrementRetainCount(void) { ++mRetainCount; }

    /**
     * Decrements the retain count.
     *
     * @returns The retain count value after decrementing it.
     */
    uint16_t DecrementRetainCount(void) { return --mRetainCount; }

private:
    uint16_t mRetainCount;
};

} // namespace ot

#endif // RETAIN_PTR_HPP_
