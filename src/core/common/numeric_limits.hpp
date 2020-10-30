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
 *   This file includes definitions for NumericLimits class.
 */

#ifndef NUMERIC_LIMITS_HPP_
#define NUMERIC_LIMITS_HPP_

#include <stdint.h>

namespace ot {

/**
 * This class template provides a way to query properties of arithmetic types.
 *
 * There are no members if `Type` is not a supported arithmetic type.
 *
 */
template <typename Type> class NumericLimits
{
};

template <> class NumericLimits<int8_t>
{
public:
    static constexpr int8_t Min(void) { return INT8_MIN; }
    static constexpr int8_t Max(void) { return INT8_MAX; }
};

template <> class NumericLimits<int16_t>
{
public:
    static constexpr int16_t Min(void) { return INT16_MIN; }
    static constexpr int16_t Max(void) { return INT16_MAX; }
};

template <> class NumericLimits<int32_t>
{
public:
    static constexpr int32_t Min(void) { return INT32_MIN; }
    static constexpr int32_t Max(void) { return INT32_MAX; }
};

template <> class NumericLimits<int64_t>
{
public:
    static constexpr int64_t Min(void) { return INT64_MIN; }
    static constexpr int64_t Max(void) { return INT64_MAX; }
};

template <> class NumericLimits<uint8_t>
{
public:
    static constexpr uint8_t Min(void) { return 0; }
    static constexpr uint8_t Max(void) { return UINT8_MAX; }
};

template <> class NumericLimits<uint16_t>
{
public:
    static constexpr uint16_t Min(void) { return 0; }
    static constexpr uint16_t Max(void) { return UINT16_MAX; }
};

template <> class NumericLimits<uint32_t>
{
public:
    static constexpr uint32_t Min(void) { return 0; }
    static constexpr uint32_t Max(void) { return UINT32_MAX; }
};

template <> class NumericLimits<uint64_t>
{
public:
    static constexpr uint64_t Min(void) { return 0; }
    static constexpr uint64_t Max(void) { return UINT64_MAX; }
};

} // namespace ot

#endif // NUMERIC_LIMITS_HPP_
