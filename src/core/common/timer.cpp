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

#include <common/code_utils.hpp>
#include <common/timer.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <net/ip6.hpp>
#include <platform/alarm.h>
#include <openthread-instance.h>

namespace Thread {

TimerScheduler::TimerScheduler(void):
    mHead(NULL)
{
}

void TimerScheduler::Add(Timer &aTimer)
{
    Remove(aTimer);

    if (mHead == NULL)
    {
        mHead = &aTimer;
        SetAlarm();
    }
    else
    {
        Timer *prev = NULL;
        Timer *cur;

        for (cur = mHead; cur; cur = cur->mNext)
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
        }
    }
}

void TimerScheduler::Remove(Timer &aTimer)
{
    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        aTimer.mNext = NULL;
        SetAlarm();
    }
    else
    {
        for (Timer *cur = mHead; cur; cur = cur->mNext)
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

bool TimerScheduler::IsAdded(const Timer &aTimer)
{
    bool rval = false;

    for (Timer *cur = mHead; cur; cur = cur->mNext)
    {
        if (cur == &aTimer)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

void TimerScheduler::SetAlarm(void)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsed;
    uint32_t remaining;

    if (mHead == NULL)
    {
        otPlatAlarmStop(GetIp6()->GetInstance());
    }
    else
    {
        elapsed = now - mHead->mT0;
        remaining = (mHead->mDt > elapsed) ? mHead->mDt - elapsed : 0;

        otPlatAlarmStartAt(GetIp6()->GetInstance(), now, remaining);
    }
}

extern "C" void otPlatAlarmFired(otInstance *aInstance)
{
    aInstance->mIp6.mTimerScheduler.FireTimers();
}

void TimerScheduler::FireTimers()
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsed;
    Timer *timer = mHead;

    if (timer)
    {
        elapsed = now - timer->mT0;

        if (elapsed >= timer->mDt)
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

Ip6::Ip6 *TimerScheduler::GetIp6()
{
    return Ip6::Ip6FromTimerScheduler(this);
}

bool TimerScheduler::TimerCompare(const Timer &aTimerA, const Timer &aTimerB)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsedA = now - aTimerA.mT0;
    uint32_t elapsedB = now - aTimerB.mT0;
    bool retval = false;

    if (aTimerA.mDt >= elapsedA && aTimerB.mDt >= elapsedB)
    {
        uint32_t remainingA = aTimerA.mDt - elapsedA;
        uint32_t remainingB = aTimerB.mDt - elapsedB;

        if (remainingA < remainingB)
        {
            retval = true;
        }
    }
    else if (aTimerA.mDt < elapsedA && aTimerB.mDt >= elapsedB)
    {
        retval = true;
    }
    else if (aTimerA.mDt < elapsedA && aTimerB.mDt < elapsedB)
    {
        uint32_t expiredByA = elapsedA - aTimerA.mDt;
        uint32_t expiredByB = elapsedB - aTimerB.mDt;

        if (expiredByB < expiredByA)
        {
            retval = true;
        }
    }

    return retval;
}

}  // namespace Thread
