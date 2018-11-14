/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include <openthread/config.h>

#include <driverlib/aon_rtc.h>
#include <stdbool.h>
#include <stdint.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "platform-cc1352.h"

/* NOTE: we could use systick, but that would sacrifice atleast a few ops
 * every ms, and not run when the processor is sleeping.
 */

static uint32_t sTime0     = 0;
static uint32_t sAlarmTime = 0;
static bool     sIsRunning = false;

/**
 * Function documented in platform-cc1352.h
 */
void cc1352AlarmInit(void)
{
    /*
     * NOTE: this will not enable the individual rtc alarm channels
     */
    AONRTCEnable();
    sIsRunning = true;
}

/**
 * Function documented in platform/alarm-milli.h
 */
uint32_t otPlatAlarmMilliGetNow(void)
{
    /*
     * This is current value of RTC as it appears in the register.
     * With seconds as the upper 32 bits and fractions of a second as the
     * lower 32 bits <32.32>.
     */
    uint64_t rtcVal = AONRTCCurrent64BitValueGet();
    return ((rtcVal * 1000) >> 32);
}

/**
 * Function documented in platform/alarm-milli.h
 */
void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sTime0     = aT0;
    sAlarmTime = aDt;
    sIsRunning = true;
}

/**
 * Function documented in platform/alarm-milli.h
 */
void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsRunning = false;
}

/**
 * Function documented in platform-cc1352.h
 */
void cc1352AlarmProcess(otInstance *aInstance)
{
    uint32_t offsetTime;

    if (sIsRunning)
    {
        /* unsinged subtraction will result in the absolute offset */
        offsetTime = otPlatAlarmMilliGetNow() - sTime0;

        if (sAlarmTime <= offsetTime)
        {
            sIsRunning = false;
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired(aInstance);
            }
            else
#endif /* OPENTHREAD_ENABLE_DIAG */
            {
                otPlatAlarmMilliFired(aInstance);
            }
        }
    }
}
