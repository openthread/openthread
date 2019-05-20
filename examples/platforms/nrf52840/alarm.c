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
#include <openthread/platform/time.h>

#include "openthread-system.h"

#include "platform-config.h"
#include "platform-nrf5.h"
#include "cmsis/core_cmFunc.h"

#include <drivers/clock/nrf_drv_clock.h>
#include <drivers/radio/nrf_802154_utils.h>
#include <drivers/radio/platform/lp_timer/nrf_802154_lp_timer.h>

#include <hal/nrf_rtc.h>

#include <openthread/config.h>

// clang-format off
#define RTC_FREQUENCY       NRF_802154_RTC_FREQUENCY

#define US_PER_MS           1000ULL
#define US_PER_S            NRF_802154_US_PER_S
#define US_PER_OVERFLOW     (512UL * NRF_802154_US_PER_S)  ///< Time that has passed between overflow events. On full RTC speed, it occurs every 512 s.

#define MS_PER_S            1000UL

#define MIN_RTC_COMPARE_EVENT_TICKS  2                                                        ///< Minimum number of RTC ticks delay that guarantees that RTC compare event will fire.
#define MIN_RTC_COMPARE_EVENT_DT     (MIN_RTC_COMPARE_EVENT_TICKS * NRF_802154_US_PER_TICK)   ///< Minimum time delta from now before RTC compare event is guaranteed to fire.
#define EPOCH_32BIT_US               (1ULL << 32)
#define EPOCH_FROM_TIME(time)        ((time) & ((uint64_t)UINT32_MAX << 32))

#define XTAL_ACCURACY       40 // The crystal used on nRF52840PDK has ±20ppm accuracy.
// clang-format on

typedef enum
{
    kMsTimer,
    kUsTimer,
    k802154Timer,
    k802154Sync,
    kNumTimers
} AlarmIndex;

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
static volatile bool     sEventPending;    ///< Timer fired and upper layer should be notified.
static AlarmData         sTimerData[kNumTimers]; ///< Data of the timers.

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
        [k802154Timer] =
            {
                .mChannelNumber    = 2,
                .mCompareEventMask = RTC_EVTEN_COMPARE2_Msk,
                .mCompareEvent     = NRF_RTC_EVENT_COMPARE_2,
                .mCompareInt       = NRF_RTC_INT_COMPARE2_MASK,
            },
        [k802154Sync] = {
            .mChannelNumber    = 3,
            .mCompareEventMask = RTC_EVTEN_COMPARE3_Msk,
            .mCompareEvent     = NRF_RTC_EVENT_COMPARE_3,
            .mCompareInt       = NRF_RTC_INT_COMPARE3_MASK,
        }};

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

static inline uint64_t TimeToTicks(uint64_t aTime, AlarmIndex aIndex)
{
    if (aIndex == kMsTimer)
    {
        aTime *= US_PER_MS;
    }

    return NRF_802154_US_TO_RTC_TICKS(aTime);
}

static inline uint64_t TicksToTime(uint64_t aTicks, AlarmIndex aIndex)
{
    uint64_t result = NRF_802154_RTC_TICKS_TO_US(aTicks);

    if (aIndex == kMsTimer)
    {
        result /= US_PER_MS;
    }

    return result;
}

static inline bool AlarmShallStrike(uint64_t aNow, AlarmIndex aIndex)
{
    return aNow >= sTimerData[aIndex].mTargetTime;
}

static uint32_t GetOverflowCounter(void)
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

static uint32_t GetRtcCounter(void)
{
    return nrf_rtc_counter_get(RTC_INSTANCE);
}

static void GetOffsetAndCounter(uint32_t *aOffset, uint32_t *aCounter)
{
    uint32_t offset1 = GetOverflowCounter();

    __DMB();

    uint32_t rtcValue1 = GetRtcCounter();

    __DMB();

    uint32_t offset2 = GetOverflowCounter();

    *aOffset  = offset2;
    *aCounter = (offset1 == offset2) ? rtcValue1 : GetRtcCounter();
}

static uint64_t GetTime(uint32_t aOffset, uint32_t aCounter, AlarmIndex aIndex)
{
    uint64_t result = (uint64_t)aOffset * US_PER_OVERFLOW + TicksToTime(aCounter, kUsTimer);

    if (aIndex == kMsTimer)
    {
        result /= US_PER_MS;
    }

    return result;
}

static uint64_t GetCurrentTime(AlarmIndex aIndex)
{
    uint32_t offset;
    uint32_t rtc_counter;

    GetOffsetAndCounter(&offset, &rtc_counter);

    return GetTime(offset, rtc_counter, aIndex);
}

static void HandleCompareMatch(AlarmIndex aIndex, bool aSkipCheck)
{
    nrf_rtc_event_clear(RTC_INSTANCE, sChannelData[aIndex].mCompareEvent);

    uint64_t now = GetCurrentTime(aIndex);

    // In case the target time was larger than single overflow,
    // we should only strike the timer on final compare event.
    if (aSkipCheck || AlarmShallStrike(now, aIndex))
    {
        nrf_rtc_event_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);
        nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);

        switch (aIndex)
        {
        case k802154Timer:
            nrf_802154_lp_timer_fired();
            break;

        case k802154Sync:
            nrf_802154_lp_timer_synchronized();
            break;

        case kMsTimer:
        case kUsTimer:
            sTimerData[aIndex].mFireAlarm = true;
            sEventPending                 = true;
            otSysEventSignalPending();
            break;

        default:
            assert(false);
        }
    }
}

static uint64_t ConvertT0AndDtTo64BitTime(uint32_t aT0, uint32_t aDt, const uint64_t *aNow)
{
    uint64_t now;
    now = *aNow;

    if (((uint32_t)now < aT0) && ((aT0 - (uint32_t)now) > (UINT32_MAX / 2)))
    {
        now -= EPOCH_32BIT_US;
    }
    else if (((uint32_t)now > aT0) && (((uint32_t)now) - aT0 > (UINT32_MAX / 2)))
    {
        now += EPOCH_32BIT_US;
    }

    return (EPOCH_FROM_TIME(now)) + aT0 + aDt;
}

static uint64_t RoundUpTimeToTimerTicksMultiply(uint64_t aTime, AlarmIndex aIndex)
{
    uint64_t ticks  = TimeToTicks(aTime, aIndex);
    uint64_t result = TicksToTime(ticks, aIndex);
    return result;
}

static void TimerStartAt(uint32_t aT0, uint32_t aDt, AlarmIndex aIndex, const uint64_t *aNow)
{
    uint64_t targetCounter;
    uint64_t targetTime;

    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[aIndex].mCompareInt);
    nrf_rtc_event_enable(RTC_INSTANCE, sChannelData[aIndex].mCompareEventMask);

    targetTime    = ConvertT0AndDtTo64BitTime(aT0, aDt, aNow);
    targetCounter = TimeToTicks(targetTime, aIndex) & RTC_CC_COMPARE_Msk;

    sTimerData[aIndex].mTargetTime = RoundUpTimeToTimerTicksMultiply(targetTime, aIndex);

    nrf_rtc_cc_set(RTC_INSTANCE, sChannelData[aIndex].mChannelNumber, targetCounter);
}

static void AlarmStartAt(uint32_t aT0, uint32_t aDt, AlarmIndex aIndex)
{
    uint32_t offset;
    uint32_t rtc_value;
    uint64_t now;
    uint64_t now_rtc_protected;

    GetOffsetAndCounter(&offset, &rtc_value);
    now = GetTime(offset, rtc_value, aIndex);

    TimerStartAt(aT0, aDt, aIndex, &now);

    if (rtc_value != GetRtcCounter())
    {
        GetOffsetAndCounter(&offset, &rtc_value);
    }

    now_rtc_protected = GetTime(offset, rtc_value + MIN_RTC_COMPARE_EVENT_TICKS, aIndex);

    if (AlarmShallStrike(now_rtc_protected, aIndex))
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

static void TimerSyncStartAt(uint32_t aT0, uint32_t aDt, const uint64_t *aNow)
{
    TimerStartAt(aT0, aDt, k802154Sync, aNow);

    nrf_rtc_int_enable(RTC_INSTANCE, sChannelData[k802154Sync].mCompareInt);
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

    nrf_802154_lp_timer_sync_stop();

    NVIC_DisableIRQ(RTC_IRQN);
    NVIC_ClearPendingIRQ(RTC_IRQN);
    NVIC_SetPriority(RTC_IRQN, 0);

    nrf_drv_clock_lfclk_release();
}

void nrf5AlarmProcess(otInstance *aInstance)
{
    do
    {
        sEventPending = false;

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
    } while (sEventPending);
}

inline uint64_t nrf5AlarmGetCurrentTime(void)
{
    return GetCurrentTime(kUsTimer);
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(nrf5AlarmGetCurrentTime() / US_PER_MS);
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    AlarmStartAt(aT0, aDt, kMsTimer);
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    AlarmStop(kMsTimer);
}

uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)nrf5AlarmGetCurrentTime();
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    AlarmStartAt(aT0, aDt, kUsTimer);
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    AlarmStop(kUsTimer);
}

/**
 * Radio driver timer abstraction API
 */

void nrf_802154_lp_timer_init(void)
{
    // Intentionally empty
}

void nrf_802154_lp_timer_deinit(void)
{
    // Intentionally empty
}

void nrf_802154_lp_timer_critical_section_enter(void)
{
    nrf_rtc_int_disable(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
    __DSB();
    __ISB();
}

void nrf_802154_lp_timer_critical_section_exit(void)
{
    nrf_rtc_int_enable(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
}

uint32_t nrf_802154_lp_timer_time_get(void)
{
    return (uint32_t)nrf5AlarmGetCurrentTime();
}

uint32_t nrf_802154_lp_timer_granularity_get(void)
{
    return NRF_802154_US_PER_TICK;
}

void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt)
{
    AlarmStartAt(t0, dt, k802154Timer);
}

bool nrf_802154_lp_timer_is_running(void)
{
    return nrf_rtc_int_is_enabled(RTC_INSTANCE, sChannelData[k802154Timer].mCompareInt);
}

void nrf_802154_lp_timer_stop(void)
{
    AlarmStop(k802154Timer);
}

void nrf_802154_lp_timer_sync_start_now(void)
{
    uint32_t counter;
    uint32_t offset;
    uint64_t now;

    do
    {
        GetOffsetAndCounter(&offset, &counter);
        now = GetTime(offset, counter, k802154Sync);
        TimerSyncStartAt((uint32_t)now, MIN_RTC_COMPARE_EVENT_DT, &now);
    } while (GetRtcCounter() != counter);
}

void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt)
{
    uint64_t now = GetCurrentTime(k802154Sync);

    TimerSyncStartAt(t0, dt, &now);
}

void nrf_802154_lp_timer_sync_stop(void)
{
    AlarmStop(k802154Sync);
}

uint32_t nrf_802154_lp_timer_sync_event_get(void)
{
    return (uint32_t)nrf_rtc_event_address_get(RTC_INSTANCE, sChannelData[k802154Sync].mCompareEvent);
}

uint32_t nrf_802154_lp_timer_sync_time_get(void)
{
    return (uint32_t)sTimerData[k802154Sync].mTargetTime;
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
        (void)GetOverflowCounter();
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

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
uint64_t otPlatTimeGet(void)
{
    return nrf5AlarmGetCurrentTime();
}

uint16_t otPlatTimeGetXtalAccuracy(void)
{
    return XTAL_ACCURACY;
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
