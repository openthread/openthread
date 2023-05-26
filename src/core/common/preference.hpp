/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes definitions for a signed preference value and its 2-bit unsigned representation used by
 *   Route Preference in RFC-4191.
 */

#ifndef PREFERENCE_HPP_
#define PREFERENCE_HPP_

#include "openthread-core-config.h"

#include "common/error.hpp"

namespace ot {

/**
 * Provides constants and static methods to convert between `int8_t` preference and its 2-bit unsigned
 * representation.
 *
 */
class Preference
{
public:
    static constexpr int8_t kHigh   = 1;  ///< High preference.
    static constexpr int8_t kMedium = 0;  ///< Medium preference.
    static constexpr int8_t kLow    = -1; ///< Low preference.

    /**
     * Converts a signed preference value to its corresponding 2-bit `uint8_t` value.
     *
     * A positive @p aPrf is mapped to "High Preference", a negative @p aPrf is mapped to "Low Preference", and
     * zero @p aPrf is mapped to "Medium Preference".
     *
     * @param[in] aPrf   The preference to convert to `uint8_t`.
     *
     * @returns The 2-bit unsigned value representing @p aPrf.
     *
     */
    static uint8_t To2BitUint(int8_t aPrf);

    /**
     * Converts a 2-bit `uint8_t` value to a signed preference value `kHigh`, `kMedium`, and `kLow`.
     *
     * Only the first two bits (LSB) of @p a2BitUint are used and the rest of the bits are ignored.
     *
     * - `0b01` (or 1) is mapped to `kHigh`.
     * - `0b00` (or 0) is mapped to `kMedium`.
     * - `0b11` (or 3) is mapped to `kLow`.
     * - `0b10` (or 2) is reserved for future and is also mapped to `kMedium` (this complies with RFC-4191 where
     *                 the reserved value `0b10` MUST be treated as `0b00` for Route Preference).
     *
     * @param[in] a2BitUint   The 2-bit unsigned value to convert from. Only two LSB bits are used and the reset are
     *                        ignored.
     *
     * @returns The signed preference `kHigh`, `kMedium`, or `kLow` corresponding to @p a2BitUint.
     *
     */
    static int8_t From2BitUint(uint8_t a2BitUint);

    /**
     * Indicates whether a given `int8_t` preference value is valid, i.e., whether it has of the
     * three values `kHigh`, `kMedium`, or `kLow`.
     *
     * @param[in] aPrf  The signed preference value to check.
     *
     * @retval TRUE   if @p aPrf is valid.
     * @retval FALSE  if @p aPrf is not valid
     *
     */
    static bool IsValid(int8_t aPrf);

    /**
     * Indicates whether a given 2-bit `uint8_t` preference value is valid.
     *
     * @param[in] a2BitUint   The 2-bit unsigned value to convert from. Only two LSB bits are used and the reset are
     *                        ignored.
     *
     * @retval TRUE   if the first 2 bits of @p a2BitUint are `0b00`, `0b01`, or `0b11`.
     * @retval FALSE  if the first 2 bits of @p a2BitUint are `0b01`.
     *
     */
    static bool Is2BitUintValid(uint8_t a2BitUint) { return ((a2BitUint & k2BitMask) != k2BitReserved); }

    /**
     * Converts a given preference to a human-readable string.
     *
     * @param[in] aPrf  The preference to convert.
     *
     * @returns The string representation of @p aPrf.
     *
     */
    static const char *ToString(int8_t aPrf);

    Preference(void) = delete;

private:
    static constexpr uint8_t k2BitMask = 3;

    static constexpr uint8_t k2BitHigh     = 1; // 0b01
    static constexpr uint8_t k2BitMedium   = 0; // 0b00
    static constexpr uint8_t k2BitLow      = 3; // 0b11
    static constexpr uint8_t k2BitReserved = 2; // 0b10
};

} // namespace ot

#endif // PREFERENCE_HPP_
