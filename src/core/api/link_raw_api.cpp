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

void otLinkRawSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    otPlatRadioSetPanId(aInstance, aPanId);
}

void otLinkRawSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress)
{
    uint8_t buf[sizeof(otExtAddress)];

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        buf[i] = aExtendedAddress->m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(aInstance, buf);
}

void otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    otPlatRadioSetShortAddress(aInstance, aShortAddress);
}

bool otLinkRawGetPromiscuous(otInstance *aInstance)
{
    return otPlatRadioGetPromiscuous(aInstance);
}

void otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    otPlatRadioSetPromiscuous(aInstance, aEnable);
}

ThreadError otLinkRawSleep(otInstance *aInstance)
{
    return otPlatRadioSleep(aInstance);
}

ThreadError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback)
{
    aInstance->mLinkRawReceiveDoneCallback = aCallback;

    return otPlatRadioReceive(aInstance, aChannel);
}

RadioPacket *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    return otPlatRadioGetTransmitBuffer(aInstance);
}

ThreadError otLinkRawTransmit(otInstance *aInstance, RadioPacket *aPacket, otLinkRawTransmitDone aCallback)
{
    aInstance->mLinkRawTransmitDoneCallback = aCallback;

    return otPlatRadioTransmit(aInstance, aPacket);
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
    aInstance->mLinkRawEnergyScanDoneCallback = aCallback;

    return otPlatRadioEnergyScan(aInstance, aScanChannel, aScanDuration);
}

void otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    otPlatRadioEnableSrcMatch(aInstance, aEnable);
}

ThreadError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    return otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);
}

ThreadError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    return otPlatRadioAddSrcMatchExtEntry(aInstance, aExtAddress);
}

ThreadError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    return otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);
}

ThreadError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    return otPlatRadioClearSrcMatchExtEntry(aInstance, aExtAddress);
}

void otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    otPlatRadioClearSrcMatchShortEntries(aInstance);
}

void otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    otPlatRadioClearSrcMatchExtEntries(aInstance);
}
