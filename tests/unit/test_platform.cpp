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

#include <sys/time.h>

bool                 g_testPlatAlarmSet     = false;
uint32_t             g_testPlatAlarmNext    = 0;
testPlatAlarmStop    g_testPlatAlarmStop    = nullptr;
testPlatAlarmStartAt g_testPlatAlarmStartAt = nullptr;
testPlatAlarmGetNow  g_testPlatAlarmGetNow  = nullptr;

otRadioCaps                     g_testPlatRadioCaps               = OT_RADIO_CAPS_NONE;
testPlatRadioSetPanId           g_testPlatRadioSetPanId           = nullptr;
testPlatRadioSetExtendedAddress g_testPlatRadioSetExtendedAddress = nullptr;
testPlatRadioIsEnabled          g_testPlatRadioIsEnabled          = nullptr;
testPlatRadioEnable             g_testPlatRadioEnable             = nullptr;
testPlatRadioDisable            g_testPlatRadioDisable            = nullptr;
testPlatRadioSetShortAddress    g_testPlatRadioSetShortAddress    = nullptr;
testPlatRadioReceive            g_testPlatRadioReceive            = nullptr;
testPlatRadioTransmit           g_testPlatRadioTransmit           = nullptr;
testPlatRadioGetTransmitBuffer  g_testPlatRadioGetTransmitBuffer  = nullptr;

enum
{
    FLASH_SWAP_SIZE = 2048,
    FLASH_SWAP_NUM  = 2,
};

uint8_t g_flash[FLASH_SWAP_SIZE * FLASH_SWAP_NUM];

ot::Instance *testInitInstance(void)
{
    otInstance *instance = nullptr;

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   instanceBufferLength = 0;
    uint8_t *instanceBuffer       = nullptr;

    // Call to query the buffer size
    (void)otInstanceInit(nullptr, &instanceBufferLength);

    // Call to allocate the buffer
    instanceBuffer = (uint8_t *)malloc(instanceBufferLength);
    VerifyOrQuit(instanceBuffer != nullptr, "Failed to allocate otInstance");
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

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    free(aInstance);
#endif
}

bool sDiagMode = false;

extern "C" {

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
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
        gettimeofday(&tv, nullptr);
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
        gettimeofday(&tv, nullptr);
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
    OT_UNUSED_VARIABLE(aFrame);

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
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);
    return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioEnergyScan(otInstance *, uint8_t, uint16_t)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);
    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return -100;
}
//
// Random
//

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aOutput, error = OT_ERROR_INVALID_ARGS);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)rand();
    }

exit:
    return error;
}

//
// Diag
//

void otPlatDiagProcess(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aOutputMaxLen);

    // no more diagnostics features for Posix platform
    sprintf(aOutput, "diag feature '%s' is not supported\r\n", aArgs[0]);
}

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;
}

bool otPlatDiagModeGet()
{
    return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t)
{
}

void otPlatDiagTxPowerSet(int8_t)
{
}

void otPlatDiagRadioReceived(otInstance *, otRadioFrame *, otError)
{
}

void otPlatDiagAlarmCallback(otInstance *)
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
    OT_UNUSED_VARIABLE(aInstance);
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
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
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aIndex);
    OT_UNUSED_VARIABLE(aValue);
    OT_UNUSED_VARIABLE(aValueLength);

    return OT_ERROR_NOT_FOUND;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aValue);
    OT_UNUSED_VARIABLE(aValueLength);

    return OT_ERROR_NONE;
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aValue);
    OT_UNUSED_VARIABLE(aValueLength);

    return OT_ERROR_NONE;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aIndex);

    return OT_ERROR_NONE;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    memset(g_flash, 0xff, sizeof(g_flash));
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t address;

    VerifyOrQuit(aSwapIndex < FLASH_SWAP_NUM, "aSwapIndex invalid");

    address = aSwapIndex ? FLASH_SWAP_SIZE : 0;

    memset(g_flash + address, 0xff, FLASH_SWAP_SIZE);
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t address;

    VerifyOrQuit(aSwapIndex < FLASH_SWAP_NUM, "aSwapIndex invalid");
    VerifyOrQuit(aSize <= FLASH_SWAP_SIZE, "aSize invalid");
    VerifyOrQuit(aOffset <= (FLASH_SWAP_SIZE - aSize), "aOffset + aSize invalid");

    address = aSwapIndex ? FLASH_SWAP_SIZE : 0;

    memcpy(aData, g_flash + address + aOffset, aSize);
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t address;

    VerifyOrQuit(aSwapIndex < FLASH_SWAP_NUM, "aSwapIndex invalid");
    VerifyOrQuit(aSize <= FLASH_SWAP_SIZE, "aSize invalid");
    VerifyOrQuit(aOffset <= (FLASH_SWAP_SIZE - aSize), "aOffset + aSize invalid");

    address = aSwapIndex ? FLASH_SWAP_SIZE : 0;

    for (uint32_t index = 0; index < aSize; index++)
    {
        g_flash[address + aOffset + index] &= ((uint8_t *)aData)[index];
    }
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
uint16_t otPlatTimeGetXtalAccuracy(void)
{
    return 0;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCslPeriod);
    OT_UNUSED_VARIABLE(aExtAddr);

    return OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCslSampleTime);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_OTNS_ENABLE
void otPlatOtnsStatus(const char *aStatus)
{
    OT_UNUSED_VARIABLE(aStatus);
}
#endif // OPENTHREAD_CONFIG_OTNS_ENABLE

} // extern "C"
