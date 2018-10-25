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

#ifdef _WIN32
const uint32_t kMinBackoffSum = kMinBackoff + (kUnitBackoffPeriod * OT_RADIO_SYMBOL_TIME * (1 << kMinBE)) / 1000;
const uint32_t kMaxBackoffSum = kMinBackoff + (kUnitBackoffPeriod * OT_RADIO_SYMBOL_TIME * (1 << kMaxBE)) / 1000;
static_assert(kMinBackoffSum > 0, "The min backoff value should be greater than zero!");
#endif

uint8_t ChannelMask::GetNumberOfChannels(void) const
{
    uint8_t num     = 0;
    uint8_t channel = kChannelIteratorFirst;

    while (GetNextChannel(channel) == OT_ERROR_NONE)
    {
        num++;
    }

    return num;
}

otError ChannelMask::GetNextChannel(uint8_t &aChannel) const
{
    otError error = OT_ERROR_NOT_FOUND;

    if (aChannel == kChannelIteratorFirst)
    {
        aChannel = (OT_RADIO_CHANNEL_MIN - 1);
    }

    for (aChannel++; aChannel <= OT_RADIO_CHANNEL_MAX; aChannel++)
    {
        if (ContainsChannel(aChannel))
        {
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

ChannelMask::InfoString ChannelMask::ToString(void) const
{
    InfoString string;
    uint8_t    channel  = kChannelIteratorFirst;
    bool       addComma = false;
    otError    error;

    string.Append("{");

    error = GetNextChannel(channel);

    while (error == OT_ERROR_NONE)
    {
        uint8_t rangeStart = channel;
        uint8_t rangeEnd   = channel;

        while ((error = GetNextChannel(channel)) == OT_ERROR_NONE)
        {
            if (channel != rangeEnd + 1)
            {
                break;
            }

            rangeEnd = channel;
        }

        string.Append("%s%d", addComma ? ", " : " ", rangeStart);
        addComma = true;

        if (rangeStart < rangeEnd)
        {
            string.Append("%s%d", rangeEnd == rangeStart + 1 ? ", " : "-", rangeEnd);
        }
    }

    string.Append("}");

    return string;
}

Mac::Mac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mOperation(kOperationIdle)
    , mPendingActiveScan(false)
    , mPendingEnergyScan(false)
    , mPendingTransmitBeacon(false)
    , mPendingTransmitData(false)
    , mPendingTransmitOobFrame(false)
    , mPendingWaitingForData(false)
    , mRxOnWhenIdle(false)
    , mBeaconsEnabled(false)
    , mTransmitAborted(false)
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    , mDelaySleep(false)
#endif
    , mOperationTask(aInstance, &Mac::HandleOperationTask, this)
    , mMacTimer(aInstance, &Mac::HandleMacTimer, this)
    , mBackoffTimer(aInstance, &Mac::HandleBackoffTimer, this)
    , mReceiveTimer(aInstance, &Mac::HandleReceiveTimer, this)
    , mShortAddress(kShortAddrInvalid)
    , mPanId(kPanIdBroadcast)
    , mPanChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannelAcquisitionId(0)
    , mSupportedChannelMask(OT_RADIO_SUPPORTED_CHANNELS)
    , mSendHead(NULL)
    , mSendTail(NULL)
    , mReceiveHead(NULL)
    , mReceiveTail(NULL)
    , mBeaconSequence(Random::GetUint8())
    , mDataSequence(Random::GetUint8())
    , mCsmaBackoffs(0)
    , mTransmitRetries(0)
    , mBroadcastTransmitCount(0)
    , mScanChannelMask()
    , mScanDuration(0)
    , mScanChannel(OT_RADIO_CHANNEL_MIN)
    , mEnergyScanCurrentMaxRssi(kInvalidRssiValue)
    , mScanContext(NULL)
    , mActiveScanHandler(NULL)
    , // Initialize `mActiveScanHandler` and `mEnergyScanHandler` union
    mPcapCallback(NULL)
    , mPcapCallbackContext(NULL)
#if OPENTHREAD_ENABLE_MAC_FILTER
    , mFilter()
#endif // OPENTHREAD_ENABLE_MAC_FILTER
    , mTxFrame(static_cast<Frame *>(otPlatRadioGetTransmitBuffer(&aInstance)))
    , mOobFrame(NULL)
    , mKeyIdMode2FrameCounter(0)
    , mCcaSampleCount(0)
    , mEnabled(true)
{
    GenerateExtAddress(&mExtAddress);
    mCcaSuccessRateTracker.Reset();
    memset(&mCounters, 0, sizeof(otMacCounters));

    otPlatRadioEnable(&GetInstance());

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);
    SetPanId(mPanId);
    SetExtAddress(mExtAddress);
    SetShortAddress(mShortAddress);
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
        memcpy(&aResult.mNetworkName, beaconPayload->GetNetworkName(), sizeof(aResult.mNetworkName));
        memcpy(&aResult.mExtendedPanId, beaconPayload->GetExtendedPanId(), sizeof(aResult.mExtendedPanId));
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
        // If there are more channels to scan, start CSMA backoff to send the beacon request.
        StartCsmaBackoff();
    }
    else
    {
        otPlatRadioSetPanId(&GetInstance(), mPanId);
        mActiveScanHandler(mScanContext, NULL);
        FinishOperation();
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
            int8_t rssi;

            RadioReceive(mScanChannel);
            rssi = otPlatRadioGetRssi(&GetInstance());
            ReportEnergyScanResult(rssi);
            SuccessOrExit(error = UpdateScanChannel());
        }
    }
    else if ((otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_ENERGY_SCAN) == 0)
    {
        RadioReceive(mScanChannel);
        mEnergyScanCurrentMaxRssi = kInvalidRssiValue;
        mMacTimer.Start(mScanDuration);
        mBackoffTimer.Start(kEnergyScanRssiSampleInterval);
        SampleRssi();
    }
    else
    {
        SuccessOrExit(error = otPlatRadioEnergyScan(&GetInstance(), mScanChannel, mScanDuration));
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        mEnergyScanHandler(mScanContext, NULL);
        FinishOperation();
    }
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(instance->IsInitialized());

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (instance->GetLinkRaw().IsEnabled())
    {
        instance->GetLinkRaw().InvokeEnergyScanDone(aEnergyScanMaxRssi);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        instance->GetThreadNetif().GetMac().EnergyScanDone(aEnergyScanMaxRssi);
    }

exit:
    return;
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

void Mac::SampleRssi(void)
{
    int8_t rssi;

    rssi = otPlatRadioGetRssi(&GetInstance());

    if (rssi != kInvalidRssiValue)
    {
        if ((mEnergyScanCurrentMaxRssi == kInvalidRssiValue) || (rssi > mEnergyScanCurrentMaxRssi))
        {
            mEnergyScanCurrentMaxRssi = rssi;
        }
    }
}

otError Mac::RegisterReceiver(Receiver &aReceiver)
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
        mReceiveTail        = &aReceiver;
    }

    return OT_ERROR_NONE;
}

void Mac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    VerifyOrExit(mRxOnWhenIdle != aRxOnWhenIdle);

    mRxOnWhenIdle = aRxOnWhenIdle;

    // If the new value for `mRxOnWhenIdle` is `true` (i.e., radio should
    // remain in Rx while idle) we stop any ongoing or pending `WaitinForData`
    // operation (since this operation only applies to sleepy devices).

    if (mRxOnWhenIdle)
    {
        mPendingWaitingForData = false;

        if (mOperation == kOperationWaitingForData)
        {
            mReceiveTimer.Stop();
            FinishOperation();
        }
    }

    UpdateIdleMode();

exit:
    return;
}

void Mac::GenerateExtAddress(ExtAddress *aExtAddress)
{
    Random::FillBuffer(aExtAddress->m8, sizeof(ExtAddress));

    aExtAddress->SetGroup(false);
    aExtAddress->SetLocal(true);
}

void Mac::SetExtAddress(const ExtAddress &aExtAddress)
{
    otExtAddress address;

    for (size_t i = 0; i < sizeof(address); i++)
    {
        address.m8[i] = aExtAddress.m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(&GetInstance(), &address);
    mExtAddress = aExtAddress;
}

otError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    otPlatRadioSetShortAddress(&GetInstance(), aShortAddress);

    return OT_ERROR_NONE;
}

otError Mac::SetPanChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(OT_RADIO_CHANNEL_MIN <= aChannel && aChannel <= OT_RADIO_CHANNEL_MAX, error = OT_ERROR_INVALID_ARGS);
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

    VerifyOrExit(OT_RADIO_CHANNEL_MIN <= aChannel && aChannel <= OT_RADIO_CHANNEL_MAX, error = OT_ERROR_INVALID_ARGS);
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
    otError error = OT_ERROR_NONE;
    uint8_t len   = static_cast<uint8_t>(strnlen(aBuffer, aLength));

    VerifyOrExit(len <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(memcmp(mNetworkName.m8, aBuffer, len) != 0,
                 GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_NETWORK_NAME));

    memcpy(mNetworkName.m8, aBuffer, len);
    mNetworkName.m8[len] = 0;
    GetNotifier().Signal(OT_CHANGED_THREAD_NETWORK_NAME);

exit:
    return error;
}

otError Mac::SetPanId(PanId aPanId)
{
    VerifyOrExit(mPanId != aPanId, GetNotifier().SignalIfFirst(OT_CHANGED_THREAD_PANID));
    mPanId = aPanId;
    otPlatRadioSetPanId(&GetInstance(), mPanId);
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

otError Mac::SendFrameRequest(Sender &aSender)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mSendTail != &aSender && aSender.mNext == NULL, error = OT_ERROR_ALREADY);

    if (mSendHead == NULL)
    {
        mSendHead = &aSender;
        mSendTail = &aSender;
    }
    else
    {
        mSendTail->mNext = &aSender;
        mSendTail        = &aSender;
    }

    StartOperation(kOperationTransmitData);

exit:
    return error;
}

otError Mac::SendOutOfBandFrameRequest(otRadioFrame *aOobFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mOobFrame == NULL, error = OT_ERROR_ALREADY);

    mOobFrame = static_cast<Frame *>(aOobFrame);

    StartOperation(kOperationTransmitOutOfBandFrame);

exit:
    return error;
}

void Mac::UpdateIdleMode(void)
{
    VerifyOrExit(mOperation == kOperationIdle);

    if (!mRxOnWhenIdle && !mReceiveTimer.IsRunning() && !otPlatRadioGetPromiscuous(&GetInstance()))
    {
        if (RadioSleep() != OT_ERROR_INVALID_STATE)
        {
            otLogDebgMac("Idle mode: Radio sleeping");
            ExitNow();
        }

        // If `RadioSleep()` returns `OT_ERROR_INVALID_STATE`
        // indicating sleep is being delayed, continue to put
        // the radio in receive mode.
    }

    otLogDebgMac("Idle mode: Radio receiving on channel %d", mRadioChannel);
    RadioReceive(mRadioChannel);

exit:
    return;
}

void Mac::StartOperation(Operation aOperation)
{
    if (aOperation != kOperationIdle)
    {
        otLogDebgMac("Request to start operation \"%s\"", OperationToString(aOperation));
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
    aTasklet.GetOwner<Mac>().HandleOperationTask();
}

void Mac::HandleOperationTask(void)
{
    // `mOperationTask` tasklet is used for two separate purposes:
    //
    // 1) To invoke `HandleTransmitDone()` from a tasklet with
    //    `OT_ERROR_ABORT` error.
    //
    // 2) To perform a scheduled MAC operation.

    if (mTransmitAborted)
    {
        mTransmitAborted = false;
        HandleTransmitDone(GetOperationFrame(), NULL, OT_ERROR_ABORT);
    }
    else
    {
        PerformOperation();
    }
}

void Mac::PerformOperation(void)
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
        mOobFrame                = NULL;
        ExitNow();
    }

    // `WaitingForData` should be checked before any other pending
    // operations since radio should remain in receive mode after
    // a data poll ack indicating a pending frame from parent.
    if (mPendingWaitingForData)
    {
        mPendingWaitingForData = false;
        mOperation             = kOperationWaitingForData;
        RadioReceive(mRadioChannel);
    }
    else if (mPendingTransmitOobFrame)
    {
        mPendingTransmitOobFrame = false;
        mOperation               = kOperationTransmitOutOfBandFrame;
        StartCsmaBackoff();
    }
    else if (mPendingActiveScan)
    {
        mPendingActiveScan = false;
        mOperation         = kOperationActiveScan;
        PerformActiveScan();
    }
    else if (mPendingEnergyScan)
    {
        mPendingEnergyScan = false;
        mOperation         = kOperationEnergyScan;
        PerformEnergyScan();
    }
    else if (mPendingTransmitBeacon)
    {
        mPendingTransmitBeacon = false;
        mOperation             = kOperationTransmitBeacon;
        StartCsmaBackoff();
    }
    else if (mPendingTransmitData)
    {
        mPendingTransmitData = false;
        mOperation           = kOperationTransmitData;
        StartCsmaBackoff();
    }
    else
    {
        UpdateIdleMode();
    }

    if (mOperation != kOperationIdle)
    {
        otLogDebgMac("Starting operation \"%s\"", OperationToString(mOperation));
    }

exit:
    return;
}

void Mac::FinishOperation(void)
{
    // Clear the current operation and start any pending ones.

    otLogDebgMac("Finishing operation \"%s\"", OperationToString(mOperation));

    mOperation = kOperationIdle;

    // Note that we do not want to post the `mOperationTask` here and
    // instead we do a direct call to `PerformOperation()`. This helps
    // ensure that if there is no pending operation, the radio is
    // switched to idle mode immediately.

    PerformOperation();
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
    aFrame.SetSrcAddr(mExtAddress);

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
        // router capable and detached (i.e., `IsBeaconeEnabled()` is
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
        extAddress = &mExtAddress;

        if (!aFrame.IsARetransmission())
        {
            aFrame.SetFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    case Frame::kKeyIdMode1:
        aFrame.SetAesKey(keyManager.GetCurrentMacKey());
        extAddress = &mExtAddress;

        // If the frame is marked as a retransmission, the `Mac::Sender` which
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

void Mac::StartCsmaBackoff(void)
{
    uint32_t backoffExponent = kMinBE + mTransmitRetries + mCsmaBackoffs;
    uint32_t backoff;
    bool     shouldReceive;

    if (RadioSupportsCsmaBackoff())
    {
        // If the radio supports CSMA back off logic, immediately schedule the send.
        BeginTransmit();
        ExitNow();
    }
#if OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
    else if (mTransmitRetries > 0)
    {
        Frame &sendFrame(*GetOperationFrame());

        if ((sendFrame.GetMaxFrameRetries() > 0) && (sendFrame.GetMaxFrameRetries() <= mTransmitRetries))
        {
            BeginTransmit();
            ExitNow();
        }
    }
#endif

    if (backoffExponent > kMaxBE)
    {
        backoffExponent = kMaxBE;
    }

    backoff = Random::GetUint32InRange(0, 1U << backoffExponent);
    backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    // Put the radio in either sleep or receive mode depending on
    // `mRxOnWhenIdle` flag before starting the backoff timer.

    shouldReceive = (mRxOnWhenIdle || otPlatRadioGetPromiscuous(&GetInstance()));

    if (!shouldReceive)
    {
        if (RadioSleep() == OT_ERROR_INVALID_STATE)
        {
            // If `RadioSleep()` returns `OT_ERROR_INVALID_STATE`
            // indicating sleep is being delayed, the radio should
            // be put in receive mode.

            shouldReceive = true;
        }
    }

    if (shouldReceive)
    {
        switch (mOperation)
        {
        case kOperationActiveScan:
        case kOperationEnergyScan:
            RadioReceive(mScanChannel);
            break;

        default:
            RadioReceive(mRadioChannel);
            break;
        }
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mBackoffTimer.Start(backoff);
#else  // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mBackoffTimer.Start(backoff / 1000UL);
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

exit:
    return;
}

void Mac::HandleBackoffTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mac>().HandleBackoffTimer();
}

void Mac::HandleBackoffTimer(void)
{
    // The backoff timer serves two purposes:
    //
    // (a) It is used to add CSMA backoff delay before a frame transmission.
    // (b) While performing Energy Scan, it is used to add delay between
    //     RSSI samples.

    if (mOperation == kOperationEnergyScan)
    {
        SampleRssi();
        mBackoffTimer.StartAt(mBackoffTimer.GetFireTime(), kEnergyScanRssiSampleInterval);
    }
    else
    {
        BeginTransmit();
    }
}

Frame *Mac::GetOperationFrame(void)
{
    Frame *frame = NULL;

    switch (mOperation)
    {
    case kOperationTransmitOutOfBandFrame:
        frame = mOobFrame;
        break;

    default:
        frame = mTxFrame;
        break;
    }

    assert(frame != NULL);

    return frame;
}

void Mac::BeginTransmit(void)
{
    otError error                 = OT_ERROR_NONE;
    bool    applyTransmitSecurity = true;
    bool    processTransmitAesCcm = true;
    Frame & sendFrame(*GetOperationFrame());
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    uint8_t timeIeOffset = 0;
#endif

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);

    if (mCsmaBackoffs == 0 && mTransmitRetries == 0 && mBroadcastTransmitCount == 0)
    {
        switch (mOperation)
        {
        case kOperationActiveScan:
            otPlatRadioSetPanId(&GetInstance(), kPanIdBroadcast);
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
            SuccessOrExit(error = mSendHead->HandleFrameRequest(sendFrame));

            // If the frame is marked as a retransmission, then data sequence number is already set by the `Sender`.
            if (!sendFrame.IsARetransmission())
            {
                sendFrame.SetSequence(mDataSequence);
            }

            break;

        case kOperationTransmitOutOfBandFrame:
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
    }

#if OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
    if ((sendFrame.GetMaxFrameRetries() > 0) && (sendFrame.GetMaxFrameRetries() <= mTransmitRetries))
    {
        sendFrame.SetCsmaCaEnabled(false);
    }
    else
#endif
    {
        sendFrame.SetCsmaCaEnabled(true);
    }

    error = RadioReceive(sendFrame.GetChannel());
    assert(error == OT_ERROR_NONE);

    error = RadioTransmit(&sendFrame);
    assert(error == OT_ERROR_NONE);

    if (mPcapCallback)
    {
        sendFrame.SetDidTx(true);
        mPcapCallback(&sendFrame, mPcapCallbackContext);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        // `HandleTrasnmitDone()` will be invoked from `mOperationTask`
        // tasklet handler with error `OT_ERROR_ABORT`.

        mTransmitAborted = true;
        mOperationTask.Post();
    }
}

extern "C" void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(instance->IsInitialized());
    instance->GetThreadNetif().GetMac().HandleTransmitStarted(aFrame);

exit:
    return;
}

void Mac::HandleTransmitStarted(otRadioFrame *aFrame)
{
    Frame *frame = static_cast<Frame *>(aFrame);

    if (frame->GetAckRequest() && !(otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_ACK_TIMEOUT))
    {
        mMacTimer.Start(kAckTimeout);
        otLogDebgMac("Ack timer start");
    }
}

extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(instance->IsInitialized());

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (instance->GetLinkRaw().IsEnabled())
    {
        instance->GetLinkRaw().InvokeTransmitDone(aFrame, aAckFrame, aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        instance->GetThreadNetif().GetMac().HandleTransmitDone(aFrame, aAckFrame, aError);
    }

exit:
    return;
}

void Mac::HandleTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    Frame &   sendFrame    = *static_cast<Frame *>(aFrame);
    bool      framePending = false;
    bool      ccaSuccess   = true;
    Address   dstAddr;
    bool      ackRequested;
    Neighbor *neighbor;

    // Stop the ack timer.

    mMacTimer.Stop();

    // Record CCA success or failure status.

    switch (aError)
    {
    case OT_ERROR_ABORT:
        mCounters.mTxErrAbort++;
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        ccaSuccess = false;

        // fall through

    case OT_ERROR_NONE:
    case OT_ERROR_NO_ACK:

        if (mCcaSampleCount < kMaxCcaSampleCount)
        {
            mCcaSampleCount++;
        }

        if (sendFrame.GetChannel() == mPanChannel)
        {
            mCcaSuccessRateTracker.AddSample(ccaSuccess, mCcaSampleCount);
        }

        break;

    default:
        assert(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

    // Determine whether a CSMA retry is required.

    if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
    {
        mCounters.mTxErrCca++;

        if (!RadioSupportsCsmaBackoff() && mCsmaBackoffs < sendFrame.GetMaxCsmaBackoffs())
        {
            mCsmaBackoffs++;
            StartCsmaBackoff();
            ExitNow();
        }
    }

    mCsmaBackoffs = 0;

    sendFrame.GetDstAddr(dstAddr);
    neighbor     = GetNetif().GetMle().GetNeighbor(dstAddr);
    ackRequested = sendFrame.GetAckRequest();

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

    // Determine whether to re-transmit the frame.

    if (aError != OT_ERROR_NONE)
    {
        LogFrameTxFailure(sendFrame, aError);

        otDumpDebgMac("TX ERR", sendFrame.GetHeader(), 16);

        if (mEnabled && !RadioSupportsRetries() && mTransmitRetries < sendFrame.GetMaxFrameRetries())
        {
            mTransmitRetries++;
            mCounters.mTxRetry++;
            StartCsmaBackoff();
            ExitNow();
        }
    }

    mTransmitRetries = 0;

    // Process the ack frame for "frame pending".

    if (aError == OT_ERROR_NONE && ackRequested && aAckFrame != NULL)
    {
        Frame &ackFrame = *static_cast<Frame *>(aAckFrame);

        framePending = ackFrame.GetFramePending();

        if (neighbor != NULL)
        {
            neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), ackFrame.GetRssi());
        }
    }

    // Update MAC counters.

    mCounters.mTxTotal++;

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

        // Determine whether to re-transmit the broadcast frame.
        mBroadcastTransmitCount++;
        if (mBroadcastTransmitCount < kTxNumBcast)
        {
            StartCsmaBackoff();
            ExitNow();
        }

        mBroadcastTransmitCount = 0;
    }
    else
    {
        mCounters.mTxUnicast++;
    }

    // Determine next action based on current operation.

    switch (mOperation)
    {
    case kOperationActiveScan:
        mCounters.mTxBeaconRequest++;
        mMacTimer.Start(mScanDuration);
        break;

    case kOperationTransmitBeacon:
        mCounters.mTxBeacon++;
        FinishOperation();
        break;

    case kOperationTransmitData:
    {
        Sender *sender;

        if (sendFrame.IsDataRequestCommand())
        {
            if (mEnabled && framePending)
            {
                mReceiveTimer.Start(kDataPollTimeout);
                StartOperation(kOperationWaitingForData);
            }

            mCounters.mTxDataPoll++;
        }
        else
        {
            mCounters.mTxData++;
        }

        sender    = mSendHead;
        mSendHead = mSendHead->mNext;

        if (mSendHead == NULL)
        {
            mSendTail = NULL;
        }

        sender->mNext = NULL;

        if (!sendFrame.IsARetransmission())
        {
            mDataSequence++;
        }

        otDumpDebgMac("TX", sendFrame.GetHeader(), sendFrame.GetLength());
        sender->HandleSentFrame(sendFrame, aError);
        FinishOperation();
        break;
    }

    case kOperationTransmitOutOfBandFrame:
        mOobFrame = NULL;
        FinishOperation();
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

otError Mac::RadioTransmit(Frame *aSendFrame)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (!mRxOnWhenIdle)
    {
        // Cancel delay sleep timer
        mReceiveTimer.Stop();

        // Delay sleep if we have another frame pending to transmit
        mDelaySleep = aSendFrame->GetFramePending();
    }

#endif

    SuccessOrExit(error = otPlatRadioTransmit(&GetInstance(), static_cast<otRadioFrame *>(aSendFrame)));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("otPlatRadioTransmit() failed with error %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mac::RadioReceive(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (!mRxOnWhenIdle)
    {
        // Cancel delay sleep timer
        mReceiveTimer.Stop();
    }

#endif

    SuccessOrExit(error = otPlatRadioReceive(&GetInstance(), aChannel));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("otPlatRadioReceive() failed with error %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mac::RadioSleep(void)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (mDelaySleep)
    {
        otLogDebgMac("Delaying sleep waiting for frame rx/tx");

        mReceiveTimer.Start(kSleepDelay);
        mDelaySleep = false;

        // If sleep is delayed, `OT_ERROR_INVALID_STATE` is
        // returned to inform the caller to put/keep the
        // radio in receive mode.

        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

#endif

    error = otPlatRadioSleep(&GetInstance());
    VerifyOrExit(error != OT_ERROR_NONE);

    otLogWarnMac("otPlatRadioSleep() failed with error %s", otThreadErrorToString(error));

exit:
    return error;
}

void Mac::HandleMacTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mac>().HandleMacTimer();
}

void Mac::HandleMacTimer(void)
{
    switch (mOperation)
    {
    case kOperationActiveScan:
        PerformActiveScan();
        break;

    case kOperationEnergyScan:
        mBackoffTimer.Stop();
        EnergyScanDone(mEnergyScanCurrentMaxRssi);
        break;

    case kOperationTransmitData:
        otLogDebgMac("Ack timer fired");
        RadioReceive(mTxFrame->mChannel);
        HandleTransmitDone(mTxFrame, NULL, OT_ERROR_NO_ACK);
        break;

    default:
        assert(false);
        break;
    }
}

void Mac::HandleReceiveTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mac>().HandleReceiveTimer();
}

void Mac::HandleReceiveTimer(void)
{
    // `mReceiveTimer` is used for two purposes: (1) for data poll timeout
    // (i.e., waiting to receive a data frame after a data poll ack
    // indicating a pending frame from parent), and (2) for delaying sleep
    // when feature `OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS` is
    // enabled.

    if (mOperation == kOperationWaitingForData)
    {
        otLogDebgMac("Data poll timeout");

        FinishOperation();

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleDataPollTimeout();
        }
    }
    else
    {
        otLogDebgMac("Sleep delay timeout expired");

        UpdateIdleMode();
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

extern "C" void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(instance->IsInitialized());

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (instance->GetLinkRaw().IsEnabled())
    {
        instance->GetLinkRaw().InvokeReceiveDone(aFrame, aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        instance->GetThreadNetif().GetMac().HandleReceivedFrame(static_cast<Frame *>(aFrame), aError);
    }

exit:
    return;
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

    if (mPcapCallback)
    {
        aFrame->SetDidTx(false);
        mPcapCallback(aFrame, mPcapCallbackContext);
    }

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
                         ((mRxOnWhenIdle && dstaddr.IsBroadcast()) || dstaddr.GetShort() == mShortAddress),
                     error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);

        // Allow  multicasts from neighbor routers if FTD
        if (neighbor == NULL && dstaddr.IsBroadcast() && netif.GetMle().IsFullThreadDevice())
        {
            neighbor = netif.GetMle().GetRxOnlyNeighborRouter(srcaddr);
        }

        break;

    case Address::kTypeExtended:
        aFrame->GetDstPanId(panid);
        VerifyOrExit(panid == mPanId && dstaddr.GetExtended() == mExtAddress,
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
        if (srcaddr.GetExtended() == mExtAddress)
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
        // so the duplicate frame will not be passed to any
        // registered `Receiver`.

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
            mReceiveTimer.Stop();

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
            mDelaySleep = aFrame->GetFramePending();
#endif
            FinishOperation();
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

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(*aFrame);
        }
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
    mPcapCallback        = aPcapCallback;
    mPcapCallbackContext = aCallbackContext;
}

bool Mac::IsPromiscuous(void)
{
    return otPlatRadioGetPromiscuous(&GetInstance());
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    otPlatRadioSetPromiscuous(&GetInstance(), aPromiscuous);
    UpdateIdleMode();
}

bool Mac::RadioSupportsCsmaBackoff(void)
{
    // Check for either of the following conditions:
    //   1) Radio provides the CSMA backoff capability (i.e.,
    //      `OT_RADIO_CAPS_CSMA_BACKOFF` bit is set) or;
    //   2) It provides `OT_RADIO_CAPS_TRANSMIT_RETRIES` which
    //      indicates support for MAC retries along with CSMA
    //      backoff.

    return (otPlatRadioGetCaps(&GetInstance()) & (OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF)) != 0;
}

bool Mac::RadioSupportsRetries(void)
{
    return (otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_TRANSMIT_RETRIES) != 0;
}

otError Mac::SetEnabled(bool aEnable)
{
    mEnabled = aEnable;

    return OT_ERROR_NONE;
}

void Mac::FillMacCountersTlv(NetworkDiagnostic::MacCountersTlv &aMacCounters) const
{
    aMacCounters.SetIfInUnknownProtos(mCounters.mRxOther);
    aMacCounters.SetIfInErrors(mCounters.mRxErrNoFrame + mCounters.mRxErrUnknownNeighbor +
                               mCounters.mRxErrInvalidSrcAddr + mCounters.mRxErrSec + mCounters.mRxErrFcs +
                               mCounters.mRxErrOther);
    aMacCounters.SetIfOutErrors(mCounters.mTxErrCca);
    aMacCounters.SetIfInUcastPkts(mCounters.mRxUnicast);
    aMacCounters.SetIfInBroadcastPkts(mCounters.mRxBroadcast);
    aMacCounters.SetIfInDiscards(mCounters.mRxAddressFiltered + mCounters.mRxDestAddrFiltered +
                                 mCounters.mRxDuplicated);
    aMacCounters.SetIfOutUcastPkts(mCounters.mTxUnicast);
    aMacCounters.SetIfOutBroadcastPkts(mCounters.mTxBroadcast);
    aMacCounters.SetIfOutDiscards(mCounters.mTxErrBusyChannel);
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
    if (aFrame == NULL)
    {
        otLogInfoMac("Frame rx failed, error:%s", otThreadErrorToString(aError));
    }
    else
    {
        otLogInfoMac("Frame rx failed, error:%s, %s", otThreadErrorToString(aError),
                     aFrame->ToInfoString().AsCString());
    }
}

void Mac::LogFrameTxFailure(const Frame &aFrame, otError aError) const
{
    otLogInfoMac("Frame tx failed, error:%s, retries:%d/%d, %s", otThreadErrorToString(aError), mTransmitRetries,
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

void Mac::LogFrameTxFailure(const Frame &, otError) const
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

/**
 * This function will be called from interrupt context, it should only read/write data passed in
 * via @p aFrame, but should not read/write any state within OpenThread.
 */
#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
extern "C" void otPlatRadioFrameUpdated(otInstance *aInstance, otRadioFrame *aFrame)
{
    Instance *        instance   = static_cast<Instance *>(aInstance);
    Frame             frame      = *static_cast<Frame *>(aFrame);
    const ExtAddress *extAddress = NULL;

    VerifyOrExit(instance->IsInitialized());

    if (frame.GetSecurityEnabled() == true)
    {
        extAddress = &instance->GetThreadNetif().GetMac().GetExtAddress();
        instance->GetThreadNetif().GetMac().ProcessTransmitAesCcm(frame, extAddress);
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_HEADER_IE_SUPPORT

} // namespace Mac
} // namespace ot
