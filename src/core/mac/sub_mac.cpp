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
    , mRadioCaps(Get<Radio>().GetCaps())
    , mTransmitFrame(Get<Radio>().GetTransmitBuffer())
    , mCallbacks(aInstance)
    , mTimer(aInstance)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mCslTimer(aInstance, SubMac::HandleCslTimer)
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    , mWedTimer(aInstance, SubMac::HandleWedTimer)
#endif
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslParentAccuracy.Init();
#endif

    Init();
}

void SubMac::Init(void)
{
    mState           = kStateDisabled;
    mCsmaBackoffs    = 0;
    mTransmitRetries = 0;
    mShortAddress    = kShortAddrInvalid;
    mExtAddress.Clear();
    mRxOnWhenIdle      = true;
    mEnergyScanMaxRssi = Radio::kInvalidRssi;
    mEnergyScanEndTime = Time{0};
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    mRetxDelayBackOffExponent = kRetxDelayMinBackoffExponent;
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    mRadioFilterEnabled = false;
#endif

    mPrevKey.Clear();
    mCurrKey.Clear();
    mNextKey.Clear();

    mFrameCounter = 0;
    mKeyId        = 0;
    mTimer.Stop();

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    CslInit();
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    WedInit();
#endif
}

otRadioCaps SubMac::GetCaps(void) const
{
    otRadioCaps caps;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    caps = mRadioCaps;

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
    caps |= OT_RADIO_CAPS_ACK_TIMEOUT;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
    caps |= OT_RADIO_CAPS_CSMA_BACKOFF;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
    caps |= OT_RADIO_CAPS_TRANSMIT_RETRIES;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE
    caps |= OT_RADIO_CAPS_ENERGY_SCAN;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    caps |= OT_RADIO_CAPS_TRANSMIT_SEC;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    caps |= OT_RADIO_CAPS_TRANSMIT_TIMING;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    caps |= OT_RADIO_CAPS_RECEIVE_TIMING;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_ON_WHEN_IDLE_ENABLE
    caps |= OT_RADIO_CAPS_RX_ON_WHEN_IDLE;
#endif

#if OPENTHREAD_RADIO
    caps |= OT_RADIO_CAPS_SLEEP_TO_TX;
#endif

#else
    caps = OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES |
           OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING |
           OT_RADIO_CAPS_RECEIVE_TIMING | OT_RADIO_CAPS_RX_ON_WHEN_IDLE;
#endif

    return caps;
}

void SubMac::SetPanId(PanId aPanId)
{
    Get<Radio>().SetPanId(aPanId);
    LogDebg("RadioPanId: 0x%04x", aPanId);
}

void SubMac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    Get<Radio>().SetShortAddress(mShortAddress);
    LogDebg("RadioShortAddress: 0x%04x", mShortAddress);
}

void SubMac::SetExtAddress(const ExtAddress &aExtAddress)
{
    ExtAddress address;

    mExtAddress = aExtAddress;

    // Reverse the byte order before setting on radio.
    address.Set(aExtAddress.m8, ExtAddress::kReverseByteOrder);
    Get<Radio>().SetExtendedAddress(address);

    LogDebg("RadioExtAddress: %s", mExtAddress.ToString().AsCString());
}

void SubMac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mRxOnWhenIdle = aRxOnWhenIdle;

    if (RadioSupportsRxOnWhenIdle())
    {
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
        Get<Radio>().SetRxOnWhenIdle(mRxOnWhenIdle);
#endif
    }

    LogDebg("RxOnWhenIdle: %d", mRxOnWhenIdle);
}

Error SubMac::Enable(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateDisabled);

    SuccessOrExit(error = Get<Radio>().Enable());
    SuccessOrExit(error = Get<Radio>().Sleep());

    SetState(kStateSleep);

exit:
    SuccessOrAssert(error);
    return error;
}

Error SubMac::Disable(void)
{
    Error error;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslTimer.Stop();
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    mWedTimer.Stop();
#endif

    mTimer.Stop();
    SuccessOrExit(error = Get<Radio>().Sleep());
    SuccessOrExit(error = Get<Radio>().Disable());
    SetState(kStateDisabled);

exit:
    return error;
}

Error SubMac::Sleep(void)
{
    Error error = kErrorNone;

    VerifyOrExit(ShouldHandleTransitionToSleep());

    error = Get<Radio>().Sleep();

exit:
    if (error != kErrorNone)
    {
        LogWarn("RadioSleep() failed, error: %s", ErrorToString(error));
    }
    else
    {
        SetState(kStateSleep);
    }

    return error;
}

Error SubMac::Receive(uint8_t aChannel)
{
    Error error;

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        error = Get<Radio>().Sleep();
    }
    else
#endif
    {
        error = Get<Radio>().Receive(aChannel);
    }

    if (error != kErrorNone)
    {
        LogWarn("RadioReceive() failed, error: %s", ErrorToString(error));
        ExitNow();
    }

    SetState(kStateReceive);

exit:
    return error;
}

void SubMac::HandleReceiveDone(RxFrame *aFrame, Error aError)
{
    if (mPcapCallback.IsSet() && (aFrame != nullptr) && (aError == kErrorNone))
    {
        mPcapCallback.Invoke(aFrame, false);
    }

    if (!ShouldHandleTransmitSecurity() && aFrame != nullptr && aFrame->mInfo.mRxInfo.mAckedWithSecEnhAck)
    {
        SignalFrameCounterUsed(aFrame->mInfo.mRxInfo.mAckFrameCounter, aFrame->mInfo.mRxInfo.mAckKeyId);
    }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    UpdateCslLastSyncTimestamp(aFrame, aError);
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
    Error error = kErrorNone;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
#endif
    case kStateTransmit:
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    case kStateDelayBeforeRetx:
#endif
    case kStateSleep:
    case kStateReceive:
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    case kStateCslSample:
#endif
        break;

    case kStateEnergyScan:
        ExitNow(error = kErrorInvalidState);
    }

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    if (mRadioFilterEnabled)
    {
        mCallbacks.TransmitDone(mTransmitFrame, nullptr, mTransmitFrame.GetAckRequest() ? kErrorNoAck : kErrorNone);
        ExitNow();
    }
#endif

    ProcessTransmitSecurity();

    mCsmaBackoffs    = 0;
    mTransmitRetries = 0;

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    mRetxDelayBackOffExponent = kRetxDelayMinBackoffExponent;
#endif

    StartCsmaBackoff();

exit:
    return error;
}

void SubMac::ProcessTransmitSecurity(void)
{
    const ExtAddress *extAddress = nullptr;
    uint8_t           keyIdMode;

    VerifyOrExit(mTransmitFrame.GetSecurityEnabled());
    VerifyOrExit(!mTransmitFrame.IsSecurityProcessed());

    SuccessOrExit(mTransmitFrame.GetKeyIdMode(keyIdMode));

    if (!mTransmitFrame.IsHeaderUpdated())
    {
        mTransmitFrame.SetKeyId(mKeyId);
    }

    VerifyOrExit(ShouldHandleTransmitSecurity());
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1);

    mTransmitFrame.SetAesKey(GetCurrentMacKey());

    if (!mTransmitFrame.IsHeaderUpdated())
    {
        uint32_t frameCounter = GetFrameCounter();

        mTransmitFrame.SetFrameCounter(frameCounter);
        SignalFrameCounterUsed(frameCounter, mKeyId);
    }

    extAddress = &GetExtAddress();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(mTransmitFrame.GetTimeIeOffset() == 0);
#endif

    mTransmitFrame.ProcessTransmitAesCcm(*extAddress);

exit:
    return;
}

void SubMac::StartCsmaBackoff(void)
{
    uint8_t backoffExponent = kCsmaMinBe + mCsmaBackoffs;

#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (mTransmitFrame.mInfo.mTxInfo.mTxDelay != 0)
    {
        SetState(kStateCslTransmit);

        if (ShouldHandleTransmitTargetTime())
        {
            static constexpr uint32_t kAheadTime = kCcaSampleInterval + kCslTransmitTimeAhead + kRadioHeaderShrDuration;
            Time                      txStartTime = Time(mTransmitFrame.mInfo.mTxInfo.mTxDelayBaseTime);
            Time                      radioNow    = Time(static_cast<uint32_t>(Get<Radio>().GetNow()));

            txStartTime += (mTransmitFrame.mInfo.mTxInfo.mTxDelay - kAheadTime);

            if (radioNow < txStartTime)
            {
                StartTimer(txStartTime - radioNow);
            }
            else // Transmit without delay
            {
                BeginTransmit();
            }
        }
        else
        {
            BeginTransmit();
        }

        ExitNow();
    }
#endif // !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

    SetState(kStateCsmaBackoff);

    VerifyOrExit(ShouldHandleCsmaBackOff(), BeginTransmit());

    backoffExponent = Min(backoffExponent, kCsmaMaxBe);

    StartTimerForBackoff(backoffExponent);

exit:
    return;
}

void SubMac::StartTimerForBackoff(uint8_t aBackoffExponent)
{
    uint32_t backoff;

    backoff = Random::NonCrypto::GetUint32InRange(0, static_cast<uint32_t>(1UL << aBackoffExponent));
    backoff *= (kUnitBackoffPeriod * Radio::kSymbolTime);

    if (mRxOnWhenIdle)
    {
        IgnoreError(Get<Radio>().Receive(mTransmitFrame.GetChannel()));
    }
    else
    {
        IgnoreError(Get<Radio>().Sleep());
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

#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    VerifyOrExit(mState == kStateCsmaBackoff || mState == kStateCslTransmit);
#else
    VerifyOrExit(mState == kStateCsmaBackoff);
#endif

    if ((mRadioCaps & OT_RADIO_CAPS_SLEEP_TO_TX) == 0)
    {
        SuccessOrAssert(Get<Radio>().Receive(mTransmitFrame.GetChannel()));
    }

    SetState(kStateTransmit);

    error = Get<Radio>().Transmit(mTransmitFrame);

    if (error == kErrorInvalidState && mTransmitFrame.mInfo.mTxInfo.mTxDelay > 0)
    {
        // Platform `transmit_at` fails and we send the frame directly.
        mTransmitFrame.mInfo.mTxInfo.mTxDelay         = 0;
        mTransmitFrame.mInfo.mTxInfo.mTxDelayBaseTime = 0;

        error = Get<Radio>().Transmit(mTransmitFrame);
    }

    SuccessOrAssert(error);

exit:
    return;
}

void SubMac::HandleTransmitStarted(TxFrame &aFrame)
{
    if (mPcapCallback.IsSet())
    {
        mPcapCallback.Invoke(&aFrame, true);
    }

    if (ShouldHandleAckTimeout() && aFrame.GetAckRequest())
    {
        StartTimer(kAckTimeout);
    }
}

void SubMac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    bool ccaSuccess = true;
    bool shouldRetx;

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
        UpdateCslLastSyncTimestamp(aFrame, aAckFrame);
#endif
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

    SignalFrameCounterUsedOnTxDone(aFrame);

    // Determine whether a CSMA retry is required.

    if (!ccaSuccess && ShouldHandleCsmaBackOff() && mCsmaBackoffs < aFrame.GetMaxCsmaBackoffs())
    {
        mCsmaBackoffs++;
        StartCsmaBackoff();
        ExitNow();
    }

    mCsmaBackoffs = 0;

    // Determine whether to re-transmit the frame.

    shouldRetx = ((aError != kErrorNone) && ShouldHandleRetries() && (mTransmitRetries < aFrame.GetMaxFrameRetries()));

    mCallbacks.RecordFrameTransmitStatus(aFrame, aError, mTransmitRetries, shouldRetx);

    if (shouldRetx)
    {
        mTransmitRetries++;
        aFrame.SetIsARetransmission(true);

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
        if (aError == kErrorNoAck)
        {
            SetState(kStateDelayBeforeRetx);
            StartTimerForBackoff(mRetxDelayBackOffExponent);
            mRetxDelayBackOffExponent =
                Min(static_cast<uint8_t>(mRetxDelayBackOffExponent + 1), kRetxDelayMaxBackoffExponent);
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

        IgnoreError(Get<Radio>().Receive(aFrame.GetRxChannelAfterTxDone()));
    }
#endif

    mCallbacks.TransmitDone(aFrame, aAckFrame, aError);

exit:
    return;
}

void SubMac::SignalFrameCounterUsedOnTxDone(const TxFrame &aFrame)
{
    uint8_t  keyIdMode;
    uint8_t  keyId;
    uint32_t frameCounter;
    bool     allowError = false;

    OT_UNUSED_VARIABLE(allowError);

    VerifyOrExit(!ShouldHandleTransmitSecurity() && aFrame.GetSecurityEnabled() && aFrame.IsHeaderUpdated());

    // In an FTD/MTD build, if/when link-raw is enabled, the `TxFrame`
    // is prepared and given by user and may not necessarily follow 15.4
    // frame format (link raw can be used with vendor-specific format),
    // so we allow failure when parsing the frame (i.e., do not assert
    // on an error). In other cases (in an RCP build or in an FTD/MTD
    // build without link-raw) since the `TxFrame` should be prepared by
    // OpenThread core, we expect no error and therefore assert if
    // parsing fails.

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    allowError = Get<LinkRaw>().IsEnabled();
#endif

    VerifyOrExit(aFrame.GetKeyIdMode(keyIdMode) == kErrorNone, OT_ASSERT(allowError));
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1);

    VerifyOrExit(aFrame.GetFrameCounter(frameCounter) == kErrorNone, OT_ASSERT(allowError));
    VerifyOrExit(aFrame.GetKeyId(keyId) == kErrorNone, OT_ASSERT(allowError));

    SignalFrameCounterUsed(frameCounter, keyId);

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
        rssi = Get<Radio>().GetRssi();
    }

    return rssi;
}

int8_t SubMac::GetNoiseFloor(void) const { return Get<Radio>().GetReceiveSensitivity(); }

Error SubMac::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    Error error = kErrorNone;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
    case kStateTransmit:
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
#endif
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    case kStateDelayBeforeRetx:
#endif
    case kStateEnergyScan:
        ExitNow(error = kErrorInvalidState);

    case kStateReceive:
    case kStateSleep:
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    case kStateCslSample:
#endif
        break;
    }

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    VerifyOrExit(!mRadioFilterEnabled, HandleEnergyScanDone(Radio::kInvalidRssi));
#endif

    if (RadioSupportsEnergyScan())
    {
        IgnoreError(Get<Radio>().EnergyScan(aScanChannel, aScanDuration));
        SetState(kStateEnergyScan);
    }
    else if (ShouldHandleEnergyScan())
    {
        SuccessOrAssert(Get<Radio>().Receive(aScanChannel));

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
    OT_ASSERT(!RadioSupportsEnergyScan());

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

void SubMac::HandleTimer(void)
{
    switch (mState)
    {
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
        BeginTransmit();
        break;
#endif
    case kStateCsmaBackoff:
        BeginTransmit();
        break;

    case kStateTransmit:
        LogDebg("Ack timer timed out");
        IgnoreError(Get<Radio>().Receive(mTransmitFrame.GetChannel()));
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

    default:
        break;
    }
}

bool SubMac::ShouldHandleTransmitSecurity(void) const
{
    bool swTxSecurity = true;

    VerifyOrExit(!RadioSupportsTransmitSecurity(), swTxSecurity = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swTxSecurity = OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE;
#endif

exit:
    return swTxSecurity;
}

bool SubMac::ShouldHandleCsmaBackOff(void) const
{
    bool swCsma = true;

    VerifyOrExit(mTransmitFrame.IsCsmaCaEnabled() && !RadioSupportsCsmaBackoff(), swCsma = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swCsma = OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE;
#endif

exit:
    return swCsma;
}

bool SubMac::ShouldHandleAckTimeout(void) const
{
    bool swAckTimeout = true;

    VerifyOrExit(!RadioSupportsAckTimeout(), swAckTimeout = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swAckTimeout = OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE;
#endif

exit:
    return swAckTimeout;
}

bool SubMac::ShouldHandleRetries(void) const
{
    bool swRetries = true;

    VerifyOrExit(!RadioSupportsRetries(), swRetries = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swRetries = OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE;
#endif

exit:
    return swRetries;
}

bool SubMac::ShouldHandleEnergyScan(void) const
{
    bool swEnergyScan = true;

    VerifyOrExit(!RadioSupportsEnergyScan(), swEnergyScan = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swEnergyScan = OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE;
#endif

exit:
    return swEnergyScan;
}

bool SubMac::ShouldHandleTransmitTargetTime(void) const
{
    bool swTxDelay = true;

    VerifyOrExit(!RadioSupportsTransmitTiming(), swTxDelay = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swTxDelay = OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE;
#endif

exit:
    return swTxDelay;
}

bool SubMac::ShouldHandleTransitionToSleep(void) const { return (mRxOnWhenIdle || !RadioSupportsRxOnWhenIdle()); }

void SubMac::SetState(State aState)
{
    if (mState != aState)
    {
        LogDebg("RadioState: %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
}

void SubMac::SetMacKey(uint8_t            aKeyIdMode,
                       uint8_t            aKeyId,
                       const KeyMaterial &aPrevKey,
                       const KeyMaterial &aCurrKey,
                       const KeyMaterial &aNextKey)
{
    switch (aKeyIdMode)
    {
    case Frame::kKeyIdMode0:
    case Frame::kKeyIdMode2:
        break;
    case Frame::kKeyIdMode1:
        mKeyId   = aKeyId;
        mPrevKey = aPrevKey;
        mCurrKey = aCurrKey;
        mNextKey = aNextKey;
        break;

    default:
        OT_ASSERT(false);
        break;
    }

    VerifyOrExit(!ShouldHandleTransmitSecurity());

    Get<Radio>().SetMacKey(aKeyIdMode, aKeyId, aPrevKey, aCurrKey, aNextKey);

exit:
    return;
}

void SubMac::SignalFrameCounterUsed(uint32_t aFrameCounter, uint8_t aKeyId)
{
    VerifyOrExit(aKeyId == mKeyId);

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

    VerifyOrExit(!ShouldHandleTransmitSecurity());

    if (aSetIfLarger)
    {
        Get<Radio>().SetMacFrameCounterIfLarger(aFrameCounter);
    }
    else
    {
        Get<Radio>().SetMacFrameCounter(aFrameCounter);
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
    static const char *const kStateStrings[] = {
        "Disabled",    // (0) kStateDisabled
        "Sleep",       // (1) kStateSleep
        "Receive",     // (2) kStateReceive
        "CsmaBackoff", // (3) kStateCsmaBackoff
        "Transmit",    // (4) kStateTransmit
        "EnergyScan",  // (5) kStateEnergyScan
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
        "DelayBeforeRetx", // (6) kStateDelayBeforeRetx
#endif
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        "CslTransmit", // (7) kStateCslTransmit
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        "CslSample", // (8) kStateCslSample
#endif
    };

    struct StateValueChecker
    {
        InitEnumValidatorCounter();

        ValidateNextEnum(kStateDisabled);
        ValidateNextEnum(kStateSleep);
        ValidateNextEnum(kStateReceive);
        ValidateNextEnum(kStateCsmaBackoff);
        ValidateNextEnum(kStateTransmit);
        ValidateNextEnum(kStateEnergyScan);
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
        ValidateNextEnum(kStateDelayBeforeRetx);
#endif
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        ValidateNextEnum(kStateCslTransmit);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        ValidateNextEnum(kStateCslSample);
#endif
    };

    return kStateStrings[aState];
}

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
