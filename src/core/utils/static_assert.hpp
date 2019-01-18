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
 *   This file includes macros for static assert for C++03.
 */

#ifndef OPENTHREAD_STATIC_ASSERT_HPP_
#define OPENTHREAD_STATIC_ASSERT_HPP_

#ifdef __cplusplus

namespace ot {

template <int> struct StaticAssertError;
template <> struct StaticAssertError<true>
{
    StaticAssertError(...);
};

} // namespace ot

/**
 * This macro does static assert.
 *
 * @param[in]   aExpression An expression to assert.
 * @param[in]   aMessage    A message to describe what is wrong when @p aExpression is false.
 *
 */
#define OT_STATIC_ASSERT(aExpression, aMessage)                                                    \
    enum                                                                                           \
    {                                                                                              \
        StaticAssertError##__LINE__ = sizeof(ot::StaticAssertError<(aExpression) != 0>(aMessage)), \
    }

#endif // __cplusplus

#endif // OPENTHREAD_STATIC_ASSERT_HPP_
