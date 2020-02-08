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
    , mPcapCallback(NULL)
    , mPcapCallbackContext(NULL)
    , mTimer(aInstance, &SubMac::HandleTimer, this)
{
    mExtAddress.Clear();
}

otRadioCaps SubMac::GetCaps(void) const
{
    otRadioCaps caps = mRadioCaps;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#if OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE
    caps |= OT_RADIO_CAPS_ACK_TIMEOUT;
#endif

#if OPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE
    caps |= OT_RADIO_CAPS_CSMA_BACKOFF;
#endif

#if OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE
    caps |= OT_RADIO_CAPS_TRANSMIT_RETRIES;
#endif

#if OPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE
    caps |= OT_RADIO_CAPS_ENERGY_SCAN;
#endif

#else
    caps = OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES |
           OT_RADIO_CAPS_ENERGY_SCAN;
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

    VerifyOrExit(mState == kStateDisabled);

    SuccessOrExit(error = Get<Radio>().Enable());
    SuccessOrExit(error = Get<Radio>().Sleep());
    SetState(kStateSleep);

exit:
    assert(error == OT_ERROR_NONE);
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

void SubMac::HandleReceiveDone(RxFrame *aFrame, otError aError)
{
    if (mPcapCallback && (aFrame != NULL) && (aError == OT_ERROR_NONE))
    {
        mPcapCallback(aFrame, false, mPcapCallbackContext);
    }

    mCallbacks.ReceiveDone(aFrame, aError);
}

otError SubMac::Send(void)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
    case kStateTransmit:
    case kStateEnergyScan:
        ExitNow(error = OT_ERROR_INVALID_STATE);
        break;

    case kStateSleep:
    case kStateReceive:
        break;
    }

    mCsmaBackoffs    = 0;
    mTransmitRetries = 0;
    StartCsmaBackoff();

exit:
    return error;
}

void SubMac::StartCsmaBackoff(void)
{
    uint32_t backoff;
    uint32_t backoffExponent = kMinBE + mTransmitRetries + mCsmaBackoffs;

    SetState(kStateCsmaBackoff);

    VerifyOrExit(ShouldHandleCsmaBackOff(), BeginTransmit());

#if OPENTHREAD_CONFIG_MAC_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
    if ((mTransmitRetries > 0) && (mTransmitRetries == mTransmitFrame.GetMaxFrameRetries()))
    {
        BeginTransmit();
        ExitNow();
    }
#endif

    if (backoffExponent > kMaxBE)
    {
        backoffExponent = kMaxBE;
    }

    backoff = Random::NonCrypto::GetUint32InRange(0, static_cast<uint32_t>(1UL << backoffExponent));
    backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    if (mRxOnWhenBackoff)
    {
        Get<Radio>().Receive(mTransmitFrame.GetChannel());
    }
    else
    {
        Get<Radio>().Sleep();
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

    VerifyOrExit(mState == kStateCsmaBackoff);

#if OPENTHREAD_CONFIG_MAC_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
    if ((mTransmitRetries > 0) && (mTransmitRetries == mTransmitFrame.GetMaxFrameRetries()))
    {
        mTransmitFrame.SetCsmaCaEnabled(false);
    }
    else
#endif
    {
        mTransmitFrame.SetCsmaCaEnabled(true);
    }

    error = Get<Radio>().Receive(mTransmitFrame.GetChannel());
    assert(error == OT_ERROR_NONE);

    SetState(kStateTransmit);

    if (mPcapCallback)
    {
        mPcapCallback(&mTransmitFrame, true, mPcapCallbackContext);
    }

    error = Get<Radio>().Transmit(mTransmitFrame);
    assert(error == OT_ERROR_NONE);

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
        assert(false);
        OT_UNREACHABLE_CODE(ExitNow());
    }

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

int8_t SubMac::GetRssi(void) const
{
    return Get<Radio>().GetRssi();
}

otError SubMac::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateDisabled:
    case kStateCsmaBackoff:
    case kStateTransmit:
    case kStateEnergyScan:
        ExitNow(error = OT_ERROR_INVALID_STATE);

    case kStateReceive:
    case kStateSleep:
        break;
    }

    if (RadioSupportsEnergyScan())
    {
        Get<Radio>().EnergyScan(aScanChannel, aScanDuration);
        SetState(kStateEnergyScan);
    }
    else if (ShouldHandleEnergyScan())
    {
        error = Get<Radio>().Receive(aScanChannel);
        assert(error == OT_ERROR_NONE);

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
    assert(!RadioSupportsEnergyScan());

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
    case kStateCsmaBackoff:
        BeginTransmit();
        break;

    case kStateTransmit:
        otLogDebgMac("Ack timer timed out");
        Get<Radio>().Receive(mTransmitFrame.GetChannel());
        HandleTransmitDone(mTransmitFrame, NULL, OT_ERROR_NO_ACK);
        break;

    case kStateEnergyScan:
        SampleRssi();
        break;

    default:
        break;
    }
}

bool SubMac::ShouldHandleCsmaBackOff(void) const
{
    bool swCsma = true;

    VerifyOrExit(!RadioSupportsCsmaBackoff(), swCsma = false);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(Get<LinkRaw>().IsEnabled());
#endif

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE || OPENTHREAD_RADIO
    swCsma = OPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE;
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
    swAckTimeout = OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE;
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
    swRetries = OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE;
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
    swEnergyScan = OPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE;
#endif

exit:
    return swEnergyScan;
}

void SubMac::SetState(State aState)
{
    if (mState != aState)
    {
        otLogDebgMac("RadioState: %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
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
    }

    return str;
}

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
