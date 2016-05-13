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
 *   This file includes definitions for the multiplexed timer service.
 */

#ifndef TIMER_HPP_
#define TIMER_HPP_

#include <stddef.h>
#include <stdint.h>

#include <openthread-types.h>
#include <common/tasklet.hpp>
#include <platform/alarm.h>

namespace Thread {

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
     * This static method adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    static void Add(Timer &aTimer);

    /**
     * This static method removes a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    static void Remove(Timer &aTimer);

    /**
     * This static method returns whether or not the timer instance is already added.
     *
     * @retval TRUE   If the timer instance is already added.
     * @retval FALSE  If the timer instance is not added.
     *
     */
    static bool IsAdded(const Timer &aTimer);

    /**
     * This static method processes all running timers.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    static void FireTimers(void *aContext);

private:
    static void SetAlarm(void);
};

/**
 * This class implements a timer.
 *
 */
class Timer
{
    friend class TimerScheduler;

public:
    /**
     * This function pointer is called when the timer expires.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    typedef void (*Handler)(void *aContext);

    /**
     * This constructor creates a timer instance.
     *
     * @param[in]  aHandler  A pointer to a function that is called when the timer expires.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    Timer(Handler aHandler, void *aContext) { mNext = NULL; mHandler = aHandler; mContext = aContext; }

    /**
     * This method returns the start time in milliseconds for the timer.
     *
     * @returns The start time in milliseconds.
     *
     */
    uint32_t Gett0(void) const { return mT0; }

    /**
     * This method returns the delta time in milliseconds for the timer.
     *
     * @returns The delta time.
     *
     */
    uint32_t Getdt(void) const { return mDt; }

    /**
     * This method indicates whether or not the timer instance is running.
     *
     * @retval TRUE   If the timer is running.
     * @retval FALSE  If the timer is not running.
     */
    bool IsRunning(void) const { return TimerScheduler::IsAdded(*this); }

    /**
     * This method schedules the timer to fire a @p dt milliseconds from now.
     *
     * @param[in]  aDt  The expire time in milliseconds from now.
     */
    void Start(uint32_t aDt) { StartAt(GetNow(), aDt); }

    /**
     * This method schedules the timer to fire at @p dt milliseconds from @p t0.
     *
     * @param[in]  aT0  The start time in milliseconds.
     * @param[in]  aDt  The expire time in milliseconds from @p t0.
     */
    void StartAt(uint32_t aT0, uint32_t aDt) { mT0 = aT0; mDt = aDt; TimerScheduler::Add(*this); }

    /**
     * This method stops the timer.
     *
     */
    void Stop(void) { TimerScheduler::Remove(*this); }

    /**
     * This static method returns the current time in milliseconds.
     *
     * @returns The current time in milliseconds.
     *
     */
    static uint32_t GetNow(void) { return otPlatAlarmGetNow(); }

    /**
     * This static method returns the number of milliseonds given seconds.
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
    void Fired(void) { mHandler(mContext); }

    Handler   mHandler;   ///< A pointer to the function that is called when the timer expires.
    void     *mContext;   ///< A pointer to arbitrary context information.
    uint32_t  mT0;        ///< The start time of the timer in milliseconds.
    uint32_t  mDt;        ///< The time delay from the start time in milliseconds.
    Timer    *mNext;      ///< The next timer in the scheduler list.
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // TIMER_HPP_
