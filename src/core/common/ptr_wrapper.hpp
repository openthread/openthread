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
 *   This file includes definitions for a pointer wrapper.
 */

#ifndef PTR_WRAPPER_HPP_
#define PTR_WRAPPER_HPP_

#include "openthread-core-config.h"

#include <stdbool.h>
#include <stdint.h>

namespace ot {

/**
 * This template class represents a wrapper over a pointer.
 *
 * This is intended as base class of `OwnedPtr` or `RetainPtr` providing common simple methods.
 *
 * @tparam Type  The pointer type.
 *
 */
template <class Type> class Ptr
{
public:
    /**
     * This is the default constructor for `Ptr` initializing it as null.
     *
     */
    Ptr(void)
        : mPointer(nullptr)
    {
    }

    /**
     * This constructor initializes the `Ptr` with a given pointer.
     *
     * @param[in] aPointer  A pointer to initialize with.
     *
     */
    explicit Ptr(Type *aPointer)
        : mPointer(aPointer)
    {
    }

    /**
     * This method indicates whether the `Ptr` is null or not.
     *
     * @retval TRUE   The `Ptr` is null.
     * @retval FALSE  The `Ptr` is not null.
     *
     */
    bool IsNull(void) const { return (mPointer == nullptr); }

    /**
     * This method gets the wrapped pointer.
     *
     * @returns The wrapped pointer.
     *
     */
    Type *Get(void) { return mPointer; }

    /**
     * This method gets the wrapped pointer.
     *
     * @returns The wrapped pointer.
     *
     */
    const Type *Get(void) const { return mPointer; }

    /**
     * This method overloads the `->` dereference operator and returns the pointer.
     *
     * @returns The wrapped pointer.
     *
     */
    Type *operator->(void) { return mPointer; }

    /**
     * This method overloads the `->` dereference operator and returns the pointer.
     *
     * @returns The wrapped pointer.
     *
     */
    const Type *operator->(void)const { return mPointer; }

    /**
     * This method overloads the `*` dereference operator and returns a reference to the pointed object.
     *
     * The behavior is undefined if `IsNull() == true`.
     *
     * @returns A reference to the pointed object.
     *
     */
    Type &operator*(void) { return *mPointer; }

    /**
     * This method overloads the `*` dereference operator and returns a reference to the pointed object.
     *
     * The behavior is undefined if `IsNull() == true`.
     *
     * @returns A reference to the pointed object.
     *
     */
    const Type &operator*(void)const { return *mPointer; }

    /**
     * This method overloads the operator `==` to compare the `Ptr` with a given pointer.
     *
     * @param[in] aPointer   The pointer to compare with.
     *
     * @retval TRUE   If `Ptr` is equal to @p aPointer.
     * @retval FALSE  If `Ptr` is not equal to @p aPointer.
     *
     */
    bool operator==(const Type *aPointer) const { return (mPointer == aPointer); }

    /**
     * This method overloads the operator `!=` to compare the `Ptr` with a given pointer.
     *
     * @param[in] aPointer   The pointer to compare with.
     *
     * @retval TRUE   If `Ptr` is not equal to @p aPointer.
     * @retval FALSE  If `Ptr` is equal to @p aPointer.
     *
     */
    bool operator!=(const Type *aPointer) const { return (mPointer != aPointer); }

    /**
     * This method overloads the operator `==` to compare the `Ptr` with another `Ptr`.
     *
     * @param[in] aOther   The other `Ptr` to compare with.
     *
     * @retval TRUE   If `Ptr` is equal to @p aOther.
     * @retval FALSE  If `Ptr` is not equal to @p aOther.
     *
     */
    bool operator==(const Ptr &aOther) const { return (mPointer == aOther.mPointer); }

    /**
     * This method overloads the operator `!=` to compare the `Ptr` with another `Ptr`.
     *
     * @param[in] aOther   The other `Ptr` to compare with.
     *
     * @retval TRUE   If `Ptr` is not equal to @p aOther.
     * @retval FALSE  If `Ptr` is equal to @p aOther.
     *
     */
    bool operator!=(const Ptr &aOther) const { return (mPointer != aOther.mPointer); }

protected:
    Type *mPointer;
};

} // namespace ot

#endif // PTR_WRAPPER_HPP_
