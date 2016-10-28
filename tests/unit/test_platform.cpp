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

#if _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include <openthread.h>

#include <common/code_utils.hpp>
#include <platform/alarm.h>
#include <platform/misc.h>
#include <platform/radio.h>
#include <platform/random.h>

#include <stdio.h>
#include <stdlib.h>

enum
{
    kCallCountIndexAlarmStop = 0,
    kCallCountIndexAlarmStart,
    kCallCountIndexTimerHandler,

    kCallCountIndexMax
};

uint32_t sNow;
uint32_t sPlatT0;
uint32_t sPlatDt;
bool     sTimerOn;
uint32_t sCallCount[kCallCountIndexMax];

bool sDiagMode = false;

extern "C" {

    void otSignalTaskletPending(otInstance *)
    {
    }

    //
    // Alarm
    //

    void otPlatAlarmStop(otInstance *)
    {
        sTimerOn = false;
        sCallCount[kCallCountIndexAlarmStop]++;
    }

    void otPlatAlarmStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
    {
        sTimerOn = true;
        sCallCount[kCallCountIndexAlarmStart]++;
        sPlatT0 = aT0;
        sPlatDt = aDt;
    }

    uint32_t otPlatAlarmGetNow(void)
    {
        return sNow;
    }

    //
    // Radio
    //

    void otPlatRadioGetIeeeEui64(otInstance *, uint8_t *)
    {
    }

    void otPlatRadioSetPanId(otInstance *, uint16_t)
    {
    }

    void otPlatRadioSetExtendedAddress(otInstance *, uint8_t *)
    {
    }

    void otPlatRadioSetShortAddress(otInstance *, uint16_t)
    {
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

    ThreadError otPlatRadioReceive(otInstance *, uint8_t)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioTransmit(otInstance *)
    {
        return kThreadError_None;
    }

    RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *)
    {
        return (RadioPacket *)0;
    }

    int8_t otPlatRadioGetRssi(otInstance *)
    {
        return 0;
    }

    otRadioCaps otPlatRadioGetCaps(otInstance *)
    {
        return kRadioCapsNone;
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

} // extern "C"
