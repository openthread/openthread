/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for timer.
 *
 */

#ifndef POSIX_TIMER_HPP_
#define POSIX_TIMER_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <stdint.h>

namespace ot {

namespace Posix {

/**
 * Defines Milliseconds as `uint64_t` because the valid lifetime of
 * prefix information options is defined as `uint32_t` in unit of seconds.
 *
 */
typedef uint64_t Milliseconds;

/**
 * This class implements a timer.
 *
 */
class Timer
{
public:
    friend class TimerScheduler;

    /**
     * This function represents the handler of the timer event.
     *
     * @param[in]  aTimer    A reference to the expired timer instance.
     * @param[in]  aContext  The arbitrary context associated with the handler.
     *
     */
    typedef void (*Handler)(Timer &aTimer, void *aContext);

    /**
     * This constructor initializes the timer with given handler and context.
     *
     * @param[in]  aHandler  A function to be called when the timer fires.
     * @param[in]  aContext  A arbitrary context associated with @p aHandler.
     *
     */
    Timer(Handler aHandler, void *aContext);

    /**
     * This method starts the timer with given delay.
     *
     * @param[in]  aDelay  The delay in milliseconds.
     *
     */
    void Start(Milliseconds aDelay);

    /**
     * This method starts the timer with given fire time.
     *
     * @param[in]  aFireTime  The fire time in milliseconds.
     *
     */
    void StartAt(Milliseconds aFireTime);

    /**
     * This method stops a timer.
     *
     */
    void Stop();

    /**
     * This method devices if the timer is running.
     *
     * @returns  A boolean indicates whether the timer is running.
     *
     */
    bool IsRunning() const { return mIsRunning; }

    /**
     * this method returns the fire time.
     *
     * @returns  The fire time.
     *
     */
    Milliseconds GetFireTime() const { return mFireTime; }

private:
    void Fire();

    Handler      mHandler;
    void *       mContext;
    Milliseconds mFireTime;
    bool         mIsRunning;
    Timer *      mNext;
};

/**
 * This class implements a timer scheduler which accepts registration of
 * timer events and drives them.
 *
 */
class TimerScheduler
{
public:
    /**
     * This method returns the TimerScheduler singleton.
     *
     * @return A single, global timer scheduler.
     *
     */
    static TimerScheduler &Get();

    /**
     * This method processes all timer events.
     *
     * @param[in]  aNow  The current time.
     *
     */
    void Process(Milliseconds aNow);

    /**
     * This method returns the earliest fire time of all timers.
     *
     * @return  The earliest fire time.
     *
     */
    Milliseconds GetEarliestFireTime() const;

    /**
     * This method adds a new timer into the scheduler.
     *
     * @param[in]  aTimer  The timer to be added.
     *
     */
    void Add(Timer *aTimer);

    /**
     * This method removes a timer.
     *
     * @param[in]  aTimer  The timer to be removed.
     *
     */
    void Remove(Timer *aTimer);

private:
    TimerScheduler()
        : mSortedTimerList(nullptr)
    {
    }

    Timer *mSortedTimerList; ///< The timer list sorted by their Fire Time. Earliest timer comes first.
};

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // POSIX_TIMER_HPP_
