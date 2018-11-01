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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#include <string.h>
#include <openthread/diag.h>
#include <openthread/platform/diag.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "mac/mac.hpp"
#include "utils/parse_cmdline.hpp"

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

namespace ot {

namespace Mac {

LinkRaw::LinkRaw(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mOperationTask(aInstance, &LinkRaw::HandleOperationTask, this)
    , mPendingTransmitData(false)
#if OPENTHREAD_LINKRAW_TIMER_REQUIRED
    , mTimer(aInstance, &LinkRaw::HandleTimer, this)
    , mTimerReason(kTimerReasonNone)
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    , mTimerMicro(aInstance, &LinkRaw::HandleTimer, this)
#else
    , mEnergyScanTimer(aInstance, &LinkRaw::HandleTimer, this)
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    , mCsmaBackoffs(0)
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    , mTransmitRetries(0)
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mReceiveDoneCallback(NULL)
    , mTransmitDoneCallback(NULL)
    , mEnergyScanDoneCallback(NULL)
    , mTransmitFrame(static_cast<Frame *>(otPlatRadioGetTransmitBuffer(&aInstance)))
    , mRadioCaps(otPlatRadioGetCaps(&aInstance))
{
}

void LinkRaw::HandleOperationTask(Tasklet &aTasklet)
{
    aTasklet.GetOwner<LinkRaw>().HandleOperationTask();
}

void LinkRaw::HandleOperationTask(void)
{
    if (mPendingTransmitData)
    {
        mPendingTransmitData = false;
        TransmitNow();
    }
}

otError LinkRaw::SetEnabled(bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    otLogInfoPlat("LinkRaw Enabled=%d", aEnabled ? 1 : 0);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    VerifyOrExit(!GetInstance().GetThreadNetif().IsUp(), error = OT_ERROR_INVALID_STATE);
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    if (aEnabled)
    {
        otPlatRadioEnable(&GetInstance());
    }
    else
    {
        otPlatRadioDisable(&GetInstance());
    }

    mEnabled = aEnabled;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
exit:
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    return error;
}

otError LinkRaw::SetPanId(uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetPanId(&GetInstance(), aPanId);
    mPanId = aPanId;

exit:
    return error;
}

otError LinkRaw::SetChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    mReceiveChannel = aChannel;

exit:
    return error;
}

otError LinkRaw::SetExtAddress(const otExtAddress &aExtAddress)
{
    otExtAddress addr;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress.m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(&GetInstance(), &addr);
    mExtAddress = aExtAddress;

exit:
    return error;
}

otError LinkRaw::SetShortAddress(uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetShortAddress(&GetInstance(), aShortAddress);
    mShortAddress = aShortAddress;

exit:
    return error;
}

otRadioCaps LinkRaw::GetCaps(void) const
{
    otRadioCaps radioCaps = mRadioCaps;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    radioCaps |= OT_RADIO_CAPS_ACK_TIMEOUT;
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    radioCaps |= OT_RADIO_CAPS_TRANSMIT_RETRIES;
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    radioCaps |= OT_RADIO_CAPS_CSMA_BACKOFF;
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    radioCaps |= OT_RADIO_CAPS_ENERGY_SCAN;
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    return radioCaps;
}

otError LinkRaw::Receive(otLinkRawReceiveDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mReceiveDoneCallback = aCallback;
        error                = otPlatRadioReceive(&GetInstance(), mReceiveChannel);
    }

    return error;
}

void LinkRaw::InvokeReceiveDone(otRadioFrame *aFrame, otError aError)
{
    if (mReceiveDoneCallback)
    {
        if (aError == OT_ERROR_NONE)
        {
            otLogInfoPlat("LinkRaw Invoke Receive Done (%d bytes)", aFrame->mLength);
            mReceiveDoneCallback(&GetInstance(), aFrame, aError);
        }
        else
        {
            otLogWarnPlat("LinkRaw Invoke Receive Done (err=0x%x)", aError);
        }
    }
}

void LinkRaw::StartTransmit(void)
{
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    if ((mRadioCaps & OT_RADIO_CAPS_CSMA_BACKOFF) == 0 && mTransmitFrame->IsCsmaCaEnabled())
    {
        StartCsmaBackoff();
    }
    else
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    {
        mPendingTransmitData = true;
        mOperationTask.Post();
    }
}

void LinkRaw::TransmitNow(void)
{
    otError error = otPlatRadioTransmit(&GetInstance(), mTransmitFrame);

    if (error != OT_ERROR_NONE)
    {
        InvokeTransmitDone(mTransmitFrame, NULL, error);
    }
}

otError LinkRaw::Transmit(otLinkRawTransmitDone aCallback)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);

    mTransmitDoneCallback = aCallback;

    mTransmitFrame->SetCsmaCaEnabled(true);
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    if ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) == 0)
    {
        mCsmaBackoffs = 0;
    }
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    if ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) == 0)
    {
        mTransmitRetries = 0;
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    StartTransmit();

exit:
    return error;
}

void LinkRaw::InvokeTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    assert(aFrame == mTransmitFrame);

    if (aError == OT_ERROR_NONE)
    {
        otLogDebgPlat("LinkRaw Transmit Done: %s", otThreadErrorToString(aError));
    }
    else
    {
        otLogWarnPlat("LinkRaw Transmit Done: %s", otThreadErrorToString(aError));
    }

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    if ((mRadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) == 0)
    {
        mTimer.Stop();
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
    if ((mRadioCaps & OT_RADIO_CAPS_CSMA_BACKOFF) == 0)
    {
        if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
        {
            mCsmaBackoffs++;

            if (mCsmaBackoffs < aFrame->mInfo.mTxInfo.mMaxCsmaBackoffs)
            {
                ExitNow(StartTransmit());
            }
        }
        else
        {
            mCsmaBackoffs = 0;
        }
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    if ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) == 0)
    {
        if (aError != OT_ERROR_NONE)
        {
            if (mTransmitRetries < aFrame->mInfo.mTxInfo.mMaxFrameRetries)
            {
                mTransmitRetries++;
#if OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
                if ((mTransmitFrame->GetMaxFrameRetries() > 0) &&
                    (mTransmitFrame->GetMaxFrameRetries() <= mTransmitRetries))
                {
                    mTransmitFrame->SetCsmaCaEnabled(false);
                }
#endif
                ExitNow(StartTransmit());
            }
        }
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    // Transition back to receive state on previous channel
    otPlatRadioReceive(&GetInstance(), mReceiveChannel);

    VerifyOrExit(mTransmitDoneCallback != NULL);

    switch (aError)
    {
    case OT_ERROR_NONE:
    case OT_ERROR_NO_ACK:
    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:
        break;
    default:
        aError = OT_ERROR_ABORT;
        break;
    }

    mTransmitDoneCallback(&GetInstance(), aFrame, aAckFrame, aError);

exit:
    return;
}

otError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mEnergyScanDoneCallback = aCallback;

        if (otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_ENERGY_SCAN)
        {
            // Do the HW offloaded energy scan
            error = otPlatRadioEnergyScan(&GetInstance(), aScanChannel, aScanDuration);
        }
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
        else
        {
            // Start listening on the scan channel
            otPlatRadioReceive(&GetInstance(), aScanChannel);

            // Reset the RSSI value and start scanning
            mEnergyScanRssi = kInvalidRssiValue;
            mTimerReason    = kTimerReasonEnergyScanComplete;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
            mTimerMicro.Start(0);
#else
            mEnergyScanTimer.Start(0);
#endif
            mTimer.Start(aScanDuration);
        }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    }

    return error;
}

void LinkRaw::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (IsEnabled() && mEnergyScanDoneCallback)
    {
        mEnergyScanDoneCallback(&GetInstance(), aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = NULL;
    }
}

void LinkRaw::TransmitStarted(otRadioFrame *aFrame)
{
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    // If we are implementing the ACK timeout logic, start a timer here (if ACK request)
    // to fire if we don't get a transmit done callback in time.
    if ((mRadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) == 0 && static_cast<Frame *>(aFrame)->GetAckRequest())
    {
        otLogDebgPlat("LinkRaw Starting AckTimeout Timer");
        mTimerReason = kTimerReasonAckTimeout;
        mTimer.Start(kAckTimeout);
    }
#else
    OT_UNUSED_VARIABLE(aFrame);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
}

#if OPENTHREAD_LINKRAW_TIMER_REQUIRED

void LinkRaw::HandleTimer(Timer &aTimer)
{
    LinkRaw &linkRaw = aTimer.GetOwner<LinkRaw>();

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    // Energy scan uses a different timer for adding delay between RSSI samples.
    if (&aTimer != &linkRaw.mTimer && linkRaw.mTimerReason == kTimerReasonEnergyScanComplete)
    {
        linkRaw.HandleEnergyScanTimer();
    }
    else
#endif
    {
        linkRaw.HandleTimer();
    }
}

void LinkRaw::HandleTimer(void)
{
    TimerReason timerReason = mTimerReason;
    mTimerReason            = kTimerReasonNone;

    switch (timerReason)
    {
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    case kTimerReasonAckTimeout:
    {
        // Transition back to receive state on previous channel
        otPlatRadioReceive(&GetInstance(), mReceiveChannel);

        // Invoke completion callback for transmit
        InvokeTransmitDone(mTransmitFrame, NULL, OT_ERROR_NO_ACK);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

    case kTimerReasonCsmaBackoffComplete:
    {
        TransmitNow();
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    case kTimerReasonEnergyScanComplete:
    {
        // Invoke completion callback for the energy scan
        InvokeEnergyScanDone(mEnergyScanRssi);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    default:
        assert(false);
        break;
    }
}

#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

void LinkRaw::StartCsmaBackoff(void)
{
    uint32_t backoffExponent = kMinBE + mTransmitRetries + mCsmaBackoffs;
    uint32_t backoff;

    if (backoffExponent > kMaxBE)
    {
        backoffExponent = kMaxBE;
    }

    backoff = Random::GetUint32InRange(0, 1U << backoffExponent);
    backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    otLogDebgPlat("LinkRaw Starting RetransmitTimeout Timer (%d ms)", backoff);
    mTimerReason = kTimerReasonCsmaBackoffComplete;

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mTimerMicro.Start(backoff);
#else  // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mTimer.Start(backoff / 1000UL);
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

void LinkRaw::HandleEnergyScanTimer(void)
{
    // Only process if we are still energy scanning
    if (mTimer.IsRunning() && mTimerReason == kTimerReasonEnergyScanComplete)
    {
        int8_t rssi = otPlatRadioGetRssi(&GetInstance());

        // Only apply the RSSI if it was a valid value
        if (rssi != kInvalidRssiValue)
        {
            if ((mEnergyScanRssi == kInvalidRssiValue) || (rssi > mEnergyScanRssi))
            {
                mEnergyScanRssi = rssi;
            }
        }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        mTimerMicro.Start(kEnergyScanRssiSampleInterval);
#else
        mEnergyScanTimer.Start(kEnergyScanRssiSampleInterval);
#endif
    }
}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

} // namespace Mac

} // namespace ot

#if OPENTHREAD_RADIO
extern "C" void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    static_cast<ot::Instance *>(aInstance)->GetLinkRaw().InvokeReceiveDone(aFrame, aError);
}

extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    static_cast<ot::Instance *>(aInstance)->GetLinkRaw().InvokeTransmitDone(aFrame, aAckFrame, aError);
}

extern "C" void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    static_cast<ot::Instance *>(aInstance)->GetLinkRaw().TransmitStarted(aFrame);
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    VerifyOrExit(otInstanceIsInitialized(aInstance));

    static_cast<ot::Instance *>(aInstance)->GetLinkRaw().InvokeEnergyScanDone(aEnergyScanMaxRssi);

exit:
    return;
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
extern "C" void otPlatRadioFrameUpdated(otInstance *aInstance, otRadioFrame *aFrame)
{
    // Note: For now this functionality is not supported in Radio Only mode.
    (void)aInstance;
    (void)aFrame;
}
#endif

#endif // OPENTHREAD_RADIO

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
