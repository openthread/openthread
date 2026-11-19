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
#include <openthread/thread_direct.h>
#include <openthread/thread_direct_listener.h>
#include <openthread/platform/radio.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "instance/instance.hpp"
#include "thread_direct/td_commands.hpp"

namespace ot {
namespace ThreadDirect {

/*
 * Unit tests for Thread Direct Wake Listener (`ThreadDirect::Listener`) RX Wake frame processing
 * (`Listener::ProcessRxFrame()`), covering MAC/security header validation, key matching and `SavedPsdu`
 * restoration across single and multiple registered keys, Wake Command payload parsing/validation,
 * `WakeCallback` invocation, and `GetWakeInfo()` / internal `ReceivedWake` state updates.
 */

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

extern "C" {

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 1000000ULL;
}

} // extern "C"

//----------------------------------------------------------------------------------------------------------------------

class UnitTester
{
public:
    typedef Listener::ReceivedWake ReceivedWake;

    static bool CallProcessRxFrame(Listener &aListener, Mac::RxFrame &aRxFrame)
    {
        Mac::RxFrame::ParseInfo frameInfo;

        IgnoreError(frameInfo.ParseFrom(aRxFrame, Mac::Frame::kParseFully));

        return aListener.ProcessRxFrame(frameInfo);
    }

    static const ReceivedWake &GetRxWake(const Listener &aListener) { return aListener.mRxWake; }
};

static constexpr uint8_t           kListenChannel   = 20;
static constexpr Mac::PanId        kDevicePanId     = 0x1234;
static constexpr Mac::ShortAddress kDeviceShort     = 0x2200;
static constexpr Mac::ShortAddress kDeviceAltShort  = 0x3300;
static constexpr Radio::Time64     kDefaultRxTime   = 0x0102030405060708ULL;
static constexpr uint32_t          kDefaultFrameCtr = 0x0000002a;
static constexpr uint8_t           kThreadMacCmdId  = 0x34;
static constexpr uint8_t           kWakeSubCmdId    = 0x01;

struct CallbackTracker : public Clearable<CallbackTracker>
{
    Instance          *mInstance;
    uint32_t           mCount;
    bool               mGotWakeInfoInCallback;
    Listener::WakeInfo mWakeInfoInCallback;
};

static void MakeKeyMaterial(otThreadDirectKeyMaterial &aMaterial, uint8_t aSeed)
{
    ClearAllBytes(aMaterial);

    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_SIZE; i++)
    {
        aMaterial.mKeyMaterial.mKey.m8[i] = static_cast<uint8_t>((aSeed * 17U) + i + 1U);
    }
}

static void MakeKeyId(otThreadDirectKeyId &aKeyId, uint8_t aSeed)
{
    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_ID_SIZE; i++)
    {
        aKeyId.m8[i] = static_cast<uint8_t>((aSeed << 4) + i + 1U);
    }
}

static void MakeExtAddress(Mac::ExtAddress &aExtAddress, uint8_t aSeed)
{
    for (uint16_t i = 0; i < sizeof(Mac::ExtAddress); i++)
    {
        aExtAddress.m8[i] = static_cast<uint8_t>((aSeed * 0x11U) + i);
    }

    aExtAddress.SetLocal(true);
}

struct FrameSpec
{
    uint8_t                   mChannel;
    Radio::Time64             mRxTimestamp;
    Mac::Frame::Type          mType;
    Mac::Frame::Version       mVersion;
    bool                      mAckRequest;
    bool                      mFramePending;
    bool                      mSuppressSequence;
    bool                      mHasDstPanId;
    Mac::PanId                mDstPanId;
    bool                      mHasSrcPanId;
    Mac::PanId                mSrcPanId;
    Mac::Addresses            mAddrs;
    Mac::Frame::SecurityLevel mSecurityLevel;
    Mac::Frame::KeyIdMode     mKeyIdMode;
    KeyId                     mKeyId;
    uint8_t                   mKeyIndex;
    uint32_t                  mFrameCounter;
    KeyMaterial               mEncryptKey;
    bool                      mUseCustomNonceAddr;
    Mac::ExtAddress           mCustomNonceAddr;
    WakeCommand::Info         mWakeCmd;
    bool                      mUseRawPayload;
    const uint8_t            *mRawPayload;
    uint16_t                  mRawPayloadLength;
    bool                      mCorruptMic;
    bool                      mCorruptCiphertext;

    void InitValid(const Mac::ExtAddress &aPeerExtAddr, const KeyMaterial &aKeyMaterial, const KeyId &aKeyId)
    {
        mChannel          = kListenChannel;
        mRxTimestamp      = kDefaultRxTime;
        mType             = Mac::Frame::kTypeMacCmd;
        mVersion          = Mac::Frame::kVersion2015;
        mAckRequest       = false;
        mFramePending     = false;
        mSuppressSequence = false;
        mHasDstPanId      = false;
        mDstPanId         = Mac::kPanIdBroadcast;
        mHasSrcPanId      = false;
        mSrcPanId         = 0;
        mAddrs.mSource.SetExtended(aPeerExtAddr);
        mAddrs.mDestination.SetNone();
        mSecurityLevel      = Mac::Frame::kSecurityEncMic32;
        mKeyIdMode          = Mac::Frame::kKeyIdMode2;
        mKeyId              = aKeyId;
        mKeyIndex           = 1;
        mFrameCounter       = kDefaultFrameCtr;
        mEncryptKey         = aKeyMaterial;
        mUseCustomNonceAddr = false;
        mCustomNonceAddr.Clear();
        mWakeCmd.Clear();
        mUseRawPayload     = false;
        mRawPayload        = nullptr;
        mRawPayloadLength  = 0;
        mCorruptMic        = false;
        mCorruptCiphertext = false;
    }

    void BuildWakeRxFrame(Mac::RxFrame &aRxFrame, uint8_t (&aPsduBuf)[OT_RADIO_FRAME_MAX_SIZE]) const
    {
        static const uint8_t kKeySourceMode3[Mac::Frame::kKeySourceSizeMode3] = {
            0xaa, 0xbb, 0xcc, 0xdd, 0x11, 0x22, 0x33, 0x44,
        };

        Mac::TxFrame                 txFrame;
        Mac::TxFrame::BuildInfo      buildInfo;
        Mac::TxFrame::PayloadBuilder payloadBuilder;
        Mac::TxFrame::ParseInfo      txInfo;
        Mac::ExtAddress              fallbackNonceAddr;
        const Mac::ExtAddress       *nonceAddr;

        memset(aPsduBuf, 0, sizeof(aPsduBuf));
        ClearAllBytes(txFrame);

        txFrame.mPsdu = aPsduBuf;
        txFrame.SetChannel(mChannel);
#if OPENTHREAD_CONFIG_MULTI_RADIO
        txFrame.SetRadioType(Radio::kTypeIeee802154);
#endif

        buildInfo.mType             = mType;
        buildInfo.mVersion          = mVersion;
        buildInfo.mAddrs            = mAddrs;
        buildInfo.mSecurityLevel    = mSecurityLevel;
        buildInfo.mKeyIdMode        = mKeyIdMode;
        buildInfo.mSuppressSequence = (mVersion == Mac::Frame::kVersion2015) ? mSuppressSequence : false;

        if ((mType == Mac::Frame::kTypeMacCmd) && (mVersion != Mac::Frame::kVersion2015))
        {
            buildInfo.mCommandId = kThreadMacCmdId;
        }

        if (mVersion == Mac::Frame::kVersion2015)
        {
            if (mHasDstPanId && !mHasSrcPanId)
            {
                if (!mAddrs.mSource.IsNone() && !mAddrs.mDestination.IsNone() &&
                    !(mAddrs.mSource.IsExtended() && mAddrs.mDestination.IsExtended()))
                {
                    // IEEE 802.15.4-2015 Rows 12, 13, 14 (both addresses present, at least one Short, Dst PAN ID
                    // only): `PrepareHeadersIn()` sets PAN ID Compression = 1 when `mSource == mDestination`.
                    buildInfo.mPanIds.SetBothSourceDestination(mDstPanId);
                }
                else
                {
                    buildInfo.mPanIds.SetDestination(mDstPanId);
                }
            }
            else
            {
                if (mHasDstPanId)
                {
                    buildInfo.mPanIds.SetDestination(mDstPanId);
                }

                if (mHasSrcPanId)
                {
                    buildInfo.mPanIds.SetSource(mSrcPanId);
                }
            }
        }
        else
        {
            if (mHasDstPanId)
            {
                buildInfo.mPanIds.SetBothSourceDestination(mDstPanId);
            }
        }

        txFrame.PrepareHeaders(buildInfo, payloadBuilder);
        txFrame.SetAckRequest(mAckRequest);
        txFrame.SetFramePending(mFramePending);

        if (mUseRawPayload)
        {
            if (mRawPayloadLength > 0)
            {
                SuccessOrQuit(payloadBuilder.AppendBytes(mRawPayload, mRawPayloadLength));
            }
        }
        else
        {
            SuccessOrQuit(mWakeCmd.PrepareIn(payloadBuilder));
        }

        txFrame.FinishPayload(payloadBuilder);

        SuccessOrQuit(txInfo.ParseFrom(txFrame, Mac::Frame::kParseFully));

        if (mVersion == Mac::Frame::kVersion2015)
        {
            VerifyOrQuit(txInfo.mPanIds.IsDestinationPresent() == mHasDstPanId);
            VerifyOrQuit(txInfo.mPanIds.IsSourcePresent() == mHasSrcPanId);
        }

        if (mSecurityLevel != Mac::Frame::kSecurityNone)
        {
            txInfo.WriteFrameCounter(mFrameCounter);

            switch (mKeyIdMode)
            {
            case Mac::Frame::kKeyIdMode0:
                break;

            case Mac::Frame::kKeyIdMode1:
                txInfo.WriteKeyIndex(mKeyIndex);
                break;

            case Mac::Frame::kKeyIdMode2:
                txInfo.WriteKeySource(mKeyId.GetBytes());
                txInfo.WriteKeyIndex(mKeyId.GetBytes()[Mac::Frame::kKeySourceSizeMode2]);
                break;

            case Mac::Frame::kKeyIdMode3:
                txInfo.WriteKeySource(kKeySourceMode3);
                txInfo.WriteKeyIndex(mKeyIndex);
                break;
            }

            if (txInfo.mMicSize >= Crypto::AesCcm::kMinTagLength)
            {
                MakeExtAddress(fallbackNonceAddr, 0xee);

                if (mUseCustomNonceAddr)
                {
                    nonceAddr = &mCustomNonceAddr;
                }
                else if (mAddrs.mSource.IsExtended())
                {
                    nonceAddr = &mAddrs.mSource.GetExtended();
                }
                else
                {
                    nonceAddr = &fallbackNonceAddr;
                }

                txFrame.SetAesKey(mEncryptKey);
                txInfo.ProcessTransmitAesCcm(*nonceAddr);
            }
        }

        if (mCorruptCiphertext)
        {
            VerifyOrQuit(txInfo.mPayload.GetLength() > 0);
            aPsduBuf[txInfo.mHeader.GetLength()] ^= 0x80;
        }

        if (mCorruptMic)
        {
            VerifyOrQuit(txInfo.mMicSize > 0);
            aPsduBuf[txFrame.GetLength() - txFrame.GetFcsSize() - 1] ^= 0x01;
        }

        ClearAllBytes(aRxFrame);
        aRxFrame.mPsdu                    = aPsduBuf;
        aRxFrame.mLength                  = txFrame.GetLength();
        aRxFrame.mChannel                 = mChannel;
        aRxFrame.mInfo.mRxInfo.mTimestamp = mRxTimestamp;
#if OPENTHREAD_CONFIG_MULTI_RADIO
        aRxFrame.SetRadioType(Radio::kTypeIeee802154);
#endif
    }
};

static void HandleWakeCallback(void *aContext)
{
    CallbackTracker *tracker = static_cast<CallbackTracker *>(aContext);

    VerifyOrQuit(tracker != nullptr);
    VerifyOrQuit(tracker->mInstance != nullptr);

    tracker->mCount++;
    tracker->mWakeInfoInCallback.mKey = nullptr;
    tracker->mGotWakeInfoInCallback =
        (otThreadDirectListenerGetWakeInfo(tracker->mInstance, &tracker->mWakeInfoInCallback) == OT_ERROR_NONE);
}

static void SetupSubMac(Instance &aInstance, const Mac::ExtAddress &aDeviceExtAddr)
{
    Mac::SubMac &subMac = aInstance.Get<Mac::SubMac>();

    SuccessOrQuit(subMac.Enable());
    SuccessOrQuit(subMac.Sleep());
    subMac.SetExtAddress(aDeviceExtAddr);
    subMac.SetShortAddress(kDeviceShort);
    subMac.SetAlternateShortAddress(kDeviceAltShort);
    subMac.SetPanId(kDevicePanId);
}

static void VerifyWakeInfoMatches(const Listener::WakeInfo &aActual, const Listener::WakeInfo &aExpected)
{
    VerifyOrQuit(aActual.mRxTimestamp == aExpected.mRxTimestamp);
    VerifyOrQuit(AsCoreType(&aActual.mExtAddress) == AsCoreType(&aExpected.mExtAddress));
    VerifyOrQuit(aActual.mKey == aExpected.mKey);
    VerifyOrQuit(aActual.mLinkChannel == aExpected.mLinkChannel);
    VerifyOrQuit(aActual.mPanId == aExpected.mPanId);
    VerifyOrQuit(aActual.mIsUnicast == aExpected.mIsUnicast);
}

static void VerifyRejectedPreAuth(Listener                 &aListener,
                                  const FrameSpec          &aSpec,
                                  CallbackTracker          &aTracker,
                                  const Listener::WakeInfo *aExpectedPriorWakeInfo = nullptr)
{
    Mac::RxFrame       rxFrame;
    uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t            origPsdu[OT_RADIO_FRAME_MAX_SIZE];
    uint16_t           origLength;
    uint32_t           prevCount = aTracker.mCount;
    Listener::WakeInfo wakeInfo;

    aSpec.BuildWakeRxFrame(rxFrame, psduBuf);

    origLength = rxFrame.GetLength();
    memcpy(origPsdu, psduBuf, origLength);

    VerifyOrQuit(!UnitTester::CallProcessRxFrame(aListener, rxFrame));
    VerifyOrQuit(aTracker.mCount == prevCount);

    // Verify PSDU bytes and frame length were left completely untouched (or restored).
    VerifyOrQuit(rxFrame.GetLength() == origLength);
    VerifyOrQuit(memcmp(rxFrame.GetPsdu(), origPsdu, origLength) == 0);

    if (aExpectedPriorWakeInfo != nullptr)
    {
        SuccessOrQuit(aListener.GetWakeInfo(wakeInfo));
        VerifyWakeInfoMatches(wakeInfo, *aExpectedPriorWakeInfo);
    }
    else if (aListener.IsRunning())
    {
        VerifyOrQuit(aListener.GetWakeInfo(wakeInfo) == kErrorNotFound);
    }
}

//------------------------------------------------------------------------------------------------------------------
// Test 1: All valid Wake addressing modes, PAN ID variants, and WakeCommand parameter combinations

void TestValidWakeAddressingAndParams(void)
{
    struct ParamCase
    {
        bool          mHasRendezvousTime;
        uint8_t       mRendezvousTime;
        bool          mHasConnParams;
        uint8_t       mRetryInterval;
        uint8_t       mRetryCount;
        bool          mHasChannel;
        uint8_t       mChannel;
        Radio::Time64 mRxTimestamp;
        uint32_t      mFrameCounter;
    };

    static const ParamCase kParamCases[] = {
        // All optional WakeCommand fields omitted (verify defaults).
        {false, 0, false, 0, 0, false, 0, 0ULL, 0},
        // Only Rendezvous Time present (boundary 0, mid, boundary 255).
        {true, 0, false, 0, 0, false, 0, 123456ULL, 1},
        {true, 42, false, 0, 0, false, 0, 0x0000000100000005ULL, 0x100},
        {true, 255, false, 0, 0, false, 0, NumericLimits<Radio::Time64>::kMax, 0xabcdef01U},
        // Only Connection Parameters present (min [1, 0], mid [6, 11], max [15, 15]).
        {false, 0, true, 1, 0, false, 0, 500000ULL, 2},
        {false, 0, true, 6, 11, false, 0, 600000ULL, 3},
        {false, 0, true, 15, 15, false, 0, 700000ULL, 4},
        // Only Channel present (kChannelMin, kChannelMax, and same as listen channel).
        {false, 0, false, 0, 0, true, Radio::kChannelMin, 800000ULL, 5},
        {false, 0, false, 0, 0, true, Radio::kChannelMax, 900000ULL, 6},
        {false, 0, false, 0, 0, true, kListenChannel, 950000ULL, 7},
        // Combinations of two and all three valid parameter groups, up to max valid frame counter (`kMax - 1`).
        {true, 18, true, 3, 9, false, 0, 0xdeadbeefcafebabeULL, 1000},
        {true, 99, false, 0, 0, true, Radio::kChannelMin + 3, 0x1122334455667788ULL, 2000},
        {false, 0, true, 12, 4, true, Radio::kChannelMax - 2, 0x8877665544332211ULL, 3000},
        {true, 200, true, 15, 15, true, Radio::kChannelMin + 5, 0xfedcba9876543210ULL,
         NumericLimits<uint32_t>::kMax - 1},
    };

    Instance                 *instance;
    Listener                 *listener;
    Mac::SubMac              *subMac;
    Mac::ExtAddress           deviceExtAddr;
    Mac::ExtAddress           peerExtAddr1;
    Mac::ExtAddress           peerExtAddr2;
    otThreadDirectKeyMaterial keyMat;
    otThreadDirectKeyId       keyId;
    otThreadDirectKey        *regKey;
    Listener::Config          config;
    CallbackTracker           tracker;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    listener = &instance->Get<Listener>();
    subMac   = &instance->Get<Mac::SubMac>();

    MakeExtAddress(deviceExtAddr, 0x01);
    MakeExtAddress(peerExtAddr1, 0x02);
    MakeExtAddress(peerExtAddr2, 0x03);
    SetupSubMac(*instance, deviceExtAddr);

    MakeKeyMaterial(keyMat, 1);
    MakeKeyId(keyId, 1);
    regKey = otThreadDirectRegisterKey(instance, &keyMat, &keyId);
    VerifyOrQuit(regKey != nullptr);

    tracker.Clear();
    tracker.mInstance = instance;

    config.Clear();
    config.mChannel             = kListenChannel;
    config.mWakeCallback        = HandleWakeCallback;
    config.mWakeCallbackContext = &tracker;
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));

    // 1. Test all parameter combinations in `kParamCases` and verify both `WakeInfo` and internal `mRxWake`.
    for (const ParamCase &tc : kParamCases)
    {
        FrameSpec          spec;
        Mac::RxFrame       rxFrame;
        uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
        Listener::WakeInfo wakeInfo;
        Listener::WakeInfo expectedInfo;
        uint32_t           prevCount = tracker.mCount;

        spec.InitValid(peerExtAddr1, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mRxTimestamp                = tc.mRxTimestamp;
        spec.mFrameCounter               = tc.mFrameCounter;
        spec.mWakeCmd.mHasRendezvousTime = tc.mHasRendezvousTime;
        spec.mWakeCmd.mRendezvousTime    = tc.mRendezvousTime;
        spec.mWakeCmd.mHasConnParams     = tc.mHasConnParams;
        spec.mWakeCmd.mRetryInterval     = tc.mRetryInterval;
        spec.mWakeCmd.mRetryCount        = tc.mRetryCount;
        spec.mWakeCmd.mHasChannel        = tc.mHasChannel;
        spec.mWakeCmd.mChannel           = tc.mChannel;

        spec.BuildWakeRxFrame(rxFrame, psduBuf);

        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
        VerifyOrQuit(tracker.mCount == prevCount + 1);
        VerifyOrQuit(tracker.mGotWakeInfoInCallback);

        expectedInfo.mRxTimestamp = tc.mRxTimestamp;
        expectedInfo.mExtAddress  = peerExtAddr1;
        expectedInfo.mKey         = regKey;
        expectedInfo.mLinkChannel = tc.mHasChannel ? tc.mChannel : kListenChannel;
        expectedInfo.mPanId       = OT_PANID_BROADCAST;
        expectedInfo.mIsUnicast   = false;

        VerifyWakeInfoMatches(tracker.mWakeInfoInCallback, expectedInfo);

        memset(&wakeInfo, 0xff, sizeof(wakeInfo));
        SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
        VerifyWakeInfoMatches(wakeInfo, expectedInfo);

        // Also verify internal `Listener::ReceivedWake` fields (`mRxWake`).
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mIsValid);
        VerifyOrQuit(!UnitTester::GetRxWake(*listener).mIsUnicast);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mKey == AsCoreTypePtr(regKey));
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRxTimestamp == tc.mRxTimestamp);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mExtAddress == peerExtAddr1);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mFrameCounter == tc.mFrameCounter);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mPanId == Mac::kPanIdBroadcast);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRendezvousTime ==
                     (tc.mHasRendezvousTime ? tc.mRendezvousTime : 0));
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryInterval ==
                     (tc.mHasConnParams ? tc.mRetryInterval : WakeCommand::kDefaultRetryInterval));
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryCount ==
                     (tc.mHasConnParams ? tc.mRetryCount : WakeCommand::kDefaultRetryCount));
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mLinkChannel == (tc.mHasChannel ? tc.mChannel : kListenChannel));
    }

    // 2. Test all valid Destination Address and Destination PAN ID combinations (`mPanId` and `mIsUnicast`).
    {
        struct AddrPanCase
        {
            enum DstMode : uint8_t
            {
                kDstNone,
                kDstBroadcast,
                kDstSelfExt,
                kDstSelfShort,
                kDstSelfAltShort,
            };

            Mac::PanId mConfiguredDevicePanId;
            DstMode    mDstMode;
            bool       mHasDstPanId;
            Mac::PanId mDstPanId;
            Mac::PanId mExpectedWakePanId;
            bool       mExpectedIsUnicast;
            bool       mFramePending;
            bool       mSuppressSeq;
        };

        static const AddrPanCase kAddrPanCases[] = {
            // Destination omitted, PAN ID omitted -> `mPanId = OT_PANID_BROADCAST`, `mIsUnicast = false`.
            {kDevicePanId, AddrPanCase::kDstNone, false, 0, OT_PANID_BROADCAST, false, false, false},
            {kDevicePanId, AddrPanCase::kDstNone, false, 0, OT_PANID_BROADCAST, false, true, true},
            // Broadcast short destination + matching PAN ID -> `mPanId = kDevicePanId`, `mIsUnicast = false`.
            {kDevicePanId, AddrPanCase::kDstBroadcast, true, kDevicePanId, kDevicePanId, false, false, false},
            // Broadcast short destination + broadcast PAN ID -> `mPanId = OT_PANID_BROADCAST`, `mIsUnicast =
            // false`.
            {kDevicePanId, AddrPanCase::kDstBroadcast, true, Mac::kPanIdBroadcast, OT_PANID_BROADCAST, false, false,
             true},
            // Unicast extended destination (self), PAN ID omitted -> `mPanId = OT_PANID_BROADCAST`, `mIsUnicast =
            // true`.
            {kDevicePanId, AddrPanCase::kDstSelfExt, false, 0, OT_PANID_BROADCAST, true, false, false},
            // Unicast extended destination (self), matching PAN ID -> `mPanId = kDevicePanId`, `mIsUnicast = true`.
            {kDevicePanId, AddrPanCase::kDstSelfExt, true, kDevicePanId, kDevicePanId, true, true, true},
            // Unicast extended destination (self), broadcast PAN ID -> `mPanId = OT_PANID_BROADCAST`, `mIsUnicast =
            // true`.
            {kDevicePanId, AddrPanCase::kDstSelfExt, true, Mac::kPanIdBroadcast, OT_PANID_BROADCAST, true, false,
             false},
            // Unicast short destination (primary short addr) -> `mIsUnicast = true`.
            {kDevicePanId, AddrPanCase::kDstSelfShort, true, kDevicePanId, kDevicePanId, true, false, false},
            // Unicast short destination (alternate short addr) -> `mIsUnicast = true`.
            {kDevicePanId, AddrPanCase::kDstSelfAltShort, true, Mac::kPanIdBroadcast, OT_PANID_BROADCAST, true, false,
             false},
            // Device PAN ID is `0xffff` (unconfigured/broadcast) and Wake carries specific PAN ID `0xabcd`.
            {Mac::kPanIdBroadcast, AddrPanCase::kDstSelfExt, true, 0xabcd, 0xabcd, true, false, false},
            {Mac::kPanIdBroadcast, AddrPanCase::kDstBroadcast, true, 0xabcd, 0xabcd, false, false, false},
        };

        for (const AddrPanCase &ac : kAddrPanCases)
        {
            FrameSpec          spec;
            Mac::RxFrame       rxFrame;
            uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
            Listener::WakeInfo wakeInfo;
            Listener::WakeInfo expectedInfo;
            uint32_t           prevCount = tracker.mCount;

            subMac->SetPanId(ac.mConfiguredDevicePanId);

            spec.InitValid(peerExtAddr2, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mHasDstPanId      = ac.mHasDstPanId;
            spec.mDstPanId         = ac.mDstPanId;
            spec.mFramePending     = ac.mFramePending;
            spec.mSuppressSequence = ac.mSuppressSeq;

            switch (ac.mDstMode)
            {
            case AddrPanCase::kDstNone:
                spec.mAddrs.mDestination.SetNone();
                break;
            case AddrPanCase::kDstBroadcast:
                spec.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
                break;
            case AddrPanCase::kDstSelfExt:
                spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
                break;
            case AddrPanCase::kDstSelfShort:
                spec.mAddrs.mDestination.SetShort(kDeviceShort);
                break;
            case AddrPanCase::kDstSelfAltShort:
                spec.mAddrs.mDestination.SetShort(kDeviceAltShort);
                break;
            }

            spec.BuildWakeRxFrame(rxFrame, psduBuf);

            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == prevCount + 1);

            expectedInfo.mRxTimestamp = kDefaultRxTime;
            expectedInfo.mExtAddress  = peerExtAddr2;
            expectedInfo.mKey         = regKey;
            expectedInfo.mLinkChannel = kListenChannel;
            expectedInfo.mPanId       = ac.mExpectedWakePanId;
            expectedInfo.mIsUnicast   = ac.mExpectedIsUnicast;

            VerifyWakeInfoMatches(tracker.mWakeInfoInCallback, expectedInfo);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyWakeInfoMatches(wakeInfo, expectedInfo);
        }

        subMac->SetPanId(kDevicePanId);
    }

    // 3. Forward-compatible Wake payload (explicit zero flags byte, or unknown reserved flag bits + trailing bytes)
    //    and `mWakeCallback == nullptr` configuration.
    {
        static const uint8_t kUnknownFlagsPayload[] = {
            kThreadMacCmdId, kWakeSubCmdId, static_cast<uint8_t>((1 << 7) | (1 << 5) | 0x0f), 0x25, 15, 0xde, 0xad,
        };

        FrameSpec          spec;
        Mac::RxFrame       rxFrame;
        uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
        Listener::WakeInfo wakeInfo;

        otThreadDirectListenerStop(instance);

        config.Clear();
        config.mChannel      = kListenChannel;
        config.mWakeCallback = nullptr;
        SuccessOrQuit(otThreadDirectListenerStart(instance, &config));

        spec.InitValid(peerExtAddr1, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mUseRawPayload    = true;
        spec.mRawPayload       = kUnknownFlagsPayload;
        spec.mRawPayloadLength = sizeof(kUnknownFlagsPayload);

        spec.BuildWakeRxFrame(rxFrame, psduBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));

        SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
        VerifyOrQuit(wakeInfo.mLinkChannel == 15);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRendezvousTime == 0x25);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryInterval == WakeCommand::kDefaultRetryInterval);
        VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryCount == WakeCommand::kDefaultRetryCount);
    }

    testFreeInstance(instance);

    printf("TestValidWakeAddressingAndParams() passed\n");
}

//------------------------------------------------------------------------------------------------------------------
// Test 2: All MAC header, addressing, PAN ID, and security header rejection cases (pre-authentication)

void TestHeaderAndSecurityFailures(void)
{
    static const Mac::Frame::SecurityLevel kInvalidSecLevels[] = {
        Mac::Frame::kSecurityNone,      Mac::Frame::kSecurityMic32, Mac::Frame::kSecurityMic64,
        Mac::Frame::kSecurityMic128,    Mac::Frame::kSecurityEnc,   Mac::Frame::kSecurityEncMic64,
        Mac::Frame::kSecurityEncMic128,
    };

    Instance                 *instance;
    Listener                 *listener;
    Mac::SubMac              *subMac;
    Mac::ExtAddress           deviceExtAddr;
    Mac::ExtAddress           peerExtAddr;
    Mac::ExtAddress           otherExtAddr;
    otThreadDirectKeyMaterial keyMat;
    otThreadDirectKeyId       keyId;
    otThreadDirectKey        *regKey;
    Listener::Config          config;
    CallbackTracker           tracker;
    Listener::WakeInfo        priorWakeInfo;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    listener = &instance->Get<Listener>();
    subMac   = &instance->Get<Mac::SubMac>();

    MakeExtAddress(deviceExtAddr, 0x10);
    MakeExtAddress(peerExtAddr, 0x20);
    MakeExtAddress(otherExtAddr, 0x30);
    SetupSubMac(*instance, deviceExtAddr);

    MakeKeyMaterial(keyMat, 2);
    MakeKeyId(keyId, 2);
    regKey = otThreadDirectRegisterKey(instance, &keyMat, &keyId);
    VerifyOrQuit(regKey != nullptr);

    tracker.Clear();
    tracker.mInstance = instance;

    // 1. Listener stopped: `ProcessRxFrame()` must return `false` even for a completely valid Wake frame.
    {
        FrameSpec spec;

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        VerifyRejectedPreAuth(*listener, spec, tracker);
    }

    // Start listener and seed `mRxWake` with a valid Wake frame so we also verify that none of the rejected
    // frames below clobber or clear the existing valid `WakeInfo`.
    config.Clear();
    config.mChannel             = kListenChannel;
    config.mWakeCallback        = HandleWakeCallback;
    config.mWakeCallbackContext = &tracker;
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));

    {
        FrameSpec    seedSpec;
        Mac::RxFrame seedFrame;
        uint8_t      seedBuf[OT_RADIO_FRAME_MAX_SIZE];

        seedSpec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        seedSpec.mRxTimestamp         = 0x777788889999aaaaULL;
        seedSpec.mWakeCmd.mHasChannel = true;
        seedSpec.mWakeCmd.mChannel    = 15;

        seedSpec.BuildWakeRxFrame(seedFrame, seedBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, seedFrame));
        VerifyOrQuit(tracker.mCount == 1);
        SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &priorWakeInfo));
    }

    // 2. Unparsed / truncated MAC frames (`!aFrameInfo.mParsedFully`).
    {
        static const uint16_t kBadLengths[] = {0, 1, 2, 8};

        FrameSpec    validSpec;
        Mac::RxFrame validFrame;
        uint8_t      validBuf[OT_RADIO_FRAME_MAX_SIZE];

        validSpec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        validSpec.BuildWakeRxFrame(validFrame, validBuf);

        // Test lengths 0, 1, 2, and truncated inside MHR / security header (8 bytes).
        for (uint16_t badLen : kBadLengths)
        {
            Mac::RxFrame       truncFrame;
            uint8_t            truncBuf[OT_RADIO_FRAME_MAX_SIZE];
            uint8_t            origBuf[OT_RADIO_FRAME_MAX_SIZE];
            Listener::WakeInfo wakeInfo;

            memcpy(truncBuf, validBuf, sizeof(truncBuf));
            memcpy(origBuf, truncBuf, sizeof(origBuf));

            ClearAllBytes(truncFrame);
            truncFrame.mPsdu    = truncBuf;
            truncFrame.mLength  = badLen;
            truncFrame.mChannel = kListenChannel;
#if OPENTHREAD_CONFIG_MULTI_RADIO
            truncFrame.SetRadioType(Radio::kTypeIeee802154);
#endif

            VerifyOrQuit(!UnitTester::CallProcessRxFrame(*listener, truncFrame));
            VerifyOrQuit(tracker.mCount == 1);
            VerifyOrQuit(memcmp(truncBuf, origBuf, sizeof(origBuf)) == 0);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyWakeInfoMatches(wakeInfo, priorWakeInfo);
        }
    }

    // 3. Channel mismatch (`GetChannel() != mTimedRx.GetChannel()`).
    {
        static const uint8_t kWrongChannels[] = {
            static_cast<uint8_t>(kListenChannel - 1),
            static_cast<uint8_t>(kListenChannel + 1),
            Radio::kChannelMin,
            Radio::kChannelMax,
        };

        for (uint8_t wrongChannel : kWrongChannels)
        {
            FrameSpec spec;

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mChannel = wrongChannel;
            VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
        }
    }

    // 4. Wrong MAC frame type (`kTypeBeacon`, `kTypeData`, `kTypeAck`).
    {
        static const Mac::Frame::Type kBadTypes[] = {
            Mac::Frame::kTypeBeacon,
            Mac::Frame::kTypeData,
            Mac::Frame::kTypeAck,
        };

        for (Mac::Frame::Type badType : kBadTypes)
        {
            FrameSpec spec;

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mType = badType;
            VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
        }
    }

    // 5. Wrong MAC frame version (`kVersion2003`, `kVersion2006`).
    {
        static const Mac::Frame::Version kBadVersions[] = {
            Mac::Frame::kVersion2003,
            Mac::Frame::kVersion2006,
        };

        for (Mac::Frame::Version badVersion : kBadVersions)
        {
            FrameSpec spec;

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mVersion     = badVersion;
            spec.mHasDstPanId = true;
            spec.mDstPanId    = kDevicePanId;
            spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
            VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
        }
    }

    // 6. Ack Request (`AR`) bit set (`mIsAckRequest == true`) across omitted, broadcast, and unicast destinations.
    {
        FrameSpec spec;

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAckRequest = true;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        spec.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    // 7. Source PAN ID present (`aFrameInfo.mPanIds.IsSourcePresent()`).
    {
        FrameSpec spec;

        // IEEE 802.15.4-2015 Row 5: DstAddr = None, SrcAddr = Extended, SrcPanId = Present.
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mHasSrcPanId = true;
        spec.mSrcPanId    = kDevicePanId;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        // IEEE 802.15.4-2015 Row 10: DstAddr = Short, SrcAddr = Extended, both DstPanId and SrcPanId present.
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId;
        spec.mHasSrcPanId = true;
        spec.mSrcPanId    = kDevicePanId + 1;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    // 8. Destination PAN ID mismatch (`!Mac::MatchPanIds(...)`).
    {
        FrameSpec spec;

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId + 1;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        spec.mAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    // 9. Invalid Source Address (omitted, short address, or device's own extended address).
    {
        FrameSpec spec;

        // Source address omitted (`None`).
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mSource.SetNone();
        spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        // Source address is Short instead of Extended.
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mSource.SetShort(0x4455);
        spec.mAddrs.mDestination.SetExtended(deviceExtAddr);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        // Source extended address equals the listener device's own extended address.
        spec.InitValid(deviceExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    // 10. Destination Address mismatch (another peer's extended/short address, or `kShortAddrInvalid`).
    {
        FrameSpec spec;

        // Different extended address.
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mDestination.SetExtended(otherExtAddr);
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        // Different short address.
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mDestination.SetShort(0x7788);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        // Device short addresses set to `kShortAddrInvalid` (`0xfffe`) and frame addressed to `0xfffe`.
        subMac->SetShortAddress(Mac::kShortAddrInvalid);
        subMac->SetAlternateShortAddress(Mac::kShortAddrInvalid);
        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mAddrs.mDestination.SetShort(Mac::kShortAddrInvalid);
        spec.mHasDstPanId = true;
        spec.mDstPanId    = kDevicePanId;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);

        subMac->SetShortAddress(kDeviceShort);
        subMac->SetAlternateShortAddress(kDeviceAltShort);
    }

    // 11. Invalid Security Levels (everything other than `kSecurityEncMic32`, including `kSecurityNone`).
    for (Mac::Frame::SecurityLevel badSecLevel : kInvalidSecLevels)
    {
        FrameSpec spec;

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mSecurityLevel = badSecLevel;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    // 12. Invalid Key ID Modes (`kKeyIdMode0`, `kKeyIdMode1`, `kKeyIdMode3`, where `keyId.ReadFrom()` fails).
    {
        static const Mac::Frame::KeyIdMode kBadKeyIdModes[] = {
            Mac::Frame::kKeyIdMode0,
            Mac::Frame::kKeyIdMode1,
            Mac::Frame::kKeyIdMode3,
        };

        for (Mac::Frame::KeyIdMode badKeyIdMode : kBadKeyIdModes)
        {
            FrameSpec spec;

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mKeyIdMode = badKeyIdMode;
            VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
        }
    }

    // 13. Frame Counter equal to `NumericLimits<uint32_t>::kMax` (`0xffffffff`).
    {
        FrameSpec spec;

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mFrameCounter = NumericLimits<uint32_t>::kMax;
        VerifyRejectedPreAuth(*listener, spec, tracker, &priorWakeInfo);
    }

    testFreeInstance(instance);

    printf("TestHeaderAndSecurityFailures() passed\n");
}

//------------------------------------------------------------------------------------------------------------------
// Test 3: Key matching (0 keys, 1 key, multiple keys, shared KeyId) and `SavedPsdu` restoration behavior

void TestKeyMatchingAndSavedPsduRestoration(void)
{
    Instance                 *instance;
    Listener                 *listener;
    Mac::ExtAddress           deviceExtAddr;
    Mac::ExtAddress           peerExtAddr;
    Mac::ExtAddress           wrongNonceAddr;
    otThreadDirectKeyMaterial matShared[3];
    otThreadDirectKeyMaterial matOther[2];
    otThreadDirectKeyMaterial matUnreg;
    otThreadDirectKeyId       idShared;
    otThreadDirectKeyId       idOther[2];
    otThreadDirectKey        *keyShared[3];
    otThreadDirectKey        *keyOther[2];
    Listener::Config          config;
    CallbackTracker           tracker;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    listener = &instance->Get<Listener>();

    MakeExtAddress(deviceExtAddr, 0x41);
    MakeExtAddress(peerExtAddr, 0x42);
    MakeExtAddress(wrongNonceAddr, 0x43);
    SetupSubMac(*instance, deviceExtAddr);

    for (uint8_t i = 0; i < 3; i++)
    {
        MakeKeyMaterial(matShared[i], static_cast<uint8_t>(10 + i));
    }

    for (uint8_t i = 0; i < 2; i++)
    {
        MakeKeyMaterial(matOther[i], static_cast<uint8_t>(20 + i));
        MakeKeyId(idOther[i], static_cast<uint8_t>(20 + i));
    }

    MakeKeyMaterial(matUnreg, 99);
    MakeKeyId(idShared, 10);

    tracker.Clear();
    tracker.mInstance = instance;

    config.Clear();
    config.mChannel             = kListenChannel;
    config.mWakeCallback        = HandleWakeCallback;
    config.mWakeCallbackContext = &tracker;
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));

    // 1. Zero registered keys: `ProcessRxFrame()` returns `false` and leaves PSDU untouched.
    {
        FrameSpec spec;

        VerifyOrQuit(instance->Get<KeyManager>().GetKeyList().IsEmpty());

        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        spec.mWakeCmd.mHasRendezvousTime = true;
        spec.mWakeCmd.mRendezvousTime    = 50;
        spec.mWakeCmd.mHasConnParams     = true;
        spec.mWakeCmd.mRetryInterval     = 4;
        spec.mWakeCmd.mRetryCount        = 9;
        spec.mWakeCmd.mHasChannel        = true;
        spec.mWakeCmd.mChannel           = 16;

        VerifyRejectedPreAuth(*listener, spec, tracker);
    }

    // 2. Keys registered, none matching the frame's `KeyId` (even when one shares the same `KeyMaterial`!).
    {
        otThreadDirectKey *sameMatDiffIdKey;
        FrameSpec          spec;

        keyOther[0] = otThreadDirectRegisterKey(instance, &matOther[0], &idOther[0]);
        VerifyOrQuit(keyOther[0] != nullptr);

        // Register `matShared[0]` under `idOther[1]` (different `KeyId` from `idShared`).
        sameMatDiffIdKey = otThreadDirectRegisterKey(instance, &matShared[0], &idOther[1]);
        VerifyOrQuit(sameMatDiffIdKey != nullptr);

        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        VerifyRejectedPreAuth(*listener, spec, tracker);

        otThreadDirectUnregisterAllKeys(instance);
    }

    // 3. Single registered key with matching `KeyId`:
    //    - First prove that a failed `ProcessReceiveAesCcm()` with the wrong key actually mutates `mPsdu` in-place.
    //    - Then verify `Listener::ProcessRxFrame()` restores `mPsdu` byte-for-byte via `SavedPsdu` on failure
    //      (wrong key material, corrupted MIC, corrupted ciphertext, or wrong nonce address).
    //    - Then verify matching key material succeeds.
    {
        FrameSpec    spec;
        Mac::RxFrame rxFrame;
        uint8_t      psduBuf[OT_RADIO_FRAME_MAX_SIZE];
        uint8_t      origPsdu[OT_RADIO_FRAME_MAX_SIZE];
        uint16_t     origLength;

        keyShared[0] = otThreadDirectRegisterKey(instance, &matShared[0], &idShared);
        VerifyOrQuit(keyShared[0] != nullptr);

        // Build frame encrypted with `matUnreg` (wrong key material) but tagged with `idShared`.
        spec.InitValid(peerExtAddr, AsCoreType(&matUnreg), AsCoreType(&idShared));
        spec.mWakeCmd.mHasRendezvousTime = true;
        spec.mWakeCmd.mRendezvousTime    = 77;
        spec.mWakeCmd.mHasConnParams     = true;
        spec.mWakeCmd.mRetryInterval     = 5;
        spec.mWakeCmd.mRetryCount        = 10;
        spec.mWakeCmd.mHasChannel        = true;
        spec.mWakeCmd.mChannel           = 24;

        // Confirm direct `ProcessReceiveAesCcm` with `matShared[0]` mutates `psduBuf` in-place when MIC fails.
        spec.BuildWakeRxFrame(rxFrame, psduBuf);
        origLength = rxFrame.GetLength();
        memcpy(origPsdu, psduBuf, origLength);
        {
            Mac::RxFrame::ParseInfo directInfo;

            SuccessOrQuit(directInfo.ParseFrom(rxFrame, Mac::Frame::kParseFully));
            VerifyOrQuit(directInfo.ProcessReceiveAesCcm(peerExtAddr, AsCoreType(&matShared[0])) == kErrorSecurity);
            VerifyOrQuit(memcmp(psduBuf, origPsdu, origLength) != 0);
        }

        // Now run through `Listener::ProcessRxFrame()` and verify `SavedPsdu` restores `psduBuf` completely!
        VerifyRejectedPreAuth(*listener, spec, tracker);

        // Corrupted MIC tag with matching `(matShared[0], idShared)` -> fails MIC and restores PSDU.
        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        spec.mWakeCmd.mHasChannel = true;
        spec.mWakeCmd.mChannel    = 22;
        spec.mCorruptMic          = true;
        VerifyRejectedPreAuth(*listener, spec, tracker);

        // Corrupted ciphertext byte with matching `(matShared[0], idShared)` -> fails MIC and restores PSDU.
        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        spec.mWakeCmd.mHasChannel = true;
        spec.mWakeCmd.mChannel    = 22;
        spec.mCorruptCiphertext   = true;
        VerifyRejectedPreAuth(*listener, spec, tracker);

        // Encrypted with wrong source extended address in AES-CCM nonce -> fails MIC and restores PSDU.
        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        spec.mUseCustomNonceAddr = true;
        spec.mCustomNonceAddr    = wrongNonceAddr;
        VerifyRejectedPreAuth(*listener, spec, tracker);

        // Valid frame with matching `(matShared[0], idShared)` -> authenticates and invokes callback.
        spec.InitValid(peerExtAddr, AsCoreType(&matShared[0]), AsCoreType(&idShared));
        spec.mWakeCmd.mHasChannel = true;
        spec.mWakeCmd.mChannel    = 22;
        spec.BuildWakeRxFrame(rxFrame, psduBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
        VerifyOrQuit(tracker.mCount == 1);
        VerifyOrQuit(tracker.mWakeInfoInCallback.mKey == keyShared[0]);
        VerifyOrQuit(tracker.mWakeInfoInCallback.mLinkChannel == 22);

        otThreadDirectUnregisterAllKeys(instance);
    }

    // 4. Multiple registered keys, including 3 keys sharing `idShared` interleaved with 2 keys using other IDs:
    //    - Verify authentication succeeds whether the matching key is 1st, 2nd, or 3rd among `idShared` entries
    //      (proving `RestorePsdu()` restores the ciphertext after each failed trial decryption so subsequent keys
    //      can decrypt and verify MIC!).
    //    - Verify that when a frame with `idShared` is encrypted with `matUnreg` (so all 3 `idShared` keys fail
    //      in-place decryption), `ProcessRxFrame()` returns `false`, leaves `rxFrame` 100% restored, AND that very
    //      same `rxFrame` buffer subsequently authenticates once `matUnreg` is registered!
    {
        keyOther[0]  = otThreadDirectRegisterKey(instance, &matOther[0], &idOther[0]);
        keyShared[0] = otThreadDirectRegisterKey(instance, &matShared[0], &idShared);
        keyOther[1]  = otThreadDirectRegisterKey(instance, &matOther[1], &idOther[1]);
        keyShared[1] = otThreadDirectRegisterKey(instance, &matShared[1], &idShared);
        keyShared[2] = otThreadDirectRegisterKey(instance, &matShared[2], &idShared);

        VerifyOrQuit(keyOther[0] != nullptr);
        VerifyOrQuit(keyShared[0] != nullptr);
        VerifyOrQuit(keyOther[1] != nullptr);
        VerifyOrQuit(keyShared[1] != nullptr);
        VerifyOrQuit(keyShared[2] != nullptr);

        // Also verify matching a unique-ID key (`keyOther[0]` and `keyOther[1]`).
        for (uint8_t idx = 0; idx < 2; idx++)
        {
            FrameSpec          spec;
            Mac::RxFrame       rxFrame;
            uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
            Listener::WakeInfo wakeInfo;
            uint32_t           prevCount = tracker.mCount;

            spec.InitValid(peerExtAddr, AsCoreType(&matOther[idx]), AsCoreType(&idOther[idx]));
            spec.mWakeCmd.mHasRendezvousTime = true;
            spec.mWakeCmd.mRendezvousTime    = static_cast<uint8_t>(10 + idx);

            spec.BuildWakeRxFrame(rxFrame, psduBuf);
            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == prevCount + 1);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyOrQuit(wakeInfo.mKey == keyOther[idx]);
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mRendezvousTime == static_cast<uint8_t>(10 + idx));
        }

        // Test every one of the 3 shared-`KeyId` keys (`keyShared[0]`, `keyShared[1]`, `keyShared[2]`).
        for (uint8_t idx = 0; idx < 3; idx++)
        {
            FrameSpec          spec;
            Mac::RxFrame       rxFrame;
            uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
            Listener::WakeInfo wakeInfo;
            uint32_t           prevCount    = tracker.mCount;
            uint8_t            expectedChan = static_cast<uint8_t>(Radio::kChannelMin + idx);

            spec.InitValid(peerExtAddr, AsCoreType(&matShared[idx]), AsCoreType(&idShared));
            spec.mRxTimestamp                = kDefaultRxTime + idx;
            spec.mFrameCounter               = 500U + idx;
            spec.mWakeCmd.mHasRendezvousTime = true;
            spec.mWakeCmd.mRendezvousTime    = static_cast<uint8_t>(30 + idx);
            spec.mWakeCmd.mHasConnParams     = true;
            spec.mWakeCmd.mRetryInterval     = static_cast<uint8_t>(2 + idx);
            spec.mWakeCmd.mRetryCount        = static_cast<uint8_t>(6 + idx);
            spec.mWakeCmd.mHasChannel        = true;
            spec.mWakeCmd.mChannel           = expectedChan;

            spec.BuildWakeRxFrame(rxFrame, psduBuf);
            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == prevCount + 1);

            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyOrQuit(wakeInfo.mKey == keyShared[idx]);
            VerifyOrQuit(wakeInfo.mRxTimestamp == kDefaultRxTime + idx);
            VerifyOrQuit(wakeInfo.mLinkChannel == expectedChan);
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mFrameCounter == 500U + idx);
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mRendezvousTime == static_cast<uint8_t>(30 + idx));
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryInterval == static_cast<uint8_t>(2 + idx));
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mRetryCount == static_cast<uint8_t>(6 + idx));
        }

        // Now build a single `rxFrame` with `(matUnreg, idShared)`. All 3 `keyShared` entries try decryption and
        // fail. Verify `rxFrame` is restored byte-for-byte, then register `matUnreg` and pass the SAME `rxFrame`
        // (without rebuilding!) into `ProcessRxFrame()` to confirm it still decrypts and authenticates!
        {
            FrameSpec          spec;
            Mac::RxFrame       rxFrame;
            uint8_t            psduBuf[OT_RADIO_FRAME_MAX_SIZE];
            uint8_t            origPsdu[OT_RADIO_FRAME_MAX_SIZE];
            uint16_t           origLength;
            otThreadDirectKey *newlyRegKey;
            Listener::WakeInfo wakeInfo;
            uint32_t           prevCount = tracker.mCount;

            spec.InitValid(peerExtAddr, AsCoreType(&matUnreg), AsCoreType(&idShared));
            spec.mWakeCmd.mHasRendezvousTime = true;
            spec.mWakeCmd.mRendezvousTime    = 0xab;
            spec.mWakeCmd.mHasChannel        = true;
            spec.mWakeCmd.mChannel           = 19;

            spec.BuildWakeRxFrame(rxFrame, psduBuf);
            origLength = rxFrame.GetLength();
            memcpy(origPsdu, psduBuf, origLength);

            // All 3 `idShared` keys fail decryption; PSDU must be restored to `origPsdu`.
            VerifyOrQuit(!UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == prevCount);
            VerifyOrQuit(rxFrame.GetLength() == origLength);
            VerifyOrQuit(memcmp(rxFrame.GetPsdu(), origPsdu, origLength) == 0);

            // Register `(matUnreg, idShared)` and reuse the exact same `rxFrame` buffer!
            newlyRegKey = otThreadDirectRegisterKey(instance, &matUnreg, &idShared);
            VerifyOrQuit(newlyRegKey != nullptr);

            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == prevCount + 1);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyOrQuit(wakeInfo.mKey == newlyRegKey);
            VerifyOrQuit(wakeInfo.mLinkChannel == 19);
            VerifyOrQuit(UnitTester::GetRxWake(*listener).mRendezvousTime == 0xab);
        }
    }

    testFreeInstance(instance);

    printf("TestKeyMatchingAndSavedPsduRestoration() passed\n");
}

//------------------------------------------------------------------------------------------------------------------
// Test 4: Post-authentication Wake payload failures (`ProcessRxFrame()` returns `true`, callback NOT invoked)

void TestPostAuthPayloadFailures(void)
{
    static const uint8_t kOneBytePayload[]         = {kThreadMacCmdId};
    static const uint8_t kWrongMacCmdPayload[]     = {0x04, kWakeSubCmdId};
    static const uint8_t kWrongSubCmd0Payload[]    = {kThreadMacCmdId, 0x00};
    static const uint8_t kWrongSubCmd2Payload[]    = {kThreadMacCmdId, 0x02};
    static const uint8_t kTruncRendezvousPayload[] = {kThreadMacCmdId, kWakeSubCmdId, (1 << 7)};
    static const uint8_t kTruncConnParamsPayload[] = {kThreadMacCmdId, kWakeSubCmdId, (1 << 6)};
    static const uint8_t kTruncChannelPayload[]    = {kThreadMacCmdId, kWakeSubCmdId, (1 << 5)};
    static const uint8_t kTruncActionPayload[]     = {kThreadMacCmdId, kWakeSubCmdId, (1 << 4)};
    static const uint8_t kTruncMultiPayload[]      = {kThreadMacCmdId, kWakeSubCmdId, (1 << 7) | (1 << 5), 0x10};

    struct RawSlice
    {
        const uint8_t *mBytes;
        uint16_t       mLength;
    };

    static const RawSlice kBadRawPayloads[] = {
        {kOneBytePayload, sizeof(kOneBytePayload)},
        {kWrongMacCmdPayload, sizeof(kWrongMacCmdPayload)},
        {kWrongSubCmd0Payload, sizeof(kWrongSubCmd0Payload)},
        {kWrongSubCmd2Payload, sizeof(kWrongSubCmd2Payload)},
        {kTruncRendezvousPayload, sizeof(kTruncRendezvousPayload)},
        {kTruncConnParamsPayload, sizeof(kTruncConnParamsPayload)},
        {kTruncChannelPayload, sizeof(kTruncChannelPayload)},
        {kTruncActionPayload, sizeof(kTruncActionPayload)},
        {kTruncMultiPayload, sizeof(kTruncMultiPayload)},
    };

    Instance                 *instance;
    Listener                 *listener;
    Mac::ExtAddress           deviceExtAddr;
    Mac::ExtAddress           peerExtAddr;
    otThreadDirectKeyMaterial keyMat;
    otThreadDirectKeyId       keyId;
    otThreadDirectKey        *regKey;
    Listener::Config          config;
    CallbackTracker           tracker;
    Listener::WakeInfo        wakeInfo;
    Listener::WakeInfo        seededWakeInfo;

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    listener = &instance->Get<Listener>();

    MakeExtAddress(deviceExtAddr, 0x51);
    MakeExtAddress(peerExtAddr, 0x52);
    SetupSubMac(*instance, deviceExtAddr);

    MakeKeyMaterial(keyMat, 5);
    MakeKeyId(keyId, 5);
    regKey = otThreadDirectRegisterKey(instance, &keyMat, &keyId);
    VerifyOrQuit(regKey != nullptr);

    tracker.Clear();
    tracker.mInstance = instance;

    config.Clear();
    config.mChannel             = kListenChannel;
    config.mWakeCallback        = HandleWakeCallback;
    config.mWakeCallbackContext = &tracker;
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));

    // 1. Before any valid Wake is received (`GetWakeInfo()` == `OT_ERROR_NOT_FOUND`), verify all post-auth bad
    //    payloads return `true` (consumed) without invoking `mWakeCallback` or populating `WakeInfo`.
    for (const RawSlice &raw : kBadRawPayloads)
    {
        FrameSpec    spec;
        Mac::RxFrame rxFrame;
        uint8_t      psduBuf[OT_RADIO_FRAME_MAX_SIZE];

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mUseRawPayload    = true;
        spec.mRawPayload       = raw.mBytes;
        spec.mRawPayloadLength = raw.mLength;

        spec.BuildWakeRxFrame(rxFrame, psduBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
        VerifyOrQuit(tracker.mCount == 0);
        VerifyOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo) == OT_ERROR_NOT_FOUND);
    }

    // 2. Seed a valid Wake frame so `mRxWake` is valid, then verify that invalid `WakeCommand::Info` values
    //    (`!cmdInfo.IsValid()`) and `Action Select` (`cmdInfo.mHasActionSelect`) still return `true` (consumed)
    //    while leaving the prior valid `mRxWake` intact and not invoking `mWakeCallback`.
    {
        FrameSpec    seedSpec;
        Mac::RxFrame seedFrame;
        uint8_t      seedBuf[OT_RADIO_FRAME_MAX_SIZE];

        seedSpec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        seedSpec.mRxTimestamp         = 0x4444555566667777ULL;
        seedSpec.mWakeCmd.mHasChannel = true;
        seedSpec.mWakeCmd.mChannel    = 25;

        seedSpec.BuildWakeRxFrame(seedFrame, seedBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, seedFrame));
        VerifyOrQuit(tracker.mCount == 1);
        SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &seededWakeInfo));
    }

    // Invalid `mRetryInterval == 0` when `mHasConnParams` is set.
    {
        FrameSpec    spec;
        Mac::RxFrame rxFrame;
        uint8_t      psduBuf[OT_RADIO_FRAME_MAX_SIZE];

        spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
        spec.mWakeCmd.mHasConnParams = true;
        spec.mWakeCmd.mRetryInterval = 0;
        spec.mWakeCmd.mRetryCount    = 8;

        spec.BuildWakeRxFrame(rxFrame, psduBuf);
        VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
        VerifyOrQuit(tracker.mCount == 1);
        SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
        VerifyWakeInfoMatches(wakeInfo, seededWakeInfo);
    }

    // Invalid `mChannel` values when `mHasChannel` is set (`0`, `kChannelMin - 1`, `kChannelMax + 1`, `255`).
    {
        static const uint8_t kBadChannels[] = {
            0,
            static_cast<uint8_t>(Radio::kChannelMin - 1),
            static_cast<uint8_t>(Radio::kChannelMax + 1),
            255,
        };

        for (uint8_t badChannel : kBadChannels)
        {
            FrameSpec    spec;
            Mac::RxFrame rxFrame;
            uint8_t      psduBuf[OT_RADIO_FRAME_MAX_SIZE];

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mWakeCmd.mHasChannel = true;
            spec.mWakeCmd.mChannel    = badChannel;

            spec.BuildWakeRxFrame(rxFrame, psduBuf);
            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == 1);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyWakeInfoMatches(wakeInfo, seededWakeInfo);
        }
    }

    // `Action Select` flag present (`mHasActionSelect == true` with `0x00`, `0x01`, `0xff`).
    {
        static const uint8_t kActionValues[] = {0x00, 0x01, 0xff};

        for (uint8_t actionVal : kActionValues)
        {
            FrameSpec    spec;
            Mac::RxFrame rxFrame;
            uint8_t      psduBuf[OT_RADIO_FRAME_MAX_SIZE];

            spec.InitValid(peerExtAddr, AsCoreType(&keyMat), AsCoreType(&keyId));
            spec.mWakeCmd.mHasActionSelect = true;
            spec.mWakeCmd.mActionSelect    = actionVal;

            spec.BuildWakeRxFrame(rxFrame, psduBuf);
            VerifyOrQuit(UnitTester::CallProcessRxFrame(*listener, rxFrame));
            VerifyOrQuit(tracker.mCount == 1);
            SuccessOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo));
            VerifyWakeInfoMatches(wakeInfo, seededWakeInfo);
        }
    }

    testFreeInstance(instance);

    printf("TestPostAuthPayloadFailures() passed\n");
}

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

} // namespace ThreadDirect
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    ot::ThreadDirect::TestValidWakeAddressingAndParams();
    ot::ThreadDirect::TestHeaderAndSecurityFailures();
    ot::ThreadDirect::TestKeyMatchingAndSavedPsduRestoration();
    ot::ThreadDirect::TestPostAuthPayloadFailures();

    printf("All tests passed\n");
#else
    printf("Thread Direct Wake Listener is not enabled (or platform key refs enabled)\n");
#endif

    return 0;
}
