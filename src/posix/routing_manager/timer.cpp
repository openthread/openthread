/*
 *    Copyright (c) 2020, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes implementation of timer.
 *
 */

#include "timer.hpp"

#if OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE

#include <openthread/platform/alarm-milli.h>

#include "common/numeric_limits.hpp"

namespace ot {

namespace Posix {

Timer::Timer(Handler aHandler, void *aContext)
    : mHandler(aHandler)
    , mContext(aContext)
    , mFireTime(0)
    , mIsRunning(false)
    , mNext(nullptr)
{
}

void Timer::Start(Milliseconds aDelay)
{
    StartAt(otPlatAlarmMilliGetNow() + aDelay);
}

void Timer::StartAt(Milliseconds aFireTime)
{
    Stop();
    mFireTime  = aFireTime;
    mIsRunning = true;
    TimerScheduler::Get().Add(this);
}

void Timer::Stop()
{
    TimerScheduler::Get().Remove(this);
    mIsRunning = false;
}

void Timer::Fire()
{
    if (mIsRunning)
    {
        Stop();
        if (mHandler != nullptr)
        {
            mHandler(*this, mContext);
        }
    }
}

TimerScheduler &TimerScheduler::Get()
{
    static TimerScheduler sScheduler;
    return sScheduler;
}

void TimerScheduler::Add(Timer *aTimer)
{
    Timer *pre = nullptr;
    Timer *cur = nullptr;

    Remove(aTimer);

    if (mSortedTimerList == nullptr)
    {
        mSortedTimerList = aTimer;
    }
    else
    {
        cur = mSortedTimerList;
        while (cur != nullptr && cur->mFireTime <= aTimer->mFireTime)
        {
            pre = cur;
            cur = cur->mNext;
        }

        aTimer->mNext = cur;

        if (pre == nullptr)
        {
            mSortedTimerList = aTimer;
        }
        else
        {
            pre->mNext = aTimer;
        }
    }
}

void TimerScheduler::Remove(Timer *aTimer)
{
    Timer *pre = nullptr;
    Timer *cur = mSortedTimerList;

    while (cur != nullptr && cur != aTimer)
    {
        pre = cur;
        cur = cur->mNext;
    }

    if (cur != nullptr)
    {
        if (pre == nullptr)
        {
            mSortedTimerList = cur->mNext;
        }
        else
        {
            pre->mNext = cur->mNext;
        }
    }
}

void TimerScheduler::Process(Milliseconds aNow)
{
    while (mSortedTimerList != nullptr && mSortedTimerList->mFireTime <= aNow)
    {
        // The timer is removed inside `Fire()`.
        mSortedTimerList->Fire();
    }
}

Milliseconds TimerScheduler::GetEarliestFireTime() const
{
    return mSortedTimerList ? mSortedTimerList->mFireTime : NumericLimits<Milliseconds>::Max();
}

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE
