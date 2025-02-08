/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for CRC computations.
 */

#ifndef CRC_HPP_
#define CRC_HPP_

#include "openthread-core-config.h"

#include "common/message.hpp"
#include "common/type_traits.hpp"

namespace ot {

constexpr uint16_t kCrc16CcittPolynomial = 0x1021; ///< CRC16-CCITT Polynomial (x^16 + x^12 + x^5 + 1)
constexpr uint16_t kCrc16AnsiPolynomial  = 0x8005; ///< CRC16-ANSI Polynomial  (x^16 + x^15 + x^2 + 1)

/**
 * CRC32-ANSI Polynomial
 *
 * (x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1)
 */
constexpr uint32_t kCrc32AnsiPolynomial = 0x04c11db7;

/**
 * Implements CRC computations.
 *
 * @tparam UintType  The unsigned int type indicating CRC bit width. MUST be either `uint16_t` or `uint32_t`.
 */
template <typename UintType> class CrcCalculator
{
    constexpr static bool kIsUint16 = TypeTraits::IsSame<UintType, uint16_t>::kValue;
    constexpr static bool kIsUint32 = TypeTraits::IsSame<UintType, uint32_t>::kValue;

    static_assert(kIsUint16 || kIsUint32, "UintType MUST be either `uint16_t` or `uint32_t`");

public:
    /**
     * Initializes the `CrcCalculator` object.
     *
     * @param[in]  aPolynomial  The polynomial to use for CRC calculation.
     */
    explicit CrcCalculator(UintType aPolynomial)
        : mPolynomial(aPolynomial)
        , mCrc(0)
    {
    }

    /**
     * Gets the current CRC value.
     *
     * @returns The current CRC value.
     */
    UintType GetCrc(void) const { return mCrc; }

    /**
     * Feeds a byte value into the CRC computation.
     *
     * @param[in]  aByte  The byte value.
     *
     * @returns The current CRC value.
     */
    UintType FeedByte(uint8_t aByte);

    /**
     * Feeds a sequence of bytes into the CRC computation.
     *
     * @param[in]  aBytes   A pointer to buffer containing the bytes.
     * @param[in]  aLength  Number of bytes in @p aBytes.
     *
     * @returns The current CRC value.
     */
    UintType FeedBytes(const void *aBytes, uint16_t aLength);

    /**
     * Feed an object (all its bytes) into the CRC computation.
     *
     * @tparam    ObjectType   The object type.
     *
     * @param[in] aObject      A reference to the object.
     *
     * @returns The current CRC value.
     */
    template <typename ObjectType> UintType Feed(const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");
        return FeedBytes(&aObject, sizeof(ObjectType));
    }

    /**
     * Feed bytes read from a given message within a given offset range into the CRC computation.
     *
     * @param[in] aMessage       The message to read from.
     * @param[in] aOffsetRaneg   The offset range in @p aMessage to read bytes from.
     *
     * @returns The current CRC value.
     */
    UintType Feed(const Message &aMessage, const OffsetRange &aOffsetRange);

private:
    UintType mPolynomial;
    UintType mCrc;
};

} // namespace ot

#endif // CRC_HPP_
