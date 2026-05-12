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
#include "common/num_utils.hpp"

namespace ot {

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

} // namespace ot
