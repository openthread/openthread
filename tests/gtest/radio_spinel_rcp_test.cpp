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

otError SendStreamRawBurst(FakeCoprocessorPlatform &aPlatform,
                           spinel_tid_t             aTid,
                           const Mac::TxFrame      &aFrame,
                           uint16_t                 aBurstCount,
                           uint16_t                 aBurstPeriod,
                           uint32_t                 aBurstChannelMask,
                           bool                     aIncludeChannelMask = true)
{
    if (aIncludeChannelMask)
    {
        return SendSpinelCommand(
            aPlatform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW, aTid,
            SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S
                SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S
                    SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S
                        SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT32_S,
            aFrame.mPsdu, aFrame.mLength, aFrame.mChannel, aFrame.mInfo.mTxInfo.mMaxCsmaBackoffs,
            aFrame.mInfo.mTxInfo.mMaxFrameRetries, aFrame.mInfo.mTxInfo.mCsmaCaEnabled,
            aFrame.mInfo.mTxInfo.mIsHeaderUpdated, aFrame.mInfo.mTxInfo.mIsARetx,
            aFrame.mInfo.mTxInfo.mIsSecurityProcessed, aFrame.mInfo.mTxInfo.mTxDelay,
            aFrame.mInfo.mTxInfo.mTxDelayBaseTime, aFrame.mInfo.mTxInfo.mRxChannelAfterTxDone,
            aFrame.mInfo.mTxInfo.mTxPower, aBurstCount, aBurstPeriod, aBurstChannelMask);
    }

    return SendSpinelCommand(
        aPlatform, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW, aTid,
        SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S
            SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S
                SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S
                    SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT16_S,
        aFrame.mPsdu, aFrame.mLength, aFrame.mChannel, aFrame.mInfo.mTxInfo.mMaxCsmaBackoffs,
        aFrame.mInfo.mTxInfo.mMaxFrameRetries, aFrame.mInfo.mTxInfo.mCsmaCaEnabled,
        aFrame.mInfo.mTxInfo.mIsHeaderUpdated, aFrame.mInfo.mTxInfo.mIsARetx, aFrame.mInfo.mTxInfo.mIsSecurityProcessed,
        aFrame.mInfo.mTxInfo.mTxDelay, aFrame.mInfo.mTxInfo.mTxDelayBaseTime,
        aFrame.mInfo.mTxInfo.mRxChannelAfterTxDone, aFrame.mInfo.mTxInfo.mTxPower, aBurstCount, aBurstPeriod);
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
    constexpr uint8_t  kRxChannelAfterTx = 20;
    constexpr uint16_t kBurstCount       = 4;
    constexpr uint16_t kBurstPeriod      = 12; // 12 * 625 us = 7500 us
    constexpr uint64_t kExpectedPeriodUs = 7500;

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = kRxChannelAfterTx;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod,
                                 /* aBurstChannelMask */ 0, /* aIncludeChannelMask */ false),
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

    EXPECT_EQ(platform.GetReceiveChannel(), kRxChannelAfterTx);
}

TEST(RadioSpinelTransmit, shouldTransmitBackToBackBurstAndIgnoreBurstParamsWhenCountIsZero)
{
    class BackToBackMockPlatform : public FakeCoprocessorPlatform
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

    BackToBackMockPlatform platform;
    uint8_t                frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame           txFrame;

    constexpr uint8_t  kTxChannel   = 15;
    constexpr uint16_t kBurstCount  = 4;
    constexpr uint32_t kChannelMask = (1UL << 11) | (1UL << 26);

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // 1. BurstCount == 0 must ignore non-zero BurstPeriod and BurstChannelMask, and allow AckRequest == true.
    txFrame.SetAckRequest(true);
    ASSERT_EQ(
        SendStreamRawBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 0, /* aBurstPeriod */ 12, kChannelMask),
        kErrorNone);
    platform.GoInMs(30);
    ASSERT_EQ(platform.mTxChannels.size(), 1u);
    EXPECT_EQ(platform.mTxChannels[0], kTxChannel);

    // 2. BurstCount > 1 with BurstPeriod == 0 must transmit back-to-back (0 us interval between ticks).
    platform.mTxChannels.clear();
    platform.mTxTimesUs.clear();
    txFrame.SetAckRequest(false);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 2, txFrame, kBurstCount, /* aBurstPeriod */ 0,
                                 /* aBurstChannelMask */ 0, /* aIncludeChannelMask */ false),
              kErrorNone);
    platform.GoInMs(10);
    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        EXPECT_EQ(platform.mTxChannels[i], kTxChannel);
        EXPECT_EQ(platform.mTxTimesUs[i], platform.mTxTimesUs[0]);
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
    constexpr uint16_t kBurstPeriod      = 12; // 12 * 625 us = 7500 us
    constexpr uint32_t kInitialBaseTime  = 1000;
    constexpr uint32_t kInitialTxDelay   = 5000;
    constexpr uint32_t kExpectedPeriodUs = 7500;

    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.mInfo.mTxInfo.mTxDelayBaseTime = kInitialBaseTime;
    txFrame.mInfo.mTxInfo.mTxDelay         = kInitialTxDelay;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    platform.GoInUs(kInitialBaseTime);

    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), kBurstCount);
    EXPECT_GT(platform.mTxTimesUs[0], kInitialBaseTime);
    for (size_t i = 0; i < kBurstCount; ++i)
    {
        EXPECT_EQ(platform.mTxChannels[i], kTxChannel);
        EXPECT_EQ(platform.mTxTimesUs[i] - platform.mTxTimesUs[0], i * kExpectedPeriodUs);
        EXPECT_EQ(platform.mTxDelayBaseTimes[i], kInitialBaseTime);
        EXPECT_EQ(platform.mTxDelays[i], kInitialTxDelay + i * kExpectedPeriodUs);
    }
}

TEST(RadioSpinelTransmit, shouldTransmitSingleMultiChannelSweep)
{
    class SweepMockPlatform : public FakeCoprocessorPlatform
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

    SweepMockPlatform platform;
    uint8_t           frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame      txFrame;

    constexpr uint8_t  kInitialChannel   = 15;
    constexpr uint8_t  kRxChannelAfterTx = 22;
    constexpr uint32_t kChannelMask      = (1UL << 11) | (1UL << 18) | (1UL << 26);

    PrepareTestFrame(txFrame, frameBuffer, kInitialChannel);
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = kRxChannelAfterTx;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(
        SendStreamRawBurst(platform, /* aTid */ 1, txFrame, /* aBurstCount */ 1, /* aBurstPeriod */ 0, kChannelMask),
        kErrorNone);

    platform.GoInMs(10);

    ASSERT_EQ(platform.mTxChannels.size(), 3u);
    EXPECT_EQ(platform.mTxChannels[0], 11);
    EXPECT_EQ(platform.mTxChannels[1], 18);
    EXPECT_EQ(platform.mTxChannels[2], 26);
    EXPECT_EQ(platform.GetReceiveChannel(), kRxChannelAfterTx);
}

TEST(RadioSpinelTransmit, shouldTransmitPeriodicMultiChannelBurst)
{
    class MultiChannelBurstMockPlatform : public FakeCoprocessorPlatform
    {
    public:
        otError Transmit(otRadioFrame *aFrame) override
        {
            mTxChannels.push_back(aFrame->mChannel);
            mTxTimesUs.push_back(GetNow());
            otPlatRadioTxStarted(CurrentInstance(), aFrame);
            otPlatRadioTxDone(CurrentInstance(), aFrame, nullptr, OT_ERROR_NONE);
            return OT_ERROR_NONE;
        }

        std::vector<uint8_t>  mTxChannels;
        std::vector<uint64_t> mTxTimesUs;
    };

    MultiChannelBurstMockPlatform platform;
    uint8_t                       frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame                  txFrame;

    constexpr uint16_t kBurstCount       = 3;
    constexpr uint16_t kBurstPeriod      = 16; // 16 * 625 us = 10000 us
    constexpr uint32_t kChannelMask      = (1UL << 12) | (1UL << 24);
    constexpr uint64_t kExpectedPeriodUs = 10000;

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 11);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), 6u);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        EXPECT_EQ(platform.mTxChannels[tick * 2], 12);
        EXPECT_EQ(platform.mTxChannels[tick * 2 + 1], 24);
        EXPECT_EQ(platform.mTxTimesUs[tick * 2] - platform.mTxTimesUs[0], tick * kExpectedPeriodUs);
        EXPECT_EQ(platform.mTxTimesUs[tick * 2 + 1] - platform.mTxTimesUs[0], tick * kExpectedPeriodUs);
    }
}

TEST(RadioSpinelTransmit, shouldNotStopBurstOnReceiveAndShouldPreemptOnNewTransmit)
{
    class PreemptMockPlatform : public FakeCoprocessorPlatform
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

    PreemptMockPlatform platform;
    uint8_t             frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    Mac::TxFrame        txFrame;

    constexpr uint8_t  kBurstChannel   = 15;
    constexpr uint8_t  kPreemptChannel = 21;
    constexpr uint16_t kBurstCount     = 10;
    constexpr uint16_t kBurstPeriod    = 16; // 10000 us

    PrepareTestFrame(txFrame, frameBuffer, kBurstChannel);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod,
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

    // Now send a new single-frame TX request via RadioSpinel::Transmit on kPreemptChannel to preempt the ongoing burst.
    // This also verifies that TxDone was already delivered to the host after the first burst tick.
    ASSERT_EQ(platform.mRadioSpinel.Receive(kBurstChannel), kErrorNone);
    txFrame.mChannel                            = kPreemptChannel;
    txFrame.mInfo.mTxInfo.mRxChannelAfterTxDone = kPreemptChannel;
    ASSERT_EQ(platform.mRadioSpinel.Transmit(txFrame), kErrorNone);

    // Run 100 ms past the original burst window; only the single preempting frame should be sent.
    platform.GoInMs(100);
    ASSERT_EQ(platform.mTxChannels.size(), 4u);
    EXPECT_EQ(platform.mTxChannels[3], kPreemptChannel);
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
    constexpr uint16_t kBurstPeriod = 12;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // 1. Burst frame with AckRequest == true must be rejected.
    PrepareTestFrame(txFrame, frameBuffer, kTxChannel);
    txFrame.SetAckRequest(true);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
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

    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 2, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
              kErrorNone);
    platform.GoInMs(30);
    EXPECT_EQ(platform.mTxChannels.size(), 0u);

    // 3. Same secured frame with mIsSecurityProcessed == true must succeed.
    txFrame.mInfo.mTxInfo.mIsSecurityProcessed = true;
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 3, txFrame, kBurstCount, kBurstPeriod, /* aBurstChannelMask */ 0),
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
    constexpr uint16_t kBurstPeriod = 16; // 10000 us
    constexpr uint32_t kChannelMask = (1UL << 11) | (1UL << 15);

    PrepareTestFrame(txFrame, frameBuffer, /* aChannel */ 11);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(SendStreamRawBurst(platform, /* aTid */ 1, txFrame, kBurstCount, kBurstPeriod, kChannelMask), kErrorNone);

    platform.GoInMs(50);

    ASSERT_EQ(platform.mTxChannels.size(), 6u);
    for (size_t tick = 0; tick < kBurstCount; ++tick)
    {
        EXPECT_EQ(platform.mTxChannels[tick * 2], 11);
        EXPECT_EQ(platform.mTxChannels[tick * 2 + 1], 15);
    }
}
