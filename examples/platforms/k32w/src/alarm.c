/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

/* Openthread configuration */
#include OPENTHREAD_PROJECT_CORE_CONFIG_FILE

#include "TMR_Adapter.h"
#include "fsl_clock.h"
#include "fsl_ctimer.h"
#include "fsl_device_registers.h"
#include "fsl_wtimer.h"
#include "openthread-system.h"
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#define ALARM_USE_CTIMER 0
#define ALARM_USE_WTIMER 1

/* Timer frequency in Hz needed for 1ms tick */
#define TARGET_FREQ 1000U
/* Wake Timer max count value that is loaded in the register */
#define TIMER0_MAX_COUNT_VALUE 0xffffffff
#define TIMER1_MAX_COUNT_VALUE 0x0fffffff

static bool     sEventFired = false;
static uint32_t refClk;

#if ALARM_USE_CTIMER
/* Match Configuration for Channel 0 */
static ctimer_match_config_t sMatchConfig = {.enableCounterReset = false,
                                             .enableCounterStop  = false,
                                             .matchValue         = 0x00,
                                             .outControl         = kCTIMER_Output_NoAction,
                                             .outPinInitState    = false,
                                             .enableInterrupt    = true};
#else
static uint32_t sRemainingTicks;
#endif

void K32WAlarmInit(void)
{
#if ALARM_USE_CTIMER
    ctimer_config_t config;
    CTIMER_GetDefaultConfig(&config);

    /* Get clk frequency and use prescale to lower it */
    refClk = CLOCK_GetFreq(kCLOCK_Timer0);

    config.prescale = refClk / TARGET_FREQ;
    CTIMER_Init(CTIMER0, &config);
    CTIMER_StartTimer(CTIMER0);

    CTIMER_EnableInterrupts(CTIMER0, kCTIMER_Match0InterruptEnable);
    NVIC_ClearPendingIRQ(Timer0_IRQn);
    NVIC_EnableIRQ(Timer0_IRQn);

#else
    RESET_PeripheralReset(kWKT_RST_SHIFT_RSTn);
    WTIMER_Init();

    /* Get clk frequency and use prescale to lower it */
    refClk = CLOCK_GetFreq(kCLOCK_Xtal32k);

    /* Wake timer 0 is 41 bits long and is used for keepig the timestamp */
    WTIMER_EnableInterrupts(WTIMER_TIMER0_ID);
    /* Wake timer 1 is 28 bits long and is used for alarm events, including waking up the MCU
       from sleep */
    WTIMER_EnableInterrupts(WTIMER_TIMER1_ID);

    NVIC_SetPriority(WAKE_UP_TIMER0_IRQn, gStackTimer_IsrPrio_c >> (8 - __NVIC_PRIO_BITS));
    NVIC_SetPriority(WAKE_UP_TIMER1_IRQn, gStackTimer_IsrPrio_c >> (8 - __NVIC_PRIO_BITS));

    /* Start wake timer 0 counter for timestamp - the counter counts down to 0 so a simple
       substracion from TIMER0_MAX_COUNT_VALUE will give us the timestamp */
    WTIMER_StartTimer(WTIMER_TIMER0_ID, TIMER0_MAX_COUNT_VALUE);
#endif
}

void K32WAlarmClean(void)
{
#if ALARM_USE_CTIMER
    CTIMER_StopTimer(CTIMER0);
    CTIMER_Deinit(CTIMER0);
    CTIMER_DisableInterrupts(CTIMER0, kCTIMER_Match0InterruptEnable);
    NVIC_ClearPendingIRQ(Timer0_IRQn);
#else
    WTIMER_StopTimer(WTIMER_TIMER0_ID);
    WTIMER_StopTimer(WTIMER_TIMER1_ID);
    WTIMER_DeInit();

    NVIC_DisableIRQ(WAKE_UP_TIMER0_IRQn);
    NVIC_ClearPendingIRQ(WAKE_UP_TIMER0_IRQn);
    NVIC_DisableIRQ(WAKE_UP_TIMER1_IRQn);
    NVIC_ClearPendingIRQ(WAKE_UP_TIMER1_IRQn);
#endif
}

void K32WAlarmProcess(otInstance *aInstance)
{
    if (sEventFired)
    {
#if OPENTHREAD_CONFIG_DIAG_ENABLE

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

#if ALARM_USE_CTIMER
    /* Load match register with current counter + app time */
    sMatchConfig.matchValue = aT0 + aDt;

    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_0, &sMatchConfig);
#else
    /* Calculate the difference between now and the requested timestamp aT0 - this time will be
       substracted from the total time until the event needs to fire */
    uint32_t timestamp = otPlatAlarmMilliGetNow();
    timestamp          = timestamp - aT0;

    uint64_t targetTicks = ((aDt - timestamp) * refClk) / TARGET_FREQ;

    /* Because timer 1 is only 28 bits long we need to take into account and event longer than this
       so we arm the timer with the maximum value and re-arm with the remaing time once it fires */
    if (targetTicks < TIMER1_MAX_COUNT_VALUE)
    {
        WTIMER_StartTimer(WTIMER_TIMER1_ID, targetTicks);
        sRemainingTicks = 0;
    }
    else
    {
        WTIMER_StartTimer(WTIMER_TIMER1_ID, TIMER1_MAX_COUNT_VALUE);
        sRemainingTicks = targetTicks - TIMER1_MAX_COUNT_VALUE;
    }

#endif
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sEventFired = false;

#if ALARM_USE_CTIMER
    sMatchConfig.matchValue = 0;
    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_0, &sMatchConfig);
#else
    sRemainingTicks = 0;
    WTIMER_StopTimer(WTIMER_TIMER1_ID);
#endif
}

uint32_t otPlatAlarmMilliGetNow(void)
{
#if ALARM_USE_CTIMER
    return CTIMER0->TC;
#else
    uint32_t timestamp  = WTIMER_ReadTimerSafe(WTIMER_TIMER0_ID);
    uint64_t tempTstamp = (TIMER0_MAX_COUNT_VALUE - timestamp);

    tempTstamp *= TARGET_FREQ;
    tempTstamp /= refClk;
    return (uint32_t)tempTstamp;
#endif
}

#if ALARM_USE_CTIMER
/**
 * Timer interrupt handler function.
 *
 */
void CTIMER0_IRQHandler(void)
{
    uint32_t flags = CTIMER_GetStatusFlags(CTIMER0);
    CTIMER_ClearStatusFlags(CTIMER0, flags);
    sEventFired = true;

#if USE_RTOS
    otSysEventSignalPending();
#endif
}
#else
void WAKE_UP_TIMER0_DriverIRQHandler()
{
    WTIMER_ClearStatusFlags(WTIMER_TIMER0_ID);
    WTIMER_StartTimer(WTIMER_TIMER0_ID, TIMER0_MAX_COUNT_VALUE);

#if USE_RTOS
    otSysEventSignalPending();
#endif
}
void WAKE_UP_TIMER1_DriverIRQHandler()
{
    WTIMER_ClearStatusFlags(WTIMER_TIMER1_ID);
    if (sRemainingTicks)
    {
        WTIMER_StartTimer(WTIMER_TIMER1_ID, sRemainingTicks);
        sRemainingTicks = 0;
    }
    else
    {
        sEventFired = true;
    }

#if USE_RTOS
    otSysEventSignalPending();
#endif
}
#endif
