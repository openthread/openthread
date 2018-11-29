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
 *  This file defines OpenThread String class.
 */

#ifndef SET_HPP_
#define SET_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/string.hpp"

namespace ot {

/**
 * @addtogroup core-set
 *
 * @brief
 *   This module includes definitions for OpenThread Set (of integers).
 *
 * @{
 *
 */

/**
 * This class defines the common (non-template) APIs and types inherited by `Set` class.
 *
 */
class SetApi
{
public:
    enum
    {
        kSetIteratorFirst = 0xffff, ///< Element value passed to `GetNextElement()` to get the first element of the set.
        kInfoStringSize   = 100,    ///< Max chars for the info string (`ToString()`).
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

protected:
    SetApi(void) {}
    static bool       IsEmpty(const uint8_t *aMaskArray, uint16_t aMaskArrayLength);
    static void       Itersect(uint8_t *aMaskArray, const uint8_t *aOtherMaskArray, uint16_t aMaskArrayLength);
    static void       Union(uint8_t *aMaskArray, const uint8_t *aOtherMaskArray, uint16_t aMaskArrayLength);
    static uint16_t   GetNumberOfElements(const uint8_t *aMaskArray, uint16_t aMaskArrayLength);
    static otError    GetNextElement(const uint8_t *aMaskArray, uint16_t &aElement, uint16_t aMaxSize);
    static InfoString ToString(const uint8_t *aMaskArray, uint16_t aMaxSize);
};

/**
 * This class defines a `Set` of integers capable of storing elements from `0` up to but not including `kMaxSize`.
 *
 * The template parameter `kMaxSize` specifies the maximum set size (maximum number of elements) and valid range of
 * values that can stored in the set.
 *
 * This is base template class for `Set` providing all common methods.
 *
 */
template <uint16_t kMaxSize> class SetBase : public SetApi
{
public:
    /**
     * This method clears the set.
     *
     */
    void Clear(void) { memset(mMaskArray, 0, kMaskArrayLength); }

    /**
     * This method indicates whether the set is empty or not.
     *
     * @retval TRUE     Set is empty.
     * @retval FALSE    Set is not empty.
     *
     */
    bool IsEmpty(void) const { return SetApi::IsEmpty(mMaskArray, kMaskArrayLength); }

    /**
     * This method indicates whether or not the set contains a given element.
     *
     * @note The given element @p aElement MUST be within valid range of `0` up to but not including `kMaxSize`.
     * Otherwise, the behavior of this method is undefined.
     *
     * @param[in] aElement   Element to check.
     *
     * @retval TRUE   The element is contained in the set.
     * @retval FALSE  The element is not contained in the set.
     *
     */
    bool Contains(uint16_t aElement) const { return (Mask(aElement) & Bit(aElement)) != 0; }

    /**
     * This method adds an element to the set.
     *
     * @note The given element @p aElement MUST be within valid range of `0` up to but not including `kMaxSize`.
     * Otherwise, the behavior of this method is undefined.
     *
     * @param[in] aElement   Element to add.
     *
     */
    void Add(uint16_t aElement) { Mask(aElement) |= Bit(aElement); }

    /**
     * This method removes an element from the set.
     *
     * @note The given element @p aElement MUST be within valid range of `0` up to but not including `kMaxSize`.
     * Otherwise, the behavior of this method is undefined.
     *
     * @param[in] aElement   Element to remove.
     *
     */
    void Remove(uint16_t aElement) { Mask(aElement) &= ~Bit(aElement); }

    /**
     * This method flips an element in the set (i.e. it is added if not in set, removed if in the set).
     *
     * @note The given element @p aElement MUST be within valid range of `0` up to but not including `kMaxSize`.
     * Otherwise, the behavior of this method is undefined.
     *
     * @param[in] aElement   Element to flip.
     *
     */
    void Flip(uint16_t aElement) { Mask(aElement) ^= Bit(aElement); }

    /**
     * This method updates the set to be intersection of the current set with another given set.
     *
     * @param[in] aAnother   Another set to perform intersect operation.
     *
     */
    void Intersect(const SetBase &aAnother) { SetApi::Itersect(mMaskArray, aAnother.mMaskArray, kMaskArrayLength); }

    /**
     * This method updates the set to be the union of the current set and another given set.
     *
     * @param[in] aAnother   Another set to perform union operation.
     *
     */
    void Union(const SetBase &aAnother) { SetApi::Union(mMaskArray, aAnother.mMaskArray, kMaskArrayLength); }

    /**
     * This method gets the number of elements in the set.
     *
     * @returns  Number of elements in the set.
     *
     */
    uint16_t GetNumberOfElements(void) const { return SetApi::GetNumberOfElements(mMaskArray, kMaskArrayLength); }

    /**
     * This method gets the next element in the set given a previous one.
     *
     * This method is used to iterate over all elements in the set.
     *
     * @param[inout] aElement   Reference to an element variable.
     *                          On entry it can be `kSetIteratorFirst` to get the first element of the set, or
     *                          a previous element. On exit, the next element in set is returned in this variable.
     *
     * @retval OT_ERROR_NONE       Successfully iterated to next element in set, @p aElement is updated.
     * @retval OT_ERROR_NOT_FOUND  No more element in the set.
     *
     */
    otError GetNextElement(uint16_t &aElement) const { return SetApi::GetNextElement(mMaskArray, aElement, kMaxSize); }

    /**
     * This method overloads `==` operator to indicate whether two sets are equal.
     *
     * @param[in] aAnother   A reference to another set to compare with the current one.
     *
     * @returns TRUE if the two set are equal, FALSE otherwise.
     *
     */
    bool operator==(const SetBase &aAnother) const
    {
        return memcmp(mMaskArray, aAnother.mMaskArray, kMaskArrayLength) == 0;
    }

    /**
     * This method overloads `!=` operator to indicate whether two sets are not equal.
     *
     * @param[in] aAnother   A reference to another set to compare with the current one.
     *
     * @returns TRUE if the two set are not equal, FALSE otherwise.
     *
     */
    bool operator!=(const SetBase &aAnother) const
    {
        return memcmp(mMaskArray, aAnother.mMaskArray, kMaskArrayLength) != 0;
    }

    /**
     * This method returns the maximum set size (maximum number of elements in the set).
     *
     * @note The elements in the set must be within range of `0` up to but not including `kMaxSize`.
     *
     * @returns The maximum set size.
     *
     */
    uint16_t GetMaxSize(void) const { return kMaxSize; }

    /**
     * This method converts the set into a human-readable string.
     *
     * Examples of possible output:
     *  -  empty set       ->  "{ }"
     *  -  ranges          ->  "{ 11-26 }"
     *  -  single element  ->  "{ 20 }"
     *  -  multiple ranges ->  "{ 11, 14-17, 20-22, 24, 25 }"
     *  -  no range        ->  "{ 14, 21, 26 }"
     *
     * @returns  An `InfoString` object representing the set.
     *
     */
    InfoString ToString(void) const { return SetApi::ToString(mMaskArray, kMaxSize); }

protected:
    enum
    {
        kMaskArrayLength = BitVectorBytes(kMaxSize), ///< Length of bitmask array (number of bytes)
    };

    /**
     * This constructor initializes the object
     *
     * Protected constructor to ensure only sub-classes can be instantiated.
     *
     */
    SetBase(void)
        : SetApi()
    {
        Clear();
    }

    // Returns a bitmask corresponding to a given element.
    uint8_t Bit(uint16_t aElement) const { return static_cast<uint8_t>(1U << (aElement & 7)); }

    // Returns a reference to the byte in the mask array corresponding to the given element in set.
    uint8_t &Mask(uint16_t aElement) { return mMaskArray[aElement >> 3]; }

    // Returns a const reference to the byte in the mask array corresponding to the given element in set.
    const uint8_t &Mask(uint16_t aElement) const { return mMaskArray[aElement >> 3]; }

    uint8_t mMaskArray[kMaskArrayLength];
};

/**
 * This class defines a set of integers (capable of storing elements from `0` up to but not including `kMaxSize`).
 *
 * The template parameter `kMaxSize` specifies the range of valid values that can stored in the set.
 *
 */
template <uint16_t kMaxSize> class Set : public SetBase<kMaxSize>
{
public:
    /**
     * This constructor initializes the set object
     *
     * Set starts as empty.
     *
     */
    Set(void)
        : SetBase<kMaxSize>()
    {
    }
};

/**
 * This definition is a template customization of `Set<>` class for size 16.
 *
 */
template <> class Set<16> : public SetBase<16>
{
public:
    /**
     * This constructor initializes the set object
     *
     * The set starts as empty.
     *
     */
    Set(void)
        : SetBase<16>()
    {
    }

    /**
     * This constructor initializes the set object from a given `uint16_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     * @param[in] aBitMask   A `uint16_t` bit-vector mask.
     *
     */
    explicit Set(uint16_t aBitMask)
        : SetBase<16>()
    {
        SetFromMask(aBitMask);
    }

    /**
     * This method converts the set into `uint16_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     * @returns A bit-vector mask corresponding to elements in the set.
     *
     */
    uint16_t GetAsMask(void) const { return Encoding::LittleEndian::ReadUint16(mMaskArray); }

    /**
     * This method populates the set from a given `uint16_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     */
    void SetFromMask(uint16_t aBitMask) { Encoding::LittleEndian::WriteUint16(aBitMask, mMaskArray); }
};

/**
 * This definition is a template customization of `Set<>` class for size 32.
 *
 */
template <> class Set<32> : public SetBase<32>
{
public:
    /**
     * This constructor initializes the set object
     *
     * The set starts as empty.
     *
     */
    Set(void)
        : SetBase<32>()
    {
    }

    /**
     * This constructor initializes the set object from a given `uint32_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     * @param[in] aBitMask   A `uint32_t` bit-vector mask.
     *
     */
    explicit Set(uint32_t mBitMask)
        : SetBase<32>()
    {
        SetFromMask(mBitMask);
    }

    /**
     * This method converts the set into `uint32_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     * @returns A bit-vector mask corresponding to elements in the set.
     *
     */
    uint32_t GetAsMask(void) const { return Encoding::LittleEndian::ReadUint32(mMaskArray); }

    /**
     * This method populates the set from a given `uint32_t` bit-vector mask (little endian order).
     *
     * Bit 0 (LSB) in the bit-vector mask corresponds to element 0, bit 1 to element 1, and so on.
     *
     */
    void SetFromMask(uint32_t aBitMask) { Encoding::LittleEndian::WriteUint32(aBitMask, mMaskArray); }
};

/**
 * @}
 *
 */

} // namespace ot

#endif // OT_SET_HPP_
