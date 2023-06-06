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
 *   This file includes definitions for generic number utility functions (min, max, clamp).
 */

#ifndef NUM_UTILS_HPP_
#define NUM_UTILS_HPP_

#include "common/numeric_limits.hpp"
#include "common/type_traits.hpp"

namespace ot {

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
 *
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
 *
 */
template <typename Type> Type Max(Type aFirst, Type aSecond) { return (aFirst < aSecond) ? aSecond : aFirst; }

/**
 * This template function returns clamped version of a given value to a given closed range [min, max].
 *
 * Uses `operator<` to compare the values. The behavior is undefined if the value of @p aMin is greater than @p aMax.
 *
 * @tparam Type   The value type.
 *
 * @param[in] aValue   The value to clamp.
 * @param[in] aMin     The minimum value.
 * @param[in] aMax     The maximum value.
 *
 * @returns The clamped version of @aValue to the closed range [@p aMin, @p aMax].
 *
 */
template <typename Type> Type Clamp(Type aValue, Type aMin, Type aMax)
{
    Type value = Max(aValue, aMin);

    return Min(value, aMax);
}

/**
 * This template function returns a clamped version of given integer to a `uint8_t`.
 *
 * If @p aValue is greater than max value of a `uint8_t`, the max value is returned.
 *
 * @tparam UintType   The value type (MUST be `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in] aValue  The value to clamp.
 *
 * @returns The clamped version of @p aValue to `uint8_t`.
 *
 */
template <typename UintType> uint8_t ClampToUint8(UintType aValue)
{
    static_assert(TypeTraits::IsSame<UintType, uint16_t>::kValue || TypeTraits::IsSame<UintType, uint32_t>::kValue ||
                      TypeTraits::IsSame<UintType, uint64_t>::kValue,
                  "UintType must be `uint16_t, `uint32_t`, or `uint64_t`");

    return static_cast<uint8_t>(Min(aValue, static_cast<UintType>(NumericLimits<uint8_t>::kMax)));
}

/**
 * This template function returns a clamped version of given integer to a `uint16_t`.
 *
 * If @p aValue is greater than max value of a `uint16_t`, the max value is returned.
 *
 * @tparam UintType   The value type (MUST be `uint32_t`, or `uint64_t`).
 *
 * @param[in] aValue  The value to clamp.
 *
 * @returns The clamped version of @p aValue to `uint16_t`.
 *
 */
template <typename UintType> uint16_t ClampToUint16(UintType aValue)
{
    static_assert(TypeTraits::IsSame<UintType, uint32_t>::kValue || TypeTraits::IsSame<UintType, uint64_t>::kValue,
                  "UintType must be `uint32_t` or `uint64_t`");

    return static_cast<uint16_t>(Min(aValue, static_cast<UintType>(NumericLimits<uint16_t>::kMax)));
}

/**
 * Returns a clamped version of given integer to a `int8_t`.
 *
 * If @p aValue is smaller than min value of a `int8_t`, the min value of `int8_t` is returned.
 * If @p aValue is larger than max value of a `int8_t`, the max value of `int8_t` is returned.
 *
 * @tparam IntType    The value type (MUST be `int16_t`, `int32_t`, or `int64_t`).
 *
 * @param[in] aValue  The value to clamp.
 *
 * @returns The clamped version of @p aValue to `int8_t`.
 *
 */
template <typename IntType> int8_t ClampToInt8(IntType aValue)
{
    static_assert(TypeTraits::IsSame<IntType, int16_t>::kValue || TypeTraits::IsSame<IntType, int32_t>::kValue ||
                      TypeTraits::IsSame<IntType, int64_t>::kValue,
                  "IntType must be `int16_t, `int32_t`, or `int64_t`");

    return static_cast<int8_t>(Clamp(aValue, static_cast<IntType>(NumericLimits<int8_t>::kMin),
                                     static_cast<IntType>(NumericLimits<int8_t>::kMax)));
}

/**
 * This template function performs a three-way comparison between two values.
 *
 * @tparam Type   The value type.
 *
 * @param[in] aFirst  The first value.
 * @param[in] aSecond The second value.
 *
 * @retval 1    If @p aFirst >  @p aSecond.
 * @retval 0    If @p aFirst == @p aSecond.
 * @retval -1   If @p aFirst <  @p aSecond.
 *
 */
template <typename Type> int ThreeWayCompare(Type aFirst, Type aSecond)
{
    return (aFirst == aSecond) ? 0 : ((aFirst > aSecond) ? 1 : -1);
}

/**
 * This is template specialization of three-way comparison between two boolean values.
 *
 * @param[in] aFirst  The first boolean value.
 * @param[in] aSecond The second boolean value.
 *
 * @retval 1    If @p aFirst is true and @p aSecond is false (true > false).
 * @retval 0    If both @p aFirst and @p aSecond are true, or both are false (they are equal).
 * @retval -1   If @p aFirst is false and @p aSecond is true (false < true).
 *
 */
template <> inline int ThreeWayCompare(bool aFirst, bool aSecond)
{
    return (aFirst == aSecond) ? 0 : (aFirst ? 1 : -1);
}

/**
 * This template function divides two numbers and rounds the result to the closest integer.
 *
 * @tparam IntType   The integer type.
 *
 * @param[in] aDividend   The dividend value.
 * @param[in] aDivisor    The divisor value.
 *
 * @return The result of division and rounding to the closest integer.
 *
 */
template <typename IntType> inline IntType DivideAndRoundToClosest(IntType aDividend, IntType aDivisor)
{
    return (aDividend + (aDivisor / 2)) / aDivisor;
}

/**
 * Casts a given `uint32_t` to `unsigned long`.
 *
 * @param[in] aUint32   A `uint32_t` value.
 *
 * @returns The @p aUint32 value as `unsigned long`.
 *
 */
inline unsigned long ToUlong(uint32_t aUint32) { return static_cast<unsigned long>(aUint32); }

/**
 * Counts the number of `1` bits in the binary representation of a given unsigned int bit-mask value.
 *
 * @tparam UintType   The unsigned int type (MUST be `uint8_t`, uint16_t`, uint32_t`, or `uint64_t`).
 *
 * @param[in] aMask   A bit mask.
 *
 * @returns The number of `1` bits in @p aMask.
 *
 */
template <typename UintType> uint8_t CountBitsInMask(UintType aMask)
{
    static_assert(TypeTraits::IsSame<UintType, uint8_t>::kValue || TypeTraits::IsSame<UintType, uint16_t>::kValue ||
                      TypeTraits::IsSame<UintType, uint32_t>::kValue || TypeTraits::IsSame<UintType, uint64_t>::kValue,
                  "UintType must be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`");

    uint8_t count = 0;

    while (aMask != 0)
    {
        aMask &= aMask - 1;
        count++;
    }

    return count;
}

} // namespace ot

#endif // NUM_UTILS_HPP_
