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

#include "common/crc.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

void TestCrc16(void)
{
    struct TestCase
    {
        const uint8_t *mData;
        uint16_t       mDataLength;
        uint16_t       mExpectedCcittCrc16;
        uint16_t       mExpectedAnsiCrc16;
    };

    static const uint8_t kTestData1[] = {0xff};
    static const uint8_t kTestData2[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}; // "123456789"
    static const uint8_t kTestData3[] = {0x10, 0x20, 0x03, 0x15, 0xbe, 0xef, 0xca, 0xfe};

    static const TestCase kTestCases[] = {
        {kTestData1, sizeof(kTestData1), 0x1ef0, 0x0202},
        {kTestData2, sizeof(kTestData2), 0x31c3, 0xfee8},
        {kTestData3, sizeof(kTestData3), 0x926a, 0x070c},
    };

    printf("\nTestCrc16");

    for (const TestCase &testCase : kTestCases)
    {
        CrcCalculator<uint16_t> ccitt(kCrc16CcittPolynomial);
        CrcCalculator<uint16_t> ansi(kCrc16AnsiPolynomial);

        ccitt.FeedBytes(testCase.mData, testCase.mDataLength);
        ansi.FeedBytes(testCase.mData, testCase.mDataLength);

        DumpBuffer("CRC16", testCase.mData, testCase.mDataLength);
        printf("-> CCIT: 0x%04x, ANSI: 0x%04x\n", ccitt.GetCrc(), ansi.GetCrc());

        VerifyOrQuit(ccitt.GetCrc() == testCase.mExpectedCcittCrc16);
        VerifyOrQuit(ansi.GetCrc() == testCase.mExpectedAnsiCrc16);
    }
}

void TestCrc32(void)
{
    struct TestCase
    {
        const uint8_t *mData;
        uint16_t       mDataLength;
        uint32_t       mExpectedAnsiCrc32;
    };

    static const uint8_t kTestData1[] = {0xff};
    static const uint8_t kTestData2[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}; // "123456789"
    static const uint8_t kTestData3[] = {0x10, 0x20, 0x03, 0x15, 0xbe, 0xef, 0xca, 0xfe};

    static const TestCase kTestCases[] = {
        {kTestData1, sizeof(kTestData1), 0xb1f740b4},
        {kTestData2, sizeof(kTestData2), 0x89a1897f},
        {kTestData3, sizeof(kTestData3), 0xd651e770},
    };

    printf("\nTestCrc32\n");

    for (const TestCase &testCase : kTestCases)
    {
        CrcCalculator<uint32_t> crc32(kCrc32AnsiPolynomial);

        crc32.FeedBytes(testCase.mData, testCase.mDataLength);

        DumpBuffer("CRC32", testCase.mData, testCase.mDataLength);
        printf("-> 0x%08lx\n", ToUlong(crc32.GetCrc()));

        VerifyOrQuit(crc32.GetCrc() == testCase.mExpectedAnsiCrc32);
    }
}

} // namespace ot

int main(void)
{
    ot::TestCrc16();
    ot::TestCrc32();
    printf("All tests passed\n");
    return 0;
}
