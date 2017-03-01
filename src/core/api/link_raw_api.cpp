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
 *   This file implements blacklist IEEE 802.15.4 frame filtering based on MAC address.
 */

#include <common/debug.hpp>
#include <common/logging.hpp>
#include <platform/random.h>
#include <platform/usec-alarm.h>
#include "openthread-instance.h"

#ifdef __cplusplus
extern "C" {
#endif

ThreadError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!aInstance->mThreadNetif.IsUp(), error = kThreadError_InvalidState);

    otLogInfoPlat("LinkRaw Enabled=%d", aEnabled ? 1 : 0);

    aInstance->mLinkRaw.SetEnabled(aEnabled);

exit:
    return error;
}

bool otLinkRawIsEnabled(otInstance *aInstance)
{
    return aInstance->mLinkRaw.IsEnabled();
}

ThreadError otLinkRawSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otPlatRadioSetPanId(aInstance, aPanId);

exit:
    return error;
}

ThreadError otLinkRawSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress)
{
    ThreadError error = kThreadError_None;
    uint8_t buf[sizeof(otExtAddress)];

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = aExtendedAddress->m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(aInstance, buf);

exit:
    return error;
}

ThreadError otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otPlatRadioSetShortAddress(aInstance, aShortAddress);

exit:
    return error;
}

bool otLinkRawGetPromiscuous(otInstance *aInstance)
{
    return otPlatRadioGetPromiscuous(aInstance);
}

ThreadError otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otLogInfoPlat("LinkRaw Promiscuous=%d", aEnabled ? 1 : 0);

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

ThreadError otLinkRawSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otLogDebgPlat("LinkRaw Sleep");

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

ThreadError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    otLogDebgPlat("LinkRaw Recv (Channel %d)", aChannel);
    return aInstance->mLinkRaw.Receive(aChannel, aCallback);
}

RadioPacket *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    RadioPacket *buffer = NULL;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(),);

    buffer = otPlatRadioGetTransmitBuffer(aInstance);

exit:
    return buffer;
}

ThreadError otLinkRawTransmit(otInstance *aInstance, RadioPacket *aPacket, otLinkRawTransmitDone aCallback)
{
    otLogDebgPlat("LinkRaw Transmit (%d bytes)", aPacket->mLength);
    return aInstance->mLinkRaw.Transmit(aPacket, aCallback);
}

int8_t otLinkRawGetRssi(otInstance *aInstance)
{
    return otPlatRadioGetRssi(aInstance);
}

otRadioCaps otLinkRawGetCaps(otInstance *aInstance)
{
    return aInstance->mLinkRaw.GetCaps();
}

ThreadError otLinkRawEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration,
                                otLinkRawEnergyScanDone aCallback)
{
    return aInstance->mLinkRaw.EnergyScan(aScanChannel, aScanDuration, aCallback);
}

ThreadError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otPlatRadioEnableSrcMatch(aInstance, aEnable);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    error = otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    error = otPlatRadioAddSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    error = otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    error = otPlatRadioClearSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otPlatRadioClearSrcMatchShortEntries(aInstance);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    otPlatRadioClearSrcMatchExtEntries(aInstance);

exit:
    return error;
}

#ifdef __cplusplus
}  // extern "C"
#endif

namespace Thread {

LinkRaw::LinkRaw(otInstance &aInstance):
    mInstance(aInstance),
    mEnabled(false),
    mReceiveDoneCallback(NULL),
    mTransmitDoneCallback(NULL),
    mEnergyScanDoneCallback(NULL)
#if OPENTHREAD_LINKRAW_TIMER_REQUIRED
    , mTimer(aInstance.mIp6.mTimerScheduler, &LinkRaw::HandleTimer, this)
    , mTimerReason(kTimerReasonNone)
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
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
    assert((RadioCaps & kRadioCapsAckTimeout) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | kRadioCapsAckTimeout);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    assert((RadioCaps & kRadioCapsTransmitRetries) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | kRadioCapsTransmitRetries);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    assert((RadioCaps & kRadioCapsEnergyScan) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | kRadioCapsEnergyScan);
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    return RadioCaps;
}

ThreadError LinkRaw::Receive(uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    ThreadError error = kThreadError_InvalidState;

    if (mEnabled)
    {
#if OPENTHREAD_LINKRAW_TIMER_REQUIRED
        // Only need to cache if we implement timer logic that might
        // revert the channel on completion.
        mReceiveChannel = aChannel;
#endif

        mReceiveDoneCallback = aCallback;
        error = otPlatRadioReceive(&mInstance, aChannel);
    }

    return error;
}

void LinkRaw::InvokeReceiveDone(RadioPacket *aPacket, ThreadError aError)
{
    if (mReceiveDoneCallback)
    {
        mReceiveDoneCallback(&mInstance, aPacket, aError);
    }
}

ThreadError LinkRaw::Transmit(RadioPacket *aPacket, otLinkRawTransmitDone aCallback)
{
    ThreadError error = kThreadError_InvalidState;

    if (mEnabled)
    {
        mTransmitDoneCallback = aCallback;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
        (void)aPacket;
        mTransmitAttempts = 0;
        mCsmaAttempts = 0;

        // Start the transmission backlog logic
        StartCsmaBackoff();
        error = kThreadError_None;
#else
        // Let the hardware do the transmission logic
        error = DoTransmit(aPacket);
#endif
    }

    return error;
}

ThreadError LinkRaw::DoTransmit(RadioPacket *aPacket)
{
    ThreadError error = otPlatRadioTransmit(&mInstance, aPacket);

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    // If we are implementing the ACK timeout logic, start a timer here (if ACK request)
    // to fire if we don't get a transmit done callback in time.
    if (static_cast<Mac::Frame *>(aPacket)->GetAckRequest())
    {
        otLogDebgPlat("LinkRaw Starting AckTimeout Timer");
        mTimerReason = kTimerReasonAckTimeout;
        mTimer.Start(kAckTimeout);
    }

#endif

    return error;
}

void LinkRaw::InvokeTransmitDone(RadioPacket *aPacket, bool aFramePending, ThreadError aError)
{
    otLogDebgPlat("LinkRaw Transmit Done (err=0x%x)", aError);

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    mTimer.Stop();
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    if (aError == kThreadError_ChannelAccessFailure)
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

    if (aError == kThreadError_NoAck)
    {
        if (mTransmitAttempts < Mac::kMaxFrameAttempts)
        {
            mTransmitAttempts++;
            StartCsmaBackoff();
            goto exit;
        }
    }

#endif

    if (mTransmitDoneCallback)
    {
        otLogDebgPlat("LinkRaw Invoke Transmit Done");
        mTransmitDoneCallback(&mInstance, aPacket, aFramePending, aError);
        mTransmitDoneCallback = NULL;
    }

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
exit:
    return;
#endif
}

ThreadError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    ThreadError error = kThreadError_InvalidState;

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
        InvokeTransmitDone(otPlatRadioGetTransmitBuffer(&mInstance), false, kThreadError_NoAck);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    case kTimerReasonRetransmitTimeout:
    {
        RadioPacket *aPacket = otPlatRadioGetTransmitBuffer(&mInstance);

        // Start the  transmit now
        ThreadError error = DoTransmit(aPacket);

        if (error != kThreadError_None)
        {
            InvokeTransmitDone(aPacket, false, error);
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
    backoff *= (Mac::kUnitBackoffPeriod * kPhyUsPerSymbol);

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_BACKOFF_TIMER
    otPlatUsecAlarmTime now;
    otPlatUsecAlarmTime delay;

    otPlatUsecAlarmGetNow(&now);
    delay.mMs = backoff / 1000UL;
    delay.mUs = backoff - (delay.mMs * 1000UL);

    otLogDebgPlat("LinkRaw Starting RetransmitTimeout Timer (%d ms)", backoff);
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

} // namespace Thread
