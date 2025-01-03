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

#ifndef BIT_SET_HPP_
#define BIT_SET_HPP_

#include "openthread-core-config.h"

#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/numeric_limits.hpp"

namespace ot {

/**
 * @addtogroup core-bit-set
 *
 * @brief
 *   This module includes definitions for bit-set.
 *
 * @{
 */

/**
 * Represents a bit-set.
 *
 * @tparam kNumBits  Specifies the number of bits.
 */
template <uint16_t kNumBits> class BitSet : public Equatable<BitSet<kNumBits>>, public Clearable<BitSet<kNumBits>>
{
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
    bool IsEmpty(void) const
    {
        bool isEmpty = true;

        for (uint8_t byte : mMask)
        {
            if (byte != 0)
            {
                isEmpty = false;
                break;
            }
        }

        return isEmpty;
    }

private:
    static uint8_t BitMaskFor(uint16_t aIndex) { return (0x80 >> (aIndex & 7)); }

    uint8_t mMask[BytesForBitSize(kNumBits)];
};

/**
 * @}
 */

} // namespace ot

#endif // BIT_SET_HPP_
