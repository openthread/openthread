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

#include <openthread/platform/alarm.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/usec-alarm.h>

#include "platform-config.h"
#include "cmsis/cmsis_gcc.h"
#include "drivers/nrf_drv_clock.h"
#include "hal/nrf_rtc.h"

#include <openthread-config.h>
#include <openthread/types.h>

#define RTC_FREQUENCY       32768ULL

#define CEIL_DIV(A, B)      (((A) + (B) - 1ULL) / (B))

#define US_PER_MS           1000ULL
#define US_PER_S            1000000ULL
#define US_PER_TICK         CEIL_DIV(US_PER_S, RTC_FREQUENCY)

#define MS_PER_S            1000UL
#define MS_PER_OVERFLOW     (512UL * MS_PER_S)  ///< Time that has passed between overflow events. On full RTC speed, it occurs every 512 s.

typedef enum
{
    kMsTimer,
    kUsTimer,
    kNumTimers
} AlarmIndex;

typedef struct
{
    volatile bool       mFireAlarm;   ///< Information for processing function, that alarm should fire.
    otPlatUsecAlarmTime mT0;          ///< Alarm start time, for tracking overflows.
    otPlatUsecAlarmTime mTargetTime;  ///< Alarm fire time.
} AlarmData;

typedef struct
{
    uint32_t        mChannelNumber;
    uint32_t        mCompareEventMask;
    nrf_rtc_event_t mCompareEvent;
    nrf_rtc_int_t   mCompareInt;
} AlarmChannelData;

static volatile otPlatUsecAlarmTime sTimeOffset = { 0 };  ///< Time offset to keep track of current time.
static AlarmData sTimerData[kNumTimers];                  ///< Data of the timers.
static const AlarmChannelData sChannelData[kNumTimers] =
{
    [kMsTimer] = {
        .mChannelNumber    = 0,
        .mCompareEventMask = RTC_EVTEN_COMPARE0_Msk,
        .mCompareEvent     = NRF_RTC_EVENT_COMPARE_0,
        .mCompareInt       = NRF_RTC_INT_COMPARE0_MASK,
    },
    [kUsTimer] = {
        .mChannelNumber    = 1,
        .mCompareEventMask = RTC_EVTEN_COMPARE1_Msk,
        .mCompareEvent     = NRF_RTC_EVENT_COMPARE_1,
        .mCompareInt       = NRF_RTC_INT_COMPARE1_MASK,
    }
};

static otPlatUsecAlarmHandler sUsecHandler = NULL;  ///< Handler called when usec alarm fires.
static void *sUsecContext = NULL;                   ///< The context information passed to the usec handler callback.

static void HandleOverflow(void);

static inline uint32_t TimeToTicks(otPlatUsecAlarmTime aTime)
{
    uint64_t microseconds = US_PER_MS * (uint64_t)aTime.mMs + (uint64_t)aTime.mUs;
    return (uint32_t)CEIL_DIV((microseconds * RTC_FREQUENCY), US_PER_S) & RTC_CC_COMPARE_Msk;
}

static inline otPlatUsecAlarmTime TicksToTime(uint32_t aTicks)
{
    uint64_t microseconds = CEIL_DIV((US_PER_S * (uint64_t)aTicks), RTC_FREQUENCY);
    return (otPlatUsecAlarmTime) {microseconds / US_PER_MS, microseconds % US_PER_MS};
}

static inline otPlatUsecAlarmTime TimeAdd(otPlatUsecAlarmTime aTime1, otPlatUsecAlarmTime aTime2)
{
    otPlatUsecAlarmTime result = { aTime1.mMs + aTime2.mMs, aTime1.mUs + aTime2.mUs };

    assert(result.mUs < 2 * US_PER_MS);

    if (result.mUs >= US_PER_MS)
    {
        result.mMs++;
        result.mUs -= US_PER_MS;
    }

    return result;
}

static inline otPlatUsecAlarmTime AlarmGetCurrentTime(void)
{
    uint32_t rtcValue1;
    uint32_t rtcValue2;
    otPlatUsecAlarmTime offset;

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

static inline otPlatUsecAlarmTime AlarmGetCurrentTimeRtcProtected(void)
{
    return TimeAdd(AlarmGetCurrentTime(), (otPlatUsecAlarmTime) {0, 2 * US_PER_TICK});
}

static inline bool TimeIsLower(otPlatUsecAlarmTime aTime1, otPlatUsecAlarmTime aTime2)
{
    return ((aTime1.mMs < aTime2.mMs) ||
            ((aTime1.mMs == aTime2.mMs) && (aTime1.mUs < aTime2.mUs)));
}

static inline bool AlarmShallStrike(otPlatUsecAlarmTime aNow, AlarmIndex aIndex)
{
    if (TimeIsLower(sTimerData[aIndex].mTargetTime, sTimerData[aIndex].mT0))
    {
        // Handle situation when timespan included timer overflow.
        if (TimeIsLower(aNow, sTimerData[aIndex].mT0) && !TimeIsLower(aNow, sTimerData[aIndex].mTargetTime))
        {
            return true;
        }
    }
    else
    {
        if (TimeIsLower(aNow, sTimerData[aIndex].mT0) || !TimeIsLower(aNow, sTimerData[aIndex].mTargetTime))
        {
            return true;
        }
    }

    return false;
}

static void HandleCompareMatch(AlarmIndex aIndex, bool aSkipCheck)
{
    nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[aIndex].mCompareEvent);

    otPlatUsecAlarmTime now = AlarmGetCurrentTime();

    // In case the target time was larger than single overflow,
    // we should only strike the timer on final compare event.
    if (aSkipCheck || AlarmShallStrike(now, aIndex))
    {
        nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);
        nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);

        sTimerData[aIndex].mFireAlarm = true;
    }
}

static void HandleOverflow(void)
{
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

    // Increment counter on overflow.
    sTimeOffset = TimeAdd(sTimeOffset, (otPlatUsecAlarmTime) {MS_PER_OVERFLOW, 0});
}

static void AlarmStartAt(const otPlatUsecAlarmTime *aT0, const otPlatUsecAlarmTime *aTargetTime, AlarmIndex aIndex)
{
    uint32_t targetCounter;
    otPlatUsecAlarmTime now;

    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);
    nrf_rtc_event_enable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);

    sTimerData[aIndex].mT0         = *aT0;
    sTimerData[aIndex].mTargetTime = *aTargetTime;

    targetCounter = TimeToTicks(sTimerData[aIndex].mTargetTime);

    nrf_rtc_cc_set(RTC_INSTANCE, sChannelData[aIndex].mChannelNumber, targetCounter);

    now = AlarmGetCurrentTimeRtcProtected();

    if (AlarmShallStrike(now, aIndex))
    {
        HandleCompareMatch(aIndex, true);
    }
    else
    {
        nrf_rtc_int_enable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);
    }
}

static void AlarmStop(AlarmIndex aIndex)
{
    nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);
    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);
    nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[aIndex].mCompareEvent);

    sTimerData[aIndex].mFireAlarm = false;
}

void nrf5AlarmInit(void)
{
    sTimeOffset.mMs = 0;
    sTimeOffset.mUs = 0;
    memset(sTimerData, 0, sizeof(sTimerData));

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

    for (uint32_t i = 0; i < kNumTimers; i++)
    {
        nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[i].mCompareEvent);
        nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[i].mCompareEventMask);
        nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[i].mCompareInt);
    }

    nrf_rtc_task_trigger(RTC_INSTANCE, NRF_RTC_TASK_START);
}

void nrf5AlarmDeinit(void)
{
    nrf_rtc_task_trigger(RTC_INSTANCE, NRF_RTC_TASK_STOP);

    for (uint32_t i = 0; i < kNumTimers; i++)
    {
        nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[i].mCompareEvent);
        nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[i].mCompareEventMask);
        nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[i].mCompareInt);
    }

    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

    NVIC_DisableIRQ(RTC_IRQN);
    NVIC_ClearPendingIRQ(RTC_IRQN);
    NVIC_SetPriority(RTC_IRQN, 0);

    nrf_drv_clock_lfclk_release();
}

void nrf5AlarmProcess(otInstance *aInstance)
{
    if (sTimerData[kMsTimer].mFireAlarm)
    {
        sTimerData[kMsTimer].mFireAlarm = false;

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

    if (sTimerData[kUsTimer].mFireAlarm)
    {
        sTimerData[kUsTimer].mFireAlarm = false;

        sUsecHandler(sUsecContext);
    }
}

uint32_t otPlatAlarmGetNow(void)
{
    otPlatUsecAlarmTime now = AlarmGetCurrentTime();

    return now.mMs;
}

void otPlatAlarmStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;
    otPlatUsecAlarmTime T0         = (otPlatUsecAlarmTime) {aT0, 0};
    otPlatUsecAlarmTime TargetTime = (otPlatUsecAlarmTime) {aT0 + aDt, 0};

    AlarmStartAt(&T0, &TargetTime, kMsTimer);
}

void otPlatAlarmStop(otInstance *aInstance)
{
    (void)aInstance;

    AlarmStop(kMsTimer);
}

void otPlatUsecAlarmGetNow(otPlatUsecAlarmTime *aNow)
{
    otPlatUsecAlarmTime now = AlarmGetCurrentTime();

    memcpy(aNow, &now, sizeof(*aNow));
}

void otPlatUsecAlarmStartAt(otInstance *aInstance,
                            const otPlatUsecAlarmTime *aT0,
                            const otPlatUsecAlarmTime *aDt,
                            otPlatUsecAlarmHandler aHandler,
                            void *aContext)
{
    (void)aInstance;
    otPlatUsecAlarmTime TargetTime = TimeAdd(*aT0, *aDt);

    AlarmStartAt(aT0, &TargetTime, kUsTimer);

    sUsecHandler = aHandler;
    sUsecContext = aContext;
}

void otPlatUsecAlarmStop(otInstance *aInstance)
{
    (void)aInstance;

    AlarmStop(kUsTimer);
}

void RTC_IRQ_HANDLER(void)
{
    // Handle overflow.
    if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW))
    {
        HandleOverflow();
    }

    // Handle compare match.
    for (uint32_t i = 0; i < kNumTimers; i++)
    {
        if (nrf_rtc_int_is_enabled(RTC_INSTANCE, sChannelData[i].mCompareInt) &&
            nrf_rtc_event_pending(RTC_INSTANCE, sChannelData[i].mCompareEvent))
        {
            HandleCompareMatch(i, false);
        }
    }
}
