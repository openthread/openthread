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
 *   This file includes definitions for `Heap::Allocatable`.
 */

#ifndef HEAP_ALLOCATABLE_HPP_
#define HEAP_ALLOCATABLE_HPP_

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/heap.hpp"
#include "common/new.hpp"

namespace ot {
namespace Heap {

/**
 * This template class defines a `Heap::Allocatable` object.
 *
 * `Heap::Allocatable` provides methods to allocate and free instances of `Type` on heap.
 *
 * Users of this class should follow CRTP-style inheritance, i.e., the `Type` class itself should inherit from
 * `Allocatable<Type>`.
 *
 * The `Type` class destructor is used when `Free()` is called. The destructor frees any heap allocated data members
 * that are stored in a `Type` instance (e.g., `Heap::String`, `Heap::Data`, etc).
 *
 */
template <class Type> class Allocatable
{
public:
    /**
     * This static method allocates a new instance of `Type` on heap and initializes it using `Type` constructor.
     *
     * The `Type` class MUST have a constructor `Type(Args...)` which is invoked upon allocation of new `Type` to
     * initialize it.
     *
     * @param[in] aArgs   A set of arguments to pass to the `Type` constructor of the allocated `Type` instance.
     *
     * @returns A pointer to the newly allocated instance or `nullptr` if it fails to allocate.
     *
     */
    template <typename... Args> static Type *Allocate(Args &&... aArgs)
    {
        void *buf = Heap::CAlloc(1, sizeof(Type));

        return (buf != nullptr) ? new (buf) Type(static_cast<Args &&>(aArgs)...) : nullptr;
    }

    /**
     * This static method allocates a new instance of `Type` on heap and initializes it using `Type::Init()` method.
     *
     * The `Type` class MUST have a default constructor (with no arguments) which is invoked upon allocation of new
     * `Type` instance. It MUST also provide an `Error Init(Args...)` method to initialize the instance. If any `Error`
     * other than `kErrorNone` is returned the initialization is considered failed.
     *
     * @param[in] aArgs   A set of arguments to initialize the allocated `Type` instance.
     *
     * @returns A pointer to the newly allocated instance or `nullptr` if it fails to allocate or initialize.
     *
     */
    template <typename... Args> static Type *AllocateAndInit(Args &&... aArgs)
    {
        void *buf    = Heap::CAlloc(1, sizeof(Type));
        Type *object = nullptr;

        VerifyOrExit(buf != nullptr);

        object = new (buf) Type();

        if (object->Init(static_cast<Args &&>(aArgs)...) != kErrorNone)
        {
            object->Free();
            object = nullptr;
        }

    exit:
        return object;
    }

    /**
     * This method frees the `Type` instance.
     *
     * The instance MUST be heap allocated using either `Allocate()` or `AllocateAndInit()`.
     *
     * The `Free()` method invokes the `Type` destructor before releasing the allocated heap buffer for the instance.
     * This ensures that any heap allocated member variables in `Type` are freed before the `Type` instance itself is
     * freed.
     *
     */
    void Free(void)
    {
        static_cast<Type *>(this)->~Type();
        Heap::Free(this);
    }

protected:
    Allocatable(void) = default;
};

} // namespace Heap
} // namespace ot

#endif // HEAP_ALLOCATABLE_HPP_
