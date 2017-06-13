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
 *   This file implements the OpenThread Link Raw API.
 */

#include <openthread/config.h>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <openthread/platform/random.h>
#include <openthread/platform/usec-alarm.h>

#include "openthread-instance.h"

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aInstance->mThreadNetif.IsUp(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(aInstance, "LinkRaw Enabled=%d", aEnabled ? 1 : 0);

    aInstance->mLinkRaw.SetEnabled(aEnabled);

exit:
    return error;
}

bool otLinkRawIsEnabled(otInstance *aInstance)
{
    return aInstance->mLinkRaw.IsEnabled();
}

otError otLinkRawSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetPanId(aInstance, aPanId);

exit:
    return error;
}

otError otLinkRawSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress)
{
    otError error = OT_ERROR_NONE;
    uint8_t buf[sizeof(otExtAddress)];

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = aExtendedAddress->m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(aInstance, buf);

exit:
    return error;
}

otError otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetShortAddress(aInstance, aShortAddress);

exit:
    return error;
}

bool otLinkRawGetPromiscuous(otInstance *aInstance)
{
    return otPlatRadioGetPromiscuous(aInstance);
}

otError otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(aInstance, "LinkRaw Promiscuous=%d", aEnable ? 1 : 0);

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(aInstance, "LinkRaw Sleep");

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

otError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    otLogInfoPlat(aInstance, "LinkRaw Recv (Channel %d)", aChannel);
    return aInstance->mLinkRaw.Receive(aChannel, aCallback);
}

otRadioFrame *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    otRadioFrame *buffer = NULL;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled());

    buffer = otPlatRadioGetTransmitBuffer(aInstance);

exit:
    return buffer;
}

otError otLinkRawTransmit(otInstance *aInstance, otRadioFrame *aFrame, otLinkRawTransmitDone aCallback)
{
    otLogInfoPlat(aInstance, "LinkRaw Transmit (%d bytes on channel %d)", aFrame->mLength, aFrame->mChannel);
    return aInstance->mLinkRaw.Transmit(aFrame, aCallback);
}

int8_t otLinkRawGetRssi(otInstance *aInstance)
{
    return otPlatRadioGetRssi(aInstance);
}

otRadioCaps otLinkRawGetCaps(otInstance *aInstance)
{
    return aInstance->mLinkRaw.GetCaps();
}

otError otLinkRawEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration,
                            otLinkRawEnergyScanDone aCallback)
{
    return aInstance->mLinkRaw.EnergyScan(aScanChannel, aScanDuration, aCallback);
}

otError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioEnableSrcMatch(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioAddSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioClearSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchShortEntries(aInstance);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchExtEntries(aInstance);

exit:
    return error;
}

namespace ot {

LinkRaw::LinkRaw(otInstance &aInstance):
    mInstance(aInstance),
    mEnabled(false),
    mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL),
    mReceiveDoneCallback(NULL),
    mTransmitDoneCallback(NULL),
    mEnergyScanDoneCallback(NULL)
#if OPENTHREAD_LINKRAW_TIMER_REQUIRED
    , mTimer(aInstance.mIp6.mTimerScheduler, &LinkRaw::HandleTimer, this)
    , mTimerReason(kTimerReasonNone)
#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    , mEnergyScanTask(aInstance.mIp6.mTaskletScheduler, &LinkRaw::HandleEnergyScanTask, this)
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
{
    // Query the capabilities to check asserts
    (void)GetCaps();
}

otRadioCaps LinkRaw::GetCaps()
{
    otRadioCaps RadioCaps = otPlatRadioGetCaps(&mInstance);

    // The radio shouldn't support a capability if it is being compile
    // time included into the raw link-layer code.

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    assert((RadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | OT_RADIO_CAPS_ACK_TIMEOUT);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    assert((RadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | OT_RADIO_CAPS_TRANSMIT_RETRIES);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    assert((RadioCaps & OT_RADIO_CAPS_ENERGY_SCAN) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | OT_RADIO_CAPS_ENERGY_SCAN);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    return RadioCaps;
}

otError LinkRaw::Receive(uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mReceiveChannel = aChannel;
        mReceiveDoneCallback = aCallback;
        error = otPlatRadioReceive(&mInstance, aChannel);
    }

    return error;
}

void LinkRaw::InvokeReceiveDone(otRadioFrame *aFrame, otError aError)
{
    if (mReceiveDoneCallback)
    {
        if (aError == OT_ERROR_NONE)
        {
            otLogInfoPlat(&mInstance, "LinkRaw Invoke Receive Done (%d bytes)", aFrame->mLength);
        }
        else
        {
            otLogWarnPlat(&mInstance, "LinkRaw Invoke Receive Done (err=0x%x)", aError);
        }

        mReceiveDoneCallback(&mInstance, aFrame, aError);
    }
}

otError LinkRaw::Transmit(otRadioFrame *aFrame, otLinkRawTransmitDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mTransmitDoneCallback = aCallback;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
        (void)aFrame;
        mTransmitAttempts = 0;
        mCsmaAttempts = 0;

        // Start the transmission backlog logic
        StartCsmaBackoff();
        error = OT_ERROR_NONE;
#else
        // Let the hardware do the transmission logic
        error = DoTransmit(aFrame);
#endif
    }

    return error;
}

otError LinkRaw::DoTransmit(otRadioFrame *aFrame)
{
    otError error = otPlatRadioTransmit(&mInstance, aFrame);

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    // If we are implementing the ACK timeout logic, start a timer here (if ACK request)
    // to fire if we don't get a transmit done callback in time.
    if (static_cast<Mac::Frame *>(aFrame)->GetAckRequest())
    {
        otLogDebgPlat(aInstance, "LinkRaw Starting AckTimeout Timer");
        mTimerReason = kTimerReasonAckTimeout;
        mTimer.Start(Mac::kAckTimeout);
    }

#endif

    return error;
}

void LinkRaw::InvokeTransmitDone(otRadioFrame *aFrame, bool aFramePending, otError aError)
{
    otLogDebgPlat(aInstance, "LinkRaw Transmit Done (err=0x%x)", aError);

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    mTimer.Stop();
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
    {
        if (mCsmaAttempts < Mac::kMaxCSMABackoffs)
        {
            mCsmaAttempts++;
            StartCsmaBackoff();
            goto exit;
        }
    }
    else
    {
        mCsmaAttempts = 0;
    }

    if (aError == OT_ERROR_NO_ACK)
    {
        if (mTransmitAttempts < aFrame->mMaxTxAttempts)
        {
            mTransmitAttempts++;
            StartCsmaBackoff();
            goto exit;
        }
    }

#endif

    // Transition back to receive state on previous channel
    otPlatRadioReceive(&mInstance, mReceiveChannel);

    if (mTransmitDoneCallback)
    {
        if (aError == OT_ERROR_NONE)
        {
            otLogInfoPlat(aInstance, "LinkRaw Invoke Transmit Done");
        }
        else
        {
            otLogWarnPlat(aInstance, "LinkRaw Invoke Transmit Failed (err=0x%x)", aError);
        }

        mTransmitDoneCallback(&mInstance, aFrame, aFramePending, aError);
        mTransmitDoneCallback = NULL;
    }

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
exit:
    return;
#endif
}

otError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mEnergyScanDoneCallback = aCallback;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
        // Start listening on the scan channel
        otPlatRadioReceive(&mInstance, aScanChannel);

        // Reset the RSSI value and start scanning
        mEnergyScanRssi = kInvalidRssiValue;
        mTimerReason = kTimerReasonEnergyScanComplete;
        mTimer.Start(aScanDuration);
        mEnergyScanTask.Post();
#else
        // Do the HW offloaded energy scan
        error = otPlatRadioEnergyScan(&mInstance, aScanChannel, aScanDuration);
#endif
    }

    return error;
}

void LinkRaw::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (mEnergyScanDoneCallback)
    {
        mEnergyScanDoneCallback(&mInstance, aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = NULL;
    }
}

#if OPENTHREAD_LINKRAW_TIMER_REQUIRED

void LinkRaw::HandleTimer(void *aContext)
{
    static_cast<LinkRaw *>(aContext)->HandleTimer();
}

void LinkRaw::HandleTimer(void)
{
    TimerReason timerReason = mTimerReason;
    mTimerReason = kTimerReasonNone;

    switch (timerReason)
    {
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    case kTimerReasonAckTimeout:
    {
        // Transition back to receive state on previous channel
        otPlatRadioReceive(&mInstance, mReceiveChannel);

        // Invoke completion callback for transmit
        InvokeTransmitDone(otPlatRadioGetTransmitBuffer(&mInstance), false, OT_ERROR_NO_ACK);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    case kTimerReasonRetransmitTimeout:
    {
        otRadioFrame *aFrame = otPlatRadioGetTransmitBuffer(&mInstance);

        // Start the  transmit now
        otError error = DoTransmit(aFrame);

        if (error != OT_ERROR_NONE)
        {
            InvokeTransmitDone(aFrame, false, error);
        }

        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

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
    }
}

#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

void LinkRaw::StartCsmaBackoff(void)
{
    uint32_t backoffExponent = Mac::kMinBE + mTransmitAttempts + mCsmaAttempts;
    uint32_t backoff;

    if (backoffExponent > Mac::kMaxBE)
    {
        backoffExponent = Mac::kMaxBE;
    }

    backoff = (otPlatRandomGet() % (1UL << backoffExponent));
    backoff *= (static_cast<uint32_t>(Mac::kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_BACKOFF_TIMER
    otPlatUsecAlarmTime now;
    otPlatUsecAlarmTime delay;

    otPlatUsecAlarmGetNow(&now);
    delay.mMs = backoff / 1000UL;
    delay.mUs = backoff - (delay.mMs * 1000UL);

    otLogDebgPlat(aInstance, "LinkRaw Starting RetransmitTimeout Timer (%d ms)", backoff);
    mTimerReason = kTimerReasonRetransmitTimeout;
    otPlatUsecAlarmStartAt(&mInstance, &now, &delay, &HandleTimer, this);
#else // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_BACKOFF_TIMER
    mTimerReason = kTimerReasonRetransmitTimeout;
    mTimer.Start(backoff / 1000UL);
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_BACKOFF_TIMER

}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

void LinkRaw::HandleEnergyScanTask(void *aContext)
{
    static_cast<LinkRaw *>(aContext)->HandleEnergyScanTask();
}

void LinkRaw::HandleEnergyScanTask(void)
{
    // Only process task if we are still energy scanning
    if (mTimerReason == kTimerReasonEnergyScanComplete)
    {
        int8_t rssi = otPlatRadioGetRssi(&mInstance);

        // Only apply the RSSI if it was a valid value
        if (rssi != kInvalidRssiValue)
        {
            if ((mEnergyScanRssi == kInvalidRssiValue) || (rssi > mEnergyScanRssi))
            {
                mEnergyScanRssi = rssi;
            }
        }

        // Post another instance of tha task, since we are
        // still doing the energy scan.
        mEnergyScanTask.Post();
    }
}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

} // namespace ot

#endif // OPENTHREAD_ENABLE_RAW_LINK_API
