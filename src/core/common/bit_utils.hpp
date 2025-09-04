/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definition for bit manipulation utility functions.
 */

#ifndef BIT_UTILS_HPP_
#define BIT_UTILS_HPP_

#include "common/encoding.hpp"
#include "common/numeric_limits.hpp"
#include "common/type_traits.hpp"

namespace ot {

static constexpr uint8_t kBitsPerByte = 8; ///< Number of bits in a byte.

/**
 * Returns the bit-size (number of bits) of a given type or variable.
 *
 * @param[in] aItem   The item (type or variable or expression) to get the bit-size of.
 *
 * @returns Number of bits of @p aItem.
 */
#define BitSizeOf(aItem) (sizeof(aItem) * kBitsPerByte)

/**
 * Determines number of bytes to represent a given number of bits.
 *
 * @param[in] aBitSize    The bit-size (number of bits).
 *
 * @returns Number of bytes to represent @p aBitSize.
 */
#define BytesForBitSize(aBitSize) static_cast<uint8_t>(((aBitSize) + (kBitsPerByte - 1)) / kBitsPerByte)

/**
 * Counts the number of `1` bits in the binary representation of a given unsigned int bit-mask value.
 *
 * @tparam UintType   The unsigned int type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in] aMask   A bit mask.
 *
 * @returns The number of `1` bits in @p aMask.
 */
template <typename UintType> uint8_t CountBitsInMask(UintType aMask)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    uint8_t count = 0;

    while (aMask != 0)
    {
        aMask &= aMask - 1;
        count++;
    }

    return count;
}

/**
 * Counts the number of consecutive matching bits between two byte arrays.
 *
 * This function compares two byte arrays bit-by-bit, starting from the most significant bit (MSB) of the first byte
 * in each array. The comparison proceeds until a mismatch is found or until a maximum of @p aMaxBitLength bits have
 * been successfully compared.
 *
 * It is the caller's responsibility to ensure that both @p aFirst and @p aSecond point to buffers large enough to
 * contain at least @p aMaxBitLength bits.
 *
 * @param[in] aFirst         A pointer to the first byte array to compare.
 * @param[in] aSecond        A pointer to the second byte array to compare.
 * @param[in] aMaxBitLength  The maximum number of bits to compare from the start of the arrays.
 *
 * @return The number of consecutive matching bits.
 */
uint16_t CountMatchingBits(const uint8_t *aFirst, const uint8_t *aSecond, uint16_t aMaxBitLength);

/**
 * Sets the specified bit in a given integer to 1.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in,out]  aBits       A reference to the integer to modify.
 * @param[in]      aBitOffset  The offset of the bit to set (0 corresponds to the least-significant bit).
 */
template <typename UintType> void SetBit(UintType &aBits, uint8_t aBitOffset)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    aBits |= static_cast<UintType>(static_cast<UintType>(1) << aBitOffset);
}

/**
 * Clears the specified bit in a given integer to 0.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in,out]  aBits       A reference to the integer to modify.
 * @param[in]      aBitOffset  The offset of the bit to clear (0 corresponds to the least-significant bit).
 */
template <typename UintType> void ClearBit(UintType &aBits, uint8_t aBitOffset)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    aBits &= (~(static_cast<UintType>(1) << aBitOffset));
}

/**
 * Gets the value of the specified bit in a given integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in] aBits       The integer to read from.
 * @param[in] aBitOffset  The offset of the bit to read (0 corresponds to the least-significant bit).
 *
 * @returns The value of the specified bit.
 */
template <typename UintType> bool GetBit(UintType aBits, uint8_t aBitOffset)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return (aBits & (static_cast<UintType>(1) << aBitOffset)) != 0;
}

/**
 * Writes the specified bit in a given integer to a given value (0 or 1).
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in,out]  aBits      A reference to the integer to modify.
 * @param[in]      aBitOffset The offset of the bit to write (0 corresponds to the least-significant bit).
 * @param[in]      aValue     The value to write.
 */
template <typename UintType> void WriteBit(UintType &aBits, uint8_t aBitOffset, bool aValue)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    if (aValue)
    {
        SetBit<UintType>(aBits, aBitOffset);
    }
    else
    {
        ClearBit<UintType>(aBits, aBitOffset);
    }
}

/**
 * Gets the offset of the lowest non-zero bit in a given mask.
 *
 * @tparam UintType  The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 *
 * @param[in] aMask  The bitmask to inspect (MUST NOT be zero).
 *
 * @returns The offset of the lowest set bit (0 corresponds to the least-significant bit).
 */
template <typename UintType> inline constexpr uint8_t BitOffsetOfMask(UintType aMask)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return (aMask & 0x1) ? 0 : (1 + BitOffsetOfMask<UintType>(aMask >> 1));
}

/**
 * Writes a value to a specified bit-field within an integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bit mask indicating the field to modify (MUST not be 0). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset to write. If not provided, this is computed from @p kMask.
 *
 * @param[in,out]  aBits   A reference to the integer to modify.
 * @param[in]      aValue  The value to write into the field (the value should not be pre-shifted).
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
void WriteBits(UintType &aBits, UintType aValue)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    aBits = ((aBits & ~kMask) | ((aValue << kOffset) & kMask));
}

/**
 * Writes a value to a specified bit-field within an integer and returns the modified integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to modify (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in] aBits  The original integer value.
 * @param[in] aValue The value to write into the field (it should not be pre-shifted).
 *
 * @returns The integer with the specified bit-field updated.
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType UpdateBits(UintType aBits, UintType aValue)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return ((aBits & ~kMask) | ((aValue << kOffset) & kMask));
}

/**
 * Reads the value of a specified bit-field from an integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to read (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in] aBits   The integer value to read from.
 *
 * @returns The value from the bit-field (shifted to start at bit 0).
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType ReadBits(UintType aBits)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return (aBits & kMask) >> kOffset;
}

/**
 * Writes a value to a specified bit-field within a little-endian integer and returns the modified integer in
 * little-endian format.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to modify (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in]  aBits   The original integer value in little-endian format.
 * @param[in]  aValue  The value to write into the field (it should not be pre-shifted).
 *
 * @returns The updated integer in little-endian format.
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType UpdateBitsLittleEndian(UintType aBits, UintType aValue)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return LittleEndian::HostSwap<UintType>((LittleEndian::HostSwap<UintType>(aBits) & ~kMask) |
                                            ((aValue << kOffset) & kMask));
}

/**
 * Writes a value to a specified bit-field within a big-endian integer and returns the modified integer in big-endian
 * format.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to modify (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in]  aBits   The original integer value in big-endian format.
 * @param[in]  aValue  The value to write into the field (it should not be pre-shifted).
 *
 * @returns The updated integer in big-endian format.
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType UpdateBitsBigEndian(UintType aBits, UintType aValue)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return BigEndian::HostSwap<UintType>((BigEndian::HostSwap<UintType>(aBits) & ~kMask) |
                                         ((aValue << kOffset) & kMask));
}

/**
 * Reads the value of a specified bit-field from a little-endian integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to read (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in] aBits   The integer value in little-endian format to read from.
 *
 * @returns The value from the bit-field (shifted to start at bit 0).
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType ReadBitsLittleEndian(UintType aBits)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return (LittleEndian::HostSwap<UintType>(aBits) & kMask) >> kOffset;
}

/**
 * Reads the value of a specified bit-field from a big-endian integer.
 *
 * @tparam UintType   The value type (MUST be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`).
 * @tparam kMask      The bitmask indicating the field to read (must not be zero). The mask must be pre-shifted.
 * @tparam kOffset    The bit offset of the field. If not provided, this is computed from @p kMask.
 *
 * @param[in] aBits   The integer value in big-endian format to read from.
 *
 * @returns The value from the bit-field (shifted to start at bit 0).
 */
template <typename UintType, UintType kMask, UintType kOffset = BitOffsetOfMask(kMask)>
UintType ReadBitsBigEndian(UintType aBits)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int (8, 16, 32, or 64 bit len)");

    return (BigEndian::HostSwap<UintType>(aBits) & kMask) >> kOffset;
}

} // namespace ot

#endif // BIT_UTILS_HPP_
