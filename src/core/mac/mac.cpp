/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

/**
 * @file
 *   This file implements the subset of IEEE 802.15.4 primitives required for Thread.
 */

#define WPP_NAME "mac.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <string.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <crypto/aes_ccm.hpp>
#include <crypto/sha256.hpp>
#include <mac/mac.hpp>
#include <mac/mac_frame.hpp>
#include <platform/random.h>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <openthread-instance.h>

using Thread::Encoding::BigEndian::HostSwap64;

namespace Thread {
namespace Mac {

static const uint8_t sMode2Key[] =
{
    0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f, 0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66
};

static const otExtAddress sMode2ExtAddress =
{
    { 0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12 },
};

static const uint8_t sExtendedPanidInit[] = {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe};
static const char sNetworkNameInit[] = "OpenThread";

#ifdef _WIN32
const uint32_t kMinBackoffSum = kMinBackoff + (kUnitBackoffPeriod *kPhyUsPerSymbol * (1 << kMinBE)) / 1000;
const uint32_t kMaxBackoffSum = kMinBackoff + (kUnitBackoffPeriod *kPhyUsPerSymbol * (1 << kMaxBE)) / 1000;
static_assert(kMinBackoffSum > 0, "The min backoff value should be greater than zero!");
#endif

void otLinkReceiveDone(otInstance *aInstance, RadioPacket *aFrame, ThreadError aError);
void otLinkTransmitDone(otInstance *aInstance, RadioPacket *aPacket, bool aRxPending, ThreadError aError);
void otLinkEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);

void Mac::StartCsmaBackoff(void)
{
    if (RadioSupportsRetriesAndCsmaBackoff())
    {
        // If the radio supports the retry and back off logic, immediately schedule the send,
        // and the radio will take care of everything.
        mBackoffTimer.Start(0);
    }
    else
    {
        uint32_t backoffExponent = kMinBE + mTransmitAttempts + mCsmaAttempts;
        uint32_t backoff;

        if (backoffExponent > kMaxBE)
        {
            backoffExponent = kMaxBE;
        }

        backoff = kMinBackoff + (kUnitBackoffPeriod * kPhyUsPerSymbol * (1 << backoffExponent)) / 1000;
        backoff = (otPlatRandomGet() % backoff);

        mBackoffTimer.Start(backoff);
    }
}

Mac::Mac(ThreadNetif &aThreadNetif):
    mMacTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mac::HandleMacTimer, this),
    mBackoffTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mac::HandleBeginTransmit, this),
    mReceiveTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mac::HandleReceiveTimer, this),
    mKeyManager(aThreadNetif.GetKeyManager()),
    mMle(aThreadNetif.GetMle()),
    mNetif(aThreadNetif),
    mEnergyScanSampleRssiTask(aThreadNetif.GetIp6().mTaskletScheduler, &Mac::HandleEnergyScanSampleRssi, this),
    mWhitelist(),
    mBlacklist()
{
    mState = kStateIdle;

    mRxOnWhenIdle = false;
    mCsmaAttempts = 0;
    mTransmitAttempts = 0;
    mTransmitBeacon = false;

    mPendingScanRequest = kScanTypeNone;
    mScanChannel = kPhyMinChannel;
    mScanChannels = 0xff;
    mScanDuration = 0;
    mScanContext = NULL;
    mActiveScanHandler = NULL;
    mEnergyScanHandler = NULL;
    mEnergyScanCurrentMaxRssi = kInvalidRssiValue;

    mSendHead = NULL;
    mSendTail = NULL;
    mReceiveHead = NULL;
    mReceiveTail = NULL;
    mChannel = OPENTHREAD_CONFIG_DEFAULT_CHANNEL;
    mMaxTransmitPower = OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER;
    mPanId = kPanIdBroadcast;
    mShortAddress = kShortAddrInvalid;

    for (size_t i = 0; i < sizeof(mExtAddress); i++)
    {
        mExtAddress.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    mExtAddress.SetGroup(false);
    mExtAddress.SetLocal(true);

    memset(&mCounters, 0, sizeof(otMacCounters));

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);
    SetPanId(mPanId);
    SetExtAddress(mExtAddress);
    SetShortAddress(kShortAddrInvalid);

    mBeaconSequence = static_cast<uint8_t>(otPlatRandomGet());
    mDataSequence = static_cast<uint8_t>(otPlatRandomGet());

    mPcapCallback = NULL;
    mPcapCallbackContext = NULL;

    otPlatRadioEnable(mNetif.GetInstance());
    mTxFrame = static_cast<Frame *>(otPlatRadioGetTransmitBuffer(mNetif.GetInstance()));
}

ThreadError Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    ThreadError error;

    SuccessOrExit(error = Scan(kScanTypeActive, aScanChannels, aScanDuration, aContext));
    mActiveScanHandler = aHandler;

exit:
    return error;
}

ThreadError Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    ThreadError error;

    SuccessOrExit(error = Scan(kScanTypeEnergy, aScanChannels, aScanDuration, aContext));
    mEnergyScanHandler = aHandler;

exit:
    return error;
}

ThreadError Mac::Scan(ScanType aScanType, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit((mState != kStateActiveScan) && (mState != kStateEnergyScan) && (mPendingScanRequest == kScanTypeNone),
                 error = kThreadError_Busy);

    mScanContext = aContext;
    mScanChannels = (aScanChannels == 0) ? static_cast<uint32_t>(kScanChannelsAll) : aScanChannels;
    mScanDuration = (aScanDuration == 0) ? static_cast<uint16_t>(kScanDurationDefault) : aScanDuration;

    mScanChannel = kPhyMinChannel;
    mScanChannels >>= kPhyMinChannel;

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;
    }

    if (mState == kStateIdle)
    {
        if (aScanType == kScanTypeActive)
        {
            mState = kStateActiveScan;
            StartCsmaBackoff();
        }
        else if (aScanType == kScanTypeEnergy)
        {
            StartEnergyScan();
        }
    }
    else
    {
        mPendingScanRequest = aScanType;
    }

exit:
    return error;
}

bool Mac::IsActiveScanInProgress(void)
{
    return (mState == kStateActiveScan) || (mPendingScanRequest == kScanTypeActive);
}

bool Mac::IsEnergyScanInProgress(void)
{
    return (mState == kStateEnergyScan) || (mPendingScanRequest == kScanTypeEnergy);
}

void Mac::StartEnergyScan(void)
{
    mState = kStateEnergyScan;

    if (!(otPlatRadioGetCaps(mNetif.GetInstance()) & kRadioCapsEnergyScan))
    {
        mEnergyScanCurrentMaxRssi = kInvalidRssiValue;
        mMacTimer.Start(mScanDuration);
        mEnergyScanSampleRssiTask.Post();
        NextOperation();
    }
    else
    {
        ThreadError error = otLinkRawEnergyScan(mNetif.GetInstance(), mScanChannel, mScanDuration,
                                                otLinkEnergyScanDone);

        if (error != kThreadError_None)
        {
            // Cancel scan
            mEnergyScanHandler(mScanContext, NULL);
            ScheduleNextTransmission();
        }
    }
}

void otLinkEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    aInstance->mThreadNetif.GetMac().EnergyScanDone(aEnergyScanMaxRssi);
}

void Mac::EnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    // Trigger a energy scan handler callback if necessary
    if (aEnergyScanMaxRssi != kInvalidRssiValue)
    {
        otEnergyScanResult result;

        result.mChannel = mScanChannel;
        result.mMaxRssi = aEnergyScanMaxRssi;
        mEnergyScanHandler(mScanContext, &result);
    }

    // Update to the next scan channel
    do
    {
        mScanChannels >>= 1;
        mScanChannel++;

        // If we have scanned all the channels, then fire the final callback
        // and start the next transmission task
        if (mScanChannels == 0 || mScanChannel > kPhyMaxChannel)
        {
            otLinkRawReceive(mNetif.GetInstance(), mChannel, otLinkReceiveDone);
            mEnergyScanHandler(mScanContext, NULL);
            ScheduleNextTransmission();
            ExitNow();
        }
    }
    while ((mScanChannels & 1) == 0);

    // Start scanning the next channel
    StartEnergyScan();

exit:
    return;
}

void Mac::HandleEnergyScanSampleRssi(void *aContext)
{
    static_cast<Mac *>(aContext)->HandleEnergyScanSampleRssi();
}

void Mac::HandleEnergyScanSampleRssi(void)
{
    int8_t rssi;

    VerifyOrExit(mState == kStateEnergyScan, ;);

    rssi = otPlatRadioGetRssi(mNetif.GetInstance());

    if (rssi != kInvalidRssiValue)
    {
        if ((mEnergyScanCurrentMaxRssi == kInvalidRssiValue) || (rssi > mEnergyScanCurrentMaxRssi))
        {
            mEnergyScanCurrentMaxRssi = rssi;
        }
    }

    mEnergyScanSampleRssiTask.Post();

exit:
    return;
}

ThreadError Mac::RegisterReceiver(Receiver &aReceiver)
{
    assert(mReceiveTail != &aReceiver && aReceiver.mNext == NULL);

    if (mReceiveTail == NULL)
    {
        mReceiveHead = &aReceiver;
        mReceiveTail = &aReceiver;
    }
    else
    {
        mReceiveTail->mNext = &aReceiver;
        mReceiveTail = &aReceiver;
    }

    return kThreadError_None;
}

bool Mac::GetRxOnWhenIdle(void) const
{
    return mRxOnWhenIdle;
}

void Mac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mRxOnWhenIdle = aRxOnWhenIdle;

    if (mState == kStateIdle)
    {
        NextOperation();
    }
}

const ExtAddress *Mac::GetExtAddress(void) const
{
    return &mExtAddress;
}

void Mac::SetExtAddress(const ExtAddress &aExtAddress)
{
    uint8_t buf[sizeof(aExtAddress)];

    otLogFuncEntry();

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = aExtAddress.m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(mNetif.GetInstance(), buf);
    mExtAddress = aExtAddress;

    otLogFuncExit();
}

void Mac::GetHashMacAddress(ExtAddress *aHashMacAddress)
{
    Crypto::Sha256 sha256;
    uint8_t buf[Crypto::Sha256::kHashSize];

    otLogFuncEntry();

    otPlatRadioGetIeeeEui64(mNetif.GetInstance(), buf);
    sha256.Start();
    sha256.Update(buf, OT_EXT_ADDRESS_SIZE);
    sha256.Finish(buf);

    memcpy(aHashMacAddress->m8, buf, OT_EXT_ADDRESS_SIZE);
    aHashMacAddress->SetLocal(true);

    otLogFuncExitMsg("%llX", HostSwap64(*reinterpret_cast<uint64_t *>(aHashMacAddress)));
}

ShortAddress Mac::GetShortAddress(void) const
{
    return mShortAddress;
}

ThreadError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    otLogFuncEntryMsg("%d", aShortAddress);
    mShortAddress = aShortAddress;
    otPlatRadioSetShortAddress(mNetif.GetInstance(), aShortAddress);
    otLogFuncExit();
    return kThreadError_None;
}

uint8_t Mac::GetChannel(void) const
{
    return mChannel;
}

ThreadError Mac::SetChannel(uint8_t aChannel)
{
    otLogFuncEntryMsg("%d", aChannel);
    mChannel = aChannel;

    if (mState == kStateIdle)
    {
        NextOperation();
    }

    otLogFuncExit();
    return kThreadError_None;
}

int8_t Mac::GetMaxTransmitPower(void) const
{
    return mMaxTransmitPower;
}

void Mac::SetMaxTransmitPower(int8_t aPower)
{
    mMaxTransmitPower = aPower;
}

const char *Mac::GetNetworkName(void) const
{
    return mNetworkName.m8;
}

ThreadError Mac::SetNetworkName(const char *aNetworkName)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntryMsg("%s", aNetworkName);

    VerifyOrExit(strlen(aNetworkName) <= OT_NETWORK_NAME_MAX_SIZE, error = kThreadError_InvalidArgs);

    strncpy(mNetworkName.m8, aNetworkName, sizeof(mNetworkName) - 1);

exit:
    otLogFuncExitErr(error);
    return error;
}

PanId Mac::GetPanId(void) const
{
    return mPanId;
}

ThreadError Mac::SetPanId(PanId aPanId)
{
    otLogFuncEntryMsg("%d", aPanId);
    mPanId = aPanId;
    otPlatRadioSetPanId(mNetif.GetInstance(), mPanId);
    otLogFuncExit();
    return kThreadError_None;
}

const uint8_t *Mac::GetExtendedPanId(void) const
{
    return mExtendedPanId.m8;
}

ThreadError Mac::SetExtendedPanId(const uint8_t *aExtPanId)
{
    memcpy(mExtendedPanId.m8, aExtPanId, sizeof(mExtendedPanId));
    return kThreadError_None;
}

ThreadError Mac::SendFrameRequest(Sender &aSender)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mSendTail != &aSender && aSender.mNext == NULL, error = kThreadError_Already);

    if (mSendHead == NULL)
    {
        mSendHead = &aSender;
        mSendTail = &aSender;
    }
    else
    {
        mSendTail->mNext = &aSender;
        mSendTail = &aSender;
    }

    if (mState == kStateIdle)
    {
        mState = kStateTransmitData;
        StartCsmaBackoff();
    }

exit:
    return error;
}

void Mac::NextOperation(void)
{
    switch (mState)
    {
    case kStateActiveScan:
    case kStateEnergyScan:
        otLinkRawReceive(mNetif.GetInstance(), mScanChannel, otLinkReceiveDone);
        break;

    default:
        if (mRxOnWhenIdle || mReceiveTimer.IsRunning() || otPlatRadioGetPromiscuous(mNetif.GetInstance()))
        {
            otLinkRawReceive(mNetif.GetInstance(), mChannel, otLinkReceiveDone);
        }
        else
        {
            otLinkRawSleep(mNetif.GetInstance());
        }

        break;
    }
}

void Mac::ScheduleNextTransmission(void)
{
    if (mPendingScanRequest == kScanTypeActive)
    {
        mPendingScanRequest = kScanTypeNone;
        mState = kStateActiveScan;
        StartCsmaBackoff();
    }
    else if (mPendingScanRequest == kScanTypeEnergy)
    {
        mPendingScanRequest = kScanTypeNone;
        StartEnergyScan();
    }
    else if (mTransmitBeacon)
    {
        mTransmitBeacon = false;
        mState = kStateTransmitBeacon;
        StartCsmaBackoff();
    }
    else if (mSendHead != NULL)
    {
        mState = kStateTransmitData;
        StartCsmaBackoff();
    }
    else
    {
        mState = kStateIdle;
    }

    NextOperation();
}

void Mac::GenerateNonce(const ExtAddress &aAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel, uint8_t *aNonce)
{
    // source address
    for (int i = 0; i < 8; i++)
    {
        aNonce[i] = aAddress.m8[i];
    }

    aNonce += 8;

    // frame counter
    aNonce[0] = (aFrameCounter >> 24) & 0xff;
    aNonce[1] = (aFrameCounter >> 16) & 0xff;
    aNonce[2] = (aFrameCounter >> 8) & 0xff;
    aNonce[3] = (aFrameCounter >> 0) & 0xff;
    aNonce += 4;

    // security level
    aNonce[0] = aSecurityLevel;
}

void Mac::SendBeaconRequest(Frame &aFrame)
{
    // initialize MAC header
    uint16_t fcf = Frame::kFcfFrameMacCmd | Frame::kFcfDstAddrShort | Frame::kFcfSrcAddrNone;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetDstPanId(kShortAddrBroadcast);
    aFrame.SetDstAddr(kShortAddrBroadcast);
    aFrame.SetCommandId(Frame::kMacCmdBeaconRequest);

    otLogInfoMac("Sent Beacon Request");
}

void Mac::SendBeacon(Frame &aFrame)
{
    uint8_t numUnsecurePorts;
    Beacon *beacon;
    uint16_t fcf;

    // initialize MAC header
    fcf = Frame::kFcfFrameBeacon | Frame::kFcfDstAddrNone | Frame::kFcfSrcAddrExt;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetSrcPanId(mPanId);
    aFrame.SetSrcAddr(mExtAddress);

    // write payload
    beacon = reinterpret_cast<Beacon *>(aFrame.GetPayload());
    beacon->Init();

    // set the Joining Permitted flag
    mNetif.GetIp6Filter().GetUnsecurePorts(numUnsecurePorts);

    if (numUnsecurePorts)
    {
        beacon->SetJoiningPermitted();
    }
    else
    {
        beacon->ClearJoiningPermitted();
    }

    beacon->SetNetworkName(mNetworkName.m8);
    beacon->SetExtendedPanId(mExtendedPanId.m8);

    aFrame.SetPayloadLength(sizeof(*beacon));

    otLogInfoMac("Sent Beacon");
}

void Mac::HandleBeginTransmit(void *aContext)
{
    static_cast<Mac *>(aContext)->HandleBeginTransmit();
}

void Mac::ProcessTransmitSecurity(Frame &aFrame)
{
    uint32_t frameCounter = 0;
    uint8_t securityLevel;
    uint8_t keyIdMode;
    uint8_t nonce[kNonceSize];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;
    const uint8_t *key = NULL;
    const ExtAddress *extAddress = NULL;

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        key = mKeyManager.GetKek();
        frameCounter = mKeyManager.GetKekFrameCounter();
        mKeyManager.IncrementKekFrameCounter();
        extAddress = &mExtAddress;
        break;

    case Frame::kKeyIdMode1:
        key = mKeyManager.GetCurrentMacKey();
        frameCounter = mKeyManager.GetMacFrameCounter();
        mKeyManager.IncrementMacFrameCounter();
        aFrame.SetKeyId((mKeyManager.GetCurrentKeySequence() & 0x7f) + 1);
        extAddress = &mExtAddress;
        break;

    case Frame::kKeyIdMode2:
    {
        const uint8_t keySource[] = {0xff, 0xff, 0xff, 0xff};
        key = sMode2Key;
        frameCounter = 0xffffffff;
        aFrame.SetKeySource(keySource);
        aFrame.SetKeyId(0xff);
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;
    }

    default:
        assert(false);
        break;
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.SetFrameCounter(frameCounter);

    GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(key, 16);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), true);
    aesCcm.Finalize(aFrame.GetFooter(), &tagLength);

exit:
    {}
}

void Mac::HandleBeginTransmit(void)
{
    Frame &sendFrame(*mTxFrame);
    ThreadError error = kThreadError_None;

    if (mCsmaAttempts == 0 && mTransmitAttempts == 0)
    {
        sendFrame.SetPower(mMaxTransmitPower);

        switch (mState)
        {
        case kStateActiveScan:
            otPlatRadioSetPanId(mNetif.GetInstance(), kPanIdBroadcast);
            sendFrame.SetChannel(mScanChannel);
            SendBeaconRequest(sendFrame);
            sendFrame.SetSequence(0);
            break;

        case kStateTransmitBeacon:
            sendFrame.SetChannel(mChannel);
            SendBeacon(sendFrame);
            sendFrame.SetSequence(mBeaconSequence++);
            break;

        case kStateTransmitData:
            sendFrame.SetChannel(mChannel);
            SuccessOrExit(error = mSendHead->HandleFrameRequest(sendFrame));
            sendFrame.SetSequence(mDataSequence);
            break;

        default:
            assert(false);
            break;
        }

        // Security Processing
        ProcessTransmitSecurity(sendFrame);

        if (sendFrame.GetPower() > mMaxTransmitPower)
        {
            sendFrame.SetPower(mMaxTransmitPower);
        }
    }

    error = otLinkRawReceive(mNetif.GetInstance(), sendFrame.GetChannel(), otLinkReceiveDone);
    assert(error == kThreadError_None);
    error = otLinkRawTransmit(mNetif.GetInstance(), static_cast<RadioPacket *>(&sendFrame), otLinkTransmitDone);
    assert(error == kThreadError_None);

    if (sendFrame.GetAckRequest() && !(otPlatRadioGetCaps(mNetif.GetInstance()) & kRadioCapsAckTimeout))
    {
        mMacTimer.Start(kAckTimeout);
        otLogDebgMac("ack timer start");
    }

    if (mPcapCallback)
    {
        sendFrame.mDidTX = true;
        mPcapCallback(&sendFrame, mPcapCallbackContext);
    }

exit:

    if (error != kThreadError_None)
    {
        TransmitDoneTask(mTxFrame, false, kThreadError_Abort);
    }
}

void otLinkTransmitDone(otInstance *aInstance, RadioPacket *aPacket, bool aRxPending, ThreadError aError)
{
    otLogFuncEntryMsg("%!otError!, aRxPending=%u", aError, aRxPending ? 1 : 0);
    aInstance->mThreadNetif.GetMac().TransmitDoneTask(aPacket, aRxPending, aError);
    otLogFuncExit();
}

void Mac::TransmitDoneTask(RadioPacket *aPacket, bool aRxPending, ThreadError aError)
{
    mMacTimer.Stop();

    mCounters.mTxTotal++;

    Frame *packet = static_cast<Frame *>(aPacket);
    Address addr;
    packet->GetDstAddr(addr);

    if (addr.mShortAddress == kShortAddrBroadcast)
    {
        // Broadcast packet
        mCounters.mTxBroadcast++;
    }
    else
    {
        // Unicast packet
        mCounters.mTxUnicast++;
    }


    if (!RadioSupportsRetriesAndCsmaBackoff() &&
        aError == kThreadError_ChannelAccessFailure &&
        mCsmaAttempts < kMaxCSMABackoffs)
    {
        mCsmaAttempts++;
        StartCsmaBackoff();

        mCounters.mTxErrCca++;

        ExitNow();
    }

    mCsmaAttempts = 0;

    switch (mState)
    {
    case kStateTransmitData:
        if (aRxPending)
        {
            mReceiveTimer.Start(kDataPollTimeout);
        }

    // fall through

    case kStateActiveScan:
    case kStateTransmitBeacon:
        SentFrame(aError);
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

void Mac::HandleMacTimer(void *aContext)
{
    static_cast<Mac *>(aContext)->HandleMacTimer();
}

void Mac::HandleMacTimer(void)
{
    Address addr;

    switch (mState)
    {
    case kStateActiveScan:
        do
        {
            mScanChannels >>= 1;
            mScanChannel++;

            if (mScanChannels == 0 || mScanChannel > kPhyMaxChannel)
            {
                otLinkRawReceive(mNetif.GetInstance(), mChannel, otLinkReceiveDone);
                otPlatRadioSetPanId(mNetif.GetInstance(), mPanId);
                mActiveScanHandler(mScanContext, NULL);
                ScheduleNextTransmission();
                ExitNow();
            }
        }
        while ((mScanChannels & 1) == 0);

        StartCsmaBackoff();
        break;

    case kStateEnergyScan:
        EnergyScanDone(mEnergyScanCurrentMaxRssi);
        break;

    case kStateTransmitData:
        otLogDebgMac("ack timer fired");
        otLinkRawReceive(mNetif.GetInstance(), mChannel, otLinkReceiveDone);
        mCounters.mTxTotal++;

        mTxFrame->GetDstAddr(addr);

        if (addr.mShortAddress == kShortAddrBroadcast)
        {
            // Broadcast packet
            mCounters.mTxBroadcast++;
        }
        else
        {
            // Unicast Packet
            mCounters.mTxUnicast++;
        }

        SentFrame(kThreadError_NoAck);
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

void Mac::HandleReceiveTimer(void *aContext)
{
    static_cast<Mac *>(aContext)->HandleReceiveTimer();
}

void Mac::HandleReceiveTimer(void)
{
    otLogInfoMac("data poll timeout!");

    if (mState == kStateIdle)
    {
        NextOperation();
    }
}

void Mac::SentFrame(ThreadError aError)
{
    Frame &sendFrame(*mTxFrame);
    Sender *sender;

    switch (aError)
    {
    case kThreadError_None:
    case kThreadError_ChannelAccessFailure:
    case kThreadError_Abort:
        break;

    case kThreadError_NoAck:
        otDumpDebgMac("NO ACK", sendFrame.GetHeader(), 16);

        if (!RadioSupportsRetriesAndCsmaBackoff() &&
            mTransmitAttempts < kMaxFrameAttempts)
        {
            mTransmitAttempts++;
            StartCsmaBackoff();

            mCounters.mTxRetry++;

            ExitNow();
        }

        break;

    default:
        assert(false);
        break;
    }

    mTransmitAttempts = 0;
    mCsmaAttempts = 0;

    if (sendFrame.GetAckRequest())
    {
        mCounters.mTxAckRequested++;

        if (aError == kThreadError_None)
        {
            mCounters.mTxAcked++;
        }
    }
    else
    {
        mCounters.mTxNoAckRequested++;
    }

    switch (mState)
    {
    case kStateActiveScan:
        mCounters.mTxBeaconRequest++;
        mMacTimer.Start(mScanDuration);
        break;

    case kStateTransmitBeacon:
        mCounters.mTxBeacon++;
        ScheduleNextTransmission();
        break;

    case kStateTransmitData:
        if (mReceiveTimer.IsRunning())
        {
            mCounters.mTxDataPoll++;
        }
        else
        {
            mCounters.mTxData++;
        }

        sender = mSendHead;
        mSendHead = mSendHead->mNext;

        if (mSendHead == NULL)
        {
            mSendTail = NULL;
        }

        sender->mNext = NULL;

        mDataSequence++;
        otDumpDebgMac("TX", sendFrame.GetHeader(), sendFrame.GetLength());
        sender->HandleSentFrame(sendFrame, aError);

        ScheduleNextTransmission();
        break;

    default:
        assert(false);
        break;
    }

exit:
    {}
}

ThreadError Mac::ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    ThreadError error = kThreadError_None;
    uint8_t securityLevel;
    uint8_t keyIdMode;
    uint32_t frameCounter;
    uint8_t nonce[kNonceSize];
    uint8_t tag[Frame::kMaxMicSize];
    uint8_t tagLength;
    uint8_t keyid;
    uint32_t keySequence = 0;
    const uint8_t *macKey;
    const ExtAddress *extAddress;
    Crypto::AesCcm aesCcm;

    aFrame.SetSecurityValid(false);

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);
    otLogDebgMac("Frame counter %u", frameCounter);

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit((macKey = mKeyManager.GetKek()) != NULL, error = kThreadError_Security);
        extAddress = &aSrcAddr.mExtAddress;
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != NULL, error = kThreadError_Security);

        aFrame.GetKeyId(keyid);
        keyid--;

        if (keyid == (mKeyManager.GetCurrentKeySequence() & 0x7f))
        {
            // same key index
            keySequence = mKeyManager.GetCurrentKeySequence();
            macKey = mKeyManager.GetCurrentMacKey();
        }
        else if (keyid == ((mKeyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            // previous key index
            keySequence = mKeyManager.GetCurrentKeySequence() - 1;
            macKey = mKeyManager.GetTemporaryMacKey(keySequence);
        }
        else if (keyid == ((mKeyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            // next key index
            keySequence = mKeyManager.GetCurrentKeySequence() + 1;
            macKey = mKeyManager.GetTemporaryMacKey(keySequence);
        }
        else
        {
            ExitNow(error = kThreadError_Security);
        }

        if (keySequence < aNeighbor->mKeySequence)
        {
            ExitNow(error = kThreadError_Security);
        }
        else if (keySequence == aNeighbor->mKeySequence)
        {
            if ((frameCounter + 1) < aNeighbor->mValid.mLinkFrameCounter)
            {
                ExitNow(error = kThreadError_Security);
            }
            else if ((frameCounter + 1) == aNeighbor->mValid.mLinkFrameCounter)
            {
                // drop duplicated packets
                ExitNow(error = kThreadError_Duplicated);
            }
        }

        extAddress = &aSrcAddr.mExtAddress;

        break;

    case Frame::kKeyIdMode2:
        macKey = sMode2Key;
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;

    default:
        ExitNow(error = kThreadError_Security);
        break;
    }

    GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.SetKey(macKey, 16);
    aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
    aesCcm.Finalize(tag, &tagLength);

    VerifyOrExit(memcmp(tag, aFrame.GetFooter(), tagLength) == 0, error = kThreadError_Security);

    if (keyIdMode == Frame::kKeyIdMode1)
    {
        if (aNeighbor->mKeySequence != keySequence)
        {
            aNeighbor->mKeySequence = keySequence;
            aNeighbor->mValid.mMleFrameCounter = 0;
        }

        aNeighbor->mValid.mLinkFrameCounter = frameCounter + 1;

        if (keySequence > mKeyManager.GetCurrentKeySequence())
        {
            mKeyManager.SetCurrentKeySequence(keySequence);
        }
    }

    aFrame.SetSecurityValid(true);

exit:
    return error;
}

void otLinkReceiveDone(otInstance *aInstance, RadioPacket *aFrame, ThreadError aError)
{
    otLogFuncEntryMsg("%!otError!", aError);
    aInstance->mThreadNetif.GetMac().ReceiveDoneTask(static_cast<Frame *>(aFrame), aError);
    otLogFuncExit();
}

void Mac::ReceiveDoneTask(Frame *aFrame, ThreadError aError)
{
    Address srcaddr;
    Address dstaddr;
    PanId panid;
    Neighbor *neighbor;
    otMacWhitelistEntry *whitelistEntry;
    otMacBlacklistEntry *blacklistEntry;
    int8_t rssi;
    bool receive = false;
    ThreadError error = aError;

    mCounters.mRxTotal++;

    VerifyOrExit(error == kThreadError_None, ;);
    VerifyOrExit(aFrame != NULL, error = kThreadError_NoFrameReceived);

    aFrame->mSecurityValid = false;

    if (mPcapCallback)
    {
        aFrame->mDidTX = false;
        mPcapCallback(aFrame, mPcapCallbackContext);
    }

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    aFrame->GetSrcAddr(srcaddr);
    neighbor = mMle.GetNeighbor(srcaddr);

    switch (srcaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        otLogDebgMac("Received from short address %x", srcaddr.mShortAddress);

        if (neighbor == NULL)
        {
            otLogDebgMac("drop not neighbor");
            ExitNow(error = kThreadError_UnknownNeighbor);
        }

        srcaddr.mLength = sizeof(srcaddr.mExtAddress);
        memcpy(&srcaddr.mExtAddress, &neighbor->mMacAddr, sizeof(srcaddr.mExtAddress));
        break;

    case sizeof(ExtAddress):
        break;

    default:
        ExitNow(error = kThreadError_InvalidSourceAddress);
    }

    // Duplicate Address Protection
    if (memcmp(&srcaddr.mExtAddress, &mExtAddress, sizeof(srcaddr.mExtAddress)) == 0)
    {
        otLogDebgMac("duplicate address received");
        ExitNow(error = kThreadError_InvalidSourceAddress);
    }

    // Source Whitelist Processing
    if (srcaddr.mLength != 0 && mWhitelist.IsEnabled())
    {
        VerifyOrExit((whitelistEntry = mWhitelist.Find(srcaddr.mExtAddress)) != NULL, error = kThreadError_WhitelistFiltered);

        if (mWhitelist.GetFixedRssi(*whitelistEntry, rssi) == kThreadError_None)
        {
            aFrame->mPower = rssi;
        }
    }

    // Source Blacklist Processing
    if (srcaddr.mLength != 0 && mBlacklist.IsEnabled())
    {
        VerifyOrExit((blacklistEntry = mBlacklist.Find(srcaddr.mExtAddress)) == NULL, error = kThreadError_BlacklistFiltered);
    }

    // Destination Address Filtering
    aFrame->GetDstAddr(dstaddr);

    switch (dstaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        aFrame->GetDstPanId(panid);
        VerifyOrExit((panid == kShortAddrBroadcast || panid == mPanId) &&
                     ((mRxOnWhenIdle && dstaddr.mShortAddress == kShortAddrBroadcast) ||
                      dstaddr.mShortAddress == mShortAddress), error = kThreadError_DestinationAddressFiltered);
        break;

    case sizeof(ExtAddress):
        aFrame->GetDstPanId(panid);
        VerifyOrExit(panid == mPanId &&
                     memcmp(&dstaddr.mExtAddress, &mExtAddress, sizeof(dstaddr.mExtAddress)) == 0,
                     error = kThreadError_DestinationAddressFiltered);
        break;
    }

    // Increment coutners
    if (dstaddr.mShortAddress == kShortAddrBroadcast)
    {
        // Broadcast packet
        mCounters.mRxBroadcast++;
    }
    else
    {
        // Unicast packet
        mCounters.mRxUnicast++;
    }

    // Security Processing
    SuccessOrExit(error = ProcessReceiveSecurity(*aFrame, srcaddr, neighbor));

    if (neighbor != NULL)
    {
        neighbor->mLinkInfo.AddRss(mNoiseFloor, aFrame->mPower);
    }

    switch (mState)
    {
    case kStateActiveScan:
        if (aFrame->GetType() == Frame::kFcfFrameBeacon)
        {
            mCounters.mRxBeacon++;
            mActiveScanHandler(mScanContext, aFrame);
        }
        else
        {
            mCounters.mRxOther++;
        }

        break;

    default:
        if (!mRxOnWhenIdle && dstaddr.mLength != 0)
        {
            mReceiveTimer.Stop();
            otLinkRawSleep(mNetif.GetInstance());
        }

        switch (aFrame->GetType())
        {
        case Frame::kFcfFrameMacCmd:
            if (HandleMacCommand(*aFrame) == kThreadError_Drop)
            {
                ExitNow(error = kThreadError_None);
            }

            receive = true;
            break;

        case Frame::kFcfFrameBeacon:
            mCounters.mRxBeacon++;
            receive = true;
            break;

        case Frame::kFcfFrameData:
            mCounters.mRxData++;
            receive = true;
            break;

        default:
            mCounters.mRxOther++;
            break;
        }

        if (receive)
        {
            otDumpDebgMac("RX", aFrame->GetHeader(), aFrame->GetLength());

            for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
            {
                receiver->HandleReceivedFrame(*aFrame);
            }
        }

        break;
    }

exit:

    if (error != kThreadError_None)
    {
        otLogDebgMacErr(error, "Dropping received frame");

        switch (error)
        {
        case kThreadError_Security:
            mCounters.mRxErrSec++;
            break;

        case kThreadError_FcsErr:
            mCounters.mRxErrFcs++;
            break;

        case kThreadError_NoFrameReceived:
            mCounters.mRxErrNoFrame++;
            break;

        case kThreadError_UnknownNeighbor:
            mCounters.mRxErrUnknownNeighbor++;
            break;

        case kThreadError_InvalidSourceAddress:
            mCounters.mRxErrInvalidSrcAddr++;
            break;

        case kThreadError_WhitelistFiltered:
            mCounters.mRxWhitelistFiltered++;
            break;

        case kThreadError_DestinationAddressFiltered:
            mCounters.mRxDestAddrFiltered++;
            break;

        case kThreadError_Duplicated:
            mCounters.mRxDuplicated++;
            break;

        default:
            mCounters.mRxErrOther++;
            break;
        }
    }
}

ThreadError Mac::HandleMacCommand(Frame &aFrame)
{
    ThreadError error = kThreadError_None;
    uint8_t commandId;

    aFrame.GetCommandId(commandId);

    switch (commandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        otLogInfoMac("Received Beacon Request");

        mTransmitBeacon = true;

        if (mState == kStateIdle)
        {
            mState = kStateTransmitBeacon;
            mTransmitBeacon = false;
            StartCsmaBackoff();
        }

        ExitNow(error = kThreadError_Drop);

    case Frame::kMacCmdDataRequest:
        mCounters.mRxDataPoll++;
        break;

    default:
        mCounters.mRxOther++;
        break;
    }

exit:
    return error;
}

void Mac::SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    mPcapCallback = aPcapCallback;
    mPcapCallbackContext = aCallbackContext;
}

bool Mac::IsPromiscuous(void)
{
    return otPlatRadioGetPromiscuous(mNetif.GetInstance());
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    otPlatRadioSetPromiscuous(mNetif.GetInstance(), aPromiscuous);

    if (mState == kStateIdle)
    {
        NextOperation();
    }
}

bool Mac::RadioSupportsRetriesAndCsmaBackoff(void)
{
    return (otPlatRadioGetCaps(mNetif.GetInstance()) & kRadioCapsTransmitRetries) != 0;
}

Whitelist &Mac::GetWhitelist(void)
{
    return mWhitelist;
}

Blacklist &Mac::GetBlacklist(void)
{
    return mBlacklist;
}

void Mac::FillMacCountersTlv(NetworkDiagnostic::MacCountersTlv &aMacCounters) const
{
    aMacCounters.SetIfInUnknownProtos(mCounters.mRxOther);
    aMacCounters.SetIfInErrors(mCounters.mRxErrNoFrame + mCounters.mRxErrUnknownNeighbor + mCounters.mRxErrInvalidSrcAddr +
                               mCounters.mRxErrSec + mCounters.mRxErrFcs + mCounters.mRxErrOther);
    aMacCounters.SetIfOutErrors(mCounters.mTxErrCca);
    aMacCounters.SetIfInUcastPkts(mCounters.mRxUnicast);
    aMacCounters.SetIfInBroadcastPkts(mCounters.mRxBroadcast);
    aMacCounters.SetIfInDiscards(mCounters.mRxWhitelistFiltered + mCounters.mRxDestAddrFiltered + mCounters.mRxDuplicated);
    aMacCounters.SetIfOutUcastPkts(mCounters.mTxUnicast);
    aMacCounters.SetIfOutBroadcastPkts(mCounters.mTxBroadcast);
    aMacCounters.SetIfOutDiscards(0);
}

void Mac::ResetCounters(void)
{
    memset(&mCounters, 0, sizeof(mCounters));
}

otMacCounters &Mac::GetCounters(void)
{
    return mCounters;
}

void Mac::EnableSrcMatch(bool aEnable)
{
    otPlatRadioEnableSrcMatch(mNetif.GetInstance(), aEnable);
    otLogDebgMac("Enable SrcMatch -- %d(0:Dis, 1:En)", aEnable);
}

ThreadError Mac::AddSrcMatchEntry(Address &aAddr)
{
    ThreadError error = kThreadError_None;

    if (aAddr.mLength == 2)
    {
        error = otPlatRadioAddSrcMatchShortEntry(mNetif.GetInstance(), aAddr.mShortAddress);
        otLogDebgMac("Adding short address: 0x%x -- %d (0:Ok, 3:NoBufs)", aAddr.mShortAddress, error);
    }
    else
    {
        uint8_t buf[8];

        for (uint8_t i = 0; i < sizeof(buf); i++)
        {
            buf[i] = aAddr.mExtAddress.m8[7 - i];
        }

        error = otPlatRadioAddSrcMatchExtEntry(mNetif.GetInstance(), buf);
        otLogDebgMac("Adding extended address: 0x%02x%02x%02x%02x%02x%02x%02x%02x -- %d (0:OK, 3:NoBufs)",
                     buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0], error);
    }

    return error;
}

ThreadError Mac::ClearSrcMatchEntry(Address &aAddr)
{
    ThreadError error = kThreadError_None;

    if (aAddr.mLength == 2)
    {
        error = otPlatRadioClearSrcMatchShortEntry(mNetif.GetInstance(), aAddr.mShortAddress);
        otLogDebgMac("Clearing short address: 0x%x -- %d (0:OK, 10:NoAddress)", aAddr.mShortAddress, error);
    }
    else
    {
        uint8_t buf[8];

        for (uint8_t i = 0; i < sizeof(buf); i++)
        {
            buf[i] = aAddr.mExtAddress.m8[7 - i];
        }

        error = otPlatRadioClearSrcMatchExtEntry(mNetif.GetInstance(), buf);
        otLogDebgMac("Clearing extended address: 0x%02x%02x%02x%02x%02x%02x%02x%02x -- %d (0:OK, 10:NoAddress)",
                     buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0], error);
    }

    return error;
}

void Mac::ClearSrcMatchEntries()
{
    otPlatRadioClearSrcMatchShortEntries(mNetif.GetInstance());
    otPlatRadioClearSrcMatchExtEntries(mNetif.GetInstance());
    otLogDebgMac("Clearing source match table");
}

}  // namespace Mac
}  // namespace Thread
