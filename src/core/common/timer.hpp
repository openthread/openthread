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
#include <stdint.h>

#include <openthread-types.h>
#include <common/tasklet.hpp>
#include <platform/alarm.h>

namespace Thread {

namespace Ip6 { class Ip6; }

class Timer;
class TimerUs;

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
 * This class implements the precise time (microseconds) type.
 *
 */
class Time: public otPlatAlarmTime
{
public:
    Time() { mMs = 0; mUs = 0; }
    Time(uint32_t aMilliseconds) { mMs = aMilliseconds; mUs = 0; }
    Time(uint32_t aMilliseconds, uint16_t aMicroseconds) { mMs = aMilliseconds; mUs = aMicroseconds; }
    Time(const Time &aTime) { mMs = aTime.mMs; mUs = aTime.mUs; }

    Time operator+(const Time &aOther) const;
    Time operator-(const Time &aOther) const;

    bool operator>(const Time &aOther) const;
    bool operator>=(const Time &aOther) const;
    bool operator<(const Time &aOther) const;
    bool operator<=(const Time &aOther) const;

    /**
     * This static method returns the current time in milliseconds.
     *
     * @returns The current time in milliseconds.
     *
     */
    static uint32_t GetNow(void) { return otPlatAlarmGetNow(); }

    /**
     * This static method returns the current time in milliseconds.
     *
     * @param[out]  aNow  The current time in milliseconds.
     *
     */
    static void GetNow(uint32_t &aNow) { aNow = otPlatAlarmGetNow(); }

    /**
     * This static method returns the current time in milliseconds and microseconds.
     *
     * @param[out]  aNow  The current time in milliseconds and microseconds.
     *
     */
    static void GetNow(Time &aNow) { otPlatAlarmGetPreciseNow(static_cast<otPlatAlarmTime *>(&aNow)); }

};

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
     * This method adds a precise timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the precise timer instance.
     *
     */
    void Add(TimerUs &aTimer);

    /**
     * This method removes a timer instance from the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    void Remove(Timer &aTimer);

    /**
     * This method removes a precise timer instance from the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the precise timer instance.
     *
     */
    void Remove(TimerUs &aTimer);

    /**
     * This method returns whether or not the timer instance is already added.
     *
     * @retval TRUE   If the timer instance is already added.
     * @retval FALSE  If the timer instance is not added.
     *
     */
    bool IsAdded(const Timer &aTimer);

    /**
     * This method returns whether or not the precise timer instance is already added.
     *
     * @retval TRUE   If the timer instance is already added.
     * @retval FALSE  If the timer instance is not added.
     *
     */
    bool IsAdded(const TimerUs &aTimer);

    /**
     * This method processes all running timers.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    void FireTimers(void);

    /**
     * This method returns the pointer to the parent Ip6 structure.
     *
     * @returns The pointer to the parent Ip6 structure.
     *
     */
    Ip6::Ip6 *GetIp6();

private:
    void SetAlarm(void);

    /**
     * This template adds a timer instance to the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    template <class T> void TemplateAdd(T &aTimer);

    /**
     * This template removes a timer instance from the timer scheduler.
     *
     * @param[in]  aTimer  A reference to the timer instance.
     *
     */
    template <class T> void TemplateRemove(T &aTimer);

    /**
     * This template returns whether or not the timer instance is already added.
     *
     * @retval TRUE   If the timer instance is already added.
     * @retval FALSE  If the timer instance is not added.
     *
     */
    template <class T> bool TemplateIsAdded(const T &aTimer);

    /**
     * This template compares two timers of the same type and returns a value to indicate
     * which timer will fire earlier.
     *
     * The template is used to create compare functions that can compare TimerUs objects containing precise timer and
     * Timer objects containing millisecond Timer (much faster comparison).
     *
     * @param[in] aTimerA   The first timer for comparison.
     * @param[in] aTimerB   The second timer for comparison.
     *
     * @retval TRUE   If aTimerA will fire before aTimerB.
     * @retval FALSE  If aTimerA will fire at the same time or after aTimerB.
     */
    template <class Ttimer, class Ttime> bool TimerCompareTemplate(const Ttimer &aTimerA, const Ttimer &aTimerB);

    /**
     * This function compares two timers and returns a value to indicate
     * which timer will fire earlier.
     *
     * It compares TimerUs objects that contain precise timer.
     *
     * @param[in] aTimerA   The first timer for comparison.
     * @param[in] aTimerB   The second timer for comparison.
     *
     * @retval TRUE   If aTimerA will fire before aTimerB.
     * @retval FALSE  If aTimerA will fire at the same time or after aTimerB.
     */
    bool TimerCompare(const TimerUs &aTimerA, const TimerUs &aTimerB);

    /**
     * This function compares two timers and returns a value to indicate
     * which timer will fire earlier.
     *
     * It compares Timer objects that are less precise than TimerUs but computations
     * are much faster.
     *
     * @param[in] aTimerA   The first timer for comparison.
     * @param[in] aTimerB   The second timer for comparison.
     *
     * @retval TRUE   If aTimerA will fire before aTimerB.
     * @retval FALSE  If aTimerA will fire at the same time or after aTimerB.
     */
    bool TimerCompare(const Timer &aTimerA, const Timer &aTimerB) ;


    /**
     * Get the head of the millisecond timers list.
     *
     * This function is used by templates.
     */
    Timer **GetHead(const Timer *) { return &mHead; }

    /**
     * Get the head of the microsecond timers list.
     *
     * This function is used by templates.
     */
    TimerUs **GetHead(const TimerUs *) { return &mHeadUs; }

    Timer *mHead;
    TimerUs *mHeadUs;

    bool mCurrentlySetUs;
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
     * @param[in]  aScheduler  A reference to the timer scheduler.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aContext    A pointer to arbitrary context information.
     *
     */
    Timer(TimerScheduler &aScheduler, Handler aHandler, void *aContext):
        mScheduler(aScheduler),
        mHandler(aHandler),
        mContext(aContext),
        mT0(0),
        mDt(0),
        mNext(NULL) {
    }

    /**
     * This constructor creates a copy of a timer instance.
     *
     * @param[in]  aTimer  A reference to the timer to copy.
     *
     */
    Timer(const Timer &aTimer):
        mScheduler(aTimer.mScheduler),
        mHandler(aTimer.mHandler),
        mContext(aTimer.mContext),
        mT0(aTimer.mT0),
        mDt(aTimer.mDt),
        mNext(NULL) {
    }

    /**
     * This method returns the start time in milliseconds for the timer.
     *
     * @returns The start time in milliseconds.
     *
     */
    uint32_t GetT0(void) const { return mT0; }

    /**
     * This method returns the delta time in milliseconds for the timer.
     *
     * @returns The delta time.
     *
     */
    uint32_t GetDt(void) const { return mDt; }

    /**
     * This method indicates whether or not the timer instance is running.
     *
     * @retval TRUE   If the timer is running.
     * @retval FALSE  If the timer is not running.
     */
    bool IsRunning(void) const { return mScheduler.IsAdded(*this); }

    /**
     * This method schedules the timer to fire a @p aDt milliseconds from now.
     *
     * @param[in]  aDt  The expire time in milliseconds from now.
     */
    void Start(uint32_t aDt) { StartAt(Time::GetNow(), aDt); }

    /**
     * This method schedules the timer to fire at @p aDt milliseconds from @p aT0.
     *
     * @param[in]  aT0  The start time in milliseconds.
     * @param[in]  aDt  The expire time in milliseconds from @p aT0.
     */
    void StartAt(uint32_t aT0, uint32_t aDt) { mT0 = aT0; mDt = aDt; mScheduler.Add(*this); }

    /**
     * This method stops the timer.
     *
     */
    void Stop(void) { mScheduler.Remove(*this); }

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

protected:
    void Fired(void) { mHandler(mContext); }

    TimerScheduler &mScheduler;
    Handler         mHandler;
    void           *mContext;
    uint32_t        mT0;
    uint32_t        mDt;
    Timer          *mNext;
};

/**
 * This class implements precise timer (microseconds accuracy).
 *
 */
class TimerUs: public Timer
{
    friend class TimerScheduler;

public:
    /**
     * This constructor creates a precise timer instance.
     *
     * @param[in]  aScheduler  A reference to the timer scheduler.
     * @param[in]  aHandler    A pointer to a function that is called when the timer expires.
     * @param[in]  aContext    A pointer to arbitrary context information.
     *
     */
    TimerUs(TimerScheduler &aScheduler, Handler aHandler, void *aContext):
        Timer(aScheduler, aHandler, aContext),
        mT0Us(0),
        mDtUs(0),
        mNext(NULL) {
    }

    /**
     * This constructor converts a timer instance to a prices timer instance.
     */
    explicit TimerUs(const Timer &aTimer):
        Timer(aTimer),
        mT0Us(0),
        mDtUs(0),
        mNext(NULL) {
    }

    /**
     * This is a copy constructor.
     */
    TimerUs(const TimerUs &aTimer):
        Timer(aTimer),
        mT0Us(aTimer.mT0Us),
        mDtUs(aTimer.mDtUs),
        mNext(aTimer.mNext) {
    }

    /**
     * This method returns the precise start time in microseconds for the timer.
     *
     * @returns The precise start time in microseconds.
     */
    Time GetT0(void) const { return Time(mT0, mT0Us); }

    /**
     * This method returns the precise delta time in microseconds for the timer.
     *
     * @returns The precise delta time.
     */
    Time GetDt(void) const { return Time(mDt, mDtUs); }

    /**
     * This method indicates whether or not the timer instance is running.
     *
     * @retval TRUE   If the timer is running.
     * @retval FALSE  If the timer is not running.
     */
    bool IsRunning(void) const { return mScheduler.IsAdded(*this); }

    /**
     * This method schedules the timer to fire a @p aDt microseconds from now.
     *
     * @param[in]  aDt  The expire time in milliseconds and microseconds from now.
     */
    void Start(const Time &aDt) { Time now; Time::GetNow(now); StartAt(now, aDt); }

    /**
     * This method schedules the timer to fire at @p aDt milliseconds and microseconds from @p aT0.
     *
     * @param[in]  aT0  The start time in milliseconds and microseconds.
     * @param[in]  aDt  The expire time in milliseconds and microseconds from @p aT0.
     */
    void StartAt(const Time &aT0, const Time &aDt) {
        mT0 = aT0.mMs;
        mT0Us = aT0.mUs;
        mDt = aDt.mMs;
        mDtUs = aDt.mUs;
        mScheduler.Add(*this);
    }

    /**
     * This static method returns the number of microseconds given milliseconds.
     *
     * @returns The number of microseconds.
     *
     */
    static uint32_t MsecToUsec(uint32_t aMilliseconds) { return aMilliseconds * 1000u; }

    /**
     * This static method returns the number of milliseconds given microseconds.
     *
     * @returns The number of milliseconds.
     *
     */
    static uint32_t UsecToMsec(uint32_t aMicroseconds) { return aMicroseconds / 1000u; }



protected:
    uint16_t mT0Us;
    uint16_t mDtUs;

    TimerUs *mNext;
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // TIMER_HPP_
