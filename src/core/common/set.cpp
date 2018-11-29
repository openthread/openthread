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
 *  This file implements OpenThread Set class.
 */

#include "set.hpp"

namespace ot {

bool SetApi::IsEmpty(const uint8_t *aMask, uint16_t aMaskLength)
{
    bool isEmpty = true;

    for (; aMaskLength > 0; aMaskLength--, aMask++)
    {
        VerifyOrExit(*aMask == 0, isEmpty = false);
    }

exit:
    return isEmpty;
}

void SetApi::Itersect(uint8_t *aMask, const uint8_t *aOtherMask, uint16_t aMaskLength)
{
    for (; aMaskLength > 0; aMaskLength--, aMask++, aOtherMask++)
    {
        *aMask &= *aOtherMask;
    }
}

void SetApi::Union(uint8_t *aMask, const uint8_t *aOtherMask, uint16_t aMaskLength)
{
    for (; aMaskLength > 0; aMaskLength--, aMask++, aOtherMask++)
    {
        *aMask |= *aOtherMask;
    }
}

uint16_t SetApi::GetNumberOfElements(const uint8_t *aMask, uint16_t aMaskLength)
{
    // Number of `1` bits in binary representation of 0x0 to 0xF
    static const uint8_t kBitCount[16] = {
        0, // 0x0 -> 0000
        1, // 0x1 -> 0001
        1, // 0x2 -> 0010
        2, // 0x3 -> 0011
        1, // 0x4 -> 0100
        2, // 0x5 -> 0101
        2, // 0x6 -> 0110
        3, // 0x7 -> 0111
        1, // 0x8 -> 1000
        2, // 0x9 -> 1001
        2, // 0xa -> 1010
        3, // 0xb -> 1011
        2, // 0xc -> 1100
        3, // 0xd -> 1101
        3, // 0xe -> 1110
        4, // 0xf -> 1111
    };

    uint16_t count = 0;

    for (; aMaskLength > 0; aMaskLength--, aMask++)
    {
        uint8_t byte = *aMask;

        count += kBitCount[byte & 0xf];
        count += kBitCount[byte >> 4];
    }

    return count;
}

otError SetApi::GetNextElement(const uint8_t *aMaskArray, uint16_t &aElement, uint16_t aMaxSize)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (uint16_t element = (aElement == kSetIteratorFirst) ? 0 : (aElement + 1); element < aMaxSize; element++)
    {
        if (aMaskArray[element >> 3] & (1U << (element & 0x7)))
        {
            aElement = element;
            error    = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

SetApi::InfoString SetApi::ToString(const uint8_t *aMaskArray, uint16_t aMaxSize)
{
    InfoString string;
    uint16_t   element  = SetApi::kSetIteratorFirst;
    bool       addComma = false;
    otError    error;

    SuccessOrExit(string.Append("{"));

    error = GetNextElement(aMaskArray, element, aMaxSize);

    while (error == OT_ERROR_NONE)
    {
        uint16_t rangeStart = element;
        uint16_t rangeEnd   = element;

        while ((error = GetNextElement(aMaskArray, element, aMaxSize)) == OT_ERROR_NONE)
        {
            if (element != rangeEnd + 1)
            {
                break;
            }

            rangeEnd = element;
        }

        SuccessOrExit(string.Append("%s%d", addComma ? ", " : " ", rangeStart));
        addComma = true;

        if (rangeStart < rangeEnd)
        {
            SuccessOrExit(string.Append("%s%d", rangeEnd == rangeStart + 1 ? ", " : "-", rangeEnd));
        }
    }

    string.Append(" }");

exit:
    return string;
}

} // namespace ot
