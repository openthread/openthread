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
 *   This file includes implementation of bit manipulation utility functions.
 */

#include "bit_utils.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/num_utils.hpp"

namespace ot {

uint8_t CountBitsInMask(uint32_t aMask)
{
    uint8_t count = 0;

    while (aMask != 0)
    {
        aMask &= aMask - 1;
        count++;
    }

    return count;
}

uint16_t CountMatchingBits(const uint8_t *aFirst, const uint8_t *aSecond, uint16_t aMaxBitLength)
{
    uint16_t remainingLen = aMaxBitLength;
    uint16_t matchedLen   = 0;
    uint8_t  diffMask     = 0;

    while (remainingLen > 0)
    {
        uint16_t len = Min<uint16_t>(remainingLen, kBitsPerByte);

        diffMask = (aFirst[0] ^ aSecond[0]);

        if (diffMask != 0)
        {
            break;
        }

        matchedLen += len;
        remainingLen -= len;
        aFirst++;
        aSecond++;
    }

    VerifyOrExit(diffMask != 0);

    while (remainingLen > 0)
    {
        if ((diffMask & 0x80) != 0)
        {
            break;
        }

        diffMask <<= 1;
        matchedLen++;
        remainingLen--;
    }

exit:
    return matchedLen;
}

uint8_t DetermineMinBitSizeFor(uint32_t aValue)
{
    uint8_t bitSize = 0;

    do
    {
        bitSize++;
        aValue >>= 1;
    } while (aValue != 0);

    return bitSize;
}

static void ReverseBytes(uint8_t *aBytes, uint16_t aLength)
{
    for (uint16_t i = 0; i < aLength / 2; i++)
    {
        uint8_t temp = aBytes[i];

        aBytes[i]               = aBytes[aLength - 1 - i];
        aBytes[aLength - 1 - i] = temp;
    }
}

static void ShiftBitmask(uint8_t *aMask, uint16_t aNumBytes, uint8_t aShift, uint8_t aStartBits)
{
    // Shifts `aNumBytes` bytes in `aMask` forward (to the right) by
    // `aShift` bits (`0 < aShift < 8`), inserting the lowest `aShift`
    // bits of `aStartBits` at the start of `aMask[0]`.

    for (uint16_t i = 0; i < aNumBytes; i++)
    {
        uint8_t byte = aMask[i];

        aMask[i]   = (byte >> aShift) | static_cast<uint8_t>(aStartBits << (kBitsPerByte - aShift));
        aStartBits = byte;
    }
}

void RotateBitmask(uint8_t *aMask, uint16_t aBitLength, uint16_t aBitShift)
{
    uint16_t numBytes;
    uint8_t  numExtraBits;
    uint16_t byteShift;
    uint8_t  extraBitShift;

    VerifyOrExit(aBitLength > 0);

    numBytes     = BytesForBitSize(aBitLength);
    numExtraBits = static_cast<uint8_t>(aBitLength % kBitsPerByte);

    aBitShift %= aBitLength;

    byteShift     = aBitShift / kBitsPerByte;
    extraBitShift = static_cast<uint8_t>(aBitShift % kBitsPerByte);

    if (byteShift > 0)
    {
        // Rotate `numBytes` bytes to the right by `byteShift` in place
        // by reversing the first `numBytes - byteShift` bytes, reversing
        // the trailing `byteShift` bytes, and then reversing the entire
        // array (e.g., `[A B C D E | F G H]` -> `[E D C B A | H G F]`
        // -> `[F G H | A B C D E]`).

        ReverseBytes(aMask, numBytes - byteShift);
        ReverseBytes(aMask + numBytes - byteShift, byteShift);
        ReverseBytes(aMask, numBytes);

        if (numExtraBits != 0)
        {
            // When `aBitLength` is not a multiple of 8, the last byte
            // originally had `8 - numExtraBits` unused trailing bits.
            // Rotating whole bytes moved those unused bits into the end
            // of the wrapped prefix `aMask[0 .. byteShift - 1]`. We shift
            // only those `byteShift` prefix bytes right by `8 - numExtraBits`,
            // pulling in the low `8 - numExtraBits` bits from the new last
            // byte `aMask[numBytes - 1]` to close the gap.

            ShiftBitmask(aMask, byteShift, kBitsPerByte - numExtraBits, aMask[numBytes - 1]);
        }
    }

    if (extraBitShift != 0)
    {
        // Extract the trailing `extraBitShift` valid bits from the end
        // of the mask to wrap around into the start of `aMask[0]`. These
        // bits may span across the last two bytes when `numExtraBits != 0`.

        uint16_t bits;

        bits = (numBytes < sizeof(uint16_t)) ? aMask[numBytes - 1] : BigEndian::Read<uint16_t>(&aMask[numBytes - 2]);

        if (numExtraBits != 0)
        {
            bits >>= (kBitsPerByte - numExtraBits);
        }

        ShiftBitmask(aMask, numBytes, extraBitShift, static_cast<uint8_t>(bits));
    }

    if (numExtraBits != 0)
    {
        aMask[numBytes - 1] &= ~MaskForBitSize<uint8_t>(kBitsPerByte - numExtraBits);
    }

exit:
    return;
}

} // namespace ot
