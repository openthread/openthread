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
 *   This file includes definitions for generic number utility functions (min, max).
 */

#ifndef LIB_UTILS_MATH_HPP_
#define LIB_UTILS_MATH_HPP_

namespace ot {
namespace Lib {
namespace Utils {

/**
 * This template function returns the minimum of two given values.
 *
 * Uses `operator<` to compare the values.
 *
 * @tparam Type   The value type.
 *
 * @param[in] aFirst  The first value.
 * @param[in] aSecond The second value.
 *
 * @returns The minimum of @p aFirst and @p aSecond.
 */
template <typename Type> Type Min(Type aFirst, Type aSecond) { return (aFirst < aSecond) ? aFirst : aSecond; }

/**
 * This template function returns the maximum of two given values.
 *
 * Uses `operator<` to compare the values.
 *
 * @tparam Type   The value type.
 *
 * @param[in] aFirst  The first value.
 * @param[in] aSecond The second value.
 *
 * @returns The maximum of @p aFirst and @p aSecond.
 */
template <typename Type> Type Max(Type aFirst, Type aSecond) { return (aFirst < aSecond) ? aSecond : aFirst; }

/**
 * Casts a given `uint32_t` to `unsigned long`.
 *
 * @param[in] aUint32   A `uint32_t` value.
 *
 * @returns The @p aUint32 value as `unsigned long`.
 */
inline unsigned long ToUlong(uint32_t aUint32) { return static_cast<unsigned long>(aUint32); }

} // namespace Utils
} // namespace Lib
} // namespace ot

#endif // LIB_UTILS_MATH_HPP_
