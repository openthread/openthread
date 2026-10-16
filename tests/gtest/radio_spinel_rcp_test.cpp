/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <openthread/platform/radio.h>

#include "common/error.hpp"
#include "lib/spinel/openthread-spinel-config.h"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"

#include "fake_coprocessor_platform.hpp"
#include "fake_platform.hpp"

using namespace ot;

using ::testing::AnyNumber;
using ::testing::Truly;

TEST(RadioSpinelTransmit, shouldPassDesiredTxPowerToRadioPlatform)
{
    class MockPlatform : public FakeCoprocessorPlatform
    {
    public:
        MOCK_METHOD(otError, Transmit, (otRadioFrame * aFrame), (override));
    };

    MockPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame{};

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mTxPower = kTxPower;
    txFrame.mChannel               = 11;

    EXPECT_CALL(platform, Transmit(Truly([](otRadioFrame *aFrame) -> bool {
                    Mac::Frame &frame = *static_cast<Mac::Frame *>(aFrame);
                    return frame.mInfo.mTxInfo.mTxPower == kTxPower;
                })))
        .Times(1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);

    platform.GoInMs(1000);
}

TEST(RadioSpinelTransmit, shouldCauseSwitchingToRxChannelAfterTxDone)
{
    FakeCoprocessorPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame;

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mTxPower              = kTxPower;
    txFrame.mChannel                            = 11;
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = 25;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);
    platform.GoInMs(1000);
    EXPECT_EQ(platform.GetReceiveChannel(), 25);
}

TEST(RadioSpinelTransmit, shouldSkipCsmaCaWhenDisabled)
{
    class MockPlatform : public FakeCoprocessorPlatform
    {
    public:
        MOCK_METHOD(otError, Transmit, (otRadioFrame * aFrame), (override));
        MOCK_METHOD(otError, Receive, (uint8_t aChannel), (override));
    };

    MockPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame{};

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mCsmaCaEnabled = false;
    txFrame.mChannel                     = 11;

    EXPECT_CALL(platform, Transmit(Truly([](otRadioFrame *aFrame) -> bool {
                    Mac::Frame &frame = *static_cast<Mac::Frame *>(aFrame);
                    return frame.mInfo.mTxInfo.mCsmaCaEnabled == false;
                })))
        .Times(1);

    EXPECT_CALL(platform, Receive).Times(AnyNumber());
    // Receive(11) will be called exactly once to prepare for TX because the fake platform doesn't support sleep-to-tx
    // capability.
    EXPECT_CALL(platform, Receive(11)).Times(1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);

    platform.GoInMs(1000);
}

TEST(RadioSpinelTransmit, shouldPerformCsmaCaWhenEnabled)
{
    class MockPlatform : public FakeCoprocessorPlatform
    {
    public:
        MOCK_METHOD(otError, Transmit, (otRadioFrame * aFrame), (override));
        MOCK_METHOD(otError, Receive, (uint8_t aChannel), (override));
    };

    MockPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame{};

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mCsmaCaEnabled   = true;
    txFrame.mInfo.mTxInfo.mMaxCsmaBackoffs = 1;
    txFrame.mChannel                       = 11;

    EXPECT_CALL(platform, Transmit(Truly([](otRadioFrame *aFrame) -> bool {
                    Mac::Frame &frame = *static_cast<Mac::Frame *>(aFrame);
                    return frame.mInfo.mTxInfo.mCsmaCaEnabled == true;
                })))
        .Times(1);

    // Receive(11) will be called exactly twice:
    // 1. one time to prepare for TX because the fake platform doesn't support sleep-to-tx capability.
    // 2. one time in CSMA backoff because rx-on-when-idle is true.
    EXPECT_CALL(platform, Receive(11)).Times(2);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);

    platform.GoInMs(1000);
}

TEST(RadioSpinelTransmit, shouldNotCauseSwitchingToRxAfterTxDoneIfNotRxOnWhenIdle)
{
    FakeCoprocessorPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame;

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mTxPower              = kTxPower;
    txFrame.mChannel                            = 11;
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = 25;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Receive(11), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetRxOnWhenIdle(false), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);
    platform.GoInMs(1000);
    EXPECT_EQ(platform.GetReceiveChannel(), 11);
}

TEST(RadioSpinelReceiveAt, shouldReceiveAtGiveRadioTime)
{
    class MockPlatform : public FakeCoprocessorPlatform
    {
    public:
        MOCK_METHOD(otError, ReceiveAt, (uint8_t aChannel, uint32_t aStart, uint32_t aDuration), (override));
    };

    MockPlatform platform;

    ON_CALL(platform, ReceiveAt)
        .WillByDefault([&platform](uint8_t aChannel, uint32_t aStart, uint32_t aDuration) -> otError {
            return platform.FakePlatform::ReceiveAt(aChannel, aStart, aDuration);
        });

    EXPECT_CALL(platform, ReceiveAt).Times(1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetRxOnWhenIdle(false), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.ReceiveAt(100000, 10000, 11), kErrorNone);
    platform.GoInUs(100000);
    EXPECT_EQ(platform.GetReceiveChannel(), 0);
    platform.GoInUs(1);
    EXPECT_EQ(platform.GetReceiveChannel(), 11);
    platform.GoInUs(10000);
    EXPECT_EQ(platform.GetReceiveChannel(), 0);
}

TEST(RadioSpinelTransmit, shouldSkipCsmaBackoffWhenCsmaCaIsEnabledAndMaxBackoffsIsZero)
{
    class MockPlatform : public FakeCoprocessorPlatform
    {
    public:
        MOCK_METHOD(otError, Transmit, (otRadioFrame * aFrame), (override));
        MOCK_METHOD(otError, Receive, (uint8_t aChannel), (override));
    };

    MockPlatform platform;

    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;
    constexpr int8_t     kTxPower   = 100;

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame txFrame{};

    txFrame.mPsdu = frameBuffer;

    {
        Mac::TxFrame::BuildInfo buildInfo;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(kSrcPanId);
        buildInfo.mPanIds.SetDestination(kDstPanId);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    }

    txFrame.mInfo.mTxInfo.mCsmaCaEnabled   = true;
    txFrame.mInfo.mTxInfo.mMaxCsmaBackoffs = 0;
    txFrame.mChannel                       = 11;

    EXPECT_CALL(platform, Transmit(Truly([](otRadioFrame *aFrame) -> bool {
                    Mac::Frame &frame = *static_cast<Mac::Frame *>(aFrame);
                    return frame.mInfo.mTxInfo.mCsmaCaEnabled == true && frame.mInfo.mTxInfo.mMaxCsmaBackoffs == 0;
                })))
        .Times(1);

    EXPECT_CALL(platform, Receive).Times(AnyNumber());
    // Receive(11) will be called exactly once to prepare for TX because the fake platform doesn't support sleep-to-tx
    // capability.
    EXPECT_CALL(platform, Receive(11)).Times(1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);

    platform.GoInMs(1000);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToEnableRadioSrcMatch)
{
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(false);
    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.EnableSrcMatch(true), kErrorNone);
    ASSERT_EQ(platform.SrcMatchIsEnabled(), true);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToDisableRadioSrcMatch)
{
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.EnableSrcMatch(false), kErrorNone);
    ASSERT_EQ(platform.SrcMatchIsEnabled(), false);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToAddRadioSrcMatchShortEntry)
{
    constexpr uint16_t      kTestShortAddr = 0x1234;
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);

    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 0);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.AddSrcMatchShortEntry(kTestShortAddr), kErrorNone);

    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 1);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToClearRadioSrcMatchShortEntry)
{
    constexpr uint16_t      kTestShortAddr = 0x1234;
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchAddShortEntry(kTestShortAddr);

    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.ClearSrcMatchShortEntry(kTestShortAddr), kErrorNone);

    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 0);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToAddRadioSrcMatchExtEntry)
{
    constexpr otExtAddress  kTestExtAddr{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr otExtAddress  kTestExtAddrReversed{0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchClearExtEntries();

    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddr), 0);
    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 0);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.AddSrcMatchExtEntry(kTestExtAddr), kErrorNone);

    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddr), 0);
    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 1);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToClearRadioSrcMatchExtEntry)
{
    constexpr otExtAddress  kTestExtAddr{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr otExtAddress  kTestExtAddrReversed{0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchAddExtEntry(kTestExtAddrReversed);

    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.ClearSrcMatchExtEntry(kTestExtAddr), kErrorNone);

    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 0);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToClearAllRadioSrcMatchShortEntres)
{
    constexpr uint16_t      kTestShortAddr = 0x1234;
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchAddShortEntry(kTestShortAddr);

    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.ClearSrcMatchShortEntries(), kErrorNone);

    ASSERT_EQ(platform.SrcMatchCountShortEntries(), 0);
}

TEST(RadioSpinelSrcMatch, shouldBeAbleToClearAllRadioSrcMatchExtEntres)
{
    constexpr otExtAddress  kTestExtAddrReversed{0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchAddExtEntry(kTestExtAddrReversed);

    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.ClearSrcMatchExtEntries(), kErrorNone);

    ASSERT_EQ(platform.SrcMatchCountExtEntries(), 0);
}

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
TEST(RadioSpinelMaxPowerTable, shouldRestoreEachChannelWithItsOwnPower)
{
    constexpr uint8_t kChannelA = 11;
    constexpr uint8_t kChannelB = 15;
    constexpr uint8_t kChannelC = 26;
    constexpr int8_t  kPowerA   = 5;
    constexpr int8_t  kPowerB   = 14;
    constexpr int8_t  kPowerC   = 20;

    FakeCoprocessorPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    ASSERT_EQ(platform.mRadioSpinel.SetChannelMaxTransmitPower(kChannelA, kPowerA), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetChannelMaxTransmitPower(kChannelB, kPowerB), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetChannelMaxTransmitPower(kChannelC, kPowerC), kErrorNone);

    ASSERT_EQ(platform.ChannelMaxTxPowerGet(kChannelA), kPowerA);
    ASSERT_EQ(platform.ChannelMaxTxPowerGet(kChannelB), kPowerB);
    ASSERT_EQ(platform.ChannelMaxTxPowerGet(kChannelC), kPowerC);

    // Forget what the RCP was told, so what follows can only come from the
    // restore itself.
    platform.ChannelMaxTxPowerClear();
    ASSERT_EQ(platform.ChannelMaxTxPowerCount(), 0u);

    platform.mRadioSpinel.RestoreProperties();

    // Each configured channel comes back with its own power, not with a
    // neighbour's and not with the table default.
    EXPECT_EQ(platform.ChannelMaxTxPowerGet(kChannelA), kPowerA);
    EXPECT_EQ(platform.ChannelMaxTxPowerGet(kChannelB), kPowerB);
    EXPECT_EQ(platform.ChannelMaxTxPowerGet(kChannelC), kPowerC);

    // Channels never configured keep the table default, which is what the
    // restore sends for them. Copied into a local first: `kPowerDefault` is a
    // `static constexpr` with no out-of-line definition, and EXPECT_EQ binds
    // its arguments by reference, which would odr-use it.
    constexpr int8_t kDefaultPower = MaxPowerTable::kPowerDefault;
    EXPECT_EQ(platform.ChannelMaxTxPowerGet(12), kDefaultPower);
}

TEST(RadioSpinelMaxPowerTable, shouldNotTouchTheRcpWhenNoChannelWasConfigured)
{
    FakeCoprocessorPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    platform.ChannelMaxTxPowerClear();
    platform.mRadioSpinel.RestoreProperties();

    // Nothing was configured, so the restore must not send the table's
    // defaults to the RCP -- 16 blocking transactions on every recovery for a
    // feature nobody asked for.
    EXPECT_EQ(platform.ChannelMaxTxPowerCount(), 0u);
}

TEST(RadioSpinelMaxPowerTable, shouldSurviveAnRcpThatDoesNotImplementIt)
{
    constexpr uint8_t kChannel = 11;
    constexpr int8_t  kPower   = 5;

    FakeCoprocessorPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetChannelMaxTransmitPower(kChannel, kPower), kErrorNone);

    // From here the RCP refuses the property. A firmware that lacks it answers
    // `SPINEL_STATUS_PROP_NOT_FOUND` while this returns
    // `SPINEL_STATUS_UNIMPLEMENTED`, but both reach the host as
    // `OT_ERROR_NOT_IMPLEMENTED`, which is the path under test.
    platform.ChannelMaxTxPowerFailWith(OT_ERROR_NOT_IMPLEMENTED);
    platform.ChannelMaxTxPowerClear();

    // Must not abort: this used to be a `DieNow()`, so a regression takes the
    // whole test binary down rather than failing an assertion.
    platform.mRadioSpinel.RestoreProperties();

    EXPECT_EQ(platform.ChannelMaxTxPowerCount(), 0u);

    // And it must give up rather than ask again on every later recovery, even
    // once the RCP would accept it.
    platform.ChannelMaxTxPowerFailWith(OT_ERROR_NONE);
    platform.mRadioSpinel.RestoreProperties();

    EXPECT_EQ(platform.ChannelMaxTxPowerCount(), 0u);
}

TEST(RadioSpinelSrcMatch, shouldNotDuplicateSrcMatchEntriesOnRestoreProperties)
{
    constexpr uint16_t      kTestShortAddr = 0x1234;
    constexpr otExtAddress  kTestExtAddr{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr otExtAddress  kTestExtAddrReversed{0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    FakeCoprocessorPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    ASSERT_EQ(platform.mRadioSpinel.AddSrcMatchShortEntry(kTestShortAddr), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.AddSrcMatchExtEntry(kTestExtAddr), kErrorNone);

    ASSERT_EQ(platform.SrcMatchCountShortEntries(), 1);
    ASSERT_EQ(platform.SrcMatchCountExtEntries(), 1);
    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 1);
    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 1);

    // Simulate RCP recovery by calling RestoreProperties multiple times.
    // Without clearing entries first, this would create duplicates.
    platform.mRadioSpinel.RestoreProperties();
    platform.mRadioSpinel.RestoreProperties();

    ASSERT_EQ(platform.SrcMatchCountShortEntries(), 1);
    ASSERT_EQ(platform.SrcMatchCountExtEntries(), 1);
    ASSERT_EQ(platform.SrcMatchHasShortEntry(kTestShortAddr), 1);
    ASSERT_EQ(platform.SrcMatchHasExtEntry(kTestExtAddrReversed), 1);
}
#endif // OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0

TEST(RadioSpinelProperty, shouldSetEmptyPropertyValue)
{
    constexpr uint16_t      kTestShortAddr = 0x1234;
    const uint8_t          *empty          = nullptr;
    FakeCoprocessorPlatform platform;

    platform.SrcMatchEnable(true);
    platform.SrcMatchAddShortEntry(kTestShortAddr);
    ASSERT_EQ(platform.SrcMatchCountShortEntries(), 1);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    EXPECT_EQ(platform.mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_DATA_S, empty, 0),
              kErrorNone);
    EXPECT_EQ(platform.SrcMatchCountShortEntries(), 0);
}

TEST(RadioSpinelProperty, shouldGetEmptyPropertyValue)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 buffer[32];
    spinel_size_t           length = sizeof(buffer);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    ASSERT_EQ(platform.mRadioSpinel.Get(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, buffer, &length),
              kErrorNone);
    ASSERT_GT(length, 0u);

    ASSERT_EQ(platform.mRadioSpinel.Set(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, nullptr, 0), kErrorNone);

    length = sizeof(buffer);
    EXPECT_EQ(platform.mRadioSpinel.Get(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, buffer, &length),
              kErrorNone);
    EXPECT_EQ(length, 0u);
    EXPECT_EQ(platform.mRadioSpinel.GetRadioSpinelMetrics().mSpinelParseErrorCount, 0u);
}

#if OPENTHREAD_CONFIG_LINK_EXT_ENABLE
namespace {

void PrepareTestFrame(Mac::TxFrame &aFrame, uint8_t (&aPsduBuffer)[OT_RADIO_FRAME_MAX_SIZE], uint8_t aChannel)
{
    constexpr Mac::PanId kSrcPanId  = 0x1234;
    constexpr Mac::PanId kDstPanId  = 0x4321;
    constexpr uint8_t    kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr uint16_t   kSrcAddr   = 0xac00;

    Mac::TxFrame::BuildInfo buildInfo;

    memset(&aFrame, 0, sizeof(aFrame));
    aFrame.mPsdu = aPsduBuffer;

    buildInfo.mType    = Mac::Frame::kTypeData;
    buildInfo.mVersion = Mac::Frame::kVersion2006;
    buildInfo.mAddrs.mSource.SetShort(kSrcAddr);
    buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
    buildInfo.mPanIds.SetSource(kSrcPanId);
    buildInfo.mPanIds.SetDestination(kDstPanId);
    buildInfo.mSecurityLevel = Mac::Frame::kSecurityNone;

    aFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    aFrame.SetAckRequest(false);

    aFrame.mChannel                            = aChannel;
    aFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = aChannel;
    aFrame.mInfo.mTxInfo.mCsmaCaEnabled        = false;
    aFrame.mInfo.mTxInfo.mMaxCsmaBackoffs      = 0;
    aFrame.mInfo.mTxInfo.mMaxFrameRetries      = 0;
    aFrame.mInfo.mTxInfo.mTxPower              = 0;
}

otError SendSpinelCommand(FakeCoprocessorPlatform &aPlatform,
                          uint32_t                 aCommand,
                          spinel_prop_key_t        aKey,
                          spinel_tid_t             aTid,
                          const char              *aFormat,
                          ...)
{
    va_list args;
    otError error;

    va_start(args, aFormat);
    error = aPlatform.mSpinelDriver.SendCommand(aCommand, aKey, aTid, aFormat, args);
    va_end(args);

    return error;
}

otError SendStreamRaw(FakeCoprocessorPlatform &aPlatform, spinel_tid_t aTid, const Mac::TxFrame &aFrame)
{
    return SendSpinelCommand(aPlatform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW, aTid,
                             SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S, aFrame.mPsdu, aFrame.mLength,
                             aFrame.mChannel);
}

otError SendTxBurst(FakeCoprocessorPlatform &aPlatform,
                    spinel_tid_t             aTid,
                    const Mac::TxFrame      &aFrame,
                    uint16_t                 aBurstCount,
                    uint32_t                 aBurstPeriodUs,
                    uint32_t                 aBurstChannelMask)
{
    return SendSpinelCommand(
        aPlatform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_TX_BURST, aTid,
        SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S)
            SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S
                SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S
                    SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S,
        aBurstCount, aBurstPeriodUs, aBurstChannelMask, aFrame.mPsdu, aFrame.mLength, aFrame.mChannel,
        aFrame.mInfo.mTxInfo.mMaxCsmaBackoffs, aFrame.mInfo.mTxInfo.mMaxFrameRetries,
        aFrame.mInfo.mTxInfo.mCsmaCaEnabled, aFrame.mInfo.mTxInfo.mIsHeaderUpdated, aFrame.mInfo.mTxInfo.mIsARetx,
        aFrame.mInfo.mTxInfo.mIsSecurityProcessed, aFrame.mInfo.mTxInfo.mTxDelay, aFrame.mInfo.mTxInfo.mTxDelayBaseTime,
        aFrame.mInfo.mTxInfo.mRxChannelAfterTxDone, aFrame.mInfo.mTxInfo.mTxPower);
}

otError SendStopBurst(FakeCoprocessorPlatform &aPlatform, spinel_tid_t aTid)
{
    return SendSpinelCommand(aPlatform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_TX_BURST, aTid,
                             SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_UINT16_S), 0);
}

struct RcpFrame
{
    spinel_tid_t mTid;
    unsigned int mCommand;
    unsigned int mKey;
    unsigned int mStatus;      // Status of `LAST_STATUS`.
    uint16_t     mBurstCount;  // `Count` of `RCP_TX_BURST`.
    uint32_t     mBurstPeriod; // `PeriodUs` of `RCP_TX_BURST`.
    uint32_t     mBurstMask;   // `ChannelMask` of `RCP_TX_BURST`.
    uint64_t     mTimeUs;
};

void ObserveRcpFrames(FakeCoprocessorPlatform &aPlatform, std::vector<RcpFrame> &aFrames)
{
    aPlatform.mSpinelInterface.mFrameObserver = [&aPlatform, &aFrames](const uint8_t *aFrame, uint16_t aLength) {
        uint8_t  header;
        RcpFrame frame{};

        if (spinel_datatype_unpack(aFrame, aLength,
                                   SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
                                   &header, &frame.mCommand, &frame.mKey) <= 0)
        {
            return;
        }

        if (frame.mKey == SPINEL_PROP_LAST_STATUS)
        {
            IgnoreReturnValue(spinel_datatype_unpack(aFrame, aLength,
                                                     SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT_PACKED_S
                                                         SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
                                                     &header, &frame.mCommand, &frame.mKey, &frame.mStatus));
        }
        else if (frame.mKey == SPINEL_PROP_RCP_TX_BURST)
        {
            IgnoreReturnValue(spinel_datatype_unpack(
                aFrame, aLength,
                SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S
                    SPINEL_DATATYPE_STRUCT_S(
                        SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S),
                &header, &frame.mCommand, &frame.mKey, &frame.mBurstCount, &frame.mBurstPeriod, &frame.mBurstMask));
        }
        else
        {
            return;
        }

        frame.mTid    = SPINEL_HEADER_GET_TID(header);
        frame.mTimeUs = aPlatform.GetNow();
        aFrames.push_back(frame);
    };
}

int FindRcpFrame(const std::vector<RcpFrame> &aFrames, spinel_tid_t aTid, spinel_prop_key_t aKey)
{
    for (size_t i = 0; i < aFrames.size(); i++)
    {
        if ((aFrames[i].mTid == aTid) && (aFrames[i].mKey == aKey))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int CountRcpFrames(const std::vector<RcpFrame> &aFrames, spinel_tid_t aTid, spinel_prop_key_t aKey)
{
    int count = 0;

    for (const RcpFrame &frame : aFrames)
    {
        if ((frame.mTid == aTid) && (frame.mKey == aKey))
        {
            count++;
        }
    }

    return count;
}

} // namespace

TEST(RadioSpinelTransmit, shouldTransmitPeriodicWakeBurst)
{
    class BurstMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            mTxTimesUs.push_back(GetNow());
            mTxDelays.push_back(aFrame->mInfo.mTxInfo.mTxDelay);
            mTxDelayBaseTimes.push_back(aFrame->mInfo.mTxInfo.mTxDelayBaseTime);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t>  mTxChannels;
        std::vector<uint64_t> mTxTimesUs;
        std::vector<uint32_t> mTxDelays;
        std::vector<uint32_t> mTxDelayBaseTimes;
    };

    BurstMockPlatform platform;
    uint8_t           frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame      txFrame;

    constexpr uint8_t  kTxChannel        = 15;
    constexpr uint8_t  kRxChannel        = 20;
    constexpr uint8_t  kRxChannelAfterTx = 21; // Ignored for bursts.
    constexpr uint16_t kBurstCount       = 4;
    constexpr uint32_t kBurstPeriod      = 7500; // us
    constexpr uint64_t kExpectedPeriodUs = 7500;

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = kRxChannelAfterTx;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Receive(kRxChannel), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        EXPECT_EQ(platform.mTxChannels[i], kTxChannel);
        EXPECT_EQ(platform.mTxTimesUs[i] - platform.mTxTimesUs[0], i * kExpectedPeriodUs);
        EXPECT_EQ(platform.mTxDelays[i], 0u);
        EXPECT_EQ(platform.mTxDelayBaseTimes[i], 0u);
    }

    // The radio receives on the host's receive channel after the burst.
    EXPECT_EQ(platform.GetReceiveChannel(), kRxChannel);
    EXPECT_FALSE(platform.IsSleeping());
}

TEST(RadioSpinelTransmit, shouldTransmitBackToBackBurst)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;

    constexpr uint8_t  kTxChannel  = 15;
    constexpr uint16_t kBurstCount = 4;

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // With a zero period, each tick starts as soon as the previous one ends.
    ASSERT_EQ(
        SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, /* aBurstPeriodUs */ 0, /* aBurstChannelMask */ 0),
        kErrorNone);
    platform.GoInMs(10);
    ASSERT_EQ(platform.mTxRecords.size(), kBurstCount);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        const FakePlatform::TxRecord &record = platform.mTxRecords[i];

        EXPECT_EQ(record.mChannel, kTxChannel);
        EXPECT_EQ(record.mCcaEnd, record.mCalledAt);
        EXPECT_EQ(record.mSfdEnd, record.mCalledAt + platform.mTxTiming.mRampUpUs + FakePlatform::kShrUs);
        EXPECT_EQ(record.mEnd,
                  record.mCalledAt + platform.mTxTiming.mRampUpUs + FakePlatform::GetAirtimeUs(txFrame.mLength));

        if (i > 0)
        {
            EXPECT_EQ(record.mCalledAt, platform.mTxRecords[i - 1].mEnd);
        }
    }
}
TEST(RadioSpinelTransmit, shouldUpdateTargetTxTimeDuringPeriodicBurst)
{
    class TimedBurstMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            mTxTimesUs.push_back(GetNow());
            mTxDelays.push_back(aFrame->mInfo.mTxInfo.mTxDelay);
            mTxDelayBaseTimes.push_back(aFrame->mInfo.mTxInfo.mTxDelayBaseTime);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t>  mTxChannels;
        std::vector<uint64_t> mTxTimesUs;
        std::vector<uint32_t> mTxDelays;
        std::vector<uint32_t> mTxDelayBaseTimes;
    };

    TimedBurstMockPlatform platform;
    uint8_t                frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame           txFrame;

    constexpr uint8_t  kTxChannel        = 15;
    constexpr uint16_t kBurstCount       = 4;
    constexpr uint32_t kBurstPeriod      = 7500; // us
    constexpr uint32_t kInitialBaseTime  = 1000;
    constexpr uint32_t kInitialTxDelay   = 5000;
    constexpr uint32_t kExpectedPeriodUs = 7500;

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = kInitialBaseTime;
    txFrame.mInfo.mTxInfo.mTxDelay         = kInitialTxDelay;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    platform.GoInUs(kInitialBaseTime);

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    EXPECT_GT(platform.mTxTimesUs[0], kInitialBaseTime);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        EXPECT_EQ(platform.mTxChannels[i], kTxChannel);
        EXPECT_EQ(platform.mTxTimesUs[i] - platform.mTxTimesUs[0], i * kExpectedPeriodUs);
        EXPECT_EQ(platform.mTxDelayBaseTimes[i] + platform.mTxDelays[i],
                  kInitialBaseTime + kInitialTxDelay + i * kExpectedPeriodUs);
    }
}

TEST(RadioSpinelTransmit, shouldTransmitSingleMultiChannelSweep)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;

    constexpr uint8_t  kRxChannel   = 22;
    constexpr uint32_t kChannelMask = (1UL << 11) | (1UL << 18) | (1UL << 26);
    constexpr uint8_t  kChannels[]  = {11, 18, 26};

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 20);
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = 21; // Ignored for bursts.
    platform.mTxTiming.mEnabled                 = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Receive(kRxChannel), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 1, /* aBurstPeriodUs */ 0, kChannelMask),
              kErrorNone);

    platform.GoInMs(10);

    // Channels are swept back-to-back in ascending order.
    ASSERT_EQ(platform.mTxRecords.size(), 3u);
    for (size_t i = 0; i < OT_ARRAY_LENGTH(kChannels); ++i)
    {
        EXPECT_EQ(platform.mTxRecords[i].mChannel, kChannels[i]);

        if (i > 0)
        {
            EXPECT_EQ(platform.mTxRecords[i].mCalledAt, platform.mTxRecords[i - 1].mEnd);
        }
    }
    EXPECT_EQ(platform.GetReceiveChannel(), kRxChannel);
}
TEST(RadioSpinelTransmit, shouldTransmitPeriodicMultiChannelBurst)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;

    constexpr uint16_t kBurstCount       = 3;
    constexpr uint32_t kBurstPeriod      = 10000; // us
    constexpr uint32_t kChannelMask      = (1UL << 12) | (1UL << 24);
    constexpr uint64_t kExpectedPeriodUs = 10000;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 11);
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(50);

    // Each tick starts at its slot, and its second channel follows its first channel back-to-back.
    ASSERT_EQ(platform.mTxRecords.size(), 6u);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        const FakePlatform::TxRecord &first  = platform.mTxRecords[tick * 2];
        const FakePlatform::TxRecord &second = platform.mTxRecords[tick * 2 + 1];

        EXPECT_EQ(first.mChannel, 12);
        EXPECT_EQ(second.mChannel, 24);
        EXPECT_EQ(first.mCalledAt - platform.mTxRecords[0].mCalledAt, tick * kExpectedPeriodUs);
        EXPECT_EQ(second.mCalledAt, first.mEnd);
    }
}
TEST(RadioSpinelTransmit, shouldNotStopBurstOnReceivedFrame)
{
    class RecorderMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t> mTxChannels;
    };

    RecorderMockPlatform platform;
    uint8_t              frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame         txFrame;

    constexpr uint8_t  kBurstChannel = 15;
    constexpr uint16_t kBurstCount   = 10;
    constexpr uint32_t kBurstPeriod  = 10000; // us

    PrepareTestFrame(txFrame, frameBuffer, kBurstChannel);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod,
                          /* aBurstChannelMask */ 0),
              kErrorNone);

    // Run 15 ms: ticks at 0 ms and 10 ms should have fired (2 transmissions).
    platform.GoInUs(15000);
    ASSERT_EQ(platform.mTxChannels.size(), 2u);

    // Simulate receiving a frame on the radio mid-burst; burst must continue unaffected.
    otRadioFrame rxFrame{};
    uint8_t      rxPsdu[OT_RADIO_FRAME_MAX_SIZE]{};
    rxFrame.mPsdu    = rxPsdu;
    rxFrame.mLength  = 10;
    rxFrame.mChannel = kBurstChannel;
    otPlatRadioReceiveDone(FakePlatform::CurrentInstance(), &rxFrame, OT_ERROR_NONE);

    // Advance another 10 ms (to 25 ms): tick at 20 ms should fire (3rd transmission).
    platform.GoInUs(10000);
    ASSERT_EQ(platform.mTxChannels.size(), 3u);
    EXPECT_EQ(platform.mTxChannels[2], kBurstChannel);

    // Run past the burst window; all ticks are sent on the burst channel.
    platform.GoInMs(100);
    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);

    for (uint8_t channel : platform.mTxChannels)
    {
        EXPECT_EQ(channel, kBurstChannel);
    }
}

TEST(RadioSpinelTransmit, shouldRejectBurstWithAckRequestOrUnprocessedSecurity)
{
    class ValidationMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t> mTxChannels;
    };

    ValidationMockPlatform platform;
    uint8_t                frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame           txFrame;

    constexpr uint8_t  kTxChannel   = 15;
    constexpr uint16_t kBurstCount  = 3;
    constexpr uint32_t kBurstPeriod = 7500; // us

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // 1. Burst frame with AckRequest == true must be rejected.
    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.SetAckRequest(true);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(30);
    EXPECT_EQ(platform.mTxChannels.size(), 0u);

    // 2. Burst frame with SecurityEnabled == true and mIsSecurityProcessed == false must be rejected.
    {
        Mac::TxFrame::BuildInfo buildInfo;
        constexpr uint8_t       kDstAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

        memset(&txFrame, 0, sizeof(txFrame));
        txFrame.mPsdu = frameBuffer;

        buildInfo.mType    = Mac::Frame::kTypeData;
        buildInfo.mVersion = Mac::Frame::kVersion2006;
        buildInfo.mAddrs.mSource.SetShort(0xac00);
        buildInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        buildInfo.mPanIds.SetSource(0x1234);
        buildInfo.mPanIds.SetDestination(0x4321);
        buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
        buildInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;

        txFrame.PrepareHeadersWithEmptyPayload(buildInfo);
        txFrame.SetAckRequest(false);
        txFrame.mChannel                            = kTxChannel;
        txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = kTxChannel;
        txFrame.mInfo.mTxInfo.mCsmaCaEnabled        = false;
        txFrame.mInfo.mTxInfo.mIsSecurityProcessed  = false;
    }

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 2, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(30);
    EXPECT_EQ(platform.mTxChannels.size(), 0u);

    // 3. Same secured frame with mIsSecurityProcessed == true must succeed.
    txFrame.mInfo.mTxInfo.mIsSecurityProcessed = true;
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 3, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(30);
    EXPECT_EQ(platform.mTxChannels.size(), kBurstCount);
}

TEST(RadioSpinelTransmit, shouldContinueBurstWhenSingleTransmitFails)
{
    class ErrorResilienceMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);

            otError txDoneError = (mTxChannels.size() == 2) ? OT_ERROR_CHANNEL_ACCESS_FAILURE : OT_ERROR_NONE;
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, txDoneError);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t> mTxChannels;
    };

    ErrorResilienceMockPlatform platform;
    uint8_t                     frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame                txFrame;

    constexpr uint16_t kBurstCount  = 3;
    constexpr uint32_t kBurstPeriod = 10000; // us
    constexpr uint32_t kChannelMask = (1UL << 11) | (1UL << 15);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 11);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), 6u);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        EXPECT_EQ(platform.mTxChannels[tick * 2], 11);
        EXPECT_EQ(platform.mTxChannels[tick * 2 + 1], 15);
    }
}

TEST(RadioSpinelTransmit, shouldClearTxDelayAfterFirstChannelInTimedSweep)
{
    class TimedMultiChannelMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            mTxDelays.push_back(aFrame->mInfo.mTxInfo.mTxDelay);
            mTxDelayBaseTimes.push_back(aFrame->mInfo.mTxInfo.mTxDelayBaseTime);
            mCsmaEnabledFlags.push_back(aFrame->mInfo.mTxInfo.mCsmaCaEnabled);
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t>  mTxChannels;
        std::vector<uint32_t> mTxDelays;
        std::vector<uint32_t> mTxDelayBaseTimes;
        std::vector<bool>     mCsmaEnabledFlags;
    };

    TimedMultiChannelMockPlatform platform;
    uint8_t                       frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame                  txFrame;

    constexpr uint16_t kBurstCount       = 3;
    constexpr uint32_t kBurstPeriod      = 10000; // us
    constexpr uint32_t kExpectedPeriodUs = 10000;
    constexpr uint32_t kInitialBaseTime  = 5000;
    constexpr uint32_t kInitialTxDelay   = 0; // Scheduled via non-zero mTxDelayBaseTime with zero mTxDelay
    constexpr uint32_t kChannelMask      = (1UL << 11) | (1UL << 18) | (1UL << 26);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 20);
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = kInitialBaseTime;
    txFrame.mInfo.mTxInfo.mTxDelay         = kInitialTxDelay;
    txFrame.mInfo.mTxInfo.mCsmaCaEnabled   = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    platform.GoInUs(1000);

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), 9u);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        size_t base = tick * 3;

        EXPECT_EQ(platform.mTxChannels[base + 0], 11);
        EXPECT_EQ(platform.mTxChannels[base + 1], 18);
        EXPECT_EQ(platform.mTxChannels[base + 2], 26);

        // First channel of tick k uses TargetTxTime = T_0 + k * periodUs; subsequent channels clear both mTxDelay and
        // mTxDelayBaseTime to 0 so IsTargetTxTimeSpecified() is false.
        EXPECT_EQ(platform.mTxDelayBaseTimes[base + 0] + platform.mTxDelays[base + 0],
                  kInitialBaseTime + kInitialTxDelay + tick * kExpectedPeriodUs);
        EXPECT_EQ(platform.mTxDelays[base + 1], 0u);
        EXPECT_EQ(platform.mTxDelayBaseTimes[base + 1], 0u);
        EXPECT_EQ(platform.mTxDelays[base + 2], 0u);
        EXPECT_EQ(platform.mTxDelayBaseTimes[base + 2], 0u);

        // CSMA-CA enable setting remains preserved across all channels in every tick.
        EXPECT_TRUE(platform.mCsmaEnabledFlags[base + 0]);
        EXPECT_TRUE(platform.mCsmaEnabledFlags[base + 1]);
        EXPECT_TRUE(platform.mCsmaEnabledFlags[base + 2]);
    }
}

TEST(RadioSpinelTransmit, shouldSkipOverrunTicksDuringPeriodicBurst)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;

    // A 127-byte frame takes 40 us ramp-up + 4256 us airtime, so a 2-channel sweep takes 8592 us, longer than the
    // 5000 us period. With slots at 0, 5, 10 and 15 ms, the ticks at 0 and 10 ms are sent, and the slots at 5 and
    // 15 ms are skipped since they have passed when the previous tick ends.
    constexpr uint16_t kBurstCount  = 4;
    constexpr uint32_t kBurstPeriod = 5000; // us
    constexpr uint32_t kChannelMask = (1UL << 11) | (1UL << 15);
    constexpr uint16_t kLength      = OT_RADIO_FRAME_MAX_SIZE;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 20);
    txFrame.mLength             = kLength;
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(60);

    ASSERT_EQ(platform.mTxRecords.size(), 4u);

    const otRadioTime64 kStart    = platform.mTxRecords[0].mCalledAt;
    const uint64_t      kTxTimeUs = platform.mTxTiming.mRampUpUs + FakePlatform::GetAirtimeUs(kLength);

    EXPECT_EQ(kTxTimeUs, 4296u);
    EXPECT_EQ(platform.mTxRecords[0].mChannel, 11);
    EXPECT_EQ(platform.mTxRecords[1].mChannel, 15);
    EXPECT_EQ(platform.mTxRecords[1].mCalledAt, kStart + kTxTimeUs);
    EXPECT_EQ(platform.mTxRecords[2].mChannel, 11);
    EXPECT_EQ(platform.mTxRecords[2].mCalledAt, kStart + 10000);
    EXPECT_EQ(platform.mTxRecords[3].mChannel, 15);
    EXPECT_EQ(platform.mTxRecords[3].mCalledAt, kStart + 10000 + kTxTimeUs);
}

TEST(RadioSpinelTransmit, shouldEndSfdAtTargetTxTimeForTimedTicks)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;

    constexpr uint16_t kBurstCount       = 3;
    constexpr uint32_t kBurstPeriod      = 7500; // us
    constexpr uint32_t kExpectedPeriodUs = 7500;
    constexpr uint32_t kTxDelay          = 5000;
    constexpr uint32_t kChannelMask      = (1UL << 11) | (1UL << 15);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 20);
    txFrame.mInfo.mTxInfo.mCsmaCaEnabled = true;
    platform.mTxTiming.mEnabled          = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    otRadioTime64 targetTime = platform.GetRadioNow() + kTxDelay;

    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = static_cast<uint32_t>(targetTime - kTxDelay);
    txFrame.mInfo.mTxInfo.mTxDelay         = kTxDelay;

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);
    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxRecords.size(), 2u * kBurstCount);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        const FakePlatform::TxRecord &first  = platform.mTxRecords[tick * 2];
        const FakePlatform::TxRecord &second = platform.mTxRecords[tick * 2 + 1];

        // The first channel of a tick performs CCA and its SFD ends exactly at the target TX time of the tick.
        EXPECT_TRUE(first.mIsTimed);
        EXPECT_FALSE(first.mAborted);
        EXPECT_EQ(first.mTargetTime, targetTime + tick * kExpectedPeriodUs);
        EXPECT_EQ(first.mSfdEnd, first.mTargetTime);

        // The second channel is not timed and starts right after the first one, performing its own CCA.
        EXPECT_FALSE(second.mIsTimed);
        EXPECT_EQ(second.mCalledAt, first.mEnd);
        EXPECT_EQ(second.mCcaEnd, second.mCalledAt + platform.mTxTiming.mCcaUs);
        EXPECT_EQ(second.mSfdEnd, second.mCcaEnd + platform.mTxTiming.mRampUpUs + FakePlatform::kShrUs);
    }
}
namespace {

class TimedBurstRecorderPlatform : public FakeCoprocessorPlatform
{
public:
    otError Transmit(otRadioFrame *aFrame) override
    {
        mTxChannels.push_back(aFrame->mChannel);
        mTxTimesUs.push_back(GetNow());
        mTxTargetTimes.push_back(aFrame->mInfo.mTxInfo.mTxDelayBaseTime + aFrame->mInfo.mTxInfo.mTxDelay);
        mIsTimed.push_back(aFrame->mInfo.mTxInfo.mTxDelayBaseTime != 0 || aFrame->mInfo.mTxInfo.mTxDelay != 0);
        otPlatRadioTxStarted(CurrentInstance(), aFrame);
        otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, mTxDoneError);
        return OT_ERROR_NONE;
    }

    otError               mTxDoneError = OT_ERROR_NONE;
    std::vector<uint8_t>  mTxChannels;
    std::vector<uint64_t> mTxTimesUs;
    std::vector<uint32_t> mTxTargetTimes;
    std::vector<bool>     mIsTimed;
};

} // namespace

TEST(RadioSpinelTransmit, shouldSkipTimedBurstSlotsAlreadyInThePast)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;

    constexpr uint16_t kBurstCount  = 4;
    constexpr uint32_t kBurstPeriod = 10000; // us
    constexpr uint32_t kTargetTime  = 5000;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = 1000;
    txFrame.mInfo.mTxInfo.mTxDelay         = kTargetTime - 1000;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // Slots are at 5 ms, 15 ms, 25 ms and 35 ms. At 20 ms, the first two slots are missed.
    platform.GoInUs(20000 - platform.GetNow());

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), 2u);
    EXPECT_TRUE(platform.mIsTimed[0]);
    EXPECT_TRUE(platform.mIsTimed[1]);
    EXPECT_EQ(platform.mTxTargetTimes[0], kTargetTime + 2 * 10000);
    EXPECT_EQ(platform.mTxTargetTimes[1], kTargetTime + 3 * 10000);
    EXPECT_LE(platform.mTxTimesUs[0], kTargetTime + 2 * 10000);
    EXPECT_EQ(platform.mTxTimesUs[1] - platform.mTxTimesUs[0], 10000u);
}

TEST(RadioSpinelTransmit, shouldRejectTimedBurstWhenAllSlotsAreInThePast)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = 1000;
    txFrame.mInfo.mTxInfo.mTxDelay         = 4000;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    platform.GoInUs(50000 - platform.GetNow());

    // 1. Periodic burst whose slots (5, 15, 25 ms) are all in the past.
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 3, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);
    EXPECT_EQ(platform.mTxChannels.size(), 0u);

    // 2. Single-tick burst whose target time is in the past.
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 2, txFrame, /* aBurstCount */ 1, /* aBurstPeriodUs */ 0,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);
    EXPECT_EQ(platform.mTxChannels.size(), 0u);

    // 3. Back-to-back burst whose first (timed) tick is missed sends the remaining ticks immediately.
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 3, txFrame, /* aBurstCount */ 3, /* aBurstPeriodUs */ 0,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);
    ASSERT_EQ(platform.mTxChannels.size(), 2u);
    EXPECT_FALSE(platform.mIsTimed[0]);
    EXPECT_FALSE(platform.mIsTimed[1]);
}

TEST(RadioSpinelTransmit, shouldNotRetryTimedBurstTickOnCcaFailure)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;

    constexpr uint16_t kBurstCount = 3;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    txFrame.mInfo.mTxInfo.mCsmaCaEnabled   = true;
    txFrame.mInfo.mTxInfo.mMaxCsmaBackoffs = 4;
    txFrame.mInfo.mTxInfo.mMaxFrameRetries = 3;

    platform.mTxDoneError = OT_ERROR_CHANNEL_ACCESS_FAILURE;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // 1. Untimed single-tick burst honors `mMaxCsmaBackoffs` and `mMaxFrameRetries`.
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 1, /* aBurstPeriodUs */ 0,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(500);
    EXPECT_GT(platform.mTxChannels.size(), 1u);

    // 2. Timed burst ticks perform a single CCA before the target TX time. The host sets no CSMA backoffs and no
    //    frame retries, since they would be late.
    platform.mTxChannels.clear();
    platform.mIsTimed.clear();
    txFrame.mInfo.mTxInfo.mMaxCsmaBackoffs = 0;
    txFrame.mInfo.mTxInfo.mMaxFrameRetries = 0;
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = static_cast<uint32_t>(platform.GetNow());
    txFrame.mInfo.mTxInfo.mTxDelay         = 5000;

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 2, txFrame, kBurstCount, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(100);
    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    for (bool isTimed : platform.mIsTimed)
    {
        EXPECT_TRUE(isTimed);
    }
}

TEST(RadioSpinelTransmit, shouldFollowReceiveAndSleepBetweenTicksWithoutEndingBurst)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;

    constexpr uint8_t  kBurstChannel = 15;
    constexpr uint8_t  kOtherChannel = 20;
    constexpr uint16_t kBurstCount   = 4;
    constexpr uint32_t kBurstPeriod  = 10000; // us

    PrepareTestFrame(txFrame, frameBuffer, kBurstChannel);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Receive(kBurstChannel), kErrorNone);

    // 1. A receive request on another channel (`SPINEL_PROP_PHY_CHAN`) moves the radio to that channel between
    //    ticks, and the burst continues.
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(15000);
    ASSERT_EQ(platform.mTxChannels.size(), 2u);

    ASSERT_EQ(platform.mRadioSpinel.Receive(kOtherChannel), kErrorNone);
    EXPECT_EQ(platform.GetReceiveChannel(), kOtherChannel);
    platform.GoInMs(100);
    EXPECT_EQ(platform.mTxChannels.size(), kBurstCount);
    EXPECT_EQ(platform.GetReceiveChannel(), kOtherChannel);
    EXPECT_FALSE(platform.IsSleeping());

    // 2. A timed burst with ticks at 5, 15, 25 and 35 ms. Each tick is handed to `SubMac` 1 ms before its target TX
    //    time (`OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US`).
    platform.mTxChannels.clear();
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = static_cast<uint32_t>(platform.GetNow());
    txFrame.mInfo.mTxInfo.mTxDelay         = 5000;

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 2, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(10000);
    ASSERT_EQ(platform.mTxChannels.size(), 1u);

    // A sleep request (`SPINEL_PROP_MAC_RAW_STREAM_ENABLED` = false) between ticks takes effect right away.
    ASSERT_EQ(platform.mRadioSpinel.Sleep(), kErrorNone);
    EXPECT_TRUE(platform.IsSleeping());

    // A receive request while the next tick waits in `SubMac` takes effect after that tick.
    platform.GoInUs(4500);
    ASSERT_EQ(platform.mTxChannels.size(), 1u);
    ASSERT_EQ(platform.mRadioSpinel.Receive(kOtherChannel), kErrorNone);
    EXPECT_TRUE(platform.IsSleeping());

    platform.GoInUs(1000);
    ASSERT_EQ(platform.mTxChannels.size(), 2u);
    EXPECT_FALSE(platform.IsSleeping());
    EXPECT_EQ(platform.GetReceiveChannel(), kOtherChannel);

    platform.GoInMs(100);
    EXPECT_EQ(platform.mTxChannels.size(), kBurstCount);
    EXPECT_FALSE(platform.IsSleeping());
}

TEST(RadioSpinelTransmit, shouldScheduleTimedBurstOnRadioClock)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;

    constexpr uint16_t kBurstCount       = 4;
    constexpr uint32_t kBurstPeriod      = 7500; // us
    constexpr uint32_t kExpectedPeriodUs = 7500;
    constexpr uint32_t kTxDelay          = 5000;

    // Use a radio clock that differs from the alarm clock and wraps its 32-bit value during the burst.
    platform.SetRadioClockOffset(0xfffff000u);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    uint64_t startUs   = platform.GetNow();
    uint32_t radioBase = static_cast<uint32_t>(platform.GetRadioNow());

    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = radioBase;
    txFrame.mInfo.mTxInfo.mTxDelay         = kTxDelay;

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    EXPECT_GT(platform.mTxTimesUs[0], startUs);
    EXPECT_LE(platform.mTxTimesUs[0], startUs + kTxDelay);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        EXPECT_EQ(platform.mTxTargetTimes[i], static_cast<uint32_t>(radioBase + kTxDelay + i * kExpectedPeriodUs));
        EXPECT_EQ(platform.mTxTimesUs[i] - platform.mTxTimesUs[0], i * kExpectedPeriodUs);
    }
}

TEST(RadioSpinelTransmit, shouldReplyRightAwayAndNotifyWhenBurstEnds)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;
    std::vector<RcpFrame>      frames;

    constexpr spinel_tid_t kTid         = 1;
    constexpr spinel_tid_t kGetTid      = 2;
    constexpr uint16_t     kBurstCount  = 3;
    constexpr uint32_t     kBurstPeriod = 10000; // us
    constexpr uint32_t     kChannelMask = (1UL << 11) | (1UL << 15);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);

    uint64_t startUs = platform.GetNow();

    ASSERT_EQ(SendTxBurst(platform, kTid, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);
    platform.GoInUs(1);

    // The request is answered right away with the configuration of the burst.
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].mTid, kTid);
    EXPECT_EQ(frames[0].mCommand, static_cast<unsigned int>(SPINEL_CMD_PROP_VALUE_IS));
    EXPECT_EQ(frames[0].mKey, static_cast<unsigned int>(SPINEL_PROP_RCP_TX_BURST));
    EXPECT_EQ(frames[0].mBurstCount, kBurstCount);
    EXPECT_EQ(frames[0].mBurstPeriod, kBurstPeriod);
    EXPECT_EQ(frames[0].mBurstMask, kChannelMask);

    // Ticks are at 0, 10 and 20 ms. Reading the property gives the remaining ticks.
    platform.GoInUs(15000);
    ASSERT_EQ(platform.mTxChannels.size(), 4u);
    ASSERT_EQ(platform.mSpinelDriver.SendCommand(SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_TX_BURST, kGetTid),
              kErrorNone);
    platform.GoInUs(1);
    ASSERT_GE(FindRcpFrame(frames, kGetTid, SPINEL_PROP_RCP_TX_BURST), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kGetTid, SPINEL_PROP_RCP_TX_BURST)].mBurstCount, 1u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 0);

    // When the last tick is done, the end of the burst is notified once.
    platform.GoInMs(50);
    ASSERT_EQ(platform.mTxChannels.size(), 2u * kBurstCount);
    ASSERT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);

    const RcpFrame &done = frames[FindRcpFrame(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST)];

    EXPECT_EQ(done.mCommand, static_cast<unsigned int>(SPINEL_CMD_PROP_VALUE_IS));
    EXPECT_EQ(done.mBurstCount, 0u);
    EXPECT_EQ(done.mBurstPeriod, 0u);
    EXPECT_EQ(done.mBurstMask, 0u);
    EXPECT_GE(done.mTimeUs, startUs + 2 * 10000);
}

TEST(RadioSpinelTransmit, shouldNotifyEndOfBurstWhenAllTransmissionsFail)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;
    std::vector<RcpFrame>      frames;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    platform.mTxDoneError = OT_ERROR_CHANNEL_ACCESS_FAILURE;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 3, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);

    EXPECT_EQ(platform.mTxChannels.size(), 3u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);
}

TEST(RadioSpinelTransmit, shouldStopBurstRightAwayWhenNoFrameIsOnAir)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;
    std::vector<RcpFrame>      frames;

    constexpr spinel_tid_t kBurstTid = 1;
    constexpr spinel_tid_t kStopTid  = 2;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);

    // A timed burst with ticks at 5, 15, 25 ms, ..., each handed to `SubMac` 1 ms before its target TX time. At
    // 14.5 ms, the second tick waits in `SubMac` for its target TX time.
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = static_cast<uint32_t>(platform.GetNow());
    txFrame.mInfo.mTxInfo.mTxDelay         = 5000;
    ASSERT_EQ(SendTxBurst(platform, kBurstTid, txFrame, /* aBurstCount */ 10, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(14500);
    ASSERT_EQ(platform.mTxChannels.size(), 1u);

    ASSERT_EQ(SendStopBurst(platform, kStopTid), kErrorNone);
    platform.GoInUs(1);

    // The burst ends right away: the reply reads no ongoing burst, and no notification is sent.
    int replyIndex = FindRcpFrame(frames, kStopTid, SPINEL_PROP_RCP_TX_BURST);

    ASSERT_GE(replyIndex, 0);
    EXPECT_EQ(frames[replyIndex].mBurstCount, 0u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 0);

    // No more ticks are sent, including the one that was waiting in `SubMac`.
    platform.GoInMs(100);
    EXPECT_EQ(platform.mTxChannels.size(), 1u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 0);

    // Stopping again, with no ongoing burst, also succeeds and notifies nothing.
    ASSERT_EQ(SendStopBurst(platform, /* aTid */ 3), kErrorNone);
    platform.GoInUs(1);
    ASSERT_GE(FindRcpFrame(frames, /* aTid */ 3, SPINEL_PROP_RCP_TX_BURST), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, /* aTid */ 3, SPINEL_PROP_RCP_TX_BURST)].mBurstCount, 0u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 0);
}

TEST(RadioSpinelTransmit, shouldStopBurstWhenFrameOnAirIsDone)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;
    std::vector<RcpFrame>   frames;

    constexpr spinel_tid_t kBurstTid    = 1;
    constexpr spinel_tid_t kStopTid     = 2;
    constexpr uint32_t     kChannelMask = (1UL << 11) | (1UL << 15);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    txFrame.mLength             = OT_RADIO_FRAME_MAX_SIZE; // 4296 us on air, including ramp-up.
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);
    ASSERT_EQ(SendTxBurst(platform, kBurstTid, txFrame, /* aBurstCount */ 10, /* aBurstPeriodUs */ 20000, kChannelMask),
              kErrorNone);

    // The first channel of the first tick is on air.
    platform.GoInUs(1000);
    ASSERT_EQ(platform.mTxRecords.size(), 1u);

    const otRadioTime64 frameEnd = platform.mTxRecords[0].mEnd;

    ASSERT_LT(platform.ToRadioTime(platform.GetNow()), frameEnd);

    // The reply reads the burst as still ongoing with only the tick in progress, and nothing is notified yet.
    ASSERT_EQ(SendStopBurst(platform, kStopTid), kErrorNone);
    platform.GoInUs(1);
    ASSERT_GE(FindRcpFrame(frames, kStopTid, SPINEL_PROP_RCP_TX_BURST), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kStopTid, SPINEL_PROP_RCP_TX_BURST)].mBurstCount, 1u);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 0);

    // The burst ends when the frame on air is done, without sending the other channel of the tick.
    platform.GoInMs(100);
    ASSERT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);
    EXPECT_EQ(platform.ToRadioTime(frames[FindRcpFrame(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST)].mTimeUs),
              frameEnd);
    EXPECT_EQ(platform.mTxRecords.size(), 1u);
}

TEST(RadioSpinelTransmit, shouldAbortTimedTicksThatCannotBeSentOnTime)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;
    std::vector<RcpFrame>   frames;

    constexpr uint16_t kBurstCount = 3;
    constexpr uint32_t kTxDelay    = 5000;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 20);
    txFrame.mInfo.mTxInfo.mCsmaCaEnabled = true;

    // A ramp-up time longer than `SubMac` budgets makes the end of SFD 40 us later than the target TX time.
    platform.mTxTiming.mEnabled  = true;
    platform.mTxTiming.mRampUpUs = 80;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);

    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = static_cast<uint32_t>(platform.GetRadioNow());
    txFrame.mInfo.mTxInfo.mTxDelay         = kTxDelay;

    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, kBurstCount, /* aBurstPeriodUs */ 7500,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(50);

    // Every tick is aborted (no retry), and the burst still ends.
    ASSERT_EQ(platform.mTxRecords.size(), kBurstCount);
    for (const FakePlatform::TxRecord &record : platform.mTxRecords)
    {
        EXPECT_TRUE(record.mIsTimed);
        EXPECT_TRUE(record.mAborted);
    }

    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);
}

TEST(RadioSpinelTransmit, shouldRejectTransmitAndEnergyScanDuringBurst)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;
    std::vector<RcpFrame>   frames;

    constexpr spinel_tid_t kBurstTid     = 1;
    constexpr spinel_tid_t kRawTid       = 2;
    constexpr spinel_tid_t kBurst2Tid    = 3;
    constexpr uint8_t      kBurstChannel = 15;
    constexpr uint8_t      kNewChannel   = 20;
    constexpr uint16_t     kBurstCount   = 3;

    PrepareTestFrame(txFrame, frameBuffer, kBurstChannel);
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);
    ASSERT_EQ(SendTxBurst(platform, kBurstTid, txFrame, kBurstCount, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);

    platform.GoInUs(5000);
    ASSERT_EQ(platform.mTxRecords.size(), 1u);

    // A single transmission, another burst, and an energy scan are rejected right away.
    txFrame.mChannel = kNewChannel;
    ASSERT_EQ(SendStreamRaw(platform, kRawTid, txFrame), kErrorNone);
    ASSERT_EQ(SendTxBurst(platform, kBurst2Tid, txFrame, kBurstCount, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(1);
    EXPECT_EQ(platform.mRadioSpinel.EnergyScan(kBurstChannel, /* aScanDuration */ 10), kErrorInvalidState);

    ASSERT_GE(FindRcpFrame(frames, kRawTid, SPINEL_PROP_LAST_STATUS), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kRawTid, SPINEL_PROP_LAST_STATUS)].mStatus,
              static_cast<unsigned int>(SPINEL_STATUS_INVALID_STATE));
    ASSERT_GE(FindRcpFrame(frames, kBurst2Tid, SPINEL_PROP_LAST_STATUS), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kBurst2Tid, SPINEL_PROP_LAST_STATUS)].mStatus,
              static_cast<unsigned int>(SPINEL_STATUS_BUSY));

    // The burst is not affected.
    platform.GoInMs(50);
    ASSERT_EQ(platform.mTxRecords.size(), kBurstCount);

    for (const auto &record : platform.mTxRecords)
    {
        EXPECT_EQ(record.mChannel, kBurstChannel);
    }

    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);

    // A new transmission is accepted once the burst is done.
    ASSERT_EQ(SendStreamRaw(platform, /* aTid */ 4, txFrame), kErrorNone);
    platform.GoInMs(10);
    ASSERT_EQ(platform.mTxRecords.size(), kBurstCount + 1u);
    EXPECT_EQ(platform.mTxRecords.back().mChannel, kNewChannel);
}

TEST(RadioSpinelTransmit, shouldRejectBurstDuringSingleTransmit)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame            txFrame;
    std::vector<RcpFrame>   frames;

    constexpr spinel_tid_t kRawTid   = 1;
    constexpr spinel_tid_t kBurstTid = 2;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);
    txFrame.mLength             = OT_RADIO_FRAME_MAX_SIZE; // 4296 us on air, including ramp-up.
    platform.mTxTiming.mEnabled = true;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);
    ASSERT_EQ(SendStreamRaw(platform, kRawTid, txFrame), kErrorNone);
    platform.GoInUs(1000);
    ASSERT_EQ(platform.mTxRecords.size(), 1u);

    ASSERT_EQ(SendTxBurst(platform, kBurstTid, txFrame, /* aBurstCount */ 3, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(1);
    ASSERT_GE(FindRcpFrame(frames, kBurstTid, SPINEL_PROP_LAST_STATUS), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kBurstTid, SPINEL_PROP_LAST_STATUS)].mStatus,
              static_cast<unsigned int>(SPINEL_STATUS_BUSY));

    // The single transmission completes normally.
    platform.GoInMs(10);
    ASSERT_GE(FindRcpFrame(frames, kRawTid, SPINEL_PROP_LAST_STATUS), 0);
    EXPECT_EQ(frames[FindRcpFrame(frames, kRawTid, SPINEL_PROP_LAST_STATUS)].mStatus,
              static_cast<unsigned int>(SPINEL_STATUS_OK));
    EXPECT_EQ(platform.mTxRecords.size(), 1u);
}

TEST(RadioSpinelTransmit, shouldNotifyEndOfBurstWhenLinkRawIsDisabled)
{
    TimedBurstRecorderPlatform platform;
    uint8_t                    frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame               txFrame;
    std::vector<RcpFrame>      frames;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 15);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ObserveRcpFrames(platform, frames);
    ASSERT_EQ(SendTxBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 10, /* aBurstPeriodUs */ 10000,
                          /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInUs(15000);
    ASSERT_EQ(platform.mTxChannels.size(), 2u);

    ASSERT_EQ(SendSpinelCommand(platform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_ENABLED, /* aTid */ 2,
                                SPINEL_DATATYPE_BOOL_S, false),
              kErrorNone);
    platform.GoInUs(1);
    EXPECT_EQ(CountRcpFrames(frames, /* aTid */ 0, SPINEL_PROP_RCP_TX_BURST), 1);

    platform.GoInMs(100);
    EXPECT_EQ(platform.mTxChannels.size(), 2u);
}
#endif // OPENTHREAD_CONFIG_LINK_EXT_ENABLE
