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
 *   This file includes definitions for the multiplexed timer service.
 */

#ifndef TIMER_HPP_
#define TIMER_HPP_

#include "openthread-core-config.h"

#include <stddef.h>
#include "utils/wrap_stdint.h"

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>

#include "common/debug.hpp"
#include "common/locator.hpp"
#include "common/tasklet.hpp"

namespace ot {

class TimerMilliScheduler;

/**
 * @addtogroup core-timer
 *
 * @brief
 *   This module includes definitions for the multiplexed timer service.
 *
 * @{
 *
 */

/**
 * This class implements a timer.
 *
 */
class Timer : public InstanceLocator, public OwnerLocator
{
    friend class TimerScheduler;

public:
    enum
    {
        kMaxDt = (1UL << 31) - 1, //< Maximum permitted value for parameter `aDt` in `Start` and `StartAt` method.
    };

    /**
     * This function pointer is called when the timer expires.
     *
     * @param[in]  aTimer    A reference to the expired timer instance.
     *
     */
    typedef void (*Handler)(Timer &aTimer);

    /**
     * This constructor creates a timer instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aOwner      A pointer to owner of the `Timer` object.
     *
     */
    Timer(Instance &aInstance, Handler aHandler, void *aOwner)
        : InstanceLocator(aInstance)
        , OwnerLocator(aOwner)
        , mHandler(aHandler)
        , mFireTime(0)
        , mNext(this)
    {
    }

    /**
     * This method returns the fire time of the timer.
     *
     * @returns The fire time in milliseconds.
     *
     */
    uint32_t GetFireTime(void) const { return mFireTime; }

    /**
     * This method indicates whether or not the timer instance is running.
     *
     * @retval TRUE   If the timer is running.
     * @retval FALSE  If the timer is not running.
     *
     */
    bool IsRunning(void) const { return (mNext != this); }

protected:
    /**
     * This method indicates if the fire time of this timer is strictly before the fire time of a second given timer.
     *
     * @param[in]  aTimer   A reference to the second timer object.
     * @param[in]  aNow     The current time (may in milliseconds or microsecond, which depends on the timer type).
     *
     * @retval TRUE  If the fire time of this timer object is strictly before aTimer's fire time
     * @retval FALSE If the fire time of this timer object is the same or after aTimer's fire time.
     *
     */
    bool DoesFireBefore(const Timer &aTimer, uint32_t aNow);

    void Fired(void) { mHandler(*this); }

    Handler  mHandler;
    uint32_t mFireTime;
    Timer *  mNext;
};

/**
 * This class implements the millisecond timer.
 *
 */
class TimerMilli : public Timer
{
public:
    /**
     * This constructor creates a millisecond timer instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aOwner      A pointer to the owner of the `TimerMilli` object.
     *
     */
    TimerMilli(Instance &aInstance, Handler aHandler, void *aOwner)
        : Timer(aInstance, aHandler, aOwner)
    {
    }

    /**
     * This method schedules the timer to fire a @p dt milliseconds from now.
     *
     * @param[in]  aDt  The expire time in milliseconds from now.
     *                  (aDt must be smaller than or equal to kMaxDt).
     *
     */
    void Start(uint32_t aDt) { StartAt(GetNow(), aDt); }

    /**
     * This method schedules the timer to fire at @p aDt milliseconds from @p aT0.
     *
     * @param[in]  aT0  The start time in milliseconds.
     * @param[in]  aDt  The expire time in milliseconds from @p aT0.
     *                  (aDt must be smaller than or equal to kMaxDt).
     *
     */
    void StartAt(uint32_t aT0, uint32_t aDt);

    /**
     * This method stops the timer.
     *
     */
    void Stop(void);

    /**
     * This static method returns the current time in milliseconds.
     *
     * @returns The current time in milliseconds.
     *
     */
    static uint32_t GetNow(void) { return otPlatAlarmMilliGetNow(); }

    /**
     * This static method returns the number of milliseconds given seconds.
     *
     * @returns The number of milliseconds.
     *
     */
    static uint32_t SecToMsec(uint32_t aSeconds) { return aSeconds * 1000u; }

    /**
     * This static method returns the number of seconds given milliseconds.
     *
     * @returns The number of seconds.
     *
     */
    static uint32_t MsecToSec(uint32_t aMilliseconds) { return aMilliseconds / 1000u; }

private:
    /**
     * This method returns a reference to the TimerMilliScheduler.
     *
     * @returns   A reference to the TimerMilliScheduler.
     *
     */
    TimerMilliScheduler &GetTimerMilliScheduler(void) const;
};

/**
 * This class implements a millisecond timer that also maintains a user context pointer.
 *
 * In typical `TimerMilli`/`TimerMicro` use, in the timer callback handler, the owner of the timer is determined using
 * `GetOwner<Type>` method. This method works if there is a single instance of `Type` within OpenThread instance
 * hierarchy. The `TimerMilliContext` is intended for cases where there may be multiple instances of the same class/type
 * using a timer object. `TimerMilliContext` will store a context `void *` information.
 *
 */
class TimerMilliContext : public TimerMilli
{
public:
    /**
     * This constructor creates a millisecond timer that also maintains a user context pointer.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aContext    A pointer to an arbitrary context information.
     *
     */
    TimerMilliContext(Instance &aInstance, Handler aHandler, void *aContext)
        : TimerMilli(aInstance, aHandler, aContext)
        , mContext(aContext)
    {
    }

    /**
     * This method returns the pointer to the arbitrary context information.
     *
     * @returns Pointer to the arbitrary context information.
     *
     */
    void *GetContext(void) { return mContext; }

private:
    void *mContext;
};

/**
 * This class implements the base timer scheduler.
 *
 */
class TimerScheduler : public InstanceLocator
{
    friend class Timer;

public:
    /**
     * This static method compares two times and indicates if the first time is strictly before (earlier) than the
     * second time.
     *
     * This method requires that the difference between the two given times to be smaller than kMaxDt.
     *
     * @param[in] aTimerA   The first time for comparison.
     * @param[in] aTimerB   The second time for comparison.
     *
     * @returns TRUE  if aTimeA is before aTimeB.
     * @returns FALSE if aTimeA is same time or after aTimeB.
     *
     */
    static bool IsStrictlyBefore(uint32_t aTimeA, uint32_t aTimeB);

protected:
    /**
     * The Alarm APIs definition
     *
     */
    struct AlarmApi
    {
        void (*AlarmStartAt)(otInstance *aInstance, uint32_t aT0, uint32_t aDt);
        void (*AlarmStop)(otInstance *aInstance);
        uint32_t (*AlarmGetNow)(void);
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the instance object.
     *
     */
    TimerScheduler(Instance &aInstance)
        : InstanceLocator(aInstance)
        , mHead(NULL)
    {
    }

    /**
     * This method adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer     A reference to the timer instance.
     * @param[in]  aAlarmApi  A reference to the Alarm APIs.
     *
     */
    void Add(Timer &aTimer, const AlarmApi &aAlarmApi);

    /**
     * This method removes a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer     A reference to the timer instance.
     * @param[in]  aAlarmApi  A reference to the Alarm APIs.
     *
     */
    void Remove(Timer &aTimer, const AlarmApi &aAlarmApi);

    /**
     * This method processes the running timers.
     *
     * @param[in]  aAlarmApi  A reference to the Alarm APIs.
     *
     */
    void ProcessTimers(const AlarmApi &aAlarmApi);

    /**
     * This method sets the platform alarm based on timer at front of the list.
     *
     * @param[in]  aAlarmApi  A reference to the Alarm APIs.
     *
     */
    void SetAlarm(const AlarmApi &aAlarmApi);

    Timer *mHead;
};

/**
 * This class implements the millisecond timer scheduler.
 *
 */
class TimerMilliScheduler : public TimerScheduler
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the instance object.
     *
     */
    TimerMilliScheduler(Instance &aInstance)
        : TimerScheduler(aInstance)
    {
    }

    /**
     * This method adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Add(TimerMilli &aTimer) { TimerScheduler::Add(aTimer, sAlarmMilliApi); }

    /**
     * This method removes a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Remove(TimerMilli &aTimer) { TimerScheduler::Remove(aTimer, sAlarmMilliApi); };

    /**
     * This method processes the running timers.
     *
     */
    void ProcessTimers(void) { TimerScheduler::ProcessTimers(sAlarmMilliApi); }

private:
    static const AlarmApi sAlarmMilliApi;
};

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
class TimerMicroScheduler;

/**
 * This class implements the microsecond timer.
 *
 */
class TimerMicro : public Timer
{
public:
    /**
     * This constructor creates a timer instance.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aOwner      A pointer to owner of the `TimerMicro` object.
     *
     */
    TimerMicro(Instance &aInstance, Handler aHandler, void *aOwner)
        : Timer(aInstance, aHandler, aOwner)
    {
    }

    /**
     * This method schedules the timer to fire a @p aDt microseconds from now.
     *
     * @param[in]  aDt  The expire time in microseconds from now.
     *                  (aDt must be smaller than or equal to kMaxDt).
     *
     */
    void Start(uint32_t aDt) { StartAt(GetNow(), aDt); }

    /**
     * This method schedules the timer to fire at @p aDt microseconds from @p aT0.
     *
     * @param[in]  aT0  The start time in microseconds.
     * @param[in]  aDt  The expire time in microseconds from @p aT0.
     *                  (aDt must be smaller than or equal to kMaxDt).
     *
     */
    void StartAt(uint32_t aT0, uint32_t aDt);

    /**
     * This method stops the timer.
     *
     */
    void Stop(void);

    /**
     * This static method returns the current time in microseconds.
     *
     * @returns The current time in microseconds.
     *
     */
    static uint32_t GetNow(void) { return otPlatAlarmMicroGetNow(); }

private:
    /**
     * This method returns a reference to the TimerMicroScheduler.
     *
     * @returns   A reference to the TimerMicroScheduler.
     *
     */
    TimerMicroScheduler &GetTimerMicroScheduler(void) const;
};

/**
 * This class implements the microsecond timer scheduler.
 *
 */
class TimerMicroScheduler : public TimerScheduler
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the instance object.
     *
     */
    TimerMicroScheduler(Instance &aInstance)
        : TimerScheduler(aInstance)
    {
    }

    /**
     * This method adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Add(TimerMicro &aTimer) { TimerScheduler::Add(aTimer, sAlarmMicroApi); }

    /**
     * This method removes a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Remove(TimerMicro &aTimer) { TimerScheduler::Remove(aTimer, sAlarmMicroApi); };

    /**
     * This method processes the running timers.
     *
     */
    void ProcessTimers(void) { TimerScheduler::ProcessTimers(sAlarmMicroApi); }

private:
    static const AlarmApi sAlarmMicroApi;
};
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

/**
 * @}
 *
 */

} // namespace ot

#endif // TIMER_HPP_
