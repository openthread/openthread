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

#include "crypto/aes_ccm.hpp"
#include "crypto/sha256.hpp"
#include "instance/instance.hpp"
#include "mac/mac_beacon.hpp"
#include "utils/static_counter.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("Mac");

const otExtAddress Mac::kMode2ExtAddress = {{0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12}};

const uint8_t Mac::kMode2KeySource[] = {0xff, 0xff, 0xff, 0xff};

const otMacKey Mac::kMode2Key = {
    {0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f, 0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66},
};

Mac::Mac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mShouldTxPollBeforeData(false)
    , mRxOnWhenIdle(false)
    , mPromiscuous(false)
    , mBeaconsEnabled(false)
    , mUsingTemporaryChannel(false)
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    , mShouldDelaySleep(false)
    , mDelayingSleep(false)
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    , mWakeupListenEnabled(false)
#endif
    , mOperation(kOperationIdle)
    , mPendingOperations(0)
    , mBeaconSequence(Random::NonCrypto::Generate<uint8_t>())
    , mDataSequence(Random::NonCrypto::Generate<uint8_t>())
    , mBroadcastTransmitCount(0)
    , mPanId(kPanIdBroadcast)
    , mPanChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mRadioChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mSupportedChannelMask(Get<Radio::Radio>().GetSupportedChannelMask())
    , mScanChannel(Radio::kChannelMin)
    , mScanDuration(0)
    , mMaxFrameRetriesDirect(kDefaultMaxFrameRetriesDirect)
#if OPENTHREAD_FTD
    , mMaxFrameRetriesIndirect(kDefaultMaxFrameRetriesIndirect)
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mIsCslEnabled(false)
    , mIsCslCapable(false)
    , mCslChannel(0)
    , mCslPeriod(0)
#endif
    , mWakeupChannel(OPENTHREAD_CONFIG_DEFAULT_WAKEUP_CHANNEL)
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    , mWakeupListenInterval(kDefaultWedListenInterval)
    , mWakeupListenDuration(kDefaultWedListenDuration)
#endif
    , mActiveScanCallback()
    , mLinks(aInstance)
    , mOperationTask(aInstance)
    , mTimer(aInstance)
    , mKeyIdMode2FrameCounter(0)
    , mCcaSampleCount(0)
#if OPENTHREAD_CONFIG_MULTI_RADIO
    , mTxError(kErrorNone)
#endif
{
    ExtAddress randomExtAddress;

    randomExtAddress.GenerateRandom();

    mCcaSuccessRateTracker.Clear();
    ResetCounters();

    SetEnabled(true);

    SetPanId(mPanId);
    SetExtAddress(randomExtAddress);
    SetShortAddress(GetShortAddress());
#if OPENTHREAD_FTD
    SetAlternateShortAddress(kShortAddrInvalid);
#endif

    mMode2KeyMaterial.SetFrom(AsCoreType(&kMode2Key));
}

void Mac::Init(void) { Get<KeyManager>().UpdateKeyMaterial(); }

void Mac::SetEnabled(bool aEnable)
{
    mEnabled = aEnable;

    if (aEnable)
    {
        mLinks.Enable();
    }
    else
    {
        mLinks.Disable();
    }
}

Error Mac::CanScan(void) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = kErrorBusy);

exit:
    return error;
}

Error Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ScanResult::Handler aHandler, void *aContext)
{
    Error error;

    SuccessOrExit(error = CanScan());

    mActiveScanCallback.Set(aHandler, aContext);

    if (aScanDuration == 0)
    {
        aScanDuration = kScanDurationDefault;
    }

    Scan(kOperationActiveScan, aScanChannels, aScanDuration);

exit:
    return error;
}

Error Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    Error error;

    SuccessOrExit(error = CanScan());

    mEnergyScanCallback.Set(aHandler, aContext);

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
        aScanChannels = mSupportedChannelMask.GetMask();
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
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
#endif
    case kOperationTransmitBeacon:
    case kOperationTransmitPoll:
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    case kOperationTransmitWakeup:
#endif
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

Error Mac::UpdateScanChannel(void)
{
    Error error;

    VerifyOrExit(IsEnabled(), error = kErrorAbort);

    error = mScanChannelMask.GetNextChannel(mScanChannel);

exit:
    return error;
}

void Mac::PerformActiveScan(void)
{
    if (UpdateScanChannel() == kErrorNone)
    {
        // If there are more channels to scan, send the beacon request.
        mLinks.SetRxOnWhenIdle(true);
        BeginTransmit();
    }
    else
    {
        mLinks.SetRxOnWhenIdle(mRxOnWhenIdle);
        mLinks.SetPanId(mPanId);
        FinishOperation();
        ReportActiveScanResult(nullptr);
        PerformNextOperation();
    }
}

void Mac::ReportActiveScanResult(const RxFrame *aBeaconFrame)
{
    VerifyOrExit(mActiveScanCallback.IsSet());

    if (aBeaconFrame == nullptr)
    {
        mActiveScanCallback.Invoke(nullptr);
    }
    else
    {
        ScanResult result;

        SuccessOrExit(result.PopulateFromBeacon(aBeaconFrame));
        LogBeacon("Received");

        mActiveScanCallback.Invoke(&result);
    }

exit:
    return;
}

void Mac::PerformEnergyScan(void)
{
    Error error = kErrorNone;

    SuccessOrExit(error = UpdateScanChannel());

    if (mScanDuration == 0)
    {
        while (true)
        {
            mLinks.Receive(mScanChannel);
            ReportEnergyScanResult(mLinks.GetRssi());
            SuccessOrExit(error = UpdateScanChannel());
        }
    }
    else
    {
        if (!mRxOnWhenIdle)
        {
            mLinks.Receive(mScanChannel);
        }
        error = mLinks.EnergyScan(mScanChannel, mScanDuration);
    }

exit:

    if (error != kErrorNone)
    {
        FinishOperation();

        mEnergyScanCallback.InvokeIfSet(nullptr);

        PerformNextOperation();
    }
}

void Mac::ReportEnergyScanResult(int8_t aRssi)
{
    EnergyScanResult result;

    VerifyOrExit(mEnergyScanCallback.IsSet() && (aRssi != Radio::kInvalidRssi));

    result.mChannel = mScanChannel;
    result.mMaxRssi = aRssi;

    mEnergyScanCallback.Invoke(&result);

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
        if (IsPending(kOperationWaitingForData))
        {
            mTimer.Stop();
            ClearPending(kOperationWaitingForData);
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

    mLinks.SetRxOnWhenIdle(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();

exit:
    return;
}

Error Mac::SetPanChannel(uint8_t aChannel)
{
    Error error = kErrorNone;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    bool isPanChannelChanged = (mPanChannel != aChannel);
#endif

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = kErrorInvalidArgs);

    SuccessOrExit(Get<Notifier>().Update(mPanChannel, aChannel, kEventThreadChannelChanged));

    mCcaSuccessRateTracker.Clear();

    VerifyOrExit(!mUsingTemporaryChannel);

    mRadioChannel = mPanChannel;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if ((mCslChannel == 0) && isPanChannelChanged)
    {
        UpdateCslParameters();
    }
#endif

    UpdateIdleMode();

exit:
    return error;
}

Error Mac::SetTemporaryChannel(uint8_t aChannel)
{
    Error error = kErrorNone;

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = kErrorInvalidArgs);

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

    newMask.Intersect(mSupportedChannelMask);
    IgnoreError(Get<Notifier>().Update(mSupportedChannelMask, newMask, kEventSupportedChannelMaskChanged));
}

void Mac::SetPanId(PanId aPanId)
{
    SuccessOrExit(Get<Notifier>().Update(mPanId, aPanId, kEventThreadPanIdChanged));
    mLinks.SetPanId(mPanId);

exit:
    return;
}

void Mac::RequestDirectFrameTransmission(void)
{
    VerifyOrExit(IsEnabled());
    VerifyOrExit(!IsActiveOrPending(kOperationTransmitDataDirect));

    // Ensure direct data frame and data poll TX requests are handled in the
    // order they are requested. If a poll TX request is already pending, it
    // should be sent before this direct data frame.

    mShouldTxPollBeforeData = IsPending(kOperationTransmitPoll);

    StartOperation(kOperationTransmitDataDirect);

exit:
    return;
}

#if OPENTHREAD_FTD
void Mac::RequestIndirectFrameTransmission(void)
{
    VerifyOrExit(IsEnabled());
    VerifyOrExit(!IsActiveOrPending(kOperationTransmitDataIndirect));

    StartOperation(kOperationTransmitDataIndirect);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
void Mac::RequestCslFrameTransmission(void)
{
    VerifyOrExit(mEnabled);
    VerifyOrExit(!IsActiveOrPending(kOperationTransmitDataCsl));

    StartOperation(kOperationTransmitDataCsl);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
void Mac::RequestWakeupFrameTransmission(void)
{
    VerifyOrExit(IsEnabled());
    StartOperation(kOperationTransmitWakeup);

exit:
    return;
}
#endif

Error Mac::RequestDataPollTransmission(void)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveOrPending(kOperationTransmitPoll));

    // Ensure direct data frame and data poll TX requests are handled in the
    // order they are requested. If a direct data frame TX request is already
    // pending, it should be sent before this poll frame.

    mShouldTxPollBeforeData = !IsPending(kOperationTransmitDataDirect);

    StartOperation(kOperationTransmitPoll);

exit:
    return error;
}

void Mac::UpdateIdleMode(void)
{
    bool shouldSleep = !mRxOnWhenIdle && !mPromiscuous;

    VerifyOrExit(mOperation == kOperationIdle);

    if (!mRxOnWhenIdle)
    {
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
        if (mShouldDelaySleep)
        {
            mTimer.Start(kSleepDelay);
            mShouldDelaySleep = false;
            mDelayingSleep    = true;
            LogDebg("Idle mode: Sleep delayed");
        }

        if (mDelayingSleep)
        {
            shouldSleep = false;
        }
#endif
    }

    if (shouldSleep)
    {
        mLinks.Sleep();
        LogDebg("Idle mode: Radio sleeping");
    }
    else
    {
        mLinks.Receive(mRadioChannel);
        LogDebg("Idle mode: Radio receiving on channel %u", mRadioChannel);
    }

exit:
    return;
}

bool Mac::IsActiveOrPending(Operation aOperation) const { return (mOperation == aOperation) || IsPending(aOperation); }

void Mac::StartOperation(Operation aOperation)
{
    if (aOperation != kOperationIdle)
    {
        SetPending(aOperation);

        LogDebg("Request to start operation \"%s\"", OperationToString(aOperation));

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
        if (mDelayingSleep)
        {
            LogDebg("Canceling sleep delay");
            mTimer.Stop();
            mDelayingSleep    = false;
            mShouldDelaySleep = false;
        }
#endif
    }

    if (mOperation == kOperationIdle)
    {
        mOperationTask.Post();
    }
}

void Mac::PerformNextOperation(void)
{
    // Operation priority list to determine the next MAC operation

    static constexpr Operation kOperationPriorityList[] = {
        // `WaitingForData` has the highest priority so that the radio
        // remains in receive mode after a data poll ACK indicating a
        // pending frame from the parent.
        kOperationWaitingForData,
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
        kOperationTransmitWakeup,
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        kOperationTransmitDataCsl,
#endif
        kOperationActiveScan,
        kOperationEnergyScan,
        kOperationTransmitBeacon,
#if OPENTHREAD_FTD
        kOperationTransmitDataIndirect,
#endif
        // `TransmitDataDirect` is listed ahead of `TransmitPoll`, but
        // if both are pending and the poll request was received
        // first, the `mShouldTxPollBeforeData` can flip the order.
        kOperationTransmitDataDirect,
        kOperationTransmitPoll,
    };

    VerifyOrExit(mOperation == kOperationIdle);

    if (!IsEnabled())
    {
        mPendingOperations = 0;
        mTimer.Stop();
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
        mDelayingSleep    = false;
        mShouldDelaySleep = false;
#endif
        ExitNow();
    }

    for (Operation operation : kOperationPriorityList)
    {
        if (IsPending(operation))
        {
            mOperation = operation;
            break;
        }
    }

    if (mShouldTxPollBeforeData && (mOperation == kOperationTransmitDataDirect) && IsPending(kOperationTransmitPoll))
    {
        mOperation = kOperationTransmitPoll;
    }

    if (mOperation != kOperationIdle)
    {
        ClearPending(mOperation);
        LogDebg("Starting operation \"%s\"", OperationToString(mOperation));
        mTimer.Stop(); // Stop the timer before any non-idle operation, have the operation itself be responsible to
                       // start the timer (if it wants to).
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
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
#endif
    case kOperationTransmitPoll:
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    case kOperationTransmitWakeup:
#endif
        BeginTransmit();
        break;

    case kOperationWaitingForData:
        mLinks.Receive(mRadioChannel);
        mTimer.Start(kDataPollTimeout);
        break;
    }

exit:
    return;
}

void Mac::FinishOperation(void)
{
    LogDebg("Finishing operation \"%s\"", OperationToString(mOperation));
    mOperation = kOperationIdle;
}

TxFrame *Mac::PrepareBeaconRequest(TxFrames &aTxFrames)
{
    TxFrame           &frame = aTxFrames.GetBroadcastTxFrame();
    TxFrame::BuildInfo buildInfo;

    buildInfo.mAddrs.mSource.SetNone();
    buildInfo.mAddrs.mDestination.SetShort(kShortAddrBroadcast);
    buildInfo.mPanIds.SetDestination(kShortAddrBroadcast);

    buildInfo.mType      = Frame::kTypeMacCmd;
    buildInfo.mCommandId = Frame::kMacCmdBeaconRequest;
    buildInfo.mVersion   = Frame::kVersion2003;

    buildInfo.PrepareHeadersIn(frame);

    LogInfo("Sending Beacon Request");

    return &frame;
}

TxFrame *Mac::PrepareBeacon(TxFrames &aTxFrames)
{
    TxFrame           *frame;
    TxFrame::BuildInfo buildInfo;
    FrameBuilder       builder;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    OT_ASSERT(!mTxBeaconRadioLinks.IsEmpty());
    frame = &aTxFrames.GetTxFrame(mTxBeaconRadioLinks);
    mTxBeaconRadioLinks.Clear();
#else
    frame = &aTxFrames.GetBroadcastTxFrame();
#endif

    buildInfo.mAddrs.mSource.SetExtended(GetExtAddress());
    buildInfo.mPanIds.SetSource(mPanId);
    buildInfo.mAddrs.mDestination.SetNone();

    buildInfo.mType    = Frame::kTypeBeacon;
    buildInfo.mVersion = Frame::kVersion2003;

    buildInfo.PrepareHeadersIn(*frame);

    builder.Init(frame->GetPayload(), frame->GetMaxPayloadLength());
    builder.Append<BeaconHeader>()->Init();

#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
    builder.Append<BeaconPayload>()->Init(Get<MeshCoP::NetworkIdentity>(), IsJoinable());
#endif

    frame->SetPayloadLength(builder.GetLength());

    LogBeacon("Sending");

    return frame;
}

bool Mac::ShouldSendBeacon(void) const
{
    bool shouldSend = false;

    VerifyOrExit(IsEnabled());

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

void Mac::ProcessTransmitSecurity(TxFrame &aFrame)
{
    KeyManager       &keyManager = Get<KeyManager>();
    uint8_t           keyIdMode;
    const ExtAddress *extAddress = nullptr;

    VerifyOrExit(aFrame.GetSecurityEnabled());

    IgnoreError(aFrame.GetKeyIdMode(keyIdMode));

    switch (keyIdMode)
    {
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode0:
        aFrame.SetAesKey(keyManager.GetKek());
        extAddress = &GetExtAddress();

        if (!aFrame.IsHeaderUpdated())
        {
            aFrame.SetFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode1:

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrame.GetRadioType() == Radio::kTypeIeee802154)
#endif
        {
            // For 15.4 radio link, the AES CCM* and frame security
            // counter (under MAC key ID mode 1) are managed by
            // `SubMac` or `Radio` modules.
            ExitNow();
        }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrame.GetRadioType() == Radio::kTypeTrel)
#endif
        {
            const KeyMaterial *macKey;

            // If the frame header is marked as updated, `MeshForwarder` which
            // prepared the frame should set the frame counter and key id to the
            // same values used in the earlier transmit attempt. For a new frame (header
            // not updated), we get a new frame counter and key id from the key
            // manager.

            if (!aFrame.IsHeaderUpdated())
            {
                mLinks.SetMacFrameCounter(aFrame);
                aFrame.SetKeyIndex(DetermineKeyIndexFor(keyManager.GetCurrentKeySequence()));
            }

            macKey = DetermineMode1Key(aFrame);
            VerifyOrExit(macKey != nullptr);
            aFrame.SetAesKey(*macKey);
            extAddress = &GetExtAddress();
        }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        break;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode2:
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
        if (aFrame.IsWakeupFrame())
        {
            uint8_t keySource[Frame::kKeySourceSizeMode2];

            // Just set the key source here, further security processing will happen in SubMac
            BigEndian::WriteUint32(keyManager.GetCurrentKeySequence(), keySource);
            aFrame.SetKeySource(keySource);
            ExitNow();
        }
#endif
        aFrame.SetAesKey(mMode2KeyMaterial);

        mKeyIdMode2FrameCounter++;
        aFrame.SetFrameCounter(mKeyIdMode2FrameCounter);
        aFrame.SetKeySource(kMode2KeySource);
        aFrame.SetKeyIndex(0xff);
        extAddress = &AsCoreType(&kMode2ExtAddress);
        break;

    default:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(!aFrame.Has<TimeIe>());
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(!aFrame.IsCslIePresent());
#endif

    aFrame.ProcessTransmitAesCcm(*extAddress);

exit:
    return;
}

void Mac::BeginTransmit(void)
{
    TxFrame  *frame    = nullptr;
    TxFrames &txFrames = mLinks.InitTxFrames();
    Address   dstAddr;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mTxPendingRadioLinks.Clear();
    mTxError = kErrorAbort;
#endif

    VerifyOrExit(IsEnabled());

    switch (mOperation)
    {
    case kOperationActiveScan:
        mLinks.SetPanId(kPanIdBroadcast);
        frame = PrepareBeaconRequest(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mScanChannel);
        frame->SetSequence(0);
        frame->SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        frame->SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitBeacon:
        frame = PrepareBeacon(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mRadioChannel);
        frame->SetSequence(mBeaconSequence++);
        frame->SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        frame->SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitPoll:
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        frame = Get<DataPollSender>().PrepareDataRequest(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetSequence(mDataSequence++);
        break;

    case kOperationTransmitDataDirect:
        // Set channel and retry counts on all TxFrames before asking
        // the next layer (`MeshForwarder`) to prepare the frame. This
        // allows next layer to possibility change these parameters.
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        frame = Get<MeshForwarder>().HandleFrameRequest(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetSequence(mDataSequence++);
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsIndirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesIndirect);
        frame = Get<DataPollHandler>().HandleFrameRequest(txFrames);
        VerifyOrExit(frame != nullptr);

        // If the frame is marked as retransmission, then data sequence number is already set.
        if (!frame->IsARetransmission())
        {
            frame->SetSequence(mDataSequence++);
        }
        break;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsCsl);
        txFrames.SetMaxFrameRetries(kMaxFrameRetriesCsl);
        frame = Get<CslTxScheduler>().HandleFrameRequest(txFrames);
        VerifyOrExit(frame != nullptr);

        // If the frame is marked as retransmission, then data sequence number is already set.
        if (!frame->IsARetransmission())
        {
            frame->SetSequence(mDataSequence++);
        }

        break;

#endif

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    case kOperationTransmitWakeup:
        frame = Get<WakeupTxScheduler>().PrepareWakeupFrame(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mWakeupChannel);
        frame->SetRxChannelAfterTxDone(mRadioChannel);
        break;
#endif

    default:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        TimeIe *timeIe = frame->Find<TimeIe>();

        if (timeIe == nullptr)
        {
            frame->SetTimeIeOffset(0);
        }
        else
        {
            uint8_t offset = static_cast<uint8_t>(timeIe->GetData() - frame->GetPsdu());

            frame->SetTimeIeOffset(offset);
            frame->SetTimeSyncSeq(Get<TimeSync>().GetTimeSyncSeq());
            frame->SetNetworkTimeOffset(Get<TimeSync>().GetNetworkTimeOffset());
        }
    }
#endif

    if (!frame->IsSecurityProcessed())
    {
#if OPENTHREAD_CONFIG_MULTI_RADIO
        // Go through all selected radio link types for this tx and
        // copy the frame into correct `TxFrame` for each radio type
        // (if it is not already prepared).

        for (Radio::Type radio : Radio::Types::kAllTypes)
        {
            if (txFrames.GetSelectedRadioTypes().Contains(radio))
            {
                TxFrame &txFrame = txFrames.GetTxFrame(radio);

                if (txFrame.IsEmpty())
                {
                    txFrame.CopyFrom(*frame);
                }
            }
        }

        // Go through all selected radio link types for this tx and
        // process security for each radio type separately. This
        // allows radio links to handle security differently, e.g.,
        // with different keys or link frame counters.
        for (Radio::Type radio : Radio::Types::kAllTypes)
        {
            if (txFrames.GetSelectedRadioTypes().Contains(radio))
            {
                ProcessTransmitSecurity(txFrames.GetTxFrame(radio));
            }
        }
#else
        ProcessTransmitSecurity(*frame);
#endif
    }

    mBroadcastTransmitCount = 0;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mTxPendingRadioLinks = txFrames.GetSelectedRadioTypes();

    // If the "required radio type set" is empty,`mTxError` starts as
    // `kErrorAbort`. In this case, successful tx over any radio
    // link is sufficient for overall tx to be considered successful.
    // When the "required radio type set" is not empty, `mTxError`
    // starts as `kErrorNone` and we update it if tx over any link
    // in the required set fails.

    if (!txFrames.GetRequiredRadioTypes().IsEmpty())
    {
        mTxError = kErrorNone;
    }
#endif

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    if (!mRxOnWhenIdle && !mPromiscuous)
    {
        mShouldDelaySleep = frame->GetFramePending();
        LogDebg("Delay sleep for pending tx");
    }
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mLinks.Send(*frame, mTxPendingRadioLinks);
#else
    mLinks.Send();
#endif

exit:

    if (frame == nullptr)
    {
        // If the frame could not be prepared and the tx is being
        // aborted, we set the frame length to zero to mark it as empty.
        // The empty frame helps differentiate between an aborted tx due
        // to OpenThread itself not being able to prepare the frame, versus
        // the radio platform aborting the tx operation.

        frame = &txFrames.GetBroadcastTxFrame();
        frame->SetLength(0);
        HandleTransmitDone(*frame, nullptr, kErrorAbort);
    }
}

void Mac::RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel)
{
    if (!aCcaSuccess)
    {
        mCounters.mTxErrCca++;
    }

    // Only track the CCA success rate for frame transmissions
    // on the PAN channel or the CSL channel.

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if ((aChannel == mPanChannel) || (IsCslEnabled() && (aChannel == mCslChannel)))
#else
    if (aChannel == mPanChannel)
#endif
    {
        if (mCcaSampleCount < kMaxCcaSampleCount)
        {
            mCcaSampleCount++;
        }

        mCcaSuccessRateTracker.AddSample(aCcaSuccess, mCcaSampleCount);
    }
}

void Mac::RecordFrameTransmitStatus(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx)
{
    bool      ackRequested = aFrame.GetAckRequest();
    Address   dstAddr;
    Neighbor *neighbor;

    VerifyOrExit(!aFrame.IsEmpty());

    IgnoreError(aFrame.GetDstAddr(dstAddr));
    neighbor = Get<NeighborTable>().FindNeighbor(dstAddr);

    // Record frame transmission success/failure state (for a neighbor).

    if ((neighbor != nullptr) && ackRequested)
    {
        bool frameTxSuccess = true;

        // CCA or abort errors are excluded from frame tx error
        // rate tracking, since when they occur, the frame is
        // not actually sent over the air.

        switch (aError)
        {
        case kErrorNoAck:
            frameTxSuccess = false;

            OT_FALL_THROUGH;

        case kErrorNone:
            neighbor->GetLinkInfo().AddFrameTxStatus(frameTxSuccess);
            break;

        default:
            break;
        }
    }

    // Log frame transmission failure.

    if (aError != kErrorNone)
    {
        LogFrameTxFailure(aFrame, aError, aRetryCount, aWillRetx);
        DumpDebg("TX ERR", aFrame.GetHeader(), 16);

        if (aWillRetx)
        {
            mCounters.mTxRetry++;

            // Since this failed transmission will be retried by `SubMac` layer
            // there is no need to update any other MAC counter. MAC counters
            // are updated on the final transmission attempt.

            ExitNow();
        }
    }

    // Update MAC counters.

    mCounters.mTxTotal++;

    if (aError == kErrorAbort)
    {
        mCounters.mTxErrAbort++;
    }

    if (aError == kErrorChannelAccessFailure)
    {
        mCounters.mTxErrBusyChannel++;
    }

    if (ackRequested)
    {
        mCounters.mTxAckRequested++;

        if (aError == kErrorNone)
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

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Error Mac::ProcessTxDone(TxFrame &aFrame, RxFrame *aAckFrame, Error &aError)
{
    // Process post-transmission actions on IEEE 802.15.4 link
    // (handling broadcast retransmissions and ACK processing).
    //
    // Returns `kErrorPending` if a broadcast frame is scheduled for
    // retransmission (indicating overall frame transmission is not yet
    // finished). Returns `kErrorNone` otherwise.
    //
    // May update `aError` (e.g., setting it to `kErrorNoAck` if Enh-ACK
    // security or MAC filter checks fail).

    Error     error = kErrorNone;
    Address   dstAddr;
    Neighbor *neighbor;

    VerifyOrExit(!aFrame.IsEmpty());

#if OPENTHREAD_CONFIG_MULTI_RADIO
    VerifyOrExit(aFrame.GetRadioType() == Radio::kTypeIeee802154);

    // Set the radio type on `AckFrame`, so we can determine the
    // proper (15.4 based) key in `ProcessEnhAckSecurity()`.

    if (aAckFrame != nullptr)
    {
        aAckFrame->SetRadioType(Radio::kTypeIeee802154);
    }
#endif

    IgnoreError(aFrame.GetDstAddr(dstAddr));

    // Determine whether to re-transmit a broadcast frame.

    if (dstAddr.IsBroadcast())
    {
        mBroadcastTransmitCount++;

        if (mBroadcastTransmitCount < kTxNumBcast)
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            {
                Radio::Types radioTypes;

                radioTypes.Add(Radio::kTypeIeee802154);
                mLinks.Send(aFrame, radioTypes);
            }
#else
            mLinks.Send();
#endif
            ExitNow(error = kErrorPending);
        }

        mBroadcastTransmitCount = 0;
    }

    // If an ACK was requested and received, process the ACK frame
    // (verifying MAC filter, Enh-ACK security, and updating
    // neighbor link info and CSL).

    VerifyOrExit(aFrame.GetAckRequest() && (aAckFrame != nullptr));

    SuccessOrExit(aError);

    neighbor = Get<NeighborTable>().FindNeighbor(dstAddr);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if ((neighbor != nullptr) && mFilter.ApplyToRxFrame(*aAckFrame, neighbor->GetExtAddress(), neighbor) != kErrorNone)
    {
        aError = kErrorNoAck;
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    if (ProcessEnhAckSecurity(aFrame, *aAckFrame) != kErrorNone)
    {
        aError = kErrorNoAck;
        ExitNow();
    }
#endif

    VerifyOrExit(neighbor != nullptr);

    UpdateNeighborLinkInfo(*neighbor, *aAckFrame);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    ProcessEnhAckProbing(*aAckFrame, *neighbor);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ProcessCsl(*aAckFrame, dstAddr);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (!mRxOnWhenIdle && aFrame.Has<CslIe>())
    {
        Get<DataPollSender>().ResetKeepAliveTimer();
    }
#endif

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO

Error Mac::ProcessMultiRadioTxDone(TxFrame &aFrame, Error &aError)
{
    // Process post-transmission actions under multi-radio config
    // (updating radio selector and tracking transmission across
    // multiple radio links).
    //
    // Returns `kErrorPending` if transmissions on other radio links are
    // still pending. Returns `kErrorNone` once all radio links have
    // completed and updates `aError` with the overall transmission
    // result (`mTxError`).

    Error        error = kErrorNone;
    Radio::Type  radio;
    Radio::Types requiredRadios;

    VerifyOrExit(!aFrame.IsEmpty());

    radio          = aFrame.GetRadioType();
    requiredRadios = mLinks.GetTxFramesRequiredRadioTypes();

    Get<RadioSelector>().UpdateOnSendDone(aFrame, aError);

    if (requiredRadios.IsEmpty())
    {
        // If the "required radio type set" is empty, successful
        // tx over any radio link is sufficient for overall tx to
        // be considered successful. In this case `mTxError`
        // starts as `kErrorAbort` and we update it only when
        // it is not already `kErrorNone`.

        if (mTxError != kErrorNone)
        {
            mTxError = aError;
        }
    }
    else
    {
        // When the "required radio type set" is not empty we
        // expect the successful frame tx on all links in this set
        // to consider the overall tx successful. In this case,
        // `mTxError` starts as `kErrorNone` and we update it
        // if tx over any link in the set fails.

        if (requiredRadios.Contains(radio) && (aError != kErrorNone))
        {
            LogDebgOnError(aError, "tx frame on required radio link %s", Radio::TypeToString(radio));
            mTxError = aError;
        }
    }

    // Keep track of radio links on which the frame is sent
    // and wait for all radio links to finish.
    mTxPendingRadioLinks.Remove(radio);

    if (!mTxPendingRadioLinks.IsEmpty())
    {
        ExitNow(error = kErrorPending);
    }

    aError = mTxError;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

void Mac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    SuccessOrExit(ProcessTxDone(aFrame, aAckFrame, aError));
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    SuccessOrExit(ProcessMultiRadioTxDone(aFrame, aError));
#endif

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
        OT_ASSERT(aFrame.IsEmpty() || aFrame.GetAckRequest());

        if ((aError == kErrorNone) && (aAckFrame != nullptr))
        {
            bool framePending = aAckFrame->GetFramePending();

            if (IsEnabled() && framePending)
            {
                StartOperation(kOperationWaitingForData);
            }

            LogInfo("Sent data poll, fp:%s", ToYesNo(framePending));
        }

        mCounters.mTxDataPoll++;
        FinishOperation();
        Get<DataPollSender>().HandlePollSent(aFrame, aError);
        PerformNextOperation();
        break;

    case kOperationTransmitDataDirect:
        mCounters.mTxData++;

        if (aError != kErrorNone)
        {
            mCounters.mTxDirectMaxRetryExpiry++;
        }
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
        else
        {
            mRetryHistogram.RecordDirectTx(mLinks.GetTransmitRetries());
        }
#endif

        DumpDebg("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<MeshForwarder>().HandleSentFrame(aFrame, aError);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        Get<DataPollSender>().ProcessTxDone(aFrame, aAckFrame, aError);
#endif
        PerformNextOperation();
        break;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        mCounters.mTxData++;

        DumpDebg("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<CslTxScheduler>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();

        break;
#endif

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        mCounters.mTxData++;

        if (aError != kErrorNone)
        {
            mCounters.mTxIndirectMaxRetryExpiry++;
        }
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
        else
        {
            mRetryHistogram.RecordIndirectTx(mLinks.GetTransmitRetries());
        }
#endif

        DumpDebg("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<DataPollHandler>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();
        break;
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    case kOperationTransmitWakeup:
        FinishOperation();
        PerformNextOperation();
        break;
#endif

    default:
        OT_ASSERT(false);
    }

    ExitNow(); // Added to suppress "unused label exit" warning (in TREL radio only).

exit:
    return;
}

void Mac::HandleTimer(void)
{
    switch (mOperation)
    {
    case kOperationActiveScan:
        PerformActiveScan();
        break;

    case kOperationWaitingForData:
        LogDebg("Data poll timeout");
        FinishOperation();
        Get<DataPollSender>().HandlePollTimeout();
        PerformNextOperation();
        break;

    case kOperationIdle:
        if (!mRxOnWhenIdle)
        {
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
            if (mDelayingSleep)
            {
                LogDebg("Sleep delay timeout expired");
                mDelayingSleep = false;
                UpdateIdleMode();
            }
#endif
        }
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        else if (IsPending(kOperationTransmitDataCsl))
        {
            PerformNextOperation();
        }
#endif
        break;

    default:
        OT_ASSERT(false);
    }
}

const KeyMaterial *Mac::DetermineMode1Key(const Frame &aFrame) const
{
    uint32_t keySequence;

    return DetermineMode1KeyAndSequence(aFrame, keySequence);
}

const KeyMaterial *Mac::DetermineMode1KeyAndSequence(const Frame &aFrame, uint32_t &aKeySequence) const
{
    // Determines the MAC key and key sequence for given `aFrame`.
    // The caller MUST already ensure that the frame's Key ID Mode
    // is Mode 1.

    const KeyMaterial *key = nullptr;
    uint8_t            keyIndex;
    KeyTrio::Type      keyType;

    SuccessOrExit(aFrame.GetKeyIndex(keyIndex));
    aKeySequence = Get<KeyManager>().GetCurrentKeySequence();

    if (keyIndex == DetermineKeyIndexFor(aKeySequence))
    {
        keyType = KeyTrio::kCur;
    }
    else if (keyIndex == DetermineKeyIndexFor(aKeySequence + 1))
    {
        aKeySequence++;
        keyType = KeyTrio::kNext;
    }
    else if (keyIndex == DetermineKeyIndexFor(aKeySequence - 1))
    {
        aKeySequence--;
        keyType = KeyTrio::kPrev;
    }
    else
    {
        ExitNow();
    }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Radio::kTypeIeee802154)
#endif
    {
        ExitNow(key = &Get<SubMac>().GetMacKey(keyType));
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Radio::kTypeTrel)
#endif
    {
        switch (keyType)
        {
        case KeyTrio::kCur:
            key = &Get<KeyManager>().GetCurrentTrelMacKey();
            break;
        case KeyTrio::kNext:
        case KeyTrio::kPrev:
            key = &Get<KeyManager>().GetTemporaryTrelMacKey(aKeySequence);
            break;
        }

        ExitNow();
    }
#endif

exit:
    return key;
}

Error Mac::ProcessReceiveSecurity(RxFrame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager        &keyManager = Get<KeyManager>();
    Error              error      = kErrorSecurity;
    uint8_t            securityLevel;
    uint8_t            keyIdMode;
    uint32_t           frameCounter;
    uint32_t           keySequence = 0;
    const KeyMaterial *macKey;
    const ExtAddress  *extAddress;

    VerifyOrExit(aFrame.GetSecurityEnabled(), error = kErrorNone);

    IgnoreError(aFrame.GetSecurityLevel(securityLevel));
    VerifyOrExit(securityLevel == Frame::kSecurityEncMic32);

    IgnoreError(aFrame.GetFrameCounter(frameCounter));
    LogDebg("Rx security - frame counter %lu", ToUlong(frameCounter));

    IgnoreError(aFrame.GetKeyIdMode(keyIdMode));

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit(keyManager.IsKekSet(), error = kErrorSecurity);
        macKey     = &keyManager.GetKek();
        extAddress = &aSrcAddr.GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != nullptr);

        macKey = DetermineMode1KeyAndSequence(aFrame, keySequence);
        VerifyOrExit(macKey != nullptr);

        // If the frame is from a neighbor not in valid state (e.g., it is from a child being
        // restored), skip the key sequence and frame counter checks but continue to verify
        // the tag/MIC. Such a frame is later filtered in `RxDoneTask` which only allows MAC
        // Data Request frames from a child being restored.

        if (aNeighbor->IsStateValid())
        {
            VerifyOrExit(keySequence >= aNeighbor->GetKeySequence());

            if (keySequence == aNeighbor->GetKeySequence())
            {
                uint32_t neighborFrameCounter;

#if OPENTHREAD_CONFIG_MULTI_RADIO
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get(aFrame.GetRadioType());
#else
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get();
#endif

                // If frame counter is one off, then frame is a duplicate.
                VerifyOrExit((frameCounter + 1) != neighborFrameCounter, error = kErrorDuplicated);

                VerifyOrExit(frameCounter >= neighborFrameCounter);
            }
        }

        extAddress = &aSrcAddr.GetExtended();

        break;

    case Frame::kKeyIdMode2:
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        if (aFrame.IsWakeupFrame())
        {
            uint32_t sequence;
            uint8_t  keyIndex;

            // TODO: Avoid generating a new key if a wake-up frame was recently received already

            IgnoreError(aFrame.GetKeyIndex(keyIndex));
            sequence = BigEndian::ReadUint32(aFrame.GetKeySource());
            VerifyOrExit(DetermineKeyIndexFor(sequence) == keyIndex, error = kErrorSecurity);

            macKey     = &keyManager.GetTemporaryMacKey(sequence);
            extAddress = &aSrcAddr.GetExtended();
        }
        else
#endif
        {
            macKey     = &mMode2KeyMaterial;
            extAddress = &AsCoreType(&kMode2ExtAddress);
        }
        break;

    default:
        ExitNow();
    }

    SuccessOrExit(aFrame.ProcessReceiveAesCcm(*extAddress, *macKey));

    if ((keyIdMode == Frame::kKeyIdMode1) && aNeighbor->IsStateValid())
    {
        if (aNeighbor->GetKeySequence() != keySequence)
        {
            aNeighbor->SetKeySequence(keySequence);
            aNeighbor->SetMleFrameCounter(0);
            aNeighbor->GetLinkFrameCounters().Reset();
        }

#if OPENTHREAD_CONFIG_MULTI_RADIO
        aNeighbor->GetLinkFrameCounters().Set(aFrame.GetRadioType(), frameCounter + 1);
#else
        aNeighbor->GetLinkFrameCounters().Set(frameCounter + 1);
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrame.GetRadioType() == Radio::kTypeIeee802154)
#endif
        {
            if ((frameCounter + 1) > aNeighbor->GetLinkAckFrameCounter())
            {
                aNeighbor->SetLinkAckFrameCounter(frameCounter + 1);
            }
        }
#endif

        if (keySequence > keyManager.GetCurrentKeySequence())
        {
            keyManager.SetCurrentKeySequence(keySequence, KeyManager::kApplySwitchGuard | KeyManager::kResetGuardTimer);
        }
    }

    error = kErrorNone;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
Error Mac::ProcessEnhAckSecurity(TxFrame &aTxFrame, RxFrame &aAckFrame)
{
    Error              error = kErrorSecurity;
    uint8_t            securityLevel;
    uint8_t            txKeyIndex;
    uint8_t            ackKeyIndex;
    uint8_t            keyIdMode;
    uint32_t           frameCounter;
    Address            srcAddr;
    Address            dstAddr;
    Neighbor          *neighbor = nullptr;
    const KeyMaterial *macKey;

    VerifyOrExit(aAckFrame.GetSecurityEnabled(), error = kErrorNone);
    VerifyOrExit(aAckFrame.IsVersion2015());

    SuccessOrExit(aAckFrame.ValidatePsdu());

    IgnoreError(aAckFrame.GetSecurityLevel(securityLevel));
    VerifyOrExit(securityLevel == Frame::kSecurityEncMic32);

    IgnoreError(aAckFrame.GetKeyIdMode(keyIdMode));
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1);

    IgnoreError(aTxFrame.GetKeyIndex(txKeyIndex));
    IgnoreError(aAckFrame.GetKeyIndex(ackKeyIndex));

    VerifyOrExit(txKeyIndex == ackKeyIndex);

    IgnoreError(aAckFrame.GetFrameCounter(frameCounter));
    LogDebg("Rx security - Ack frame counter %lu", ToUlong(frameCounter));

    IgnoreError(aAckFrame.GetSrcAddr(srcAddr));

    if (!srcAddr.IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(srcAddr);
    }
    else
    {
        IgnoreError(aTxFrame.GetDstAddr(dstAddr));

        if (!dstAddr.IsNone())
        {
            // Get neighbor from destination address of transmitted frame
            neighbor = Get<NeighborTable>().FindNeighbor(dstAddr);
        }
    }

    if (!srcAddr.IsExtended() && neighbor != nullptr)
    {
        srcAddr.SetExtended(neighbor->GetExtAddress());
    }

    VerifyOrExit(srcAddr.IsExtended() && neighbor != nullptr);

    macKey = DetermineMode1Key(aAckFrame);
    VerifyOrExit(macKey != nullptr);

    if (neighbor->IsStateValid())
    {
        VerifyOrExit(frameCounter >= neighbor->GetLinkAckFrameCounter());
    }

    error = aAckFrame.ProcessReceiveAesCcm(srcAddr.GetExtended(), *macKey);
    SuccessOrExit(error);

    if (neighbor->IsStateValid())
    {
        neighbor->SetLinkAckFrameCounter(frameCounter + 1);
    }

exit:
    if (error != kErrorNone)
    {
        LogInfo("Frame tx attempt failed, error: Enh-ACK security check fail");
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

Error Mac::FilterDestShortAddress(ShortAddress aDestAddress) const
{
    Error error = kErrorNone;

    if (aDestAddress == GetShortAddress())
    {
        ExitNow();
    }

#if OPENTHREAD_FTD
    if ((GetAlternateShortAddress() != kShortAddrInvalid) && (aDestAddress == GetAlternateShortAddress()))
    {
        ExitNow();
    }
#endif

    if (mRxOnWhenIdle && (aDestAddress == kShortAddrBroadcast))
    {
        ExitNow();
    }

    error = kErrorDestinationAddressFiltered;

exit:
    return error;
}

void Mac::HandleReceivedFrame(RxFrame *aFrame, Error aError)
{
    Address   srcaddr;
    Address   dstaddr;
    PanId     panid;
    Neighbor *neighbor;
    Error     error            = aError;
    bool      isFrameValidated = false;

    mCounters.mRxTotal++;

    SuccessOrExit(error);
    VerifyOrExit(aFrame != nullptr, error = kErrorNoFrameReceived);
    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    isFrameValidated = true;

    IgnoreError(aFrame->GetSrcAddr(srcaddr));
    IgnoreError(aFrame->GetDstAddr(dstaddr));
    neighbor = !srcaddr.IsNone() ? Get<NeighborTable>().FindNeighbor(srcaddr) : nullptr;

    // Destination Address Filtering
    switch (dstaddr.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        SuccessOrExit(error = FilterDestShortAddress(dstaddr.GetShort()));

#if OPENTHREAD_FTD
        // Allow multicasts from neighbor routers if FTD
        if (neighbor == nullptr && dstaddr.IsBroadcast() && Get<Mle::Mle>().IsFullThreadDevice())
        {
            neighbor = Get<NeighborTable>().FindRxOnlyNeighborRouter(srcaddr);
        }
#endif

        break;

    case Address::kTypeExtended:
        VerifyOrExit(dstaddr.GetExtended() == GetExtAddress(), error = kErrorDestinationAddressFiltered);
        break;
    }

    // Verify destination PAN ID if present
    if (kErrorNone == aFrame->GetDstPanId(panid))
    {
        VerifyOrExit(panid == kShortAddrBroadcast || panid == mPanId, error = kErrorDestinationAddressFiltered);
    }

    // Source Address Filtering
    switch (srcaddr.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        LogDebg("Received frame from short address 0x%04x", srcaddr.GetShort());

        VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);

        srcaddr.SetExtended(neighbor->GetExtAddress());

        OT_FALL_THROUGH;

    case Address::kTypeExtended:

        // Duplicate Address Protection
        VerifyOrExit(srcaddr.GetExtended() != GetExtAddress(), error = kErrorInvalidSourceAddress);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        SuccessOrExit(error = mFilter.ApplyToRxFrame(*aFrame, srcaddr.GetExtended(), neighbor));
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
    case kErrorDuplicated:

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

        OT_FALL_THROUGH;

    case kErrorNone:
        break;

    default:
        ExitNow();
    }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ProcessCsl(*aFrame, srcaddr);
#endif

    Get<DataPollSender>().ProcessRxFrame(*aFrame);

    if (neighbor != nullptr)
    {
        UpdateNeighborLinkInfo(*neighbor, *aFrame);

        if (aFrame->GetSecurityEnabled())
        {
            uint8_t keyIdMode;

            IgnoreError(aFrame->GetKeyIdMode(keyIdMode));

            if (keyIdMode == Frame::kKeyIdMode1)
            {
                switch (neighbor->GetState())
                {
                case Neighbor::kStateValid:
                    break;

                case Neighbor::kStateRestored:
                case Neighbor::kStateChildUpdateRequest:

                    // Only accept a "MAC Data Request" frame from a child being restored.
                    VerifyOrExit(aFrame->IsDataRequestCommand(), error = kErrorDrop);
                    break;

                default:
                    ExitNow(error = kErrorUnknownNeighbor);
                }

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2 && OPENTHREAD_FTD
                // From Thread 1.2, MAC Data Frame can also act as keep-alive message if child supports
                if (aFrame->GetType() == Frame::kTypeData && !neighbor->IsRxOnWhenIdle() &&
                    neighbor->IsEnhancedKeepAliveSupported())
                {
                    neighbor->SetLastHeard(TimerMilli::GetNow());
                }
#endif
            }

#if OPENTHREAD_CONFIG_MULTI_RADIO
            Get<RadioSelector>().UpdateOnReceive(*neighbor, aFrame->GetRadioType(), /* aIsDuplicate */ false);
#endif
        }
    }

    switch (mOperation)
    {
    case kOperationActiveScan:

        if (aFrame->GetType() == Frame::kTypeBeacon)
        {
            mCounters.mRxBeacon++;
            ReportActiveScanResult(aFrame);
            ExitNow();
        }

        OT_FALL_THROUGH;

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
                LogDebg("Delay sleep for pending rx");
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
    case Frame::kTypeMacCmd:
        if (HandleMacCommand(*aFrame)) // returns `true` when handled
        {
            ExitNow(error = kErrorNone);
        }

        break;

    case Frame::kTypeBeacon:
        mCounters.mRxBeacon++;
        break;

    case Frame::kTypeData:
        mCounters.mRxData++;
        break;

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    case Frame::kTypeMultipurpose:
        SuccessOrExit(error = HandleWakeupFrame(*aFrame));
        OT_FALL_THROUGH;
#endif

    default:
        mCounters.mRxOther++;
        ExitNow();
    }

    DumpDebg("RX", aFrame->GetHeader(), aFrame->GetLength());
    Get<MeshForwarder>().HandleReceivedFrame(*aFrame);

    UpdateIdleMode();

exit:

    if (error != kErrorNone)
    {
        LogFrameRxFailure(isFrameValidated ? aFrame : nullptr, error);

        switch (error)
        {
        case kErrorSecurity:
            mCounters.mRxErrSec++;
            break;

        case kErrorFcs:
            mCounters.mRxErrFcs++;
            break;

        case kErrorNoFrameReceived:
            mCounters.mRxErrNoFrame++;
            break;

        case kErrorUnknownNeighbor:
            mCounters.mRxErrUnknownNeighbor++;
            break;

        case kErrorInvalidSourceAddress:
            mCounters.mRxErrInvalidSrcAddr++;
            break;

        case kErrorAddressFiltered:
            mCounters.mRxAddressFiltered++;
            break;

        case kErrorDestinationAddressFiltered:
            mCounters.mRxDestAddrFiltered++;
            break;

        case kErrorDuplicated:
            mCounters.mRxDuplicated++;
            break;

        default:
            mCounters.mRxErrOther++;
            break;
        }
    }

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame->GetRadioType() == Radio::kTypeTrel)
#endif
    {
        if (error == kErrorNone)
        {
            // If the received frame is using TREL and is successfully
            // processed, check for any discrepancy between the socket
            // address of the received TREL packet and the information
            // saved in the corresponding TREL peer, and signal this to
            // the platform layer.
            //
            // If the frame used link security and was successfully
            // processed, we allow the `Peer` entry socket information
            // to be updated directly.

            Get<Trel::Link>().CheckPeerAddrOnRxSuccess(aFrame->GetSecurityEnabled()
                                                           ? Trel::Link::kAllowPeerSockAddrUpdate
                                                           : Trel::Link::kDisallowPeerSockAddrUpdate);
        }
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
}

void Mac::UpdateNeighborLinkInfo(Neighbor &aNeighbor, const RxFrame &aRxFrame)
{
    LinkQuality oldLinkQuality = aNeighbor.GetLinkInfo().GetLinkQualityIn();

    aNeighbor.GetLinkInfo().AddRss(aRxFrame.GetRssi());

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    aNeighbor.AggregateLinkMetrics(/* aSeriesId */ 0, aRxFrame.GetType(), aRxFrame.GetLqi(), aRxFrame.GetRssi());
#endif

    // Signal when `aNeighbor` is the current parent and its link
    // quality gets changed.

    VerifyOrExit(Get<Mle::Mle>().IsChild() && (&aNeighbor == &Get<Mle::Mle>().GetParent()));
    VerifyOrExit(aNeighbor.GetLinkInfo().GetLinkQualityIn() != oldLinkQuality);
    Get<Notifier>().Signal(kEventParentLinkQualityChanged);

exit:
    return;
}

bool Mac::HandleMacCommand(RxFrame &aFrame)
{
    bool    didHandle = false;
    uint8_t commandId;

    IgnoreError(aFrame.GetCommandId(commandId));

    switch (commandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        LogInfo("Received Beacon Request");

        if (ShouldSendBeacon())
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            mTxBeaconRadioLinks.Add(aFrame.GetRadioType());
#endif
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
    Get<Radio::Radio>().SetPromiscuous(aPromiscuous);

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    mDelayingSleep    = false;
    mShouldDelaySleep = false;
#endif

    mLinks.SetRxOnWhenIdle(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();
}

Error Mac::SetRegion(uint16_t aRegionCode)
{
    Error       error;
    ChannelMask oldMask = mSupportedChannelMask;

    SuccessOrExit(error = Get<Radio::Radio>().SetRegion(aRegionCode));
    mSupportedChannelMask.SetMask(Get<Radio::Radio>().GetSupportedChannelMask());
    IgnoreError(Get<Notifier>().Update(oldMask, mSupportedChannelMask, kEventSupportedChannelMaskChanged));

exit:
    return error;
}

Error Mac::GetRegion(uint16_t &aRegionCode) const { return Get<Radio::Radio>().GetRegion(aRegionCode); }

#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
const uint32_t *Mac::GetDirectRetrySuccessHistogram(uint16_t &aSize) const
{
    aSize = Min<uint16_t>(RetryHistogram::kMaxDirect, static_cast<uint16_t>(mMaxFrameRetriesDirect) + 1);
    return mRetryHistogram.mDirect;
}

#if OPENTHREAD_FTD
const uint32_t *Mac::GetIndirectRetrySuccessHistogram(uint16_t &aSize) const
{
    aSize = Min<uint16_t>(RetryHistogram::kMaxIndirect, static_cast<uint16_t>(mMaxFrameRetriesIndirect) + 1);
    return mRetryHistogram.mIndirect;
}
#endif
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

uint8_t Mac::ComputeLinkMargin(int8_t aRss) const { return ot::ComputeLinkMargin(GetNoiseFloor(), aRss); }

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Mac::OperationToString(Operation aOperation)
{
#define OperationMapList(_)                               \
    _(kOperationIdle, "Idle")                             \
    _(kOperationActiveScan, "ActiveScan")                 \
    _(kOperationEnergyScan, "EnergyScan")                 \
    _(kOperationTransmitBeacon, "TransmitBeacon")         \
    _(kOperationTransmitDataDirect, "TransmitDataDirect") \
    _(kOperationTransmitPoll, "TransmitPoll")             \
    _(kOperationWaitingForData, "WaitingForData")         \
    FtdOperationMapList(_) CslTxOperationMapList(_) WakeupOperationMapList(_)

#if OPENTHREAD_FTD
#define FtdOperationMapList(_) _(kOperationTransmitDataIndirect, "TransmitDataIndirect")
#else
#define FtdOperationMapList(_)
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
#define CslTxOperationMapList(_) _(kOperationTransmitDataCsl, "TransmitDataCsl")
#else
#define CslTxOperationMapList(_)
#endif

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
#define WakeupOperationMapList(_) _(kOperationTransmitWakeup, "TransmitWakeup")
#else
#define WakeupOperationMapList(_)
#endif

    DefineEnumStringArray(OperationMapList);

    return kStrings[aOperation];
}

void Mac::LogFrameRxFailure(const RxFrame *aFrame, Error aError) const
{
    LogLevel logLevel;

    switch (aError)
    {
    case kErrorAbort:
    case kErrorNoFrameReceived:
    case kErrorAddressFiltered:
    case kErrorDestinationAddressFiltered:
        logLevel = kLogLevelDebg;
        break;

    default:
        logLevel = kLogLevelInfo;
        break;
    }

    if (aFrame == nullptr)
    {
        LogAt(logLevel, "Frame rx failed, error:%s", ErrorToString(aError));
    }
    else
    {
        LogAt(logLevel, "Frame rx failed, error:%s, %s", ErrorToString(aError), aFrame->ToInfoString().AsCString());
    }
}

void Mac::LogFrameTxFailure(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx) const
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Radio::kTypeIeee802154)
#endif
    {
        uint8_t maxAttempts = aFrame.GetMaxFrameRetries() + 1;
        uint8_t curAttempt  = aWillRetx ? (aRetryCount + 1) : maxAttempts;

        LogInfo("Frame tx attempt %u/%u failed, error:%s, %s", curAttempt, maxAttempts, ErrorToString(aError),
                aFrame.ToInfoString().AsCString());
    }
#else
    OT_UNUSED_VARIABLE(aRetryCount);
    OT_UNUSED_VARIABLE(aWillRetx);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Radio::kTypeTrel)
#endif
    {
        if (Get<Trel::Interface>().IsEnabled())
        {
            LogInfo("Frame tx failed, error:%s, %s", ErrorToString(aError), aFrame.ToInfoString().AsCString());
        }
    }
#endif
}

void Mac::LogBeacon(const char *aActionText) const { LogInfo("%s Beacon", aActionText); }

#else // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Mac::LogFrameRxFailure(const RxFrame *, Error) const {}

void Mac::LogBeacon(const char *) const {}

void Mac::LogFrameTxFailure(const TxFrame &, Error, uint8_t, bool) const {}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

// LCOV_EXCL_STOP

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Mac::SetCslCapable(bool aIsCslCapable)
{
    VerifyOrExit(mIsCslCapable != aIsCslCapable);
    mIsCslCapable = aIsCslCapable;
    UpdateCslState();

exit:
    return;
}

void Mac::SetCslChannel(uint8_t aChannel)
{
    VerifyOrExit(mCslChannel != aChannel);
    mCslChannel = aChannel;
    UpdateCslParameters();

exit:
    return;
}

void Mac::SetCslPeriod(uint16_t aPeriod)
{
    bool shouldUpdateCslState;

    VerifyOrExit(mCslPeriod != aPeriod);

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    if (IsWakeupListenEnabled() && aPeriod != 0)
    {
        IgnoreError(SetWakeupListenEnabled(false));
        LogWarn("Disabling wake-up frame listening due to CSL period change");
    }
#endif

    // A CSL period value of 0 means that the CSL is disabled.
    shouldUpdateCslState = ((mCslPeriod == 0) != (aPeriod == 0));
    mCslPeriod           = aPeriod;

    if (shouldUpdateCslState)
    {
        UpdateCslState();
    }
    else
    {
        UpdateCslParameters();
    }

exit:
    return;
}

void Mac::UpdateCslState(void)
{
    // This method will enable/disable CSL when the CSL state (enabled/disabled) is changed. Otherwise, nothing to do.
    bool isCslEnabled = mIsCslCapable && (mCslPeriod > 0);

    VerifyOrExit(mIsCslEnabled != isCslEnabled);

    mIsCslEnabled = isCslEnabled;

    if (mIsCslEnabled)
    {
        UpdateCslParameters();
        // Request the Mac to enter sleep state.
        UpdateIdleMode();
    }
    else
    {
        // The platform API `otPlatRadioEnableCsl()` description says that disable CSL by setting the CSL period to 0.
        // However, this description does not say whether the parameter `aExtAddr` can be set to nullptr or how to set
        // the `aExtAddr` when the CSL is disabled. Here, an empty ExtAddress is set to meet the API requirement.
        ExtAddress extAddress;

        extAddress.Fill(0);
        mLinks.SetCslParams(0, 0, kShortAddrInvalid, extAddress);
    }

    LogInfo("CSL receiver is %s", mIsCslEnabled ? "enabled" : "disabled");

exit:
    return;
}

void Mac::UpdateCslParameters(void)
{
    // This method will set all CSL parameters when the CSL is enabled. Otherwise, nothing to do.
    uint8_t cslChannel;

    VerifyOrExit(mIsCslEnabled);

    cslChannel = GetCslChannel() ? GetCslChannel() : mPanChannel;
    mLinks.SetCslParams(GetCslPeriod(), cslChannel, Get<Mle::Mle>().GetParent().GetRloc16(),
                        Get<Mle::Mle>().GetParent().GetExtAddress());
    Get<DataPollSender>().RecalculatePollPeriod();
    Get<Mle::Mle>().ScheduleChildUpdateRequest();

exit:
    return;
}

uint32_t Mac::GetCslPeriodInMsec(void) const
{
    return DivideAndRoundToClosest<uint32_t>(CslPeriodToUsec(GetCslPeriod()), 1000u);
}

uint32_t Mac::CslPeriodToUsec(uint16_t aPeriodInTenSymbols)
{
    return static_cast<uint32_t>(aPeriodInTenSymbols) * Radio::kUsPerTenSymbols;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

void Mac::ProcessCsl(const RxFrame &aFrame, const Address &aSrcAddr)
{
    CslNeighbor *neighbor = nullptr;
    const CslIe *csl;

    VerifyOrExit(aFrame.IsVersion2015());
    VerifyOrExit(aFrame.IsSecuredWith(RxFrame::kAllowKeyIdMode1));

    csl = aFrame.Find<CslIe>();
    VerifyOrExit(csl != nullptr);

#if OPENTHREAD_FTD
    neighbor = Get<ChildTable>().FindChild(aSrcAddr, Child::kInStateAnyExceptInvalid);
#else
    OT_UNUSED_VARIABLE(aSrcAddr);
#endif

    VerifyOrExit(neighbor != nullptr);

    VerifyOrExit(csl->GetPeriod() >= kMinCslIePeriod);

    neighbor->SetCslPeriod(csl->GetPeriod());
    neighbor->SetCslPhase(csl->GetPhase());
    neighbor->SetCslSynchronized(true);
    neighbor->SetCslLastHeard(TimerMilli::GetNow());
    neighbor->SetLastRxTimestamp(aFrame.GetTimestamp());
    LogDebg("Timestamp=%lu Sequence=%u CslPeriod=%u CslPhase=%u TransmitPhase=%u",
            ToUlong(Radio::ConvertTime64To32(aFrame.GetTimestamp())), aFrame.GetSequence(), csl->GetPeriod(),
            csl->GetPhase(), neighbor->GetCslPhase());

#if OPENTHREAD_FTD
    Get<CslTxScheduler>().Update();
#endif

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mac::ProcessEnhAckProbing(const RxFrame &aFrame, const Neighbor &aNeighbor)
{
    const LinkMetricsProbingIe *probingIe = aFrame.Find<LinkMetricsProbingIe>();
    uint8_t                     dataLen;

    VerifyOrExit(probingIe != nullptr);

    dataLen = probingIe->GetMetricsDataLen();
    VerifyOrExit(dataLen <= LinkMetricsProbingIe::kMaxMetricsDataLen);

    Get<LinkMetrics::Initiator>().ProcessEnhAckIeData(probingIe->GetMetricsData(), dataLen, aNeighbor);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
void Mac::SetRadioFilterEnabled(bool aFilterEnabled)
{
    mLinks.GetSubMac().SetRadioFilterEnabled(aFilterEnabled);
    UpdateIdleMode();
}
#endif

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
Error Mac::SetWakeupChannel(uint8_t aChannel)
{
    Error error = kErrorNone;

    if (aChannel == 0)
    {
        mWakeupChannel = GetPanChannel();
        ExitNow();
    }

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = kErrorInvalidArgs);
    mWakeupChannel = aChannel;

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    UpdateWakeupListening();
#endif

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
void Mac::GetWakeupListenParameters(uint32_t &aInterval, uint32_t &aDuration) const
{
    aInterval = mWakeupListenInterval;
    aDuration = mWakeupListenDuration;
}

Error Mac::SetWakeupListenParameters(uint32_t aInterval, uint32_t aDuration)
{
    Error error = kErrorNone;

    VerifyOrExit(aDuration >= Radio::kMinWakeupListenDuration, error = kErrorInvalidArgs);
    VerifyOrExit(aInterval > aDuration, error = kErrorInvalidArgs);

    mWakeupListenInterval = aInterval;
    mWakeupListenDuration = aDuration;
    UpdateWakeupListening();

exit:
    return error;
}

Error Mac::SetWakeupListenEnabled(bool aEnable)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (aEnable && GetCslPeriod() > 0)
    {
        LogWarn("Cannot enable wake-up frame listening while CSL is enabled");
        ExitNow(error = kErrorInvalidState);
    }
#endif

    if (aEnable == mWakeupListenEnabled)
    {
        LogInfo("Listening for wake up frames was already %s", aEnable ? "started" : "stopped");
        ExitNow();
    }

    mWakeupListenEnabled = aEnable;
    UpdateWakeupListening();

    LogInfo("Listening for wake up frames %s: chan:%u, addr:%s", aEnable ? "started" : "stopped", mWakeupChannel,
            GetExtAddress().ToString().AsCString());

exit:
    return error;
}

void Mac::UpdateWakeupListening(void)
{
    uint8_t channel = mWakeupChannel ? mWakeupChannel : mPanChannel;

    mLinks.UpdateWakeupListening(mWakeupListenEnabled, mWakeupListenInterval, mWakeupListenDuration, channel);
}

Error Mac::HandleWakeupFrame(const RxFrame &aFrame)
{
    Error               error = kErrorNone;
    const ConnectionIe *connectionIe;
    Address             srcAddress;
    WakeupInfo          wakeupInfo;
    Radio::Time32       rvTimeUs;
    Radio::Time64       rvTimestampUs;
    Radio::Time64       radioNowUs;

    VerifyOrExit(mWakeupListenEnabled && aFrame.IsWakeupFrame());

    SuccessOrExit(error = aFrame.GetSrcAddr(srcAddress));
    VerifyOrExit(srcAddress.IsExtended(), error = kErrorDrop);

    wakeupInfo.mExtAddress    = srcAddress.GetExtended();
    connectionIe              = aFrame.Find<ConnectionIe>();
    wakeupInfo.mRetryInterval = connectionIe->GetRetryInterval();
    wakeupInfo.mRetryCount    = connectionIe->GetRetryCount();
    VerifyOrExit(wakeupInfo.mRetryInterval > 0 && wakeupInfo.mRetryCount > 0, error = kErrorInvalidArgs);

    radioNowUs = Get<Radio::Radio>().GetNow();
    rvTimeUs   = aFrame.Find<RendezvousTimeIe>()->GetRendezvousTime() * Radio::kUsPerTenSymbols;
    rvTimestampUs =
        aFrame.GetTimestamp() + Radio::kHeaderPhrDuration + aFrame.GetLength() * Radio::kOctetDuration + rvTimeUs;

    if (rvTimestampUs > radioNowUs + kCslRequestAhead)
    {
        wakeupInfo.mAttachDelayMs = Radio::ConvertTime64To32(rvTimestampUs - radioNowUs - kCslRequestAhead);
        wakeupInfo.mAttachDelayMs = wakeupInfo.mAttachDelayMs / Time::kOneMsecInUsec;
    }
    else
    {
        wakeupInfo.mAttachDelayMs = 0;
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    {
        uint32_t frameCounter;

        IgnoreError(aFrame.GetFrameCounter(frameCounter));
        LogInfo("Received wake-up frame, fc:%lu, rendezvous:%luus, retries:%u/%u", ToUlong(frameCounter),
                ToUlong(rvTimeUs), wakeupInfo.mRetryCount, wakeupInfo.mRetryInterval);
    }
#endif

    // Stop receiving more wake up frames
    IgnoreError(SetWakeupListenEnabled(false));

    Get<Mle::Mle>().HandleWakeupFrame(wakeupInfo);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

} // namespace Mac
} // namespace ot
