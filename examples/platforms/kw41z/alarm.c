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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include "fsl_clock.h"
#include "fsl_device_registers.h"
#include "fsl_pit.h"
#include <stdint.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

static volatile uint32_t sTime      = 0;
static uint32_t          sAlarmTime = 0;

void kw41zAlarmInit(void)
{
    pit_config_t config;
    uint32_t     count = (CLOCK_GetBusClkFreq() / 1000) - 1;

    PIT_GetDefaultConfig(&config);
    PIT_Init(PIT, &config);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, count);
    PIT_StartTimer(PIT, kPIT_Chnl_0);

    PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
    NVIC_ClearPendingIRQ(PIT_IRQn);
    NVIC_EnableIRQ(PIT_IRQn);
}

void kw41zAlarmProcess(otInstance *aInstance)
{
    if ((sAlarmTime != 0) && (sTime >= sAlarmTime))
    {
        sAlarmTime = 0;
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagAlarmFired(aInstance);
        }
        else
#endif
        {
            otPlatAlarmMilliFired(aInstance);
        }
    }
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmTime = aT0 + aDt;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmTime = 0;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return sTime;
}

void PIT_IRQHandler(void)
{
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
    sTime++;
}
