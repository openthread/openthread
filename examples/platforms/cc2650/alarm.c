/******************************************************************************
 *  Copyright (c) 2016, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1) Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2) Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3) Neither the name of the ORGANIZATION nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
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
 *
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <aon_rtc.h>

#include <platform/alarm.h>
#include <platform/diag.h>

/**
 * /NOTE: we could use systick, but that would sacrifice atleast a few ops
 * every ms, and not run when the processor is sleeping.
 */

uint32_t time0 = 0;
uint32_t alarmTime = 0;
bool isRunning = false;

void cc2650AlarmInit(void)
{
    /*
     * NOTE: this will not enable the individual rtc alarm channels
     */
    AONRTCEnable();
    isRunning = true;
}

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

void otPlatAlarmStartAt(uint32_t t0, uint32_t dt)
{
    time0 = t0;
    alarmTime = dt;
    isRunning = true;
}

void otPlatAlarmStop(void)
{
    isRunning = false;
}

void cc2650AlarmProcess(void)
{
    uint32_t offsetTime;

    if(isRunning)
    {
        /* unsinged subtraction will result in the absolute offset */
        offsetTime = otPlatAlarmGetNow() - time0;

        if(alarmTime <= offsetTime)
        {
            isRunning = false;
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired();
            }
            else
#endif /* OPENTHREAD_ENABLE_DIAG */
            {
                otPlatAlarmFired();
            }
        }
    }
}

