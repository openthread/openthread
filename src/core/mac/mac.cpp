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
#include <common/logging.hpp>
#include <crypto/aes_ccm.hpp>
#include <crypto/sha256.hpp>
#include <mac/mac.hpp>
#include <mac/mac_frame.hpp>
#include <platform/random.h>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>

namespace Thread {
namespace Mac {

static const uint8_t sExtendedPanidInit[] = {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe};
static const char sNetworkNameInit[] = "OpenThread";
static Mac *sMac;

void Mac::StartCsmaBackoff(void)
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
    sMac = this;

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

    otPlatRadioEnable(NULL);
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

    if (!(otPlatRadioGetCaps(NULL) & kRadioCapsEnergyScan))
    {
        mEnergyScanCurrentMaxRssi = kInvalidRssiValue;
        mMacTimer.Start(mScanDuration);
        mEnergyScanSampleRssiTask.Post();
        NextOperation();
    }
    else
    {
        ThreadError error = otPlatRadioEnergyScan(NULL, mScanChannel, mScanDuration);

        if (error != kThreadError_None)
        {
            // Cancel scan
            mEnergyScanHandler(mScanContext, NULL);
            ScheduleNextTransmission();
        }
    }
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *, int8_t aEnergyScanMaxRssi)
{
    sMac->EnergyScanDone(aEnergyScanMaxRssi);
}

void Mac::EnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (aEnergyScanMaxRssi != kInvalidRssiValue)
    {
        otEnergyScanResult result;

        result.mChannel = mScanChannel;
        result.mMaxRssi = aEnergyScanMaxRssi;
        mEnergyScanHandler(mScanContext, &result);
    }

    do
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannels == 0 || mScanChannel > kPhyMaxChannel)
        {
            mEnergyScanHandler(mScanContext, NULL);
            ScheduleNextTransmission();
            ExitNow();
        }
    }
    while ((mScanChannels & 1) == 0);

    if (!(otPlatRadioGetCaps(NULL) & kRadioCapsEnergyScan))
    {
        mEnergyScanCurrentMaxRssi = kInvalidRssiValue;
        mMacTimer.Start(mScanDuration);
    }
    else
    {
        ThreadError error = otPlatRadioEnergyScan(NULL, mScanChannel, mScanDuration);

        if (error != kThreadError_None)
        {
            // Cancel scan
            mEnergyScanHandler(mScanContext, NULL);
            ScheduleNextTransmission();
        }
    }

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

    rssi = otPlatRadioGetRssi(NULL);

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

ThreadError Mac::SetExtAddress(const ExtAddress &aExtAddress)
{
    ThreadError error;
    uint8_t buf[8];

    VerifyOrExit(!aExtAddress.IsGroup(), error = kThreadError_InvalidArgs);

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = aExtAddress.m8[7 - i];
    }

    SuccessOrExit(error = otPlatRadioSetExtendedAddress(NULL, buf));
    mExtAddress = aExtAddress;

exit:
    return error;
}

void Mac::GetHashMacAddress(ExtAddress *aHashMacAddress)
{
    Crypto::Sha256 sha256;
    uint8_t buf[Crypto::Sha256::kHashSize];

    otPlatRadioGetIeeeEui64(NULL, buf);
    sha256.Start();
    sha256.Update(buf, OT_EXT_ADDRESS_SIZE);
    sha256.Finish(buf);

    memcpy(aHashMacAddress->m8, buf, OT_EXT_ADDRESS_SIZE);
    aHashMacAddress->SetLocal(true);
}

ShortAddress Mac::GetShortAddress(void) const
{
    return mShortAddress;
}

ThreadError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    return otPlatRadioSetShortAddress(NULL, aShortAddress);
}

uint8_t Mac::GetChannel(void) const
{
    return mChannel;
}

ThreadError Mac::SetChannel(uint8_t aChannel)
{
    mChannel = aChannel;

    if (mState == kStateIdle)
    {
        NextOperation();
    }

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

    VerifyOrExit(strlen(aNetworkName) <= OT_NETWORK_NAME_MAX_SIZE, error = kThreadError_InvalidArgs);

    memset(&mNetworkName, 0, sizeof(mNetworkName));
    strncpy(mNetworkName.m8, aNetworkName, sizeof(mNetworkName));

exit:
    return error;
}

PanId Mac::GetPanId(void) const
{
    return mPanId;
}

ThreadError Mac::SetPanId(PanId aPanId)
{
    mPanId = aPanId;
    return otPlatRadioSetPanId(NULL, mPanId);
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

    VerifyOrExit(mSendTail != &aSender && aSender.mNext == NULL, error = kThreadError_Busy);

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
        otPlatRadioReceive(NULL, mScanChannel);
        break;

    default:
        if (mRxOnWhenIdle || mReceiveTimer.IsRunning() || otPlatRadioGetPromiscuous(NULL))
        {
            otPlatRadioReceive(NULL, mChannel);
        }
        else
        {
            otPlatRadioSleep(NULL);
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

    otLogInfoMac("Sent Beacon Request\n");
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

    otLogInfoMac("Sent Beacon\n");
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
        break;

    case Frame::kKeyIdMode1:
        key = mKeyManager.GetCurrentMacKey();
        frameCounter = mKeyManager.GetMacFrameCounter();
        mKeyManager.IncrementMacFrameCounter();
        aFrame.SetKeyId((mKeyManager.GetCurrentKeySequence() & 0x7f) + 1);
        break;

    default:
        assert(false);
        break;
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.SetFrameCounter(frameCounter);

    GenerateNonce(mExtAddress, frameCounter, securityLevel, nonce);

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
    Frame &sendFrame(*static_cast<Frame *>(otPlatRadioGetTransmitBuffer(NULL)));
    ThreadError error = kThreadError_None;

    sendFrame.SetPower(mMaxTransmitPower);

    switch (mState)
    {
    case kStateActiveScan:
        otPlatRadioSetPanId(NULL, kPanIdBroadcast);
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

    error = otPlatRadioReceive(NULL, sendFrame.GetChannel());
    assert(error == kThreadError_None);
    error = otPlatRadioTransmit(NULL);
    assert(error == kThreadError_None);

    if (sendFrame.GetAckRequest() && !(otPlatRadioGetCaps(NULL) & kRadioCapsAckTimeout))
    {
        mMacTimer.Start(kAckTimeout);
        otLogDebgMac("ack timer start\n");
    }

    if (mPcapCallback)
    {
        sendFrame.mDidTX = true;
        mPcapCallback(&sendFrame, mPcapCallbackContext);
    }

exit:

    if (error != kThreadError_None)
    {
        TransmitDoneTask(false, kThreadError_Abort);
    }
}

extern "C" void otPlatRadioTransmitDone(otInstance *, bool aRxPending, ThreadError aError)
{
    sMac->TransmitDoneTask(aRxPending, aError);
}

void Mac::TransmitDoneTask(bool aRxPending, ThreadError aError)
{
    mMacTimer.Stop();

    mCounters.mTxTotal++;

    if (aError == kThreadError_ChannelAccessFailure &&
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
    otPlatRadioReceive(NULL, mChannel);

    switch (mState)
    {
    case kStateActiveScan:
        do
        {
            mScanChannels >>= 1;
            mScanChannel++;

            if (mScanChannels == 0 || mScanChannel > kPhyMaxChannel)
            {
                otPlatRadioSetPanId(NULL, mPanId);
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
        otLogDebgMac("ack timer fired\n");
        mCounters.mTxTotal++;
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
    otLogInfoMac("data poll timeout!\n");

    if (mState == kStateIdle)
    {
        NextOperation();
    }
}

void Mac::SentFrame(ThreadError aError)
{
    Frame &sendFrame(*static_cast<Frame *>(otPlatRadioGetTransmitBuffer(NULL)));
    Sender *sender;

    switch (aError)
    {
    case kThreadError_None:
    case kThreadError_ChannelAccessFailure:
    case kThreadError_Abort:
        break;

    case kThreadError_NoAck:
        otDumpDebgMac("NO ACK", sendFrame.GetHeader(), 16);

        if (mTransmitAttempts < kMaxFrameAttempts)
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
    Crypto::AesCcm aesCcm;

    aFrame.SetSecurityValid(false);

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);
    otLogDebgMac("Frame counter %u\n", frameCounter);

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit((macKey = mKeyManager.GetKek()) != NULL, error = kThreadError_Security);
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

        VerifyOrExit((keySequence > aNeighbor->mKeySequence) ||
                     ((keySequence == aNeighbor->mKeySequence) &&
                      (frameCounter >= aNeighbor->mValid.mLinkFrameCounter)),
                     error = kThreadError_Security);

        break;

    default:
        ExitNow(error = kThreadError_Security);
        break;
    }

    GenerateNonce(aSrcAddr.mExtAddress, frameCounter, securityLevel, nonce);
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

    if (error != kThreadError_None)
    {
        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(aFrame, kThreadError_Security);
        }
    }

    return error;
}

extern "C" void otPlatRadioReceiveDone(otInstance *, RadioPacket *aFrame, ThreadError aError)
{
    sMac->ReceiveDoneTask(static_cast<Frame *>(aFrame), aError);
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

    aFrame->GetSrcAddr(srcaddr);
    neighbor = mMle.GetNeighbor(srcaddr);

    switch (srcaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        otLogDebgMac("Received from short address %x\n", srcaddr.mShortAddress);

        if (neighbor == NULL)
        {
            otLogDebgMac("drop not neighbor\n");
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
        otLogDebgMac("duplicate address received\n");
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
        if (dstaddr.mLength != 0)
        {
            mReceiveTimer.Stop();
        }

        switch (aFrame->GetType())
        {
        case Frame::kFcfFrameMacCmd:
            if (HandleMacCommand(*aFrame) == kThreadError_Drop)
            {
                ExitNow(error = kThreadError_None);
            }

            break;

        case Frame::kFcfFrameBeacon:
            mCounters.mRxBeacon++;
            break;

        case Frame::kFcfFrameData:
            mCounters.mRxData++;
            break;

        default:
            mCounters.mRxOther++;
            break;
        }

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(*aFrame, kThreadError_None);
        }

        break;
    }

exit:

    if (error != kThreadError_None)
    {
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
        otLogInfoMac("Received Beacon Request\n");

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
    return otPlatRadioGetPromiscuous(NULL);
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    otPlatRadioSetPromiscuous(NULL, aPromiscuous);

    if (mState == kStateIdle)
    {
        NextOperation();
    }
}

Whitelist &Mac::GetWhitelist(void)
{
    return mWhitelist;
}

Blacklist &Mac::GetBlacklist(void)
{
    return mBlacklist;
}

otMacCounters &Mac::GetCounters(void)
{
    return mCounters;
}

void Mac::EnableSrcMatch(bool aEnable)
{
    otPlatRadioEnableSrcMatch(NULL, aEnable);
    otLogDebgMac("Enable SrcMatch -- %d(0:Dis, 1:En)\n", aEnable);
}

ThreadError Mac::AddSrcMatchEntry(Address &aAddr)
{
    ThreadError error = kThreadError_None;

    if (aAddr.mLength == 2)
    {
        error = otPlatRadioAddSrcMatchShortEntry(NULL, aAddr.mShortAddress);
        otLogDebgMac("Adding short address: 0x%x -- %d (0:Ok, 3:NoBufs)\n", aAddr.mShortAddress, error);
    }
    else
    {
        uint8_t buf[8];

        for (uint8_t i = 0; i < sizeof(buf); i++)
        {
            buf[i] = aAddr.mExtAddress.m8[7 - i];
        }

        error = otPlatRadioAddSrcMatchExtEntry(NULL, buf);
        otLogDebgMac("Adding extended address: 0x%02x%02x%02x%02x%02x%02x%02x%02x -- %d (0:OK, 3:NoBufs)\n",
                     buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0], error);
    }

    return error;
}

ThreadError Mac::ClearSrcMatchEntry(Address &aAddr)
{
    ThreadError error = kThreadError_None;

    if (aAddr.mLength == 2)
    {
        error = otPlatRadioClearSrcMatchShortEntry(NULL, aAddr.mShortAddress);
        otLogDebgMac("Clearing short address: 0x%x -- %d (0:OK, 10:NoAddress)\n", aAddr.mShortAddress, error);
    }
    else
    {
        uint8_t buf[8];

        for (uint8_t i = 0; i < sizeof(buf); i++)
        {
            buf[i] = aAddr.mExtAddress.m8[7 - i];
        }

        error = otPlatRadioClearSrcMatchExtEntry(NULL, buf);
        otLogDebgMac("Clearing extended address: 0x%02x%02x%02x%02x%02x%02x%02x%02x -- %d (0:OK, 10:NoAddress)\n",
                     buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0], error);
    }

    return error;
}

void Mac::ClearSrcMatchEntries()
{
    otPlatRadioClearSrcMatchShortEntries(NULL);
    otPlatRadioClearSrcMatchExtEntries(NULL);
    otLogDebgMac("Clearing source match table");
}

}  // namespace Mac
}  // namespace Thread
