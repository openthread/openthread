/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *  This file defines OpenThread `Callback` class.
 */

#ifndef CALLBACK_HPP_
#define CALLBACK_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include "common/type_traits.hpp"

namespace ot {

/**
 * Specifies the context argument position in a callback function pointer.
 */
enum CallbackContextPosition : uint8_t
{
    kContextAsFirstArg, ///< Context is the first argument.
    kContextAsLastArg,  ///< Context is the last argument.
};

/**
 * Is the base class for `Callback` (a function pointer handler and a `void *` context).
 *
 * @tparam HandlerType    The handler function pointer type.
 */
template <typename HandlerType> class CallbackBase
{
public:
    /**
     * Clears the `Callback` by setting the handler function pointer to `nullptr`.
     */
    void Clear(void) { mHandler = nullptr; }

    /**
     * Sets the callback handler function pointer and its associated context.
     *
     * @param[in] aHandler   The handler function pointer.
     * @param[in] aContext   The context associated with handler.
     */
    void Set(HandlerType aHandler, void *aContext)
    {
        mHandler = aHandler;
        mContext = aContext;
    }

    /**
     * Indicates whether or not the callback is set (not `nullptr`).
     *
     * @retval TRUE   The handler is set.
     * @retval FALSE  The handler is not set.
     */
    bool IsSet(void) const { return (mHandler != nullptr); }

    /**
     * Returns the handler function pointer.
     *
     * @returns The handler function pointer.
     */
    HandlerType GetHandler(void) const { return mHandler; }

    /**
     * Returns the context associated with callback.
     *
     * @returns The context.
     */
    void *GetContext(void) const { return mContext; }

    /**
     * Indicates whether the callback matches a given handler function pointer and context.
     *
     * @param[in] aHandler   The handler function pointer to compare with.
     * @param[in] aContext   The context associated with handler.
     *
     * @retval TRUE   The callback matches @p aHandler and @p aContext.
     * @retval FALSE  The callback does not match @p aHandler and @p aContext.
     */
    bool Matches(HandlerType aHandler, void *aContext) const
    {
        return (mHandler == aHandler) && (mContext == aContext);
    }

protected:
    CallbackBase(void)
        : mHandler(nullptr)
        , mContext(nullptr)
    {
    }

    HandlerType mHandler;
    void       *mContext;
};

/**
 * Represents a `Callback` (a function pointer handler and a `void *` context).
 *
 * The context is passed as one of the arguments to the function pointer handler when invoked.
 *
 * The `Callback` provides two specializations based on `CallbackContextPosition` in the function pointer, i.e., whether
 * it is passed as the first argument or as the last argument.
 *
 * The `CallbackContextPosition` template parameter is automatically determined at compile-time based on the given
 * `HandlerType`. So user can simply use `Callback<HandlerType>`. The `Invoke()` method will properly pass the context
 * to the function handler.
 *
 * @tparam  HandlerType                The function pointer handler type.
 * @tparam  CallbackContextPosition    Context position (first or last). Automatically determined at compile-time.
 */
template <typename HandlerType,
          CallbackContextPosition =
              (TypeTraits::IsSame<typename TypeTraits::FirstArgTypeOf<HandlerType>::Type, void *>::kValue
                   ? kContextAsFirstArg
                   : kContextAsLastArg)>
class Callback
{
};

// Specialization for `kContextAsLastArg`
template <typename HandlerType> class Callback<HandlerType, kContextAsLastArg> : public CallbackBase<HandlerType>
{
    using CallbackBase<HandlerType>::mHandler;
    using CallbackBase<HandlerType>::mContext;

public:
    using ReturnType = typename TypeTraits::ReturnTypeOf<HandlerType>::Type; ///< Return type of `HandlerType`.

    static constexpr CallbackContextPosition kContextPosition = kContextAsLastArg; ///< Context position.

    /**
     * Initializes `Callback` as empty (`nullptr` handler function pointer).
     */
    Callback(void) = default;

    /**
     * Invokes the callback handler.
     *
     * The caller MUST ensure that callback is set (`IsSet()` returns `true`) before calling this method.
     *
     * @param[in] aArgs   The args to pass to the callback handler.
     *
     * @returns The return value from handler.
     */
    template <typename... Args> ReturnType Invoke(Args &&...aArgs) const
    {
        return mHandler(static_cast<Args &&>(aArgs)..., mContext);
    }

    /**
     * Invokes the callback handler if it is set.
     *
     * The method MUST be used when the handler function returns `void`.
     *
     * @param[in] aArgs   The args to pass to the callback handler.
     */
    template <typename... Args> void InvokeIfSet(Args &&...aArgs) const
    {
        static_assert(TypeTraits::IsSame<ReturnType, void>::kValue,
                      "InvokeIfSet() MUST be used with `void` returning handler");

        if (mHandler != nullptr)
        {
            Invoke(static_cast<Args &&>(aArgs)...);
        }
    }

    /**
     * Invokes the callback handler if it is set and clears it.
     *
     * The method MUST be used when the handler function returns `void`.
     *
     * The callback is cleared first before invoking its handler to allow it to be set again from the handler
     * implementation.
     *
     * @param[in] aArgs   The args to pass to the callback handler.
     */
    template <typename... Args> void InvokeAndClearIfSet(Args &&...aArgs)
    {
        Callback<HandlerType, kContextAsLastArg> callbackCopy = *this;

        CallbackBase<HandlerType>::Clear();
        callbackCopy.InvokeIfSet(static_cast<Args &&>(aArgs)...);
    }
};

// Specialization for `kContextAsFirstArg`
template <typename HandlerType> class Callback<HandlerType, kContextAsFirstArg> : public CallbackBase<HandlerType>
{
    using CallbackBase<HandlerType>::mHandler;
    using CallbackBase<HandlerType>::mContext;

public:
    using ReturnType = typename TypeTraits::ReturnTypeOf<HandlerType>::Type;

    static constexpr CallbackContextPosition kContextPosition = kContextAsFirstArg;

    Callback(void) = default;

    template <typename... Args> ReturnType Invoke(Args &&...aArgs) const
    {
        return mHandler(mContext, static_cast<Args &&>(aArgs)...);
    }

    template <typename... Args> void InvokeIfSet(Args &&...aArgs) const
    {
        static_assert(TypeTraits::IsSame<ReturnType, void>::kValue,
                      "InvokeIfSet() MUST be used with `void` returning handler");

        if (mHandler != nullptr)
        {
            Invoke(static_cast<Args &&>(aArgs)...);
        }
    }

    /**
     * Invokes the callback handler if it is set and clears it.
     *
     * The method MUST be used when the handler function returns `void`.
     *
     * The callback is cleared first before invoking its handler to allow it to be set again from the handler
     * implementation.
     *
     * @param[in] aArgs   The args to pass to the callback handler.
     */
    template <typename... Args> void InvokeAndClearIfSet(Args &&...aArgs)
    {
        Callback<HandlerType, kContextAsFirstArg> callbackCopy = *this;

        CallbackBase<HandlerType>::Clear();
        callbackCopy.InvokeIfSet(static_cast<Args &&>(aArgs)...);
    }
};

} // namespace ot

#endif // CALLBACK_HPP_
