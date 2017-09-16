/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implents data encoding functions.
 */

#include "encoding.hpp"

#include <string.h>

namespace ot {
namespace Encoding {
namespace Thread32 {

const uint8_t INVALID_BYTE = 0xff;

const char ENCODE_TABLE[32] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y'
};

const uint8_t DECODE_TABLE[256] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0xff, 0x12, 0x13, 0x14, 0x15, 0x16, 0xff,
    0x17, 0xff, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
    0x1e, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

otError Encode(const uint8_t *aInput, uint32_t aInputLength, char *aOutput, uint32_t &aOutputLength)
{
    uint32_t bits = 0;
    uint32_t remainingBits = 0;
    uint32_t requiredLength = (aInputLength * 8 + 4) / 5 + 1; // +1 for null-terminator

    if (aOutputLength < requiredLength)
    {
        aOutputLength = requiredLength;
        return OT_ERROR_INVALID_ARGS;
    }

    aOutputLength = 0;

    for (uint32_t i = 0; i < aInputLength; i++)
    {
        bits = (bits << 8) | aInput[i];
        remainingBits += 8;

        while (remainingBits >= 5)
        {
            remainingBits -= 5;
            aOutput[aOutputLength++] = ENCODE_TABLE[bits >> remainingBits];
            bits &= (1 << remainingBits) - 1;
        }
    }

    if (remainingBits != 0)
    {
        aOutput[aOutputLength++] = ENCODE_TABLE[bits << (5 - remainingBits)];
    }

    aOutput[aOutputLength++] = '\0';

    return OT_ERROR_NONE;
}

otError Decode(const char *aInput, uint8_t *aOutput, uint32_t &aOutputLength)
{
    uint32_t inputLength = static_cast<uint32_t>(strlen(aInput));
    uint32_t outputLength;
    uint8_t value;
    uint32_t bits;
    uint32_t remainingBits;

    // Output check
    outputLength = inputLength * 5 / 8;

    if (aOutputLength < outputLength)
    {
        aOutputLength = outputLength;
        return OT_ERROR_INVALID_ARGS;
    }

    aOutputLength = outputLength;

    // Decode
    bits = 0;
    remainingBits = 0;

    for (uint32_t i = 0; i < inputLength; i++)
    {
        value = DECODE_TABLE[static_cast<uint8_t>(aInput[i])];

        if (value == INVALID_BYTE)
        {
            return OT_ERROR_INVALID_ARGS;
        }

        bits = (bits << 5) | value;
        remainingBits += 5;

        if (remainingBits >= 8)
        {
            remainingBits -= 8;
            *aOutput++ = static_cast<uint8_t>(bits >> remainingBits);
            bits &= (1 << remainingBits) - 1;
        }
    }

    // bit-padding should be zero filled and contain no redundant (extra) input symbols
    if (remainingBits >= 5 || bits != 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }

    return OT_ERROR_NONE;
}

}  // namespace Thread32
}  // namepsace Encoding
}  // namespace ot
