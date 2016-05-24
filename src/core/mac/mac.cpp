/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <string.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <crypto/aes_ccm.hpp>
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

static Tasklet sReceiveDoneTask(&Mac::ReceiveDoneTask, NULL);
static Tasklet sTransmitDoneTask(&Mac::TransmitDoneTask, NULL);

void Mac::StartCsmaBackoff(void)
{
    uint32_t backoffExponent = kMinBE + mCsmaAttempts;
    uint32_t backoff;

    if (backoffExponent > kMaxBE)
    {
        backoffExponent = kMaxBE;
    }

    backoff = (kUnitBackoffPeriod * kPhyUsPerSymbol * (1 << backoffExponent)) / 1000;
    backoff = (otPlatRandomGet() % backoff) + kMinBackoff;

    mBackoffTimer.Start(backoff);
}

Mac::Mac(ThreadNetif &aThreadNetif):
    mBeginTransmit(&HandleBeginTransmit, this),
    mAckTimer(&HandleAckTimer, this),
    mBackoffTimer(&HandleBeginTransmit, this),
    mReceiveTimer(&HandleReceiveTimer, this),
    mKeyManager(aThreadNetif.GetKeyManager()),
    mMle(aThreadNetif.GetMle()),
    mWhitelist()
{
    sMac = this;

    mState = kStateDisabled;

    mRxOnWhenIdle = true;
    mCsmaAttempts = 0;
    mTransmitBeacon = false;
    mBeacon.Init();

    mActiveScanRequest = false;
    mScanChannel = kPhyMinChannel;
    mScanChannels = 0xff;
    mScanDuration = 0;
    mActiveScanHandler = NULL;
    mActiveScanContext = NULL;

    mSendHead = NULL;
    mSendTail = NULL;
    mReceiveHead = NULL;
    mReceiveTail = NULL;
    mChannel = OPENTHREAD_CONFIG_DEFAULT_CHANNEL;
    mPanId = kPanIdBroadcast;
    mShortAddress = kShortAddrInvalid;

    for (size_t i = 0; i < sizeof(mExtAddress); i++)
    {
        mExtAddress.m8[i] = otPlatRandomGet();
    }

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);

    mBeaconSequence = otPlatRandomGet();
    mDataSequence = otPlatRandomGet();
}

ThreadError Mac::Start(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState == kStateDisabled, ;);

    SuccessOrExit(error = otPlatRadioEnable());

    SetExtendedPanId(mBeacon.GetExtendedPanId());
    otPlatRadioSetPanId(mPanId);
    otPlatRadioSetShortAddress(mShortAddress);
    {
        uint8_t buf[8];

        for (size_t i = 0; i < sizeof(buf); i++)
        {
            buf[i] = mExtAddress.m8[7 - i];
        }

        otPlatRadioSetExtendedAddress(buf);
    }
    mState = kStateIdle;
    NextOperation();

exit:
    return error;
}

ThreadError Mac::Stop(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = otPlatRadioDisable());
    mAckTimer.Stop();
    mBackoffTimer.Stop();
    mState = kStateDisabled;

    while (mSendHead != NULL)
    {
        Sender *cur;
        cur = mSendHead;
        mSendHead = mSendHead->mNext;
        cur->mNext = NULL;
    }

    mSendTail = NULL;

    while (mReceiveHead != NULL)
    {
        Receiver *cur;
        cur = mReceiveHead;
        mReceiveHead = mReceiveHead->mNext;
        cur->mNext = NULL;
    }

    mReceiveTail = NULL;

exit:
    return error;
}

ThreadError Mac::ActiveScan(uint16_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState != kStateDisabled && mState != kStateActiveScan && mActiveScanRequest == false,
                 error = kThreadError_Busy);

    mActiveScanHandler = aHandler;
    mActiveScanContext = aContext;
    mScanChannels = (aScanChannels == 0) ? kScanChannelsAll : aScanChannels;
    mScanDuration = (aScanDuration == 0) ? kScanDurationDefault : aScanDuration;

    mScanChannel = kPhyMinChannel;

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;
    }

    if (mState == kStateIdle)
    {
        mState = kStateActiveScan;
        StartCsmaBackoff();
    }
    else
    {
        mActiveScanRequest = true;
    }

exit:
    return error;
}

bool Mac::IsActiveScanInProgress(void)
{
    return (mState == kStateActiveScan) || mActiveScanRequest;
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
}

const ExtAddress *Mac::GetExtAddress(void) const
{
    return &mExtAddress;
}

ShortAddress Mac::GetShortAddress(void) const
{
    return mShortAddress;
}

ThreadError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    return otPlatRadioSetShortAddress(aShortAddress);
}

uint8_t Mac::GetChannel(void) const
{
    return mChannel;
}

ThreadError Mac::SetChannel(uint8_t aChannel)
{
    mChannel = aChannel;
    return kThreadError_None;
}

const char *Mac::GetNetworkName(void) const
{
    return mBeacon.GetNetworkName();
}

ThreadError Mac::SetNetworkName(const char *aNetworkName)
{
    mBeacon.SetNetworkName(aNetworkName);
    return kThreadError_None;
}

PanId Mac::GetPanId(void) const
{
    return mPanId;
}

ThreadError Mac::SetPanId(PanId aPanId)
{
    mPanId = aPanId;
    return otPlatRadioSetPanId(mPanId);
}

const uint8_t *Mac::GetExtendedPanId(void) const
{
    return mBeacon.GetExtendedPanId();
}

ThreadError Mac::SetExtendedPanId(const uint8_t *aExtPanId)
{
    mBeacon.SetExtendedPanId(aExtPanId);
    return kThreadError_None;
}

ThreadError Mac::SendFrameRequest(Sender &aSender)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState != kStateDisabled &&
                 mSendTail != &aSender && aSender.mNext == NULL,
                 error = kThreadError_Busy);

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
    case kStateDisabled:
        break;

    case kStateActiveScan:
        mReceiveFrame.SetChannel(mScanChannel);
        otPlatRadioReceive(&mReceiveFrame);
        break;

    default:
        if (mRxOnWhenIdle || mReceiveTimer.IsRunning())
        {
            mReceiveFrame.SetChannel(mChannel);
            otPlatRadioReceive(&mReceiveFrame);
        }
        else
        {
            otPlatRadioSleep();
        }

        break;
    }
}

void Mac::ScheduleNextTransmission(void)
{
    if (mActiveScanRequest)
    {
        mActiveScanRequest = false;
        mState = kStateActiveScan;
        StartCsmaBackoff();
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
    aNonce[0] = aFrameCounter >> 24;
    aNonce[1] = aFrameCounter >> 16;
    aNonce[2] = aFrameCounter >> 8;
    aNonce[3] = aFrameCounter >> 0;
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
    uint16_t fcf;

    // initialize MAC header
    fcf = Frame::kFcfFrameBeacon | Frame::kFcfDstAddrNone | Frame::kFcfSrcAddrExt;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetSrcPanId(mPanId);
    aFrame.SetSrcAddr(mExtAddress);

    // write payload
    memcpy(aFrame.GetPayload(), &mBeacon, sizeof(mBeacon));
    aFrame.SetPayloadLength(sizeof(mBeacon));

    otLogInfoMac("Sent Beacon\n");
}

void Mac::HandleBeginTransmit(void *aContext)
{
    Mac *obj = reinterpret_cast<Mac *>(aContext);
    obj->HandleBeginTransmit();
}

void Mac::ProcessTransmitSecurity(void)
{
    uint8_t securityLevel;
    uint8_t nonce[kNonceSize];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;

    if (mSendFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    mSendFrame.GetSecurityLevel(securityLevel);
    mSendFrame.SetFrameCounter(mKeyManager.GetMacFrameCounter());

    mSendFrame.SetKeyId((mKeyManager.GetCurrentKeySequence() & 0x7f) + 1);

    GenerateNonce(mExtAddress, mKeyManager.GetMacFrameCounter(), securityLevel, nonce);

    aesCcm.SetKey(mKeyManager.GetCurrentMacKey(), 16);
    tagLength = mSendFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.Init(mSendFrame.GetHeaderLength(), mSendFrame.GetPayloadLength(), tagLength,
                nonce, sizeof(nonce));

    aesCcm.Header(mSendFrame.GetHeader(), mSendFrame.GetHeaderLength());
    aesCcm.Payload(mSendFrame.GetPayload(), mSendFrame.GetPayload(), mSendFrame.GetPayloadLength(), true);
    aesCcm.Finalize(mSendFrame.GetFooter(), &tagLength);

    mKeyManager.IncrementMacFrameCounter();

exit:
    {}
}

void Mac::HandleBeginTransmit(void)
{
    ThreadError error = kThreadError_None;

    if (otPlatRadioIdle() != kThreadError_None)
    {
        mBeginTransmit.Post();
        ExitNow();
    }

    switch (mState)
    {
    case kStateActiveScan:
        mSendFrame.SetChannel(mScanChannel);
        SendBeaconRequest(mSendFrame);
        mSendFrame.SetSequence(0);
        break;

    case kStateTransmitBeacon:
        mSendFrame.SetChannel(mChannel);
        SendBeacon(mSendFrame);
        mSendFrame.SetSequence(mBeaconSequence++);
        break;

    case kStateTransmitData:
        mSendFrame.SetChannel(mChannel);
        SuccessOrExit(error = mSendHead->HandleFrameRequest(mSendFrame));
        mSendFrame.SetSequence(mDataSequence);
        break;

    default:
        assert(false);
        break;
    }

    // Security Processing
    ProcessTransmitSecurity();

    SuccessOrExit(error = otPlatRadioTransmit(&mSendFrame));

    if (mSendFrame.GetAckRequest() && !(otPlatRadioGetCaps() & kRadioCapsAckTimeout))
    {
        mAckTimer.Start(kAckTimeout);
        otLogDebgMac("ack timer start\n");
    }

exit:

    if (error != kThreadError_None)
    {
        assert(false);
    }
}

extern "C" void otPlatRadioSignalTransmitDone(void)
{
    sTransmitDoneTask.Post();
}

void Mac::TransmitDoneTask(void *aContext)
{
    sMac->TransmitDoneTask();
}

void Mac::TransmitDoneTask(void)
{
    ThreadError error;
    bool rxPending;

    error = otPlatRadioHandleTransmitDone(&rxPending);

    mAckTimer.Stop();

    if (error != kThreadError_None)
    {
        if (mCsmaAttempts < kMaxCSMABackoffs)
        {
            mCsmaAttempts++;
        }

        StartCsmaBackoff();
        ExitNow();
    }

    switch (mState)
    {
    case kStateActiveScan:
        mAckTimer.Start(mScanDuration);
        break;

    case kStateTransmitBeacon:
        SentFrame(true);
        break;

    case kStateTransmitData:
        if (rxPending)
        {
            mReceiveTimer.Start(kDataPollTimeout);
        }
        else
        {
            mReceiveTimer.Stop();
        }

        SentFrame(true);
        break;

    default:
        assert(false);
        break;
    }

exit:
    NextOperation();
}

void Mac::HandleAckTimer(void *aContext)
{
    Mac *obj = reinterpret_cast<Mac *>(aContext);
    obj->HandleAckTimer();
}

void Mac::HandleAckTimer(void)
{
    otPlatRadioIdle();

    switch (mState)
    {
    case kStateActiveScan:
        do
        {
            mScanChannels >>= 1;
            mScanChannel++;

            if (mScanChannels == 0 || mScanChannel > kPhyMaxChannel)
            {
                mActiveScanHandler(mActiveScanContext, NULL);
                ScheduleNextTransmission();
                ExitNow();
            }
        }
        while ((mScanChannels & 1) == 0);

        StartCsmaBackoff();
        break;

    case kStateTransmitData:
        otLogDebgMac("ack timer fired\n");
        SentFrame(false);
        break;

    default:
        assert(false);
        break;
    }

exit:
    NextOperation();
}

void Mac::HandleReceiveTimer(void *aContext)
{
    Mac *obj = reinterpret_cast<Mac *>(aContext);
    obj->HandleReceiveTimer();
}

void Mac::HandleReceiveTimer(void)
{
    otLogInfoMac("data poll timeout!\n");
    NextOperation();
}

void Mac::SentFrame(bool aAcked)
{
    Address destination;
    Neighbor *neighbor;
    Sender *sender;

    switch (mState)
    {
    case kStateActiveScan:
        mAckTimer.Start(mScanDuration);
        break;

    case kStateTransmitBeacon:
        ScheduleNextTransmission();
        break;

    case kStateTransmitData:
        if (mSendFrame.GetAckRequest() && !aAcked)
        {
            otDumpDebgMac("NO ACK", mSendFrame.GetHeader(), 16);

            if (mCsmaAttempts < kMaxCSMABackoffs)
            {
                mCsmaAttempts++;
                StartCsmaBackoff();
                ExitNow();
            }

            mSendFrame.GetDstAddr(destination);

            if ((neighbor = mMle.GetNeighbor(destination)) != NULL)
            {
                neighbor->mState = Neighbor::kStateInvalid;
            }
        }

        mCsmaAttempts = 0;

        sender = mSendHead;
        mSendHead = mSendHead->mNext;

        if (mSendHead == NULL)
        {
            mSendTail = NULL;
        }

        mDataSequence++;
        sender->HandleSentFrame(mSendFrame);

        ScheduleNextTransmission();
        break;

    default:
        assert(false);
        break;
    }

exit:
    {}
}

ThreadError Mac::ProcessReceiveSecurity(const Address &aSrcAddr, Neighbor *aNeighbor)
{
    ThreadError error = kThreadError_None;
    uint8_t securityLevel;
    uint32_t frameCounter;
    uint8_t nonce[kNonceSize];
    uint8_t tag[Frame::kMaxMicSize];
    uint8_t tagLength;
    uint8_t keyid;
    uint32_t keySequence;
    const uint8_t *macKey;
    Crypto::AesCcm aesCcm;

    if (mReceiveFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    VerifyOrExit(aNeighbor != NULL, error = kThreadError_Security);

    mReceiveFrame.GetSecurityLevel(securityLevel);
    mReceiveFrame.GetFrameCounter(frameCounter);

    GenerateNonce(aSrcAddr.mExtAddress, frameCounter, securityLevel, nonce);

    tagLength = mReceiveFrame.GetFooterLength() - Frame::kFcsSize;

    mReceiveFrame.GetKeyId(keyid);
    keyid--;

    if (keyid == (mKeyManager.GetCurrentKeySequence() & 0x7f))
    {
        // same key index
        keySequence = mKeyManager.GetCurrentKeySequence();
        macKey = mKeyManager.GetCurrentMacKey();
        VerifyOrExit(aNeighbor->mPreviousKey == true || frameCounter >= aNeighbor->mValid.mLinkFrameCounter,
                     error = kThreadError_Security);
    }
    else if (aNeighbor->mPreviousKey &&
             mKeyManager.IsPreviousKeyValid() &&
             keyid == (mKeyManager.GetPreviousKeySequence() & 0x7f))
    {
        // previous key index
        keySequence = mKeyManager.GetPreviousKeySequence();
        macKey = mKeyManager.GetPreviousMacKey();
        VerifyOrExit(frameCounter >= aNeighbor->mValid.mLinkFrameCounter, error = kThreadError_Security);
    }
    else if (keyid == ((mKeyManager.GetCurrentKeySequence() + 1) & 0x7f))
    {
        // next key index
        keySequence = mKeyManager.GetCurrentKeySequence() + 1;
        macKey = mKeyManager.GetTemporaryMacKey(keySequence);
    }
    else
    {
        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(mReceiveFrame, kThreadError_Security);
        }

        ExitNow(error = kThreadError_Security);
    }

    aesCcm.SetKey(macKey, 16);
    aesCcm.Init(mReceiveFrame.GetHeaderLength(), mReceiveFrame.GetPayloadLength(),
                tagLength, nonce, sizeof(nonce));
    aesCcm.Header(mReceiveFrame.GetHeader(), mReceiveFrame.GetHeaderLength());
    aesCcm.Payload(mReceiveFrame.GetPayload(), mReceiveFrame.GetPayload(), mReceiveFrame.GetPayloadLength(),
                   false);
    aesCcm.Finalize(tag, &tagLength);

    VerifyOrExit(memcmp(tag, mReceiveFrame.GetFooter(), tagLength) == 0, error = kThreadError_Security);

    if (keySequence > mKeyManager.GetCurrentKeySequence())
    {
        mKeyManager.SetCurrentKeySequence(keySequence);
    }

    if (keySequence == mKeyManager.GetCurrentKeySequence())
    {
        aNeighbor->mPreviousKey = false;
    }

    aNeighbor->mValid.mLinkFrameCounter = frameCounter + 1;

exit:
    return error;
}

extern "C" void otPlatRadioSignalReceiveDone(void)
{
    sReceiveDoneTask.Post();
}

void Mac::ReceiveDoneTask(void *aContext)
{
    sMac->ReceiveDoneTask();
}

void Mac::ReceiveDoneTask(void)
{
    ThreadError error;
    Address srcaddr;
    Address dstaddr;
    PanId panid;
    Neighbor *neighbor;
    Whitelist::Entry *entry;
    int8_t rssi;

    error = otPlatRadioHandleReceiveDone();
    VerifyOrExit(error == kThreadError_None, ;);

    mReceiveFrame.GetSrcAddr(srcaddr);
    neighbor = mMle.GetNeighbor(srcaddr);

    switch (srcaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        VerifyOrExit(neighbor != NULL, otLogDebgMac("drop not neighbor\n"));
        srcaddr.mLength = sizeof(srcaddr.mExtAddress);
        memcpy(&srcaddr.mExtAddress, &neighbor->mMacAddr, sizeof(srcaddr.mExtAddress));
        break;

    case sizeof(ExtAddress):
        break;

    default:
        ExitNow();
    }

    // Source Whitelist Processing
    if (srcaddr.mLength != 0 && mWhitelist.IsEnabled())
    {
        VerifyOrExit((entry = mWhitelist.Find(srcaddr.mExtAddress)) != NULL, ;);

        if (mWhitelist.GetConstantRssi(*entry, rssi) == kThreadError_None)
        {
            mReceiveFrame.mPower = rssi;
        }
    }

    // Destination Address Filtering
    mReceiveFrame.GetDstAddr(dstaddr);

    switch (dstaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        mReceiveFrame.GetDstPanId(panid);
        VerifyOrExit((panid == kShortAddrBroadcast || panid == mPanId) &&
                     ((mRxOnWhenIdle && dstaddr.mShortAddress == kShortAddrBroadcast) ||
                      dstaddr.mShortAddress == mShortAddress), ;);
        break;

    case sizeof(ExtAddress):
        mReceiveFrame.GetDstPanId(panid);
        VerifyOrExit(panid == mPanId &&
                     memcmp(&dstaddr.mExtAddress, &mExtAddress, sizeof(dstaddr.mExtAddress)) == 0, ;);
        break;
    }

    // Security Processing
    SuccessOrExit(ProcessReceiveSecurity(srcaddr, neighbor));

    switch (mState)
    {
    case kStateActiveScan:
        mActiveScanHandler(mActiveScanContext, &mReceiveFrame);
        break;

    default:
        if (dstaddr.mLength != 0)
        {
            mReceiveTimer.Stop();
        }

        if (mReceiveFrame.GetType() == Frame::kFcfFrameMacCmd)
        {
            SuccessOrExit(HandleMacCommand());
        }

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(mReceiveFrame, kThreadError_None);
        }

        break;
    }

exit:
    NextOperation();
}

ThreadError Mac::HandleMacCommand(void)
{
    ThreadError error = kThreadError_None;
    uint8_t commandId;
    mReceiveFrame.GetCommandId(commandId);

    if (commandId == Frame::kMacCmdBeaconRequest)
    {
        otLogInfoMac("Received Beacon Request\n");
        mTransmitBeacon = true;

        if (mState == kStateIdle)
        {
            mState = kStateTransmitBeacon;
            mTransmitBeacon = false;
            StartCsmaBackoff();
        }

        ExitNow(error = kThreadError_Drop);
    }

exit:
    return error;
}

Whitelist &Mac::GetWhitelist(void)
{
    return mWhitelist;
}

}  // namespace Mac
}  // namespace Thread
