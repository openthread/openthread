/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file provides a compile time counter and utilities based on it.
 */

#ifndef OT_UTILS_STATIC_COUNTER_HPP_
#define OT_UTILS_STATIC_COUNTER_HPP_

namespace ot {

template <int N> struct StaticInt : public StaticInt<N - 1>
{
public:
    static constexpr int kValue = N;
};

template <> struct StaticInt<0>
{
public:
    static constexpr int kValue = 0;
};

#define StaticCounterInit(N) static constexpr StaticInt<N> StaticCounter(StaticInt<N>)

#define StaticCounterValue() decltype(StaticCounter(StaticInt<255>{}))::kValue

#define StaticCounterIncr() \
    static constexpr StaticInt<StaticCounterValue() + 1> StaticCounter(StaticInt<StaticCounterValue() + 1>)

#define InitEnumValidatorCounter() StaticCounterInit(0)

#define ValidateNextEnum(kEnumartor)                                                      \
    static_assert(kEnumartor == StaticCounterValue(), #kEnumartor " value is incorrect"); \
    StaticCounterIncr()

#define SkipNextEnum() StaticCounterIncr()

} // namespace ot

#endif // OT_UTILS_STATIC_COUNTER_HPP_
