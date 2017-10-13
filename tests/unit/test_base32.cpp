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

#include <string.h>

#include "common/encoding.hpp"

#include "test_util.h"

void TestBase32Inputs(void)
{

}

void TestBase32Encoding(void)
{

}

void TestBase32Decoding(void)
{

}

void TestBase32Random(void)
{
    uint8_t input[512];
    char encoded[512];
    uint8_t decoded[512];
    uint32_t inputLength, encodedLength, decodedLength;
    otError error;

    srand(123456789); // initialize to a known seed to have reproducability

    for (int i = 0; i < 1000; i++)
    {
        // limit the length so `encoded` is not overflown
        inputLength = rand() % 300;

        for (uint32_t r = 0; r < inputLength; r++)
        {
            input[r] = rand();
        }

        encodedLength = sizeof(encoded);
        error = ot::Encoding::Thread32::Encode(input, inputLength, encoded, encodedLength);
        VerifyOrQuit(error == OT_ERROR_NONE, "TestRandom encoding failed");

        decodedLength = inputLength;
        error = ot::Encoding::Thread32::Decode(encoded, decoded, decodedLength);
        VerifyOrQuit(error == OT_ERROR_NONE, "TestRandom decoding failed");

        VerifyOrQuit(decodedLength == inputLength && memcmp(input, decoded, inputLength) == 0,
                     "TestRandom input and output do not match");
    }
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestBase32Inputs();
    TestBase32Encoding();
    TestBase32Decoding();
    TestBase32Random();
    printf("All tests passed\n");
    return 0;
}
#endif
