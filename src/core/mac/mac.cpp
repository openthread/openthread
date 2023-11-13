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

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/random.hpp"
#include "common/string.hpp"
#include "crypto/aes_ccm.hpp"
#include "crypto/sha256.hpp"
#include "instance/instance.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"
#include "thread/child.hpp"
#include "thread/child_table.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_router.hpp"
#include "thread/neighbor.hpp"
#include "thread/thread_netif.hpp"

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
    , mOperation(kOperationIdle)
    , mPendingOperations(0)
    , mBeaconSequence(Random::NonCrypto::GetUint8())
    , mDataSequence(Random::NonCrypto::GetUint8())
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
    , mCslChannel(0)
    , mCslPeriod(0)
#endif
    , mActiveScanHandler(nullptr) // Initialize `mActiveScanHandler` and `mEnergyScanHandler` union
    , mScanHandlerContext(nullptr)
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

    Get<KeyManager>().UpdateKeyMaterial();
    SetPanId(mPanId);
    SetExtAddress(randomExtAddress);
    SetShortAddress(GetShortAddress());

    mMode2KeyMaterial.SetFrom(AsCoreType(&sMode2Key));
}

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

Error Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = kErrorBusy);

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

Error Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = kErrorBusy);

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
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
#endif
#endif
    case kOperationTransmitBeacon:
    case kOperationTransmitPoll:
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

Error Mac::ConvertBeaconToActiveScanResult(const RxFrame *aBeaconFrame, ActiveScanResult &aResult)
{
    Error   error = kErrorNone;
    Address address;
#if OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
    const BeaconPayload *beaconPayload = nullptr;
    const Beacon        *beacon        = nullptr;
    uint16_t             payloadLength;
#endif

    memset(&aResult, 0, sizeof(ActiveScanResult));

    VerifyOrExit(aBeaconFrame != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(aBeaconFrame->GetType() == Frame::kTypeBeacon, error = kErrorParse);
    SuccessOrExit(error = aBeaconFrame->GetSrcAddr(address));
    VerifyOrExit(address.IsExtended(), error = kErrorParse);
    aResult.mExtAddress = address.GetExtended();

    if (kErrorNone != aBeaconFrame->GetSrcPanId(aResult.mPanId))
    {
        IgnoreError(aBeaconFrame->GetDstPanId(aResult.mPanId));
    }

    aResult.mChannel = aBeaconFrame->GetChannel();
    aResult.mRssi    = aBeaconFrame->GetRssi();
    aResult.mLqi     = aBeaconFrame->GetLqi();

#if OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
    payloadLength = aBeaconFrame->GetPayloadLength();

    beacon        = reinterpret_cast<const Beacon *>(aBeaconFrame->GetPayload());
    beaconPayload = reinterpret_cast<const BeaconPayload *>(beacon->GetPayload());

    if ((payloadLength >= (sizeof(*beacon) + sizeof(*beaconPayload))) && beacon->IsValid() && beaconPayload->IsValid())
    {
        aResult.mVersion    = beaconPayload->GetProtocolVersion();
        aResult.mIsJoinable = beaconPayload->IsJoiningPermitted();
        aResult.mIsNative   = beaconPayload->IsNative();
        IgnoreError(AsCoreType(&aResult.mNetworkName).Set(beaconPayload->GetNetworkName()));
        VerifyOrExit(IsValidUtf8String(aResult.mNetworkName.m8), error = kErrorParse);
        aResult.mExtendedPanId = beaconPayload->GetExtendedPanId();
    }
#endif

    LogBeacon("Received");

exit:
    return error;
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
        BeginTransmit();
    }
    else
    {
        mLinks.SetPanId(mPanId);
        FinishOperation();
        ReportActiveScanResult(nullptr);
        PerformNextOperation();
    }
}

void Mac::ReportActiveScanResult(const RxFrame *aBeaconFrame)
{
    VerifyOrExit(mActiveScanHandler != nullptr);

    if (aBeaconFrame == nullptr)
    {
        mActiveScanHandler(nullptr, mScanHandlerContext);
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
        if (!GetRxOnWhenIdle())
        {
            mLinks.Receive(mScanChannel);
        }
        error = mLinks.EnergyScan(mScanChannel, mScanDuration);
    }

exit:

    if (error != kErrorNone)
    {
        FinishOperation();

        if (mEnergyScanHandler != nullptr)
        {
            mEnergyScanHandler(nullptr, mScanHandlerContext);
        }

        PerformNextOperation();
    }
}

void Mac::ReportEnergyScanResult(int8_t aRssi)
{
    EnergyScanResult result;

    VerifyOrExit((mEnergyScanHandler != nullptr) && (aRssi != Radio::kInvalidRssi));

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

    VerifyOrExit(mSupportedChannelMask.ContainsChannel(aChannel), error = kErrorInvalidArgs);

    SuccessOrExit(Get<Notifier>().Update(mPanChannel, aChannel, kEventThreadChannelChanged));

    mCcaSuccessRateTracker.Clear();

    VerifyOrExit(!mUsingTemporaryChannel);

    mRadioChannel = mPanChannel;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    UpdateCsl();
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
#endif // OPENTHREAD_FTD

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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    else if (IsPending(kOperationTransmitDataCsl))
    {
        mTimer.FireAt(mCslTxFireTime);
    }
#endif

    if (shouldSleep)
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (IsCslEnabled())
        {
            mLinks.CslSample();
            ExitNow();
        }
#endif
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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
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
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
#endif
#endif
    case kOperationTransmitPoll:
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

TxFrame *Mac::PrepareBeaconRequest(void)
{
    TxFrame  &frame = mLinks.GetTxFrames().GetBroadcastTxFrame();
    Addresses addrs;
    PanIds    panIds;

    addrs.mSource.SetNone();
    addrs.mDestination.SetShort(kShortAddrBroadcast);
    panIds.SetDestination(kShortAddrBroadcast);

    frame.InitMacHeader(Frame::kTypeMacCmd, Frame::kVersion2003, addrs, panIds, Frame::kSecurityNone);

    IgnoreError(frame.SetCommandId(Frame::kMacCmdBeaconRequest));

    LogInfo("Sending Beacon Request");

    return &frame;
}

TxFrame *Mac::PrepareBeacon(void)
{
    TxFrame  *frame;
    Beacon   *beacon = nullptr;
    Addresses addrs;
    PanIds    panIds;
#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
    uint8_t        beaconLength;
    BeaconPayload *beaconPayload = nullptr;
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    OT_ASSERT(!mTxBeaconRadioLinks.IsEmpty());
    frame = &mLinks.GetTxFrames().GetTxFrame(mTxBeaconRadioLinks);
    mTxBeaconRadioLinks.Clear();
#else
    frame = &mLinks.GetTxFrames().GetBroadcastTxFrame();
#endif

    addrs.mSource.SetExtended(GetExtAddress());
    panIds.SetSource(mPanId);
    addrs.mDestination.SetNone();

    frame->InitMacHeader(Frame::kTypeBeacon, Frame::kVersion2003, addrs, panIds, Frame::kSecurityNone);

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

    beaconPayload->SetNetworkName(Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsData());
    beaconPayload->SetExtendedPanId(Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());

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
    uint8_t           keyIdMode;
    const ExtAddress *extAddress = nullptr;

    VerifyOrExit(aFrame.GetSecurityEnabled());

    IgnoreError(aFrame.GetKeyIdMode(keyIdMode));

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        aFrame.SetAesKey(keyManager.GetKek());
        extAddress = &GetExtAddress();

        if (!aFrame.IsHeaderUpdated())
        {
            aFrame.SetFrameCounter(keyManager.GetKekFrameCounter());
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
        VerifyOrExit(aFrame.GetRadioType() != kRadioTypeIeee802154);
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
            mLinks.SetMacFrameCounter(aFrame);
            aFrame.SetKeyId((keyManager.GetCurrentKeySequence() & 0x7f) + 1);
        }
#endif
        break;

    case Frame::kKeyIdMode2:
    {
        const uint8_t keySource[] = {0xff, 0xff, 0xff, 0xff};

        aFrame.SetAesKey(mMode2KeyMaterial);

        mKeyIdMode2FrameCounter++;
        aFrame.SetFrameCounter(mKeyIdMode2FrameCounter);
        aFrame.SetKeySource(keySource);
        aFrame.SetKeyId(0xff);
        extAddress = &AsCoreType(&sMode2ExtAddress);
        break;
    }

    default:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(aFrame.GetTimeIeOffset() == 0);
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
    TxFrames &txFrames = mLinks.GetTxFrames();
    Address   dstAddr;

    txFrames.Clear();

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mTxPendingRadioLinks.Clear();
    mTxError = kErrorAbort;
#endif

    VerifyOrExit(IsEnabled());

    switch (mOperation)
    {
    case kOperationActiveScan:
        mLinks.SetPanId(kPanIdBroadcast);
        frame = PrepareBeaconRequest();
        VerifyOrExit(frame != nullptr);
        frame->SetChannel(mScanChannel);
        frame->SetSequence(0);
        frame->SetMaxCsmaBackoffs(kMaxCsmaBackoffsDirect);
        frame->SetMaxFrameRetries(mMaxFrameRetriesDirect);
        break;

    case kOperationTransmitBeacon:
        frame = PrepareBeacon();
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
#endif // OPENTHREAD_FTD

    default:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        uint8_t timeIeOffset = GetTimeIeOffset(*frame);

        frame->SetTimeIeOffset(timeIeOffset);

        if (timeIeOffset != 0)
        {
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

void Mac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    bool ackRequested = aFrame.GetAckRequest();

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (!aFrame.IsEmpty()
#if OPENTHREAD_CONFIG_MULTI_RADIO
        && (aFrame.GetRadioType() == kRadioTypeIeee802154)
#endif
    )
    {
        Address dstAddr;

        IgnoreError(aFrame.GetDstAddr(dstAddr));

        // Determine whether to re-transmit a broadcast frame.
        if (dstAddr.IsBroadcast())
        {
            mBroadcastTransmitCount++;

            if (mBroadcastTransmitCount < kTxNumBcast)
            {
#if OPENTHREAD_CONFIG_MULTI_RADIO
                {
                    RadioTypes radioTypes;
                    radioTypes.Add(kRadioTypeIeee802154);
                    mLinks.Send(aFrame, radioTypes);
                }
#else
                mLinks.Send();
#endif
                ExitNow();
            }

            mBroadcastTransmitCount = 0;
        }

        if (ackRequested && (aAckFrame != nullptr))
        {
            Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(dstAddr);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
            if ((aError == kErrorNone) && (neighbor != nullptr) &&
                (mFilter.ApplyToRxFrame(*aAckFrame, neighbor->GetExtAddress(), neighbor) != kErrorNone))
            {
                aError = kErrorNoAck;
            }
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
            // Verify Enh-ACK integrity by checking its MIC
            if ((aError == kErrorNone) && (ProcessEnhAckSecurity(aFrame, *aAckFrame) != kErrorNone))
            {
                aError = kErrorNoAck;
            }
#endif

            if ((aError == kErrorNone) && (neighbor != nullptr))
            {
                UpdateNeighborLinkInfo(*neighbor, *aAckFrame);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
                ProcessEnhAckProbing(*aAckFrame, *neighbor);
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
                ProcessCsl(*aAckFrame, dstAddr);
#endif
            }
        }
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (!aFrame.IsEmpty())
    {
        RadioType  radio          = aFrame.GetRadioType();
        RadioTypes requiredRadios = mLinks.GetTxFrames().GetRequiredRadioTypes();

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
                LogDebg("Frame tx failed on required radio link %s with error %s", RadioTypeToString(radio),
                        ErrorToString(aError));

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
        OT_ASSERT(aFrame.IsEmpty() || ackRequested);

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
        else if (mLinks.GetTransmitRetries() < OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT)
        {
            mRetryHistogram.mTxDirectRetrySuccess[mLinks.GetTransmitRetries()]++;
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

#if OPENTHREAD_FTD
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kOperationTransmitDataCsl:
        mCounters.mTxData++;

        DumpDebg("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<CslTxScheduler>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();

        break;
#endif
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

        DumpDebg("TX", aFrame.GetHeader(), aFrame.GetLength());
        FinishOperation();
        Get<DataPollHandler>().HandleSentFrame(aFrame, aError);
        PerformNextOperation();
        break;
#endif // OPENTHREAD_FTD

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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
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

Error Mac::ProcessReceiveSecurity(RxFrame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager        &keyManager = Get<KeyManager>();
    Error              error      = kErrorSecurity;
    uint8_t            securityLevel;
    uint8_t            keyIdMode;
    uint32_t           frameCounter;
    uint8_t            keyid;
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
        macKey     = &keyManager.GetKek();
        extAddress = &aSrcAddr.GetExtended();
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != nullptr);

        IgnoreError(aFrame.GetKeyId(keyid));
        keyid--;

        if (keyid == (keyManager.GetCurrentKeySequence() & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence();
            macKey      = mLinks.GetCurrentMacKey(aFrame);
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() - 1;
            macKey      = mLinks.GetTemporaryMacKey(aFrame, keySequence);
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            keySequence = keyManager.GetCurrentKeySequence() + 1;
            macKey      = mLinks.GetTemporaryMacKey(aFrame, keySequence);
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
        macKey     = &mMode2KeyMaterial;
        extAddress = &AsCoreType(&sMode2ExtAddress);
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
        if (aFrame.GetRadioType() == kRadioTypeIeee802154)
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
            keyManager.SetCurrentKeySequence(keySequence);
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
    uint8_t            txKeyId;
    uint8_t            ackKeyId;
    uint8_t            keyIdMode;
    uint32_t           frameCounter;
    Address            srcAddr;
    Address            dstAddr;
    Neighbor          *neighbor   = nullptr;
    KeyManager        &keyManager = Get<KeyManager>();
    const KeyMaterial *macKey;

    VerifyOrExit(aAckFrame.GetSecurityEnabled(), error = kErrorNone);
    VerifyOrExit(aAckFrame.IsVersion2015());

    SuccessOrExit(aAckFrame.ValidatePsdu());

    IgnoreError(aAckFrame.GetSecurityLevel(securityLevel));
    VerifyOrExit(securityLevel == Frame::kSecurityEncMic32);

    IgnoreError(aAckFrame.GetKeyIdMode(keyIdMode));
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1);

    IgnoreError(aTxFrame.GetKeyId(txKeyId));
    IgnoreError(aAckFrame.GetKeyId(ackKeyId));

    VerifyOrExit(txKeyId == ackKeyId);

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

void Mac::HandleReceivedFrame(RxFrame *aFrame, Error aError)
{
    Address   srcaddr;
    Address   dstaddr;
    PanId     panid;
    Neighbor *neighbor;
    Error     error = aError;

    mCounters.mRxTotal++;

    SuccessOrExit(error);
    VerifyOrExit(aFrame != nullptr, error = kErrorNoFrameReceived);
    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    IgnoreError(aFrame->GetSrcAddr(srcaddr));
    IgnoreError(aFrame->GetDstAddr(dstaddr));
    neighbor = !srcaddr.IsNone() ? Get<NeighborTable>().FindNeighbor(srcaddr) : nullptr;

    // Destination Address Filtering
    switch (dstaddr.GetType())
    {
    case Address::kTypeNone:
        break;

    case Address::kTypeShort:
        VerifyOrExit((mRxOnWhenIdle && dstaddr.IsBroadcast()) || dstaddr.GetShort() == GetShortAddress(),
                     error = kErrorDestinationAddressFiltered);

#if OPENTHREAD_FTD
        // Allow multicasts from neighbor routers if FTD
        if (neighbor == nullptr && dstaddr.IsBroadcast() && Get<Mle::MleRouter>().IsFullThreadDevice())
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

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
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
        LogFrameRxFailure(aFrame, error);

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
}

void Mac::UpdateNeighborLinkInfo(Neighbor &aNeighbor, const RxFrame &aRxFrame)
{
    LinkQuality oldLinkQuality = aNeighbor.GetLinkInfo().GetLinkQuality();

    aNeighbor.GetLinkInfo().AddRss(aRxFrame.GetRssi());

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    aNeighbor.AggregateLinkMetrics(/* aSeriesId */ 0, aRxFrame.GetType(), aRxFrame.GetLqi(), aRxFrame.GetRssi());
#endif

    // Signal when `aNeighbor` is the current parent and its link
    // quality gets changed.

    VerifyOrExit(Get<Mle::Mle>().IsChild() && (&aNeighbor == &Get<Mle::Mle>().GetParent()));
    VerifyOrExit(aNeighbor.GetLinkInfo().GetLinkQuality() != oldLinkQuality);
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

void Mac::ResetRetrySuccessHistogram() { memset(&mRetryHistogram, 0, sizeof(mRetryHistogram)); }
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

uint8_t Mac::ComputeLinkMargin(int8_t aRss) const { return ot::ComputeLinkMargin(GetNoiseFloor(), aRss); }

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Mac::OperationToString(Operation aOperation)
{
    static const char *const kOperationStrings[] = {
        "Idle",               // (0) kOperationIdle
        "ActiveScan",         // (1) kOperationActiveScan
        "EnergyScan",         // (2) kOperationEnergyScan
        "TransmitBeacon",     // (3) kOperationTransmitBeacon
        "TransmitDataDirect", // (4) kOperationTransmitDataDirect
        "TransmitPoll",       // (5) kOperationTransmitPoll
        "WaitingForData",     // (6) kOperationWaitingForData
#if OPENTHREAD_FTD
        "TransmitDataIndirect", // (7) kOperationTransmitDataIndirect
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        "TransmitDataCsl", // (8) kOperationTransmitDataCsl
#endif
#endif
    };

    static_assert(kOperationIdle == 0, "kOperationIdle value is incorrect");
    static_assert(kOperationActiveScan == 1, "kOperationActiveScan value is incorrect");
    static_assert(kOperationEnergyScan == 2, "kOperationEnergyScan value is incorrect");
    static_assert(kOperationTransmitBeacon == 3, "kOperationTransmitBeacon value is incorrect");
    static_assert(kOperationTransmitDataDirect == 4, "kOperationTransmitDataDirect value is incorrect");
    static_assert(kOperationTransmitPoll == 5, "kOperationTransmitPoll value is incorrect");
    static_assert(kOperationWaitingForData == 6, "kOperationWaitingForData value is incorrect");
#if OPENTHREAD_FTD
    static_assert(kOperationTransmitDataIndirect == 7, "kOperationTransmitDataIndirect value is incorrect");
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    static_assert(kOperationTransmitDataCsl == 8, "TransmitDataCsl value is incorrect");
#endif
#endif

    return kOperationStrings[aOperation];
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
    if (aFrame.GetRadioType() == kRadioTypeIeee802154)
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
    if (aFrame.GetRadioType() == kRadioTypeTrel)
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

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
uint8_t Mac::GetTimeIeOffset(const Frame &aFrame)
{
    uint8_t        offset = 0;
    const uint8_t *base   = aFrame.GetPsdu();
    const uint8_t *cur    = nullptr;

    cur = reinterpret_cast<const uint8_t *>(aFrame.GetTimeIe());
    VerifyOrExit(cur != nullptr);

    cur += sizeof(VendorIeHeader);
    offset = static_cast<uint8_t>(cur - base);

exit:
    return offset;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Mac::UpdateCsl(void)
{
    uint16_t period  = IsCslEnabled() ? GetCslPeriod() : 0;
    uint8_t  channel = GetCslChannel() ? GetCslChannel() : mRadioChannel;

    if (mLinks.UpdateCsl(period, channel, Get<Mle::Mle>().GetParent().GetRloc16(),
                         &Get<Mle::Mle>().GetParent().GetExtAddress()))
    {
        if (Get<Mle::Mle>().IsChild())
        {
            Get<DataPollSender>().RecalculatePollPeriod();

            if (period != 0)
            {
                Get<Mle::Mle>().ScheduleChildUpdateRequest();
            }
        }

        UpdateIdleMode();
    }
}

void Mac::SetCslChannel(uint8_t aChannel)
{
    mCslChannel = aChannel;
    UpdateCsl();
}

void Mac::SetCslPeriod(uint16_t aPeriod)
{
    mCslPeriod = aPeriod;
    UpdateCsl();
}

uint32_t Mac::GetCslPeriodInMsec(void) const
{
    return DivideAndRoundToClosest<uint32_t>(CslPeriodToUsec(GetCslPeriod()), 1000u);
}

uint32_t Mac::CslPeriodToUsec(uint16_t aPeriodInTenSymbols)
{
    return static_cast<uint32_t>(aPeriodInTenSymbols) * kUsPerTenSymbols;
}

bool Mac::IsCslEnabled(void) const { return !Get<Mle::Mle>().IsRxOnWhenIdle() && IsCslCapable(); }

bool Mac::IsCslCapable(void) const { return (GetCslPeriod() > 0) && IsCslSupported(); }

bool Mac::IsCslSupported(void) const
{
    return Get<Mle::MleRouter>().IsChild() && Get<Mle::Mle>().GetParent().IsEnhancedKeepAliveSupported();
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
void Mac::ProcessCsl(const RxFrame &aFrame, const Address &aSrcAddr)
{
    const uint8_t *cur;
    Child         *child;
    const CslIe   *csl;

    VerifyOrExit(aFrame.IsVersion2015() && aFrame.GetSecurityEnabled());

    cur = aFrame.GetHeaderIe(CslIe::kHeaderIeId);
    VerifyOrExit(cur != nullptr);

    child = Get<ChildTable>().FindChild(aSrcAddr, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != nullptr);

    csl = reinterpret_cast<const CslIe *>(cur + sizeof(HeaderIe));
    VerifyOrExit(csl->GetPeriod() >= kMinCslIePeriod);

    child->SetCslPeriod(csl->GetPeriod());
    child->SetCslPhase(csl->GetPhase());
    child->SetCslSynchronized(true);
    child->SetCslLastHeard(TimerMilli::GetNow());
    child->SetLastRxTimestamp(aFrame.GetTimestamp());
    LogDebg("Timestamp=%lu Sequence=%u CslPeriod=%u CslPhase=%u TransmitPhase=%u",
            ToUlong(static_cast<uint32_t>(aFrame.GetTimestamp())), aFrame.GetSequence(), csl->GetPeriod(),
            csl->GetPhase(), child->GetCslPhase());

    Get<CslTxScheduler>().Update();

exit:
    return;
}
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mac::ProcessEnhAckProbing(const RxFrame &aFrame, const Neighbor &aNeighbor)
{
    constexpr uint8_t kEnhAckProbingIeMaxLen = 2;

    const HeaderIe *enhAckProbingIe =
        reinterpret_cast<const HeaderIe *>(aFrame.GetThreadIe(ThreadIe::kEnhAckProbingIe));
    const uint8_t *data =
        reinterpret_cast<const uint8_t *>(enhAckProbingIe) + sizeof(HeaderIe) + sizeof(VendorIeHeader);
    uint8_t dataLen = 0;

    VerifyOrExit(enhAckProbingIe != nullptr);

    dataLen = enhAckProbingIe->GetLength() - sizeof(VendorIeHeader);
    VerifyOrExit(dataLen <= kEnhAckProbingIeMaxLen);

    Get<LinkMetrics::Initiator>().ProcessEnhAckIeData(data, dataLen, aNeighbor);
exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
void Mac::SetRadioFilterEnabled(bool aFilterEnabled)
{
    mLinks.GetSubMac().SetRadioFilterEnabled(aFilterEnabled);
    UpdateIdleMode();
}
#endif

} // namespace Mac
} // namespace ot
