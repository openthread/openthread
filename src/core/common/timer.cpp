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

#include <openthread/platform/alarm-milli.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "net/ip6.hpp"

namespace ot {

TimerMilliScheduler::TimerMilliScheduler(Ip6::Ip6 &aIp6):
    Ip6Locator(aIp6),
    mHead(NULL)
{
}

void TimerMilliScheduler::Add(TimerMilli &aTimer)
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
        TimerMilli *prev = NULL;
        TimerMilli *cur;

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

void TimerMilliScheduler::Remove(TimerMilli &aTimer)
{
    VerifyOrExit(aTimer.mNext != &aTimer);

    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        SetAlarm();
    }
    else
    {
        for (TimerMilli *cur = mHead; cur; cur = cur->mNext)
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

void TimerMilliScheduler::SetAlarm(void)
{
    if (mHead == NULL)
    {
        otPlatAlarmMilliStop(GetIp6().GetInstance());
    }
    else
    {
        uint32_t now = TimerMilli::GetNow();
        uint32_t remaining = IsStrictlyBefore(now, mHead->mFireTime) ? (mHead->mFireTime - now) : 0;

        otPlatAlarmMilliStartAt(GetIp6().GetInstance(), now, remaining);
    }
}

extern "C" void otPlatAlarmMilliFired(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mTimerMilliScheduler.ProcessTimers();
    otLogFuncExit();
}

void TimerMilliScheduler::ProcessTimers(void)
{
    TimerMilli *timer = mHead;

    if (timer)
    {
        if (!IsStrictlyBefore(TimerMilli::GetNow(), timer->mFireTime))
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

bool TimerMilliScheduler::IsStrictlyBefore(uint32_t aTimeA, uint32_t aTimeB)
{
    uint32_t diff = aTimeA - aTimeB;

    // Three cases:
    // 1) aTimeA is before  aTimeB  =>  Difference is negative (last bit of difference is set)   => Returning true.
    // 2) aTimeA is same as aTimeB  =>  Difference is zero     (last bit of difference is clear) => Returning false.
    // 3) aTimeA is after   aTimeB  =>  Difference is positive (last bit of difference is clear) => Returning false.

    return ((diff & (1UL << 31)) != 0);
}

TimerMilliScheduler &TimerMilli::GetTimerMilliScheduler(void) const
{
    return GetIp6().mTimerMilliScheduler;
}

bool TimerMilli::DoesFireBefore(const TimerMilli &aSecondTimer)
{
    bool retval;
    uint32_t now = GetNow();
    bool isBeforeNow = TimerMilliScheduler::IsStrictlyBefore(GetFireTime(), now);

    // Check if one timer is before `now` and the other one is not.
    if (TimerMilliScheduler::IsStrictlyBefore(aSecondTimer.GetFireTime(), now) != isBeforeNow)
    {
        // One timer is before `now` and the other one is not, so if this timer's fire time is before `now` then
        // the second fire time would be after `now` and this timer would fire before the second timer.

        retval = isBeforeNow;
    }
    else
    {
        // Both timers are before `now` or both are after `now`. Either way the difference is guaranteed to be less
        // than `kMaxDt` so we can safely compare the fire times directly.

        retval = TimerMilliScheduler::IsStrictlyBefore(GetFireTime(), aSecondTimer.GetFireTime());
    }

    return retval;
}

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
TimerMicroScheduler::TimerMicroScheduler(Ip6::Ip6 &aIp6):
    Ip6Locator(aIp6),
    mHead(NULL)
{
}

void TimerMicroScheduler::Add(TimerMicro &aTimer)
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
        TimerMicro *prev = NULL;
        TimerMicro *cur;

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

void TimerMicroScheduler::Remove(TimerMicro &aTimer)
{
    VerifyOrExit(aTimer.mNext != &aTimer);

    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        SetAlarm();
    }
    else
    {
        for (TimerMicro *cur = mHead; cur; cur = cur->mNext)
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

void TimerMicroScheduler::SetAlarm(void)
{
    if (mHead == NULL)
    {
        otPlatAlarmMicroStop(GetIp6().GetInstance());
    }
    else
    {
        uint32_t now = TimerMicro::GetNow();
        uint32_t remaining = TimerMilliScheduler::IsStrictlyBefore(now, mHead->mFireTime) ? (mHead->mFireTime - now) : 0;

        otPlatAlarmMicroStartAt(GetIp6().GetInstance(), now, remaining);
    }
}

extern "C" void otPlatAlarmMicroFired(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mTimerMicroScheduler.ProcessTimers();
    otLogFuncExit();
}

void TimerMicroScheduler::ProcessTimers(void)
{
    TimerMicro *timer = mHead;

    if (timer)
    {
        if (!TimerMilliScheduler::IsStrictlyBefore(TimerMicro::GetNow(), timer->mFireTime))
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

TimerMicroScheduler &TimerMicro::GetTimerMicroScheduler(void) const
{
    return GetIp6().mTimerMicroScheduler;
}

bool TimerMicro::DoesFireBefore(const TimerMicro &aSecondTimer)
{
    bool retval;
    uint32_t now = GetNow();
    bool isBeforeNow = TimerMilliScheduler::IsStrictlyBefore(GetFireTime(), now);

    // Check if one timer is before `now` and the other one is not.
    if (TimerMilliScheduler::IsStrictlyBefore(aSecondTimer.GetFireTime(), now) != isBeforeNow)
    {
        // One timer is before `now` and the other one is not, so if this timer's fire time is before `now` then
        // the second fire time would be after `now` and this timer would fire before the second timer.

        retval = isBeforeNow;
    }
    else
    {
        // Both timers are before `now` or both are after `now`. Either way the difference is guaranteed to be less
        // than `kMaxDt` so we can safely compare the fire times directly.

        retval = TimerMilliScheduler::IsStrictlyBefore(GetFireTime(), aSecondTimer.GetFireTime());
    }

    return retval;
}
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

}  // namespace ot
