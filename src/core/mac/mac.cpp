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
    mBeginTransmit(&HandleBeginTransmit, this),
    mAckTimer(&HandleAckTimer, this),
    mBackoffTimer(&HandleBeginTransmit, this),
    mReceiveTimer(&HandleReceiveTimer, this),
    mKeyManager(aThreadNetif.GetKeyManager()),
    mMle(aThreadNetif.GetMle()),
    mNetif(aThreadNetif),
    mWhitelist()
{
    sMac = this;

    mState = kStateIdle;

    mRxOnWhenIdle = false;
    mCsmaAttempts = 0;
    mTransmitAttempts = 0;
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

    memset(&mCounters, 0, sizeof(otMacCounters));

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);
    SetPanId(mPanId);
    SetExtAddress(mExtAddress);
    SetShortAddress(kShortAddrInvalid);

    mBeaconSequence = otPlatRandomGet();
    mDataSequence = otPlatRandomGet();

    mPcapCallback = NULL;

    otPlatRadioEnable();
}

ThreadError Mac::ActiveScan(uint16_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState != kStateActiveScan && mActiveScanRequest == false, error = kThreadError_Busy);

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

ThreadError Mac::SetExtAddress(const ExtAddress &aExtAddress)
{
    uint8_t buf[8];

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = mExtAddress.m8[7 - i];
    }

    mExtAddress = aExtAddress;
    return otPlatRadioSetExtendedAddress(buf);
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
        otPlatRadioReceive(mScanChannel);
        break;

    default:
        if (mRxOnWhenIdle || mReceiveTimer.IsRunning() || otPlatRadioGetPromiscuous())
        {
            otPlatRadioReceive(mChannel);
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

void Mac::ProcessTransmitSecurity(Frame &aFrame)
{
    uint8_t securityLevel;
    uint8_t nonce[kNonceSize];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.SetFrameCounter(mKeyManager.GetMacFrameCounter());

    aFrame.SetKeyId((mKeyManager.GetCurrentKeySequence() & 0x7f) + 1);

    GenerateNonce(mExtAddress, mKeyManager.GetMacFrameCounter(), securityLevel, nonce);

    aesCcm.SetKey(mKeyManager.GetCurrentMacKey(), 16);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), true);
    aesCcm.Finalize(aFrame.GetFooter(), &tagLength);

    mKeyManager.IncrementMacFrameCounter();

exit:
    {}
}

void Mac::HandleBeginTransmit(void)
{
    Frame &sendFrame(*static_cast<Frame *>(otPlatRadioGetTransmitBuffer()));
    ThreadError error = kThreadError_None;

    if (otPlatRadioIdle() != kThreadError_None)
    {
        mBeginTransmit.Post();
        ExitNow();
    }

    switch (mState)
    {
    case kStateActiveScan:
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

    SuccessOrExit(error = otPlatRadioTransmit());

    if (sendFrame.GetAckRequest() && !(otPlatRadioGetCaps() & kRadioCapsAckTimeout))
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

extern "C" void otPlatRadioTransmitDone(bool aRxPending, ThreadError aError)
{
    sMac->TransmitDoneTask(aRxPending, aError);
}

void Mac::TransmitDoneTask(bool aRxPending, ThreadError aError)
{
    mAckTimer.Stop();

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
        else
        {
            mReceiveTimer.Stop();
        }

    // fall through

    case kStateActiveScan:
    case kStateTransmitBeacon:
        SentFrame(aError == kThreadError_None);
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
        mCounters.mTxTotal++;
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
    Frame &sendFrame(*static_cast<Frame *>(otPlatRadioGetTransmitBuffer()));
    Address destination;
    Neighbor *neighbor;
    Sender *sender;

    if (sendFrame.GetAckRequest() && !aAcked)
    {
        otDumpDebgMac("NO ACK", sendFrame.GetHeader(), 16);

        if (mTransmitAttempts < kMaxFrameAttempts)
        {
            mTransmitAttempts++;
            StartCsmaBackoff();

            mCounters.mTxRetry++;

            ExitNow();
        }

        sendFrame.GetDstAddr(destination);

        if ((neighbor = mMle.GetNeighbor(destination)) != NULL)
        {
            if (neighbor->mState == Neighbor::kStateValid && mMle.GetChildId(neighbor->mValid.mRloc16) != 0)
            {
                mNetif.SetStateChangedFlags(OT_THREAD_CHILD_REMOVED);
            }

            neighbor->mState = Neighbor::kStateInvalid;
        }
    }

    mTransmitAttempts = 0;

    if (sendFrame.GetAckRequest())
    {
        mCounters.mTxAckRequested++;

        if (aAcked)
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
        mAckTimer.Start(mScanDuration);
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
        sender->HandleSentFrame(sendFrame);

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
    uint32_t frameCounter;
    uint8_t nonce[kNonceSize];
    uint8_t tag[Frame::kMaxMicSize];
    uint8_t tagLength;
    uint8_t keyid;
    uint32_t keySequence;
    const uint8_t *macKey;
    Crypto::AesCcm aesCcm;

    aFrame.SetSecurityValid(false);

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    VerifyOrExit(aNeighbor != NULL, error = kThreadError_Security);

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);

    GenerateNonce(aSrcAddr.mExtAddress, frameCounter, securityLevel, nonce);

    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

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
                 ((keySequence == aNeighbor->mKeySequence) && (frameCounter >= aNeighbor->mValid.mLinkFrameCounter)),
                 error = kThreadError_Security);

    aesCcm.SetKey(macKey, 16);
    aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
    aesCcm.Finalize(tag, &tagLength);

    VerifyOrExit(memcmp(tag, aFrame.GetFooter(), tagLength) == 0, error = kThreadError_Security);

    if (aNeighbor->mKeySequence != keySequence)
    {
        aNeighbor->mKeySequence = keySequence;
        aNeighbor->mValid.mMleFrameCounter = 0;
    }

    aNeighbor->mValid.mLinkFrameCounter = frameCounter + 1;

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

extern "C" void otPlatRadioReceiveDone(RadioPacket *aFrame, ThreadError aError)
{
    sMac->ReceiveDoneTask(static_cast<Frame *>(aFrame), aError);
}

void Mac::ReceiveDoneTask(Frame *aFrame, ThreadError aError)
{
    Address srcaddr;
    Address dstaddr;
    PanId panid;
    Neighbor *neighbor;
    otMacWhitelistEntry *entry;
    int8_t rssi;
    ThreadError error = aError;

    mCounters.mRxTotal++;

    VerifyOrExit(error == kThreadError_None, ;);
    VerifyOrExit(aFrame != NULL, error = kThreadError_NoFrameReceived);

    aFrame->mSecurityValid = false;

    if (mPcapCallback)
    {
        mPcapCallback(aFrame);
    }

    aFrame->GetSrcAddr(srcaddr);
    neighbor = mMle.GetNeighbor(srcaddr);

    switch (srcaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
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

    // Source Whitelist Processing
    if (srcaddr.mLength != 0 && mWhitelist.IsEnabled())
    {
        VerifyOrExit((entry = mWhitelist.Find(srcaddr.mExtAddress)) != NULL, error = kThreadError_WhitelistFiltered);

        if (mWhitelist.GetFixedRssi(*entry, rssi) == kThreadError_None)
        {
            aFrame->mPower = rssi;
        }
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
        neighbor->mLinkInfo.AddRss(aFrame->mPower);
    }

    switch (mState)
    {
    case kStateActiveScan:
        if (aFrame->GetType() == Frame::kFcfFrameBeacon)
        {
            mCounters.mRxBeacon++;
            mActiveScanHandler(mActiveScanContext, aFrame);
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

    NextOperation();
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

void Mac::SetPcapCallback(otLinkPcapCallback aPcapCallback)
{
    mPcapCallback = aPcapCallback;
}

bool Mac::IsPromiscuous(void)
{
    return otPlatRadioGetPromiscuous();
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    otPlatRadioSetPromiscuous(aPromiscuous);

    SuccessOrExit(otPlatRadioIdle());
    NextOperation();

exit:
    return;
}

Whitelist &Mac::GetWhitelist(void)
{
    return mWhitelist;
}

otMacCounters &Mac::GetCounters(void)
{
    return mCounters;
}

}  // namespace Mac
}  // namespace Thread
