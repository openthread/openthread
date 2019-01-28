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

#define WPP_NAME "sub_mac.tmh"

#include "sub_mac.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"

namespace ot {
namespace Mac {

SubMac::SubMac(Instance &aInstance, Callbacks &aCallbacks)
    : InstanceLocator(aInstance)
    , mRadioCaps(otPlatRadioGetCaps(&aInstance))
    , mState(kStateDisabled)
    , mCsmaBackoffs(0)
    , mTransmitRetries(0)
    , mShortAddress(kShortAddrInvalid)
    , mRxOnWhenBackoff(true)
    , mEnergyScanMaxRssi(kInvalidRssiValue)
    , mEnergyScanEndTime(0)
    , mTransmitFrame(*static_cast<Frame *>(otPlatRadioGetTransmitBuffer(&aInstance)))
    , mCallbacks(aCallbacks)
    , mPcapCallback(NULL)
    , mPcapCallbackContext(NULL)
    , mTimer(aInstance, &SubMac::HandleTimer, this)
{
    memset(mExtAddress.m8, 0, sizeof(mExtAddress));
}

otRadioCaps SubMac::GetCaps(void) const
{
    otRadioCaps caps = mRadioCaps;

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    caps |= OT_RADIO_CAPS_ACK_TIMEOUT;
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    caps |= OT_RADIO_CAPS_CSMA_BACKOFF;
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    caps |= OT_RADIO_CAPS_TRANSMIT_RETRIES;
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
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
    otPlatRadioSetPanId(&GetInstance(), aPanId);
    otLogDebgMac("RadioPanId: 0x%04x", aPanId);
}

void SubMac::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    otPlatRadioSetShortAddress(&GetInstance(), mShortAddress);
    otLogDebgMac("RadioShortAddress: 0x%04x", mShortAddress);
}

void SubMac::SetExtAddress(const ExtAddress &aExtAddress)
{
    Address address;

    mExtAddress = aExtAddress;

    // Reverse the byte order before setting on radio.
    address.SetExtended(aExtAddress.m8, /* aReverse */ true);
    otPlatRadioSetExtendedAddress(&GetInstance(), &address.GetExtended());

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

    SuccessOrExit(error = otPlatRadioEnable(&GetInstance()));
    error = otPlatRadioSleep(&GetInstance());
    assert(error == OT_ERROR_NONE);
    SetState(kStateSleep);

exit:
    return error;
}

otError SubMac::Disable(void)
{
    otError error;

    mTimer.Stop();
    error = otPlatRadioDisable(&GetInstance());
    assert(error == OT_ERROR_NONE);
    SetState(kStateDisabled);

    return error;
}

otError SubMac::Sleep(void)
{
    otError error = otPlatRadioSleep(&GetInstance());

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("otPlatRadioSleep() failed, error: %s", otThreadErrorToString(error));
        ExitNow();
    }

    SetState(kStateSleep);

exit:
    return error;
}

otError SubMac::Receive(uint8_t aChannel)
{
    otError error = otPlatRadioReceive(&GetInstance(), aChannel);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMac("otPlatRadioReceive() failed, error: %s", otThreadErrorToString(error));
        ExitNow();
    }

    SetState(kStateReceive);

exit:
    return error;
}

void SubMac::HandleReceiveDone(Frame *aFrame, otError aError)
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

#if OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
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

    backoff = Random::GetUint32InRange(0, static_cast<uint32_t>(1UL << backoffExponent));
    backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    if (mRxOnWhenBackoff)
    {
        otPlatRadioReceive(&GetInstance(), mTransmitFrame.GetChannel());
    }
    else
    {
        otPlatRadioSleep(&GetInstance());
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
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

#if OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
    if ((mTransmitRetries > 0) && (mTransmitRetries == mTransmitFrame.GetMaxFrameRetries()))
    {
        mTransmitFrame.SetCsmaCaEnabled(false);
    }
    else
#endif
    {
        mTransmitFrame.SetCsmaCaEnabled(true);
    }

    error = otPlatRadioReceive(&GetInstance(), mTransmitFrame.GetChannel());
    assert(error == OT_ERROR_NONE);

    error = otPlatRadioTransmit(&GetInstance(), &mTransmitFrame);
    assert(error == OT_ERROR_NONE);

    SetState(kStateTransmit);

    if (mPcapCallback)
    {
        mPcapCallback(&mTransmitFrame, true, mPcapCallbackContext);
    }

exit:
    return;
}

void SubMac::HandleTransmitStarted(Frame &aFrame)
{
    if (ShouldHandleAckTimeout() && aFrame.GetAckRequest())
    {
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        mTimer.Start(kAckTimeout * 1000UL);
#else
        mTimer.Start(kAckTimeout);
#endif
    }
}

void SubMac::HandleTransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError)
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
        StartCsmaBackoff();
        ExitNow();
    }

    mTransmitRetries = 0;

    SetState(kStateReceive);

    mCallbacks.TransmitDone(aFrame, aAckFrame, aError);

exit:
    return;
}

int8_t SubMac::GetRssi(void) const
{
    return otPlatRadioGetRssi(&GetInstance());
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
        otPlatRadioEnergyScan(&GetInstance(), aScanChannel, aScanDuration);
        SetState(kStateEnergyScan);
    }
    else if (ShouldHandleEnergyScan())
    {
        error = otPlatRadioReceive(&GetInstance(), aScanChannel);
        assert(error == OT_ERROR_NONE);

        SetState(kStateEnergyScan);
        mEnergyScanMaxRssi = kInvalidRssiValue;
        mEnergyScanEndTime = TimerMilli::GetNow() + aScanDuration;
        mTimer.Start(kEnergyScanRssiSampleInterval);
        SampleRssi();
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
    int8_t rssi = GetRssi();

    if (rssi != kInvalidRssiValue)
    {
        if ((mEnergyScanMaxRssi == kInvalidRssiValue) || (rssi > mEnergyScanMaxRssi))
        {
            mEnergyScanMaxRssi = rssi;
        }
    }

    if (TimerMilliScheduler::IsStrictlyBefore(TimerMilli::GetNow(), mEnergyScanEndTime))
    {
        mTimer.StartAt(mTimer.GetFireTime(), kEnergyScanRssiSampleInterval);
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
        otPlatRadioReceive(&GetInstance(), mTransmitFrame.GetChannel());
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

#if OPENTHREAD_ENABLE_RAW_LINK_API
    VerifyOrExit(GetInstance().GetLinkRaw().IsEnabled());
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API || OPENTHREAD_RADIO
    swCsma = OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF;
#endif

exit:
    return swCsma;
}

bool SubMac::ShouldHandleAckTimeout(void) const
{
    bool swAckTimeout = true;

    VerifyOrExit(!RadioSupportsAckTimeout(), swAckTimeout = false);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    VerifyOrExit(GetInstance().GetLinkRaw().IsEnabled());
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API || OPENTHREAD_RADIO
    swAckTimeout = OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT;
#endif

exit:
    return swAckTimeout;
}

bool SubMac::ShouldHandleRetries(void) const
{
    bool swRetries = true;

    VerifyOrExit(!RadioSupportsRetries(), swRetries = false);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    VerifyOrExit(GetInstance().GetLinkRaw().IsEnabled());
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API || OPENTHREAD_RADIO
    swRetries = OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT;
#endif

exit:
    return swRetries;
}

bool SubMac::ShouldHandleEnergyScan(void) const
{
    bool swEnergyScan = true;

    VerifyOrExit(!RadioSupportsEnergyScan(), swEnergyScan = false);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    VerifyOrExit(GetInstance().GetLinkRaw().IsEnabled());
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API || OPENTHREAD_RADIO
    swEnergyScan = OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN;
#endif

exit:
    return swEnergyScan;
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
void SubMac::HandleFrameUpdated(Frame &aFrame)
{
    mCallbacks.FrameUpdated(aFrame);
}
#endif

void SubMac::SetState(State aState)
{
    if (mState != aState)
    {
        otLogDebgMac("RadioState: %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
}

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

//---------------------------------------------------------------------------------------------------------------------
// otPlatRadio callbacks

extern "C" void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<SubMac>().HandleReceiveDone(static_cast<Frame *>(aFrame), aError);
    }
}

extern "C" void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<SubMac>().HandleTransmitStarted(*static_cast<Frame *>(aFrame));
    }
}

extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<SubMac>().HandleTransmitDone(*static_cast<Frame *>(aFrame), static_cast<Frame *>(aAckFrame),
                                                   aError);
    }
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<SubMac>().HandleEnergyScanDone(aEnergyScanMaxRssi);
    }
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
extern "C" void otPlatRadioFrameUpdated(otInstance *aInstance, otRadioFrame *aFrame)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<SubMac>().HandleFrameUpdated(*static_cast<Frame *>(aFrame));
    }
}
#endif

} // namespace Mac
} // namespace ot
