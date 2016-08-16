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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include <openthread-config.h>
#include <platform/alarm.h>
#include <platform/diag.h>
#include "platform-cc2538.h"

enum
{
    kSystemClock = 32000000,  ///< MHz
    kTicksPerSec = 1000,      ///< Ticks per second
};

static uint32_t sCounter = 0;
static uint32_t sAlarmT0 = 0;
static uint32_t sAlarmDt = 0;
static bool sIsRunning = false;

void cc2538AlarmInit(void)
{
    HWREG(NVIC_ST_RELOAD) = kSystemClock / kTicksPerSec;
    HWREG(NVIC_ST_CTRL) = NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
}

uint32_t otPlatAlarmGetNow(void)
{
    return sCounter;
}

void otPlatAlarmStartAt(uint32_t t0, uint32_t dt)
{
    sAlarmT0 = t0;
    sAlarmDt = dt;
    sIsRunning = true;
}

void otPlatAlarmStop(void)
{
    sIsRunning = false;
}

void cc2538AlarmProcess(void)
{
    uint32_t expires;
    bool fire = false;

    if (sIsRunning)
    {
        expires = sAlarmT0 + sAlarmDt;

        if (sAlarmT0 <= sCounter)
        {
            if (expires >= sAlarmT0 && expires <= sCounter)
            {
                fire = true;
            }
        }
        else
        {
            if (expires >= sAlarmT0 || expires <= sCounter)
            {
                fire = true;
            }
        }

        if (fire)
        {
            sIsRunning = false;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired();
            }
            else
#endif
            {
                otPlatAlarmFired();
            }
        }
    }

}

void SysTick_Handler()
{
    sCounter++;
}
