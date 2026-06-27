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
#include "utils/static_counter.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("Mac");

const otExtAddress Mac::sMode2ExtAddress = {
    {0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12},
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
    , mSupportedChannelMask(Get<Radio>().GetSupportedChannelMask())
    , mScanChannel(Radio::kChannelMin)
    , mScanDuration(0)
    , mMaxFrameRetriesDirect(kDefaultMaxFrameRetriesDirect)
#if OPENTHREAD_FTD
    , mMaxFrameRetriesIndirect(kDefaultMaxFrameRetriesIndirect)
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    , mCslTxFireTime(TimeMilli::kMaxDuration)
#endif
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

    static const otMacKey sMode2Key = {
        {0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f, 0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66}};

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

    mMode2KeyMaterial.SetFrom(AsCoreType(&sMode2Key));
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

Error Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ScanResult::Handler aHandler, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = kErrorBusy);

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
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = kErrorBusy);

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

void Mac::ReportActiveScanResult(const RxFrame::Info *aBeaconFrameInfo)
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
void Mac::RequestCslFrameTransmission(uint32_t aDelay)
{
    VerifyOrExit(mEnabled);

    mCslTxFireTime = TimerMilli::GetNow() + aDelay;

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

    // We ensure data frame and data poll tx requests are handled in the
    // order they are requested. So if we have a pending direct data frame
    // tx request, it should be sent before the poll frame.

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
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    else if (IsPending(kOperationTransmitDataCsl))
    {
        mTimer.FireAt(mCslTxFireTime);
    }
#endif

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

    // `WaitingForData` should be checked before any other pending
    // operations since radio should remain in receive mode after
    // a data poll ack indicating a pending frame from parent.
    if (IsPending(kOperationWaitingForData))
    {
        mOperation = kOperationWaitingForData;
    }
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    else if (IsPending(kOperationTransmitWakeup))
    {
        mOperation = kOperationTransmitWakeup;
    }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    else if (IsPending(kOperationTransmitDataCsl) && TimerMilli::GetNow() >= mCslTxFireTime)
    {
        mOperation = kOperationTransmitDataCsl;
    }
#endif
    else if (IsPending(kOperationActiveScan))
    {
        mOperation = kOperationActiveScan;
    }
    else if (IsPending(kOperationEnergyScan))
    {
        mOperation = kOperationEnergyScan;
    }
    else if (IsPending(kOperationTransmitBeacon))
    {
        mOperation = kOperationTransmitBeacon;
    }
#if OPENTHREAD_FTD
    else if (IsPending(kOperationTransmitDataIndirect))
    {
        mOperation = kOperationTransmitDataIndirect;
    }
#endif // OPENTHREAD_FTD
    else if (IsPending(kOperationTransmitPoll) && (!IsPending(kOperationTransmitDataDirect) || mShouldTxPollBeforeData))
    {
        mOperation = kOperationTransmitPoll;
    }
    else if (IsPending(kOperationTransmitDataDirect))
    {
        mOperation = kOperationTransmitDataDirect;

        if (IsPending(kOperationTransmitPoll))
        {
            // Ensure that a pending "transmit poll" operation request
            // is prioritized over any future "transmit data" requests.
            mShouldTxPollBeforeData = true;
        }
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
    Beacon            *beacon = nullptr;
#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
    uint8_t        beaconLength;
    BeaconPayload *beaconPayload = nullptr;
#endif

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

    beacon = reinterpret_cast<Beacon *>(frame->GetPayload());
    beacon->Init();

#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
    beaconLength = sizeof(*beacon);

    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    beaconPayload->Init();

    if (IsJoinable())
    {
        beaconPayload->SetJoiningPermitted();
    }
    else
    {
        beaconPayload->ClearJoiningPermitted();
    }

    beaconPayload->SetNetworkName(Get<MeshCoP::NetworkIdentity>().GetNetworkName().GetAsData());
    beaconPayload->SetExtendedPanId(Get<MeshCoP::NetworkIdentity>().GetExtPanId());

    beaconLength += sizeof(*beaconPayload);

    frame->SetPayloadLength(beaconLength);
#endif

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
    const ExtAddress *extAddress = nullptr;
    TxFrame::Info frameInfo;

    SuccessOrExit(frameInfo.ParseFrom(aFrame));

    VerifyOrExit(frameInfo.mSecurityEnabled);

    switch (frameInfo.mKeyIdMode)
    {
    case Frame::kKeyIdMode0:
        aFrame.SetAesKey(keyManager.GetKek());
        extAddress = &GetExtAddress();

        if (!aFrame.IsHeaderUpdated())
        {
            frameInfo.UpdateFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    case Frame::kKeyIdMode1:

        // For 15.4 radio link, the AES CCM* and frame security counter (under MAC
        // key ID mode 1) are managed by `SubMac` or `Radio` modules.
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if !OPENTHREAD_CONFIG_MULTI_RADIO
        ExitNow();
#else
        VerifyOrExit(frameInfo.mRadioType != kRadioTypeIeee802154);
#endif
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        aFrame.SetAesKey(*mLinks.GetCurrentMacKey(aFrame));
        extAddress = &GetExtAddress();

        // If the frame header is marked as updated, `MeshForwarder` which
        // prepared the frame should set the frame counter and key id to the
        // same values used in the earlier transmit attempt. For a new frame (header
        // not updated), we get a new frame counter and key id from the key
        // manager.

        if (!aFrame.IsHeaderUpdated())
        {
            mLinks.SetMacFrameCounter(frameInfo);
            frameInfo.UpdateKeyId((keyManager.GetCurrentKeySequence() & 0x7f) + 1);
        }
#endif
        break;

    case Frame::kKeyIdMode2:
    {
        uint8_t keySource[] = {0xff, 0xff, 0xff, 0xff};

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
        if (aFrame.IsWakeupFrame())
        {
            // Just set the key source here, further security processing will happen in SubMac
            BigEndian::WriteUint32(keyManager.GetCurrentKeySequence(), keySource);
            frameInfo.UpdateKeySource(keySource);
            ExitNow();
        }
#endif
        aFrame.SetAesKey(mMode2KeyMaterial);

        mKeyIdMode2FrameCounter++;
        frameInfo.UpdateFrameCounter(mKeyIdMode2FrameCounter);
        frameInfo.UpdateKeySource(keySource);
        frameInfo.UpdateKeyId(0xff);
        extAddress = &AsCoreType(&sMode2ExtAddress);
        break;
    }

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

    frameInfo.ProcessTransmitAesCcm(*extAddress);

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

        for (RadioType radio : RadioTypes::kAllRadioTypes)
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
        for (RadioType radio : RadioTypes::kAllRadioTypes)
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
        TxFrame::Info frameInfo;

        // If the frame could not be prepared and the tx is being
        // aborted, we set the frame length to zero to mark it as empty.
        // The empty frame helps differentiate between an aborted tx due
        // to OpenThread itself not being able to prepare the frame, versus
        // the radio platform aborting the tx operation.

        frame = &txFrames.GetBroadcastTxFrame();
        frame->SetLength(0);
        IgnoreError(frameInfo.ParseFrom(*frame));
        HandleTransmitDone(frameInfo, nullptr, kErrorAbort);
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

void Mac::RecordFrameTransmitStatus(const TxFrame::Info &aFrameInfo, Error aError, uint8_t aRetryCount, bool aWillRetx)
{
    Neighbor *neighbor;

    VerifyOrExit(aFrameInfo.mFullyValidated);

    VerifyOrExit(aFrameInfo.mTotalLength > 0);

    neighbor = Get<NeighborTable>().FindNeighbor(aFrameInfo.GetDstAddr());

    // Record frame transmission success/failure state (for a neighbor).

    if ((neighbor != nullptr) && aFrameInfo.mAckRequest)
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
        DumpDebg("TX ERR", aFrameInfo.mHeader.GetBytes(), Min<uint16_t>(aFrameInfo.mHeader.GetLength(), 16));

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

    if (aFrameInfo.mAckRequest)
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

    if (aFrameInfo.GetDstAddr().IsBroadcast())
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

void Mac::HandleTransmitDone(TxFrame::Info &aFrameInfo, RxFrame *aAckFrame, Error aError)
{
    RxFrame::Info ackFrameInfo;

    if (aAckFrame != nullptr)
    {
        IgnoreError(ackFrameInfo.ParseFrom(*aAckFrame));
    }

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (!aFrameInfo.mIsEmptyFrame && aFrameInfo.mFullyValidated
#if OPENTHREAD_CONFIG_MULTI_RADIO
        && (aFrameInfo.mRadioType == kRadioTypeIeee802154)
#endif
    )
    {
        // Determine whether to re-transmit a broadcast frame.
        if (aFrameInfo.GetDstAddr().IsBroadcast())
        {
            mBroadcastTransmitCount++;

            if (mBroadcastTransmitCount < kTxNumBcast)
            {
#if OPENTHREAD_CONFIG_MULTI_RADIO
                {
                    RadioTypes radioTypes;
                    radioTypes.Add(kRadioTypeIeee802154);
                    mLinks.Send(aFrameInfo.GetTxFrame(), radioTypes);
                }
#else
                mLinks.Send();
#endif
                ExitNow();
            }

            mBroadcastTransmitCount = 0;
        }

        if (aFrameInfo.mAckRequest && (aAckFrame != nullptr))
        {
            Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(aFrameInfo.GetDstAddr());

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
            if ((aError == kErrorNone) && (neighbor != nullptr) &&
                (mFilter.ApplyToRxFrame(*aAckFrame, neighbor->GetExtAddress(), neighbor) != kErrorNone))
            {
                aError = kErrorNoAck;
            }
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
            // Verify Enh-ACK integrity by checking its MIC
            if ((aError == kErrorNone) && (ProcessEnhAckSecurity(aFrameInfo, ackFrameInfo) != kErrorNone))
            {
                aError = kErrorNoAck;
            }
#endif

            if ((aError == kErrorNone) && (neighbor != nullptr))
            {
                UpdateNeighborLinkInfo(*neighbor, ackFrameInfo);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
                ProcessEnhAckProbing(ackFrameInfo, *neighbor);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
                ProcessCsl(ackFrameInfo, aFrameInfo.GetDstAddr());
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
                if (!mRxOnWhenIdle && aFrameInfo.GetTxFrame().Has<CslIe>())
                {
                    Get<DataPollSender>().ResetKeepAliveTimer();
                }
#endif
            }
        }
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (!aFrameInfo.mIsEmptyFrame && aFrameInfo.mFullyValidated)
    {
        RadioType  radio          = aFrameInfo.mRadioType;
        RadioTypes requiredRadios = mLinks.GetTxFramesRequiredRadioTypes();

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
                LogDebgOnError(aError, "tx frame on required radio link %s", RadioTypeToString(radio));
                mTxError = aError;
            }
        }

        // Keep track of radio links on which the frame is sent
        // and wait for all radio links to finish.
        mTxPendingRadioLinks.Remove(radio);

        VerifyOrExit(mTxPendingRadioLinks.IsEmpty());

        aError = mTxError;
    }
#endif // OPENTHREAD_CONFIG_MULTI_RADIO

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
        OT_ASSERT(aFrameInfo.mIsEmptyFrame || aFrameInfo.mAckRequest);

        if ((aError == kErrorNone) && (aAckFrame != nullptr))
        {
            if (IsEnabled() && ackFrameInfo.mFramePending)
            {
                StartOperation(kOperationWaitingForData);
            }

            LogInfo("Sent data poll, fp:%s", ToYesNo(ackFrameInfo.mFramePending));
        }

        mCounters.mTxDataPoll++;
        FinishOperation();
        Get<DataPollSender>().HandlePollSent(aFrameInfo, aError);
        PerformNextOperation();
        break;

    case kOperationTransmitDataDirect:
        mCounters.mTxData++;

        if (aError != kErrorNone)
        {
            mCounters.mTxDirectMaxRetryExpiry++;
        }
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
        else if (mLinks.GetTransmitRetries() < OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT)
        {
            mRetryHistogram.mTxDirectRetrySuccess[mLinks.GetTransmitRetries()]++;
        }
#endif

        DumpDebg("TX", aFrameInfo.GetTxFrame().mPsdu, aFrameInfo.GetTxFrame().mLength);
        FinishOperation();
        Get<MeshForwarder>().HandleSentFrame(aFrameInfo, aError);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        Get<DataPollSender>().ProcessTxDone(aFrameInfo, ackFrameInfo, aError);
#endif
        PerformNextOperation();
        break;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        mCounters.mTxData++;

        DumpDebg("TX", aFrameInfo.GetTxFrame().mPsdu, aFrameInfo.GetTxFrame().mLength);
        FinishOperation();
        Get<CslTxScheduler>().HandleSentFrame(aFrameInfo, aError);
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
        else if (mLinks.GetTransmitRetries() < OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT)
        {
            mRetryHistogram.mTxIndirectRetrySuccess[mLinks.GetTransmitRetries()]++;
        }
#endif

        DumpDebg("TX", aFrameInfo.GetTxFrame().mPsdu, aFrameInfo.GetTxFrame().mLength);
        FinishOperation();
        Get<DataPollHandler>().HandleSentFrame(aFrameInfo, aError);
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

Error Mac::ProcessReceiveSecurity(RxFrame::Info &aFrameInfo, Neighbor *aNeighbor)
{
    KeyManager        &keyManager = Get<KeyManager>();
    Error              error      = kErrorSecurity;
    uint8_t            keyId;
    uint32_t           keySequence = 0;
    const KeyMaterial *macKey;
    const ExtAddress  *extAddress;

    VerifyOrExit(aFrameInfo.mFullyValidated);

    VerifyOrExit(aFrameInfo.mSecurityEnabled, error = kErrorNone);
    VerifyOrExit(aFrameInfo.mSecurityLevel == Frame::kSecurityEncMic32);

    LogDebg("Rx security - frame counter %lu", ToUlong(aFrameInfo.mFrameCounter));

    switch (aFrameInfo.mKeyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit(keyManager.IsKekSet(), error = kErrorSecurity);
        macKey     = &keyManager.GetKek();
        extAddress = &aFrameInfo.GetSrcAddr().GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != nullptr);

        keyId = aFrameInfo.mKeyId;
        keyId--;

        if (keyId == (keyManager.GetCurrentKeySequence() & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence();
            macKey      = mLinks.GetCurrentMacKey(aFrameInfo.GetRxFrame());
        }
        else if (keyId == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() - 1;
            macKey      = mLinks.GetTemporaryMacKey(aFrameInfo.GetRxFrame(), keySequence);
        }
        else if (keyId == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() + 1;
            macKey      = mLinks.GetTemporaryMacKey(aFrameInfo.GetRxFrame(), keySequence);
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
                uint32_t neighborFrameCounter;

#if OPENTHREAD_CONFIG_MULTI_RADIO
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get(aFrameInfo.mRadioType);
#else
                neighborFrameCounter = aNeighbor->GetLinkFrameCounters().Get();
#endif

                // If frame counter is one off, then frame is a duplicate.
                VerifyOrExit((aFrameInfo.mFrameCounter + 1) != neighborFrameCounter, error = kErrorDuplicated);

                VerifyOrExit(aFrameInfo.mFrameCounter >= neighborFrameCounter);
            }
        }

        extAddress = &aFrameInfo.GetSrcAddr().GetExtended();

        break;

    case Frame::kKeyIdMode2:
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        if (aFrameInfo.GetRxFrame().IsWakeupFrame())
        {
            uint32_t sequence;

            // TODO: Avoid generating a new key if a wake-up frame was recently received already

            keyId    = aFrameInfo.mKeyId;
            sequence = BigEndian::ReadUint32(aFrameInfo.mKeySource.GetBytes());
            VerifyOrExit(((sequence & 0x7f) + 1) == keyId, error = kErrorSecurity);

            macKey     = (sequence == keyManager.GetCurrentKeySequence()) ? mLinks.GetCurrentMacKey(aFrameInfo.GetRxFrame())
                                                                          : &keyManager.GetTemporaryMacKey(sequence);
            extAddress = &aFrameInfo.GetSrcAddr().GetExtended();
        }
        else
#endif
        {
            macKey     = &mMode2KeyMaterial;
            extAddress = &AsCoreType(&sMode2ExtAddress);
        }
        break;

    default:
        ExitNow();
    }

    SuccessOrExit(aFrameInfo.ProcessReceiveAesCcm(*extAddress, *macKey));

    if ((aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1) && aNeighbor->IsStateValid())
    {
        if (aNeighbor->GetKeySequence() != keySequence)
        {
            aNeighbor->SetKeySequence(keySequence);
            aNeighbor->SetMleFrameCounter(0);
            aNeighbor->GetLinkFrameCounters().Reset();
        }

#if OPENTHREAD_CONFIG_MULTI_RADIO
        aNeighbor->GetLinkFrameCounters().Set(aFrameInfo.mRadioType, aFrameInfo.mFrameCounter + 1);
#else
        aNeighbor->GetLinkFrameCounters().Set(aFrameInfo.mFrameCounter + 1);
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
        if (aFrameInfo.mRadioType == kRadioTypeIeee802154)
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
Error Mac::ProcessEnhAckSecurity(TxFrame::Info &aTxFrameInfo, RxFrame::Info &aAckFrameInfo)
{
    Error              error = kErrorSecurity;
    uint8_t            ackKeyId;
    Neighbor          *neighbor   = nullptr;
    KeyManager        &keyManager = Get<KeyManager>();
    const KeyMaterial *macKey;

    VerifyOrExit(aAckFrameInfo.mFullyValidated);

    VerifyOrExit(aAckFrameInfo.mSecurityEnabled, error = kErrorNone);

    VerifyOrExit(aAckFrameInfo.mVersion == Frame::kVersion2015);
    VerifyOrExit(aAckFrameInfo.mSecurityLevel == Frame::kSecurityEncMic32);
    VerifyOrExit(aAckFrameInfo.mKeyIdMode == Frame::kKeyIdMode1);

    ackKeyId = aAckFrameInfo.mKeyId;
    VerifyOrExit(ackKeyId == aTxFrameInfo.mKeyId);

    LogDebg("Rx security - Ack frame counter %lu", ToUlong(aAckFrameInfo.mFrameCounter));

    if (!aAckFrameInfo.GetSrcAddr().IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(aAckFrameInfo.GetSrcAddr());
    }
    else if (!aTxFrameInfo.GetDstAddr().IsNone())
    {
        // Get neighbor from destination address of transmitted frame
        neighbor = Get<NeighborTable>().FindNeighbor(aTxFrameInfo.GetDstAddr());
    }

    if (!aAckFrameInfo.GetSrcAddr().IsExtended() && neighbor != nullptr)
    {
        aAckFrameInfo.mAddrs.mSource.SetExtended(neighbor->GetExtAddress());
    }

    VerifyOrExit(aAckFrameInfo.GetSrcAddr().IsExtended() && neighbor != nullptr);

    ackKeyId--;

    if (ackKeyId == (keyManager.GetCurrentKeySequence() & 0x7f))
    {
        macKey = &mLinks.GetSubMac().GetCurrentMacKey();
    }
    else if (ackKeyId == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
    {
        macKey = &mLinks.GetSubMac().GetPreviousMacKey();
    }
    else if (ackKeyId == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
    {
        macKey = &mLinks.GetSubMac().GetNextMacKey();
    }
    else
    {
        ExitNow();
    }

    if (neighbor->IsStateValid())
    {
        VerifyOrExit(aAckFrameInfo.mFrameCounter >= neighbor->GetLinkAckFrameCounter());
    }

    error = aAckFrameInfo.ProcessReceiveAesCcm(aAckFrameInfo.GetSrcAddr().GetExtended(), *macKey);
    SuccessOrExit(error);

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
    Error         error    = aError;
    Neighbor     *neighbor = nullptr;
    RxFrame::Info frameInfo;

    mCounters.mRxTotal++;

    SuccessOrExit(error);
    VerifyOrExit(aFrame != nullptr, error = kErrorNoFrameReceived);
    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

    SuccessOrExit(error = frameInfo.ParseFrom(*aFrame));

    if (!frameInfo.GetSrcAddr().IsNone())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(frameInfo.GetSrcAddr());
    }

    // Destination Address Filtering
    switch (frameInfo.GetDstAddr().GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        SuccessOrExit(error = FilterDestShortAddress(frameInfo.GetDstAddr().GetShort()));

#if OPENTHREAD_FTD
        // Allow multicasts from neighbor routers if FTD
        if (neighbor == nullptr && frameInfo.GetDstAddr().IsBroadcast() && Get<Mle::Mle>().IsFullThreadDevice())
        {
            neighbor = Get<NeighborTable>().FindRxOnlyNeighborRouter(frameInfo.GetSrcAddr());
        }
#endif

        break;

    case Address::kTypeExtended:
        VerifyOrExit(frameInfo.GetDstAddr().GetExtended() == GetExtAddress(), error = kErrorDestinationAddressFiltered);
        break;
    }

    // PAN ID Filtering
    if (frameInfo.mPanIds.IsDestinationPresent())
    {
        PanId panId = frameInfo.mPanIds.GetDestination();

        VerifyOrExit(panId == kShortAddrBroadcast || panId == mPanId, error = kErrorDestinationAddressFiltered);
    }

    // Source Address Filtering
    switch (frameInfo.GetSrcAddr().GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        LogDebg("Received frame from short address 0x%04x", frameInfo.GetSrcAddr().GetShort());

        VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);

        frameInfo.mAddrs.mSource.SetExtended(neighbor->GetExtAddress());

        OT_FALL_THROUGH;

    case Address::kTypeExtended:

        // Duplicate Address Protection
        VerifyOrExit(frameInfo.GetSrcAddr().GetExtended() != GetExtAddress(), error = kErrorInvalidSourceAddress);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        SuccessOrExit(error = mFilter.ApplyToRxFrame(*aFrame, frameInfo.GetSrcAddr().GetExtended(), neighbor));
#endif

        break;
    }

    if (frameInfo.GetDstAddr().IsBroadcast())
    {
        mCounters.mRxBroadcast++;
    }
    else
    {
        mCounters.mRxUnicast++;
    }

    error = ProcessReceiveSecurity(frameInfo, neighbor);

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
    ProcessCsl(frameInfo, frameInfo.GetSrcAddr());
#endif

    Get<DataPollSender>().ProcessRxFrame(frameInfo);

    if (neighbor != nullptr)
    {
        UpdateNeighborLinkInfo(*neighbor, frameInfo);

        if (frameInfo.mSecurityEnabled)
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
                    VerifyOrExit(frameInfo.IsMacCommand(Frame::kMacCmdDataRequest), error = kErrorDrop);
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
            Get<RadioSelector>().UpdateOnReceive(*neighbor, frameInfo.mRadioType,
                                                 /* aIsDuplicate */ false);
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

        if (!frameInfo.GetDstAddr().IsNone())
        {
            mTimer.Stop();

#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
            if (!mRxOnWhenIdle && !mPromiscuous && frameInfo.mFramePending)
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
        if (HandleMacCommand(frameInfo)) // returns `true` when handled
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
        SuccessOrExit(error = HandleWakeupFrame(frameInfo));
        OT_FALL_THROUGH;
#endif

    default:
        mCounters.mRxOther++;
        ExitNow();
    }

    DumpDebg("RX", frameInfo.mHeader.GetBytes(), frameInfo.mHeader.GetLength());
    Get<MeshForwarder>().HandleReceivedFrame(frameInfo);

    UpdateIdleMode();

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
    if (aFrame->GetRadioType() == kRadioTypeTrel)
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

void Mac::UpdateNeighborLinkInfo(Neighbor &aNeighbor, const RxFrame::Info &aFrameInfo)
{
    LinkQuality oldLinkQuality = aNeighbor.GetLinkInfo().GetLinkQualityIn();

    VerifyOrExit(aFrameInfo.mFullyValidated);

    aNeighbor.GetLinkInfo().AddRss(aFrameInfo.GetRxFrame().GetRssi());

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    aNeighbor.AggregateLinkMetrics(/* aSeriesId */ 0, aFrameInfo.mType, aFrameInfo.GetRxFrame().GetLqi(),
                                   aFrameInfo.GetRxFrame().GetRssi());
#endif

    // Signal when `aNeighbor` is the current parent and its link
    // quality gets changed.

    VerifyOrExit(Get<Mle::Mle>().IsChild() && (&aNeighbor == &Get<Mle::Mle>().GetParent()));
    VerifyOrExit(aNeighbor.GetLinkInfo().GetLinkQualityIn() != oldLinkQuality);
    Get<Notifier>().Signal(kEventParentLinkQualityChanged);

exit:
    return;
}

bool Mac::HandleMacCommand(RxFrame::Info &aFrameInfo)
{
    bool didHandle = false;

    switch (aFrameInfo.mCommandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        LogInfo("Received Beacon Request");

        if (ShouldSendBeacon())
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            mTxBeaconRadioLinks.Add(aFrameInfo.mRadioType);
#endif
            StartOperation(kOperationTransmitBeacon);
        }

        didHandle = true;
        break;

    case Frame::kMacCmdDataRequest:
        mCounters.mRxDataPoll++;
#if OPENTHREAD_FTD
        Get<DataPollHandler>().HandleDataPoll(aFrameInfo);
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

    mLinks.SetRxOnWhenIdle(mRxOnWhenIdle || mPromiscuous);
    UpdateIdleMode();
}

Error Mac::SetRegion(uint16_t aRegionCode)
{
    Error       error;
    ChannelMask oldMask = mSupportedChannelMask;

    SuccessOrExit(error = Get<Radio>().SetRegion(aRegionCode));
    mSupportedChannelMask.SetMask(Get<Radio>().GetSupportedChannelMask());
    IgnoreError(Get<Notifier>().Update(oldMask, mSupportedChannelMask, kEventSupportedChannelMaskChanged));

exit:
    return error;
}

Error Mac::GetRegion(uint16_t &aRegionCode) const { return Get<Radio>().GetRegion(aRegionCode); }

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

void Mac::ResetRetrySuccessHistogram() { ClearAllBytes(mRetryHistogram); }
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

void Mac::LogFrameRxFailure(const RxFrame::Info &aFrameInfo, Error aError) const
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

void Mac::LogFrameTxFailure(const TxFrame::Info &aFrameInfo, Error aError, uint8_t aRetryCount, bool aWillRetx) const
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrameInfo.mRadioType == kRadioTypeIeee802154)
#endif
    {
        uint8_t maxAttempts = aFrameInfo.GetTxFrame().GetMaxFrameRetries() + 1;
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
    if (aFrameInfo.mRadioType == kRadioTypeTrel)
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

void Mac::LogFrameRxFailure(const RxFrame::Info &, Error) const {}

void Mac::LogBeacon(const char *) const {}

void Mac::LogFrameTxFailure(const TxFrame::Info &, Error, uint8_t, bool) const {}

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
    return static_cast<uint32_t>(aPeriodInTenSymbols) * kUsPerTenSymbols;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

void Mac::ProcessCsl(const RxFrame::Info &aFrameInfo, const Address &aSrcAddr)
{
    CslNeighbor *neighbor = nullptr;
    const CslIe *csl;

    VerifyOrExit(aFrameInfo.mFullyValidated);

    VerifyOrExit(aFrameInfo.mVersion == Frame::kVersion2015);
    VerifyOrExit(aFrameInfo.IsSecuredWith(RxFrame::kAllowKeyIdMode1));

    csl = aFrameInfo.GetRxFrame().Find<CslIe>();
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
    neighbor->SetLastRxTimestamp(aFrameInfo.GetRxFrame().GetTimestamp());
    LogDebg("Timestamp=%lu Sequence=%u CslPeriod=%u CslPhase=%u TransmitPhase=%u",
            ToUlong(ConvertRadioTime64To32(aFrameInfo.GetRxFrame().GetTimestamp())), aFrameInfo.mSequenceNum,
            csl->GetPeriod(), csl->GetPhase(), neighbor->GetCslPhase());

#if OPENTHREAD_FTD
    Get<CslTxScheduler>().Update();
#endif

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mac::ProcessEnhAckProbing(const RxFrame::Info &aFrameInfo, const Neighbor &aNeighbor)
{
    const LinkMetricsProbingIe *probingIe;
    uint8_t                     dataLen;

    VerifyOrExit(aFrameInfo.mFullyValidated);

    probingIe = aFrameInfo.GetRxFrame().Find<LinkMetricsProbingIe>();

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

    VerifyOrExit(aDuration >= kMinWakeupListenDuration, error = kErrorInvalidArgs);
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

Error Mac::HandleWakeupFrame(const RxFrame::Info &aFrameInfo)
{
    Error               error = kErrorNone;
    const ConnectionIe *connectionIe;
    Address             srcAddress;
    WakeupInfo          wakeupInfo;
    RadioTime32         rvTimeUs;
    RadioTime64         rvTimestampUs;
    RadioTime64         radioNowUs;

    VerifyOrExit(mWakeupListenEnabled && aFrameInfo.GetRxFrame().IsWakeupFrame());

    VerifyOrExit(aFrameInfo.GetSrcAddr().IsExtended(), error = kErrorDrop);

    wakeupInfo.mExtAddress    = aFrameInfo.GetSrcAddr().GetExtended();
    connectionIe              = aFrameInfo.GetRxFrame().Find<ConnectionIe>();
    wakeupInfo.mRetryInterval = connectionIe->GetRetryInterval();
    wakeupInfo.mRetryCount    = connectionIe->GetRetryCount();
    VerifyOrExit(wakeupInfo.mRetryInterval > 0 && wakeupInfo.mRetryCount > 0, error = kErrorInvalidArgs);

    radioNowUs    = Get<Radio>().GetNow();
    rvTimeUs      = aFrameInfo.GetRxFrame().Find<RendezvousTimeIe>()->GetRendezvousTime() * kUsPerTenSymbols;
    rvTimestampUs = aFrameInfo.GetRxFrame().GetTimestamp() + kRadioHeaderPhrDuration +
                    aFrameInfo.GetRxFrame().GetLength() * kOctetDuration + rvTimeUs;

    if (rvTimestampUs > radioNowUs + kCslRequestAhead)
    {
        wakeupInfo.mAttachDelayMs = ConvertRadioTime64To32(rvTimestampUs - radioNowUs - kCslRequestAhead);
        wakeupInfo.mAttachDelayMs = wakeupInfo.mAttachDelayMs / Time::kOneMsecInUsec;
    }
    else
    {
        wakeupInfo.mAttachDelayMs = 0;
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    {
        LogInfo("Received wake-up frame, fc:%lu, rendezvous:%luus, retries:%u/%u", ToUlong(aFrameInfo.mFrameCounter),
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

uint32_t Mac::CalculateRadioBusTransferTime(uint16_t aFrameSize) const
{
    uint32_t busSpeed     = Get<Radio>().GetBusSpeed();
    uint32_t trasnferTime = 0;

    if (busSpeed != 0)
    {
        trasnferTime = DivideAndRoundUp<uint32_t>(aFrameSize * kBitsPerByte * Time::kOneSecondInUsec, busSpeed);
    }

    trasnferTime += Get<Radio>().GetBusLatency();

    return trasnferTime;
}

} // namespace Mac
} // namespace ot
