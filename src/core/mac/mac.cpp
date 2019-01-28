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

#include "mac.hpp"

#include <stdio.h>
#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "crypto/aes_ccm.hpp"
#include "crypto/sha256.hpp"
#include "mac/mac_frame.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

using ot::Encoding::BigEndian::HostSwap64;

namespace ot {
namespace Mac {

static const uint8_t sMode2Key[] = {0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f,
                                    0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66};

static const otExtAddress sMode2ExtAddress = {
    {0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12},
};

static const otExtendedPanId sExtendedPanidInit = {
    {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe},
};
static const char sNetworkNameInit[] = "OpenThread";

Mac::Mac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mPendingActiveScan(false)
    , mPendingEnergyScan(false)
    , mPendingTransmitBeacon(false)
    , mPendingTransmitData(false)
    , mPendingTransmitOobFrame(false)
    , mPendingWaitingForData(false)
    , mRxOnWhenIdle(false)
    , mBeaconsEnabled(false)
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    , mShouldDelaySleep(false)
    , mDelayingSleep(false)
#endif
    , mOperation(kOperationIdle)
    , mBeaconSequence(Random::GetUint8())
    , mDataSequence(Random::GetUint8())
    , mBroadcastTransmitCount(0)
    , mPanId(kPanIdBroadcast)
    , mPanChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannelAcquisitionId(0)
    , mSupportedChannelMask(OT_RADIO_SUPPORTED_CHANNELS)
    , mScanChannel(OT_RADIO_CHANNEL_MIN)
    , mScanDuration(0)
    , mScanChannelMask()
    , mScanContext(NULL)
    , mActiveScanHandler(NULL) /* Initialize `mActiveScanHandler` and `mEnergyScanHandler` union */
    , mSubMac(aInstance, *this)
    , mOperationTask(aInstance, &Mac::HandleOperationTask, this)
    , mTimer(aInstance, &Mac::HandleTimer, this)
    , mOobFrame(NULL)
    , mKeyIdMode2FrameCounter(0)
    , mCcaSampleCount(0)
#if OPENTHREAD_ENABLE_MAC_FILTER
    , mFilter()
#endif
{
    ExtAddress randomExtAddress;

    randomExtAddress.GenerateRandom();

    mCcaSuccessRateTracker.Reset();
    memset(&mCounters, 0, sizeof(otMacCounters));
    memset(&mNetworkName, 0, sizeof(otNetworkName));
    memset(&mExtendedPanId, 0, sizeof(otExtendedPanId));

    mSubMac.Enable();

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);
    SetPanId(mPanId);
    SetExtAddress(randomExtAddress);
    SetShortAddress(GetShortAddress());
}

otError Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = OT_ERROR_BUSY);

    mActiveScanHandler = aHandler;

    if (aScanDuration == 0)
    {
        aScanDuration = kScanDurationDefault;
    }

    Scan(kOperationActiveScan, aScanChannels, aScanDuration, aContext);

exit:
    return error;
}

otError Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = OT_ERROR_BUSY);

    mEnergyScanHandler = aHandler;
    Scan(kOperationEnergyScan, aScanChannels, aScanDuration, aContext);

exit:
    return error;
}

void Mac::Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext)
{
    mScanContext  = aContext;
    mScanDuration = aScanDuration;
    mScanChannel  = ChannelMask::kChannelIteratorFirst;

    if (aScanChannels == 0)
    {
        aScanChannels = OT_RADIO_SUPPORTED_CHANNELS;
    }

    mScanChannelMask.SetMask(aScanChannels);
    mScanChannelMask.Intersect(mSupportedChannelMask);
    StartOperation(aScanOperation);
}

bool Mac::IsActiveScanInProgress(void)
{
    return (mOperation == kOperationActiveScan) || (mPendingActiveScan);
}

bool Mac::IsEnergyScanInProgress(void)
{
    return (mOperation == kOperationEnergyScan) || (mPendingEnergyScan);
}

bool Mac::IsInTransmitState(void)
{
    return (mOperation == kOperationTransmitData) || (mOperation == kOperationTransmitBeacon) ||
           (mOperation == kOperationTransmitOutOfBandFrame);
}

otError Mac::ConvertBeaconToActiveScanResult(Frame *aBeaconFrame, otActiveScanResult &aResult)
{
    otError        error = OT_ERROR_NONE;
    Address        address;
    Beacon *       beacon        = NULL;
    BeaconPayload *beaconPayload = NULL;
    uint8_t        payloadLength;

    memset(&aResult, 0, sizeof(otActiveScanResult));

    VerifyOrExit(aBeaconFrame != NULL, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(aBeaconFrame->GetType() == Frame::kFcfFrameBeacon, error = OT_ERROR_PARSE);
    SuccessOrExit(error = aBeaconFrame->GetSrcAddr(address));
    VerifyOrExit(address.IsExtended(), error = OT_ERROR_PARSE);
    aResult.mExtAddress = address.GetExtended();

    aBeaconFrame->GetSrcPanId(aResult.mPanId);
    aResult.mChannel = aBeaconFrame->GetChannel();
    aResult.mRssi    = aBeaconFrame->GetRssi();
    aResult.mLqi     = aBeaconFrame->GetLqi();

    payloadLength = aBeaconFrame->GetPayloadLength();

    beacon        = reinterpret_cast<Beacon *>(aBeaconFrame->GetPayload());
    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    if ((payloadLength >= (sizeof(*beacon) + sizeof(*beaconPayload))) && beacon->IsValid() && beaconPayload->IsValid())
    {
        aResult.mVersion    = beaconPayload->GetProtocolVersion();
        aResult.mIsJoinable = beaconPayload->IsJoiningPermitted();
        aResult.mIsNative   = beaconPayload->IsNative();
        memcpy(&aResult.mNetworkName, beaconPayload->GetNetworkName(), BeaconPayload::kNetworkNameSize);
        memcpy(&aResult.mExtendedPanId, beaconPayload->GetExtendedPanId(), BeaconPayload::kExtPanIdSize);
    }

    LogBeacon("Received", *beaconPayload);

exit:
    return error;
}

otError Mac::UpdateScanChannel(void)
{
    otError error;

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);

    error = mScanChannelMask.GetNextChannel(mScanChannel);

exit:
    return error;
}

void Mac::PerformActiveScan(void)
{
    if (UpdateScanChannel() == OT_ERROR_NONE)
    {
        // If there are more channels to scan, send the beacon request.
        BeginTransmit();
    }
    else
    {
        mSubMac.SetPanId(mPanId);
        FinishOperation();
        mActiveScanHandler(mScanContext, NULL);
        PerformNextOperation();
    }
}

void Mac::PerformEnergyScan(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = UpdateScanChannel());

    if (mScanDuration == 0)
    {
        while (true)
        {
            mSubMac.Receive(mScanChannel);
            ReportEnergyScanResult(mSubMac.GetRssi());
            SuccessOrExit(error = UpdateScanChannel());
        }
    }
    else
    {
        error = mSubMac.EnergyScan(mScanChannel, mScanDuration);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        FinishOperation();
        mEnergyScanHandler(mScanContext, NULL);
        PerformNextOperation();
    }
}

void Mac::ReportEnergyScanResult(int8_t aRssi)
{
    if (aRssi != kInvalidRssiValue)
    {
        otEnergyScanResult result;

        result.mChannel = mScanChannel;
        result.mMaxRssi = aRssi;

        mEnergyScanHandler(mScanContext, &result);
    }
}

void Mac::EnergyScanDone(int8_t aRssi)
{
    ReportEnergyScanResult(aRssi);
    PerformEnergyScan();
}

void Mac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    VerifyOrExit(mRxOnWhenIdle != aRxOnWhenIdle);

    mRxOnWhenIdle = aRxOnWhenIdle;

    // If the new value for `mRxOnWhenIdle` is `true` (i.e., radio should
    // remain in Rx while idle) we stop any ongoing or pending `WaitingForData`
    // operation (since this operation only applies to sleepy devices).

    if (mRxOnWhenIdle)
    {
        if (mPendingWaitingForData)
        {
            mTimer.Stop();
            mPendingWaitingForData = false;
        }

        if (mOperation == kOperationWaitingForData)
        {
            mTimer.Stop();
            FinishOperation();
            mOperationTask.Post();
        }

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
        mDelayingSleep    = false;
        mShouldDelaySleep = false;
#endif
    }

    mSubMac.SetRxOnWhenBackoff(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();

exit:
    return;
}

otError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    mSubMac.SetShortAddress(aShortAddress);
    return OT_ERROR_NONE;
}

otError Mac::SetPanChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mPanChannel != aChannel, GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_CHANNEL));

    mPanChannel = aChannel;
    mCcaSuccessRateTracker.Reset();

    VerifyOrExit(!mRadioChannelAcquisitionId);

    mRadioChannel = mPanChannel;

    UpdateIdleMode();

    GetNotifier().Signal(OT_CHANGED_THREAD_CHANNEL);

exit:
    return error;
}

otError Mac::SetRadioChannel(uint16_t aAcquisitionId, uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mRadioChannelAcquisitionId && aAcquisitionId == mRadioChannelAcquisitionId,
                 error = OT_ERROR_INVALID_STATE);

    mRadioChannel = aChannel;

    UpdateIdleMode();

exit:
    return error;
}

otError Mac::AcquireRadioChannel(uint16_t *aAcquisitionId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aAcquisitionId != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!mRadioChannelAcquisitionId, error = OT_ERROR_INVALID_STATE);

    mRadioChannelAcquisitionId = Random::GetUint16InRange(1, kMaxAcquisitionId);

    *aAcquisitionId = mRadioChannelAcquisitionId;

exit:
    return error;
}

otError Mac::ReleaseRadioChannel(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRadioChannelAcquisitionId, error = OT_ERROR_INVALID_STATE);

    mRadioChannelAcquisitionId = 0;
    mRadioChannel              = mPanChannel;

    UpdateIdleMode();

exit:
    return error;
}

void Mac::SetSupportedChannelMask(const ChannelMask &aMask)
{
    ChannelMask newMask = aMask;

    newMask.Intersect(OT_RADIO_SUPPORTED_CHANNELS);
    VerifyOrExit(newMask != mSupportedChannelMask, GetNotifier().SignalIfFirst(OT_CHANGED_SUPPORTED_CHANNEL_MASK));

    mSupportedChannelMask = newMask;
    GetNotifier().Signal(OT_CHANGED_SUPPORTED_CHANNEL_MASK);

exit:
    return;
}

otError Mac::SetNetworkName(const char *aNetworkName)
{
    return SetNetworkName(aNetworkName, OT_NETWORK_NAME_MAX_SIZE + 1);
}

otError Mac::SetNetworkName(const char *aBuffer, uint8_t aLength)
{
    otError error  = OT_ERROR_NONE;
    uint8_t newLen = static_cast<uint8_t>(strnlen(aBuffer, aLength));

    VerifyOrExit(newLen <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(newLen != strlen(mNetworkName.m8) || memcmp(mNetworkName.m8, aBuffer, newLen) != 0,
                 GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_NETWORK_NAME));

    memcpy(mNetworkName.m8, aBuffer, newLen);
    mNetworkName.m8[newLen] = 0;
    GetNotifier().Signal(OT_CHANGED_THREAD_NETWORK_NAME);

exit:
    return error;
}

otError Mac::SetPanId(PanId aPanId)
{
    VerifyOrExit(mPanId != aPanId, GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_PANID));
    mPanId = aPanId;
    mSubMac.SetPanId(mPanId);
    GetNotifier().Signal(OT_CHANGED_THREAD_PANID);

exit:
    return OT_ERROR_NONE;
}

otError Mac::SetExtendedPanId(const otExtendedPanId &aExtendedPanId)
{
    VerifyOrExit(memcmp(mExtendedPanId.m8, aExtendedPanId.m8, sizeof(mExtendedPanId)) != 0,
                 GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_EXT_PANID));

    mExtendedPanId = aExtendedPanId;
    GetNotifier().Signal(OT_CHANGED_THREAD_EXT_PANID);

exit:
    return OT_ERROR_NONE;
}

otError Mac::SendFrameRequest(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitData && (mOperation != kOperationTransmitData), error = OT_ERROR_ALREADY);

    StartOperation(kOperationTransmitData);

exit:
    return error;
}

otError Mac::SendOutOfBandFrameRequest(otRadioFrame *aOobFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aOobFrame != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitOobFrame && (mOperation != kOperationTransmitOutOfBandFrame),
                 error = OT_ERROR_ALREADY);

    mOobFrame = static_cast<Frame *>(aOobFrame);

    StartOperation(kOperationTransmitOutOfBandFrame);

exit:
    return error;
}

void Mac::UpdateIdleMode(void)
{
    bool shouldSleep = !mRxOnWhenIdle && !mPromiscuous;

    VerifyOrExit(mOperation == kOperationIdle);

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    if (mShouldDelaySleep)
    {
        mTimer.Start(kSleepDelay);
        mShouldDelaySleep = false;
        mDelayingSleep    = true;
        otLogDebgMac("Idle mode: Sleep delayed");
    }

    if (mDelayingSleep)
    {
        shouldSleep = false;
    }
#endif

    if (shouldSleep)
    {
        mSubMac.Sleep();
        otLogDebgMac("Idle mode: Radio sleeping");
    }
    else
    {
        mSubMac.Receive(mRadioChannel);
        otLogDebgMac("Idle mode: Radio receiving on channel %d", mRadioChannel);
    }

exit:
    return;
}

void Mac::StartOperation(Operation aOperation)
{
    if (aOperation != kOperationIdle)
    {
        otLogDebgMac("Request to start operation \"%s\"", OperationToString(aOperation));

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
        if (mDelayingSleep)
        {
            otLogDebgMac("Canceling sleep delay");
            mTimer.Stop();
            mDelayingSleep    = false;
            mShouldDelaySleep = false;
        }
#endif
    }

    switch (aOperation)
    {
    case kOperationIdle:
        break;

    case kOperationActiveScan:
        mPendingActiveScan = true;
        break;

    case kOperationEnergyScan:
        mPendingEnergyScan = true;
        break;

    case kOperationTransmitBeacon:
        mPendingTransmitBeacon = true;
        break;

    case kOperationTransmitData:
        mPendingTransmitData = true;
        break;

    case kOperationWaitingForData:
        mPendingWaitingForData = true;
        break;

    case kOperationTransmitOutOfBandFrame:
        mPendingTransmitOobFrame = true;
        break;
    }

    if (mOperation == kOperationIdle)
    {
        mOperationTask.Post();
    }
}

void Mac::HandleOperationTask(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Mac>().PerformNextOperation();
}

void Mac::PerformNextOperation(void)
{
    VerifyOrExit(mOperation == kOperationIdle);

    if (!mEnabled)
    {
        mPendingWaitingForData   = false;
        mPendingTransmitOobFrame = false;
        mPendingActiveScan       = false;
        mPendingEnergyScan       = false;
        mPendingTransmitBeacon   = false;
        mPendingTransmitData     = false;
        mTimer.Stop();
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
        mDelayingSleep    = false;
        mShouldDelaySleep = false;
#endif
        ExitNow();
    }

    // `WaitingForData` should be checked before any other pending
    // operations since radio should remain in receive mode after
    // a data poll ack indicating a pending frame from parent.
    if (mPendingWaitingForData)
    {
        mPendingWaitingForData = false;
        mOperation             = kOperationWaitingForData;
    }
    else if (mPendingTransmitOobFrame)
    {
        mPendingTransmitOobFrame = false;
        mOperation               = kOperationTransmitOutOfBandFrame;
    }
    else if (mPendingActiveScan)
    {
        mPendingActiveScan = false;
        mOperation         = kOperationActiveScan;
    }
    else if (mPendingEnergyScan)
    {
        mPendingEnergyScan = false;
        mOperation         = kOperationEnergyScan;
    }
    else if (mPendingTransmitBeacon)
    {
        mPendingTransmitBeacon = false;
        mOperation             = kOperationTransmitBeacon;
    }
    else if (mPendingTransmitData)
    {
        mPendingTransmitData = false;
        mOperation           = kOperationTransmitData;
    }

    if (mOperation != kOperationIdle)
    {
        otLogDebgMac("Starting operation \"%s\"", OperationToString(mOperation));
    }

    switch (mOperation)
    {
    case kOperationIdle:
        UpdateIdleMode();
        break;

    case kOperationActiveScan:
        PerformActiveScan();
        break;

    case kOperationEnergyScan:
        PerformEnergyScan();
        break;

    case kOperationTransmitBeacon:
    case kOperationTransmitData:
    case kOperationTransmitOutOfBandFrame:
        BeginTransmit();
        break;

    case kOperationWaitingForData:
        mSubMac.Receive(mRadioChannel);
        break;
    }

exit:
    return;
}

void Mac::FinishOperation(void)
{
    otLogDebgMac("Finishing operation \"%s\"", OperationToString(mOperation));
    mOperation = kOperationIdle;
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
    otLogInfoMac("Sending Beacon Request");
}

void Mac::SendBeacon(Frame &aFrame)
{
    uint8_t        numUnsecurePorts;
    uint8_t        beaconLength;
    uint16_t       fcf;
    Beacon *       beacon        = NULL;
    BeaconPayload *beaconPayload = NULL;

    // initialize MAC header
    fcf = Frame::kFcfFrameBeacon | Frame::kFcfDstAddrNone | Frame::kFcfSrcAddrExt;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetSrcPanId(mPanId);
    aFrame.SetSrcAddr(GetExtAddress());

    // write payload
    beacon = reinterpret_cast<Beacon *>(aFrame.GetPayload());
    beacon->Init();
    beaconLength = sizeof(*beacon);

    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    if (GetNetif().GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_BEACONS)
    {
        beaconPayload->Init();

        // set the Joining Permitted flag
        GetNetif().GetIp6Filter().GetUnsecurePorts(numUnsecurePorts);

        if (numUnsecurePorts)
        {
            beaconPayload->SetJoiningPermitted();
        }
        else
        {
            beaconPayload->ClearJoiningPermitted();
        }

        beaconPayload->SetNetworkName(mNetworkName.m8);
        beaconPayload->SetExtendedPanId(mExtendedPanId.m8);

        beaconLength += sizeof(*beaconPayload);
    }

    aFrame.SetPayloadLength(beaconLength);

    LogBeacon("Sending", *beaconPayload);
}

bool Mac::ShouldSendBeacon(void) const
{
    bool shouldSend = false;

    VerifyOrExit(mEnabled);

    shouldSend = IsBeaconEnabled();

#if OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE
    if (!shouldSend)
    {
        // When `ENABLE_BEACON_RSP_WHEN_JOINABLE` feature is enabled,
        // the device should transmit IEEE 802.15.4 Beacons in response
        // to IEEE 802.15.4 Beacon Requests even while the device is not
        // router capable and detached (i.e., `IsBeaconEnabled()` is
        // false) but only if it is in joinable state (unsecure port
        // list is not empty).

        uint8_t numUnsecurePorts;

        GetNetif().GetIp6Filter().GetUnsecurePorts(numUnsecurePorts);
        shouldSend = (numUnsecurePorts != 0);
    }
#endif

exit:
    return shouldSend;
}

void Mac::ProcessTransmitAesCcm(Frame &aFrame, const ExtAddress *aExtAddress)
{
    uint32_t       frameCounter = 0;
    uint8_t        securityLevel;
    uint8_t        nonce[kNonceSize];
    uint8_t        tagLength;
    Crypto::AesCcm aesCcm;
    otError        error;

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);

    GenerateNonce(*aExtAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(aFrame.GetAesKey(), 16);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    error = aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    assert(error == OT_ERROR_NONE);

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), true);
    aesCcm.Finalize(aFrame.GetFooter(), &tagLength);
}

void Mac::ProcessTransmitSecurity(Frame &aFrame, bool aProcessAesCcm)
{
    KeyManager &      keyManager = GetNetif().GetKeyManager();
    uint8_t           keyIdMode;
    const ExtAddress *extAddress = NULL;

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        aFrame.SetAesKey(keyManager.GetKek());
        extAddress = &GetExtAddress();

        if (!aFrame.IsARetransmission())
        {
            aFrame.SetFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    case Frame::kKeyIdMode1:
        aFrame.SetAesKey(keyManager.GetCurrentMacKey());
        extAddress = &GetExtAddress();

        // If the frame is marked as a retransmission, `MeshForwarder` which
        // prepared the frame should set the frame counter and key id to the
        // same values used in the earlier transmit attempt. For a new frame (not
        // a retransmission), we get a new frame counter and key id from the key
        // manager.

        if (!aFrame.IsARetransmission())
        {
            aFrame.SetFrameCounter(keyManager.GetMacFrameCounter());
            keyManager.IncrementMacFrameCounter();
            aFrame.SetKeyId((keyManager.GetCurrentKeySequence() & 0x7f) + 1);
        }

        break;

    case Frame::kKeyIdMode2:
    {
        const uint8_t keySource[] = {0xff, 0xff, 0xff, 0xff};
        aFrame.SetAesKey(sMode2Key);
        mKeyIdMode2FrameCounter++;
        aFrame.SetFrameCounter(mKeyIdMode2FrameCounter);
        aFrame.SetKeySource(keySource);
        aFrame.SetKeyId(0xff);
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;
    }

    default:
        assert(false);
        break;
    }

    if (aProcessAesCcm)
    {
        ProcessTransmitAesCcm(aFrame, extAddress);
    }

exit:
    return;
}

void Mac::BeginTransmit(void)
{
    otError error                 = OT_ERROR_NONE;
    bool    applyTransmitSecurity = true;
    bool    processTransmitAesCcm = true;
    Frame & sendFrame             = mSubMac.GetTransmitFrame();
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    uint8_t timeIeOffset = 0;
#endif

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);

    switch (mOperation)
    {
    case kOperationActiveScan:
        mSubMac.SetPanId(kPanIdBroadcast);
        sendFrame.SetChannel(mScanChannel);
        SendBeaconRequest(sendFrame);
        sendFrame.SetSequence(0);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(kMaxFrameRetriesDirect);
        break;

    case kOperationTransmitBeacon:
        sendFrame.SetChannel(mRadioChannel);
        SendBeacon(sendFrame);
        sendFrame.SetSequence(mBeaconSequence++);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(kMaxFrameRetriesDirect);
        break;

    case kOperationTransmitData:
        sendFrame.SetChannel(mRadioChannel);

        SuccessOrExit(error = GetNetif().GetMeshForwarder().HandleFrameRequest(sendFrame));

        // If the frame is marked as a retransmission, then data sequence number is already set.
        if (!sendFrame.IsARetransmission())
        {
            sendFrame.SetSequence(mDataSequence);
        }

        break;

    case kOperationTransmitOutOfBandFrame:
        sendFrame.CopyFrom(*mOobFrame);
        applyTransmitSecurity = false;
        break;

    default:
        assert(false);
        break;
    }

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    timeIeOffset = GetTimeIeOffset(sendFrame);
    sendFrame.SetTimeIeOffset(timeIeOffset);

    if (timeIeOffset != 0)
    {
        // Transmit security will be processed after time IE content is updated.
        processTransmitAesCcm = false;
        sendFrame.SetTimeSyncSeq(GetNetif().GetTimeSync().GetTimeSyncSeq());
        sendFrame.SetNetworkTimeOffset(GetNetif().GetTimeSync().GetNetworkTimeOffset());
    }
#endif

    if (applyTransmitSecurity)
    {
        // Security Processing
        ProcessTransmitSecurity(sendFrame, processTransmitAesCcm);
    }

    mBroadcastTransmitCount = 0;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    if (!mRxOnWhenIdle && !mPromiscuous)
    {
        mShouldDelaySleep = sendFrame.GetFramePending();
        otLogDebgMac("Delay sleep for pending tx");
    }
#endif

    error = mSubMac.Send();

exit:

    if (error != OT_ERROR_NONE)
    {
        HandleTransmitDone(sendFrame, NULL, OT_ERROR_ABORT);
    }
}

void Mac::RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel)
{
    if (!aCcaSuccess)
    {
        mCounters.mTxErrCca++;
    }

    // Only track the CCA success rate for frame transmissions
    // on the PAN channel.

    if (aChannel == mPanChannel)
    {
        if (mCcaSampleCount < kMaxCcaSampleCount)
        {
            mCcaSampleCount++;
        }

        mCcaSuccessRateTracker.AddSample(aCcaSuccess, mCcaSampleCount);
    }
}

void Mac::RecordFrameTransmitStatus(const Frame &aFrame,
                                    const Frame *aAckFrame,
                                    otError      aError,
                                    uint8_t      aRetryCount,
                                    bool         aWillRetx)
{
    bool      ackRequested = aFrame.GetAckRequest();
    Address   dstAddr;
    Neighbor *neighbor;

    aFrame.GetDstAddr(dstAddr);
    neighbor = GetNetif().GetMle().GetNeighbor(dstAddr);

#if OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING

    // Record frame transmission success/failure state (for a neighbor).

    if ((neighbor != NULL) && ackRequested)
    {
        bool frameTxSuccess = true;

        // CCA or abort errors are excluded from frame tx error
        // rate tracking, since when they occur, the frame is
        // not actually sent over the air.

        switch (aError)
        {
        case OT_ERROR_NO_ACK:
            frameTxSuccess = false;

            // Fall through

        case OT_ERROR_NONE:
            neighbor->GetLinkInfo().AddFrameTxStatus(frameTxSuccess);
            break;

        default:
            break;
        }
    }

#endif // OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING

    // Log frame transmission failure.

    if (aError != OT_ERROR_NONE)
    {
        LogFrameTxFailure(aFrame, aError, aRetryCount);
        otDumpDebgMac("TX ERR", aFrame.GetHeader(), 16);

        if (aWillRetx)
        {
            mCounters.mTxRetry++;

            // Since this failed transmission will be retried by `SubMac` layer
            // there is no need to update any other MAC counter. MAC counters
            // are updated on the final transmission attempt.

            ExitNow();
        }
    }

    // Update neighbor's RSSI link info from the received Ack.

    if ((aError == OT_ERROR_NONE) && ackRequested && (aAckFrame != NULL) && (neighbor != NULL))
    {
        neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), aAckFrame->GetRssi());
    }

    // Update MAC counters.

    mCounters.mTxTotal++;

    if (aError == OT_ERROR_ABORT)
    {
        mCounters.mTxErrAbort++;
    }

    if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
    {
        mCounters.mTxErrBusyChannel++;
    }

    if (ackRequested)
    {
        mCounters.mTxAckRequested++;

        if (aError == OT_ERROR_NONE)
        {
            mCounters.mTxAcked++;
        }
    }
    else
    {
        mCounters.mTxNoAckRequested++;
    }

    if (dstAddr.IsBroadcast())
    {
        mCounters.mTxBroadcast++;
    }
    else
    {
        mCounters.mTxUnicast++;
    }

exit:
    return;
}

void Mac::HandleTransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError)
{
    Address dstAddr;

    // Determine whether to re-transmit a broadcast frame.

    aFrame.GetDstAddr(dstAddr);

    if (dstAddr.IsBroadcast())
    {
        mBroadcastTransmitCount++;

        if (mBroadcastTransmitCount < kTxNumBcast)
        {
            mSubMac.Send();
            ExitNow();
        }

        mBroadcastTransmitCount = 0;
    }

    // Determine next action based on current operation.

    switch (mOperation)
    {
    case kOperationActiveScan:
        mCounters.mTxBeaconRequest++;
        mTimer.Start(mScanDuration);
        break;

    case kOperationTransmitBeacon:
        mCounters.mTxBeacon++;
        FinishOperation();
        PerformNextOperation();
        break;

    case kOperationTransmitData:
        if (aFrame.IsDataRequestCommand())
        {
            if (mEnabled && (aError == OT_ERROR_NONE) && aFrame.GetAckRequest() && (aAckFrame != NULL) &&
                aAckFrame->GetFramePending())
            {
                mTimer.Start(kDataPollTimeout);
                StartOperation(kOperationWaitingForData);
            }

            mCounters.mTxDataPoll++;
        }
        else
        {
            mCounters.mTxData++;
        }

        if (!aFrame.IsARetransmission())
        {
            mDataSequence++;
        }

        otDumpDebgMac("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        GetNetif().GetMeshForwarder().HandleSentFrame(aFrame, aError);
        PerformNextOperation();
        break;

    case kOperationTransmitOutOfBandFrame:
        FinishOperation();
        PerformNextOperation();
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

void Mac::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mac>().HandleTimer();
}

void Mac::HandleTimer(void)
{
    switch (mOperation)
    {
    case kOperationActiveScan:
        PerformActiveScan();
        break;

    case kOperationWaitingForData:
        otLogDebgMac("Data poll timeout");
        FinishOperation();
        GetNetif().GetMeshForwarder().GetDataPollManager().HandlePollTimeout();
        PerformNextOperation();
        break;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    case kOperationIdle:
        if (mDelayingSleep)
        {
            otLogDebgMac("Sleep delay timeout expired");
            mDelayingSleep = false;
            UpdateIdleMode();
        }

        break;
#endif

    default:
        assert(false);
        break;
    }
}

otError Mac::ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager &      keyManager = GetNetif().GetKeyManager();
    otError           error      = OT_ERROR_NONE;
    uint8_t           securityLevel;
    uint8_t           keyIdMode;
    uint32_t          frameCounter;
    uint8_t           nonce[kNonceSize];
    uint8_t           tag[Frame::kMaxMicSize];
    uint8_t           tagLength;
    uint8_t           keyid;
    uint32_t          keySequence = 0;
    const uint8_t *   macKey;
    const ExtAddress *extAddress;
    Crypto::AesCcm    aesCcm;

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
        VerifyOrExit((macKey = keyManager.GetKek()) != NULL, error = OT_ERROR_SECURITY);
        extAddress = &aSrcAddr.GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != NULL, error = OT_ERROR_SECURITY);

        aFrame.GetKeyId(keyid);
        keyid--;

        if (keyid == (keyManager.GetCurrentKeySequence() & 0x7f))
        {
            // same key index
            keySequence = keyManager.GetCurrentKeySequence();
            macKey      = keyManager.GetCurrentMacKey();
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            // previous key index
            keySequence = keyManager.GetCurrentKeySequence() - 1;
            macKey      = keyManager.GetTemporaryMacKey(keySequence);
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            // next key index
            keySequence = keyManager.GetCurrentKeySequence() + 1;
            macKey      = keyManager.GetTemporaryMacKey(keySequence);
        }
        else
        {
            ExitNow(error = OT_ERROR_SECURITY);
        }

        // If the frame is from a neighbor not in valid state (e.g., it is from a child being
        // restored), skip the key sequence and frame counter checks but continue to verify
        // the tag/MIC. Such a frame is later filtered in `RxDoneTask` which only allows MAC
        // Data Request frames from a child being restored.

        if (aNeighbor->GetState() == Neighbor::kStateValid)
        {
            if (keySequence < aNeighbor->GetKeySequence())
            {
                ExitNow(error = OT_ERROR_SECURITY);
            }
            else if (keySequence == aNeighbor->GetKeySequence())
            {
                if ((frameCounter + 1) < aNeighbor->GetLinkFrameCounter())
                {
                    ExitNow(error = OT_ERROR_SECURITY);
                }
                else if ((frameCounter + 1) == aNeighbor->GetLinkFrameCounter())
                {
                    // drop duplicated frames
                    ExitNow(error = OT_ERROR_DUPLICATED);
                }
            }
        }

        extAddress = &aSrcAddr.GetExtended();

        break;

    case Frame::kKeyIdMode2:
        macKey     = sMode2Key;
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;

    default:
        ExitNow(error = OT_ERROR_SECURITY);
        break;
    }

    GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.SetKey(macKey, 16);

    error = aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    VerifyOrExit(error == OT_ERROR_NONE, error = OT_ERROR_SECURITY);

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#else
    // for fuzz tests, execute AES but do not alter the payload
    uint8_t fuzz[OT_RADIO_FRAME_MAX_SIZE];
    aesCcm.Payload(fuzz, aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#endif
    aesCcm.Finalize(tag, &tagLength);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(memcmp(tag, aFrame.GetFooter(), tagLength) == 0, error = OT_ERROR_SECURITY);
#endif

    if ((keyIdMode == Frame::kKeyIdMode1) && (aNeighbor->GetState() == Neighbor::kStateValid))
    {
        if (aNeighbor->GetKeySequence() != keySequence)
        {
            aNeighbor->SetKeySequence(keySequence);
            aNeighbor->SetMleFrameCounter(0);
        }

        aNeighbor->SetLinkFrameCounter(frameCounter + 1);

        if (keySequence > keyManager.GetCurrentKeySequence())
        {
            keyManager.SetCurrentKeySequence(keySequence);
        }
    }

exit:
    return error;
}

void Mac::HandleReceivedFrame(Frame *aFrame, otError aError)
{
    ThreadNetif &netif = GetNetif();
    Address      srcaddr;
    Address      dstaddr;
    PanId        panid;
    Neighbor *   neighbor;
    bool         receive = false;
    otError      error   = aError;
#if OPENTHREAD_ENABLE_MAC_FILTER
    int8_t rssi = OT_MAC_FILTER_FIXED_RSS_DISABLED;
#endif // OPENTHREAD_ENABLE_MAC_FILTER

    mCounters.mRxTotal++;

    VerifyOrExit(error == OT_ERROR_NONE);
    VerifyOrExit(aFrame != NULL, error = OT_ERROR_NO_FRAME_RECEIVED);
    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    aFrame->GetSrcAddr(srcaddr);
    aFrame->GetDstAddr(dstaddr);
    neighbor = netif.GetMle().GetNeighbor(srcaddr);

    // Destination Address Filtering
    switch (dstaddr.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        aFrame->GetDstPanId(panid);
        VerifyOrExit((panid == kShortAddrBroadcast || panid == mPanId) &&
                         ((mRxOnWhenIdle && dstaddr.IsBroadcast()) || dstaddr.GetShort() == GetShortAddress()),
                     error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);

        // Allow  multicasts from neighbor routers if FTD
        if (neighbor == NULL && dstaddr.IsBroadcast() && netif.GetMle().IsFullThreadDevice())
        {
            neighbor = netif.GetMle().GetRxOnlyNeighborRouter(srcaddr);
        }

        break;

    case Address::kTypeExtended:
        aFrame->GetDstPanId(panid);
        VerifyOrExit(panid == mPanId && dstaddr.GetExtended() == GetExtAddress(),
                     error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);
        break;
    }

    // Source Address Filtering
    switch (srcaddr.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        otLogDebgMac("Received frame from short address 0x%04x", srcaddr.GetShort());

        if (neighbor == NULL)
        {
            ExitNow(error = OT_ERROR_UNKNOWN_NEIGHBOR);
        }

        srcaddr.SetExtended(neighbor->GetExtAddress());

        // fall through

    case Address::kTypeExtended:

        // Duplicate Address Protection
        if (srcaddr.GetExtended() == GetExtAddress())
        {
            ExitNow(error = OT_ERROR_INVALID_SOURCE_ADDRESS);
        }

#if OPENTHREAD_ENABLE_MAC_FILTER

        // Source filter Processing. Check if filtered out by whitelist or blacklist.
        SuccessOrExit(error = mFilter.Apply(srcaddr.GetExtended(), rssi));

        // override with the rssi in setting
        if (rssi != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            aFrame->SetRssi(rssi);
        }

#endif // OPENTHREAD_ENABLE_MAC_FILTER

        break;
    }

    // Increment counters
    if (dstaddr.IsBroadcast())
    {
        // Broadcast frame
        mCounters.mRxBroadcast++;
    }
    else
    {
        // Unicast frame
        mCounters.mRxUnicast++;
    }

    // Security Processing
    error = ProcessReceiveSecurity(*aFrame, srcaddr, neighbor);

    switch (error)
    {
    case OT_ERROR_DUPLICATED:

        // Allow a duplicate received frame pass, only if the
        // current operation is `kOperationWaitingForData` (i.e.,
        // the sleepy device is waiting to receive a frame after
        // a data poll ack from parent indicating there is a
        // pending frame for it). This ensures that the sleepy
        // device goes to sleep faster and avoids a data poll
        // timeout.
        //
        // Note that `error` is checked again later after the
        // operation `kOperationWaitingForData` is processed
        // so the duplicate frame will not be passed to next
        // layer (`MeshForwarder`).

        VerifyOrExit(mOperation == kOperationWaitingForData);

        // Fall through

    case OT_ERROR_NONE:
        break;

    default:
        ExitNow();
    }

    netif.GetMeshForwarder().GetDataPollManager().CheckFramePending(*aFrame);

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT

    if (aFrame->GetVersion() == Frame::kFcfFrameVersion2015)
    {
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
        ProcessTimeIe(*aFrame);
#endif
    }

#endif // OPENTHREAD_CONFIG_HEADER_IE_SUPPORT

    if (neighbor != NULL)
    {
#if OPENTHREAD_ENABLE_MAC_FILTER

        // make assigned rssi to take effect quickly
        if (rssi != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            neighbor->GetLinkInfo().Clear();
        }

#endif // OPENTHREAD_ENABLE_MAC_FILTER

        neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), aFrame->GetRssi());

        if (aFrame->GetSecurityEnabled() == true)
        {
            switch (neighbor->GetState())
            {
            case Neighbor::kStateValid:
                break;

            case Neighbor::kStateRestored:
            case Neighbor::kStateChildUpdateRequest:

                // Only accept a "MAC Data Request" frame from a child being restored.
                VerifyOrExit(aFrame->IsDataRequestCommand(), error = OT_ERROR_DROP);
                break;

            default:
                ExitNow(error = OT_ERROR_UNKNOWN_NEIGHBOR);
            }
        }
    }

    switch (mOperation)
    {
    case kOperationActiveScan:

        if (aFrame->GetType() == Frame::kFcfFrameBeacon)
        {
            mCounters.mRxBeacon++;
            mActiveScanHandler(mScanContext, aFrame);
            ExitNow();
        }

        // Fall-through

    case kOperationEnergyScan:

        // We can possibly receive a data frame while either active or
        // energy scan is ongoing. We continue to process the frame only
        // if the current scan channel matches `mPanChannel`.

        VerifyOrExit(mScanChannel == mPanChannel, mCounters.mRxOther++);
        break;

    case kOperationWaitingForData:

        if (!dstaddr.IsNone())
        {
            mTimer.Stop();

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
            if (!mRxOnWhenIdle && !mPromiscuous && aFrame->GetFramePending())
            {
                mShouldDelaySleep = true;
                otLogDebgMac("Delay sleep for pending rx");
            }
#endif
            FinishOperation();
            PerformNextOperation();
        }

        SuccessOrExit(error);

        break;

    default:
        break;
    }

    switch (aFrame->GetType())
    {
    case Frame::kFcfFrameMacCmd:
        if (HandleMacCommand(*aFrame) == OT_ERROR_DROP)
        {
            ExitNow(error = OT_ERROR_NONE);
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
        netif.GetMeshForwarder().HandleReceivedFrame(*aFrame);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        LogFrameRxFailure(aFrame, error);

        switch (error)
        {
        case OT_ERROR_SECURITY:
            mCounters.mRxErrSec++;
            break;

        case OT_ERROR_FCS:
            mCounters.mRxErrFcs++;
            break;

        case OT_ERROR_NO_FRAME_RECEIVED:
            mCounters.mRxErrNoFrame++;
            break;

        case OT_ERROR_UNKNOWN_NEIGHBOR:
            mCounters.mRxErrUnknownNeighbor++;
            break;

        case OT_ERROR_INVALID_SOURCE_ADDRESS:
            mCounters.mRxErrInvalidSrcAddr++;
            break;

        case OT_ERROR_ADDRESS_FILTERED:
            mCounters.mRxAddressFiltered++;
            break;

        case OT_ERROR_DESTINATION_ADDRESS_FILTERED:
            mCounters.mRxDestAddrFiltered++;
            break;

        case OT_ERROR_DUPLICATED:
            mCounters.mRxDuplicated++;
            break;

        default:
            mCounters.mRxErrOther++;
            break;
        }
    }
}

otError Mac::HandleMacCommand(Frame &aFrame)
{
    otError error = OT_ERROR_NONE;
    uint8_t commandId;

    aFrame.GetCommandId(commandId);

    switch (commandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        otLogInfoMac("Received Beacon Request");

        if (ShouldSendBeacon())
        {
            StartOperation(kOperationTransmitBeacon);
        }

        ExitNow(error = OT_ERROR_DROP);

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
    mSubMac.SetPcapCallback(aPcapCallback, aCallbackContext);
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    mPromiscuous = aPromiscuous;
    otPlatRadioSetPromiscuous(&GetInstance(), aPromiscuous);

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    mDelayingSleep    = false;
    mShouldDelaySleep = false;
#endif

    mSubMac.SetRxOnWhenBackoff(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();
}

otError Mac::SetEnabled(bool aEnable)
{
    mEnabled = aEnable;

    return OT_ERROR_NONE;
}

void Mac::ResetCounters(void)
{
    memset(&mCounters, 0, sizeof(mCounters));
}

int8_t Mac::GetNoiseFloor(void)
{
    return otPlatRadioGetReceiveSensitivity(&GetInstance());
}

const char *Mac::OperationToString(Operation aOperation)
{
    const char *retval = "";

    switch (aOperation)
    {
    case kOperationIdle:
        retval = "Idle";
        break;

    case kOperationActiveScan:
        retval = "ActiveScan";
        break;

    case kOperationEnergyScan:
        retval = "EnergyScan";
        break;

    case kOperationTransmitBeacon:
        retval = "TransmitBeacon";
        break;

    case kOperationTransmitData:
        retval = "TransmitData";
        break;

    case kOperationWaitingForData:
        retval = "WaitingForData";
        break;

    case kOperationTransmitOutOfBandFrame:
        retval = "TransmitOobFrame";
        break;
    }

    return retval;
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void Mac::LogFrameRxFailure(const Frame *aFrame, otError aError) const
{
    otLogLevel logLevel;

    switch (aError)
    {
    case OT_ERROR_ABORT:
    case OT_ERROR_NO_FRAME_RECEIVED:
    case OT_ERROR_DESTINATION_ADDRESS_FILTERED:
        logLevel = OT_LOG_LEVEL_DEBG;
        break;

    default:
        logLevel = OT_LOG_LEVEL_INFO;
        break;
    }

    if (aFrame == NULL)
    {
        otLogMac(logLevel, "Frame rx failed, error:%s", otThreadErrorToString(aError));
    }
    else
    {
        otLogMac(logLevel, "Frame rx failed, error:%s, %s", otThreadErrorToString(aError),
                 aFrame->ToInfoString().AsCString());
    }
}

void Mac::LogFrameTxFailure(const Frame &aFrame, otError aError, uint8_t aRetryCount) const
{
    otLogInfoMac("Frame tx failed, error:%s, retries:%d/%d, %s", otThreadErrorToString(aError), aRetryCount,
                 aFrame.GetMaxFrameRetries(), aFrame.ToInfoString().AsCString());
}

void Mac::LogBeacon(const char *aActionText, const BeaconPayload &aBeaconPayload) const
{
    otLogInfoMac("%s Beacon, %s", aActionText, aBeaconPayload.ToInfoString().AsCString());
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void Mac::LogFrameRxFailure(const Frame *, otError) const
{
}

void Mac::LogBeacon(const char *, const BeaconPayload &) const
{
}

void Mac::LogFrameTxFailure(const Frame &, otError, uint8_t) const
{
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
void Mac::ProcessTimeIe(Frame &aFrame)
{
    TimeIe *timeIe = reinterpret_cast<TimeIe *>(aFrame.GetTimeIe());

    VerifyOrExit(timeIe != NULL);

    aFrame.SetNetworkTimeOffset(static_cast<int64_t>(timeIe->GetTime()) - static_cast<int64_t>(aFrame.GetTimestamp()));
    aFrame.SetTimeSyncSeq(timeIe->GetSequence());

exit:
    return;
}

uint8_t Mac::GetTimeIeOffset(Frame &aFrame)
{
    uint8_t  offset = 0;
    uint8_t *base   = aFrame.GetPsdu();
    uint8_t *cur    = NULL;

    cur = aFrame.GetTimeIe();
    VerifyOrExit(cur != NULL);

    cur += sizeof(VendorIeHeader);
    offset = static_cast<uint8_t>(cur - base);

exit:
    return offset;
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

} // namespace Mac
} // namespace ot
