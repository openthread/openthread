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

#include <stddef.h>
#include "utils/wrap_stdint.h"

#include <openthread/types.h>
#include <openthread/platform/alarm.h>

#include "common/debug.hpp"
#include "common/tasklet.hpp"

namespace ot {

namespace Ip6 { class Ip6; }

class Timer;

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
 * This class implements the timer scheduler.
 *
 */
class TimerScheduler
{
    friend class Timer;

public:
    /**
     * This constructor initializes the object.
     *
     */
    TimerScheduler(void);

    /**
     * This method adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Add(Timer &aTimer);

    /**
     * This method removes a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Remove(Timer &aTimer);

    /**
     * This method processes the running timers.
     *
     */
    void ProcessTimers(void);

private:
    /**
     * This method sets the platform alarm based on timer at front of the list.
     *
     */
    void SetAlarm(void);

    /**
     * This method returns the pointer to the parent Ip6 structure.
     *
     * @returns The pointer to the parent Ip6 structure.
     *
     */
    Ip6::Ip6 *GetIp6(void);

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

    Timer *mHead;
};

/**
 * This class implements a timer.
 *
 */
class Timer
{
    friend class TimerScheduler;

public:

    enum
    {
        kMaxDt = (1UL << 31) - 1,  //< Maximum permitted value for parameter `aDt` in `Start` and `StartAt` method.
    };

    /**
     * This function pointer is called when the timer expires.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    typedef void (*Handler)(void *aContext);

    /**
     * This constructor creates a timer instance.
     *
     * @param[in]  aScheduler  A reference to the timer scheduler.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aContext    A pointer to arbitrary context information.
     *
     */
    Timer(TimerScheduler &aScheduler, Handler aHandler, void *aContext):
        mScheduler(aScheduler),
        mHandler(aHandler),
        mContext(aContext),
        mFireTime(0),
        mNext(this) {
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
    void StartAt(uint32_t aT0, uint32_t aDt) { assert(aDt <= kMaxDt); mFireTime = aT0 + aDt; mScheduler.Add(*this); }

    /**
     * This method stops the timer.
     *
     */
    void Stop(void) { mScheduler.Remove(*this); }

    /**
     * This static method returns the current time in milliseconds.
     *
     * @returns The current time in milliseconds.
     *
     */
    static uint32_t GetNow(void) { return otPlatAlarmGetNow(); }

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

    /**
     * This static method returns the number of milliseconds given hours.
     *
     * @returns The number of milliseconds.
     *
     */
    static uint32_t HoursToMsec(uint32_t aHours) { return SecToMsec(aHours * 3600u); }

    /**
     * This static method returns the number of hours given milliseconds.
     *
     * @returns The number of hours.
     *
     */
    static uint32_t MsecToHours(uint32_t aMilliseconds) { return MsecToSec(aMilliseconds / 3600u); }

private:
    /**
     * This method indicates if the fire time of this timer is strictly before the fire time of a second given timer.
     *
     * @param[in]  aTimer   A reference to the second timer object.
     *
     * @retval TRUE  If the fire time of this timer object is strictly before aTimer's fire time
     * @retval FALSE If the fire time of this timer object is the same or after aTimer's fire time.
     *
     */
    bool DoesFireBefore(const Timer &aTimer);

    void Fired(void) { mHandler(mContext); }

    TimerScheduler &mScheduler;
    Handler         mHandler;
    void           *mContext;
    uint32_t        mFireTime;
    Timer          *mNext;
};

/**
 * @}
 *
 */

}  // namespace ot

#endif  // TIMER_HPP_
