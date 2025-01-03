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
 *   This file includes definitions for an offset range.
 */

#ifndef OFFSET_RANGE_HPP_
#define OFFSET_RANGE_HPP_

#include "openthread-core-config.h"

#include <stdbool.h>
#include <stdint.h>

#include "common/clearable.hpp"

namespace ot {

class Message;

/**
 * Represents an offset range.
 */
class OffsetRange : public Clearable<OffsetRange>
{
public:
    /**
     * Initializes the `OffsetRange`.
     *
     * @param[in] aOffset   The start offset.
     * @param[in] aLength   The range length (number of bytes).
     */
    void Init(uint16_t aOffset, uint16_t aLength);

    /**
     * Initializes the `OffsetRange` from given start and end offsets.
     *
     * The range is inclusive of the start offset (@p aStartOffset) but exclusive of the end offset (@p aEndOffset).
     *
     * @param[in]  aStartOffset The start offset (inclusive).
     * @param[in]  aEndOffset   The end offset (exclusive).
     */
    void InitFromRange(uint16_t aStartOffset, uint16_t aEndOffset);

    /**
     * Initializes the `OffsetRange` from a given `Message` from its offset to the message end.
     *
     * The start offset of the range is set to `aMessage.GetOffset()`, and the end offset is set to include all bytes
     * in the message up to its current length `aMessage.GetLength()`.
     *
     * @param[in] aMessage    The `Message` to initialize the `OffsetRange` from.
     */
    void InitFromMessageOffsetToEnd(const Message &aMessage);

    /**
     * Initializes the `OffsetRange` from a given `Message` from zero offset up to to its full length.
     *
     * The start offset of the range is set to zero, and the end offset is set to include full length of @p aMessage.
     *
     * @param[in] aMessage    The `Message` to initialize the `OffsetRange` from.
     */
    void InitFromMessageFullLength(const Message &aMessage);

    /**
     * Gets the start offset of the `OffsetRange`
     *
     * @returns The start offset.
     */
    uint16_t GetOffset(void) const { return mOffset; }

    /**
     * Gets the end offset of the `OffsetRange`.
     *
     * This offset is exclusive, meaning it marks the position immediately after the last byte within the range.
     *
     * @returns The end offset.
     */
    uint16_t GetEndOffset(void) const { return (mOffset + mLength); }

    /**
     * Gets the `OffsetRange` length.
     *
     * @returns The length of the `OffsetRange` in bytes.
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Indicates whether or not the `OffsetRange` is empty.
     *
     * @retval TRUE   The `OffsetRange` is empty.
     * @retval FALSE  The `OffsetRange` is not empty (contains at least one byte).
     */
    bool IsEmpty(void) const { return (mLength == 0); }

    /**
     * Indicates whether or not the `OffsetRange` contains a given number of bytes.
     *
     * @param[in] aLength   The length to check.
     *
     * @retval TRUE   The `OffsetRange` contains @p aLength or more bytes.
     * @retval FALSE  The `OffsetRange` does not contain @p aLength bytes.
     */
    bool Contains(uint32_t aLength) const { return aLength <= mLength; }

    /**
     * Advances the start offset forward by the given number of bytes.
     *
     * This method ensures the start offset does not go beyond the end offset of the `OffsetRange`. If @p aLength is
     * greater than the available bytes in the `OffsetRange`, the start offset is adjusted to the end offset,
     * effectively shrinking the range to zero length.
     *
     * @param[in]  aLength   The number of bytes to advance the start offset.
     */
    void AdvanceOffset(uint32_t aLength);

    /**
     * Shrinks the `OffsetRange` length to a given length.
     *
     * If the current length of the `OffsetRange` is longer than @p aLength, the offset range is shortened to
     * @p aLength. If the range is already shorter or the same, it remains unchanged.
     *
     * @param[in] aLength  The new length to use.
     */
    void ShrinkLength(uint16_t aLength);

private:
    uint16_t mOffset;
    uint16_t mLength;
};

} // namespace ot

#endif // OFFSET_RANGE_HPP_
