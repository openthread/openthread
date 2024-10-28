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
#include "gmock/gmock.h"
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
        Mac::TxFrame::Info frameInfo;

        frameInfo.mType    = Mac::Frame::kTypeData;
        frameInfo.mVersion = Mac::Frame::kVersion2006;
        frameInfo.mAddrs.mSource.SetShort(kSrcAddr);
        frameInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        frameInfo.mPanIds.SetSource(kSrcPanId);
        frameInfo.mPanIds.SetDestination(kDstPanId);
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        frameInfo.PrepareHeadersIn(txFrame);
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
        Mac::TxFrame::Info frameInfo;

        frameInfo.mType    = Mac::Frame::kTypeData;
        frameInfo.mVersion = Mac::Frame::kVersion2006;
        frameInfo.mAddrs.mSource.SetShort(kSrcAddr);
        frameInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        frameInfo.mPanIds.SetSource(kSrcPanId);
        frameInfo.mPanIds.SetDestination(kDstPanId);
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        frameInfo.PrepareHeadersIn(txFrame);
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
        Mac::TxFrame::Info frameInfo;

        frameInfo.mType    = Mac::Frame::kTypeData;
        frameInfo.mVersion = Mac::Frame::kVersion2006;
        frameInfo.mAddrs.mSource.SetShort(kSrcAddr);
        frameInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        frameInfo.mPanIds.SetSource(kSrcPanId);
        frameInfo.mPanIds.SetDestination(kDstPanId);
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        frameInfo.PrepareHeadersIn(txFrame);
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
        Mac::TxFrame::Info frameInfo;

        frameInfo.mType    = Mac::Frame::kTypeData;
        frameInfo.mVersion = Mac::Frame::kVersion2006;
        frameInfo.mAddrs.mSource.SetShort(kSrcAddr);
        frameInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        frameInfo.mPanIds.SetSource(kSrcPanId);
        frameInfo.mPanIds.SetDestination(kDstPanId);
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        frameInfo.PrepareHeadersIn(txFrame);
    }

    txFrame.mInfo.mTxInfo.mCsmaCaEnabled = true;
    txFrame.mChannel                     = 11;

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
        Mac::TxFrame::Info frameInfo;

        frameInfo.mType    = Mac::Frame::kTypeData;
        frameInfo.mVersion = Mac::Frame::kVersion2006;
        frameInfo.mAddrs.mSource.SetShort(kSrcAddr);
        frameInfo.mAddrs.mDestination.SetExtended(kDstAddr);
        frameInfo.mPanIds.SetSource(kSrcPanId);
        frameInfo.mPanIds.SetDestination(kDstPanId);
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        frameInfo.PrepareHeadersIn(txFrame);
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
