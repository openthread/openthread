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

#include <openthread/config.h>

#include "timer.hpp"

#include <openthread/platform/alarm.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "net/ip6.hpp"

namespace ot {

TimerScheduler::TimerScheduler(Ip6::Ip6 &aIp6):
    Ip6Locator(aIp6),
    mHead(NULL)
{
}

void TimerScheduler::Add(Timer &aTimer)
{
    Remove(aTimer);

    if (mHead == NULL)
    {
        mHead = &aTimer;
        aTimer.mNext = NULL;
        SetAlarm();
    }
    else
    {
        Timer *prev = NULL;
        Timer *cur;

        for (cur = mHead; cur; cur = cur->mNext)
        {
            if (aTimer.DoesFireBefore(*cur))
            {
                if (prev)
                {
                    aTimer.mNext = cur;
                    prev->mNext = &aTimer;
                }
                else
                {
                    aTimer.mNext = mHead;
                    mHead = &aTimer;
                    SetAlarm();
                }

                break;
            }

            prev = cur;
        }

        if (cur == NULL)
        {
            prev->mNext = &aTimer;
            aTimer.mNext = NULL;
        }
    }
}

void TimerScheduler::Remove(Timer &aTimer)
{
    VerifyOrExit(aTimer.mNext != &aTimer);

    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        SetAlarm();
    }
    else
    {
        for (Timer *cur = mHead; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aTimer)
            {
                cur->mNext = aTimer.mNext;
                break;
            }
        }
    }

    aTimer.mNext = &aTimer;

exit:
    return;
}

void TimerScheduler::SetAlarm(void)
{
    if (mHead == NULL)
    {
        otPlatAlarmStop(GetIp6().GetInstance());
    }
    else
    {
        uint32_t now = otPlatAlarmGetNow();
        uint32_t remaining = IsStrictlyBefore(now, mHead->mFireTime) ? (mHead->mFireTime - now) : 0;

        otPlatAlarmStartAt(GetIp6().GetInstance(), now, remaining);
    }
}

extern "C" void otPlatAlarmFired(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mTimerScheduler.ProcessTimers();
    otLogFuncExit();
}

void TimerScheduler::ProcessTimers(void)
{
    Timer *timer = mHead;

    if (timer)
    {
        if (!IsStrictlyBefore(otPlatAlarmGetNow(), timer->mFireTime))
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

bool TimerScheduler::IsStrictlyBefore(uint32_t aTimeA, uint32_t aTimeB)
{
    uint32_t diff = aTimeA - aTimeB;

    // Three cases:
    // 1) aTimeA is before  aTimeB  =>  Difference is negative (last bit of difference is set)   => Returning true.
    // 2) aTimeA is same as aTimeB  =>  Difference is zero     (last bit of difference is clear) => Returning false.
    // 3) aTimeA is after   aTimeB  =>  Difference is positive (last bit of difference is clear) => Returning false.

    return ((diff & (1UL << 31)) != 0);
}

TimerScheduler &Timer::GetTimerScheduler(void) const
{
    return GetIp6().mTimerScheduler;
}

bool Timer::DoesFireBefore(const Timer &aSecondTimer)
{
    bool retval;
    uint32_t now = GetNow();
    bool isBeforeNow = TimerScheduler::IsStrictlyBefore(GetFireTime(), now);

    // Check if one timer is before `now` and the other one is not.
    if (TimerScheduler::IsStrictlyBefore(aSecondTimer.GetFireTime(), now) != isBeforeNow)
    {
        // One timer is before `now` and the other one is not, so if this timer's fire time is before `now` then
        // the second fire time would be after `now` and this timer would fire before the second timer.

        retval = isBeforeNow;
    }
    else
    {
        // Both timers are before `now` or both are after `now`. Either way the difference is guaranteed to be less
        // than `kMaxDt` so we can safely compare the fire times directly.

        retval = TimerScheduler::IsStrictlyBefore(GetFireTime(), aSecondTimer.GetFireTime());
    }

    return retval;
}

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
UsecTimerScheduler::UsecTimerScheduler(Ip6::Ip6 &aIp6):
    Ip6Locator(aIp6),
    mHead(NULL)
{
}

void UsecTimerScheduler::Add(UsecTimer &aTimer)
{
    Remove(aTimer);

    if (mHead == NULL)
    {
        mHead = &aTimer;
        aTimer.mNext = NULL;
        SetAlarm();
    }
    else
    {
        UsecTimer *prev = NULL;
        UsecTimer *cur;

        for (cur = mHead; cur; cur = cur->mNext)
        {
            if (aTimer.DoesFireBefore(*cur))
            {
                if (prev)
                {
                    aTimer.mNext = cur;
                    prev->mNext = &aTimer;
                }
                else
                {
                    aTimer.mNext = mHead;
                    mHead = &aTimer;
                    SetAlarm();
                }

                break;
            }

            prev = cur;
        }

        if (cur == NULL)
        {
            prev->mNext = &aTimer;
            aTimer.mNext = NULL;
        }
    }
}

void UsecTimerScheduler::Remove(UsecTimer &aTimer)
{
    VerifyOrExit(aTimer.mNext != &aTimer);

    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        SetAlarm();
    }
    else
    {
        for (UsecTimer *cur = mHead; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aTimer)
            {
                cur->mNext = aTimer.mNext;
                break;
            }
        }
    }

    aTimer.mNext = &aTimer;

exit:
    return;
}

void UsecTimerScheduler::SetAlarm(void)
{
    if (mHead == NULL)
    {
        otPlatUsecAlarmStop(GetIp6().GetInstance());
    }
    else
    {
        uint32_t now = otPlatUsecAlarmGetNow();
        uint32_t remaining = TimerScheduler::IsStrictlyBefore(now, mHead->mFireTime) ? (mHead->mFireTime - now) : 0;

        otPlatUsecAlarmStartAt(GetIp6().GetInstance(), now, remaining);
    }
}

extern "C" void otPlatUsecAlarmFired(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mUsecTimerScheduler.ProcessTimers();
    otLogFuncExit();
}

void UsecTimerScheduler::ProcessTimers(void)
{
    UsecTimer *timer = mHead;

    if (timer)
    {
        if (!TimerScheduler::IsStrictlyBefore(otPlatUsecAlarmGetNow(), timer->mFireTime))
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

UsecTimerScheduler &UsecTimer::GetUsecTimerScheduler(void) const
{
    return GetIp6().mUsecTimerScheduler;
}

bool UsecTimer::DoesFireBefore(const UsecTimer &aSecondTimer)
{
    bool retval;
    uint32_t now = GetNow();
    bool isBeforeNow = TimerScheduler::IsStrictlyBefore(GetFireTime(), now);

    // Check if one timer is before `now` and the other one is not.
    if (TimerScheduler::IsStrictlyBefore(aSecondTimer.GetFireTime(), now) != isBeforeNow)
    {
        // One timer is before `now` and the other one is not, so if this timer's fire time is before `now` then
        // the second fire time would be after `now` and this timer would fire before the second timer.

        retval = isBeforeNow;
    }
    else
    {
        // Both timers are before `now` or both are after `now`. Either way the difference is guaranteed to be less
        // than `kMaxDt` so we can safely compare the fire times directly.

        retval = TimerScheduler::IsStrictlyBefore(GetFireTime(), aSecondTimer.GetFireTime());
    }

    return retval;
}
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

}  // namespace ot
