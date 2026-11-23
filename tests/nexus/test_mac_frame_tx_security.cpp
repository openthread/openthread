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

#include <stdio.h>

#include "mac_frame.h"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t          kMacFrameCounter     = 1000;
static constexpr uint32_t          kPrevMacFrameCounter = 500;
static constexpr uint32_t          kFrameCounter        = 0x12345678;
static constexpr Mac::PanId        kPanId               = 0xface;
static constexpr Mac::ShortAddress kSrcShortAddr        = 0x1000;
static constexpr Mac::ShortAddress kDstShortAddr        = 0x2000;

enum KeyType : uint8_t
{
    kPrevKey,
    kCurrKey,
    kNextKey,
};

static void InitRadioContext(otRadioContext &aContext, uint8_t aKeyId)
{
    ClearAllBytes(aContext);

    aContext.mKeyId               = aKeyId;
    aContext.mMacFrameCounter     = kMacFrameCounter;
    aContext.mPrevMacFrameCounter = kPrevMacFrameCounter;
}

static const otMacKeyMaterial *GetKey(const otRadioContext &aContext, KeyType aKeyType)
{
    const otMacKeyMaterial *key = nullptr;

    switch (aKeyType)
    {
    case kPrevKey:
        key = &aContext.mPrevKey;
        break;
    case kCurrKey:
        key = &aContext.mCurrKey;
        break;
    case kNextKey:
        key = &aContext.mNextKey;
        break;
    }

    return key;
}

static void PrepareSecuredFrame(Radio::Frame &aFrame, Mac::Frame::Version aVersion, uint8_t aKeyId)
{
    Mac::TxFrame::BuildInfo buildInfo;

    buildInfo.mType          = Mac::Frame::kTypeData;
    buildInfo.mVersion       = aVersion;
    buildInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    buildInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;
    buildInfo.mAddrs.mSource.SetShort(kSrcShortAddr);
    buildInfo.mAddrs.mDestination.SetShort(kDstShortAddr);
    buildInfo.mPanIds.SetBothSourceDestination(kPanId);

    aFrame.PrepareHeadersWithEmptyPayload(buildInfo);
    otMacFrameSetKeyId(&aFrame, aKeyId);
}

static void PrepareEnhAck(Radio::Frame &aAckFrame, uint8_t aKeyId)
{
    Radio::Frame rxFrame;

    PrepareSecuredFrame(rxFrame, Mac::Frame::kVersion2015, aKeyId);
    SuccessOrQuit(otMacFrameGenerateEnhAck(&rxFrame, /* aIsFramePending */ false, nullptr, 0, &aAckFrame));
}

static void TestNewFrame(void)
{
    static constexpr uint8_t kKeyId = 5;

    otRadioContext context;
    Radio::Frame   frame;

    Log("New frame uses current key, key ID and next frame counter");

    InitRadioContext(context, kKeyId);
    PrepareSecuredFrame(frame, Mac::Frame::kVersion2006, kKeyId + 1);
    frame.SetIsHeaderUpdated(false);

    SuccessOrQuit(otMacFrameProcessTransmitSecurity(&frame, &context));

    VerifyOrQuit(frame.mInfo.mTxInfo.mAesKey == &context.mCurrKey);
    VerifyOrQuit(frame.IsSecurityProcessed());
    VerifyOrQuit(frame.IsHeaderUpdated());
    VerifyOrQuit(otMacFrameGetKeyId(&frame) == kKeyId);
    VerifyOrQuit(otMacFrameGetFrameCounter(&frame) == kMacFrameCounter);
    VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter + 1);
}

static void TestHeaderUpdatedFrame(uint8_t aCurKeyId, uint8_t aFrameKeyId, KeyType aExpectedKey)
{
    otRadioContext context;
    Radio::Frame   frame;

    Log("Header-updated frame: radio key ID %u, frame key ID %u", aCurKeyId, aFrameKeyId);

    InitRadioContext(context, aCurKeyId);
    PrepareSecuredFrame(frame, Mac::Frame::kVersion2006, aFrameKeyId);
    otMacFrameSetFrameCounter(&frame, kFrameCounter);
    frame.SetIsHeaderUpdated(true);

    SuccessOrQuit(otMacFrameProcessTransmitSecurity(&frame, &context));

    VerifyOrQuit(frame.mInfo.mTxInfo.mAesKey == GetKey(context, aExpectedKey));
    VerifyOrQuit(frame.IsSecurityProcessed());
    VerifyOrQuit(otMacFrameGetKeyId(&frame) == aFrameKeyId);
    VerifyOrQuit(otMacFrameGetFrameCounter(&frame) == kFrameCounter);
    VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter);
    VerifyOrQuit(context.mPrevMacFrameCounter == kPrevMacFrameCounter);
}

static void TestEnhAck(uint8_t aCurKeyId, uint8_t aAckKeyId, KeyType aExpectedKey)
{
    otRadioContext context;
    Radio::Frame   ackFrame;
    uint32_t       frameCounter = 0;

    Log("Enh-ACK: radio key ID %u, ACK key ID %u", aCurKeyId, aAckKeyId);

    InitRadioContext(context, aCurKeyId);
    PrepareEnhAck(ackFrame, aAckKeyId);

    SuccessOrQuit(otMacFrameProcessTransmitSecurity(&ackFrame, &context));

    switch (aExpectedKey)
    {
    case kPrevKey:
        frameCounter = kPrevMacFrameCounter;
        VerifyOrQuit(context.mPrevMacFrameCounter == kPrevMacFrameCounter + 1);
        VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter);
        break;
    case kCurrKey:
        frameCounter = kMacFrameCounter;
        VerifyOrQuit(context.mPrevMacFrameCounter == kPrevMacFrameCounter);
        VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter + 1);
        break;
    case kNextKey:
        frameCounter = 0;
        VerifyOrQuit(context.mPrevMacFrameCounter == kPrevMacFrameCounter);
        VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter);
        break;
    }

    VerifyOrQuit(ackFrame.mInfo.mTxInfo.mAesKey == GetKey(context, aExpectedKey));
    VerifyOrQuit(ackFrame.IsSecurityProcessed());
    VerifyOrQuit(otMacFrameGetKeyId(&ackFrame) == aAckKeyId);
    VerifyOrQuit(otMacFrameGetFrameCounter(&ackFrame) == frameCounter);
}

static void TestEnhAckError(uint8_t aCurKeyId, uint8_t aAckKeyId, Error aExpectedError)
{
    otRadioContext context;
    Radio::Frame   ackFrame;

    Log("Enh-ACK error: radio key ID %u, ACK key ID %u", aCurKeyId, aAckKeyId);

    InitRadioContext(context, aCurKeyId);
    PrepareEnhAck(ackFrame, aAckKeyId);

    VerifyOrQuit(otMacFrameProcessTransmitSecurity(&ackFrame, &context) == aExpectedError);

    VerifyOrQuit(ackFrame.mInfo.mTxInfo.mAesKey == nullptr);
    VerifyOrQuit(!ackFrame.IsSecurityProcessed());
    VerifyOrQuit(context.mMacFrameCounter == kMacFrameCounter);
    VerifyOrQuit(context.mPrevMacFrameCounter == kPrevMacFrameCounter);
}

void TestMacFrameTxSecurity(void)
{
    Core nexus;

    TestNewFrame();

    TestHeaderUpdatedFrame(5, 5, kCurrKey);
    TestHeaderUpdatedFrame(5, 4, kPrevKey);
    TestHeaderUpdatedFrame(5, 6, kNextKey);
    TestHeaderUpdatedFrame(Mac::kMaxKeyIndex, Mac::kMinKeyIndex, kNextKey);
    TestHeaderUpdatedFrame(Mac::kMinKeyIndex, Mac::kMaxKeyIndex, kPrevKey);

    // A key ID matching no key, or no key set on the radio (key ID 0),
    // falls back to the current key.
    TestHeaderUpdatedFrame(5, 9, kCurrKey);
    TestHeaderUpdatedFrame(0, 1, kCurrKey);

    TestEnhAck(5, 5, kCurrKey);
    TestEnhAck(5, 4, kPrevKey);
    TestEnhAck(5, 6, kNextKey);
    TestEnhAck(Mac::kMaxKeyIndex, Mac::kMinKeyIndex, kNextKey);
    TestEnhAck(Mac::kMinKeyIndex, Mac::kMaxKeyIndex, kPrevKey);

    TestEnhAckError(5, 9, kErrorSecurity);
    TestEnhAckError(5, 0, kErrorFailed);
    TestEnhAckError(0, 1, kErrorSecurity);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMacFrameTxSecurity();
    printf("All tests passed\n");
    return 0;
}
