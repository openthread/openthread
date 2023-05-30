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
 *   This file includes definitions for serial number comparison similar to RFC-1982.
 */

#ifndef SERIAL_NUMBER_HPP_
#define SERIAL_NUMBER_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include "common/numeric_limits.hpp"
#include "common/type_traits.hpp"

namespace ot {

class SerialNumber
{
public:
    /**
     * Indicates whether or not a first serial number is strictly less than a second serial number.
     *
     * The comparison takes into account the wrapping of serial number values (similar to RFC-1982). It is semantically
     * equivalent to `aFirst < aSecond`.
     *
     * @tparam UintType    The unsigned integer type.
     *
     * @param[in] aFirst   The first serial number.
     * @param[in] aSecond  The second serial number.
     *
     * @retval TRUE  If @p aFirst is less than @p aSecond.
     * @retval FALSE If @p aFirst is not less than @p aSecond.
     *
     */
    template <typename UintType> static bool IsLess(UintType aFirst, UintType aSecond)
    {
        static_assert(TypeTraits::IsSame<UintType, uint8_t>::kValue || TypeTraits::IsSame<UintType, uint16_t>::kValue ||
                          TypeTraits::IsSame<UintType, uint32_t>::kValue ||
                          TypeTraits::IsSame<UintType, uint64_t>::kValue,
                      "UintType MUST be an 8, 16, 32, or 64 bit `uint` type");

        static constexpr UintType kNegativeMask = (NumericLimits<UintType>::kMax >> 1) + 1;

        return ((aFirst - aSecond) & kNegativeMask) != 0;
    }

    /**
     * Indicates whether or not a first serial number is strictly greater than a second serial
     * number.
     *
     * The comparison takes into account the wrapping of serial number values (similar to RFC-1982). It is semantically
     * equivalent to `aFirst > aSecond`.
     *
     * @tparam UintType    The unsigned integer type.
     *
     * @param[in] aFirst   The first serial number.
     * @param[in] aSecond  The second serial number.
     *
     * @retval TRUE  If @p aFirst is greater than @p aSecond.
     * @retval FALSE If @p aFirst is not greater than @p aSecond.
     *
     */
    template <typename UintType> static bool IsGreater(UintType aFirst, UintType aSecond)
    {
        return IsLess(aSecond, aFirst);
    }
};

} // namespace ot

#endif // SERIAL_NUMBER_HPP_
