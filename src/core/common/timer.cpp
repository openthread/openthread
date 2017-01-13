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
 *   This file implements a multiplexed timer service on top of the alarm abstraction.
 */

#define WPP_NAME "timer.tmh"

#include <common/code_utils.hpp>
#include <common/timer.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <net/ip6.hpp>
#include <platform/alarm.h>
#include <openthread-instance.h>

namespace Thread {

Time Time::operator+(const Time &aOther) const
{
    Time result(*this);

    result.mMs += aOther.mMs;
    result.mUs += aOther.mUs;

    if (result.mUs >= 1000)
    {
        result.mUs -= 1000;
        result.mMs++;
    }

    return result;
}

Time Time::operator-(const Time &aOther) const
{
    Time result(*this);

    result.mMs -= aOther.mMs;
    result.mUs -= aOther.mUs;

    if (result.mUs > this->mUs)
    {
        result.mUs += 1000;
        result.mMs--;
    }

    return result;
}

bool Time::operator>(const Time &aOther) const
{
    if (mMs > aOther.mMs)
    {
        return true;
    }

    if ((mMs == aOther.mMs) && (mUs > aOther.mUs))
    {
        return true;
    }

    return false;
}

bool Time::operator>=(const Time &aOther) const
{
    if (mMs > aOther.mMs)
    {
        return true;
    }

    if ((mMs == aOther.mMs) && (mUs >= aOther.mUs))
    {
        return true;
    }

    return false;
}

bool Time::operator<(const Time &aOther) const
{
    if (mMs < aOther.mMs)
    {
        return true;
    }

    if ((mMs == aOther.mMs) && (mUs < aOther.mUs))
    {
        return true;
    }

    return false;
}

bool Time::operator<=(const Time &aOther) const
{
    if (mMs < aOther.mMs)
    {
        return true;
    }

    if ((mMs == aOther.mMs) && (mUs <= aOther.mUs))
    {
        return true;
    }

    return false;
}

TimerScheduler::TimerScheduler(void):
    mHead(NULL),
    mHeadUs(NULL),
    mCurrentlySetUs(false)
{
}

void TimerScheduler::Add(Timer &aTimer)
{
    TemplateAdd(aTimer);
}

void TimerScheduler::Add(TimerUs &aTimer)
{
    TemplateAdd(aTimer);
}

void TimerScheduler::Remove(Timer &aTimer)
{
    TemplateRemove(aTimer);
}

void TimerScheduler::Remove(TimerUs &aTimer)
{
    TemplateRemove(aTimer);
}

bool TimerScheduler::IsAdded(const Timer &aTimer)
{
    return TemplateIsAdded(aTimer);
}

bool TimerScheduler::IsAdded(const TimerUs &aTimer)
{
    return TemplateIsAdded(aTimer);
}

template <class T> void TimerScheduler::TemplateAdd(T &aTimer)
{
    T **head = GetHead(&aTimer);

    Remove(aTimer);

    if (*head == NULL)
    {
        *head = &aTimer;
        SetAlarm();
    }
    else
    {
        T *prev = NULL;
        T *cur;

        for (cur = *head; cur; cur = cur->mNext)
        {
            if (TimerCompare(aTimer, *cur))
            {
                if (prev)
                {
                    aTimer.mNext = cur;
                    prev->mNext = &aTimer;
                }
                else
                {
                    aTimer.mNext = *head;
                    *head = &aTimer;
                    SetAlarm();
                }

                break;
            }

            prev = cur;
        }

        if (cur == NULL)
        {
            prev->mNext = &aTimer;
        }
    }
}

template <class T> void TimerScheduler::TemplateRemove(T &aTimer)
{
    T **head = GetHead(&aTimer);

    if (*head == &aTimer)
    {
        *head = aTimer.mNext;
        aTimer.mNext = NULL;
        SetAlarm();
    }
    else
    {
        for (T *cur = *head; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aTimer)
            {
                cur->mNext = aTimer.mNext;
                aTimer.mNext = NULL;
                break;
            }
        }
    }
}

template <class T> bool TimerScheduler::TemplateIsAdded(const T &aTimer)
{
    T **head = GetHead(&aTimer);

    bool rval = false;

    for (T *cur = *head; cur; cur = cur->mNext)
    {
        if (cur == &aTimer)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

template <class Ttimer, class Ttime> bool TimerScheduler::TimerCompareTemplate(const Ttimer &aTimerA,
                                                                               const Ttimer &aTimerB)
{
    Ttime now;
    Time::GetNow(now);

    Ttime elapsedA = now - aTimerA.GetT0();
    Ttime elapsedB = now - aTimerB.GetT0();
    bool retval = false;

    if (aTimerA.GetDt() >= elapsedA && aTimerB.GetDt() >= elapsedB)
    {
        Ttime remainingA = aTimerA.GetDt() - elapsedA;
        Ttime remainingB = aTimerB.GetDt() - elapsedB;

        if (remainingA < remainingB)
        {
            retval = true;
        }
    }
    else if (aTimerA.GetDt() < elapsedA && aTimerB.GetDt() >= elapsedB)
    {
        retval = true;
    }
    else if (aTimerA.GetDt() < elapsedA && aTimerB.GetDt() < elapsedB)
    {
        Ttime expiredByA = elapsedA - aTimerA.GetDt();
        Ttime expiredByB = elapsedB - aTimerB.GetDt();

        if (expiredByB < expiredByA)
        {
            retval = true;
        }
    }

    return retval;
}

bool TimerScheduler::TimerCompare(const TimerUs &aTimerA, const TimerUs &aTimerB) { return TimerCompareTemplate<TimerUs, Time>(aTimerA, aTimerB); }
bool TimerScheduler::TimerCompare(const Timer &aTimerA, const Timer &aTimerB) { return TimerCompareTemplate<Timer, uint32_t>(aTimerA, aTimerB); }

void TimerScheduler::SetAlarm(void)
{
    Time now;
    Time::GetNow(now);

    Time elapsed;
    Time remaining;

    if ((mHead == NULL) && (mHeadUs == NULL))
    {
        otPlatAlarmStop(GetIp6()->GetInstance());
    }
    else
    {
        if (mHeadUs == NULL)
        {
            mCurrentlySetUs = false;
        }
        else if (mHead == NULL)
        {
            mCurrentlySetUs = true;
        }
        else
        {
            mCurrentlySetUs = TimerCompare(*mHeadUs, static_cast<TimerUs>(*mHead));
        }

        if (mCurrentlySetUs)
        {
            elapsed = now - mHeadUs->GetT0();
            remaining = (mHeadUs->GetDt() > elapsed) ? mHeadUs->GetDt() - elapsed : Time(0);

            otPlatAlarmStartAt(GetIp6()->GetInstance(), &now, &remaining);
        }
        else
        {
            elapsed = now - Time(mHead->GetT0());
            remaining = (Time(mHead->GetDt()) > elapsed) ? Time(mHead->GetDt()) - elapsed : Time(0);

            otPlatAlarmStartAt(GetIp6()->GetInstance(), &now, &remaining);
        }
    }
}

extern "C" void otPlatAlarmFired(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mTimerScheduler.FireTimers();
    otLogFuncExit();
}

void TimerScheduler::FireTimers()
{
    Timer *timer = mHead;
    TimerUs *timerUs = mHeadUs;

    if (mCurrentlySetUs)
    {
        if (timerUs)
        {
            Time now;
            Time::GetNow(now);

            Time elapsed = now - timerUs->GetT0();

            if (elapsed >= timerUs->GetDt())
            {
                Remove(*timerUs);
                timerUs->Fired();
            }
            else
            {
                SetAlarm();
            }
        }
    }
    else
    {
        if (timer)
        {
            uint32_t now;
            Time::GetNow(now);

            uint32_t elapsed = now - timer->GetT0();

            if (elapsed >= timer->GetDt())
            {
                Remove(*timer);
                timer->Fired();
            }
            else
            {
                SetAlarm();
            }
        }
        else
        {
            SetAlarm();
        }
    }
}

Ip6::Ip6 *TimerScheduler::GetIp6()
{
    return Ip6::Ip6FromTimerScheduler(this);
}

}  // namespace Thread
