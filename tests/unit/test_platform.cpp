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

bool                            g_testPlatAlarmSet = false;
uint32_t                        g_testPlatAlarmNext = 0;
testPlatAlarmStop               g_testPlatAlarmStop = NULL;
testPlatAlarmStartAt            g_testPlatAlarmStartAt = NULL;
testPlatAlarmGetNow             g_testPlatAlarmGetNow = NULL;

otRadioCaps                     g_testPlatRadioCaps = kRadioCapsNone;
testPlatRadioSetPanId           g_testPlatRadioSetPanId = NULL;
testPlatRadioSetExtendedAddress g_testPlatRadioSetExtendedAddress = NULL;
testPlatRadioSetShortAddress    g_testPlatRadioSetShortAddress = NULL;
testPlatRadioReceive            g_testPlatRadioReceive = NULL;
testPlatRadioTransmit           g_testPlatRadioTransmit = NULL;
testPlatRadioGetTransmitBuffer  g_testPlatRadioGetTransmitBuffer = NULL;

void testPlatResetToDefaults(void)
{
    g_testPlatAlarmSet = false;
    g_testPlatAlarmNext = 0;
    g_testPlatAlarmStop = NULL;
    g_testPlatAlarmStartAt = NULL;
    g_testPlatAlarmGetNow = NULL;

    g_testPlatRadioCaps = kRadioCapsNone;
    g_testPlatRadioSetPanId = NULL;
    g_testPlatRadioSetExtendedAddress = NULL;
    g_testPlatRadioSetShortAddress = NULL;
    g_testPlatRadioReceive = NULL;
    g_testPlatRadioTransmit = NULL;
    g_testPlatRadioGetTransmitBuffer = NULL;
}

bool sDiagMode = false;

enum
{
    kFlashSize = 0x40000,
    kFlashPageSize = 0x800,
};

uint8_t sFlashBuffer[kFlashSize];

extern "C" {

    void otSignalTaskletPending(otInstance *)
    {
    }

    //
    // Alarm
    //

#if _WIN32
    __forceinline int gettimeofday(struct timeval *tv, struct timezone *)
    {
        DWORD tick = GetTickCount();
        tv->tv_sec = (long)(tick / 1000);
        tv->tv_usec = (long)(tick * 1000);
        return 0;
    }
#endif

    void otPlatAlarmStop(otInstance *aInstance)
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

    void otPlatAlarmStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
    {
        if (g_testPlatAlarmStartAt)
        {
            g_testPlatAlarmStartAt(aInstance, aT0, aDt);
        }
        else
        {
            g_testPlatAlarmSet = true;
            g_testPlatAlarmNext = aT0 + aDt;
        }
    }

    uint32_t otPlatAlarmGetNow(void)
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

    void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aExtAddr)
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

    ThreadError otPlatRadioEnable(otInstance *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioDisable(otInstance *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioSleep(otInstance *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
    {
        if (g_testPlatRadioReceive)
        {
            return g_testPlatRadioReceive(aInstance, aChannel);
        }
        else
        {
            return kThreadError_None;
        }
    }

    ThreadError otPlatRadioTransmit(otInstance *aInstance)
    {
        if (g_testPlatRadioTransmit)
        {
            return g_testPlatRadioTransmit(aInstance);
        }
        else
        {
            return kThreadError_None;
        }
    }

    RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
    {
        if (g_testPlatRadioGetTransmitBuffer)
        {
            return g_testPlatRadioGetTransmitBuffer(aInstance);
        }
        else
        {
            return (RadioPacket *)0;
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

    ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
    {
        (void)aInstance;
        (void)aShortAddress;
        return kThreadError_None;
    }

    ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
    {
        (void)aInstance;
        (void)aExtAddress;
        return kThreadError_None;
    }

    ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
    {
        (void)aInstance;
        (void)aShortAddress;
        return kThreadError_None;
    }

    ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
    {
        (void)aInstance;
        (void)aExtAddress;
        return kThreadError_None;
    }

    void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
    {
        (void)aInstance;
    }

    void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
    {
        (void)aInstance;
    }

    ThreadError otPlatRadioEnergyScan(otInstance *, uint8_t, uint16_t)
    {
        return kThreadError_NotImplemented;
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

    ThreadError otPlatRandomSecureGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
    {
        ThreadError error = kThreadError_None;

        VerifyOrExit(aOutput && aOutputLength, error = kThreadError_InvalidArgs);

        for (uint16_t length = 0; length < aInputLength; length++)
        {
            aOutput[length] = (uint8_t)otPlatRandomGet();
        }

        *aOutputLength = aInputLength;

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

    void otPlatDiagRadioTransmitDone(otInstance *, bool, ThreadError)
    {
    }

    void otPlatDiagRadioReceiveDone(otInstance *, RadioPacket *, ThreadError)
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
        return kPlatResetReason_PowerOn;
    }

    void otPlatLog(otLogLevel , otLogRegion , const char *, ...)
    {
    }

    //
    // Flash
    //
    ThreadError otPlatFlashInit(void)
    {
        memset(sFlashBuffer, 0xff, kFlashSize);
        return kThreadError_None;
    }

    uint32_t otPlatFlashGetSize(void)
    {
        return kFlashSize;
    }

    ThreadError otPlatFlashErasePage(uint32_t aAddress)
    {
        ThreadError error = kThreadError_None;
        uint32_t address;

        VerifyOrExit(aAddress < kFlashSize, error = kThreadError_InvalidArgs);

        // Get start address of the flash page that includes aAddress
        address = aAddress & (~(uint32_t)(kFlashPageSize - 1));
        memset(sFlashBuffer + address, 0xff, kFlashPageSize);

exit:
        return error;
    }

    ThreadError otPlatFlashStatusWait(uint32_t aTimeout)
    {
        (void)aTimeout;
        return kThreadError_None;
    }

    uint32_t otPlatFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
    {
        uint32_t ret = 0;
        uint8_t byte;

        VerifyOrExit(aAddress < kFlashSize, ;);

        for (uint32_t index = 0; index < aSize; index++)
        {
            byte = sFlashBuffer[aAddress + index];
            byte &= aData[index];
            sFlashBuffer[aAddress + index] = byte;
        }

        ret = aSize;

exit:
        return ret;
    }

    uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
    {
        uint32_t ret = 0;

        VerifyOrExit(aAddress < kFlashSize, ;);

        memcpy(aData, sFlashBuffer + aAddress, aSize);
        ret = aSize;

exit:
        return ret;
    }

} // extern "C"
