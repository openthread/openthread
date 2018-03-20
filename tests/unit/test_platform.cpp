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

#include "test_platform.h"

#if _WIN32
__forceinline int gettimeofday(struct timeval *tv, struct timezone *)
{
    DWORD tick  = GetTickCount();
    tv->tv_sec  = (long)(tick / 1000);
    tv->tv_usec = (long)(tick * 1000);
    return 0;
}
#else
#include <sys/time.h>
#endif

bool                 g_testPlatAlarmSet     = false;
uint32_t             g_testPlatAlarmNext    = 0;
testPlatAlarmStop    g_testPlatAlarmStop    = NULL;
testPlatAlarmStartAt g_testPlatAlarmStartAt = NULL;
testPlatAlarmGetNow  g_testPlatAlarmGetNow  = NULL;

otRadioCaps                     g_testPlatRadioCaps               = OT_RADIO_CAPS_NONE;
testPlatRadioSetPanId           g_testPlatRadioSetPanId           = NULL;
testPlatRadioSetExtendedAddress g_testPlatRadioSetExtendedAddress = NULL;
testPlatRadioIsEnabled          g_testPlatRadioIsEnabled          = NULL;
testPlatRadioEnable             g_testPlatRadioEnable             = NULL;
testPlatRadioDisable            g_testPlatRadioDisable            = NULL;
testPlatRadioSetShortAddress    g_testPlatRadioSetShortAddress    = NULL;
testPlatRadioReceive            g_testPlatRadioReceive            = NULL;
testPlatRadioTransmit           g_testPlatRadioTransmit           = NULL;
testPlatRadioGetTransmitBuffer  g_testPlatRadioGetTransmitBuffer  = NULL;

void testPlatResetToDefaults(void)
{
    g_testPlatAlarmSet     = false;
    g_testPlatAlarmNext    = 0;
    g_testPlatAlarmStop    = NULL;
    g_testPlatAlarmStartAt = NULL;
    g_testPlatAlarmGetNow  = NULL;

    g_testPlatRadioCaps               = OT_RADIO_CAPS_NONE;
    g_testPlatRadioSetPanId           = NULL;
    g_testPlatRadioSetExtendedAddress = NULL;
    g_testPlatRadioSetShortAddress    = NULL;
    g_testPlatRadioIsEnabled          = NULL;
    g_testPlatRadioEnable             = NULL;
    g_testPlatRadioDisable            = NULL;
    g_testPlatRadioReceive            = NULL;
    g_testPlatRadioTransmit           = NULL;
    g_testPlatRadioGetTransmitBuffer  = NULL;
}

ot::Instance *testInitInstance(void)
{
    otInstance *instance = NULL;

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    size_t   instanceBufferLength = 0;
    uint8_t *instanceBuffer       = NULL;

    // Call to query the buffer size
    (void)otInstanceInit(NULL, &instanceBufferLength);

    // Call to allocate the buffer
    instanceBuffer = (uint8_t *)malloc(instanceBufferLength);
    VerifyOrQuit(instanceBuffer != NULL, "Failed to allocate otInstance");
    memset(instanceBuffer, 0, instanceBufferLength);

    // Initialize OpenThread with the buffer
    instance = otInstanceInit(instanceBuffer, &instanceBufferLength);
#else
    instance = otInstanceInitSingle();
#endif

    return static_cast<ot::Instance *>(instance);
}

void testFreeInstance(otInstance *aInstance)
{
    otInstanceFinalize(aInstance);

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    free(aInstance);
#endif
}

bool sDiagMode = false;

extern "C" {

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
    free(aPtr);
}
#endif

void otTaskletsSignalPending(otInstance *)
{
}

//
// Alarm
//

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    if (g_testPlatAlarmStop)
    {
        g_testPlatAlarmStop(aInstance);
    }
    else
    {
        g_testPlatAlarmSet = false;
    }
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    if (g_testPlatAlarmStartAt)
    {
        g_testPlatAlarmStartAt(aInstance, aT0, aDt);
    }
    else
    {
        g_testPlatAlarmSet  = true;
        g_testPlatAlarmNext = aT0 + aDt;
    }
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    if (g_testPlatAlarmGetNow)
    {
        return g_testPlatAlarmGetNow();
    }
    else
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000) + 123456);
    }
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    if (g_testPlatAlarmStop)
    {
        g_testPlatAlarmStop(aInstance);
    }
    else
    {
        g_testPlatAlarmSet = false;
    }
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    if (g_testPlatAlarmStartAt)
    {
        g_testPlatAlarmStartAt(aInstance, aT0, aDt);
    }
    else
    {
        g_testPlatAlarmSet  = true;
        g_testPlatAlarmNext = aT0 + aDt;
    }
}

uint32_t otPlatAlarmMicroGetNow(void)
{
    if (g_testPlatAlarmGetNow)
    {
        return g_testPlatAlarmGetNow();
    }
    else
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint32_t)((tv.tv_sec * 1000000) + tv.tv_usec + 123456);
    }
}

//
// Radio
//

void otPlatRadioGetIeeeEui64(otInstance *, uint8_t *)
{
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    if (g_testPlatRadioSetPanId)
    {
        g_testPlatRadioSetPanId(aInstance, aPanId);
    }
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddr)
{
    if (g_testPlatRadioSetExtendedAddress)
    {
        g_testPlatRadioSetExtendedAddress(aInstance, aExtAddr);
    }
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddr)
{
    if (g_testPlatRadioSetShortAddress)
    {
        g_testPlatRadioSetShortAddress(aInstance, aShortAddr);
    }
}

void otPlatRadioSetPromiscuous(otInstance *, bool)
{
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    if (g_testPlatRadioIsEnabled)
    {
        return g_testPlatRadioIsEnabled(aInstance);
    }
    else
    {
        return true;
    }
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    if (g_testPlatRadioEnable)
    {
        return g_testPlatRadioEnable(aInstance);
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    if (g_testPlatRadioEnable)
    {
        return g_testPlatRadioDisable(aInstance);
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

otError otPlatRadioSleep(otInstance *)
{
    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    if (g_testPlatRadioReceive)
    {
        return g_testPlatRadioReceive(aInstance, aChannel);
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    (void)aFrame;

    if (g_testPlatRadioTransmit)
    {
        return g_testPlatRadioTransmit(aInstance);
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    if (g_testPlatRadioGetTransmitBuffer)
    {
        return g_testPlatRadioGetTransmitBuffer(aInstance);
    }
    else
    {
        return (otRadioFrame *)0;
    }
}

int8_t otPlatRadioGetRssi(otInstance *)
{
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *)
{
    return g_testPlatRadioCaps;
}

bool otPlatRadioGetPromiscuous(otInstance *)
{
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

otError otPlatRadioEnergyScan(otInstance *, uint8_t, uint16_t)
{
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
//
// Random
//

uint32_t otPlatRandomGet(void)
{
#if _WIN32
    return (uint32_t)rand();
#else
    return (uint32_t)random();
#endif
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aOutput, error = OT_ERROR_INVALID_ARGS);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

exit:
    return error;
}

//
// Diag
//

void otPlatDiagProcess(int argc, char *argv[], char *aOutput)
{
    // no more diagnostics features for Posix platform
    sprintf(aOutput, "diag feature '%s' is not supported\r\n", argv[0]);
    (void)argc;
}

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;
}

bool otPlatDiagModeGet()
{
    return sDiagMode;
}

void otPlatDiagAlarmFired(otInstance *)
{
}

void otPlatDiagRadioTransmitDone(otInstance *, otRadioFrame *, otError)
{
}

void otPlatDiagRadioReceiveDone(otInstance *, otRadioFrame *, otError)
{
}

//
// Uart
//

void otPlatUartSendDone(void)
{
}

void otPlatUartReceived(const uint8_t *, uint16_t)
{
}

//
// Misc
//

void otPlatReset(otInstance *aInstance)
{
    (void)aInstance;
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    (void)aInstance;
    return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatLog(otLogLevel, otLogRegion, const char *, ...)
{
}

//
// Settings
//

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

} // extern "C"
