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
 *   This file implements a multiplexed timer service on top of the alarm abstraction.
 */

#include <common/code_utils.hpp>
#include <common/timer.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <platform/alarm.h>
#include <openthreadinstance.h>

namespace Thread {

void TimerScheduler::Add(Timer &aTimer)
{
    otInstance *aInstance = aTimer.mInstance;

    Remove(aTimer);

    if (aInstance->mTimerHead == NULL)
    {
        aInstance->mTimerHead = &aTimer;
        aInstance->mTimerTail = &aTimer;
        SetAlarm(aTimer.mInstance);
    }
    else
    {
        Timer *prev = NULL;
        Timer *cur;

        for (cur = aInstance->mTimerHead; cur; cur = cur->mNext)
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
                    aTimer.mNext = aInstance->mTimerHead;
                    aInstance->mTimerHead = &aTimer;
                    SetAlarm(aTimer.mInstance);
                }

                break;
            }

            prev = cur;
        }

        if (cur == NULL)
        {
            aInstance->mTimerTail->mNext = &aTimer;
            aInstance->mTimerTail = &aTimer;
        }
    }
}

void TimerScheduler::Remove(Timer &aTimer)
{
    otInstance *aInstance = aTimer.mInstance;

    VerifyOrExit(aTimer.mNext != NULL || aInstance->mTimerTail == &aTimer, ;);

    if (aInstance->mTimerHead == &aTimer)
    {
        aInstance->mTimerHead = aTimer.mNext;

        if (aInstance->mTimerTail == &aTimer)
        {
            aInstance->mTimerTail = NULL;
        }

        SetAlarm(aTimer.mInstance);
    }
    else
    {
        for (Timer *cur = aInstance->mTimerHead; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aTimer)
            {
                cur->mNext = aTimer.mNext;

                if (aInstance->mTimerTail == &aTimer)
                {
                    aInstance->mTimerTail = cur;
                }

                break;
            }
        }
    }

    aTimer.mNext = NULL;

exit:
    return;
}

bool TimerScheduler::IsAdded(const Timer &aTimer)
{
    bool rval = false;

    for (Timer *cur = aTimer.mInstance->mTimerHead; cur; cur = cur->mNext)
    {
        if (cur == &aTimer)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

void TimerScheduler::SetAlarm(otInstance *aInstance)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsed;
    uint32_t remaining;
    Timer *timer = aInstance->mTimerHead;

    if (timer == NULL)
    {
        otPlatAlarmStop(aInstance);
    }
    else
    {
        elapsed = now - timer->mT0;
        remaining = (timer->mDt > elapsed) ? timer->mDt - elapsed : 0;

        otPlatAlarmStartAt(aInstance, now, remaining);
    }
}

extern "C" void otPlatAlarmFired(otInstance *aInstance)
{
    TimerScheduler::FireTimers(aInstance);
}

void TimerScheduler::FireTimers(otInstance *aInstance)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsed;
    Timer *timer = aInstance->mTimerHead;

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
            SetAlarm(aInstance);
        }
    }
    else
    {
        SetAlarm(aInstance);
    }
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
