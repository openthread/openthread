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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/code_utils.hpp"
#include "thread_direct/td_commands.hpp"

namespace ot {
namespace ThreadDirect {

#if OT_CONFIG_THREAD_DIRECT_ENABLE

static constexpr uint8_t kThreadMacCmdId    = 0x34;
static constexpr uint8_t kWakeSubCmdId      = 0x01;
static constexpr uint8_t kFlagRendezvous    = (1 << 7);
static constexpr uint8_t kFlagConnParams    = (1 << 6);
static constexpr uint8_t kFlagChannel       = (1 << 5);
static constexpr uint8_t kFlagActionSelect  = (1 << 4);
static constexpr uint8_t kMaxWakeCmdBufSize = 32;

static void PopulateDirtyWakeInfo(WakeCommand::Info &aInfo)
{
    aInfo.mHasRendezvousTime = true;
    aInfo.mHasConnParams     = true;
    aInfo.mHasChannel        = true;
    aInfo.mHasActionSelect   = true;
    aInfo.mRendezvousTime    = 0xee;
    aInfo.mRetryInterval     = 0x0e;
    aInfo.mRetryCount        = 0x0d;
    aInfo.mChannel           = Radio::kChannelMin;
    aInfo.mActionSelect      = 0xcc;
}

static void VerifyWakeInfoMatches(const WakeCommand::Info &aActual, const WakeCommand::Info &aExpected)
{
    VerifyOrQuit(aActual.mHasRendezvousTime == aExpected.mHasRendezvousTime);
    VerifyOrQuit(aActual.mHasConnParams == aExpected.mHasConnParams);
    VerifyOrQuit(aActual.mHasChannel == aExpected.mHasChannel);
    VerifyOrQuit(aActual.mHasActionSelect == aExpected.mHasActionSelect);
    VerifyOrQuit(aActual.mRendezvousTime == aExpected.mRendezvousTime);
    VerifyOrQuit(aActual.mRetryInterval == aExpected.mRetryInterval);
    VerifyOrQuit(aActual.mRetryCount == aExpected.mRetryCount);
    VerifyOrQuit(aActual.mChannel == aExpected.mChannel);
    VerifyOrQuit(aActual.mActionSelect == aExpected.mActionSelect);
}

struct TestCase
{
    bool    mHasRendezvousTime;
    uint8_t mRendezvousTime;
    bool    mHasConnParams;
    uint8_t mRetryInterval;
    uint8_t mRetryCount;
    bool    mHasChannel;
    uint8_t mChannel;
    bool    mHasActionSelect;
    uint8_t mActionSelect;
    bool    mIsValid;

    void ToWakeInfo(WakeCommand::Info &aInfo) const
    {
        aInfo.Clear();

        if (mHasRendezvousTime)
        {
            aInfo.mHasRendezvousTime = true;
            aInfo.mRendezvousTime    = mRendezvousTime;
        }

        if (mHasConnParams)
        {
            aInfo.mHasConnParams = true;
            aInfo.mRetryInterval = mRetryInterval;
            aInfo.mRetryCount    = mRetryCount;
        }

        if (mHasChannel)
        {
            aInfo.mHasChannel = true;
            aInfo.mChannel    = mChannel;
        }

        if (mHasActionSelect)
        {
            aInfo.mHasActionSelect = true;
            aInfo.mActionSelect    = mActionSelect;
        }
    }

    void VerifyEncodedBytes(const FrameBuilder &aBuilder) const
    {
        FrameData data;
        uint8_t   byte;
        uint8_t   expectedFlags;

        data.Init(aBuilder.GetBytes(), aBuilder.GetLength());

        SuccessOrQuit(data.ReadUint8(byte));
        VerifyOrQuit(byte == kThreadMacCmdId);

        SuccessOrQuit(data.ReadUint8(byte));
        VerifyOrQuit(byte == kWakeSubCmdId);

        expectedFlags = 0;
        expectedFlags |= mHasRendezvousTime ? kFlagRendezvous : 0;
        expectedFlags |= mHasConnParams ? kFlagConnParams : 0;
        expectedFlags |= mHasChannel ? kFlagChannel : 0;
        expectedFlags |= mHasActionSelect ? kFlagActionSelect : 0;

        if (expectedFlags != 0)
        {
            SuccessOrQuit(data.ReadUint8(byte));
            VerifyOrQuit(byte == expectedFlags);

            if (mHasRendezvousTime)
            {
                SuccessOrQuit(data.ReadUint8(byte));
                VerifyOrQuit(byte == mRendezvousTime);
            }

            if (mHasConnParams)
            {
                SuccessOrQuit(data.ReadUint8(byte));
                VerifyOrQuit(byte == ((mRetryInterval << 4) | (mRetryCount & 0x0f)));
            }

            if (mHasChannel)
            {
                SuccessOrQuit(data.ReadUint8(byte));
                VerifyOrQuit(byte == mChannel);
            }

            if (mHasActionSelect)
            {
                SuccessOrQuit(data.ReadUint8(byte));
                VerifyOrQuit(byte == mActionSelect);
            }
        }

        // No trailing bytes (also verifies flags byte is omitted when `expectedFlags == 0`).
        VerifyOrQuit(data.GetLength() == 0);
    }
};

static constexpr bool kNoRendezvous  = false;
static constexpr bool kHasRendezvous = true;
static constexpr bool kNoConnParam   = false;
static constexpr bool kHasConnParam  = true;
static constexpr bool kNoChan        = false;
static constexpr bool kHasChan       = true;
static constexpr bool kNoAction      = false;
static constexpr bool kHasAction     = true;
static constexpr bool kInvalid       = false;
static constexpr bool kValid         = true;

static const TestCase kTestCases[] = {
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kNoChan, 0, kNoAction, 0, kValid},
    {kHasRendezvous, 0, kNoConnParam, 0, 0, kNoChan, 0, kNoAction, 0, kValid},
    {kHasRendezvous, 0x2a, kNoConnParam, 0, 0, kNoChan, 0, kNoAction, 0, kValid},
    {kHasRendezvous, 255, kNoConnParam, 0, 0, kNoChan, 0, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kHasConnParam, 1, 0, kNoChan, 0, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kHasConnParam, 5, 12, kNoChan, 0, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kHasConnParam, 15, 15, kNoChan, 0, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMin, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMax, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kNoChan, 0, kHasAction, 0, kValid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kNoChan, 0, kHasAction, 0xa7, kValid},
    {kHasRendezvous, 10, kHasConnParam, 1, 8, kNoChan, 0, kNoAction, 0, kValid},
    {kHasRendezvous, 42, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMin + 4, kNoAction, 0, kValid},
    {kHasRendezvous, 100, kNoConnParam, 0, 0, kNoChan, 0, kHasAction, 0x55, kValid},
    {kNoRendezvous, 0, kHasConnParam, 3, 7, kHasChan, Radio::kChannelMin + 2, kNoAction, 0, kValid},
    {kNoRendezvous, 0, kHasConnParam, 9, 4, kNoChan, 0, kHasAction, 0x12, kValid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMax - 1, kHasAction, 0xfe, kValid},
    {kHasRendezvous, 12, kHasConnParam, 2, 6, kHasChan, Radio::kChannelMin + 1, kNoAction, 0, kValid},
    {kHasRendezvous, 34, kHasConnParam, 4, 10, kNoChan, 0, kHasAction, 0x33, kValid},
    {kHasRendezvous, 56, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMin + 8, kHasAction, 0x44, kValid},
    {kNoRendezvous, 0, kHasConnParam, 7, 14, kHasChan, Radio::kChannelMin + 5, kHasAction, 0x66, kValid},
    {kHasRendezvous, 0, kHasConnParam, 1, 0, kHasChan, Radio::kChannelMin, kHasAction, 0, kValid},
    {kHasRendezvous, 255, kHasConnParam, 15, 15, kHasChan, Radio::kChannelMax, kHasAction, 255, kValid},

    // Invalid field values (still encoded and parsed as-is without error):
    {kNoRendezvous, 0, kHasConnParam, 0, 8, kNoChan, 0, kNoAction, 0, kInvalid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, 0, kNoAction, 0, kInvalid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMin - 1, kNoAction, 0, kInvalid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, Radio::kChannelMax + 1, kNoAction, 0, kInvalid},
    {kNoRendezvous, 0, kNoConnParam, 0, 0, kHasChan, 255, kNoAction, 0, kInvalid},
    {kHasRendezvous, 255, kHasConnParam, 0, 15, kHasChan, 255, kHasAction, 255, kInvalid},
};

void TestWakeCommandPrepareAndParse(void)
{
    for (const TestCase &testCase : kTestCases)
    {
        WakeCommand::Info info;
        WakeCommand::Info parsedInfo;
        FrameBuilder      builder;
        FrameData         frameData;
        uint8_t           buffer[kMaxWakeCmdBufSize];

        testCase.ToWakeInfo(info);
        VerifyOrQuit(info.IsValid() == testCase.mIsValid);

        memset(buffer, 0xff, sizeof(buffer));
        builder.Init(buffer, sizeof(buffer));

        SuccessOrQuit(info.PrepareIn(builder));
        testCase.VerifyEncodedBytes(builder);

        frameData.Init(builder.GetBytes(), builder.GetLength());

        PopulateDirtyWakeInfo(parsedInfo);
        SuccessOrQuit(parsedInfo.ParseFrom(frameData));

        VerifyOrQuit(parsedInfo.IsValid() == testCase.mIsValid);
        VerifyWakeInfoMatches(parsedInfo, info);
    }

    // Check `IsValid()` for values exceeding 4-bit nibble limits (> 15) and ignored fields when `mHas*` flags are
    // false.
    {
        WakeCommand::Info info;

        info.Clear();
        info.mRetryInterval = 0;
        info.mRetryCount    = 255;
        info.mChannel       = 0;
        VerifyOrQuit(info.IsValid());

        info.Clear();
        info.mHasConnParams = true;
        info.mRetryInterval = 16;
        info.mRetryCount    = WakeCommand::kDefaultRetryCount;
        VerifyOrQuit(!info.IsValid());

        info.mRetryInterval = 255;
        VerifyOrQuit(!info.IsValid());

        info.mRetryInterval = WakeCommand::kDefaultRetryInterval;
        info.mRetryCount    = 16;
        VerifyOrQuit(!info.IsValid());

        info.mRetryCount = 255;
        VerifyOrQuit(!info.IsValid());
    }

    printf("TestWakeCommandPrepareAndParse() passed\n");
}

void TestWakeCommandForwardCompatibility(void)
{
    // Explicit `0x00` flags byte with no trailing bytes is valid and equivalent to omitted flags.
    {
        static const uint8_t kExplicitZeroFlags[] = {kThreadMacCmdId, kWakeSubCmdId, 0x00};

        WakeCommand::Info parsedInfo;
        WakeCommand::Info expectedInfo;
        FrameData         frameData;

        expectedInfo.Clear();
        PopulateDirtyWakeInfo(parsedInfo);
        frameData.Init(kExplicitZeroFlags, sizeof(kExplicitZeroFlags));

        SuccessOrQuit(parsedInfo.ParseFrom(frameData));
        VerifyOrQuit(parsedInfo.IsValid());
        VerifyWakeInfoMatches(parsedInfo, expectedInfo);
    }

    // Unrecognized flag bits (bits 4..7) with zero trailing bytes.
    {
        static const uint8_t kUnknownFlagsNoTrailing[] = {kThreadMacCmdId, kWakeSubCmdId, 0x08};

        WakeCommand::Info parsedInfo;
        WakeCommand::Info expectedInfo;
        FrameData         frameData;

        expectedInfo.Clear();
        PopulateDirtyWakeInfo(parsedInfo);
        frameData.Init(kUnknownFlagsNoTrailing, sizeof(kUnknownFlagsNoTrailing));

        SuccessOrQuit(parsedInfo.ParseFrom(frameData));
        VerifyOrQuit(parsedInfo.IsValid());
        VerifyWakeInfoMatches(parsedInfo, expectedInfo);
    }

    // Unrecognized flag bits with unparsed trailing bytes.
    {
        static const uint8_t kUnknownFlagsWithTrailing[] = {
            kThreadMacCmdId, kWakeSubCmdId, 0x0f, 0xde, 0xad, 0xbe, 0xef,
        };

        WakeCommand::Info parsedInfo;
        WakeCommand::Info expectedInfo;
        FrameData         frameData;

        expectedInfo.Clear();
        PopulateDirtyWakeInfo(parsedInfo);
        frameData.Init(kUnknownFlagsWithTrailing, sizeof(kUnknownFlagsWithTrailing));

        SuccessOrQuit(parsedInfo.ParseFrom(frameData));
        VerifyOrQuit(parsedInfo.IsValid());
        VerifyWakeInfoMatches(parsedInfo, expectedInfo);
    }

    // Subset of known flags combined with unrecognized flag bits and trailing bytes.
    {
        static const uint8_t kMixedFlagsFrame[] = {
            kThreadMacCmdId,
            kWakeSubCmdId,
            static_cast<uint8_t>(kFlagRendezvous | kFlagConnParams | 0x09),
            0x14,
            0x38,
            0xaa,
            0xbb,
            0xcc,
        };

        WakeCommand::Info parsedInfo;
        WakeCommand::Info expectedInfo;
        FrameData         frameData;

        expectedInfo.Clear();
        expectedInfo.mHasRendezvousTime = true;
        expectedInfo.mRendezvousTime    = 0x14;
        expectedInfo.mHasConnParams     = true;
        expectedInfo.mRetryInterval     = 3;
        expectedInfo.mRetryCount        = 8;

        PopulateDirtyWakeInfo(parsedInfo);
        frameData.Init(kMixedFlagsFrame, sizeof(kMixedFlagsFrame));

        SuccessOrQuit(parsedInfo.ParseFrom(frameData));
        VerifyOrQuit(parsedInfo.IsValid());
        VerifyWakeInfoMatches(parsedInfo, expectedInfo);
    }

    // All 4 known flags combined with all unrecognized flag bits (`0xff`) and trailing bytes.
    {
        static const uint8_t kAllBitsSetFrame[] = {
            kThreadMacCmdId, kWakeSubCmdId, 0xff, 0x10, 0x25, Radio::kChannelMin, 0x03, 0x99, 0x88,
        };

        WakeCommand::Info parsedInfo;
        WakeCommand::Info expectedInfo;
        FrameData         frameData;

        expectedInfo.Clear();
        expectedInfo.mHasRendezvousTime = true;
        expectedInfo.mRendezvousTime    = 0x10;
        expectedInfo.mHasConnParams     = true;
        expectedInfo.mRetryInterval     = 2;
        expectedInfo.mRetryCount        = 5;
        expectedInfo.mHasChannel        = true;
        expectedInfo.mChannel           = Radio::kChannelMin;
        expectedInfo.mHasActionSelect   = true;
        expectedInfo.mActionSelect      = 0x03;

        PopulateDirtyWakeInfo(parsedInfo);
        frameData.Init(kAllBitsSetFrame, sizeof(kAllBitsSetFrame));

        SuccessOrQuit(parsedInfo.ParseFrom(frameData));
        VerifyOrQuit(parsedInfo.IsValid());
        VerifyWakeInfoMatches(parsedInfo, expectedInfo);
    }

    printf("TestWakeCommandForwardCompatibility() passed\n");
}

void TestWakeCommandParseFailures(void)
{
    static const uint8_t kInvalidCmdIds[]    = {0x00, 0x04, 0x07, 0x33, 0x35, 0xff};
    static const uint8_t kInvalidSubCmdIds[] = {0x00, 0x02, 0x03, 0x04, 0x05, 0xff};

    // Empty payload (0 bytes) and 1-byte header (missing Sub-ID).
    {
        static const uint8_t kOneByte[] = {kThreadMacCmdId};

        WakeCommand::Info info;
        FrameData         frameData;

        frameData.Init(nullptr, 0);
        VerifyOrQuit(info.ParseFrom(frameData) == kErrorParse);

        frameData.Init(kOneByte, 0);
        VerifyOrQuit(info.ParseFrom(frameData) == kErrorParse);

        frameData.Init(kOneByte, sizeof(kOneByte));
        VerifyOrQuit(info.ParseFrom(frameData) == kErrorParse);
    }

    // Wrong MAC Command ID.
    for (uint8_t badCmdId : kInvalidCmdIds)
    {
        uint8_t           frame[] = {badCmdId, kWakeSubCmdId};
        WakeCommand::Info info;
        FrameData         frameData;

        frameData.Init(frame, sizeof(frame));
        VerifyOrQuit(info.ParseFrom(frameData) == kErrorParse);
    }

    // Wrong Thread Direct Sub-Command ID.
    for (uint8_t badSubCmdId : kInvalidSubCmdIds)
    {
        uint8_t           frame[] = {kThreadMacCmdId, badSubCmdId};
        WakeCommand::Info info;
        FrameData         frameData;

        frameData.Init(frame, sizeof(frame));
        VerifyOrQuit(info.ParseFrom(frameData) == kErrorParse);
    }

    // Truncated parameter bytes for every test case with present flags (both without and with unrecognized bits set).
    for (const TestCase &testCase : kTestCases)
    {
        WakeCommand::Info sourceInfo;
        uint8_t           buffer[kMaxWakeCmdBufSize];
        FrameBuilder      builder;
        uint16_t          fullLength;

        testCase.ToWakeInfo(sourceInfo);

        builder.Init(buffer, sizeof(buffer));
        SuccessOrQuit(sourceInfo.PrepareIn(builder));
        fullLength = builder.GetLength();

        if (fullLength <= 2)
        {
            continue;
        }

        for (uint16_t truncLen = 3; truncLen < fullLength; truncLen++)
        {
            WakeCommand::Info parsedInfo;
            FrameData         frameData;

            frameData.Init(buffer, truncLen);
            VerifyOrQuit(parsedInfo.ParseFrom(frameData) == kErrorParse);
        }

        // Setting unrecognized bit flags (0x0f) must still fail on truncation of recognized flag fields.
        buffer[2] |= 0x0f;

        for (uint16_t truncLen = 3; truncLen < fullLength; truncLen++)
        {
            WakeCommand::Info parsedInfo;
            FrameData         frameData;

            frameData.Init(buffer, truncLen);
            VerifyOrQuit(parsedInfo.ParseFrom(frameData) == kErrorParse);
        }
    }

    printf("TestWakeCommandParseFailures() passed\n");
}

void TestWakeCommandPrepareFailures(void)
{
    static const uint8_t kPrefixBytes[] = {0xde, 0xad};

    // Exhaustively test insufficient buffer capacity (`0 .. requiredLength - 1`) across all test cases.
    for (const TestCase &testCase : kTestCases)
    {
        WakeCommand::Info info;
        uint8_t           fullBuffer[kMaxWakeCmdBufSize];
        FrameBuilder      fullBuilder;
        uint16_t          requiredLength;

        testCase.ToWakeInfo(info);

        fullBuilder.Init(fullBuffer, sizeof(fullBuffer));
        SuccessOrQuit(info.PrepareIn(fullBuilder));
        requiredLength = fullBuilder.GetLength();

        for (uint16_t maxLen = 0; maxLen < requiredLength; maxLen++)
        {
            uint8_t      smallBuffer[kMaxWakeCmdBufSize];
            FrameBuilder smallBuilder;

            smallBuilder.Init(smallBuffer, maxLen);
            VerifyOrQuit(info.PrepareIn(smallBuilder) == kErrorNoBufs);
        }

        // Exact required capacity succeeds.
        {
            uint8_t      exactBuffer[kMaxWakeCmdBufSize];
            FrameBuilder exactBuilder;

            exactBuilder.Init(exactBuffer, requiredLength);
            SuccessOrQuit(info.PrepareIn(exactBuilder));
            VerifyOrQuit(exactBuilder.GetLength() == requiredLength);
            VerifyOrQuit(exactBuilder.GetRemainingLength() == 0);
            VerifyOrQuit(memcmp(exactBuilder.GetBytes(), fullBuilder.GetBytes(), requiredLength) == 0);
        }

        // Non-empty `FrameBuilder` with 1 byte less remaining capacity than needed fails with `kErrorNoBufs`,
        // and succeeds while preserving prefix bytes when given exact capacity.
        {
            uint8_t      prefixedBuffer[kMaxWakeCmdBufSize];
            FrameBuilder prefixedBuilder;

            prefixedBuilder.Init(prefixedBuffer, static_cast<uint16_t>(sizeof(kPrefixBytes) + requiredLength - 1));
            SuccessOrQuit(prefixedBuilder.AppendBytes(kPrefixBytes, sizeof(kPrefixBytes)));
            VerifyOrQuit(info.PrepareIn(prefixedBuilder) == kErrorNoBufs);

            prefixedBuilder.Init(prefixedBuffer, static_cast<uint16_t>(sizeof(kPrefixBytes) + requiredLength));
            SuccessOrQuit(prefixedBuilder.AppendBytes(kPrefixBytes, sizeof(kPrefixBytes)));
            SuccessOrQuit(info.PrepareIn(prefixedBuilder));
            VerifyOrQuit(prefixedBuilder.GetRemainingLength() == 0);
            VerifyOrQuit(memcmp(prefixedBuilder.GetBytes(), kPrefixBytes, sizeof(kPrefixBytes)) == 0);
            VerifyOrQuit(
                memcmp(prefixedBuilder.GetBytes() + sizeof(kPrefixBytes), fullBuilder.GetBytes(), requiredLength) == 0);
        }
    }

    printf("TestWakeCommandPrepareFailures() passed\n");
}

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

} // namespace ThreadDirect
} // namespace ot

int main(void)
{
#if OT_CONFIG_THREAD_DIRECT_ENABLE
    ot::ThreadDirect::TestWakeCommandPrepareAndParse();
    ot::ThreadDirect::TestWakeCommandForwardCompatibility();
    ot::ThreadDirect::TestWakeCommandParseFailures();
    ot::ThreadDirect::TestWakeCommandPrepareFailures();

    printf("All tests passed\n");
#else
    printf("Thread Direct is not enabled\n");
#endif

    return 0;
}
