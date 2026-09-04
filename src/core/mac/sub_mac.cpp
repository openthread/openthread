/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file implements the subset of IEEE 802.15.4 MAC primitives.
 */

#include "sub_mac.hpp"

#include <stdio.h>

#include <openthread/platform/time.h>

#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "utils/static_counter.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("SubMac");

SubMac::SubMac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRadioCaps(Get<Radio::Radio>().GetCaps())
    , mTransmitFrame(Get<Radio::Radio>().GetTransmitBuffer())
    , mCallbacks(aInstance)
    , mTimer(aInstance)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mCslReceiver(aInstance)
#endif
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    , mWedTimer(aInstance)
#endif
{
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && !OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE
    // Assuming the platform must deal with the retransmission security correctly.
    OT_ASSERT(RadioSupports(kCapTransmitRetries));
#endif
    Init();
}

void SubMac::Init(void)
{
    mState                 = kStateDisabled;
    mCsmaBackoffs          = 0;
    mTransmitRetries       = 0;
    mShortAddress          = kShortAddrInvalid;
    mAlternateShortAddress = kShortAddrInvalid;
    mExtAddress.Clear();
    mRxOnWhenIdle      = true;
    mEnergyScanMaxRssi = Radio::kInvalidRssi;
    mEnergyScanEndTime = Time{0};
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    mRetxDelayBackoffExponent = kRetxDelayMinBackoffExponent;
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    mRadioFilterEnabled = false;
#endif

    mKeyTrio.Clear();
    mFrameCounter = 0;
    mTimer.Stop();

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    mActiveTimedRx.Clear();
    mPendingTimedRx.Clear();
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslReceiver.Init();
#endif
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    WedInit();
#endif
}

#if OPENTHREAD_FTD || OPENTHREAD_MTD

SubMac::Capabilities SubMac::GetCaps(void) const
{
    Capabilities caps = mRadioCaps;

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    if (Get<LinkRaw>().IsEnabled())
    {
        caps |= kSwEnabledCapabilities;
    }
    else
#endif
    {
        caps |= (kCapAckTimeout | kCapCsmaBackoff | kCapTransmitRetries | kCapEnergyScan | kCapTransmitSec);
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
        caps |= kCapTransmitTiming;
#endif
    }

    return caps;
}

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

void SubMac::SetPanId(PanId aPanId)
{
    Get<Radio::Radio>().SetPanId(aPanId);
    LogDebg("RadioPanId: 0x%04x", aPanId);
}

void SubMac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    Get<Radio::Radio>().SetShortAddress(mShortAddress);
    LogDebg("RadioShortAddress: 0x%04x", mShortAddress);
}

void SubMac::SetAlternateShortAddress(ShortAddress aShortAddress)
{
    VerifyOrExit(mAlternateShortAddress != aShortAddress);

    mAlternateShortAddress = aShortAddress;
    Get<Radio::Radio>().SetAlternateShortAddress(mAlternateShortAddress);
    LogDebg("RadioAlternateShortAddress: 0x%04x", mAlternateShortAddress);

exit:
    return;
}

void SubMac::SetExtAddress(const ExtAddress &aExtAddress)
{
    mExtAddress = aExtAddress;
    Get<Radio::Radio>().SetExtendedAddress(aExtAddress);

    LogDebg("RadioExtAddress: %s", mExtAddress.ToString().AsCString());
}

void SubMac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mRxOnWhenIdle = aRxOnWhenIdle;

    LogDebg("RxOnWhenIdle: %u", mRxOnWhenIdle);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE && OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    // Keep radio rx-on-when-idle enabled for debugging when `MAC_CSL_DEBUG_ENABLE`.
    ExitNow();
#endif

    VerifyOrExit(RadioSupports(kCapRxOnWhenIdle));
    Get<Radio::Radio>().SetRxOnWhenIdle(mRxOnWhenIdle);

exit:
    return;
}

Error SubMac::Enable(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateDisabled);

    SuccessOrExit(error = Get<Radio::Radio>().Enable());
    SuccessOrExit(error = Get<Radio::Radio>().Sleep());

    SetState(kStateSleep);

exit:
    SuccessOrAssert(error);
    return error;
}

Error SubMac::Disable(void)
{
    Error error;

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    mActiveTimedRx.Clear();
    mPendingTimedRx.Clear();
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslReceiver.Stop();
#endif
#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    mWedTimer.Stop();
#endif

    mTimer.Stop();
    SuccessOrExit(error = Get<Radio::Radio>().Sleep());
    SuccessOrExit(error = Get<Radio::Radio>().Disable());
    SetState(kStateDisabled);

exit:
    return error;
}

Error SubMac::Sleep(void)
{
    Error error = kErrorNone;

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    // `ProcessTimedRx()` evaluates active and pending timed RX windows.
    //
    // If the current time is within a timed reception window, it transitions
    // the state to `kStateTimedReceive`.
    //
    // `ProcessTimedRx()` does not put the radio to sleep unless we were
    // already in `kStateTimedReceive` and the RX window has ended.
    //
    // Therefore, after calling `ProcessTimedRx()`, we verify that `mState` is
    // not `kStateTimedReceive` before proceeding to transition the radio to sleep.

    ProcessTimedRx();

    VerifyOrExit(mState != kStateTimedReceive);
#endif

    // If the radio platform layer supports `kCapRxOnWhenIdle`, it is
    // responsible for putting the radio to sleep, so we skip calling
    // `Radio::Sleep()`.
    //
    // However, if `SubMac::Sleep()` is explicitly called while `mRxOnWhenIdle`
    // is true, we still call `Radio::Sleep()` to support test scenarios where
    // the radio is forced to sleep.

    SetState(kStateSleep);

    if (!RadioSupports(kCapRxOnWhenIdle) || mRxOnWhenIdle)
    {
        SuccessOrExit(error = Get<Radio::Radio>().Sleep());
    }

exit:
    LogWarnOnError(error, "Sleep()");
    return error;
}

Error SubMac::Receive(uint8_t aChannel)
{
    Error error;

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        error = Get<Radio::Radio>().Sleep();
    }
    else
#endif
    {
        error = Get<Radio::Radio>().Receive(aChannel);
    }

    SuccessOrExit(error);

    SetState(kStateReceive);

exit:
    LogWarnOnError(error, "RadioReceive()");
    return error;
}

void SubMac::HandleReceiveDone(RxFrame *aFrame, Error aError)
{
    if (mPcapCallback.IsSet() && (aFrame != nullptr) && (aError == kErrorNone))
    {
        mPcapCallback.Invoke(aFrame, false);
    }

    if (!ShouldHandle(kCapTransmitSec) && aFrame != nullptr && aFrame->IsAckedWithSecEnhAck())
    {
        SignalFrameCounterUsed(aFrame->GetAckFrameCounter(), aFrame->GetAckKeyIndex());
    }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslReceiver.UpdateLastSyncTimestamp(aFrame, aError);
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (!mRadioFilterEnabled)
#endif
    {
        mCallbacks.ReceiveDone(aFrame, aError);
    }
}

Error SubMac::Send(void)
{
    Error              error = kErrorNone;
    TxFrame::ParseInfo frameInfo;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    case kStateTimedTransmit:
#endif
    case kStateTransmit:
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    case kStateDelayBeforeRetx:
#endif
    case kStateSleep:
    case kStateReceive:
#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    case kStateTimedReceive:
#endif
        break;

    case kStateEnergyScan:
        ExitNow(error = kErrorInvalidState);
    }

    mTimer.Stop();

    // We ignore the parsing error here because `Send()` must allow
    // transmission of raw frames (when `LinkRaw` is enabled) which may
    // not follow the standard IEEE 802.15.4 frame format.
    // `ProcessTransmitSecurity()` validates `mParsedFully` before
    // performing any security operations on the frame.

    IgnoreError(frameInfo.ParseFrom(mTransmitFrame, Frame::kParseFully));

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        mCallbacks.TransmitDone(frameInfo, nullptr, frameInfo.mIsAckRequest ? kErrorNoAck : kErrorNone);
        ExitNow();
    }
#endif

    ProcessTransmitSecurity(frameInfo);

    mCsmaBackoffs    = 0;
    mTransmitRetries = 0;

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    mRetxDelayBackoffExponent = kRetxDelayMinBackoffExponent;
#endif

    StartCsmaBackoff();

exit:
    return error;
}

void SubMac::ProcessTransmitSecurity(TxFrame::ParseInfo &aFrameInfo)
{
    VerifyOrExit(aFrameInfo.mParsedFully);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);

    VerifyOrExit(!aFrameInfo.GetTxFrame()->IsSecurityProcessed());

    if (!aFrameInfo.GetTxFrame()->IsHeaderUpdated())
    {
        aFrameInfo.WriteKeyIndex(mKeyTrio.GetKeyIndex());
    }

    VerifyOrExit(ShouldHandle(kCapTransmitSec));

    VerifyOrExit(aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1);

    aFrameInfo.GetTxFrame()->SetAesKey(mKeyTrio.SelectKey(aFrameInfo.mKeyIndex));

    if (!aFrameInfo.GetTxFrame()->IsHeaderUpdated())
    {
        uint32_t frameCounter = GetFrameCounter();

        aFrameInfo.WriteFrameCounter(frameCounter);
        SignalFrameCounterUsed(frameCounter, aFrameInfo.mKeyIndex);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(!aFrameInfo.Has<TimeIe>());
#endif

    aFrameInfo.ProcessTransmitAesCcm(GetExtAddress());

exit:
    return;
}

void SubMac::StartCsmaBackoff(void)
{
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    if (mTransmitFrame.IsTargetTxTimeSpecified())
    {
        SetState(kStateTimedTransmit);

        if (ShouldHandle(kCapTransmitTiming))
        {
            Radio::Time32 txStart  = mTransmitFrame.GetTargetTxTime() - kTimedTxLeadTime;
            Radio::Time32 radioNow = Get<Radio::Radio>().GetNowAsTime32();

            if (Radio::IsTimeStrictlyBefore(radioNow, txStart))
            {
                StartTimer(txStart - radioNow);
                ExitNow();
            }

            // Transmit without delay
        }

        BeginTransmit();
        ExitNow();
    }
#endif // OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE

    SetState(kStateCsmaBackoff);

    if (mTransmitFrame.GetMaxCsmaBackoffs() > 0 && ShouldHandleCsmaBackoff())
    {
        uint8_t backoffExponent = kCsmaMinBe + mCsmaBackoffs;

        backoffExponent = Min(backoffExponent, kCsmaMaxBe);
        StartTimerForBackoff(backoffExponent);
        ExitNow();
    }

    BeginTransmit();

exit:
    return;
}

void SubMac::StartTimerForBackoff(uint8_t aBackoffExponent)
{
    uint32_t backoff;

    backoff = Random::NonCrypto::GenerateUpToExcluding(static_cast<uint32_t>(1UL << aBackoffExponent));
    backoff *= (kUnitBackoffPeriod * Radio::kSymbolTime);

    if (mRxOnWhenIdle)
    {
        IgnoreError(Get<Radio::Radio>().Receive(mTransmitFrame.GetChannel()));
    }
    else
    {
        IgnoreError(Get<Radio::Radio>().Sleep());
    }

    StartTimer(backoff);

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    if (mState == kStateDelayBeforeRetx)
    {
        LogDebg("Delaying retx for %lu usec (be=%u)", ToUlong(backoff), aBackoffExponent);
    }
#endif
}

void SubMac::BeginTransmit(void)
{
    Error error;

#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    VerifyOrExit(mState == kStateCsmaBackoff || mState == kStateTimedTransmit);
#else
    VerifyOrExit(mState == kStateCsmaBackoff);
#endif

    if (!RadioSupports(kCapSleepToTx))
    {
        SuccessOrAssert(Get<Radio::Radio>().Receive(mTransmitFrame.GetChannel()));
    }

    SetState(kStateTransmit);

    error = Get<Radio::Radio>().Transmit(mTransmitFrame);

#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    if (error == kErrorInvalidState && mTransmitFrame.IsTargetTxTimeSpecified())
    {
        // Platform `transmit_at` fails and we send the frame directly.
        mTransmitFrame.ClearTargetTxTime();
        error = Get<Radio::Radio>().Transmit(mTransmitFrame);
    }
#endif

    SuccessOrAssert(error);

exit:
    return;
}

void SubMac::HandleTransmitStarted(TxFrame &aFrame)
{
    TxFrame::ParseInfo frameInfo;

    if (mPcapCallback.IsSet())
    {
        mPcapCallback.Invoke(&aFrame, true);
    }

    VerifyOrExit(ShouldHandle(kCapAckTimeout));

    SuccessOrExit(frameInfo.ParseFrom(aFrame, Frame::kParseAddrFields));

    if (frameInfo.mIsAckRequest)
    {
        StartTimer(kAckTimeout);
    }

exit:
    return;
}

void SubMac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    bool               ccaSuccess = true;
    bool               shouldRetx;
    TxFrame::ParseInfo frameInfo;

    // We ignore the parsing error here because `HandleTransmitDone()`
    // must proceed with transmit-done handling (stopping timers,
    // recording CCA status, handling retries) even if the frame is not
    // a valid IEEE 802.15.4 frame (e.g., when `LinkRaw` is enabled with
    // a vendor-specific format). Sub-handlers methods like
    // `SignalFrameCounterUsedOnTxDone()` validate `mParsedFully`
    // in `frameInfo` individually.

    IgnoreError(frameInfo.ParseFrom(aFrame, Frame::kParseFully));

    // Stop ack timeout timer.

    mTimer.Stop();

    // Record CCA success or failure status.

    switch (aError)
    {
    case kErrorAbort:
        // Do not record CCA status in case of `ABORT` error
        // since there may be no CCA check performed by radio.
        break;

    case kErrorChannelAccessFailure:
        ccaSuccess = false;

        OT_FALL_THROUGH;

    case kErrorNone:
    case kErrorNoAck:
        if (aFrame.IsCsmaCaEnabled())
        {
            mCallbacks.RecordCcaStatus(ccaSuccess, aFrame.GetChannel());
        }
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        mCslReceiver.UpdateLastSyncTimestamp(frameInfo, aAckFrame);
#endif
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

    SignalFrameCounterUsedOnTxDone(frameInfo);

    // Determine whether a CSMA retry is required.

    if (!ccaSuccess && ShouldHandleCsmaBackoff() && mCsmaBackoffs < aFrame.GetMaxCsmaBackoffs())
    {
        mCsmaBackoffs++;
        StartCsmaBackoff();
        ExitNow();
    }

    mCsmaBackoffs = 0;

    // Determine whether to re-transmit the frame.

    shouldRetx = ((aError != kErrorNone) && ShouldHandle(kCapTransmitRetries) &&
                  (mTransmitRetries < aFrame.GetMaxFrameRetries()));

    mCallbacks.RecordFrameTransmitStatus(frameInfo, aError, mTransmitRetries, shouldRetx);

    if (shouldRetx)
    {
        mTransmitRetries++;
        aFrame.SetIsARetransmission(true);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE
        ReprocessSecurityForRetx(frameInfo);
#endif

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
        if (aError == kErrorNoAck)
        {
            SetState(kStateDelayBeforeRetx);
            StartTimerForBackoff(mRetxDelayBackoffExponent);
            mRetxDelayBackoffExponent =
                Min(static_cast<uint8_t>(mRetxDelayBackoffExponent + 1), kRetxDelayMaxBackoffExponent);
            ExitNow();
        }
#endif

        StartCsmaBackoff();
        ExitNow();
    }

    SetState(kStateReceive);

#if OPENTHREAD_RADIO
    if (aFrame.GetChannel() != aFrame.GetRxChannelAfterTxDone() && mRxOnWhenIdle)
    {
        // On RCP build, we switch immediately to the specified RX
        // channel if it is different from the channel on which frame
        // was sent. On FTD or MTD builds we don't need to do
        // the same as the `Mac` will switch the channel from the
        // `mCallbacks.TransmitDone()`.

        IgnoreError(Get<Radio::Radio>().Receive(aFrame.GetRxChannelAfterTxDone()));
    }
#endif

    mCallbacks.TransmitDone(frameInfo, aAckFrame, aError);

exit:
    return;
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE

void SubMac::ReprocessSecurityForRetx(TxFrame::ParseInfo &aFrameInfo)
{
    // Re-processes transmit security on a frame being retransmitted if
    // it contains Header IEs. The frame is first restored back to
    // plaintext and then re-encrypted with a new frame counter value.

    VerifyOrExit(aFrameInfo.mParsedFully);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aFrameInfo.mIsIePresent);

    // When transmit security is handled by `SubMac`, the AES key is already set
    // on `aFrameInfo.GetTxFrame()`. However, when transmit security is delegated
    // to the radio platform, the radio is not required to set or preserve the AES
    // key on the frame. To ensure `RestoreTransmitSecurity()` can properly decrypt
    // the frame back to plaintext, we determine and set the key on the frame
    // using its key index.

    if (!ShouldHandle(kCapTransmitSec) && (aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1))
    {
        aFrameInfo.GetTxFrame()->SetAesKey(mKeyTrio.SelectKey(aFrameInfo.mKeyIndex));
    }

    aFrameInfo.RestoreTransmitSecurity(GetExtAddress());

    ProcessTransmitSecurity(aFrameInfo);

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE

void SubMac::SignalFrameCounterUsedOnTxDone(const TxFrame::ParseInfo &aFrameInfo)
{
    VerifyOrExit(!ShouldHandle(kCapTransmitSec));

    // In an FTD/MTD build, if/when link-raw is enabled, the `TxFrame`
    // is prepared and given by user and may not necessarily follow 15.4
    // frame format (link raw can be used with vendor-specific format),
    // so we allow failure when parsing the frame (i.e., do not assert
    // on an error). In other cases (in an RCP build or in an FTD/MTD
    // build without link-raw) since the `TxFrame` should be prepared by
    // OpenThread core, we expect no error and therefore assert if
    // parsing fails.

    if (!aFrameInfo.mParsedFully)
    {
#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
        VerifyOrExit(!Get<LinkRaw>().IsEnabled());
#endif
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aFrameInfo.GetTxFrame()->IsHeaderUpdated());

    VerifyOrExit(aFrameInfo.mKeyIdMode == Frame::kKeyIdMode1);

    SignalFrameCounterUsed(aFrameInfo.mFrameCounter, aFrameInfo.mKeyIndex);

exit:
    return;
}

int8_t SubMac::GetRssi(void) const
{
    int8_t rssi;

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        rssi = Radio::kInvalidRssi;
    }
    else
#endif
    {
        rssi = Get<Radio::Radio>().GetRssi();
    }

    return rssi;
}

int8_t SubMac::GetNoiseFloor(void) const { return Get<Radio::Radio>().GetReceiveSensitivity(); }

Error SubMac::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    Error error = kErrorNone;

    switch (mState)
    {
    case kStateSleep:
    case kStateReceive:
#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    case kStateTimedReceive:
#endif
        break;

    case kStateDisabled:
    case kStateCsmaBackoff:
    case kStateTransmit:
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    case kStateTimedTransmit:
#endif
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    case kStateDelayBeforeRetx:
#endif
    case kStateEnergyScan:
        ExitNow(error = kErrorInvalidState);
    }

    mTimer.Stop();

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        HandleEnergyScanDone(Radio::kInvalidRssi);
        ExitNow();
    }
#endif

    if (RadioSupports(kCapEnergyScan))
    {
        IgnoreError(Get<Radio::Radio>().EnergyScan(aScanChannel, aScanDuration));
        SetState(kStateEnergyScan);
    }
    else if (ShouldHandle(kCapEnergyScan))
    {
        SuccessOrAssert(Get<Radio::Radio>().Receive(aScanChannel));

        SetState(kStateEnergyScan);
        mEnergyScanMaxRssi = Radio::kInvalidRssi;
        mEnergyScanEndTime = TimerMilli::GetNow() + static_cast<uint32_t>(aScanDuration);
        StartTimer(0);
    }
    else
    {
        error = kErrorNotImplemented;
    }

exit:
    return error;
}

void SubMac::SampleRssi(void)
{
    OT_ASSERT(!RadioSupports(kCapEnergyScan));

    int8_t rssi = GetRssi();

    if (rssi != Radio::kInvalidRssi)
    {
        if ((mEnergyScanMaxRssi == Radio::kInvalidRssi) || (rssi > mEnergyScanMaxRssi))
        {
            mEnergyScanMaxRssi = rssi;
        }
    }

    if (TimerMilli::GetNow() < mEnergyScanEndTime)
    {
        StartTimerAt(mTimer.GetFireTime(), kEnergyScanRssiSampleInterval);
    }
    else
    {
        HandleEnergyScanDone(mEnergyScanMaxRssi);
    }
}

void SubMac::HandleEnergyScanDone(int8_t aMaxRssi)
{
    SetState(kStateReceive);
    mCallbacks.EnergyScanDone(aMaxRssi);
}

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE

void SubMac::ReceiveAt(Radio::Time64 aStartTime, uint32_t aDuration, uint8_t aChannel)
{
    Radio::SyncedTime now;
    TimedRx           timedRx;

    VerifyOrExit(mState != kStateDisabled);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        ExitNow();
    }
#endif

    timedRx.Init(aStartTime, aDuration, aChannel);

    now.SetToNow(Get<Radio::Radio>());

    VerifyOrExit(!timedRx.HasEnded(now));

    if (!ShouldHandle(kCapReceiveTiming))
    {
        timedRx.ScheduleOnRadio(Get<Radio::Radio>());
        ExitNow();
    }

    if (mPendingTimedRx.IsSpecified() && mPendingTimedRx.HasStarted(now) && !mPendingTimedRx.HasEnded(now))
    {
        // Before replacing `mPendingTimedRx` check if the existing one
        // should be started, and start if we can (we are in right states).
        // Otherwise copy it as `mActiveTimedRx` (resume/start it once
        // state changes and timed-rx is allowed).

        switch (mState)
        {
        case kStateSleep:
        case kStateTimedReceive:
            StartPendingTimedRx();
            break;
        default:
            mActiveTimedRx = mPendingTimedRx;
            break;
        }
    }

    mPendingTimedRx = timedRx;

    switch (mState)
    {
    case kStateSleep:
    case kStateTimedReceive:
        ProcessTimedRx();
        break;
    default:
        break;
    }

exit:
    return;
}

void SubMac::CancelPendingReceiveAt(void)
{
    VerifyOrExit(mPendingTimedRx.IsSpecified());
    mPendingTimedRx.Clear();

    switch (mState)
    {
    case kStateSleep:
    case kStateTimedReceive:
        ProcessTimedRx();
        break;
    default:
        break;
    }

exit:
    return;
}

void SubMac::StartPendingTimedRx(void)
{
    if ((mState == kStateTimedReceive) && (mActiveTimedRx.GetChannel() == mPendingTimedRx.GetChannel()))
    {
        // Skip transitioning the radio if already receiving on the same
        // channel
    }
    else
    {
        IgnoreError(Get<Radio::Radio>().Receive(mPendingTimedRx.GetChannel()));
    }

    mActiveTimedRx = mPendingTimedRx;
    SetState(kStateTimedReceive);
}

void SubMac::ProcessTimedRx(void)
{
    // Processes the timed RX state machine, starting any due pending
    // timed RX, scheduling the timer for upcoming windows, or putting
    // the radio to sleep when active reception window ends.

    Radio::Time64     fireTime = Radio::kMaxTime64;
    Radio::SyncedTime now;

    mTimer.Stop();

    now.SetToNow(Get<Radio::Radio>());

    // Start pending `TimedRx` if due, or clear it if missed.

    if (mPendingTimedRx.IsSpecified())
    {
        if (mPendingTimedRx.HasStarted(now))
        {
            if (!mPendingTimedRx.HasEnded(now))
            {
                StartPendingTimedRx();
            }

            mPendingTimedRx.Clear();
        }
        else
        {
            fireTime = mPendingTimedRx.GetStartTime();
        }
    }

    VerifyOrExit(mActiveTimedRx.IsSpecified());

    if (!mActiveTimedRx.HasEnded(now))
    {
        if (mState != kStateTimedReceive)
        {
            IgnoreError(Get<Radio::Radio>().Receive(mActiveTimedRx.GetChannel()));
            SetState(kStateTimedReceive);
        }

        fireTime = Min(fireTime, mActiveTimedRx.GetEndTime());
        ExitNow();
    }

    // Active `TimedRx` has ended. clear it and transition the radio
    // to sleep if it was actively receiving.

    mActiveTimedRx.Clear();

    if (mState == kStateTimedReceive)
    {
        SetState(kStateSleep);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE && OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
        // Don't actually sleep for debugging when `MAC_CSL_DEBUG_ENABLE`.
        ExitNow();
#endif
        IgnoreError(Get<Radio::Radio>().Sleep());
    }

exit:

    if (fireTime != Radio::kMaxTime64)
    {
        // Schedule the timer to fire at a target radio time `fireTime`,
        // using the synced reference `now` to translate radio time to
        // local time.

        uint32_t delay = 0;

        if (fireTime > now.GetAsTime64())
        {
            delay = ClampToUint32(fireTime - now.GetAsTime64());
        }

        StartTimerAt(now.GetAsLocalTimeMicro(), delay);
    }
}

void SubMac::TimedRx::Init(Radio::Time64 aStartTime, uint32_t aDuration, uint8_t aChannel)
{
    mStartTime   = aStartTime;
    mDuration    = aDuration;
    mChannel     = aChannel;
    mIsSpecified = true;
}

void SubMac::TimedRx::ScheduleOnRadio(Radio::Radio &aRadio) const
{
    Error error = aRadio.ReceiveAt(mChannel, Radio::ConvertTime64To32(mStartTime), mDuration);

    LogWarnOnError(error, "Radio::ReceiveAt()");
}

#endif // OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE

void SubMac::HandleTimer(void)
{
    switch (mState)
    {
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    case kStateTimedTransmit:
#endif
    case kStateCsmaBackoff:
        BeginTransmit();
        break;

    case kStateTransmit:
        LogDebg("Ack timer timed out");
        IgnoreError(Get<Radio::Radio>().Receive(mTransmitFrame.GetChannel()));
        HandleTransmitDone(mTransmitFrame, nullptr, kErrorNoAck);
        break;

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    case kStateDelayBeforeRetx:
        StartCsmaBackoff();
        break;
#endif

    case kStateEnergyScan:
        SampleRssi();
        break;

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    case kStateTimedReceive:
    case kStateSleep:
        ProcessTimedRx();
        break;
#endif

    default:
        break;
    }
}

bool SubMac::ShouldHandle(Capability aCapability) const
{
    // Determines whether `SubMac` should handle a given radio
    // capability.
    //
    // If the radio platform supports it, we delegate it to the radio.
    // Otherwise, `SubMac` will handle it.
    //
    // Under `OPENTHREAD_RADIO` (radio-only build) or when `LinkRaw`
    // is enabled, there are a set of `OPENTHREAD_CONFIG_MAC_SOFTWARE_*`
    // configs which control whether `SubMac` should implement each
    // capability. This is tracked by `kSwEnabledCapabilities`.

    bool shouldHandle = false;

    if (RadioSupports(aCapability))
    {
        ExitNow();
    }

#if OPENTHREAD_RADIO
    shouldHandle = ((kSwEnabledCapabilities & aCapability) != 0);
    ExitNow();
#endif

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    if (Get<LinkRaw>().IsEnabled())
    {
        shouldHandle = ((kSwEnabledCapabilities & aCapability) != 0);
        ExitNow();
    }
#endif

    shouldHandle = true;

#endif

exit:
    return shouldHandle;
}

bool SubMac::ShouldHandleCsmaBackoff(void) const
{
    bool shouldHandle = false;

    VerifyOrExit(mTransmitFrame.IsCsmaCaEnabled());

    if (RadioSupports(kCapTransmitRetries))
    {
        ExitNow();
    }

    shouldHandle = ShouldHandle(kCapCsmaBackoff);

exit:
    return shouldHandle;
}

void SubMac::SetState(State aState)
{
    if (mState != aState)
    {
        LogDebg("RadioState: %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
}

void SubMac::SetMode1MacKeys(uint8_t aKeyIndex, const Key &aPrevKey, const Key &aCurKey, const Key &aNextKey)
{
    mKeyTrio.Set(aKeyIndex, aPrevKey, aCurKey, aNextKey);

    VerifyOrExit(!ShouldHandle(kCapTransmitSec));

    Get<Radio::Radio>().SetMode1MacKeys(mKeyTrio);

exit:
    return;
}

void SubMac::SignalFrameCounterUsed(uint32_t aFrameCounter, uint8_t aKeyIndex)
{
    VerifyOrExit(aKeyIndex == mKeyTrio.GetKeyIndex());

    mCallbacks.FrameCounterUsed(aFrameCounter);

    // It not always guaranteed that this method is invoked in order
    // for different counter values (i.e., we may get it for a
    // smaller counter value after a lager one). This may happen due
    // to a new counter value being used for an enhanced-ack during
    // tx of a frame. Note that the newer counter used for enhanced-ack
    // is processed from `HandleReceiveDone()` which can happen before
    // processing of the older counter value from `HandleTransmitDone()`.

    VerifyOrExit(mFrameCounter <= aFrameCounter);
    mFrameCounter = aFrameCounter + 1;

exit:
    return;
}

void SubMac::SetFrameCounter(uint32_t aFrameCounter, bool aSetIfLarger)
{
    if (!aSetIfLarger || (aFrameCounter > mFrameCounter))
    {
        mFrameCounter = aFrameCounter;
    }

    VerifyOrExit(!ShouldHandle(kCapTransmitSec));

    if (aSetIfLarger)
    {
        Get<Radio::Radio>().SetMacFrameCounterIfLarger(aFrameCounter);
    }
    else
    {
        Get<Radio::Radio>().SetMacFrameCounter(aFrameCounter);
    }

exit:
    return;
}

void SubMac::StartTimer(uint32_t aDelayUs)
{
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    mTimer.Start(aDelayUs);
#else
    mTimer.Start(aDelayUs / Time::kOneMsecInUsec);
#endif
}

void SubMac::StartTimerAt(Time aStartTime, uint32_t aDelayUs)
{
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    mTimer.StartAt(aStartTime, aDelayUs);
#else
    mTimer.StartAt(aStartTime, aDelayUs / Time::kOneMsecInUsec);
#endif
}

// LCOV_EXCL_START

const char *SubMac::StateToString(State aState)
{
#define StateMapList(_)                 \
    _(kStateDisabled, "Disabled")       \
    _(kStateSleep, "Sleep")             \
    _(kStateReceive, "Receive")         \
    _(kStateCsmaBackoff, "CsmaBackoff") \
    _(kStateTransmit, "Transmit")       \
    _(kStateEnergyScan, "EnergyScan")   \
    DelayBeforeRetxStateMapList(_) TimedTxStateMapList(_) TimedRxStateMapList(_)

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
#define DelayBeforeRetxStateMapList(_) _(kStateDelayBeforeRetx, "DelayBeforeRetx")
#else
#define DelayBeforeRetxStateMapList(_)
#endif

#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
#define TimedTxStateMapList(_) _(kStateTimedTransmit, "TimedTransmit")
#else
#define TimedTxStateMapList(_)
#endif

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
#define TimedRxStateMapList(_) _(kStateTimedReceive, "TimedReceive")
#else
#define TimedRxStateMapList(_)
#endif

    DefineEnumStringArray(StateMapList);

    return kStrings[aState];
}

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
