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

#include "mac_frame.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"

namespace ot {
namespace Mac {

SubMac::SubMac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRadioCaps(Get<Radio>().GetCaps())
    , mState(kStateDisabled)
    , mCsmaBackoffs(0)
    , mTransmitRetries(0)
    , mShortAddress(kShortAddrInvalid)
    , mRxOnWhenBackoff(true)
    , mEnergyScanMaxRssi(kInvalidRssiValue)
    , mEnergyScanEndTime(0)
    , mTransmitFrame(Get<Radio>().GetTransmitBuffer())
    , mCallbacks(aInstance)
    , mPcapCallback(nullptr)
    , mPcapCallbackContext(nullptr)
    , mFrameCounter(0)
    , mKeyId(0)
    , mTimer(aInstance, SubMac::HandleTimer, this)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mCslTimeout(OPENTHREAD_CONFIG_CSL_TIMEOUT)
    , mCslPeriod(0)
    , mCslChannel(0)
    , mIsCslChannelSpecified(false)
    , mCslState(kCslIdle)
    , mCslTimer(aInstance, SubMac::HandleCslTimer, this)
#endif
{
    mExtAddress.Clear();
    mPrevKey.Clear();
    mCurrKey.Clear();
    mNextKey.Clear();
}

otRadioCaps SubMac::GetCaps(void) const
{
    otRadioCaps caps = mRadioCaps;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

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

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
    caps |= OT_RADIO_CAPS_TRANSMIT_SEC;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
    caps |= OT_RADIO_CAPS_TRANSMIT_TIMING;
#endif

#else
    caps = OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES |
           OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING;
#endif

    return caps;
}

void SubMac::SetPanId(PanId aPanId)
{
    Get<Radio>().SetPanId(aPanId);
    otLogDebgMac("RadioPanId: 0x%04x", aPanId);
}

void SubMac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    Get<Radio>().SetShortAddress(mShortAddress);
    otLogDebgMac("RadioShortAddress: 0x%04x", mShortAddress);
}

void SubMac::SetExtAddress(const ExtAddress &aExtAddress)
{
    ExtAddress address;

    mExtAddress = aExtAddress;

    // Reverse the byte order before setting on radio.
    address.Set(aExtAddress.m8, ExtAddress::kReverseByteOrder);
    Get<Radio>().SetExtendedAddress(address);

    otLogDebgMac("RadioExtAddress: %s", mExtAddress.ToString().AsCString());
}

void SubMac::SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    mPcapCallback        = aPcapCallback;
    mPcapCallbackContext = aCallbackContext;
}

otError SubMac::Enable(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kStateDisabled, OT_NOOP);

    SuccessOrExit(error = Get<Radio>().Enable());
    SuccessOrExit(error = Get<Radio>().Sleep());

    SetState(kStateSleep);

exit:
    OT_ASSERT(error == OT_ERROR_NONE);
    return error;
}

otError SubMac::Disable(void)
{
    otError error;

    mTimer.Stop();
    SuccessOrExit(error = Get<Radio>().Sleep());
    SuccessOrExit(error = Get<Radio>().Disable());
    SetState(kStateDisabled);

exit:
    return error;
}

otError SubMac::Sleep(void)
{
    otError error = Get<Radio>().Sleep();

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("RadioSleep() failed, error: %s", otThreadErrorToString(error));
        ExitNow();
    }

    SetState(kStateSleep);

exit:
    return error;
}

otError SubMac::Receive(uint8_t aChannel)
{
    otError error = Get<Radio>().Receive(aChannel);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("RadioReceive() failed, error: %s", otThreadErrorToString(error));
        ExitNow();
    }

    SetState(kStateReceive);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError SubMac::CslSample(uint8_t aPanChannel)
{
    otError error = OT_ERROR_NONE;

    if (!IsCslChannelSpecified())
    {
        mCslChannel = aPanChannel;
    }

    switch (mCslState)
    {
    case kCslSample:
        error = Get<Radio>().Receive(mCslChannel);
        break;
    case kCslSleep:
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
        error = Get<Radio>().Sleep(); // Don't actually sleep for debugging
#endif
        break;
    case kCslIdle:
        ExitNow(error = OT_ERROR_INVALID_STATE);
    default:
        OT_ASSERT(false);
    }

    SetState(kStateCslSample);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("CslSample() failed, error: %s", otThreadErrorToString(error));
    }
    return error;
}
#endif

void SubMac::HandleReceiveDone(RxFrame *aFrame, otError aError)
{
    if (mPcapCallback && (aFrame != nullptr) && (aError == OT_ERROR_NONE))
    {
        mPcapCallback(aFrame, false, mPcapCallbackContext);
    }

    if (!ShouldHandleTransmitSecurity() && aFrame != nullptr && aFrame->mInfo.mRxInfo.mAckedWithSecEnhAck)
    {
        UpdateFrameCounter(aFrame->mInfo.mRxInfo.mAckFrameCounter);
    }

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    if (aFrame != nullptr && aError == OT_ERROR_NONE)
    {
        // Split the log into two lines for RTT to output
        otLogDebgMac("Received frame in state (SubMac %s, CSL %s), timestamp %u", StateToString(mState),
                     CslStateToString(mCslState), static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp));
        otLogDebgMac("Target sample start time %u, time drift %d", mCslSampleTime.GetValue(),
                     static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp) - mCslSampleTime.GetValue());
    }
#endif

    mCallbacks.ReceiveDone(aFrame, aError);
}

otError SubMac::Send(void)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
#endif
    case kStateTransmit:
    case kStateEnergyScan:
        ExitNow(error = OT_ERROR_INVALID_STATE);
        OT_UNREACHABLE_CODE(break);

    case kStateSleep:
    case kStateReceive:
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    case kStateCslSample:
#endif
        break;
    }

    ProcessTransmitSecurity();
    mCsmaBackoffs    = 0;
    mTransmitRetries = 0;
    StartCsmaBackoff();

exit:
    return error;
}

void SubMac::ProcessTransmitSecurity(void)
{
    const ExtAddress *extAddress = nullptr;
    uint8_t           keyIdMode;

    VerifyOrExit(ShouldHandleTransmitSecurity(), OT_NOOP);
    VerifyOrExit(mTransmitFrame.GetSecurityEnabled(), OT_NOOP);
    VerifyOrExit(!mTransmitFrame.IsSecurityProcessed(), OT_NOOP);

    SuccessOrExit(mTransmitFrame.GetKeyIdMode(keyIdMode));
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1, OT_NOOP);

    mTransmitFrame.SetAesKey(GetCurrentMacKey());

    if (!mTransmitFrame.IsARetransmission())
    {
        uint32_t frameCounter = GetFrameCounter();

        mTransmitFrame.SetKeyId(mKeyId);
        mTransmitFrame.SetFrameCounter(frameCounter);
        UpdateFrameCounter(frameCounter + 1);
    }

    extAddress = &GetExtAddress();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Transmit security will be processed after time IE content is updated.
    VerifyOrExit(mTransmitFrame.GetTimeIeOffset() == 0, OT_NOOP);
#endif

    mTransmitFrame.ProcessTransmitAesCcm(*extAddress);

exit:
    return;
}

void SubMac::StartCsmaBackoff(void)
{
    uint32_t backoff;
    uint32_t backoffExponent = kMinBE + mTransmitRetries + mCsmaBackoffs;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (mTransmitFrame.mInfo.mTxInfo.mPeriod != 0)
    {
        SetState(kStateCslTransmit);

        if (ShouldHandleTransmitTargetTime())
        {
            uint32_t phaseNow =
                (otPlatRadioGetNow(&GetInstance()) / kUsPerTenSymbols) % mTransmitFrame.mInfo.mTxInfo.mPeriod;
            uint32_t phaseDesired = mTransmitFrame.mInfo.mTxInfo.mPhase;

            if (phaseNow < phaseDesired)
            {
                mTimer.Start((phaseDesired - phaseNow) * kUsPerTenSymbols);
            }
            else if (phaseNow > phaseDesired)
            {
                mTimer.Start((phaseDesired + mTransmitFrame.mInfo.mTxInfo.mPeriod - phaseNow) * kUsPerTenSymbols);
            }
            else
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
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    SetState(kStateCsmaBackoff);

    VerifyOrExit(ShouldHandleCsmaBackOff(), BeginTransmit());

    if (backoffExponent > kMaxBE)
    {
        backoffExponent = kMaxBE;
    }

    backoff = Random::NonCrypto::GetUint32InRange(0, static_cast<uint32_t>(1UL << backoffExponent));
    backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    if (mRxOnWhenBackoff)
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

exit:
    return;
}

void SubMac::BeginTransmit(void)
{
    otError error;

    OT_UNUSED_VARIABLE(error);

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    VerifyOrExit(mState == kStateCsmaBackoff || mState == kStateCslTransmit, OT_NOOP);
#else
    VerifyOrExit(mState == kStateCsmaBackoff, OT_NOOP);
#endif

    mTransmitFrame.SetCsmaCaEnabled(mTransmitFrame.mInfo.mTxInfo.mPeriod == 0);

    if ((mRadioCaps & OT_RADIO_CAPS_SLEEP_TO_TX) == 0)
    {
        error = Get<Radio>().Receive(mTransmitFrame.GetChannel());
        OT_ASSERT(error == OT_ERROR_NONE);
    }

    SetState(kStateTransmit);

    if (mPcapCallback)
    {
        mPcapCallback(&mTransmitFrame, true, mPcapCallbackContext);
    }

    error = Get<Radio>().Transmit(mTransmitFrame);
    OT_ASSERT(error == OT_ERROR_NONE);

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

void SubMac::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, otError aError)
{
    bool ccaSuccess = true;
    bool shouldRetx;

    // Stop ack timeout timer.

    mTimer.Stop();

    // Record CCA success or failure status.

    switch (aError)
    {
    case OT_ERROR_ABORT:
        // Do not record CCA status in case of `ABORT` error
        // since there may be no CCA check performed by radio.
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        ccaSuccess = false;

        // fall through

    case OT_ERROR_NONE:
    case OT_ERROR_NO_ACK:
        if (aFrame.IsCsmaCaEnabled())
        {
            mCallbacks.RecordCcaStatus(ccaSuccess, aFrame.GetChannel());
        }

        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

    UpdateFrameCounterOnTxDone(aFrame);

    // Determine whether a CSMA retry is required.

    if (!ccaSuccess && ShouldHandleCsmaBackOff() && mCsmaBackoffs < aFrame.GetMaxCsmaBackoffs())
    {
        mCsmaBackoffs++;
        StartCsmaBackoff();
        ExitNow();
    }

    mCsmaBackoffs = 0;

    // Determine whether to re-transmit the frame.

    shouldRetx =
        ((aError != OT_ERROR_NONE) && ShouldHandleRetries() && (mTransmitRetries < aFrame.GetMaxFrameRetries()));

    mCallbacks.RecordFrameTransmitStatus(aFrame, aAckFrame, aError, mTransmitRetries, shouldRetx);

    if (shouldRetx)
    {
        mTransmitRetries++;
        aFrame.SetIsARetransmission(true);
        StartCsmaBackoff();
        ExitNow();
    }

    SetState(kStateReceive);

    mCallbacks.TransmitDone(aFrame, aAckFrame, aError);

exit:
    return;
}

void SubMac::UpdateFrameCounterOnTxDone(const TxFrame &aFrame)
{
    uint8_t  keyIdMode;
    uint32_t frameCounter;
    bool     allowError = false;

    OT_UNUSED_VARIABLE(allowError);

    VerifyOrExit(!ShouldHandleTransmitSecurity() && aFrame.GetSecurityEnabled(), OT_NOOP);

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

    VerifyOrExit(aFrame.GetKeyIdMode(keyIdMode) == OT_ERROR_NONE, OT_ASSERT(allowError));
    VerifyOrExit(keyIdMode == Frame::kKeyIdMode1, OT_NOOP);

    VerifyOrExit(aFrame.GetFrameCounter(frameCounter) == OT_ERROR_NONE, OT_ASSERT(allowError));
    UpdateFrameCounter(frameCounter);

exit:
    return;
}

int8_t SubMac::GetRssi(void) const
{
    return Get<Radio>().GetRssi();
}

int8_t SubMac::GetNoiseFloor(void)
{
    return Get<Radio>().GetReceiveSensitivity();
}

otError SubMac::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
    case kStateTransmit:
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
#endif
    case kStateEnergyScan:
        ExitNow(error = OT_ERROR_INVALID_STATE);

    case kStateReceive:
    case kStateSleep:
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    case kStateCslSample:
#endif
        break;
    }

    if (RadioSupportsEnergyScan())
    {
        IgnoreError(Get<Radio>().EnergyScan(aScanChannel, aScanDuration));
        SetState(kStateEnergyScan);
    }
    else if (ShouldHandleEnergyScan())
    {
        error = Get<Radio>().Receive(aScanChannel);
        OT_ASSERT(error == OT_ERROR_NONE);

        SetState(kStateEnergyScan);
        mEnergyScanMaxRssi = kInvalidRssiValue;
        mEnergyScanEndTime = TimerMilli::GetNow() + static_cast<uint32_t>(aScanDuration);
        mTimer.Start(0);
    }
    else
    {
        error = OT_ERROR_NOT_IMPLEMENTED;
    }

exit:
    return error;
}

void SubMac::SampleRssi(void)
{
    OT_ASSERT(!RadioSupportsEnergyScan());

    int8_t rssi = GetRssi();

    if (rssi != kInvalidRssiValue)
    {
        if ((mEnergyScanMaxRssi == kInvalidRssiValue) || (rssi > mEnergyScanMaxRssi))
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

void SubMac::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<SubMac>().HandleTimer();
}

void SubMac::HandleTimer(void)
{
    switch (mState)
    {
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
        BeginTransmit();
        break;
#endif
    case kStateCsmaBackoff:
        BeginTransmit();
        break;

    case kStateTransmit:
        otLogDebgMac("Ack timer timed out");
        IgnoreError(Get<Radio>().Receive(mTransmitFrame.GetChannel()));
        HandleTransmitDone(mTransmitFrame, nullptr, OT_ERROR_NO_ACK);
        break;

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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
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
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), OT_NOOP);
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swTxDelay = OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE;
#endif

exit:
    return swTxDelay;
}

void SubMac::SetState(State aState)
{
    if (mState != aState)
    {
        otLogDebgMac("RadioState: %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
}

void SubMac::SetMacKey(uint8_t    aKeyIdMode,
                       uint8_t    aKeyId,
                       const Key &aPrevKey,
                       const Key &aCurrKey,
                       const Key &aNextKey)
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

    VerifyOrExit(!ShouldHandleTransmitSecurity(), OT_NOOP);

    Get<Radio>().SetMacKey(aKeyIdMode, aKeyId, aPrevKey, aCurrKey, aNextKey);

exit:
    return;
}

void SubMac::UpdateFrameCounter(uint32_t aFrameCounter)
{
    mFrameCounter = aFrameCounter;

    mCallbacks.FrameCounterUpdated(aFrameCounter);
}

void SubMac::SetFrameCounter(uint32_t aFrameCounter)
{
    mFrameCounter = aFrameCounter;

    VerifyOrExit(!ShouldHandleTransmitSecurity(), OT_NOOP);

    Get<Radio>().SetMacFrameCounter(aFrameCounter);

exit:
    return;
}

// LCOV_EXCL_START

const char *SubMac::StateToString(State aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case kStateDisabled:
        str = "Disabled";
        break;
    case kStateSleep:
        str = "Sleep";
        break;
    case kStateReceive:
        str = "Receive";
        break;
    case kStateCsmaBackoff:
        str = "CsmaBackoff";
        break;
    case kStateTransmit:
        str = "Transmit";
        break;
    case kStateEnergyScan:
        str = "EnergyScan";
        break;
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    case kStateCslTransmit:
        str = "CslTransmit";
        break;
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    case kStateCslSample:
        str = "CslSample";
        break;
#endif
    }

    return str;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
const char *SubMac::CslStateToString(CslState aCslState)
{
    const char *str = "Unknown";

    switch (aCslState)
    {
    case kCslIdle:
        str = "CslIdle";
        break;
    case kCslSample:
        str = "CslSample";
        break;
    case kCslSleep:
        str = "kCslSleep";
        break;
    default:
        break;
    }

    return str;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

// LCOV_EXCL_STOP

//---------------------------------------------------------------------------------------------------------------------
// CSL Receiver methods

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void SubMac::SetCslChannel(uint8_t aChannel)
{
    mCslChannel = aChannel;
}

void SubMac::SetCslPeriod(uint16_t aPeriod)
{
    VerifyOrExit(mCslPeriod != aPeriod, OT_NOOP);

    mCslPeriod = aPeriod;

    mCslTimer.Stop();

    if (mCslPeriod > 0)
    {
        mCslSampleTime = mCslTimer.GetNow();
        Get<Radio>().UpdateCslSampleTime(mCslSampleTime.GetValue());

        mCslState = kCslSample;
        HandleCslTimer();
    }
    else
    {
        mCslState = kCslIdle;

        if (mState == kStateCslSample)
        {
            IgnoreError(Get<Radio>().Sleep());
            SetState(kStateSleep);
        }
    }

    otLogDebgMac("CSL Period: %u", mCslPeriod);

exit:
    return;
}

void SubMac::SetCslTimeout(uint32_t aTimeout)
{
    mCslTimeout = aTimeout;
}

void SubMac::FillCsl(Frame &aFrame)
{
    uint8_t *cur = aFrame.GetHeaderIe(Frame::kHeaderIeCsl);

    if (cur != nullptr)
    {
        CslIe *csl = reinterpret_cast<CslIe *>(cur + sizeof(HeaderIe));

        csl->SetPeriod(mCslPeriod);
        csl->SetPhase(GetCslPhase());
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        otLogDebgMac("%10u:FillCsl() seq=%u phase=%hu", static_cast<uint32_t>(otPlatTimeGet()), aFrame.GetSequence(),
                     csl->GetPhase());
#endif
    }
}

void SubMac::HandleCslTimer(Timer &aTimer)
{
    aTimer.GetOwner<SubMac>().HandleCslTimer();
}

void SubMac::HandleCslTimer(void)
{
    switch (mCslState)
    {
    case kCslSample:
        mCslState = kCslSleep;
        // kUsPerTenSymbols: computing CSL Phase using floor division.
        mCslTimer.StartAt(mCslSampleTime, mCslPeriod * kUsPerTenSymbols - kUsPerTenSymbols);
        if (mState == kStateCslSample)
        {
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
            IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif
            otLogDebgMac("CSL sleep %u", mCslTimer.GetNow().GetValue());
        }
        break;

    case kCslSleep:
        mCslState = kCslSample;

        mCslSampleTime += mCslPeriod * kUsPerTenSymbols;
        Get<Radio>().UpdateCslSampleTime(mCslSampleTime.GetValue());

        mCslTimer.StartAt(mCslSampleTime, kCslSampleWindow);

        if (mState == kStateCslSample)
        {
            IgnoreError(Get<Radio>().Receive(mCslChannel));
            otLogDebgMac("CSL sample %u", mCslTimer.GetNow().GetValue());
        }
        break;

    case kCslIdle:
        break;

    default:
        OT_ASSERT(false);
        break;
    }
}

uint16_t SubMac::GetCslPhase(void) const
{
    TimeMicro now = TimerMicro::GetNow();
    uint32_t  delta;

    if (mCslSampleTime < now)
    {
        delta = mCslSampleTime + mCslPeriod * kUsPerTenSymbols - now;
    }
    else
    {
        delta = mCslSampleTime - now;
    }

    return static_cast<uint16_t>(delta / kUsPerTenSymbols);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

} // namespace Mac
} // namespace ot
