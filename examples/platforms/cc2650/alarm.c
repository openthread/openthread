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

#include <stdbool.h>
#include <stdint.h>
#include <openthread-types.h>
#include <driverlib/aon_rtc.h>

#include <platform/alarm.h>
#include <platform/diag.h>

/**
 * /NOTE: we could use systick, but that would sacrifice atleast a few ops
 * every ms, and not run when the processor is sleeping.
 */

static uint32_t sTime0     = 0;
static uint32_t sAlarmTime = 0;
static bool     sIsRunning = false;

/**
 * Function documented in platform-cc2650.h
 */
void cc2650AlarmInit(void)
{
    /*
     * NOTE: this will not enable the individual rtc alarm channels
     */
    AONRTCEnable();
    sIsRunning = true;
}

/**
 * Function documented in platform/alarm.h
 */
uint32_t otPlatAlarmGetNow(void)
{
    /*
     * This is current value of RTC as it appears in the register.
     * With seconds as the upper 32 bytes and fractions of a second as the
     * lower 32 bytes <32.32>.
     */
    uint64_t rtcVal = AONRTCCurrent64BitValueGet();
    return ((rtcVal * 1000) >> 32);
}

/**
 * Function documented in platform/alarm.h
 */
void otPlatAlarmGetPreciseNow(otPlatAlarmTime *aNow)
{
    aNow->mMs = otPlatAlarmGetNow();
    aNow->mUs = 0;
}

/**
 * Function documented in platform/alarm.h
 */
void otPlatAlarmStartAt(otInstance *aInstance, const otPlatAlarmTime *aT0, const otPlatAlarmTime *aDt)
{
    (void)aInstance;
    sTime0 = aT0->mMs;
    sAlarmTime = aDt->mMs;

    if (aDt->mUs)
    {
        sAlarmTime++;
    }

    sIsRunning = true;
}

/**
 * Function documented in platform/alarm.h
 */
void otPlatAlarmStop(otInstance *aInstance)
{
    (void)aInstance;
    sIsRunning = false;
}

/**
 * Function documented in platform-cc2650.h
 */
void cc2650AlarmProcess(otInstance *aInstance)
{
    uint32_t offsetTime;

    if (sIsRunning)
    {
        /* unsinged subtraction will result in the absolute offset */
        offsetTime = otPlatAlarmGetNow() - sTime0;

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
                otPlatAlarmFired(aInstance);
            }
        }
    }
}

