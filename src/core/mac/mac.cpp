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

#include "mac.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "crypto/aes_ccm.hpp"
#include "crypto/sha256.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/topology.hpp"

namespace ot {
namespace Mac {

const uint8_t Mac::sMode2Key[] = {0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f,
                                  0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66};

const otExtAddress Mac::sMode2ExtAddress = {
    {0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12},
};

const otExtendedPanId Mac::sExtendedPanidInit = {
    {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe},
};

const char Mac::sNetworkNameInit[] = "OpenThread";

Mac::Mac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mPendingActiveScan(false)
    , mPendingEnergyScan(false)
    , mPendingTransmitBeacon(false)
    , mPendingTransmitDataDirect(false)
#if OPENTHREAD_FTD
    , mPendingTransmitDataIndirect(false)
#endif
    , mPendingTransmitPoll(false)
    , mPendingTransmitOobFrame(false)
    , mPendingWaitingForData(false)
    , mShouldTxPollBeforeData(false)
    , mRxOnWhenIdle(false)
    , mPromiscuous(false)
    , mBeaconsEnabled(false)
    , mUsingTemporaryChannel(false)
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    , mShouldDelaySleep(false)
    , mDelayingSleep(false)
#endif
    , mOperation(kOperationIdle)
    , mBeaconSequence(Random::NonCrypto::GetUint8())
    , mDataSequence(Random::NonCrypto::GetUint8())
    , mBroadcastTransmitCount(0)
    , mPanId(kPanIdBroadcast)
    , mPanChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mSupportedChannelMask(Get<Radio>().GetSupportedChannelMask())
    , mNetworkName()
    , mScanChannel(Radio::kChannelMin)
    , mScanDuration(0)
    , mScanChannelMask()
    , mMaxFrameRetriesDirect(kDefaultMaxFrameRetriesDirect)
#if OPENTHREAD_FTD
    , mMaxFrameRetriesIndirect(kDefaultMaxFrameRetriesIndirect)
#endif
    , mActiveScanHandler(NULL) // Initialize `mActiveScanHandler` and `mEnergyScanHandler` union
    , mScanHandlerContext(NULL)
    , mSubMac(aInstance)
    , mOperationTask(aInstance, &Mac::HandleOperationTask, this)
    , mTimer(aInstance, &Mac::HandleTimer, this)
    , mOobFrame(NULL)
    , mKeyIdMode2FrameCounter(0)
    , mCcaSampleCount(0)
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    , mFilter()
#endif
{
    ExtAddress randomExtAddress;

    randomExtAddress.GenerateRandom();

    mCcaSuccessRateTracker.Reset();
    ResetCounters();
    mExtendedPanId.Clear();

    mSubMac.Enable();

    SetExtendedPanId(static_cast<const ExtendedPanId &>(sExtendedPanidInit));
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

    mActiveScanHandler  = aHandler;
    mScanHandlerContext = aContext;

    if (aScanDuration == 0)
    {
        aScanDuration = kScanDurationDefault;
    }

    Scan(kOperationActiveScan, aScanChannels, aScanDuration);

exit:
    return error;
}

otError Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = OT_ERROR_BUSY);

    mEnergyScanHandler  = aHandler;
    mScanHandlerContext = aContext;

    Scan(kOperationEnergyScan, aScanChannels, aScanDuration);

exit:
    return error;
}

void Mac::Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration)
{
    mScanDuration = aScanDuration;
    mScanChannel  = ChannelMask::kChannelIteratorFirst;

    if (aScanChannels == 0)
    {
        aScanChannels = GetSupportedChannelMask().GetMask();
    }

    mScanChannelMask.SetMask(aScanChannels);
    mScanChannelMask.Intersect(mSupportedChannelMask);
    StartOperation(aScanOperation);
}

bool Mac::IsInTransmitState(void) const
{
    bool retval = false;

    switch (mOperation)
    {
    case kOperationTransmitDataDirect:
#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
#endif
    case kOperationTransmitBeacon:
    case kOperationTransmitPoll:
    case kOperationTransmitOutOfBandFrame:
        retval = true;
        break;

    case kOperationIdle:
    case kOperationActiveScan:
    case kOperationEnergyScan:
    case kOperationWaitingForData:
        retval = false;
        break;
    }

    return retval;
}

otError Mac::ConvertBeaconToActiveScanResult(const RxFrame *aBeaconFrame, ActiveScanResult &aResult)
{
    otError              error = OT_ERROR_NONE;
    Address              address;
    const Beacon *       beacon        = NULL;
    const BeaconPayload *beaconPayload = NULL;
    uint16_t             payloadLength;

    memset(&aResult, 0, sizeof(ActiveScanResult));

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

    beacon        = reinterpret_cast<const Beacon *>(aBeaconFrame->GetPayload());
    beaconPayload = reinterpret_cast<const BeaconPayload *>(beacon->GetPayload());

    if ((payloadLength >= (sizeof(*beacon) + sizeof(*beaconPayload))) && beacon->IsValid() && beaconPayload->IsValid())
    {
        aResult.mVersion    = beaconPayload->GetProtocolVersion();
        aResult.mIsJoinable = beaconPayload->IsJoiningPermitted();
        aResult.mIsNative   = beaconPayload->IsNative();
        static_cast<NetworkName &>(aResult.mNetworkName).Set(beaconPayload->GetNetworkName());
        aResult.mExtendedPanId = beaconPayload->GetExtendedPanId();
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
        ReportActiveScanResult(NULL);
        PerformNextOperation();
    }
}

void Mac::ReportActiveScanResult(const RxFrame *aBeaconFrame)
{
    VerifyOrExit(mActiveScanHandler != NULL);

    if (aBeaconFrame == NULL)
    {
        mActiveScanHandler(NULL, mScanHandlerContext);
    }
    else
    {
        ActiveScanResult result;

        SuccessOrExit(ConvertBeaconToActiveScanResult(aBeaconFrame, result));
        mActiveScanHandler(&result, mScanHandlerContext);
    }

exit:
    return;
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

        if (mEnergyScanHandler != NULL)
        {
            mEnergyScanHandler(NULL, mScanHandlerContext);
        }

        PerformNextOperation();
    }
}

void Mac::ReportEnergyScanResult(int8_t aRssi)
{
    EnergyScanResult result;

    VerifyOrExit((mEnergyScanHandler != NULL) && (aRssi != kInvalidRssiValue));

    result.mChannel = mScanChannel;
    result.mMaxRssi = aRssi;

    mEnergyScanHandler(&result, mScanHandlerContext);

exit:
    return;
}

void Mac::EnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    ReportEnergyScanResult(aEnergyScanMaxRssi);
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

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
        mDelayingSleep    = false;
        mShouldDelaySleep = false;
#endif
    }

    mSubMac.SetRxOnWhenBackoff(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();

exit:
    return;
}

otError Mac::SetPanChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(Get<Notifier>().Update(mPanChannel, aChannel, OT_CHANGED_THREAD_CHANNEL));

    mCcaSuccessRateTracker.Reset();

    VerifyOrExit(!mUsingTemporaryChannel);

    mRadioChannel = mPanChannel;

    UpdateIdleMode();

exit:
    return error;
}

otError Mac::SetTemporaryChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);

    mUsingTemporaryChannel = true;
    mRadioChannel          = aChannel;

    UpdateIdleMode();

exit:
    return error;
}

void Mac::ClearTemporaryChannel(void)
{
    if (mUsingTemporaryChannel)
    {
        mUsingTemporaryChannel = false;
        mRadioChannel          = mPanChannel;
        UpdateIdleMode();
    }
}

void Mac::SetSupportedChannelMask(const ChannelMask &aMask)
{
    ChannelMask newMask = aMask;

    newMask.Intersect(ChannelMask(Get<Radio>().GetSupportedChannelMask()));
    Get<Notifier>().Update(mSupportedChannelMask, newMask, OT_CHANGED_SUPPORTED_CHANNEL_MASK);
}

otError Mac::SetNetworkName(const char *aNameString)
{
    // When setting Network Name from a string, we treat it as `Data`
    // with `kMaxSize + 1` chars. `NetworkName::Set(data)` will look
    // for null char in the data (within its given size) to calculate
    // the name's length and ensure that the name fits in `kMaxSize`
    // chars. The `+ 1` ensures that a `aNameString` with length
    // longer than `kMaxSize` is correctly rejected (returning error
    // `OT_ERROR_INVALID_ARGS`).

    NetworkName::Data data(aNameString, NetworkName::kMaxSize + 1);

    return SetNetworkName(data);
}

otError Mac::SetNetworkName(const NetworkName::Data &aName)
{
    otError error = mNetworkName.Set(aName);

    if (error == OT_ERROR_ALREADY)
    {
        Get<Notifier>().SignalIfFirst(OT_CHANGED_THREAD_NETWORK_NAME);
        error = OT_ERROR_NONE;
        ExitNow();
    }

    SuccessOrExit(error);
    Get<Notifier>().Signal(OT_CHANGED_THREAD_NETWORK_NAME);

exit:
    return error;
}

void Mac::SetPanId(PanId aPanId)
{
    SuccessOrExit(Get<Notifier>().Update(mPanId, aPanId, OT_CHANGED_THREAD_PANID));
    mSubMac.SetPanId(mPanId);

exit:
    return;
}

void Mac::SetExtendedPanId(const ExtendedPanId &aExtendedPanId)
{
    Get<Notifier>().Update(mExtendedPanId, aExtendedPanId, OT_CHANGED_THREAD_EXT_PANID);
}

otError Mac::RequestDirectFrameTransmission(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitDataDirect && (mOperation != kOperationTransmitDataDirect), error = OT_ERROR_ALREADY);

    StartOperation(kOperationTransmitDataDirect);

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Mac::RequestIndirectFrameTransmission(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitDataIndirect && (mOperation != kOperationTransmitDataIndirect),
                 error = OT_ERROR_ALREADY);

    StartOperation(kOperationTransmitDataIndirect);

exit:
    return error;
}
#endif

otError Mac::RequestOutOfBandFrameTransmission(otRadioFrame *aOobFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aOobFrame != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitOobFrame && (mOperation != kOperationTransmitOutOfBandFrame),
                 error = OT_ERROR_ALREADY);

    mOobFrame = static_cast<TxFrame *>(aOobFrame);

    StartOperation(kOperationTransmitOutOfBandFrame);

exit:
    return error;
}

otError Mac::RequestDataPollTransmission(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mPendingTransmitPoll && (mOperation != kOperationTransmitPoll), error = OT_ERROR_ALREADY);

    // We ensure data frame and data poll tx requests are handled in the
    // order they are requested. So if we have a pending direct data frame
    // tx request, it should be sent before the poll frame.

    mShouldTxPollBeforeData = !mPendingTransmitDataDirect;

    StartOperation(kOperationTransmitPoll);

exit:
    return error;
}

void Mac::UpdateIdleMode(void)
{
    bool shouldSleep = !mRxOnWhenIdle && !mPromiscuous;

    VerifyOrExit(mOperation == kOperationIdle);

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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

    case kOperationTransmitDataDirect:
        mPendingTransmitDataDirect = true;
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        mPendingTransmitDataIndirect = true;
        break;
#endif

    case kOperationTransmitPoll:
        mPendingTransmitPoll = true;
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
        mPendingWaitingForData     = false;
        mPendingTransmitOobFrame   = false;
        mPendingActiveScan         = false;
        mPendingEnergyScan         = false;
        mPendingTransmitBeacon     = false;
        mPendingTransmitDataDirect = false;
#if OPENTHREAD_FTD
        mPendingTransmitDataIndirect = false;
#endif
        mPendingTransmitPoll = false;
        mTimer.Stop();
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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
#if OPENTHREAD_FTD
    else if (mPendingTransmitDataIndirect)
    {
        mPendingTransmitDataIndirect = false;
        mOperation                   = kOperationTransmitDataIndirect;
    }
#endif
    else if (mPendingTransmitPoll && (!mPendingTransmitDataDirect || mShouldTxPollBeforeData))
    {
        mPendingTransmitPoll = false;
        mOperation           = kOperationTransmitPoll;
    }
    else if (mPendingTransmitDataDirect)
    {
        mPendingTransmitDataDirect = false;
        mOperation                 = kOperationTransmitDataDirect;

        if (mPendingTransmitPoll)
        {
            // Ensure that a pending "transmit poll" operation request
            // is prioritized over any future "transmit data" requests.
            mShouldTxPollBeforeData = true;
        }
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
    case kOperationTransmitDataDirect:
#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
#endif
    case kOperationTransmitPoll:
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

otError Mac::PrepareDataRequest(TxFrame &aFrame)
{
    otError  error = OT_ERROR_NONE;
    Address  src, dst;
    uint16_t fcf;

    SuccessOrExit(error = Get<DataPollSender>().GetPollDestinationAddress(dst));
    VerifyOrExit(!dst.IsNone(), error = OT_ERROR_ABORT);

    fcf = Frame::kFcfFrameMacCmd | Frame::kFcfPanidCompression | Frame::kFcfFrameVersion2006 | Frame::kFcfAckRequest |
          Frame::kFcfSecurityEnabled;

    if (dst.IsExtended())
    {
        fcf |= Frame::kFcfDstAddrExt | Frame::kFcfSrcAddrExt;
        src.SetExtended(GetExtAddress());
    }
    else
    {
        fcf |= Frame::kFcfDstAddrShort | Frame::kFcfSrcAddrShort;
        src.SetShort(GetShortAddress());
    }

    aFrame.InitMacHeader(fcf, Frame::kKeyIdMode1 | Frame::kSecEncMic32);
    aFrame.SetDstPanId(GetPanId());
    aFrame.SetSrcAddr(src);
    aFrame.SetDstAddr(dst);
    aFrame.SetCommandId(Frame::kMacCmdDataRequest);

exit:
    return error;
}

void Mac::PrepareBeaconRequest(TxFrame &aFrame)
{
    uint16_t fcf = Frame::kFcfFrameMacCmd | Frame::kFcfDstAddrShort | Frame::kFcfSrcAddrNone;

    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetDstPanId(kShortAddrBroadcast);
    aFrame.SetDstAddr(kShortAddrBroadcast);
    aFrame.SetCommandId(Frame::kMacCmdBeaconRequest);

    otLogInfoMac("Sending Beacon Request");
}

void Mac::PrepareBeacon(TxFrame &aFrame)
{
    uint8_t        beaconLength;
    uint16_t       fcf;
    Beacon *       beacon        = NULL;
    BeaconPayload *beaconPayload = NULL;

    fcf = Frame::kFcfFrameBeacon | Frame::kFcfDstAddrNone | Frame::kFcfSrcAddrExt;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetSrcPanId(mPanId);
    aFrame.SetSrcAddr(GetExtAddress());

    beacon = reinterpret_cast<Beacon *>(aFrame.GetPayload());
    beacon->Init();
    beaconLength = sizeof(*beacon);

    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    if (Get<KeyManager>().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_BEACONS)
    {
        beaconPayload->Init();

        if (IsJoinable())
        {
            beaconPayload->SetJoiningPermitted();
        }
        else
        {
            beaconPayload->ClearJoiningPermitted();
        }

        beaconPayload->SetNetworkName(mNetworkName.GetAsData());
        beaconPayload->SetExtendedPanId(mExtendedPanId);

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

#if OPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE
    if (!shouldSend)
    {
        // When `ENABLE_BEACON_RSP_WHEN_JOINABLE` feature is enabled,
        // the device should transmit IEEE 802.15.4 Beacons in response
        // to IEEE 802.15.4 Beacon Requests even while the device is not
        // router capable and detached (i.e., `IsBeaconEnabled()` is
        // false) but only if it is in joinable state (unsecure port
        // list is not empty).

        shouldSend = IsJoinable();
    }
#endif

exit:
    return shouldSend;
}

bool Mac::IsJoinable(void) const
{
    uint8_t numUnsecurePorts;

    Get<Ip6::Filter>().GetUnsecurePorts(numUnsecurePorts);

    return (numUnsecurePorts != 0);
}

void Mac::ProcessTransmitSecurity(TxFrame &aFrame, bool aProcessAesCcm)
{
    KeyManager &      keyManager = Get<KeyManager>();
    uint8_t           keyIdMode;
    const ExtAddress *extAddress = NULL;

    VerifyOrExit(aFrame.GetSecurityEnabled());

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
        aFrame.ProcessTransmitAesCcm(*extAddress);
    }

exit:
    return;
}

void Mac::BeginTransmit(void)
{
    otError  error                 = OT_ERROR_NONE;
    bool     applyTransmitSecurity = true;
    bool     processTransmitAesCcm = true;
    TxFrame &sendFrame             = mSubMac.GetTransmitFrame();

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);
    sendFrame.SetIsARetransmission(false);

    switch (mOperation)
    {
    case kOperationActiveScan:
        mSubMac.SetPanId(kPanIdBroadcast);
        sendFrame.SetChannel(mScanChannel);
        PrepareBeaconRequest(sendFrame);
        sendFrame.SetSequence(0);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitBeacon:
        sendFrame.SetChannel(mRadioChannel);
        PrepareBeacon(sendFrame);
        sendFrame.SetSequence(mBeaconSequence++);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitPoll:
        sendFrame.SetChannel(mRadioChannel);
        SuccessOrExit(error = PrepareDataRequest(sendFrame));
        sendFrame.SetSequence(mDataSequence++);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitDataDirect:
        sendFrame.SetChannel(mRadioChannel);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        sendFrame.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        SuccessOrExit(error = Get<MeshForwarder>().HandleFrameRequest(sendFrame));
        sendFrame.SetSequence(mDataSequence++);
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        sendFrame.SetChannel(mRadioChannel);
        sendFrame.SetMaxCsmaBackoffs(kMaxCsmaBackoffsIndirect);
        sendFrame.SetMaxFrameRetries(mMaxFrameRetriesIndirect);
        SuccessOrExit(error = Get<DataPollHandler>().HandleFrameRequest(sendFrame));

        // If the frame is marked as a retransmission, then data sequence number is already set.
        if (!sendFrame.IsARetransmission())
        {
            sendFrame.SetSequence(mDataSequence++);
        }
        break;
#endif

    case kOperationTransmitOutOfBandFrame:
        sendFrame.CopyFrom(*mOobFrame);
        applyTransmitSecurity = false;
        break;

    default:
        assert(false);
        break;
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        uint8_t timeIeOffset = GetTimeIeOffset(sendFrame);

        sendFrame.SetTimeIeOffset(timeIeOffset);

        if (timeIeOffset != 0)
        {
            // Transmit security will be processed after time IE content is updated.
            processTransmitAesCcm = false;
            sendFrame.SetTimeSyncSeq(Get<TimeSync>().GetTimeSyncSeq());
            sendFrame.SetNetworkTimeOffset(Get<TimeSync>().GetNetworkTimeOffset());
        }
    }
#endif

    if (applyTransmitSecurity)
    {
        ProcessTransmitSecurity(sendFrame, processTransmitAesCcm);
    }

    mBroadcastTransmitCount = 0;

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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
        // If the sendFrame could not be prepared and the tx is being
        // aborted, we set the frame length to zero to mark it as empty.
        // The empty frame helps differentiate between an aborted tx due
        // to OpenThread itself not being able to prepare the frame, versus
        // the radio platform aborting the tx operation.

        sendFrame.SetLength(0);
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

void Mac::RecordFrameTransmitStatus(const TxFrame &aFrame,
                                    const RxFrame *aAckFrame,
                                    otError        aError,
                                    uint8_t        aRetryCount,
                                    bool           aWillRetx)
{
    bool      ackRequested = aFrame.GetAckRequest();
    Address   dstAddr;
    Neighbor *neighbor;

    VerifyOrExit(!aFrame.IsEmpty());

    aFrame.GetDstAddr(dstAddr);
    neighbor = Get<Mle::MleRouter>().GetNeighbor(dstAddr);

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

    // Log frame transmission failure.

    if (aError != OT_ERROR_NONE)
    {
        LogFrameTxFailure(aFrame, aError, aRetryCount, aWillRetx);
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

void Mac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, otError aError)
{
    if (!aFrame.IsEmpty())
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

    case kOperationTransmitPoll:
        assert(aFrame.IsEmpty() || aFrame.GetAckRequest());

        if ((aError == OT_ERROR_NONE) && (aAckFrame != NULL))
        {
            bool framePending = aAckFrame->GetFramePending();

            if (mEnabled && framePending)
            {
                mTimer.Start(kDataPollTimeout);
                StartOperation(kOperationWaitingForData);
            }

            otLogInfoMac("Sent data poll, fp:%s", framePending ? "yes" : "no");
        }

        mCounters.mTxDataPoll++;
        FinishOperation();
        Get<DataPollSender>().HandlePollSent(aFrame, aError);
        PerformNextOperation();
        break;

    case kOperationTransmitDataDirect:
        mCounters.mTxData++;

        if (aError != OT_ERROR_NONE)
        {
            mCounters.mTxDirectMaxRetryExpiry++;
        }
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
        else if (mSubMac.GetTransmitRetries() < OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT)
        {
            mRetryHistogram.mTxDirectRetrySuccess[mSubMac.GetTransmitRetries()]++;
        }
#endif

        otDumpDebgMac("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<MeshForwarder>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        mCounters.mTxData++;

        if (aError != OT_ERROR_NONE)
        {
            mCounters.mTxIndirectMaxRetryExpiry++;
        }
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
        else if (mSubMac.GetTransmitRetries() < OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT)
        {
            mRetryHistogram.mTxIndirectRetrySuccess[mSubMac.GetTransmitRetries()]++;
        }
#endif

        otDumpDebgMac("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<DataPollHandler>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();
        break;
#endif

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
        Get<DataPollSender>().HandlePollTimeout();
        PerformNextOperation();
        break;

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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

otError Mac::ProcessReceiveSecurity(RxFrame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager &      keyManager = Get<KeyManager>();
    otError           error      = OT_ERROR_SECURITY;
    uint8_t           securityLevel;
    uint8_t           keyIdMode;
    uint32_t          frameCounter;
    uint8_t           nonce[KeyManager::kNonceSize];
    uint8_t           tag[Frame::kMaxMicSize];
    uint8_t           tagLength;
    uint8_t           keyid;
    uint32_t          keySequence = 0;
    const uint8_t *   macKey;
    const ExtAddress *extAddress;
    Crypto::AesCcm    aesCcm;

    VerifyOrExit(aFrame.GetSecurityEnabled(), error = OT_ERROR_NONE);

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);
    otLogDebgMac("Rx security - frame counter %u", frameCounter);

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        macKey = keyManager.GetKek();
        VerifyOrExit(macKey != NULL);
        extAddress = &aSrcAddr.GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != NULL);

        aFrame.GetKeyId(keyid);
        keyid--;

        if (keyid == (keyManager.GetCurrentKeySequence() & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence();
            macKey      = keyManager.GetCurrentMacKey();
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() - 1;
            macKey      = keyManager.GetTemporaryMacKey(keySequence);
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() + 1;
            macKey      = keyManager.GetTemporaryMacKey(keySequence);
        }
        else
        {
            ExitNow();
        }

        // If the frame is from a neighbor not in valid state (e.g., it is from a child being
        // restored), skip the key sequence and frame counter checks but continue to verify
        // the tag/MIC. Such a frame is later filtered in `RxDoneTask` which only allows MAC
        // Data Request frames from a child being restored.

        if (aNeighbor->IsStateValid())
        {
            VerifyOrExit(keySequence >= aNeighbor->GetKeySequence());

            if (keySequence == aNeighbor->GetKeySequence())
            {
                // If frame counter is one off, then frame is a duplicate.
                VerifyOrExit((frameCounter + 1) != aNeighbor->GetLinkFrameCounter(), error = OT_ERROR_DUPLICATED);

                VerifyOrExit(frameCounter >= aNeighbor->GetLinkFrameCounter());
            }
        }

        extAddress = &aSrcAddr.GetExtended();

        break;

    case Frame::kKeyIdMode2:
        macKey     = sMode2Key;
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;

    default:
        ExitNow();
        break;
    }

    KeyManager::GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.SetKey(macKey, 16);

    SuccessOrExit(aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce)));

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#else
    // For fuzz tests, execute AES but do not alter the payload
    uint8_t fuzz[OT_RADIO_FRAME_MAX_SIZE];
    aesCcm.Payload(fuzz, aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#endif
    aesCcm.Finalize(tag, &tagLength);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(memcmp(tag, aFrame.GetFooter(), tagLength) == 0);
#endif

    if ((keyIdMode == Frame::kKeyIdMode1) && aNeighbor->IsStateValid())
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

    error = OT_ERROR_NONE;

exit:
    return error;
}

void Mac::HandleReceivedFrame(RxFrame *aFrame, otError aError)
{
    Address   srcaddr;
    Address   dstaddr;
    PanId     panid;
    Neighbor *neighbor;
    otError   error = aError;

    mCounters.mRxTotal++;

    SuccessOrExit(error);
    VerifyOrExit(aFrame != NULL, error = OT_ERROR_NO_FRAME_RECEIVED);
    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    aFrame->GetSrcAddr(srcaddr);
    aFrame->GetDstAddr(dstaddr);
    neighbor = Get<Mle::MleRouter>().GetNeighbor(srcaddr);

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
        if (neighbor == NULL && dstaddr.IsBroadcast() && Get<Mle::MleRouter>().IsFullThreadDevice())
        {
            neighbor = Get<Mle::MleRouter>().GetRxOnlyNeighborRouter(srcaddr);
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

        VerifyOrExit(neighbor != NULL, error = OT_ERROR_UNKNOWN_NEIGHBOR);

        srcaddr.SetExtended(neighbor->GetExtAddress());

        // Fall through

    case Address::kTypeExtended:

        // Duplicate Address Protection
        VerifyOrExit(srcaddr.GetExtended() != GetExtAddress(), error = OT_ERROR_INVALID_SOURCE_ADDRESS);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        {
            int8_t fixedRss;

            SuccessOrExit(error = mFilter.Apply(srcaddr.GetExtended(), fixedRss));

            if (fixedRss != Filter::kFixedRssDisabled)
            {
                aFrame->SetRssi(fixedRss);

                // Clear any previous link info to ensure the fixed RSSI
                // value takes effect quickly.
                if (neighbor != NULL)
                {
                    neighbor->GetLinkInfo().Clear();
                }
            }
        }
#endif

        break;
    }

    if (dstaddr.IsBroadcast())
    {
        mCounters.mRxBroadcast++;
    }
    else
    {
        mCounters.mRxUnicast++;
    }

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

    Get<DataPollSender>().CheckFramePending(*aFrame);

    if (neighbor != NULL)
    {
        neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), aFrame->GetRssi());

        if (aFrame->GetSecurityEnabled())
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
            ReportActiveScanResult(aFrame);
            ExitNow();
        }

        // Fall through

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

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
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
        if (HandleMacCommand(*aFrame)) // returns `true` when handled
        {
            ExitNow(error = OT_ERROR_NONE);
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
        ExitNow();
    }

    otDumpDebgMac("RX", aFrame->GetHeader(), aFrame->GetLength());
    Get<MeshForwarder>().HandleReceivedFrame(*aFrame);

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

bool Mac::HandleMacCommand(RxFrame &aFrame)
{
    bool    didHandle = false;
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
        didHandle = true;
        break;

    case Frame::kMacCmdDataRequest:
        mCounters.mRxDataPoll++;
#if OPENTHREAD_FTD
        Get<DataPollHandler>().HandleDataPoll(aFrame);
        didHandle = true;
#endif
        break;

    default:
        mCounters.mRxOther++;
        break;
    }

    return didHandle;
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    mPromiscuous = aPromiscuous;
    Get<Radio>().SetPromiscuous(aPromiscuous);

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    mDelayingSleep    = false;
    mShouldDelaySleep = false;
#endif

    mSubMac.SetRxOnWhenBackoff(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();
}

#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
const uint32_t *Mac::GetDirectRetrySuccessHistogram(uint8_t &aNumberOfEntries)
{
    if (mMaxFrameRetriesDirect >= OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT)
    {
        aNumberOfEntries = OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT;
    }
    else
    {
        aNumberOfEntries = mMaxFrameRetriesDirect + 1;
    }

    return mRetryHistogram.mTxDirectRetrySuccess;
}

#if OPENTHREAD_FTD
const uint32_t *Mac::GetIndirectRetrySuccessHistogram(uint8_t &aNumberOfEntries)
{
    if (mMaxFrameRetriesIndirect >= OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT)
    {
        aNumberOfEntries = OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT;
    }
    else
    {
        aNumberOfEntries = mMaxFrameRetriesIndirect + 1;
    }

    return mRetryHistogram.mTxIndirectRetrySuccess;
}
#endif

void Mac::ResetRetrySuccessHistogram()
{
    memset(&mRetryHistogram, 0, sizeof(mRetryHistogram));
}
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

int8_t Mac::GetNoiseFloor(void)
{
    return Get<Radio>().GetReceiveSensitivity();
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

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

    case kOperationTransmitDataDirect:
        retval = "TransmitDataDirect";
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        retval = "TransmitDataIndirect";
        break;
#endif

    case kOperationTransmitPoll:
        retval = "TransmitPoll";
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

void Mac::LogFrameRxFailure(const RxFrame *aFrame, otError aError) const
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

void Mac::LogFrameTxFailure(const TxFrame &aFrame, otError aError, uint8_t aRetryCount, bool aWillRetx) const
{
    uint8_t maxAttempts = aFrame.GetMaxFrameRetries() + 1;
    uint8_t curAttempt  = aWillRetx ? (aRetryCount + 1) : maxAttempts;

    otLogInfoMac("Frame tx attempt %d/%d failed, error:%s, %s", curAttempt, maxAttempts, otThreadErrorToString(aError),
                 aFrame.ToInfoString().AsCString());
}

void Mac::LogBeacon(const char *aActionText, const BeaconPayload &aBeaconPayload) const
{
    otLogInfoMac("%s Beacon, %s", aActionText, aBeaconPayload.ToInfoString().AsCString());
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void Mac::LogFrameRxFailure(const RxFrame *, otError) const
{
}

void Mac::LogBeacon(const char *, const BeaconPayload &) const
{
}

void Mac::LogFrameTxFailure(const TxFrame &, otError, uint8_t, bool) const
{
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

// LCOV_EXCL_STOP

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
uint8_t Mac::GetTimeIeOffset(const Frame &aFrame)
{
    uint8_t        offset = 0;
    const uint8_t *base   = aFrame.GetPsdu();
    const uint8_t *cur    = NULL;

    cur = reinterpret_cast<const uint8_t *>(aFrame.GetTimeIe());
    VerifyOrExit(cur != NULL);

    cur += sizeof(VendorIeHeader);
    offset = static_cast<uint8_t>(cur - base);

exit:
    return offset;
}
#endif

} // namespace Mac
} // namespace ot
