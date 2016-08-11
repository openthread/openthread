/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
#include <windows.h>
#endif

#include <openthread.h>

#include <platform/alarm.h>
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

    void otSignalTaskletPending(otContext *)
    {
    }

    //
    // Alarm
    //

    void otPlatAlarmStop(otContext *)
    {
        sTimerOn = false;
        sCallCount[kCallCountIndexAlarmStop]++;
    }

    void otPlatAlarmStartAt(otContext *, uint32_t aT0, uint32_t aDt)
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

    ThreadError otPlatRadioSetPanId(otContext *, uint16_t)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioSetExtendedAddress(otContext *, uint8_t *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioSetShortAddress(otContext *, uint16_t)
    {
        return kThreadError_None;
    }

    void otPlatRadioSetPromiscuous(otContext *, bool)
    {
    }

    ThreadError otPlatRadioEnable(otContext *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioDisable(otContext *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioSleep(otContext *)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioReceive(otContext *, uint8_t)
    {
        return kThreadError_None;
    }

    ThreadError otPlatRadioTransmit(otContext *)
    {
        return kThreadError_None;
    }

    RadioPacket *otPlatRadioGetTransmitBuffer(otContext *)
    {
        return (RadioPacket *)0;
    }

    int8_t otPlatRadioGetNoiseFloor(otContext *)
    {
        return 0;
    }

    otRadioCaps otPlatRadioGetCaps(otContext *)
    {
        return kRadioCapsNone;
    }

    bool otPlatRadioGetPromiscuous(otContext *)
    {
        return false;
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

} // extern "C"