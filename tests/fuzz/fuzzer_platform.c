/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include <string.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>
#include <openthread/platform/settings.h>
#include <openthread/platform/uart.h>

static uint32_t sRandomState = 1;

void FuzzerPlatformInit(void)
{
    sRandomState = 1;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return 0;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;
    (void)aT0;
    (void)aDt;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    (void)aInstance;
}

uint32_t otPlatAlarmMicroGetNow(void)
{
    return 0;
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;
    (void)aT0;
    (void)aDt;
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    (void)aInstance;
}

bool otDiagIsEnabled(void)
{
    return false;
}

void otDiagProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    (void)aArgCount;
    (void)aArgVector;
    (void)aOutput;
    (void)aOutputMaxLen;
}

void otDiagProcessCmdLine(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    (void)aString;
    (void)aOutput;
    (void)aOutputMaxLen;
}

void otPlatReset(otInstance *aInstance)
{
    (void)aInstance;
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    (void)aInstance;
    return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    (void)aLogLevel;
    (void)aLogRegion;
    (void)aFormat;
}

void otPlatWakeHost(void)
{
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void)aInstance;
    (void)aIeeeEui64;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    (void)aInstance;
    (void)aPanId;
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddr)
{
    (void)aInstance;
    (void)aExtAddr;
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddr)
{
    (void)aInstance;
    (void)aShortAddr;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnabled)
{
    (void)aInstance;
    (void)aEnabled;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return true;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    (void)aInstance;
    (void)aChannel;
    return OT_ERROR_NONE;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    (void)aInstance;
    (void)aFrame;
    return OT_ERROR_NONE;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    (void)aInstance;
    (void)aPower;
    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;
    return NULL;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;
    return OT_RADIO_CAPS_NONE;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;
    return false;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    (void)aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;
    return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    (void)aInstance;
    (void)aExtAddress;
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    (void)aInstance;
    (void)aExtAddress;
    return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void)aInstance;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    (void)aInstance;
    (void)aPower;
    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    (void)aInstance;
    return 0;
}

uint32_t otPlatRandomGet(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    tmpstate = (uint64_t)33614 * (uint64_t)sRandomState;
    q        = tmpstate & 0xffffffff;
    q        = q >> 1;
    p        = tmpstate >> 32;
    mlcg     = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    sRandomState = mlcg;

    return mlcg;
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

    return OT_ERROR_NONE;
}

void otPlatSettingsInit(otInstance *aInstance)
{
    (void)aInstance;
}

otError otPlatSettingsBeginChange(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatSettingsCommitChange(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatSettingsAbandonChange(otInstance *aInstance)
{
    (void)aInstance;
    return OT_ERROR_NONE;
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    (void)aInstance;
    (void)aKey;
    (void)aIndex;
    (void)aValue;
    (void)aValueLength;
    return OT_ERROR_NOT_FOUND;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    (void)aInstance;
    (void)aKey;
    (void)aValue;
    (void)aValueLength;
    return OT_ERROR_NONE;
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    (void)aInstance;
    (void)aKey;
    (void)aValue;
    (void)aValueLength;
    return OT_ERROR_NONE;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    (void)aInstance;
    (void)aKey;
    (void)aIndex;
    return OT_ERROR_NONE;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    (void)aInstance;
}

otError otPlatUartEnable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    (void)aBuf;
    (void)aBufLength;
    return OT_ERROR_NONE;
}
