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
#include "openthread-instance.h"

#ifdef __cplusplus
extern "C" {
#endif

ThreadError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!aInstance->mThreadNetif.IsUp(), error = kThreadError_InvalidState);

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

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

ThreadError otLinkRawSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRaw.IsEnabled(), error = kThreadError_InvalidState);

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

ThreadError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
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
#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT
    , mAckTimer(aInstance.mIp6.mTimerScheduler, &LinkRaw::HandleAckTimer, this)
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
#endif // OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT
{
    // Query the capabilities to check asserts
    (void)GetCaps();
}

otRadioCaps LinkRaw::GetCaps()
{
    otRadioCaps RadioCaps = otPlatRadioGetCaps(&mInstance);

#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT
    // The radio shouldn't support this capability if it is being compile
    // time included into the raw link-layer code.
    assert((RadioCaps & kRadioCapsAckTimeout) == 0);
    RadioCaps = (otRadioCaps)(RadioCaps | kRadioCapsAckTimeout);
#endif // OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT

    return RadioCaps;
}

ThreadError LinkRaw::Receive(uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    ThreadError error = kThreadError_InvalidState;

    if (mEnabled)
    {
#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT
        // Only need to cache if we implement ACK timeout logic, so
        // we can revert back to receive internally on timeout.
        mReceiveChannel = aChannel;
#endif

        mReceiveDoneCallback = aCallback;
        error = otPlatRadioReceive(&mInstance, aChannel);
    }

    return error;
}

void LinkRaw::InvokeRawReceiveDone(RadioPacket *aPacket, ThreadError aError)
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
        error = otPlatRadioTransmit(&mInstance, aPacket);

#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT

        // If we are implementing the ACK timeout logic, start a timer here (if ACK request)
        // to fire if we don't get a transmit done callback in time.
        if (static_cast<Mac::Frame *>(otPlatRadioGetTransmitBuffer(&mInstance))->GetAckRequest())
        {
            mAckTimer.Start(kAckTimeout);
        }

#endif
    }

    return error;
}

void LinkRaw::InvokeRawTransmitDone(RadioPacket *aPacket, bool aFramePending, ThreadError aError)
{
#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT
    mAckTimer.Stop();
#endif

    if (mTransmitDoneCallback)
    {
        mTransmitDoneCallback(&mInstance, aPacket, aFramePending, aError);
        mTransmitDoneCallback = NULL;
    }
}

ThreadError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    ThreadError error = kThreadError_InvalidState;

    if (mEnabled)
    {
        mEnergyScanDoneCallback = aCallback;
        error = otPlatRadioEnergyScan(&mInstance, aScanChannel, aScanDuration);
    }

    return error;
}

void LinkRaw::InvokeRawEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (mEnergyScanDoneCallback)
    {
        mEnergyScanDoneCallback(&mInstance, aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = NULL;
    }
}

#if OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT

void LinkRaw::HandleAckTimer(void *aContext)
{
    static_cast<LinkRaw *>(aContext)->HandleAckTimer();
}

void LinkRaw::HandleAckTimer(void)
{
    // Transition back to receive state
    otPlatRadioReceive(&mInstance, mReceiveChannel);

    // Invoke completion callback for transmit
    InvokeRawTransmitDone(otPlatRadioGetTransmitBuffer(&mInstance), false, kThreadError_NoAck);
}

#endif // OPENTHREAD_ENABLE_SOFTWARE_ACK_TIMEOUT

} // namespace Thread
