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
 *   This file includes definitions for a bit-set.
 */

#ifndef OT_CORE_COMMON_BIT_SET_HPP_
#define OT_CORE_COMMON_BIT_SET_HPP_

#include "openthread-core-config.h"

#include "common/bit_utils.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"

namespace ot {

/**
 * @addtogroup core-bit-set
 *
 * @brief
 *   This module includes definitions for bit-set.
 *
 * @{
 */

class Message;
class OffsetRange;

class BitSetUtils
{
protected:
    // Common helpers used by `BitSet<>` templates

    static uint8_t  BitMaskFor(uint16_t aIndex) { return (0x80 >> (aIndex & 7)); }
    static bool     IsAllZero(const uint8_t *aMask, uint16_t aSize);
    static uint16_t CountBits(const uint8_t *aMask, uint16_t aSize);
    static void     FlipBits(uint8_t *aTargetMask, uint16_t aSize, uint16_t aNumBits);
    static bool     IsSubset(const uint8_t *aMask, const uint8_t *aSuperMask, uint16_t aSize);
    static void     Union(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize);
    static void     Subtract(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize);
    static void     Intersect(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize);
    static void     Copy(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize, uint16_t aNumBits);
    static void     Tidy(uint8_t *aTargetMask, uint16_t aSize, uint16_t aNumBits);
    static Error    AppendMask(Message &aMessage, const uint8_t *aMask, uint16_t aSize);
    static Error    ReadMask(const Message     &aMessage,
                             const OffsetRange &aOffsetRange,
                             uint8_t           *aTargetMask,
                             uint16_t           aSize,
                             uint16_t           aNumBits);
};

/**
 * Represents a bit-set.
 *
 * @tparam kNumBits  Specifies the number of bits.
 */
template <uint16_t kNumBits>
class BitSet : public Equatable<BitSet<kNumBits>>, public Clearable<BitSet<kNumBits>>, private BitSetUtils
{
    static_assert(kNumBits > 0, "kNumBits cannot be zero");

public:
    /**
     * Indicates whether a given bit index is contained in the set.
     *
     * The caller MUST ensure that @p aIndex is smaller than `kNumBits`. Otherwise, the behavior of this method is
     * undefined.
     *
     * @param[in] aIndex  The bit index to check
     *
     * @retval TRUE   If the bit index @p aIndex is contained in the set.
     * @retval FALSE  If the bit index @p aIndex is not contained in the set.
     */
    bool Has(uint16_t aIndex) const { return (mMask[aIndex / 8] & BitMaskFor(aIndex)) != 0; }

    /**
     * Adds the given bit index to the set.
     *
     * The caller MUST ensure that @p aIndex is smaller than `kNumBits`. Otherwise, the behavior of this method is
     * undefined.
     *
     * @param[in] aIndex  The bit index to add.
     */
    void Add(uint16_t aIndex) { mMask[aIndex / 8] |= BitMaskFor(aIndex); }

    /**
     * Removes the given bit index from the set.
     *
     * The caller MUST ensure that @p aIndex is smaller than `kNumBits`. Otherwise, the behavior of this method is
     * undefined.
     *
     * @param[in] aIndex  The bit index to remove.
     */
    void Remove(uint16_t aIndex) { mMask[aIndex / 8] &= ~BitMaskFor(aIndex); }

    /**
     * Updates the set by either adding or removing the given bit index.
     *
     * The caller MUST ensure that @p aIndex is smaller than `kNumBits`. Otherwise, the behavior of this method is
     * undefined.
     *
     * @param[in] aIndex  The bit index.
     * @param[in] aToAdd  Boolean indicating whether to add (when set to TRUE) or to remove (when set to FALSE).
     */
    void Update(uint16_t aIndex, bool aToAdd) { aToAdd ? Add(aIndex) : Remove(aIndex); }

    /**
     * Indicates whether or not the set is empty.
     *
     * @retval TRUE   If the set is empty.
     * @retval FALSE  If the set is not empty.
     */
    bool IsEmpty(void) const { return IsAllZero(mMask, kMaskSize); }

    /**
     * Returns the number of elements (bits set to TRUE) in the set.
     *
     * @returns The number of elements in the set.
     */
    uint16_t CountElements(void) const { return CountBits(mMask, kMaskSize); }

    /**
     * Indicates whether the set is a subset of another set.
     *
     * @param[in] aOther  The other set to check against.
     *
     * @retval TRUE   If the set is a subset of @p aOther.
     * @retval FALSE  If the set is not a subset of @p aOther.
     */
    bool IsSubsetOf(const BitSet<kNumBits> &aOther) const { return IsSubset(mMask, aOther.mMask, kMaskSize); }

    /**
     * Indicates whether the set is a superset of another set.
     *
     * @param[in] aOther  The other set to check against.
     *
     * @retval TRUE   If the set is a superset of @p aOther.
     * @retval FALSE  If the set is not a superset of @p aOther.
     */
    bool IsSupersetOf(const BitSet<kNumBits> &aOther) const { return aOther.IsSubsetOf(*this); }

    /**
     * Complements the set (flips all bits in the mask).
     */
    void Complement(void) { FlipBits(mMask, kMaskSize, kNumBits); }

    /**
     * Performs a union operation with another set.
     *
     * @param[in] aOther  The other set.
     */
    void UnionWith(const BitSet<kNumBits> &aOther) { Union(mMask, aOther.mMask, kMaskSize); }

    /**
     * Performs an intersection operation with another set.
     *
     * @param[in] aOther  The other set.
     */
    void IntersectWith(const BitSet<kNumBits> &aOther) { Intersect(mMask, aOther.mMask, kMaskSize); }

    /**
     * Performs a subtraction operation with another set.
     *
     * @param[in] aOther  The other set.
     */
    void SubtractWith(const BitSet<kNumBits> &aOther) { Subtract(mMask, aOther.mMask, kMaskSize); }

    /**
     * Gets a pointer to the byte array representing the bit mask.
     *
     * @returns A pointer to the byte array representing the bit mask.
     */
    const uint8_t *GetMaskBytes(void) const { return mMask; }

    /**
     * Gets the size of the bit mask in bytes.
     *
     * @returns The size of the bit mask in bytes.
     */
    uint16_t GetMaskSize(void) const { return kMaskSize; }

    /**
     * Gets the maximum number of bits in the set (`kNumBits` template parameter).
     *
     * @returns The maximum number of bits in the set.
     */
    uint16_t GetNumBits(void) const { return kNumBits; }

    /**
     * Sets the bit mask from a given byte array.
     *
     * @param[in] aMask  A pointer to the byte array to copy from. MUST contain proper size
     */
    void SetMask(const uint8_t *aMask) { Copy(mMask, aMask, kMaskSize, kNumBits); }

    /**
     * Appends the bit mask bytes to a message.
     *
     * @param[in] aMessage  The message to append to.
     *
     * @retval kErrorNone    Successfully appended the bit mask.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    Error AppendTo(Message &aMessage) const { return AppendMask(aMessage, mMask, kMaskSize); }

    /**
     * Reads the bit mask bytes from a message.
     *
     * @param[in]  aMessage      The message to read from.
     * @param[in]  aOffsetRange  The offset range in the message to read from.
     *
     * @retval kErrorNone   Successfully read the bit mask.
     * @retval kErrorParse  Failed to read the bit mask.
     */
    Error ReadFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
    {
        return ReadMask(aMessage, aOffsetRange, mMask, kMaskSize, kNumBits);
    }

private:
    static constexpr uint16_t kMaskSize = BytesForBitSize(kNumBits);

    uint8_t mMask[kMaskSize];
};

/**
 * @}
 */

} // namespace ot

#endif // OT_CORE_COMMON_BIT_SET_HPP_
