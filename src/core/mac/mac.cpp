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
    , mEnabled(true)
    , mShouldTxPollBeforeData(false)
    , mRxOnWhenIdle(false)
    , mPromiscuous(false)
    , mBeaconsEnabled(false)
    , mUsingTemporaryChannel(false)
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    , mShouldDelaySleep(false)
    , mDelayingSleep(false)
#endif
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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

    // MAC starts in the enabled state (`mEnabled` is set to `true`).
    // Enable `mLinks` directly instead of calling `SetEnabled()` to
    // avoid invoking callbacks on other modules (e.g., `CslTxScheduler`)
    // before they are fully initialized during `Instance` initialization
    // and constructor calls.

    mLinks.Enable();

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
    VerifyOrExit(mEnabled != aEnable);

    mEnabled = aEnable;

    if (aEnable)
    {
        mLinks.Enable();
    }
    else
    {
        mLinks.Disable();
    }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    Get<CslTxScheduler>().HandleMacEnableStatusChanged();
#endif

exit:
    return;
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
#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
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

void Mac::ReportActiveScanResult(const RxFrame::ParseInfo *aBeaconFrameInfo)
{
    VerifyOrExit(mActiveScanCallback.IsSet());

    if (aBeaconFrameInfo == nullptr)
    {
        mActiveScanCallback.Invoke(nullptr);
    }
    else
    {
        ScanResult result;

        SuccessOrExit(result.PopulateFromBeacon(*aBeaconFrameInfo));
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

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
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

        LogOperation(kRequest, aOperation);

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
#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
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
        LogOperation(kStarting, mOperation);
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
#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
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
    LogOperation(kFinishing, mOperation);
    mOperation = kOperationIdle;
}

TxFrame *Mac::PrepareBeaconRequest(TxFrames &aTxFrames)
{
    TxFrame           &frame = aTxFrames.GetBroadcastTxFrame();
    TxFrame::BuildInfo buildInfo;

    buildInfo.mAddrs.mSource.SetNone();
    buildInfo.mAddrs.mDestination.SetShort(kShortAddrBroadcast);
    buildInfo.mPanIds.SetDestination(kPanIdBroadcast);

    buildInfo.mType      = Frame::kTypeMacCmd;
    buildInfo.mCommandId = Frame::kMacCmdBeaconRequest;
    buildInfo.mVersion   = Frame::kVersion2003;

    frame.PrepareHeadersWithEmptyPayload(buildInfo);

    LogInfo("Sending Beacon Request");

    return &frame;
}

TxFrame *Mac::PrepareBeacon(TxFrames &aTxFrames)
{
    TxFrame                *frame;
    TxFrame::BuildInfo      buildInfo;
    TxFrame::PayloadBuilder builder;

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

    frame->PrepareHeaders(buildInfo, builder);

    builder.Append<BeaconHeader>()->Init();

#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
    builder.Append<BeaconPayload>()->Init(Get<MeshCoP::NetworkIdentity>(), IsJoinable());
#endif

    frame->FinishPayload(builder);

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
    TxFrame::ParseInfo frameInfo;

    IgnoreError(frameInfo.ParseFrom(aFrame, Frame::kParseFully));
    ProcessTransmitSecurity(frameInfo);
}

void Mac::ProcessTransmitSecurity(TxFrame::ParseInfo &aFrameInfo)
{
    KeyManager       &keyManager = Get<KeyManager>();
    const ExtAddress *extAddress = nullptr;

    VerifyOrExit(aFrameInfo.mParsedFully);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);

    switch (aFrameInfo.mKeyIdMode)
    {
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode0:
        aFrameInfo.GetTxFrame()->SetAesKey(keyManager.GetKek());
        extAddress = &GetExtAddress();

        if (!aFrameInfo.GetTxFrame()->IsHeaderUpdated())
        {
            aFrameInfo.WriteFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode1:

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrameInfo.GetTxFrame()->GetRadioType() == Radio::kTypeIeee802154)
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
        if (aFrameInfo.GetTxFrame()->GetRadioType() == Radio::kTypeTrel)
#endif
        {
            const KeyMaterial *macKey;

            // If the frame header is marked as updated, `MeshForwarder` which
            // prepared the frame should set the frame counter and key id to the
            // same values used in the earlier transmit attempt. For a new frame (header
            // not updated), we get a new frame counter and key id from the key
            // manager.

            if (!aFrameInfo.GetTxFrame()->IsHeaderUpdated())
            {
                mLinks.SetMacFrameCounter(aFrameInfo);
                aFrameInfo.WriteKeyIndex(DetermineKeyIndexFor(keyManager.GetCurrentKeySequence()));
            }

            macKey = DetermineMode1Key(aFrameInfo);
            VerifyOrExit(macKey != nullptr);
            aFrameInfo.GetTxFrame()->SetAesKey(*macKey);
            extAddress = &GetExtAddress();
        }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        break;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case Frame::kKeyIdMode2:
        aFrameInfo.GetTxFrame()->SetAesKey(mMode2KeyMaterial);

        mKeyIdMode2FrameCounter++;
        aFrameInfo.WriteFrameCounter(mKeyIdMode2FrameCounter);
        aFrameInfo.WriteKeySource(kMode2KeySource);
        aFrameInfo.WriteKeyIndex(0xff);
        extAddress = &AsCoreType(&kMode2ExtAddress);
        break;

    default:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(!aFrameInfo.Has<TimeIe>());
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(!aFrameInfo.GetTxFrame()->IsCslIePresent());
#endif

    aFrameInfo.ProcessTransmitAesCcm(*extAddress);

exit:
    return;
}

void Mac::BeginTransmit(void)
{
    TxFrame           *frame             = nullptr;
    TxFrames          &txFrames          = mLinks.InitTxFrames();
    bool               shouldWriteSeqNum = true;
    uint8_t            seqNum            = 0;
    TxFrame::ParseInfo frameInfo;

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
        frame->SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        frame->SetMaxFrameRetries(mMaxFrameRetriesDirect);
        seqNum = 0;
        break;

    case kOperationTransmitBeacon:
        frame = PrepareBeacon(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mRadioChannel);
        frame->SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        frame->SetMaxFrameRetries(mMaxFrameRetriesDirect);
        seqNum = mBeaconSequence++;
        break;

    case kOperationTransmitPoll:
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        frame = Get<DataPollSender>().PrepareDataRequest(txFrames);
        VerifyOrExit(frame != nullptr);
        seqNum = mDataSequence++;
        break;

    case kOperationTransmitDataDirect:
        // Set channel and retry counts on all TxFrames before asking
        // the next layer (`MeshForwarder`) to prepare the frame. This
        // allows next layer to possibility change these parameters.
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesDirect);
        frame = Get<MeshForwarder>().PrepareFrame(txFrames);
        VerifyOrExit(frame != nullptr);
        seqNum = mDataSequence++;
        break;

#if OPENTHREAD_FTD
    case kOperationTransmitDataIndirect:
        txFrames.SetChannel(mRadioChannel);
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsIndirect);
        txFrames.SetMaxFrameRetries(mMaxFrameRetriesIndirect);
        frame = Get<DataPollHandler>().PrepareFrame(txFrames);
        VerifyOrExit(frame != nullptr);
        // If the frame is marked as retransmission, then data sequence number is already set.
        shouldWriteSeqNum = !frame->IsARetransmission();
        seqNum            = shouldWriteSeqNum ? mDataSequence++ : 0;
        break;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        txFrames.SetMaxCsmaBackoffs(kMaxCsmaBackoffsCsl);
        txFrames.SetMaxFrameRetries(kMaxFrameRetriesCsl);
        frame = Get<CslTxScheduler>().PrepareFrame(txFrames);
        VerifyOrExit(frame != nullptr);
        // If the frame is marked as retransmission, then data sequence number is already set.
        shouldWriteSeqNum = !frame->IsARetransmission();
        seqNum            = shouldWriteSeqNum ? mDataSequence++ : 0;
        break;

#endif

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
    case kOperationTransmitWakeup:
        frame = Get<WakeupTxScheduler>().PrepareWakeupFrame(txFrames);
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mWakeupChannel);
        frame->SetRxChannelAfterTxDone(mRadioChannel);
        shouldWriteSeqNum = false;
        break;
#endif

    default:
        OT_ASSERT(false);
    }

    VerifyOrExit(frame != nullptr);
    IgnoreError(frameInfo.ParseFrom(*frame, Frame::kParseFully));

    if (shouldWriteSeqNum)
    {
        frameInfo.WriteSequenceNum(seqNum);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        TimeIe *timeIe = frameInfo.Find<TimeIe>();

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

    if (!frameInfo.GetTxFrame()->IsSecurityProcessed())
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
        ProcessTransmitSecurity(frameInfo);
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
        mShouldDelaySleep = frameInfo.mIsFramePending;
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
        HandleTxFramePrepFailed(txFrames);
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

void Mac::RecordFrameTransmitStatus(const TxFrame::ParseInfo &aFrameInfo,
                                    Error                     aError,
                                    uint8_t                   aRetryCount,
                                    bool                      aWillRetx)
{
    Neighbor *neighbor = nullptr;

    VerifyOrExit(aFrameInfo.mParsedFully);

    if (!aFrameInfo.mAddrs.mDestination.IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(aFrameInfo.mAddrs.mDestination);
    }

    // Record frame transmission success/failure state (for a neighbor).

    if ((neighbor != nullptr) && aFrameInfo.mIsAckRequest)
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
        LogFrameTxFailure(aFrameInfo, aError, aRetryCount, aWillRetx);
        DumpDebg("TX ERR", aFrameInfo.GetTxFrame()->GetPsdu(), 16);

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

    if (aFrameInfo.mIsAckRequest)
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

    if (aFrameInfo.mAddrs.mDestination.IsBroadcast())
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

Error Mac::ProcessTxDone(TxFrame::ParseInfo &aFrameInfo, RxFrame::ParseInfo &aAckFrameInfo, Error &aError)
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

    Error     error    = kErrorNone;
    Neighbor *neighbor = nullptr;

    VerifyOrExit(aFrameInfo.GetTxFrame() != nullptr);
    VerifyOrExit(!aFrameInfo.GetTxFrame()->IsEmpty());

#if OPENTHREAD_CONFIG_MULTI_RADIO
    VerifyOrExit(aFrameInfo.GetTxFrame()->GetRadioType() == Radio::kTypeIeee802154);

    // Set the radio type on `AckFrame`, so we can determine the
    // proper (15.4 based) key in `ProcessEnhAckSecurity()`.

    if (aAckFrameInfo.GetRxFrame() != nullptr)
    {
        aAckFrameInfo.GetRxFrame()->SetRadioType(Radio::kTypeIeee802154);
    }
#endif

    // Determine whether to re-transmit a broadcast frame.

    if (aFrameInfo.mAddrs.mDestination.IsBroadcast())
    {
        mBroadcastTransmitCount++;

        if (mBroadcastTransmitCount < kTxNumBcast)
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            {
                Radio::Types radioTypes;

                radioTypes.Add(Radio::kTypeIeee802154);
                mLinks.Send(*aFrameInfo.GetTxFrame(), radioTypes);
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

    VerifyOrExit(aFrameInfo.mIsAckRequest);
    VerifyOrExit(aAckFrameInfo.GetRxFrame() != nullptr);

    SuccessOrExit(aError);

    if (!aFrameInfo.mAddrs.mDestination.IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(aFrameInfo.mAddrs.mDestination);
    }

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if ((neighbor != nullptr) &&
        mFilter.ApplyToRxFrame(*aAckFrameInfo.GetRxFrame(), neighbor->GetExtAddress(), neighbor) != kErrorNone)
    {
        aError = kErrorNoAck;
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    if (ProcessEnhAckSecurity(aFrameInfo, aAckFrameInfo) != kErrorNone)
    {
        aError = kErrorNoAck;
        ExitNow();
    }
#endif

    VerifyOrExit(neighbor != nullptr);

    UpdateNeighborLinkInfo(*neighbor, aAckFrameInfo);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    ProcessEnhAckProbing(aAckFrameInfo, *neighbor);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ProcessCsl(aAckFrameInfo, aFrameInfo.mAddrs.mDestination);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (!mRxOnWhenIdle && aFrameInfo.mParsedFully && aFrameInfo.Has<CslIe>())
    {
        Get<DataPollSender>().ResetKeepAliveTimer();
    }
#endif

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO

Error Mac::ProcessMultiRadioTxDone(TxFrame::ParseInfo &aFrameInfo, Error &aError)
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

    VerifyOrExit(aFrameInfo.GetTxFrame() != nullptr);
    VerifyOrExit(!aFrameInfo.GetTxFrame()->IsEmpty());

    radio          = aFrameInfo.GetTxFrame()->GetRadioType();
    requiredRadios = mLinks.GetTxFramesRequiredRadioTypes();

    Get<RadioSelector>().UpdateOnSendDone(aFrameInfo, aError);

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

void Mac::HandleTxFramePrepFailed(TxFrames &aTxFrames)
{
    // If the frame could not be prepared, TX done is called with
    // an aborted error. We set the frame length to zero to mark it as empty.
    // The empty frame helps differentiate between an aborted tx due
    // to OpenThread itself not being able to prepare the frame, versus
    // the radio platform aborting the tx operation.

    TxFrame           &frame = aTxFrames.GetBroadcastTxFrame();
    TxFrame::ParseInfo frameInfo;

    frame.SetLength(0);
    frameInfo.mFrame = &frame;

    HandleTransmitDone(frameInfo, nullptr, kErrorAbort);
}

void Mac::HandleTransmitDone(TxFrame::ParseInfo &aFrameInfo, RxFrame *aAckFrame, Error aError)
{
    RxFrame::ParseInfo ackFrameInfo;

    if (aAckFrame != nullptr)
    {
        IgnoreError(ackFrameInfo.ParseFrom(*aAckFrame, Frame::kParseFully));
    }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    SuccessOrExit(ProcessTxDone(aFrameInfo, ackFrameInfo, aError));
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    SuccessOrExit(ProcessMultiRadioTxDone(aFrameInfo, aError));
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
        OT_ASSERT(aFrameInfo.GetTxFrame()->IsEmpty() || aFrameInfo.mIsAckRequest);

        if ((aError == kErrorNone) && (aAckFrame != nullptr))
        {
            if (IsEnabled() && ackFrameInfo.mIsFramePending)
            {
                StartOperation(kOperationWaitingForData);
            }

            LogInfo("Sent data poll, fp:%s", ToYesNo(ackFrameInfo.mIsFramePending));
        }

        mCounters.mTxDataPoll++;
        FinishOperation();
        Get<DataPollSender>().HandlePollTxDone(aFrameInfo, aError);
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

        DumpDebg("TX", aFrameInfo.GetTxFrame()->GetPsdu(), aFrameInfo.GetTxFrame()->GetLength());
        FinishOperation();
        Get<MeshForwarder>().HandleFrameTxDone(aFrameInfo, aError);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        Get<DataPollSender>().ProcessTxDone(aFrameInfo, ackFrameInfo, aError);
#endif
        PerformNextOperation();
        break;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        mCounters.mTxData++;

        DumpDebg("TX", aFrameInfo.GetTxFrame()->GetPsdu(), aFrameInfo.GetTxFrame()->GetLength());
        FinishOperation();
        Get<CslTxScheduler>().HandleFrameTxDone(aFrameInfo, aError);
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

        DumpDebg("TX", aFrameInfo.GetTxFrame()->GetPsdu(), aFrameInfo.GetTxFrame()->GetLength());
        FinishOperation();
        Get<DataPollHandler>().HandleFrameTxDone(aFrameInfo, aError);
        PerformNextOperation();
        break;
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
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

const KeyMaterial *Mac::DetermineMode1Key(const Frame::ParseInfo &aFrameInfo) const
{
    uint32_t keySequence;

    return DetermineMode1KeyAndSequence(aFrameInfo, keySequence);
}

const KeyMaterial *Mac::DetermineMode1KeyAndSequence(const Frame::ParseInfo &aFrameInfo, uint32_t &aKeySequence) const
{
    // Determines the MAC key and key sequence for given `aFrameInfo`.
    // The caller MUST already ensure that the frame's Key ID Mode
    // is Mode 1.

    const KeyMaterial *key = nullptr;
    KeyTrio::Type      keyType;

    aKeySequence = Get<KeyManager>().GetCurrentKeySequence();

    if (aFrameInfo.mKeyIndex == DetermineKeyIndexFor(aKeySequence))
    {
        keyType = KeyTrio::kCur;
    }
    else if (aFrameInfo.mKeyIndex == DetermineKeyIndexFor(aKeySequence + 1))
    {
        aKeySequence++;
        keyType = KeyTrio::kNext;
    }
    else if (aFrameInfo.mKeyIndex == DetermineKeyIndexFor(aKeySequence - 1))
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
    if (aFrameInfo.mFrame->GetRadioType() == Radio::kTypeIeee802154)
#endif
    {
        ExitNow(key = &Get<SubMac>().GetMacKey(keyType));
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrameInfo.mFrame->GetRadioType() == Radio::kTypeTrel)
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

Error Mac::ProcessReceiveSecurity(RxFrame::ParseInfo &aFrameInfo, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager        &keyManager  = Get<KeyManager>();
    Error              error       = kErrorSecurity;
    uint32_t           keySequence = 0;
    const KeyMaterial *macKey;
    const ExtAddress  *extAddress;

    VerifyOrExit(aFrameInfo.mIsSecurityEnabled, error = kErrorNone);

    VerifyOrExit(aFrameInfo.mSecurityLevel == Frame::kSecurityEncMic32);

    LogDebg("Rx security - frame counter %lu", ToUlong(aFrameInfo.mFrameCounter));

    switch (aFrameInfo.mKeyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit(keyManager.IsKekSet());
        macKey = &keyManager.GetKek();
        VerifyOrExit(aSrcAddr.IsExtended());
        extAddress = &aSrcAddr.GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != nullptr);

        macKey = DetermineMode1KeyAndSequence(aFrameInfo, keySequence);
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
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get(aFrameInfo.GetRxFrame()->GetRadioType());
#else
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get();
#endif

                // If frame counter is one off, then frame is a duplicate.
                VerifyOrExit((aFrameInfo.mFrameCounter + 1) != neighborFrameCounter, error = kErrorDuplicated);

                VerifyOrExit(aFrameInfo.mFrameCounter >= neighborFrameCounter);
            }
        }

        VerifyOrExit(aSrcAddr.IsExtended());
        extAddress = &aSrcAddr.GetExtended();

        break;

    case Frame::kKeyIdMode2:
        macKey     = &mMode2KeyMaterial;
        extAddress = &AsCoreType(&kMode2ExtAddress);
        break;

    default:
        ExitNow();
    }

    SuccessOrExit(aFrameInfo.ProcessReceiveAesCcm(*extAddress, *macKey));

    if ((aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1) && (aNeighbor != nullptr) && aNeighbor->IsStateValid())
    {
        if (aNeighbor->GetKeySequence() != keySequence)
        {
            aNeighbor->SetKeySequence(keySequence);
            aNeighbor->SetMleFrameCounter(0);
            aNeighbor->GetLinkFrameCounters().Reset();
        }

#if OPENTHREAD_CONFIG_MULTI_RADIO
        aNeighbor->GetLinkFrameCounters().Set(aFrameInfo.mFrame->GetRadioType(), aFrameInfo.mFrameCounter + 1);
#else
        aNeighbor->GetLinkFrameCounters().Set(aFrameInfo.mFrameCounter + 1);
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrameInfo.mFrame->GetRadioType() == Radio::kTypeIeee802154)
#endif
        {
            if ((aFrameInfo.mFrameCounter + 1) > aNeighbor->GetLinkAckFrameCounter())
            {
                aNeighbor->SetLinkAckFrameCounter(aFrameInfo.mFrameCounter + 1);
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
Error Mac::ProcessEnhAckSecurity(TxFrame::ParseInfo &aTxFrameInfo, RxFrame::ParseInfo &aAckFrameInfo)
{
    Error              error    = kErrorSecurity;
    Neighbor          *neighbor = nullptr;
    const KeyMaterial *macKey;
    Address            srcAddr;

    VerifyOrExit(aTxFrameInfo.mParsedFully, error = kErrorParse);
    VerifyOrExit(aAckFrameInfo.mParsedFully, error = kErrorParse);

    if (!aAckFrameInfo.mIsSecurityEnabled)
    {
        // Reject an unsecured ACK carrying IEs in response to a secured 2015 frame.

        if (aTxFrameInfo.mIsSecurityEnabled && (aTxFrameInfo.mVersion == Frame::kVersion2015) &&
            aAckFrameInfo.mIsIePresent)
        {
            ExitNow();
        }

        ExitNow(error = kErrorNone);
    }

    VerifyOrExit(aAckFrameInfo.mVersion == Frame::kVersion2015);
    VerifyOrExit(aAckFrameInfo.mSecurityLevel == Frame::kSecurityEncMic32);
    VerifyOrExit(aAckFrameInfo.mKeyIdMode == Frame::kKeyIdMode1);

    VerifyOrExit(aTxFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aTxFrameInfo.mKeyIndex == aAckFrameInfo.mKeyIndex);

    LogDebg("Rx security - Ack frame counter %lu", ToUlong(aAckFrameInfo.mFrameCounter));

    srcAddr = aAckFrameInfo.mAddrs.mSource;

    if (!srcAddr.IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(srcAddr);
    }
    else if (!aTxFrameInfo.mAddrs.mDestination.IsNone())
    {
        // Get neighbor from destination address of transmitted frame
        neighbor = Get<NeighborTable>().FindNeighbor(aTxFrameInfo.mAddrs.mDestination);
    }

    if (!srcAddr.IsExtended() && neighbor != nullptr)
    {
        srcAddr.SetExtended(neighbor->GetExtAddress());
    }

    VerifyOrExit(srcAddr.IsExtended() && neighbor != nullptr);

    macKey = DetermineMode1Key(aAckFrameInfo);
    VerifyOrExit(macKey != nullptr);

    if (neighbor->IsStateValid())
    {
        VerifyOrExit(aAckFrameInfo.mFrameCounter >= neighbor->GetLinkAckFrameCounter());
    }

    SuccessOrExit(error = aAckFrameInfo.ProcessReceiveAesCcm(srcAddr.GetExtended(), *macKey));

    if (neighbor->IsStateValid())
    {
        neighbor->SetLinkAckFrameCounter(aAckFrameInfo.mFrameCounter + 1);
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
    Error              error    = aError;
    Neighbor          *neighbor = nullptr;
    RxFrame::ParseInfo frameInfo;
    Address            srcAddr;

    mCounters.mRxTotal++;

    frameInfo.mFrame = aFrame;

    SuccessOrExit(error);
    VerifyOrExit(aFrame != nullptr, error = kErrorNoFrameReceived);
    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = frameInfo.ParseFrom(*aFrame, Frame::kParseFully));

    // Destination Address Filtering
    switch (frameInfo.mAddrs.mDestination.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        SuccessOrExit(error = FilterDestShortAddress(frameInfo.mAddrs.mDestination.GetShort()));
        break;

    case Address::kTypeExtended:
        VerifyOrExit(frameInfo.mAddrs.mDestination.GetExtended() == GetExtAddress(),
                     error = kErrorDestinationAddressFiltered);
        break;
    }

    // Verify destination PAN ID if present
    if (frameInfo.mPanIds.IsDestinationPresent())
    {
        PanId panId = frameInfo.mPanIds.GetDestination();

        VerifyOrExit(panId == kPanIdBroadcast || panId == mPanId, error = kErrorDestinationAddressFiltered);
    }

    // Source Address Filtering
    //
    // If the`srcAddr` is associated with a known neighbor, a short
    // `srcAddr` is replaced with the `ExtAddress` of the neighbor. The
    // Extended Address is then used for nonce calculation during RX
    // security processing.

    srcAddr = frameInfo.mAddrs.mSource;

    if (!srcAddr.IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(srcAddr);

#if OPENTHREAD_FTD
        // Allow multicasts from neighbor routers if FTD
        if ((neighbor == nullptr) && frameInfo.mAddrs.mDestination.IsBroadcast() &&
            Get<Mle::Mle>().IsFullThreadDevice())
        {
            neighbor = Get<NeighborTable>().FindRxOnlyNeighborRouter(srcAddr);
        }
#endif

        if (srcAddr.IsShort())
        {
            LogDebg("Received frame from short address 0x%04x", srcAddr.GetShort());
            VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);
            srcAddr.SetExtended(neighbor->GetExtAddress());
        }

        VerifyOrExit(srcAddr.GetExtended() != GetExtAddress(), error = kErrorInvalidSourceAddress);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        SuccessOrExit(error = mFilter.ApplyToRxFrame(*aFrame, srcAddr.GetExtended(), neighbor));
#endif
    }

    if (frameInfo.mAddrs.mDestination.IsBroadcast())
    {
        mCounters.mRxBroadcast++;
    }
    else
    {
        mCounters.mRxUnicast++;
    }

    error = ProcessReceiveSecurity(frameInfo, srcAddr, neighbor);

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
    ProcessCsl(frameInfo, srcAddr);
#endif

    Get<DataPollSender>().ProcessRxFrame(frameInfo);

    if (neighbor != nullptr)
    {
        UpdateNeighborLinkInfo(*neighbor, frameInfo);

        if (frameInfo.mIsSecurityEnabled)
        {
            if (frameInfo.mKeyIdMode == Frame::kKeyIdMode1)
            {
                switch (neighbor->GetState())
                {
                case Neighbor::kStateValid:
                    break;

                case Neighbor::kStateRestored:
                case Neighbor::kStateChildUpdateRequest:

                    // Only accept a "MAC Data Request" frame from a child being restored.
                    VerifyOrExit((frameInfo.mType == Frame::kTypeMacCmd) &&
                                     (frameInfo.mCommandId == Frame::kMacCmdDataRequest),
                                 error = kErrorDrop);
                    break;

                default:
                    ExitNow(error = kErrorUnknownNeighbor);
                }

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2 && OPENTHREAD_FTD
                // From Thread 1.2, MAC Data Frame can also act as keep-alive message if child supports
                if (frameInfo.mType == Frame::kTypeData && !neighbor->IsRxOnWhenIdle() &&
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

        if (frameInfo.mType == Frame::kTypeBeacon)
        {
            mCounters.mRxBeacon++;
            ReportActiveScanResult(&frameInfo);
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

        if (!frameInfo.mAddrs.mDestination.IsNone())
        {
            mTimer.Stop();

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
            if (!mRxOnWhenIdle && !mPromiscuous && frameInfo.mIsFramePending)
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

    switch (frameInfo.mType)
    {
    case Frame::kTypeMacCmd:
        HandleMacCommand(frameInfo);
        break;

    case Frame::kTypeBeacon:
        mCounters.mRxBeacon++;
        break;

    case Frame::kTypeData:
        mCounters.mRxData++;
        DumpDebg("RX", aFrame->GetPsdu(), aFrame->GetLength());
        Get<MeshForwarder>().HandleReceivedFrame(frameInfo);
        UpdateIdleMode();
        break;

    default:
        mCounters.mRxOther++;
        break;
    }

exit:

    if (error != kErrorNone)
    {
        LogFrameRxFailure(frameInfo, error);

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
    if ((aFrame != nullptr) && aFrame->GetRadioType() == Radio::kTypeTrel)
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
            // If the frame was secured with the network key (key ID
            // mode 1) or the KEK (key ID mode 0) and was successfully
            // processed, we allow the `Peer` entry socket information
            // to be updated directly. Key ID mode 2 uses the
            // well-known key and therefore does not identify a
            // specific sender, so it is not accepted for a direct
            // update (matching the policy in
            // `ThreadLinkInfo::SetFrom()`).

            Trel::Link::PeerSockAddrUpdateMode peerSockAddrUpdateMode = Trel::Link::kDisallowPeerSockAddrUpdate;

            if (frameInfo.mIsSecurityEnabled)
            {
                switch (frameInfo.mKeyIdMode)
                {
                case Frame::kKeyIdMode0:
                case Frame::kKeyIdMode1:
                    peerSockAddrUpdateMode = Trel::Link::kAllowPeerSockAddrUpdate;
                    break;
                default:
                    break;
                }
            }

            Get<Trel::Link>().CheckPeerAddrOnRxSuccess(peerSockAddrUpdateMode);
        }
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
}

void Mac::UpdateNeighborLinkInfo(Neighbor &aNeighbor, const RxFrame::ParseInfo &aRxFrameInfo)
{
    LinkQuality oldLinkQuality = aNeighbor.GetLinkInfo().GetLinkQualityIn();

    VerifyOrExit(aRxFrameInfo.mParsedFully);

    aNeighbor.GetLinkInfo().AddRss(aRxFrameInfo.GetRxFrame()->GetRssi());

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    aNeighbor.AggregateLinkMetrics(/* aSeriesId */ 0, aRxFrameInfo.mType, aRxFrameInfo.GetRxFrame()->GetLqi(),
                                   aRxFrameInfo.GetRxFrame()->GetRssi());
#endif

    // Signal when `aNeighbor` is the current parent and its link
    // quality gets changed.

    VerifyOrExit(Get<Mle::Mle>().IsChild() && (&aNeighbor == &Get<Mle::Mle>().GetParent()));
    VerifyOrExit(aNeighbor.GetLinkInfo().GetLinkQualityIn() != oldLinkQuality);
    Get<Notifier>().Signal(kEventParentLinkQualityChanged);

exit:
    return;
}

void Mac::HandleMacCommand(RxFrame::ParseInfo &aFrameInfo)
{
    switch (aFrameInfo.mCommandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        LogInfo("Received Beacon Request");

        if (ShouldSendBeacon())
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            mTxBeaconRadioLinks.Add(aFrameInfo.GetRxFrame()->GetRadioType());
#endif
            StartOperation(kOperationTransmitBeacon);
        }

        break;

    case Frame::kMacCmdDataRequest:
        mCounters.mRxDataPoll++;
#if OPENTHREAD_FTD
        Get<DataPollHandler>().HandleDataPoll(aFrameInfo);
#endif
        break;

    default:
        mCounters.mRxOther++;
        break;
    }
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

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)

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

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
#define WakeupOperationMapList(_) _(kOperationTransmitWakeup, "TransmitWakeup")
#else
#define WakeupOperationMapList(_)
#endif

    DefineEnumStringArray(OperationMapList);

    return kStrings[aOperation];
}

const char *Mac::OperationActionToString(OperationAction aAction)
{
#define OperationActionMapList(_)   \
    _(kRequest, "Request to start") \
    _(kStarting, "Starting")        \
    _(kFinishing, "Finishing")

    DefineEnumStringArray(OperationActionMapList);

    return kStrings[aAction];
}

void Mac::LogOperation(OperationAction aAction, Operation aOperation) const
{
    LogDebg("%s operation \"%s\"", OperationActionToString(aAction), OperationToString(aOperation));
}

#else // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)

void Mac::LogOperation(OperationAction, Operation) const {}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Mac::LogFrameRxFailure(const RxFrame::ParseInfo &aFrameInfo, Error aError) const
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

    LogAt(logLevel, "Frame rx failed, error:%s, %s", ErrorToString(aError), aFrameInfo.ToInfoString().AsCString());
}

void Mac::LogFrameTxFailure(const TxFrame::ParseInfo &aFrameInfo,
                            Error                     aError,
                            uint8_t                   aRetryCount,
                            bool                      aWillRetx) const
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrameInfo.GetTxFrame()->GetRadioType() == Radio::kTypeIeee802154)
#endif
    {
        uint8_t maxAttempts = aFrameInfo.GetTxFrame()->GetMaxFrameRetries() + 1;
        uint8_t curAttempt  = aWillRetx ? (aRetryCount + 1) : maxAttempts;

        LogInfo("Frame tx attempt %u/%u failed, error:%s, %s", curAttempt, maxAttempts, ErrorToString(aError),
                aFrameInfo.ToInfoString().AsCString());
    }
#else
    OT_UNUSED_VARIABLE(aRetryCount);
    OT_UNUSED_VARIABLE(aWillRetx);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrameInfo.GetTxFrame()->GetRadioType() == Radio::kTypeTrel)
#endif
    {
        if (Get<Trel::Interface>().IsEnabled())
        {
            LogInfo("Frame tx failed, error:%s, %s", ErrorToString(aError), aFrameInfo.ToInfoString().AsCString());
        }
    }
#endif
}

void Mac::LogBeacon(const char *aActionText) const { LogInfo("%s Beacon", aActionText); }

#else // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Mac::LogFrameRxFailure(const RxFrame::ParseInfo &, Error) const {}

void Mac::LogBeacon(const char *) const {}

void Mac::LogFrameTxFailure(const TxFrame::ParseInfo &, Error, uint8_t, bool) const {}

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

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

void Mac::ProcessCsl(const RxFrame::ParseInfo &aFrameInfo, const Address &aSrcAddr)
{
    CslNeighbor *neighbor = nullptr;
    const CslIe *csl;

    VerifyOrExit(!aSrcAddr.IsNone());

    VerifyOrExit(aFrameInfo.mVersion == Frame::kVersion2015);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1);

    csl = aFrameInfo.Find<CslIe>();
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
    neighbor->SetLastRxTimestamp(aFrameInfo.GetRxFrame()->GetTimestamp());
    LogDebg("Timestamp=%lu Sequence=%u CslPeriod=%u CslPhase=%u TransmitPhase=%u",
            ToUlong(Radio::ConvertTime64To32(aFrameInfo.GetRxFrame()->GetTimestamp())), aFrameInfo.mSequenceNum,
            csl->GetPeriod(), csl->GetPhase(), neighbor->GetCslPhase());

#if OPENTHREAD_FTD
    Get<CslTxScheduler>().Update();
#endif

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mac::ProcessEnhAckProbing(const RxFrame::ParseInfo &aFrameInfo, const Neighbor &aNeighbor)
{
    const LinkMetricsProbingIe *probingIe;
    uint8_t                     dataLen;

    VerifyOrExit(aFrameInfo.mParsedFully);

    probingIe = aFrameInfo.Find<LinkMetricsProbingIe>();
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

#if OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    UpdateWakeupListening();
#endif

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

} // namespace Mac
} // namespace ot
