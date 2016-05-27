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
#include <platform/alarm.h>

namespace Thread {

static Timer   *sHead = NULL;
static Timer   *sTail = NULL;

void TimerScheduler::Add(Timer &aTimer)
{
    VerifyOrExit(aTimer.mNext == NULL && sTail != &aTimer, ;);

    if (sTail == NULL)
    {
        sHead = &aTimer;
    }
    else
    {
        sTail->mNext = &aTimer;
    }

    aTimer.mNext = NULL;
    sTail = &aTimer;

exit:
    SetAlarm();
}

void TimerScheduler::Remove(Timer &aTimer)
{
    VerifyOrExit(aTimer.mNext != NULL || sTail == &aTimer, ;);

    if (sHead == &aTimer)
    {
        sHead = aTimer.mNext;

        if (sTail == &aTimer)
        {
            sTail = NULL;
        }
    }
    else
    {
        for (Timer *cur = sHead; cur; cur = cur->mNext)
        {
            if (cur->mNext == &aTimer)
            {
                cur->mNext = aTimer.mNext;

                if (sTail == &aTimer)
                {
                    sTail = cur;
                }

                break;
            }
        }
    }

    aTimer.mNext = NULL;
    SetAlarm();

exit:
    {}
}

bool TimerScheduler::IsAdded(const Timer &aTimer)
{
    bool rval = false;

    if (sHead == &aTimer)
    {
        ExitNow(rval = true);
    }

    for (Timer *cur = sHead; cur; cur = cur->mNext)
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
    int32_t  minRemaining = (1UL << 31) - 1;
    uint32_t elapsed;
    int32_t  remaining;

    if (sHead == NULL)
    {
        otPlatAlarmStop();
        ExitNow();
    }

    for (Timer *timer = sHead; timer; timer = timer->mNext)
    {
        elapsed = now - timer->mT0;
        remaining = timer->mDt - elapsed;

        if (remaining < minRemaining)
        {
            minRemaining = remaining;
        }
    }

    otPlatAlarmStartAt(now, minRemaining);

exit:
    {}
}

extern "C" void otPlatAlarmFired(void)
{
    TimerScheduler::FireTimers();
}

void TimerScheduler::FireTimers(void)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t elapsed;

    for (Timer *cur = sHead; cur; cur = cur->mNext)
    {
        elapsed = now - cur->mT0;

        if (elapsed >= cur->mDt)
        {
            Remove(*cur);
            cur->Fired();
            break;
        }
    }

    SetAlarm();
}

}  // namespace Thread
