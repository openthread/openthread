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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "platform.h"

#include "platform-config.h"
#include "platform-nrf5.h"
#include "cmsis/core_cmFunc.h"

#include <drivers/clock/nrf_drv_clock.h>
#include <drivers/radio/platform/timer/nrf_802154_timer.h>

#include <hal/nrf_rtc.h>

#include <openthread/config.h>
#include <openthread/types.h>

// clang-format off
#define RTC_FREQUENCY       32768ULL

#define US_PER_MS           1000ULL
#define US_PER_S            1000000ULL
#define US_PER_TICK         CEIL_DIV(US_PER_S, RTC_FREQUENCY)
#define US_PER_OVERFLOW     (512UL * US_PER_S)  ///< Time that has passed between overflow events. On full RTC speed, it occurs every 512 s.

#define MS_PER_S            1000UL
// clang-format on

typedef enum { kMsTimer, kUsTimer, k802154Timer, kNumTimers } AlarmIndex;

typedef struct
{
    volatile bool mFireAlarm;  ///< Information for processing function, that alarm should fire.
    uint64_t      mTargetTime; ///< Alarm fire time (in millisecond for MsTimer, in microsecond for UsTimer)
} AlarmData;

typedef struct
{
    uint32_t        mChannelNumber;
    uint32_t        mCompareEventMask;
    nrf_rtc_event_t mCompareEvent;
    nrf_rtc_int_t   mCompareInt;
} AlarmChannelData;

static volatile uint32_t sOverflowCounter; ///< Counter of RTC overflowCounter, incremented by 2 on each OVERFLOW event.
static volatile uint8_t  sMutex;           ///< Mutex for write access to @ref sOverflowCounter.
static volatile uint64_t sTimeOffset = 0;  ///< Time overflowCounter to keep track of current time (in millisecond).
static AlarmData         sTimerData[kNumTimers];         ///< Data of the timers.
static const AlarmChannelData sChannelData[kNumTimers] = //
    {                                                    //
        [kMsTimer] =
            {
                .mChannelNumber    = 0,
                .mCompareEventMask = RTC_EVTEN_COMPARE0_Msk,
                .mCompareEvent     = NRF_RTC_EVENT_COMPARE_0,
                .mCompareInt       = NRF_RTC_INT_COMPARE0_MASK,
            },
        [kUsTimer] =
            {
                .mChannelNumber    = 1,
                .mCompareEventMask = RTC_EVTEN_COMPARE1_Msk,
                .mCompareEvent     = NRF_RTC_EVENT_COMPARE_1,
                .mCompareInt       = NRF_RTC_INT_COMPARE1_MASK,
            },
        [k802154Timer] = {
            .mChannelNumber    = 2,
            .mCompareEventMask = RTC_EVTEN_COMPARE2_Msk,
            .mCompareEvent     = NRF_RTC_EVENT_COMPARE_2,
            .mCompareInt       = NRF_RTC_INT_COMPARE2_MASK,
        }};

static uint32_t OverflowCounterGet(void);

static inline bool MutexGet(void)
{
    do
    {
        volatile uint8_t mutexValue = __LDREXB(&sMutex);

        if (mutexValue)
        {
            __CLREX();
            return false;
        }
    } while (__STREXB(1, &sMutex));

    // Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority
    // context and OVERFLOW event flag is stil up.
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    __DMB();

    return true;
}

static inline void MutexRelease(void)
{
    // Re-enable OVERFLOW interrupt.
    nrf_rtc_int_enable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    __DMB();
    sMutex = 0;
}

static inline uint32_t TimeToTicks(uint64_t aTime, AlarmIndex aIndex)
{
    uint32_t ticks;

    if (aIndex == kMsTimer)
    {
        ticks = (uint32_t)CEIL_DIV((aTime * US_PER_MS * RTC_FREQUENCY), US_PER_S) & RTC_CC_COMPARE_Msk;
    }
    else
    {
        ticks = (uint32_t)CEIL_DIV((aTime * RTC_FREQUENCY), US_PER_S) & RTC_CC_COMPARE_Msk;
    }

    return ticks;
}

static inline uint64_t TicksToTime(uint32_t aTicks)
{
    return CEIL_DIV((US_PER_S * (uint64_t)aTicks), RTC_FREQUENCY);
}

static inline uint64_t AlarmGetCurrentTimeRtcProtected(AlarmIndex aIndex)
{
    uint64_t usecTime = nrf5AlarmGetCurrentTime() + 2 * US_PER_TICK;
    uint64_t currentTime;

    if (aIndex == kMsTimer)
    {
        currentTime = usecTime / US_PER_MS;
    }
    else
    {
        currentTime = usecTime;
    }

    return currentTime;
}

static inline bool AlarmShallStrike(uint64_t aNow, AlarmIndex aIndex)
{
    return aNow >= sTimerData[aIndex].mTargetTime;
}

static void HandleCompareMatch(AlarmIndex aIndex, bool aSkipCheck)
{
    nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[aIndex].mCompareEvent);

    uint64_t now = nrf5AlarmGetCurrentTime();

    if (aIndex == kMsTimer)
    {
        now /= US_PER_MS;
    }

    // In case the target time was larger than single overflow,
    // we should only strike the timer on final compare event.
    if (aSkipCheck || AlarmShallStrike(now, aIndex))
    {
        nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);
        nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);

        if (aIndex == k802154Timer)
        {
            nrf_802154_timer_fired();
        }
        else
        {
            sTimerData[aIndex].mFireAlarm = true;
            PlatformEventSignalPending();
        }
    }
}

static uint32_t OverflowCounterGet(void)
{
    uint32_t overflowCounter;

    // Get mutual access for writing to sOverflowCounter variable.
    if (MutexGet())
    {
        bool increasing = false;

        // Check if interrupt was handled already.
        if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW))
        {
            sOverflowCounter++;
            increasing = true;

            __DMB();

            // Mark that interrupt was handled.
            nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

            // Result should be incremented. sOverflowCounter will be incremented after mutex is released.
        }
        else
        {
            // Either overflow handling is not needed OR we acquired the mutex just after it was released.
            // Overflow is handled after mutex is released, but it cannot be assured that sOverflowCounter
            // was incremented for the second time, so we increment the result here.
        }

        overflowCounter = (sOverflowCounter + 1) / 2;

        MutexRelease();

        if (increasing)
        {
            // It's virtually impossible that overflow event is pending again before next instruction is performed. It
            // is an error condition.
            assert(sOverflowCounter & 0x01);

            // Increment the counter for the second time, to allow instructions from other context get correct value of
            // the counter.
            sOverflowCounter++;
        }
    }
    else
    {
        // Failed to acquire mutex.
        if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW) || (sOverflowCounter & 0x01))
        {
            // Lower priority context is currently incrementing sOverflowCounter variable.
            overflowCounter = (sOverflowCounter + 2) / 2;
        }
        else
        {
            // Lower priority context has already incremented sOverflowCounter variable or incrementing is not needed
            // now.
            overflowCounter = sOverflowCounter / 2;
        }
    }

    return overflowCounter;
}

static void AlarmStartAt(uint32_t aT0, uint32_t aDt, AlarmIndex aIndex)
{
    uint64_t now;
    uint32_t targetCounter;

    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);
    nrf_rtc_event_enable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);

    now = nrf5AlarmGetCurrentTime();

    if (aIndex == kMsTimer)
    {
        now /= US_PER_MS;
    }

    // Check if 32 LSB of `now` overflowed between getting aT0 and loading `now` value.
    if ((uint32_t)now < aT0)
    {
        now -= 0x0000000100000000;
    }

    sTimerData[aIndex].mTargetTime = (now & 0xffffffff00000000) + aT0 + aDt;

    targetCounter = TimeToTicks(sTimerData[aIndex].mTargetTime, aIndex);

    nrf_rtc_cc_set(RTC_INSTANCE, sChannelData[aIndex].mChannelNumber, targetCounter);

    now = AlarmGetCurrentTimeRtcProtected(aIndex);

    if (AlarmShallStrike(now, aIndex))
    {
        HandleCompareMatch(aIndex, true);

        /**
         * Normally ISR sets event flag automatically.
         * Here we are calling HandleCompareMatch explicitly and no ISR will be fired.
         * To prevent possible permanent sleep on next WFE we have to set event flag.
         */
        __SEV();
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
    memset(sTimerData, 0, sizeof(sTimerData));
    sOverflowCounter = 0;
    sMutex           = 0;
    sTimeOffset      = 0;

    // Setup low frequency clock.
    nrf_drv_clock_lfclk_request(NULL);

    while (!nrf_drv_clock_lfclk_is_running())
    {
    }

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
            otPlatAlarmMilliFired(aInstance);
        }
    }

    if (sTimerData[kUsTimer].mFireAlarm)
    {
        sTimerData[kUsTimer].mFireAlarm = false;

        otPlatAlarmMicroFired(aInstance);
    }
}

inline uint64_t nrf5AlarmGetCurrentTime(void)
{
    uint32_t offset1 = OverflowCounterGet();

    __DMB();

    uint32_t rtcValue1 = nrf_rtc_counter_get(RTC_INSTANCE);

    __DMB();

    uint32_t offset2 = OverflowCounterGet();

    __DMB();

    uint32_t rtcValue2 = nrf_rtc_counter_get(RTC_INSTANCE);

    if (offset1 == offset2)
    {
        return (uint64_t)offset1 * US_PER_OVERFLOW + TicksToTime(rtcValue1);
    }
    else
    {
        return (uint64_t)offset2 * US_PER_OVERFLOW + TicksToTime(rtcValue2);
    }
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(nrf5AlarmGetCurrentTime() / US_PER_MS);
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;

    AlarmStartAt(aT0, aDt, kMsTimer);
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    (void)aInstance;

    AlarmStop(kMsTimer);
}

uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)nrf5AlarmGetCurrentTime();
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    (void)aInstance;

    AlarmStartAt(aT0, aDt, kUsTimer);
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    (void)aInstance;

    AlarmStop(kUsTimer);
}

/**
 * Radio driver timer abstraction API
 */

void nrf_802154_timer_init(void)
{
    // Intentionally empty
}

void nrf_802154_timer_deinit(void)
{
    // Intentionally empty
}

void nrf_802154_timer_critical_section_enter(void)
{
    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
    __DSB();
    __ISB();
}

void nrf_802154_timer_critical_section_exit(void)
{
    nrf_rtc_int_enable(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
}

uint32_t nrf_802154_timer_time_get(void)
{
    return (uint32_t)nrf5AlarmGetCurrentTime();
}

uint32_t nrf_802154_timer_granularity_get(void)
{
    return US_PER_TICK;
}

void nrf_802154_timer_start(uint32_t t0, uint32_t dt)
{
    AlarmStartAt(t0, dt, k802154Timer);
}

void nrf_802154_timer_stop(void)
{
    AlarmStop(k802154Timer);
}

bool nrf_802154_timer_is_running(void)
{
    return nrf_rtc_int_is_enabled(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
}

/**
 * RTC IRQ handler
 */

void RTC_IRQ_HANDLER(void)
{
    // Handle overflow.
    if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW))
    {
        // Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority
        // context and OVERFLOW event flag is stil up. OVERFLOW interrupt will be re-enabled when mutex is released -
        // either from this handler, or from lower priority context, that locked the mutex.
        nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

        // Handle OVERFLOW event by reading current value of overflow counter.
        (void)OverflowCounterGet();
    }

    // Handle compare match.
    for (uint32_t i = 0; i < kNumTimers; i++)
    {
        if (nrf_rtc_int_is_enabled(RTC_INSTANCE, sChannelData[i].mCompareInt) &&
            nrf_rtc_event_pending(RTC_INSTANCE, sChannelData[i].mCompareEvent))
        {
            HandleCompareMatch((AlarmIndex)i, false);
        }
    }
}
