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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/bit_utils.hpp"

namespace ot {

void TestCountBitsInMask(void)
{
    VerifyOrQuit(CountBitsInMask<uint8_t>(0) == 0);
    VerifyOrQuit(CountBitsInMask<uint8_t>(1) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(2) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(3) == 2);
    VerifyOrQuit(CountBitsInMask<uint8_t>(4) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(7) == 3);
    VerifyOrQuit(CountBitsInMask<uint8_t>(11) == 3);
    VerifyOrQuit(CountBitsInMask<uint8_t>(15) == 4);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0x11) == 2);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0xef) == 7);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0xff) == 8);

    VerifyOrQuit(CountBitsInMask<uint16_t>(0) == 0);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xff00) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xff) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xaa55) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xffff) == 16);

    printf("TestCountBitsInMask() passed\n");
}

void TestCountMatchingBitsAllCombinations(void)
{
    // This test iterates through all possible byte combinations and
    // bit-lengths (0-8). It calculates the expected matched length
    // by exhaustively checking bits one-by-one and then verifies
    // that `CountMatchingBits()` returns the correct length.

    uint8_t firstByte = 0;

    do
    {
        uint8_t secondByte = 0;

        do
        {
            for (uint8_t blen = 0; blen <= 8; blen++)
            {
                uint16_t matchedLen = 0;

                for (uint8_t i = 0; i < blen; i++)
                {
                    uint8_t bit = (0x80 >> i);

                    if ((firstByte & bit) != (secondByte & bit))
                    {
                        break;
                    }

                    matchedLen++;
                }

                VerifyOrQuit(CountMatchingBits(&firstByte, &secondByte, blen) == matchedLen);
            }

            secondByte++;
        } while (secondByte != 0);

        firstByte++;

    } while (firstByte != 0);

    printf("TestCountMatchingBitsAllCombinations() passed\n");
}

void TestCountMatchingBitsExamples(void)
{
    struct TestCase
    {
        uint8_t mFirst[3];
        uint8_t mSecond[3];
        uint8_t mBitLength;
        uint8_t mExpectedMatchedLength;
    };

    static const TestCase kTestCases[] = {
        {{0x00, 0x00, 0x00}, {0x00, 0x11, 0x22}, 0, 0},   {{0x6d, 0x13, 0xb0}, {0x6d, 0x13, 0xb0}, 20, 20},
        {{0x6d, 0x13, 0xb0}, {0x6d, 0x13, 0xbf}, 20, 20}, {{0x6d, 0x13, 0xb0}, {0x6d, 0x13, 0xa0}, 20, 19},
        {{0x6d, 0xa3, 0xb0}, {0x6d, 0xa3, 0xa0}, 20, 19}, {{0x77, 0xa3, 0x25}, {0x77, 0xa3, 0xa5}, 20, 16},
        {{0x77, 0xa3, 0x25}, {0x77, 0xa3, 0x65}, 20, 17}, {{0x77, 0xa3, 0x25}, {0x77, 0xa3, 0x05}, 20, 18},
        {{0x77, 0xa3, 0x25}, {0x77, 0xa3, 0x05}, 18, 18}, {{0x77, 0xa3, 0x25}, {0x77, 0xa3, 0x05}, 17, 17},
    };

    for (const TestCase &testCase : kTestCases)
    {
        uint16_t matchedLen = CountMatchingBits(testCase.mFirst, testCase.mSecond, testCase.mBitLength);

        VerifyOrQuit(matchedLen == testCase.mExpectedMatchedLength);
    }

    printf("TestCountMatchingBitsExamples() passed\n");
}

} // namespace ot

int main(void)
{
    ot::TestCountBitsInMask();
    ot::TestCountMatchingBitsAllCombinations();
    ot::TestCountMatchingBitsExamples();

    printf("All tests passed\n");
    return 0;
}
