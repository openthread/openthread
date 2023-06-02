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
 *   This file includes definitions for an owned smart pointer.
 */

#ifndef OWNED_PTR_HPP_
#define OWNED_PTR_HPP_

#include "openthread-core-config.h"

#include "common/ptr_wrapper.hpp"

namespace ot {

/**
 * This template class represents an owned smart pointer.
 *
 * `OwnedPtr` acts as sole owner of the object it manages. An `OwnedPtr` is non-copyable (copy constructor is deleted)
 * but the ownership can be transferred from one `OwnedPtr` to another using move semantics.
 *
 * The `Type` class MUST provide `Free()` method which frees the instance.
 *
 * @tparam Type  The pointer type.
 *
 */
template <class Type> class OwnedPtr : public Ptr<Type>
{
    using Ptr<Type>::mPointer;

public:
    /**
     * This is the default constructor for `OwnedPtr` initializing it as null.
     *
     */
    OwnedPtr(void) = default;

    /**
     * This constructor initializes the `OwnedPtr` with a given pointer.
     *
     * The `OwnedPtr` takes the ownership of the object at @p aPointer.
     *
     * @param[in] aPointer  A pointer to object to initialize with.
     *
     */
    explicit OwnedPtr(Type *aPointer)
        : Ptr<Type>(aPointer)
    {
    }

    /**
     * This constructor initializes the `OwnedPtr` from another `OwnedPtr` using move semantics.
     *
     * The `OwnedPtr` takes over the ownership of the object from @p aOther. After this call, @p aOther will be null.
     *
     * @param[in] aOther   An rvalue reference to another `OwnedPtr`.
     *
     */
    OwnedPtr(OwnedPtr &&aOther)
    {
        mPointer        = aOther.mPointer;
        aOther.mPointer = nullptr;
    }

    /**
     * This is the destructor for `OwnedPtr`.
     *
     * Upon destruction, the `OwnedPtr` invokes `Free()` method on its managed object (if any).
     *
     */
    ~OwnedPtr(void) { Delete(); }

    /**
     * This method frees the owned object (if any).
     *
     * This method invokes `Free()` method on the `Type` object owned by `OwnedPtr` (if any). It will also set the
     * `OwnedPtr` to null.
     *
     */
    void Free(void)
    {
        Delete();
        mPointer = nullptr;
    }

    /**
     * This method frees the current object owned by `OwnedPtr` (if any) and replaces it with a new one.
     *
     * The method will `Free()` the current object managed by `OwnedPtr` (if different from @p aPointer) before taking
     * the ownership of the object at @p aPointer. The method correctly handles a self `Reset()` (i.e., @p aPointer
     * being the same pointer as the one currently managed by `OwnedPtr`).
     *
     * @param[in] aPointer   A pointer to the new object to replace with.
     *
     */
    void Reset(Type *aPointer = nullptr)
    {
        if (mPointer != aPointer)
        {
            Delete();
            mPointer = aPointer;
        }
    }

    /**
     * This method releases the ownership of the current object in `OwnedPtr` (if any).
     *
     * After this call, the `OwnedPtr` will be null.
     *
     * @returns The pointer to the object owned by `OwnedPtr` or `nullptr` if `OwnedPtr` was null.
     *
     */
    Type *Release(void)
    {
        Type *pointer = mPointer;
        mPointer      = nullptr;
        return pointer;
    }

    /**
     * This method allows passing of the ownership to another `OwnedPtr` using move semantics.
     *
     * @returns An rvalue reference of the pointer to move from.
     *
     */
    OwnedPtr &&PassOwnership(void) { return static_cast<OwnedPtr &&>(*this); }

    /**
     * This method overload the assignment operator `=` to replace the object owned by the `OwnedPtr` with another one
     * using move semantics.
     *
     * The `OwnedPtr` first frees its current owned object (if there is any and it is different from @p aOther) before
     * taking over the ownership of the object from @p aOther. This method correctly handles a self assignment (i.e.,
     * assigning the pointer to itself).
     *
     * @param[in] aOther   An rvalue reference to an `OwnedPtr` to move from.
     *
     * @returns A reference to this `OwnedPtr`.
     *
     */
    OwnedPtr &operator=(OwnedPtr &&aOther)
    {
        Reset(aOther.Release());
        return *this;
    }

    OwnedPtr(const OwnedPtr &) = delete;
    OwnedPtr(OwnedPtr &)       = delete;
    OwnedPtr &operator=(const OwnedPtr &) = delete;

private:
    void Delete(void)
    {
        if (mPointer != nullptr)
        {
            mPointer->Free();
        }
    }
};

} // namespace ot

#endif // OWNED_PTR_HPP_
