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

#include "openthread-instance.h"

#ifdef __cplusplus
extern "C" {
#endif

ThreadError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!aInstance->mThreadNetif.IsUp(), error = kThreadError_InvalidState);

    aInstance->mLinkRawEnabled = aEnabled;

exit:
    return error;
}

bool otLinkRawIsEnabled(otInstance *aInstance)
{
    return aInstance->mLinkRawEnabled;
}

ThreadError otLinkRawSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    otPlatRadioSetPanId(aInstance, aPanId);

exit:
    return error;
}

ThreadError otLinkRawSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress)
{
    ThreadError error = kThreadError_None;
    uint8_t buf[sizeof(otExtAddress)];

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

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

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

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

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

ThreadError otLinkRawSleep(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

ThreadError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    aInstance->mLinkRawReceiveDoneCallback = aCallback;

    error = otPlatRadioReceive(aInstance, aChannel);

exit:
    return error;
}

RadioPacket *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    RadioPacket *buffer = NULL;

    VerifyOrExit(aInstance->mLinkRawEnabled,);

    buffer = otPlatRadioGetTransmitBuffer(aInstance);

exit:
    return buffer;
}

ThreadError otLinkRawTransmit(otInstance *aInstance, RadioPacket *aPacket, otLinkRawTransmitDone aCallback)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    aInstance->mLinkRawTransmitDoneCallback = aCallback;

    error = otPlatRadioTransmit(aInstance, aPacket);

exit:
    return error;
}

int8_t otLinkRawGetRssi(otInstance *aInstance)
{
    return otPlatRadioGetRssi(aInstance);
}

otRadioCaps otLinkRawGetCaps(otInstance *aInstance)
{
    return otPlatRadioGetCaps(aInstance);
}

ThreadError otLinkRawEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration,
                                otLinkRawEnergyScanDone aCallback)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    aInstance->mLinkRawEnergyScanDoneCallback = aCallback;

    error = otPlatRadioEnergyScan(aInstance, aScanChannel, aScanDuration);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    otPlatRadioEnableSrcMatch(aInstance, aEnable);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    error = otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    error = otPlatRadioAddSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    error = otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    error = otPlatRadioClearSrcMatchExtEntry(aInstance, aExtAddress);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    otPlatRadioClearSrcMatchShortEntries(aInstance);

exit:
    return error;
}

ThreadError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aInstance->mLinkRawEnabled, error = kThreadError_InvalidState);

    otPlatRadioClearSrcMatchExtEntries(aInstance);

exit:
    return error;
}

#ifdef __cplusplus
}  // extern "C"
#endif
