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

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <openthread/platform/radio.h>

#include "common/error.hpp"
#include "crypto/aes_ccm.hpp"
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

#if OPENTHREAD_CONFIG_LINK_EXT_ENABLE && (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
static_assert(OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES == 3,
              "The periodic RX tests assume the default number of schedules");

namespace {

using PeriodicRxSchedule = Spinel::RadioSpinel::PeriodicRxSchedule;

constexpr uint32_t kShrDurationUs   = 160; // SHR duration, part of the minimum kept window fragment.
constexpr uint8_t  kMinNumSchedules = 3;   // Recommended minimum (and default) number of schedules.

bool operator==(const PeriodicRxSchedule &aLhs, const PeriodicRxSchedule &aRhs)
{
    return aLhs.mPhaseUs == aRhs.mPhaseUs && aLhs.mPeriodUs == aRhs.mPeriodUs && aLhs.mDurationUs == aRhs.mDurationUs &&
           aLhs.mChannel == aRhs.mChannel;
}

/**
 * Records every `otPlatRadioReceiveAt()` call made by the RCP.
 *
 * `aStart` is interpreted as the lower 32 bits of the absolute radio time, as specified by `otPlatRadioReceiveAt()`.
 */
class PeriodicRxPlatform : public FakeCoprocessorPlatform
{
public:
    struct Window
    {
        uint64_t mCalledAt;
        uint64_t mStart;
        uint32_t mDuration;
        uint8_t  mChannel;
    };

    otError ReceiveAt(uint8_t aChannel, uint32_t aStart, uint32_t aDuration) override
    {
        uint64_t start = mNow + static_cast<int32_t>(aStart - static_cast<uint32_t>(mNow));

        if (aDuration == 0)
        {
            mNumCancels++;
        }
        else
        {
            mWindows.push_back({mNow, start, aDuration, aChannel});
        }

        mReceiveAtChannel = aChannel;
        mReceiveAtStart   = start;
        mReceiveAtEnd     = start + aDuration;

        return OT_ERROR_NONE;
    }

    void SetNow(uint64_t aNow) { mNow = aNow; }

    // Returns the windows whose start is before `aBefore`. The last window is
    // scheduled ahead of its start, so it may be beyond the simulated time.
    std::vector<Window> GetWindowsBefore(uint64_t aBefore) const
    {
        std::vector<Window> windows;

        for (const Window &window : mWindows)
        {
            if (window.mStart < aBefore)
            {
                windows.push_back(window);
            }
        }

        return windows;
    }

    std::vector<Window> mWindows;
    uint32_t            mNumCancels = 0;
};

} // namespace

TEST(RadioSpinelPeriodicRx, shouldGetTheSchedulesThatWereSet)
{
    const PeriodicRxSchedule kSchedules[] = {
        {1000, 500000, 1600, 11},
        {0, 1000000, 65535, 15},
        {0xffffff, 0xffffffff, 1, 26},
    };

    FakeCoprocessorPlatform platform;
    PeriodicRxSchedule      schedules[kMinNumSchedules];
    uint8_t                 numSchedules = kMinNumSchedules;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    numSchedules = kMinNumSchedules;
    ASSERT_EQ(platform.mRadioSpinel.GetPeriodicRxSchedules(schedules, numSchedules), kErrorNone);
    EXPECT_EQ(numSchedules, 0);

    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 3), kErrorNone);

    numSchedules = kMinNumSchedules;
    ASSERT_EQ(platform.mRadioSpinel.GetPeriodicRxSchedules(schedules, numSchedules), kErrorNone);
    ASSERT_EQ(numSchedules, 3);

    for (uint8_t i = 0; i < numSchedules; i++)
    {
        EXPECT_TRUE(schedules[i] == kSchedules[i]) << "index " << static_cast<int>(i);
    }

    // An empty table disables periodic RX.
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(nullptr, 0), kErrorNone);

    numSchedules = kMinNumSchedules;
    ASSERT_EQ(platform.mRadioSpinel.GetPeriodicRxSchedules(schedules, numSchedules), kErrorNone);
    EXPECT_EQ(numSchedules, 0);
}

TEST(RadioSpinelPeriodicRx, shouldRejectInvalidSchedulesAndKeepThePreviousOnes)
{
    const PeriodicRxSchedule kValid[]      = {{1000, 500000, 1600, 11}};
    const PeriodicRxSchedule kInvalid[][1] = {
        {{1000, 500000, 1600, 10}},   // Channel not in the supported channel mask (11-26).
        {{1000, 500000, 1600, 27}},   // Channel not in the supported channel mask (11-26).
        {{0, 0, 1600, 11}},           // Zero period.
        {{500000, 500000, 1600, 11}}, // Phase not less than period.
        {{1000, 500000, 0, 11}},      // Zero duration.
        {{0, 1000, 1001, 11}},        // Duration longer than period.
    };
    const PeriodicRxSchedule kTooMany[] = {
        {0, 500000, 160, 11},
        {0, 500000, 160, 12},
        {0, 500000, 160, 13},
        {0, 500000, 160, 14},
    };

    FakeCoprocessorPlatform platform;
    PeriodicRxSchedule      schedules[kMinNumSchedules];
    uint8_t                 numSchedules;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kValid, 1), kErrorNone);

    for (const auto &invalid : kInvalid)
    {
        EXPECT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(invalid, 1), kErrorInvalidArgs);
    }

    // More schedules than the RCP supports (`OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES`).
    EXPECT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kTooMany, 4), kErrorNoBufs);

    numSchedules = kMinNumSchedules;
    ASSERT_EQ(platform.mRadioSpinel.GetPeriodicRxSchedules(schedules, numSchedules), kErrorNone);
    ASSERT_EQ(numSchedules, 1);
    EXPECT_TRUE(schedules[0] == kValid[0]);
}

TEST(RadioSpinelPeriodicRx, shouldStartWindowsWhenRadioTimeModuloPeriodEqualsPhase)
{
    constexpr uint32_t kPhaseUs    = 3000;
    constexpr uint32_t kPeriodUs   = 100000;
    constexpr uint16_t kDurationUs = 1250;
    constexpr uint8_t  kChannel    = 15;

    const PeriodicRxSchedule kSchedules[] = {{kPhaseUs, kPeriodUs, kDurationUs, kChannel}};

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 1), kErrorNone);

    platform.GoInMs(1000);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 9u);

    for (size_t i = 0; i < windows.size(); i++)
    {
        EXPECT_EQ(windows[i].mStart % kPeriodUs, kPhaseUs);
        EXPECT_EQ(windows[i].mDuration, kDurationUs);
        EXPECT_EQ(windows[i].mChannel, kChannel);
        EXPECT_LE(windows[i].mCalledAt, windows[i].mStart);

        if (i > 0)
        {
            EXPECT_EQ(windows[i].mStart - windows[i - 1].mStart, kPeriodUs);
        }
    }
}

TEST(RadioSpinelPeriodicRx, shouldUseFull64BitRadioTimeAcross32BitWrap)
{
    constexpr uint32_t kPhaseUs  = 12345;
    constexpr uint32_t kPeriodUs = 30000;
    constexpr uint64_t kWrap     = (1ULL << 32);

    const PeriodicRxSchedule kSchedules[] = {{kPhaseUs, kPeriodUs, 800, 20}};

    PeriodicRxPlatform platform;

    platform.SetNow(kWrap - 100000);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 1), kErrorNone);

    platform.GoInMs(300);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());
    bool                                    crossed = false;

    ASSERT_GE(windows.size(), 8u);

    for (size_t i = 0; i < windows.size(); i++)
    {
        // `kWrap` is not a multiple of `kPeriodUs`, so this only holds if the
        // modulo is evaluated on the 64-bit time.
        EXPECT_EQ(windows[i].mStart % kPeriodUs, kPhaseUs);

        if (i > 0)
        {
            EXPECT_EQ(windows[i].mStart - windows[i - 1].mStart, kPeriodUs);
        }

        crossed |= (windows[i].mStart >= kWrap);
    }

    EXPECT_TRUE(crossed);
}

TEST(RadioSpinelPeriodicRx, shouldKeepSchedulingWindowsAcrossReceiveAndSleep)
{
    // Timed RX windows only take effect while the radio is in the Sleep
    // state, which is enforced by the radio platform per the contract of
    // `otPlatRadioReceiveAt()`. The RCP keeps scheduling windows (without
    // drift) while the radio is in Receive state.

    constexpr uint32_t kPeriodUs = 50000;

    const PeriodicRxSchedule kSchedules[] = {{0, kPeriodUs, 1600, 11}};

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 1), kErrorNone);

    platform.GoInMs(200);
    ASSERT_EQ(platform.mRadioSpinel.Receive(11), kErrorNone);
    platform.GoInMs(500);
    ASSERT_EQ(platform.mRadioSpinel.Sleep(), kErrorNone);
    platform.GoInMs(500);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 23u);
    EXPECT_EQ(platform.mNumCancels, 0u);

    for (size_t i = 0; i < windows.size(); i++)
    {
        EXPECT_EQ(windows[i].mStart % kPeriodUs, 0u);
        EXPECT_EQ(windows[i].mDuration, 1600u);

        if (i > 0)
        {
            EXPECT_EQ(windows[i].mStart - windows[i - 1].mStart, kPeriodUs);
        }
    }
}

TEST(RadioSpinelPeriodicRx, shouldStopSchedulingWindowsWhenSchedulesAreCleared)
{
    const PeriodicRxSchedule kSchedules[] = {{0, 50000, 1600, 15}};

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 1), kErrorNone);
    platform.GoInMs(200);

    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(nullptr, 0), kErrorNone);

    // A window already scheduled on the radio is not cancelled (it may
    // still occur once), but no further windows are scheduled.
    platform.mWindows.clear();
    platform.GoInMs(500);
    EXPECT_TRUE(platform.mWindows.empty());
}

TEST(RadioSpinelPeriodicRx, shouldKeepTheRemainderOfAWindowPreemptedAtItsStart)
{
    // Schedule 0 on channel 11 at [0, 1600) every 10 ms.
    // Schedule 1 on channel 12 at 1000 + 25000k (1600 us): occurrences
    // alternate between overlapping schedule 0 (1000 mod 10000), which
    // keeps the remainder [1600, 2600), and not (6000 mod 10000).
    const PeriodicRxSchedule kSchedules[] = {
        {0, 10000, 1600, 11},
        {1000, 25000, 1600, 12},
    };

    PeriodicRxPlatform platform;
    size_t             numChannel11 = 0;
    size_t             numFull      = 0;
    size_t             numPartial   = 0;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 2), kErrorNone);

    platform.GoInMs(500);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    for (size_t i = 0; i < windows.size(); i++)
    {
        const PeriodicRxPlatform::Window &window = windows[i];

        if (window.mChannel == 11)
        {
            numChannel11++;
            EXPECT_EQ(window.mStart % 10000, 0u);
            EXPECT_EQ(window.mDuration, 1600u);
        }
        else if (window.mStart % 10000 == 6000)
        {
            numFull++;
            EXPECT_EQ(window.mChannel, 12);
            EXPECT_EQ(window.mStart % 25000, 1000u);
            EXPECT_EQ(window.mDuration, 1600u);
        }
        else
        {
            numPartial++;
            EXPECT_EQ(window.mChannel, 12);
            EXPECT_EQ(window.mStart % 10000, 1600u);
            EXPECT_EQ((window.mStart - 600) % 25000, 1000u);
            EXPECT_EQ(window.mDuration, 1000u);
        }

        if (i > 0)
        {
            EXPECT_GE(window.mStart, windows[i - 1].mStart + windows[i - 1].mDuration);
        }
    }

    EXPECT_GE(numChannel11, 45u);
    EXPECT_GE(numFull, 9u);
    EXPECT_GE(numPartial, 9u);
}

TEST(RadioSpinelPeriodicRx, shouldSplitAPreemptedWindowAndTakeTheGuardTimeFromIt)
{
    // Schedule 1 on channel 12 at [4000, 8000) is preempted by schedule 0
    // on channel 11 at [5000, 6600). Schedule 0 starts the guard time
    // early, which is taken from schedule 1:
    //
    //   channel 12: [4000, 5000 - guard)
    //   channel 11: [5000 - guard, 6600)
    //   channel 12: [6600, 8000)
    constexpr uint32_t kGuardUs = OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US;

    static_assert(1000 - kGuardUs >= kGuardUs + kShrDurationUs, "the leading fragment must be kept");

    const PeriodicRxSchedule kSchedules[] = {
        {5000, 10000, 1600, 11},
        {4000, 10000, 4000, 12},
    };

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 2), kErrorNone);

    platform.GoInMs(200);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 3 * 19u);

    for (const PeriodicRxPlatform::Window &window : windows)
    {
        switch (window.mStart % 10000)
        {
        case 4000:
            EXPECT_EQ(window.mChannel, 12);
            EXPECT_EQ(window.mDuration, 1000 - kGuardUs);
            break;
        case 5000 - kGuardUs:
            EXPECT_EQ(window.mChannel, 11);
            EXPECT_EQ(window.mDuration, 1600 + kGuardUs);
            break;
        case 6600:
            EXPECT_EQ(window.mChannel, 12);
            EXPECT_EQ(window.mDuration, 1400u);
            break;
        default:
            ADD_FAILURE() << "unexpected window start " << window.mStart;
            break;
        }
    }
}

TEST(RadioSpinelPeriodicRx, shouldDropAPartialWindowShorterThanTheMinimum)
{
    // Schedule 1 on channel 12 at [kPhase1, kPhase1 + 4000) is preempted by
    // schedule 0 on channel 11 at [5000, 6600). The part before the
    // preemption (80 us) is shorter than guard + 160 us and is dropped:
    //
    //   channel 11: [5000 - guard, 6600)
    //   channel 12: [6600, kPhase1 + 4000)
    constexpr uint32_t kGuardUs = OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US;
    constexpr uint32_t kPhase1  = 5000 - kGuardUs - kShrDurationUs / 2;

    static_assert(kPhase1 + 4000 > 6600, "the trailing fragment must exist");

    const PeriodicRxSchedule kSchedules[] = {
        {5000, 10000, 1600, 11},
        {kPhase1, 10000, 4000, 12},
    };

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 2), kErrorNone);

    platform.GoInMs(200);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 2 * 19u);

    for (const PeriodicRxPlatform::Window &window : windows)
    {
        switch (window.mStart % 10000)
        {
        case 5000 - kGuardUs:
            EXPECT_EQ(window.mChannel, 11);
            EXPECT_EQ(window.mDuration, 1600 + kGuardUs);
            break;
        case 6600:
            EXPECT_EQ(window.mChannel, 12);
            EXPECT_EQ(window.mDuration, kPhase1 + 4000 - 6600);
            break;
        default:
            ADD_FAILURE() << "unexpected window start " << window.mStart;
            break;
        }
    }
}

TEST(RadioSpinelPeriodicRx, shouldMergeOverlappingWindowsOnTheSameChannel)
{
    // [0, 1600) and [1000, 2600) on the same channel merge into [0, 2600).
    const PeriodicRxSchedule kSchedules[] = {
        {0, 10000, 1600, 11},
        {1000, 10000, 1600, 11},
    };

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 2), kErrorNone);

    platform.GoInMs(200);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 18u);

    // The first window may be a partial one if the schedules were set in the
    // middle of a period.
    for (size_t i = 1; i < windows.size(); i++)
    {
        EXPECT_EQ(windows[i].mStart % 10000, 0u);
        EXPECT_EQ(windows[i].mDuration, 2600u);
        EXPECT_EQ(windows[i].mChannel, 11);
    }
}

TEST(RadioSpinelPeriodicRx, shouldSplitContinuousCoverageIntoBackToBackWindows)
{
    // Schedule 0 covers channel 11 continuously (duration == period). With a
    // long-period schedule on the same channel, merging used to walk every
    // event up to the long period. Merging is now bounded, and the continuous
    // coverage is split into back-to-back windows on the same channel.
    const PeriodicRxSchedule kSchedules[] = {
        {0, 160, 160, 11},
        {0, 4000000000UL, 160, 11},
    };

    PeriodicRxPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 2), kErrorNone);

    platform.GoInMs(100);

    std::vector<PeriodicRxPlatform::Window> windows = platform.GetWindowsBefore(platform.GetNow());

    ASSERT_GE(windows.size(), 2u);

    for (size_t i = 0; i < windows.size(); i++)
    {
        EXPECT_EQ(windows[i].mChannel, 11);
        EXPECT_LT(windows[i].mDuration, 100000u);

        if (i > 0)
        {
            EXPECT_EQ(windows[i].mStart, windows[i - 1].mStart + windows[i - 1].mDuration);
        }
    }
}

#else // OPENTHREAD_CONFIG_LINK_EXT_ENABLE && (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)

TEST(RadioSpinelPeriodicRx, shouldReportNotImplementedByAnRcpWithoutPeriodicRx)
{
    const Spinel::RadioSpinel::PeriodicRxSchedule kSchedules[] = {{0, 50000, 1600, 11}};

    FakeCoprocessorPlatform                 platform;
    Spinel::RadioSpinel::PeriodicRxSchedule schedules[3];
    uint8_t                                 numSchedules = 3;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    EXPECT_EQ(platform.mRadioSpinel.GetPeriodicRxSchedules(schedules, numSchedules), kErrorNotImplemented);
    EXPECT_EQ(numSchedules, 0);
    EXPECT_EQ(platform.mRadioSpinel.SetPeriodicRxSchedules(kSchedules, 1), kErrorNotImplemented);
}

#endif // OPENTHREAD_CONFIG_LINK_EXT_ENABLE && (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)

#if OPENTHREAD_CONFIG_LINK_EXT_ENABLE

namespace {

otRadioFrame sLastMacFilterRxFrame;
bool         sMacFilterFrameReceived = false;

void HandleMacFilterReceiveDone(otInstance *, otRadioFrame *aFrame, otError aError)
{
    if (aError == OT_ERROR_NONE && aFrame != nullptr)
    {
        sMacFilterFrameReceived = true;
        sLastMacFilterRxFrame   = *aFrame;
    }
}

void PrepareUnsecuredFrame(Mac::TxFrame       &aTxFrame,
                           uint8_t            *aPsdu,
                           uint8_t            &aLength,
                           Mac::Frame::Version aVersion,
                           const Mac::Address &aSrcAddr,
                           const Mac::Address &aDstAddr,
                           Mac::PanId          aSrcPanId,
                           Mac::PanId          aDstPanId,
                           const uint8_t      *aPayload,
                           uint16_t            aPayloadLength)
{
    aTxFrame.mPsdu = aPsdu;

    Mac::TxFrame::BuildInfo buildInfo;
    buildInfo.mType               = Mac::Frame::kTypeData;
    buildInfo.mVersion            = aVersion;
    buildInfo.mAddrs.mSource      = aSrcAddr;
    buildInfo.mAddrs.mDestination = aDstAddr;
    buildInfo.mPanIds.SetSource(aSrcPanId);
    buildInfo.mPanIds.SetDestination(aDstPanId);
    buildInfo.mSecurityLevel = Mac::Frame::kSecurityNone;

    Mac::TxFrame::PayloadBuilder builder;
    aTxFrame.PrepareHeaders(buildInfo, builder);
    if (aPayload != nullptr && aPayloadLength > 0)
    {
        EXPECT_EQ(builder.AppendBytes(aPayload, aPayloadLength), kErrorNone);
    }
    aTxFrame.FinishPayload(builder);
    aLength = aTxFrame.GetLength();
}

void PrepareSecuredFrame(Mac::TxFrame          &aTxFrame,
                         uint8_t               *aPsdu,
                         uint8_t               &aLength,
                         Mac::Frame::Version    aVersion,
                         const Mac::Address    &aSrcAddr,
                         const Mac::Address    &aDstAddr,
                         Mac::PanId             aSrcPanId,
                         Mac::PanId             aDstPanId,
                         Mac::Frame::KeyIdMode  aKeyIdMode,
                         const uint8_t         *aKeyId,
                         uint32_t               aFrameCounter,
                         const uint8_t         *aPayload,
                         uint16_t               aPayloadLength,
                         const otMacKey        *aKey,
                         const Mac::ExtAddress *aNonceExtAddr)
{
    aTxFrame.mPsdu = aPsdu;

    Mac::TxFrame::BuildInfo buildInfo;
    buildInfo.mType               = Mac::Frame::kTypeData;
    buildInfo.mVersion            = aVersion;
    buildInfo.mAddrs.mSource      = aSrcAddr;
    buildInfo.mAddrs.mDestination = aDstAddr;
    buildInfo.mPanIds.SetSource(aSrcPanId);
    buildInfo.mPanIds.SetDestination(aDstPanId);
    buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    buildInfo.mKeyIdMode     = aKeyIdMode;

    Mac::TxFrame::PayloadBuilder builder;
    aTxFrame.PrepareHeaders(buildInfo, builder);
    if (aPayload != nullptr && aPayloadLength > 0)
    {
        EXPECT_EQ(builder.AppendBytes(aPayload, aPayloadLength), kErrorNone);
    }
    aTxFrame.FinishPayload(builder);

    Mac::TxFrame::ParseInfo parseInfo;
    EXPECT_EQ(parseInfo.ParseFrom(aTxFrame, Mac::Frame::kParseFully), kErrorNone);
    parseInfo.WriteFrameCounter(aFrameCounter);

    if (aKeyIdMode == Mac::Frame::kKeyIdMode1)
    {
        parseInfo.WriteKeyIndex(aKeyId[0]);
    }
    else if (aKeyIdMode == Mac::Frame::kKeyIdMode2)
    {
        parseInfo.WriteKeySource(aKeyId);
        parseInfo.WriteKeyIndex(aKeyId[4]);
    }

    EXPECT_EQ(parseInfo.ParseFrom(aTxFrame, Mac::Frame::kParseFully), kErrorNone);

    if (aKey != nullptr && aNonceExtAddr != nullptr)
    {
        Crypto::AesCcm        aesCcm;
        Crypto::AesCcm::Nonce nonce;

        nonce.InitFrom(*aNonceExtAddr, aFrameCounter, Mac::Frame::kSecurityEncMic32);
        aesCcm.SetKey(aKey->m8, sizeof(otMacKey));
        aesCcm.SetNonce(nonce);
        aesCcm.SetAuthData(parseInfo.mHeader.GetBytes(), parseInfo.mHeader.GetLength());
        aesCcm.SetTagLength(parseInfo.mMicSize);
        EXPECT_EQ(aesCcm.Process(Crypto::AesCcm::kEncrypt, const_cast<uint8_t *>(parseInfo.mPayload.GetBytes()),
                                 aPayloadLength),
                  kErrorNone);
    }

    aLength = aTxFrame.GetLength();
}

} // namespace

TEST(RadioSpinelMacFilter, PolicySetAndGet)
{
    FakeCoprocessorPlatform platform;
    uint8_t                 policy = 0xff;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

    // Initial default policy is 0 (permissive)
    ASSERT_EQ(platform.mRadioSpinel.GetMacFilterPolicy(policy), kErrorNone);
    EXPECT_EQ(policy, 0);

    // Valid policy bit combinations
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.GetMacFilterPolicy(policy), kErrorNone);
    EXPECT_EQ(policy, SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED);

    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED |
                                                       SPINEL_MAC_FILTER_POLICY_DROP_UNSECURED),
              kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.GetMacFilterPolicy(policy), kErrorNone);
    EXPECT_EQ(policy, SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED | SPINEL_MAC_FILTER_POLICY_DROP_UNSECURED);

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED |
                                                       SPINEL_MAC_FILTER_POLICY_DROP_UNSECURED |
                                                       SPINEL_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY),
              kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.GetMacFilterPolicy(policy), kErrorNone);
    EXPECT_EQ(policy, 0x07);
#else
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY),
              kErrorNotImplemented);
#endif

    // Reserved bits (bits 3-7) must be rejected
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0x08), kErrorInvalidArgs);
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0x10), kErrorInvalidArgs);
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0x80), kErrorInvalidArgs);
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0xf8), kErrorInvalidArgs);

    // Reset back to 0
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.GetMacFilterPolicy(policy), kErrorNone);
    EXPECT_EQ(policy, 0);
}

TEST(RadioSpinelMacFilter, KeyTableSetAndGet)
{
    FakeCoprocessorPlatform platform;

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    constexpr uint8_t                     kNumEntries = 3;
    Spinel::RadioSpinel::MacKeyTableEntry entries[10];
    Spinel::RadioSpinel::MacKeyTableEntry setEntries[kNumEntries];
    uint8_t                               count = OT_ARRAY_LENGTH(entries);

    // Initial table is empty
    ASSERT_EQ(platform.mRadioSpinel.GetMacKeyTable(entries, count), kErrorNone);
    EXPECT_EQ(count, 0);

    for (Spinel::RadioSpinel::MacKeyTableEntry &entry : setEntries)
    {
        entry.Clear();
        entry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);
    }

    // Entry 0: Key ID Mode 2 (4-byte Key Source + 1-byte Key Index)
    setEntries[0].mKeyIdLength = 5;
    for (uint8_t i = 0; i < 5; i++)
    {
        setEntries[0].mKeyId[i] = i + 0x01;
    }

    // Entry 1: Key ID Mode 1 (1-byte Key Index) with all security levels allowed
    setEntries[1].mKeyIdLength      = 1;
    setEntries[1].mKeyId[0]         = 0x81;
    setEntries[1].mAllowedSecLevels = 0xfe;

    // Entry 2: Key ID Mode 0 (no Key ID)
    setEntries[2].mKeyIdLength = 0;

    ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, kNumEntries), kErrorNone);

    // Retrieve and verify
    count = OT_ARRAY_LENGTH(entries);
    ASSERT_EQ(platform.mRadioSpinel.GetMacKeyTable(entries, count), kErrorNone);
    ASSERT_EQ(count, kNumEntries);

    for (uint8_t i = 0; i < kNumEntries; i++)
    {
        EXPECT_EQ(entries[i].mKeyIdLength, setEntries[i].mKeyIdLength);
        EXPECT_EQ(memcmp(entries[i].mKeyId, setEntries[i].mKeyId, setEntries[i].mKeyIdLength), 0);
        EXPECT_FALSE(entries[i].mHasKey);
        EXPECT_EQ(entries[i].mAllowedSecLevels, setEntries[i].mAllowedSecLevels);
    }

    // Clear key table
    ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(nullptr, 0), kErrorNone);
    count = OT_ARRAY_LENGTH(entries);
    ASSERT_EQ(platform.mRadioSpinel.GetMacKeyTable(entries, count), kErrorNone);
    EXPECT_EQ(count, 0);

    // Unsupported / invalid Key ID lengths (2, 4, 9) must be rejected
    setEntries[0].mKeyIdLength = 2;
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mKeyIdLength = 4;
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mKeyIdLength = 9; // Mode 3
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mKeyIdLength = Spinel::RadioSpinel::MacKeyTableEntry::kMaxKeyIdLength + 1; // Rejected by host
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mKeyIdLength = 5;

    // An entry with a key (MIC verification) is not supported
    setEntries[0].mHasKey = true;
    memset(setEntries[0].mKeyMaterial.m8, 0x11, sizeof(otMacKey));
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorNotCapable);
    setEntries[0].mHasKey = false;

    // Bit 0 (Security Level 0: None) or 0 must return kErrorInvalidArgs
    setEntries[0].mAllowedSecLevels = (1 << Mac::Frame::kSecurityNone);
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mAllowedSecLevels = (1 << Mac::Frame::kSecurityNone) | (1 << Mac::Frame::kSecurityEncMic32);
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mAllowedSecLevels = 0;
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(setEntries, 1), kErrorInvalidArgs);
    setEntries[0].mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);

    // More entries than supported must be rejected
    Spinel::RadioSpinel::MacKeyTableEntry overflowEntries[OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES + 1];

    for (Spinel::RadioSpinel::MacKeyTableEntry &entry : overflowEntries)
    {
        entry.Clear();
        entry.mKeyIdLength      = 5;
        entry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);
    }
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(overflowEntries, OT_ARRAY_LENGTH(overflowEntries)), kErrorNoBufs);
#else
    Spinel::RadioSpinel::MacKeyTableEntry entry;
    uint8_t                               count = 1;

    entry.Clear();
    EXPECT_EQ(platform.mRadioSpinel.SetMacKeyTable(&entry, 1), kErrorNotImplemented);
    EXPECT_EQ(platform.mRadioSpinel.GetMacKeyTable(&entry, count), kErrorNotImplemented);
#endif
}

TEST(RadioSpinelMacFilter, FrameFilteringPolicy)
{
    FakeCoprocessorPlatform platform;

    Spinel::RadioSpinelCallbacks callbacks{};
    callbacks.mReceiveDone    = HandleMacFilterReceiveDone;
    callbacks.mTransmitDone   = [](otInstance *, otRadioFrame *, otRadioFrame *, otError) {};
    callbacks.mEnergyScanDone = [](otInstance *, int8_t) {};
    callbacks.mTxStarted      = [](otInstance *, otRadioFrame *) {};
    platform.mRadioSpinel.SetCallbacks(callbacks);

    ASSERT_EQ(platform.mRadioSpinel.Enable(FakePlatform::CurrentInstance()), kErrorNone);
    ASSERT_EQ(platform.mRadioSpinel.Receive(11), kErrorNone);

    uint8_t sentFrame[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t sentFrameLength = 0;

    auto deliverFrame = [&](const uint8_t *aPsdu, uint8_t aLength) {
        sMacFilterFrameReceived = false;
        memcpy(sentFrame, aPsdu, aLength);
        sentFrameLength = aLength;
        otRadioFrame rxFrame{};
        rxFrame.mPsdu               = const_cast<uint8_t *>(aPsdu);
        rxFrame.mLength             = aLength;
        rxFrame.mChannel            = 11;
        rxFrame.mInfo.mRxInfo.mRssi = -50;
        rxFrame.mInfo.mRxInfo.mLqi  = 100;
        otPlatRadioReceiveDone(FakePlatform::CurrentInstance(), &rxFrame, OT_ERROR_NONE);
        platform.GoInMs(10);
        platform.mSpinelDriver.Process(nullptr);
    };

    // A delivered frame must be passed to the host unmodified.
    auto expectDeliveredUnmodified = [&]() {
        EXPECT_TRUE(sMacFilterFrameReceived);
        if (sMacFilterFrameReceived)
        {
            EXPECT_EQ(sLastMacFilterRxFrame.mLength, sentFrameLength);
            EXPECT_EQ(memcmp(sLastMacFilterRxFrame.mPsdu, sentFrame, sentFrameLength), 0);
        }
    };

    // 1. Malformed frame check
    uint8_t malformedFrame[] = {0x01, 0x02}; // 2 bytes: invalid IEEE 802.15.4 frame

    // Policy = 0 (permissive): malformed frame is passed through to host
    ASSERT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(0), kErrorNone);
    deliverFrame(malformedFrame, sizeof(malformedFrame));
    expectDeliveredUnmodified();

    // Policy = DROP_MALFORMED: malformed frame is dropped
    ASSERT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED), kErrorNone);
    deliverFrame(malformedFrame, sizeof(malformedFrame));
    EXPECT_FALSE(sMacFilterFrameReceived);

    // 2. Unsecured frame check
    constexpr Mac::PanId   kPanId = 0x1234;
    constexpr otExtAddress kSrcExtAddr{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    constexpr otExtAddress kDstExtAddr{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11};
    constexpr uint8_t      kPayload[] = "Hello OpenThread";
    Mac::Address           srcAddr;
    Mac::Address           dstAddr;
    Mac::ExtAddress        srcExt;
    Mac::ExtAddress        dstExt;

    srcExt.Set(kSrcExtAddr.m8);
    dstExt.Set(kDstExtAddr.m8);
    srcAddr.SetExtended(srcExt);
    dstAddr.SetExtended(dstExt);

    uint8_t      frameBuffer[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t      frameLength = 0;
    Mac::TxFrame txFrame{};

    PrepareUnsecuredFrame(txFrame, frameBuffer, frameLength, Mac::Frame::kVersion2006, srcAddr, dstAddr, kPanId, kPanId,
                          kPayload, sizeof(kPayload));

    // Policy = DROP_MALFORMED: valid unsecured frame is passed
    deliverFrame(frameBuffer, frameLength);
    expectDeliveredUnmodified();

    // Policy = DROP_MALFORMED | DROP_UNSECURED: unsecured frame is dropped
    ASSERT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED |
                                                       SPINEL_MAC_FILTER_POLICY_DROP_UNSECURED),
              kErrorNone);
    deliverFrame(frameBuffer, frameLength);
    EXPECT_FALSE(sMacFilterFrameReceived);

    const uint8_t kKeyIdMode2[5] = {0x01, 0x02, 0x03, 0x04, 0x02};
    otMacKey      testKey;

    memset(testKey.m8, 0x55, sizeof(testKey.m8));

    auto prepareSecuredFrame = [&](Mac::Frame::Version aVersion, const Mac::Address &aSrcAddr, Mac::PanId aSrcPanId,
                                   Mac::PanId aDstPanId, Mac::Frame::KeyIdMode aKeyIdMode, const uint8_t *aKeyId) {
        PrepareSecuredFrame(txFrame, frameBuffer, frameLength, aVersion, aSrcAddr, dstAddr, aSrcPanId, aDstPanId,
                            aKeyIdMode, aKeyId, 100, kPayload, sizeof(kPayload), &testKey, &srcExt);
    };

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    const uint8_t kOtherKeyIdMode2[5] = {0x01, 0x02, 0x03, 0x04, 0x03};

    // 3. Key ID matching
    prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode2, kKeyIdMode2);

    // Case 3a: Empty key table without DROP_UNKNOWN_KEY: secured frame is passed through
    deliverFrame(frameBuffer, frameLength);
    expectDeliveredUnmodified();

    // Case 3b: Empty key table with DROP_UNKNOWN_KEY: secured frame is dropped
    ASSERT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_UNSECURED |
                                                       SPINEL_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY),
              kErrorNone);
    deliverFrame(frameBuffer, frameLength);
    EXPECT_FALSE(sMacFilterFrameReceived);

    // Case 3c: Matching Key ID: secured frame is passed through unmodified
    Spinel::RadioSpinel::MacKeyTableEntry keyEntry;
    keyEntry.Clear();
    keyEntry.mKeyIdLength = sizeof(kKeyIdMode2);
    memcpy(keyEntry.mKeyId, kKeyIdMode2, sizeof(kKeyIdMode2));
    keyEntry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);
    ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(&keyEntry, 1), kErrorNone);

    deliverFrame(frameBuffer, frameLength);
    expectDeliveredUnmodified();

    // Case 3d: Different Key Index: secured frame is dropped
    prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode2, kOtherKeyIdMode2);
    deliverFrame(frameBuffer, frameLength);
    EXPECT_FALSE(sMacFilterFrameReceived);

    // Case 3e: Security level not allowed by the entry: secured frame is dropped
    keyEntry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic64);
    ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(&keyEntry, 1), kErrorNone);
    prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode2, kKeyIdMode2);
    deliverFrame(frameBuffer, frameLength);
    EXPECT_FALSE(sMacFilterFrameReceived);
    keyEntry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);

    // 4. Key ID Modes 0 and 1
    {
        const uint8_t                         kKeyIndex[1]      = {0x81};
        const uint8_t                         kOtherKeyIndex[1] = {0x82};
        Spinel::RadioSpinel::MacKeyTableEntry modeEntry;

        modeEntry.Clear();
        modeEntry.mAllowedSecLevels = (1 << Mac::Frame::kSecurityEncMic32);

        // Case 4a: Key ID Mode 1 entry matches frames with the same Key Index only
        modeEntry.mKeyIdLength = 1;
        modeEntry.mKeyId[0]    = kKeyIndex[0];
        ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(&modeEntry, 1), kErrorNone);

        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode1, kKeyIndex);
        deliverFrame(frameBuffer, frameLength);
        expectDeliveredUnmodified();

        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode1, kOtherKeyIndex);
        deliverFrame(frameBuffer, frameLength);
        EXPECT_FALSE(sMacFilterFrameReceived);

        // Case 4b: A Key ID Mode 2 frame does not match a Key ID Mode 1 entry
        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode2, kKeyIdMode2);
        deliverFrame(frameBuffer, frameLength);
        EXPECT_FALSE(sMacFilterFrameReceived);

        // Case 4c: Key ID Mode 0 entry matches Key ID Mode 0 frames
        modeEntry.mKeyIdLength = 0;
        ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(&modeEntry, 1), kErrorNone);

        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode0, nullptr);
        deliverFrame(frameBuffer, frameLength);
        expectDeliveredUnmodified();

        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode1, kKeyIndex);
        deliverFrame(frameBuffer, frameLength);
        EXPECT_FALSE(sMacFilterFrameReceived);

        // Case 4d: Key ID Mode 1 frames using the previous, current, or next key index set by
        // `SPINEL_PROP_RCP_MAC_KEY` are known without a key table entry.
        constexpr uint8_t kCurKeyIndex = 10;
        otMacKeyMaterial  macKey;
        uint8_t           keyIndex[1];

        memset(&macKey, 0, sizeof(macKey));
        memcpy(macKey.mKeyMaterial.mKey.m8, testKey.m8, sizeof(otMacKey));
        ASSERT_EQ(platform.mRadioSpinel.SetMacKeyTable(nullptr, 0), kErrorNone);
        ASSERT_EQ(platform.mRadioSpinel.SetMacKey(Mac::Frame::kKeyIdMode1, kCurKeyIndex, &macKey, &macKey, &macKey),
                  kErrorNone);

        for (uint8_t offset = 0; offset < 3; offset++)
        {
            keyIndex[0] = static_cast<uint8_t>(kCurKeyIndex - 1 + offset);
            prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode1, keyIndex);
            deliverFrame(frameBuffer, frameLength);
            expectDeliveredUnmodified();
        }

        keyIndex[0] = kCurKeyIndex + 2;
        prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode1, keyIndex);
        deliverFrame(frameBuffer, frameLength);
        EXPECT_FALSE(sMacFilterFrameReceived);
    }
#else
    // When MAX_ENTRIES == 0, the key table is disabled.
    // Setting policies requiring the key table must fail.
    EXPECT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY),
              kErrorNotImplemented);

    // Secured frame is passed through unmodified
    prepareSecuredFrame(Mac::Frame::kVersion2006, srcAddr, kPanId, kPanId, Mac::Frame::kKeyIdMode2, kKeyIdMode2);
    ASSERT_EQ(platform.mRadioSpinel.SetMacFilterPolicy(SPINEL_MAC_FILTER_POLICY_DROP_MALFORMED), kErrorNone);
    deliverFrame(frameBuffer, frameLength);
    expectDeliveredUnmodified();
#endif
}

#endif // OPENTHREAD_CONFIG_LINK_EXT_ENABLE
