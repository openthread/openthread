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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <platform/alarm.h>
#include <platform/diag.h>

#include "platform-config.h"
#include "cmsis/cmsis_gcc.h"
#include "drivers/nrf_drv_clock.h"
#include "hal/nrf_rtc.h"

#include <openthread-config.h>
#include <openthread-types.h>

#define RTC_FREQUENCY       32768ULL

#define US_PER_MS           1000ULL
#define US_PER_S            1000000ULL
#define US_PER_TICK         ((US_PER_S + RTC_FREQUENCY - 1ULL) / RTC_FREQUENCY)

#define MS_PER_S            1000UL
#define MS_PER_OVERFLOW     (512UL * MS_PER_S)  ///< Time that has passed between overflow events. On full RTC speed, it occurs every 512 s.

typedef struct
{
    uint32_t ms;
    uint16_t us;
} AlarmTime;

static volatile bool sFireAlarm = false;        ///< Information for processing function, that alarm should fire.
static volatile AlarmTime sTimeOffset = { 0 };  ///< Time offset to keep track of current time.
static AlarmTime sT0Time = { 0 };               ///< Alarm start time, for tracking overflows.
static AlarmTime sTargetTime = { 0 };           ///< Alarm fire time.

static void HandleOverflow(void);

static inline uint32_t TimeToTicks(AlarmTime aTime)
{
    uint64_t microseconds = US_PER_MS * (uint64_t)aTime.ms + (uint64_t)aTime.us;
    return (uint32_t)((microseconds * RTC_FREQUENCY) / US_PER_S) & RTC_CC_COMPARE_Msk;
}

static inline AlarmTime TicksToTime(uint32_t aTicks)
{
    uint64_t microseconds = (US_PER_S * (uint64_t)aTicks) / RTC_FREQUENCY;
    return (AlarmTime) {microseconds / US_PER_MS, microseconds % US_PER_MS};
}

static inline AlarmTime TimeAdd(AlarmTime aTime1, AlarmTime aTime2)
{
    AlarmTime result = { aTime1.ms + aTime2.ms, aTime1.us + aTime2.us };

    assert(result.us < 2 * US_PER_MS);

    if (result.us >= US_PER_MS)
    {
        result.ms++;
        result.us -= US_PER_MS;
    }

    return result;
}

static inline AlarmTime AlarmGetCurrentTime(void)
{
    uint32_t rtcValue1;
    uint32_t rtcValue2;
    AlarmTime offset;

    rtcValue1 = nrf_rtc_counter_get(RTC_INSTANCE);

    __DMB();

    offset = sTimeOffset;

    __DMB();

    rtcValue2 = nrf_rtc_counter_get(RTC_INSTANCE);

    if ((rtcValue2 < rtcValue1) || (rtcValue1 == 0))
    {
        // Overflow detected. Additional condition (rtcValue1 == 0) covers situation when overflow occurred in
        // interrupt state, before this function was entered. But in general, this function shall not be called
        // from interrupt other than alarm interrupt.

        // Wait at least 20 cycles, to ensure that if interrupt is going to be called, it will be called now.
        for (uint32_t i = 0; i < 4; i++)
        {
            __NOP();
            __NOP();
            __NOP();
        }

        // If the event flag is still on, it means that the interrupt was not called, as we are in interrupt state.
        if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW))
        {
            HandleOverflow();
        }

        offset = sTimeOffset;
    }

    return TimeAdd(offset, TicksToTime(rtcValue2));
}

static inline AlarmTime AlarmGetCurrentTimeRtcProtected(void)
{
    return TimeAdd(AlarmGetCurrentTime(), (AlarmTime) {0, 2 * US_PER_TICK});
}

static inline bool TimeIsLower(AlarmTime aTime1, AlarmTime aTime2)
{
    return ((aTime1.ms < aTime2.ms) ||
            ((aTime1.ms == aTime2.ms) && (aTime1.us < aTime2.us)));
}

static inline bool AlarmShallStrike(AlarmTime aNow)
{
    if (TimeIsLower(sTargetTime, sT0Time))
    {
        // Handle situation when timespan included timer overflow.
        if (TimeIsLower(aNow, sT0Time) && !TimeIsLower(aNow, sTargetTime))
        {
            return true;
        }
    }
    else
    {
        if (TimeIsLower(aNow, sT0Time) || !TimeIsLower(aNow, sTargetTime))
        {
            return true;
        }
    }

    return false;
}

static void HandleCompare0Match(void)
{
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);

    AlarmTime now = AlarmGetCurrentTimeRtcProtected();

    // In case the target time was larger than single overflow,
    // we should only strike the timer on final compare event.
    if (AlarmShallStrike(now))
    {
        nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE0_Msk);
        nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);

        sFireAlarm = true;
    }
}

static void HandleOverflow(void)
{
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

    // Increment counter on overflow.
    sTimeOffset = TimeAdd(sTimeOffset, (AlarmTime) {MS_PER_OVERFLOW, 0});
}

void nrf5AlarmInit(void)
{
    sFireAlarm = false;
    memset((void *)&sTimeOffset, 0, sizeof(sTimeOffset));
    memset((void *)&sTargetTime, 0, sizeof(sTargetTime));
    memset((void *)&sT0Time, 0, sizeof(sT0Time));

    // Setup low frequency clock.
    nrf_drv_clock_lfclk_request(NULL);

    while (!nrf_drv_clock_lfclk_is_running()) {}

    // Setup RTC timer.
    NVIC_SetPriority(RTC_IRQN, RTC_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(RTC_IRQN);
    NVIC_EnableIRQ(RTC_IRQN);

    nrf_rtc_prescaler_set(RTC_INSTANCE, 0);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);
    nrf_rtc_event_enable(RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
    nrf_rtc_int_enable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE0_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);

    nrf_rtc_task_trigger(RTC_INSTANCE, NRF_RTC_TASK_START);
}

void nrf5AlarmProcess(otInstance *aInstance)
{
    if (sFireAlarm)
    {
        sFireAlarm = false;

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagAlarmFired(aInstance);
        }
        else
#endif
        {
            otPlatAlarmFired(aInstance);
        }
    }
}

uint32_t otPlatAlarmGetNow(void)
{
    AlarmTime now = AlarmGetCurrentTime();

    return now.ms;
}

void otPlatAlarmStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;
    uint32_t targetCounter;
    AlarmTime now;

    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);
    nrf_rtc_event_enable(RTC_INSTANCE, RTC_EVTEN_COMPARE0_Msk);

    sT0Time = (AlarmTime) {aT0, 0};
    sTargetTime = (AlarmTime) {aT0 + aDt, 0};

    targetCounter = TimeToTicks(sTargetTime);

    nrf_rtc_cc_set(RTC_INSTANCE, 0, targetCounter);

    now = AlarmGetCurrentTimeRtcProtected();

    if (AlarmShallStrike(now))
    {
        HandleCompare0Match();
    }
    else
    {
        nrf_rtc_int_enable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);
    }
}

void otPlatAlarmStop(otInstance *aInstance)
{
    (void)aInstance;

    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE0_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);

    sFireAlarm = false;
}

void RTC0_IRQHandler(void)
{
    // Handle overflow.
    if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW))
    {
        HandleOverflow();
    }

    // Handle compare match.
    if (nrf_rtc_int_is_enabled(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK) &&
        nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0))
    {
        HandleCompare0Match();
    }
}
