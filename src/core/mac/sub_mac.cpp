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
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/random.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"
#include "mac/mac_frame.hpp"

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
    mCslPeriod     = 0;
    mCslChannel    = 0;
    mCslPeerShort  = 0;
    mIsCslSampling = false;
    mCslSampleTime = TimeMicro{0};
    mCslLastSync   = TimeMicro{0};
    mCslTimer.Stop();
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

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void SubMac::CslSample(void)
{
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    VerifyOrExit(!mRadioFilterEnabled, IgnoreError(Get<Radio>().Sleep()));
#endif

    SetState(kStateCslSample);

    if (mIsCslSampling && !RadioSupportsReceiveTiming())
    {
        IgnoreError(Get<Radio>().Receive(mCslChannel));
        ExitNow();
    }

#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
void SubMac::LogReceived(RxFrame *aFrame)
{
    static constexpr uint8_t kLogStringSize = 72;

    String<kLogStringSize> logString;
    Address                dst;
    int32_t                deviation;
    uint32_t               sampleTime, ahead, after;

    IgnoreError(aFrame->GetDstAddr(dst));

    VerifyOrExit((dst.GetType() == Address::kTypeShort && dst.GetShort() == GetShortAddress()) ||
                 (dst.GetType() == Address::kTypeExtended && dst.GetExtended() == GetExtAddress()));

    LogDebg("Received frame in state (SubMac %s, CSL %s), timestamp %lu", StateToString(mState),
            mIsCslSampling ? "CslSample" : "CslSleep",
            ToUlong(static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp)));

    VerifyOrExit(mState == kStateCslSample);

    GetCslWindowEdges(ahead, after);
    ahead -= kMinReceiveOnAhead + kCslReceiveTimeAhead;

    sampleTime = mCslSampleTime.GetValue() - mCslPeriod * kUsPerTenSymbols;
    deviation  = aFrame->mInfo.mRxInfo.mTimestamp + kRadioHeaderPhrDuration - sampleTime;

    // This logs three values (all in microseconds):
    // - Absolute sample time in which the CSL receiver expected the MHR of the received frame.
    // - Allowed margin around that time accounting for accuracy and uncertainty from both devices.
    // - Real deviation on the reception of the MHR with regards to expected sample time. This can
    //   be due to clocks drift and/or CSL Phase rounding error.
    // This means that a deviation absolute value greater than the margin would result in the frame
    // not being received out of the debug mode.
    logString.Append("Expected sample time %lu, margin ±%lu, deviation %d", ToUlong(sampleTime), ToUlong(ahead),
                     deviation);

    // Treat as a warning when the deviation is not within the margins. Neither kCslReceiveTimeAhead
    // or kMinReceiveOnAhead/kMinReceiveOnAfter are considered for the margin since they have no
    // impact on understanding possible deviation errors between transmitter and receiver. So in this
    // case only `ahead` is used, as an allowable max deviation in both +/- directions.
    if ((deviation + ahead > 0) && (deviation < static_cast<int32_t>(ahead)))
    {
        LogDebg("%s", logString.AsCString());
    }
    else
    {
        LogWarn("%s", logString.AsCString());
    }

exit:
    return;
}
#endif

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
    if (aFrame != nullptr && aError == kErrorNone)
    {
#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
        LogReceived(aFrame);
#endif
        // Assuming the risk of the parent missing the Enh-ACK in favor of smaller CSL receive window
        if ((mCslPeriod > 0) && aFrame->mInfo.mRxInfo.mAckedWithSecEnhAck)
        {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
            mCslLastSync = TimerMicro::GetNow();
#else
            mCslLastSync = TimeMicro(static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp));
#endif
        }
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

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
            if (Time(static_cast<uint32_t>(otPlatRadioGetNow(&GetInstance()))) <
                Time(mTransmitFrame.mInfo.mTxInfo.mTxDelayBaseTime) + mTransmitFrame.mInfo.mTxInfo.mTxDelay -
                    kCcaSampleInterval - kCslTransmitTimeAhead - kRadioHeaderShrDuration)
            {
                mTimer.StartAt(Time(mTransmitFrame.mInfo.mTxInfo.mTxDelayBaseTime) - kCcaSampleInterval -
                                   kCslTransmitTimeAhead - kRadioHeaderShrDuration,
                               mTransmitFrame.mInfo.mTxInfo.mTxDelay);
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

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    mTimer.Start(backoff);
#else
    mTimer.Start(backoff / 1000UL);
#endif

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

    if (mPcapCallback.IsSet())
    {
        mPcapCallback.Invoke(&mTransmitFrame, true);
    }

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
    if (ShouldHandleAckTimeout() && aFrame.GetAckRequest())
    {
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
        mTimer.Start(kAckTimeout * 1000UL);
#else
        mTimer.Start(kAckTimeout);
#endif
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
        // Actual synchronization timestamp should be from the sent frame instead of the current time.
        // Assuming the error here since it is bounded and has very small effect on the final window duration.
        if (aAckFrame != nullptr && aFrame.GetHeaderIe(CslIe::kHeaderIeId) != nullptr)
        {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
            mCslLastSync = TimerMicro::GetNow();
#else
            mCslLastSync = TimeMicro(static_cast<uint32_t>(otPlatRadioGetNow(&GetInstance())));
#endif
        }
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
    if (aFrame.GetChannel() != aFrame.GetRxChannelAfterTxDone())
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
        mTimer.Start(0);
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
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
        mTimer.StartAt(mTimer.GetFireTime(), kEnergyScanRssiSampleInterval * 1000UL);
#else
        mTimer.StartAt(mTimer.GetFireTime(), kEnergyScanRssiSampleInterval);
#endif
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

    VerifyOrExit(!RadioSupportsCsmaBackoff(), swCsma = false);

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

    static_assert(kStateDisabled == 0, "kStateDisabled value is not correct");
    static_assert(kStateSleep == 1, "kStateSleep value is not correct");
    static_assert(kStateReceive == 2, "kStateReceive value is not correct");
    static_assert(kStateCsmaBackoff == 3, "kStateCsmaBackoff value is not correct");
    static_assert(kStateTransmit == 4, "kStateTransmit value is not correct");
    static_assert(kStateEnergyScan == 5, "kStateEnergyScan value is not correct");

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    static_assert(kStateDelayBeforeRetx == 6, "kStateDelayBeforeRetx value is not correct");
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    static_assert(kStateCslTransmit == 7, "kStateCslTransmit value is not correct");
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    static_assert(kStateCslSample == 8, "kStateCslSample value is not correct");
#endif
#elif OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    static_assert(kStateCslSample == 7, "kStateCslSample value is not correct");
#endif

#elif !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    static_assert(kStateCslTransmit == 6, "kStateCslTransmit value is not correct");
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    static_assert(kStateCslSample == 7, "kStateCslSample value is not correct");
#endif
#elif OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    static_assert(kStateCslSample == 6, "kStateCslSample value is not correct");
#endif

    return kStateStrings[aState];
}

// LCOV_EXCL_STOP

//---------------------------------------------------------------------------------------------------------------------
// CSL Receiver methods

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
bool SubMac::UpdateCsl(uint16_t aPeriod, uint8_t aChannel, otShortAddress aShortAddr, const otExtAddress *aExtAddr)
{
    bool diffPeriod  = aPeriod != mCslPeriod;
    bool diffChannel = aChannel != mCslChannel;
    bool diffPeer    = aShortAddr != mCslPeerShort;
    bool retval      = diffPeriod || diffChannel || diffPeer;

    VerifyOrExit(retval);
    mCslChannel = aChannel;

    VerifyOrExit(diffPeriod || diffPeer);
    mCslPeriod    = aPeriod;
    mCslPeerShort = aShortAddr;
    IgnoreError(Get<Radio>().EnableCsl(aPeriod, aShortAddr, aExtAddr));

    mCslTimer.Stop();
    if (mCslPeriod > 0)
    {
        mCslSampleTime = TimeMicro(static_cast<uint32_t>(otPlatRadioGetNow(&GetInstance())));
        mIsCslSampling = false;
        HandleCslTimer();
    }

exit:
    return retval;
}

void SubMac::HandleCslTimer(Timer &aTimer) { aTimer.Get<SubMac>().HandleCslTimer(); }

void SubMac::HandleCslTimer(void)
{
    /*
     * CSL sample timing diagram
     *    |<---------------------------------Sample--------------------------------->|<--------Sleep--------->|
     *    |                                                                          |                        |
     *    |<--Ahead-->|<--UnCert-->|<--Drift-->|<--Drift-->|<--UnCert-->|<--MinWin-->|                        |
     *    |           |            |           |           |            |            |                        |
     * ---|-----------|------------|-----------|-----------|------------|------------|----------//------------|---
     * -timeAhead                           CslPhase                             +timeAfter             -timeAhead
     *
     * The handler works in different ways when the radio supports receive-timing and doesn't.
     *
     * When the radio supports receive-timing:
     *   The handler will be called once per CSL period. When the handler is called, it will set the timer to
     *   fire at the next CSL sample time and call `Radio::ReceiveAt` to start sampling for the current CSL period.
     *   The timer fires some time before the actual sample time. After `Radio::ReceiveAt` is called, the radio will
     *   remain in sleep state until the actual sample time.
     *   Note that it never call `Radio::Sleep` explicitly. The radio will fall into sleep after `ReceiveAt` ends. This
     *   will be done by the platform as part of the `otPlatRadioReceiveAt` API.
     *
     *   Timer fires                                         Timer fires
     *       ^                                                    ^
     *       x-|------------|-------------------------------------x-|------------|---------------------------------------|
     *            sample                   sleep                        sample                    sleep
     *
     * When the radio doesn't support receive-timing:
     *   The handler will be called twice per CSL period: at the beginning of sample and sleep. When the handler is
     *   called, it will explicitly change the radio state due to the current state by calling `Radio::Receive` or
     *   `Radio::Sleep`.
     *
     *   Timer fires  Timer fires                            Timer fires  Timer fires
     *       ^            ^                                       ^            ^
     *       |------------|---------------------------------------|------------|---------------------------------------|
     *          sample                   sleep                        sample                    sleep
     *
     */
    uint32_t periodUs = mCslPeriod * kUsPerTenSymbols;
    uint32_t timeAhead, timeAfter, winStart, winDuration;

    GetCslWindowEdges(timeAhead, timeAfter);

    if (mIsCslSampling)
    {
        mIsCslSampling = false;
        mCslTimer.FireAt(mCslSampleTime - timeAhead);
        if (mState == kStateCslSample)
        {
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
            IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif
            LogDebg("CSL sleep %lu", ToUlong(mCslTimer.GetNow().GetValue()));
        }
    }
    else
    {
        if (RadioSupportsReceiveTiming())
        {
            mCslTimer.FireAt(mCslSampleTime - timeAhead + periodUs);
            timeAhead -= kCslReceiveTimeAhead;
            winStart = mCslSampleTime.GetValue() - timeAhead;
        }
        else
        {
            mCslTimer.FireAt(mCslSampleTime + timeAfter);
            mIsCslSampling = true;
            winStart       = ot::TimerMicro::GetNow().GetValue();
        }

        winDuration = timeAhead + timeAfter;
        mCslSampleTime += periodUs;

        Get<Radio>().UpdateCslSampleTime(mCslSampleTime.GetValue());

        // Schedule reception window for any state except RX - so that CSL RX Window has lower priority
        // than scanning or RX after the data poll.
        if (RadioSupportsReceiveTiming() && (mState != kStateDisabled) && (mState != kStateReceive))
        {
            IgnoreError(Get<Radio>().ReceiveAt(mCslChannel, winStart, winDuration));
        }
        else if (mState == kStateCslSample)
        {
            IgnoreError(Get<Radio>().Receive(mCslChannel));
        }

        LogDebg("CSL window start %lu, duration %lu", ToUlong(winStart), ToUlong(winDuration));
    }
}

void SubMac::GetCslWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
{
    uint32_t semiPeriod = mCslPeriod * kUsPerTenSymbols / 2;
    uint32_t curTime, elapsed, semiWindow;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    curTime = TimerMicro::GetNow().GetValue();
#else
    curTime = static_cast<uint32_t>(otPlatRadioGetNow(&GetInstance()));
#endif

    elapsed = curTime - mCslLastSync.GetValue();

    semiWindow =
        static_cast<uint32_t>(static_cast<uint64_t>(elapsed) *
                              (Get<Radio>().GetCslAccuracy() + mCslParentAccuracy.GetClockAccuracy()) / 1000000);
    semiWindow += mCslParentAccuracy.GetUncertaintyInMicrosec() + Get<Radio>().GetCslUncertainty() * 10;

    aAhead = Min(semiPeriod, semiWindow + kMinReceiveOnAhead + kCslReceiveTimeAhead);
    aAfter = Min(semiPeriod, semiWindow + kMinReceiveOnAfter);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

} // namespace Mac
} // namespace ot
