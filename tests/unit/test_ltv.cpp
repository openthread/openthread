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

#include "common/code_utils.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "mac/mac_ltvs.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
namespace Mac {

void TestSingleLtvEncodingAndDecoding(void)
{
    struct TestCase
    {
        uint8_t mType;
        uint8_t mLength;
        bool    mShouldBePacked;
    };

    static const TestCase kTestCases[] = {
        // Zero-length LTVs (len bit size = 1, available type bits = 7)
        {0, 0, true},
        {1, 0, true},
        {5, 0, true},
        {64, 0, true},
        {126, 0, true},
        {127, 0, true},
        {128, 0, true},
        {255, 0, true},

        // Length 1 LTVs (len bit size = 1, available type bits = 7)
        {0, 1, true},
        {1, 1, true},
        {3, 1, true},
        {32, 1, true},
        {126, 1, true},
        {127, 1, false},
        {128, 1, false},
        {255, 1, false},

        // Length 2 LTVs (bitSize = 2, available type bits = 6)
        {1, 2, true},
        {15, 2, true},
        {31, 2, true},
        {62, 2, true},
        {63, 2, false},
        {64, 2, false},
        {255, 2, false},

        // Length 3 LTVs (bitSize = 2, available type bits = 6)
        {0, 3, true},
        {62, 3, true},
        {63, 3, false},
        {64, 3, false},
        {128, 3, false},

        // Length 4 LTVs (bitSize = 3, available type bits = 5)
        {7, 4, true},
        {30, 4, true},
        {31, 4, false},
        {32, 4, false},
        {100, 4, false},

        // Length 7 LTVs (bitSize = 3, available type bits = 5)
        {1, 7, true},
        {30, 7, true},
        {31, 7, false},
        {32, 7, false},

        // Length 15 LTVs (bitSize = 4, available type bits = 4)
        {1, 15, true},
        {7, 15, true},
        {14, 15, true},
        {15, 15, false},
        {16, 15, false},

        // Length 31 LTVs (bitSize = 5, available type bits = 3)
        {1, 31, true},
        {3, 31, true},
        {6, 31, true},
        {7, 31, false},
        {15, 31, false},

        // Length 32 LTVs (bitSize = 6, available type bits = 2)
        {0, 32, true},
        {1, 32, true},
        {2, 32, true},
        {3, 32, false},
        {4, 32, false},
        {70, 32, false},

        // Length 64 LTVs (bitSize = 7, available type bits = 1)
        {0, 64, true},
        {1, 64, false},
        {10, 64, false},

        // Length 127 LTVs (bitSize = 7, available type bits = 1)
        {0, 127, true},
        {1, 127, false},
        {127, 127, false},

        // Large lengths (bitSize >= 8)
        {0, 128, false},
        {1, 128, false},
        {10, 128, false},
        {0, 240, false},
        {0, 255, false},
        {255, 255, false},
    };

    static constexpr uint16_t kMaxBytesToPrint = 7;

    uint8_t      buffer[512];
    uint8_t      valuePattern[256];
    FrameBuilder builder;

    printf("------------------------------------------------------------------------------\n");
    printf("TestTwoLtvsEncodingAndDecoding\n");

    printf("TestSingleLtvEncodingAndDecoding\n");

    for (size_t i = 0; i < sizeof(valuePattern); i++)
    {
        valuePattern[i] = static_cast<uint8_t>(i & 0xff);
    }

    for (const TestCase &testCase : kTestCases)
    {
        Ltv::AppendInfo appendLtv;
        Ltv::ParsedInfo parsedLtv;
        FrameData       frameData;
        uint16_t        expectedHeaderSize;
        uint16_t        expectedTotalSize;

        // EncodeAndAppend

        builder.Init(buffer, sizeof(buffer));

        appendLtv.InitTypeLength(testCase.mType, testCase.mLength);
        SuccessOrQuit(Ltv::EncodeAndAppend(&appendLtv, 1, builder));

        VerifyOrQuit(appendLtv.GetType() == testCase.mType);
        VerifyOrQuit(appendLtv.GetLength() == testCase.mLength);

        expectedHeaderSize = testCase.mShouldBePacked ? 1 : 2;
        expectedTotalSize  = expectedHeaderSize + testCase.mLength;
        VerifyOrQuit(builder.GetLength() == expectedTotalSize);

        VerifyOrQuit(appendLtv.GetValue() != nullptr);
        memcpy(appendLtv.GetValue(), valuePattern, testCase.mLength);

        printf("type:%-3u length:%-3u -> %-6s  encodedLen:%u [", testCase.mType, testCase.mLength,
               testCase.mShouldBePacked ? "packed" : "base", builder.GetLength());

        for (uint16_t i = 0; i < Min(builder.GetLength(), kMaxBytesToPrint); i++)
        {
            printf(" %02x", buffer[i]);
        }

        printf("%s ]\n", builder.GetLength() > kMaxBytesToPrint ? " ..." : "");

        // ValidateAllIn

        frameData.Init(buffer, builder.GetLength());
        SuccessOrQuit(Ltv::ValidateAllIn(frameData));

        // ParseFromAndAdvance

        SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));

        VerifyOrQuit(parsedLtv.GetType() == testCase.mType);
        VerifyOrQuit(parsedLtv.GetLength() == testCase.mLength);
        VerifyOrQuit(parsedLtv.GetValue() != nullptr);
        VerifyOrQuit(memcmp(parsedLtv.GetValue(), valuePattern, testCase.mLength) == 0);
        VerifyOrQuit(frameData.IsEmpty());
        VerifyOrQuit(parsedLtv.ParseFromAndAdvance(frameData) == kErrorNotFound);

        // FindIn

        frameData.Init(buffer, builder.GetLength());
        SuccessOrQuit(parsedLtv.FindIn(frameData, testCase.mType));
        VerifyOrQuit(parsedLtv.GetType() == testCase.mType);
        VerifyOrQuit(parsedLtv.GetLength() == testCase.mLength);
        VerifyOrQuit(frameData.GetLength() == builder.GetLength());
    }

    printf("TestSingleLtvEncodingAndDecoding - PASSED\n");
}

void TestTwoLtvsEncodingAndDecoding(void)
{
    struct TestCase
    {
        uint8_t mType;
        uint8_t mLength;
    };

    static const TestCase kTestCases[] = {
        // Zero length LTVs
        {0, 0},
        {1, 0},
        {5, 0},
        {64, 0},
        {255, 0},

        // Length 1 LTVs
        {0, 1},
        {1, 1},
        {3, 1},
        {126, 1},
        {127, 1},
        {255, 1},

        // Length 2 LTVs
        {1, 2},
        {15, 2},
        {62, 2},
        {63, 2},
        {255, 2},

        // Length 3 LTVs
        {0, 3},
        {15, 3},
        {62, 3},
        {66, 3},
        {129, 3},

        // Length 5 LTVs
        {1, 5},
        {3, 5},
        {7, 5},
        {8, 5},
        {17, 5},
        {30, 5},

        // Length 7 LTVs
        {1, 7},
        {30, 7},
        {31, 7},
        {32, 7},

        // Length 13 LTVs
        {0, 13},
        {1, 13},
        {3, 13},
        {7, 13},
        {8, 13},
        {13, 13},
        {15, 13},
        {16, 13},

        // Length 31 LTVs
        {1, 31},
        {6, 31},
        {7, 31},
        {15, 31},

        // Misc
        {0, 100},
        {1, 100},
        {5, 100},
        {0, 200},
        {1, 200},
        {2, 200},
        {8, 200},
    };

    static const uint8_t kSecondLtvSizes[] = {
        1,  2,  3,  4,  5,  6,  7,  8,   9,   10,  11,  12,  13,  14,  15,  16,  29,  30,  31,
        32, 33, 34, 62, 63, 64, 65, 126, 127, 128, 129, 130, 200, 201, 202, 250, 253, 254, 255,
    };

    uint8_t      buffer[512];
    uint8_t      valPattern1[256];
    uint8_t      valPattern2[256];
    FrameBuilder builder;

    printf("------------------------------------------------------------------------------\n");
    printf("TestTwoLtvsEncodingAndDecoding\n");

    for (size_t i = 0; i < sizeof(valPattern1); i++)
    {
        valPattern1[i] = static_cast<uint8_t>(i + 0x10);
        valPattern2[i] = static_cast<uint8_t>(i + 0x80);
    }

    for (const TestCase &testCase : kTestCases)
    {
        for (uint8_t secondSize : kSecondLtvSizes)
        {
            Ltv::AppendInfo appendLtvs[2];
            Ltv::ParsedInfo parsedLtv;
            FrameData       frameData;
            uint8_t         secondType;
            uint8_t         secondLength;
            uint16_t        firstSize;

            if (secondSize == 1)
            {
                secondType   = 1;
                secondLength = 0;
            }
            else if (secondSize == 2)
            {
                secondType   = 1;
                secondLength = 1;
            }
            else
            {
                // Will use base (unpacked) format since type is 129 > 127
                secondType   = 129;
                secondLength = secondSize - 2;
            }

            // EncodeAndAppend

            builder.Init(buffer, sizeof(buffer));

            appendLtvs[0].InitTypeLength(testCase.mType, testCase.mLength);
            appendLtvs[1].InitTypeLength(secondType, secondLength);

            SuccessOrQuit(Ltv::EncodeAndAppend(appendLtvs, 2, builder));

            VerifyOrQuit(appendLtvs[0].GetType() == testCase.mType);
            VerifyOrQuit(appendLtvs[0].GetLength() == testCase.mLength);
            VerifyOrQuit(appendLtvs[0].GetValue() != nullptr);
            memcpy(appendLtvs[0].GetValue(), valPattern1, testCase.mLength);

            VerifyOrQuit(appendLtvs[1].GetType() == secondType);
            VerifyOrQuit(appendLtvs[1].GetLength() == secondLength);
            VerifyOrQuit(appendLtvs[1].GetValue() != nullptr);
            memcpy(appendLtvs[1].GetValue(), valPattern2, secondLength);

            VerifyOrQuit(builder.GetLength() > secondSize);
            firstSize = builder.GetLength() - secondSize;
            VerifyOrQuit(firstSize > testCase.mLength);
            VerifyOrQuit((firstSize == testCase.mLength + 1) || (firstSize == testCase.mLength + 2));

            fprintf(stderr, "type1:%-3u len1:%-3u | type2:%-3u len2:%-3u -> encoded-len:%-4u = [%u + %u]\n",
                    testCase.mType, testCase.mLength, secondType, secondLength, builder.GetLength(), firstSize,
                    secondSize);

            // ValidateAllIn
            frameData.Init(buffer, builder.GetLength());
            SuccessOrQuit(Ltv::ValidateAllIn(frameData));

            // Parse first LTV
            SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));
            VerifyOrQuit(parsedLtv.GetType() == testCase.mType);
            VerifyOrQuit(parsedLtv.GetLength() == testCase.mLength);
            VerifyOrQuit(parsedLtv.GetValue() != nullptr);
            VerifyOrQuit(memcmp(parsedLtv.GetValue(), valPattern1, testCase.mLength) == 0);

            // Parse second LTV
            SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));

            VerifyOrQuit(parsedLtv.GetType() == secondType);
            VerifyOrQuit(parsedLtv.GetLength() == secondLength);
            VerifyOrQuit(parsedLtv.GetValue() != nullptr);
            VerifyOrQuit(memcmp(parsedLtv.GetValue(), valPattern2, secondLength) == 0);

            VerifyOrQuit(frameData.IsEmpty());
            VerifyOrQuit(parsedLtv.ParseFromAndAdvance(frameData) == kErrorNotFound);

            frameData.Init(buffer, builder.GetLength());

            // Test FindIn for first LTV
            SuccessOrQuit(parsedLtv.FindIn(frameData, testCase.mType));
            VerifyOrQuit(parsedLtv.GetType() == testCase.mType);
            VerifyOrQuit(parsedLtv.GetLength() == testCase.mLength);

            if (secondType != testCase.mType)
            {
                // Test FindIn for second LTV type
                SuccessOrQuit(parsedLtv.FindIn(frameData, secondType));
                VerifyOrQuit(parsedLtv.GetType() == secondType);
                VerifyOrQuit(parsedLtv.GetLength() == secondLength);
            }
        }
    }

    printf("TestTwoLtvsEncodingAndDecoding - PASSED\n");
}

void TestMultipleLtvsAndFinding(void)
{
    const uint8_t kType1           = 1;
    const uint8_t kType2           = 2;
    const uint8_t kType3           = 3;
    const uint8_t kTypeNonExistent = 99;

    const uint8_t kPatternA[] = {0xaa, 0xaa};
    const uint8_t kPatternB[] = {0xbb, 0xbb, 0xbb};
    const uint8_t kPatternC[] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
    const uint8_t kPatternD[] = {0xdd};
    const uint8_t kPatternF[] = {0xff, 0xff, 0xff, 0xff};

    uint8_t         buffer[256];
    FrameBuilder    builder;
    Ltv::AppendInfo appendLtvs[6];
    Ltv::ParsedInfo parsedLtv;
    FrameData       frameData;

    printf("------------------------------------------------------------------------------\n");
    printf("TestMultipleLtvsAndFinding\n");

    builder.Init(buffer, sizeof(buffer));

    // Initialize 6 LTVs:
    // LTV 0: Type 1, Length 2 (kPatternA)
    // LTV 1: Type 2, Length 3 (kPatternB)
    // LTV 2: Type 1, Length 5 (kPatternC)
    // LTV 3: Type 3, Length 1 (kPatternD)
    // LTV 4: Type 1, Length 0 (Empty)
    // LTV 5: Type 2, Length 4 (kPatternF)

    appendLtvs[0].InitTypeLengthId(kType1, sizeof(kPatternA), 0);
    appendLtvs[1].InitTypeLengthId(kType2, sizeof(kPatternB), 1);
    appendLtvs[2].InitTypeLengthId(kType1, sizeof(kPatternC), 2);
    appendLtvs[3].InitTypeLengthId(kType3, sizeof(kPatternD), 3);
    appendLtvs[4].InitTypeLengthId(kType1, 0, 4);
    appendLtvs[5].InitTypeLengthId(kType2, sizeof(kPatternF), 5);

    SuccessOrQuit(Ltv::EncodeAndAppend(appendLtvs, 6, builder));

    memcpy(appendLtvs[0].GetValue(), kPatternA, sizeof(kPatternA));
    memcpy(appendLtvs[1].GetValue(), kPatternB, sizeof(kPatternB));
    memcpy(appendLtvs[2].GetValue(), kPatternC, sizeof(kPatternC));
    memcpy(appendLtvs[3].GetValue(), kPatternD, sizeof(kPatternD));
    memcpy(appendLtvs[5].GetValue(), kPatternF, sizeof(kPatternF));

    // Validate overall buffer
    frameData.Init(buffer, builder.GetLength());
    SuccessOrQuit(Ltv::ValidateAllIn(frameData));

    // --- Test FindIn (Non-advancing search) ---

    // Repeated FindIn(kType1) must always return the first instance (LTV 0)

    frameData.Init(buffer, builder.GetLength());

    for (int i = 0; i < 3; i++)
    {
        SuccessOrQuit(parsedLtv.FindIn(frameData, kType1));
        VerifyOrQuit(parsedLtv.GetType() == kType1);
        VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternA));
        VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternA, sizeof(kPatternA)) == 0);
        VerifyOrQuit(frameData.GetLength() == builder.GetLength());
    }

    // Repeated FindIn(kType2) must always return the first instance (LTV 1)

    for (int i = 0; i < 3; i++)
    {
        SuccessOrQuit(parsedLtv.FindIn(frameData, kType2));
        VerifyOrQuit(parsedLtv.GetType() == kType2);
        VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternB));
        VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternB, sizeof(kPatternB)) == 0);
        VerifyOrQuit(frameData.GetLength() == builder.GetLength());
    }

    // FindIn for non-existent type returns kErrorNotFound without modifying frameData

    VerifyOrQuit(parsedLtv.FindIn(frameData, kTypeNonExistent) == kErrorNotFound);
    VerifyOrQuit(frameData.GetLength() == builder.GetLength());

    // --- Test FindInAndAdvance (Advancing search to find all instances) ---

    // Iterate through all instances of kType1
    frameData.Init(buffer, builder.GetLength());

    // 1st instance of kType1 (LTV 0)
    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType1));
    VerifyOrQuit(parsedLtv.GetType() == kType1);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternA));
    VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternA, sizeof(kPatternA)) == 0);

    // 2nd instance of kType1 (LTV 2)
    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType1));
    VerifyOrQuit(parsedLtv.GetType() == kType1);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternC));
    VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternC, sizeof(kPatternC)) == 0);

    // 3rd instance of kType1 (LTV 4, length 0)
    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType1));
    VerifyOrQuit(parsedLtv.GetType() == kType1);
    VerifyOrQuit(parsedLtv.GetLength() == 0);

    // 4th search for kType1 returns kErrorNotFound
    VerifyOrQuit(parsedLtv.FindInAndAdvance(frameData, kType1) == kErrorNotFound);

    // Iterate through all instances of kType2
    frameData.Init(buffer, builder.GetLength());

    // 1st instance of kType2 (LTV 1)
    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType2));
    VerifyOrQuit(parsedLtv.GetType() == kType2);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternB));
    VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternB, sizeof(kPatternB)) == 0);

    // 2nd instance of kType2 (LTV 5)
    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType2));
    VerifyOrQuit(parsedLtv.GetType() == kType2);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternF));
    VerifyOrQuit(memcmp(parsedLtv.GetValue(), kPatternF, sizeof(kPatternF)) == 0);

    // 3rd search for kType2 returns kErrorNotFound
    VerifyOrQuit(parsedLtv.FindInAndAdvance(frameData, kType2) == kErrorNotFound);

    // Mixed search: find kType3, then search remaining buffer for kType1 and kType2
    frameData.Init(buffer, builder.GetLength());

    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType3));
    VerifyOrQuit(parsedLtv.GetType() == kType3);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternD));

    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType1));
    VerifyOrQuit(parsedLtv.GetType() == kType1);
    VerifyOrQuit(parsedLtv.GetLength() == 0);

    SuccessOrQuit(parsedLtv.FindInAndAdvance(frameData, kType2));
    VerifyOrQuit(parsedLtv.GetType() == kType2);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(kPatternF));

    VerifyOrQuit(frameData.IsEmpty());

    printf("TestMultipleLtvsAndFinding - PASSED\n");
}

void TestLtvErrorsAndNegativeCases(void)
{
    uint8_t         buffer[256];
    uint8_t         smallBuffer[2];
    FrameBuilder    builder;
    Ltv::AppendInfo appendLtv;
    Ltv::ParsedInfo parsedLtv;
    FrameData       frameData;

    printf("------------------------------------------------------------------------------\n");
    printf("TestLtvErrorsAndNegativeCases\n");

    // --- EncodeAndAppend buffer overflow (kErrorNoBufs) ---

    // Trying to append a 10-byte LTV into a 2-byte FrameBuilder buffer
    builder.Init(smallBuffer, sizeof(smallBuffer));
    appendLtv.InitTypeLength(/* aType */ 1, /* aLength */ 10);
    VerifyOrQuit(Ltv::EncodeAndAppend(&appendLtv, 1, builder) == kErrorNoBufs);

    // --- Truncated LTV payload during parsing (kErrorParse) ---

    // Encode a valid LTV with Length = 20
    builder.Init(buffer, sizeof(buffer));
    appendLtv.InitTypeLength(/* aType */ 1, /* aLength */ 20);
    SuccessOrQuit(Ltv::EncodeAndAppend(&appendLtv, 1, builder));

    // Provide a truncated FrameData buffer (header + only 19 payload bytes instead of 20)
    frameData.Init(buffer, builder.GetLength() - 1);
    VerifyOrQuit(parsedLtv.ParseFromAndAdvance(frameData) == kErrorParse);

    frameData.Init(buffer, builder.GetLength() - 1);
    VerifyOrQuit(Ltv::ValidateAllIn(frameData) == kErrorParse);

    printf("TestLtvErrorsAndNegativeCases - PASSED\n");
}

void TestAppendInfoGetId(void)
{
    uint8_t         buffer[256];
    FrameBuilder    builder;
    Ltv::AppendInfo appendLtvs[3];
    Ltv::AppendInfo defaultInfo;
    FrameData       frameData;

    printf("------------------------------------------------------------------------------\n");
    printf("TestAppendInfoGetId\n");

    // Default InitTypeLength(aType, aLength) sets mId to 0
    defaultInfo.InitTypeLength(/* aType */ 1, /* aLength */ 2);
    VerifyOrQuit(defaultInfo.GetId() == 0);

    // Init with explicit aId
    appendLtvs[0].InitTypeLengthId(/* aType */ 1, /* aLength */ 2, /* aId */ 0x101);
    appendLtvs[1].InitTypeLengthId(/* aType */ 1, /* aLength */ 4, /* aId */ 0x102);
    appendLtvs[2].InitTypeLengthId(/* aType */ 2, /* aLength */ 1, /* aId */ 0x201);

    VerifyOrQuit(appendLtvs[0].GetId() == 0x101);
    VerifyOrQuit(appendLtvs[1].GetId() == 0x102);
    VerifyOrQuit(appendLtvs[2].GetId() == 0x201);

    builder.Init(buffer, sizeof(buffer));
    SuccessOrQuit(Ltv::EncodeAndAppend(appendLtvs, 3, builder));

    // Verify mId is preserved after EncodeAndAppend
    VerifyOrQuit(appendLtvs[0].GetId() == 0x101);
    VerifyOrQuit(appendLtvs[1].GetId() == 0x102);
    VerifyOrQuit(appendLtvs[2].GetId() == 0x201);

    // Verify caller can locate AppendInfo entries by ID
    for (Ltv::AppendInfo &ltv : appendLtvs)
    {
        if (ltv.GetId() == 0x101)
        {
            VerifyOrQuit(ltv.GetType() == 1 && ltv.GetLength() == 2);
        }
        else if (ltv.GetId() == 0x102)
        {
            VerifyOrQuit(ltv.GetType() == 1 && ltv.GetLength() == 4);
        }
        else if (ltv.GetId() == 0x201)
        {
            VerifyOrQuit(ltv.GetType() == 2 && ltv.GetLength() == 1);
        }
        else
        {
            VerifyOrQuit(false);
        }
    }

    frameData.Init(buffer, builder.GetLength());
    SuccessOrQuit(Ltv::ValidateAllIn(frameData));

    printf("TestAppendInfoGetId - PASSED\n");
}

void TestFindAndValidate(void)
{
    uint8_t         buffer[256];
    FrameBuilder    builder;
    Ltv::AppendInfo appendLtvs[3];
    Ltv::ParsedInfo parsedLtv;
    FrameData       frameData;
    uint8_t         targetId[] = {0x12, 0x34, 0x56};

    printf("------------------------------------------------------------------------------\n");
    printf("TestFindAndValidate\n");

    builder.Init(buffer, sizeof(buffer));

    // LTV 0: Type = TargetIdLtv::kType (1), Length = 3 (Valid TargetIdLtv)
    // LTV 1: Type = 2, Length = 4
    // LTV 2: Type = TargetIdLtv::kType (1), Length = 0 (Invalid TargetIdLtv, length 0 < kMinLength 1)
    appendLtvs[0].InitAsWithLength<TargetIdLtv>(sizeof(targetId));
    appendLtvs[1].InitTypeLength(2, 4);
    appendLtvs[2].InitAsWithLength<TargetIdLtv>(0);

    SuccessOrQuit(Ltv::EncodeAndAppend(appendLtvs, 3, builder));
    memcpy(appendLtvs[0].GetValue(), targetId, sizeof(targetId));

    // -- Test `ValidateAs<>` along with  `ParseFromAndAdvance` --

    frameData.Init(buffer, builder.GetLength());

    // LTV 0 is a valid TargetIdLtv
    SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));
    SuccessOrQuit(parsedLtv.ValidateAs<TargetIdLtv>());

    // LTV 1 has different type (Type 2), so ValidateAs returns kErrorParse
    SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));
    VerifyOrQuit(parsedLtv.ValidateAs<TargetIdLtv>() == kErrorParse);

    // LTV 2 has matching type (TargetIdLtv::kType) but invalid length (0), so ValidateAs returns kErrorParse
    SuccessOrQuit(parsedLtv.ParseFromAndAdvance(frameData));
    VerifyOrQuit(parsedLtv.ValidateAs<TargetIdLtv>() == kErrorParse);

    // -- Test `FindAndValidate` on valid first TargetIdLtv --

    frameData.Init(buffer, builder.GetLength());
    SuccessOrQuit(parsedLtv.FindAndValidate<TargetIdLtv>(frameData));
    VerifyOrQuit(parsedLtv.GetType() == TargetIdLtv::kType);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(targetId));
    VerifyOrQuit(memcmp(parsedLtv.GetValue(), targetId, sizeof(targetId)) == 0);

    VerifyOrQuit(frameData.GetLength() == builder.GetLength()); // Not Advanced

    // -- Test `FindValidateAndAdvance` --

    frameData.Init(buffer, builder.GetLength());

    SuccessOrQuit(parsedLtv.FindValidateAndAdvance<TargetIdLtv>(frameData));
    VerifyOrQuit(parsedLtv.GetType() == TargetIdLtv::kType);
    VerifyOrQuit(parsedLtv.GetLength() == sizeof(targetId));

    VerifyOrQuit(frameData.GetLength() < builder.GetLength());

    // Next search for TargetIdLtv finds LTV 2 (Length 0) which should fail validation
    VerifyOrQuit(parsedLtv.FindAndValidate<TargetIdLtv>(frameData) == kErrorParse);
    VerifyOrQuit(parsedLtv.FindValidateAndAdvance<TargetIdLtv>(frameData) == kErrorParse);

    printf("TestFindAndValidate - PASSED\n");
}

} // namespace Mac
} // namespace ot

int main(void)
{
    ot::Mac::TestSingleLtvEncodingAndDecoding();
    ot::Mac::TestTwoLtvsEncodingAndDecoding();
    ot::Mac::TestMultipleLtvsAndFinding();
    ot::Mac::TestLtvErrorsAndNegativeCases();
    ot::Mac::TestAppendInfoGetId();
    ot::Mac::TestFindAndValidate();

    printf("\nAll tests passed\n");
    return 0;
}
