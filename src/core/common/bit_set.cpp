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
 *   This file implements utility function for a bit-set.
 */

#include "bit_set.hpp"

#include "common/code_utils.hpp"
#include "common/message.hpp"

namespace ot {

bool BitSetUtils::IsAllZero(const uint8_t *aMask, uint16_t aSize)
{
    bool allZero = true;

    for (uint16_t i = 0; i < aSize; i++)
    {
        VerifyOrExit(aMask[i] == 0, allZero = false);
    }

exit:
    return allZero;
}

uint16_t BitSetUtils::CountBits(const uint8_t *aMask, uint16_t aSize)
{
    uint16_t count = 0;

    for (uint16_t i = 0; i < aSize; i++)
    {
        count += CountBitsInMask(aMask[i]);
    }

    return count;
}

void BitSetUtils::FlipBits(uint8_t *aTargetMask, uint16_t aSize, uint16_t aNumBits)
{
    for (uint16_t i = 0; i < aSize; i++)
    {
        aTargetMask[i] = static_cast<uint8_t>(~aTargetMask[i]);
    }

    Tidy(aTargetMask, aSize, aNumBits);
}

bool BitSetUtils::IsSubset(const uint8_t *aMask, const uint8_t *aSuperMask, uint16_t aSize)
{
    bool isSubset = true;

    for (uint16_t i = 0; i < aSize; i++)
    {
        VerifyOrExit((aMask[i] & aSuperMask[i]) == aMask[i], isSubset = false);
    }

exit:
    return isSubset;
}

void BitSetUtils::Union(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize)
{
    for (uint16_t i = 0; i < aSize; i++)
    {
        aTargetMask[i] |= aMask[i];
    }
}

void BitSetUtils::Intersect(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize)
{
    for (uint16_t i = 0; i < aSize; i++)
    {
        aTargetMask[i] &= aMask[i];
    }
}

void BitSetUtils::Subtract(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize)
{
    for (uint16_t i = 0; i < aSize; i++)
    {
        aTargetMask[i] &= static_cast<uint8_t>(~aMask[i]);
    }
}

void BitSetUtils::Copy(uint8_t *aTargetMask, const uint8_t *aMask, uint16_t aSize, uint16_t aNumBits)
{
    for (uint16_t i = 0; i < aSize; i++)
    {
        aTargetMask[i] = aMask[i];
    }

    Tidy(aTargetMask, aSize, aNumBits);
}

void BitSetUtils::Tidy(uint8_t *aTargetMask, uint16_t aSize, uint16_t aNumBits)
{
    // `Tidy` clears any unused bits in the last byte of the mask.
    //
    // Since bits are allocated starting from the MSB of the first byte
    // (byte 0), the unused bits will be the lowest bits of the last
    // byte (byte `aSize - 1`). The number of unused bits is calculated
    // as `(aSize * kBitsPerByte) - aNumBits`.

    aTargetMask[aSize - 1] &= static_cast<uint8_t>(~(static_cast<uint8_t>(1 << (aSize * kBitsPerByte - aNumBits)) - 1));
}

Error BitSetUtils::AppendMask(Message &aMessage, const uint8_t *aMask, uint16_t aSize)
{
    return aMessage.AppendBytes(aMask, aSize);
}

Error BitSetUtils::ReadMask(const Message     &aMessage,
                            const OffsetRange &aOffsetRange,
                            uint8_t           *aTargetMask,
                            uint16_t           aSize,
                            uint16_t           aNumBits)
{
    Error error;

    SuccessOrExit(error = aMessage.Read(aOffsetRange, aTargetMask, aSize));
    Tidy(aTargetMask, aSize, aNumBits);

exit:
    return error;
}

} // namespace ot
