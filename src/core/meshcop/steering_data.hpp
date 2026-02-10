/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for MeshCoP Steering Data.
 */

#ifndef OT_CORE_MESHCOP_STEERING_DATA_HPP_
#define OT_CORE_MESHCOP_STEERING_DATA_HPP_

#include "openthread-core-config.h"

#include <openthread/steering_data.h>

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"

namespace ot {
namespace MeshCoP {

class JoinerDiscerner;

/**
 * Represents Steering Data (bloom filter).
 */
class SteeringData : public otSteeringData
{
public:
    static constexpr uint8_t kMinLength = OT_STEERING_DATA_MIN_LENGTH; ///< Minimum Steering Data length (in bytes).
    static constexpr uint8_t kMaxLength = OT_STEERING_DATA_MAX_LENGTH; ///< Maximum Steering Data length (in bytes).

    static constexpr uint16_t kInfoStringSize = 45; ///< Size of `InfoString` to use with `ToString()`.

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Represents the hash bit index values for the bloom filter calculated from a Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     */
    struct HashBitIndexes
    {
        static constexpr uint8_t kNumIndexes = 2; ///< Number of hash bit indexes.

        uint16_t mIndex[kNumIndexes]; ///< The hash bit index array.
    };

    /**
     * Initializes the Steering Data and clears the bloom filter.
     *
     * @param[in]  aLength   The Steering Data length (in bytes).
     *
     * @retval kErrorSuccess      Successfully initialized the Steering Data.
     * @retval kErrorInvalidArgs  @p aLength is not valid.
     */
    Error Init(uint8_t aLength);

    /**
     * Initializes the Steering Data.
     *
     * @param[in]  aLength   The Steering Data length (in bytes).
     * @param[in]  aData     A pointer to a buffer containing the data bytes.
     *
     * @retval kErrorSuccess      Successfully initialized the Steering Data.
     * @retval kErrorInvalidArgs  @p aLength is not valid.
     */
    Error Init(uint8_t aLength, const uint8_t *aData);

    /**
     * Checks whether the Steering Data has a valid length.
     *
     * @returns TRUE if the Steering Data's length is valid (within min to max range), FALSE otherwise.
     */
    bool IsValid(void) const { return IsValueInRange(mLength, kMinLength, kMaxLength); }

    /**
     * Clears the bloom filter (all bits are cleared and no Joiner Id is accepted)..
     *
     * The Steering Data length (bloom filter length) is set to one byte with all bits cleared.
     */
    void Clear(void) { IgnoreError(Init(1)); }

    /**
     * Sets the bloom filter to permit all Joiner IDs.
     *
     * To permit all Joiner IDs, The Steering Data length (bloom filter length) is set to one byte with all bits set.
     */
    void SetToPermitAllJoiners(void);

    /**
     * Returns the Steering Data length (in bytes).
     *
     * @returns The Steering Data length (in bytes).
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     */
    const uint8_t *GetData(void) const { return m8; }

    /**
     * Gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     */
    uint8_t *GetData(void) { return m8; }

    /**
     * Updates the bloom filter adding the given Joiner ID.
     *
     * @param[in]  aJoinerId  The Joiner ID to add to bloom filter.
     *
     * @retval kErrorNone         Successfully updated the bloom filter.
     * @retval kErrorInvalidArgs  The Steering Data's length is invalid.
     */
    Error UpdateBloomFilter(const Mac::ExtAddress &aJoinerId);

    /**
     * Updates the bloom filter adding a given Joiner Discerner.
     *
     * @param[in]  aDiscerner  The Joiner Discerner to add to bloom filter.
     *
     * @retval kErrorNone         Successfully updated the bloom filter.
     * @retval kErrorInvalidArgs  The Steering Data's length is invalid.
     */
    Error UpdateBloomFilter(const JoinerDiscerner &aDiscerner);

    /**
     * Merges the bloom filter by combining it with another steering data filter.
     *
     * Both bloom filters must have valid lengths (non-zero and not exceeding `kMaxLength`).
     *
     * The bloom filter from @p aOther must have a length that is a divisor of the current filter's length.
     *
     * @param[in] aOther  The other bloom filter to combine with current filter.
     *
     * @retval kErrorNone         Successfully merged @p aOther into the current bloom filter.
     * @retval kErrorInvalidArgs  The filter lengths are not valid or they cannot be merged.
     */
    Error MergeBloomFilterWith(const SteeringData &aOther);

    /**
     * Indicates whether the bloom filter is empty (all the bits are cleared).
     *
     * @returns TRUE if the bloom filter is empty, FALSE otherwise.
     */
    bool IsEmpty(void) const { return DoesAllMatch(0); }

    /**
     * Indicates whether the bloom filter permits all Joiner IDs (all the bits are set).
     *
     * @returns TRUE if the bloom filter permits all Joiners IDs, FALSE otherwise.
     */
    bool PermitsAllJoiners(void) const { return (mLength > 0) && DoesAllMatch(kPermitAll); }

    /**
     * Indicates whether the bloom filter contains a given Joiner ID.
     *
     * @param[in] aJoinerId  A Joiner ID.
     *
     * @returns TRUE if the bloom filter contains @p aJoinerId, FALSE otherwise.
     */
    bool Contains(const Mac::ExtAddress &aJoinerId) const;

    /**
     * Indicates whether the bloom filter contains a given Joiner Discerner.
     *
     * @param[in] aDiscerner   A Joiner Discerner.
     *
     * @returns TRUE if the bloom filter contains @p aDiscerner, FALSE otherwise.
     */
    bool Contains(const JoinerDiscerner &aDiscerner) const;

    /**
     * Indicates whether the bloom filter contains the hash bit indexes (derived from a Joiner ID).
     *
     * @param[in]  aIndexes   A hash bit index structure (derived from a Joiner ID).
     *
     * @returns TRUE if the bloom filter contains the Joiner ID mapping to @p aIndexes, FALSE otherwise.
     */
    bool Contains(const HashBitIndexes &aIndexes) const;

    /**
     * Converts the Steering Data to a human-readable string representation.
     *
     * @returns An `InfoString` representation of the Steering Data.
     */
    InfoString ToString(void) const;

    /**
     * Calculates the bloom filter hash bit indexes from a given Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     * @param[in]  aJoinerId  The Joiner ID to calculate the hash bit indexes.
     * @param[out] aIndexes   A reference to a `HashBitIndexes` structure to output the calculated index values.
     */
    static void CalculateHashBitIndexes(const Mac::ExtAddress &aJoinerId, HashBitIndexes &aIndexes);

    /**
     * Calculates the bloom filter hash bit indexes from a given Joiner Discerner.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     * @param[in]  aDiscerner     The Joiner Discerner to calculate the hash bit indexes.
     * @param[out] aIndexes       A reference to a `HashBitIndexes` structure to output the calculated index values.
     */
    static void CalculateHashBitIndexes(const JoinerDiscerner &aDiscerner, HashBitIndexes &aIndexes);

private:
    static constexpr uint8_t kPermitAll = 0xff;

    uint8_t GetNumBits(void) const { return (mLength * kBitsPerByte); }

    uint8_t BitIndex(uint8_t aBit) const { return (mLength - 1 - (aBit / kBitsPerByte)); }
    uint8_t BitFlag(uint8_t aBit) const { return static_cast<uint8_t>(1U << (aBit % kBitsPerByte)); }

    bool GetBit(uint8_t aBit) const { return (m8[BitIndex(aBit)] & BitFlag(aBit)) != 0; }
    void SetBit(uint8_t aBit) { m8[BitIndex(aBit)] |= BitFlag(aBit); }
    void ClearBit(uint8_t aBit) { m8[BitIndex(aBit)] &= ~BitFlag(aBit); }

    bool  DoesAllMatch(uint8_t aMatch) const;
    Error UpdateBloomFilter(const HashBitIndexes &aIndexes);
};

} // namespace MeshCoP

DefineCoreType(otSteeringData, MeshCoP::SteeringData);

} // namespace ot

#endif // OT_CORE_MESHCOP_STEERING_DATA_HPP_
