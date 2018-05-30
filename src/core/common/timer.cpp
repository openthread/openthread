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

#include "timer.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"

namespace ot {

const TimerScheduler::AlarmApi TimerMilliScheduler::sAlarmMilliApi = {&otPlatAlarmMilliStartAt, &otPlatAlarmMilliStop,
                                                                      &otPlatAlarmMilliGetNow};

bool Timer::DoesFireBefore(const Timer &aSecondTimer, uint32_t aNow)
{
    bool retval;
    bool isBeforeNow = TimerScheduler::IsStrictlyBefore(GetFireTime(), aNow);

    // Check if one timer is before `now` and the other one is not.
    if (TimerScheduler::IsStrictlyBefore(aSecondTimer.GetFireTime(), aNow) != isBeforeNow)
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

void TimerMilli::StartAt(uint32_t aT0, uint32_t aDt)
{
    assert(aDt <= kMaxDt);
    mFireTime = aT0 + aDt;
    GetTimerMilliScheduler().Add(*this);
}

void TimerMilli::Stop(void)
{
    GetTimerMilliScheduler().Remove(*this);
}

TimerMilliScheduler &TimerMilli::GetTimerMilliScheduler(void) const
{
    return GetInstance().GetTimerMilliScheduler();
}

void TimerScheduler::Add(Timer &aTimer, const AlarmApi &aAlarmApi)
{
    Remove(aTimer, aAlarmApi);

    if (mHead == NULL)
    {
        mHead        = &aTimer;
        aTimer.mNext = NULL;
        SetAlarm(aAlarmApi);
    }
    else
    {
        Timer *prev = NULL;
        Timer *cur;

        for (cur = mHead; cur; cur = cur->mNext)
        {
            if (aTimer.DoesFireBefore(*cur, aAlarmApi.AlarmGetNow()))
            {
                if (prev)
                {
                    aTimer.mNext = cur;
                    prev->mNext  = &aTimer;
                }
                else
                {
                    aTimer.mNext = mHead;
                    mHead        = &aTimer;
                    SetAlarm(aAlarmApi);
                }

                break;
            }

            prev = cur;
        }

        if (cur == NULL)
        {
            prev->mNext  = &aTimer;
            aTimer.mNext = NULL;
        }
    }
}

void TimerScheduler::Remove(Timer &aTimer, const AlarmApi &aAlarmApi)
{
    VerifyOrExit(aTimer.mNext != &aTimer);

    if (mHead == &aTimer)
    {
        mHead = aTimer.mNext;
        SetAlarm(aAlarmApi);
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

void TimerScheduler::SetAlarm(const AlarmApi &aAlarmApi)
{
    if (mHead == NULL)
    {
        aAlarmApi.AlarmStop(&GetInstance());
    }
    else
    {
        uint32_t now       = aAlarmApi.AlarmGetNow();
        uint32_t remaining = IsStrictlyBefore(now, mHead->mFireTime) ? (mHead->mFireTime - now) : 0;

        aAlarmApi.AlarmStartAt(&GetInstance(), now, remaining);
    }
}

void TimerScheduler::ProcessTimers(const AlarmApi &aAlarmApi)
{
    Timer *timer = mHead;

    if (timer)
    {
        if (!IsStrictlyBefore(aAlarmApi.AlarmGetNow(), timer->mFireTime))
        {
            Remove(*timer, aAlarmApi);
            timer->Fired();
        }
        else
        {
            SetAlarm(aAlarmApi);
        }
    }
    else
    {
        SetAlarm(aAlarmApi);
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

extern "C" void otPlatAlarmMilliFired(otInstance *aInstance)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(otInstanceIsInitialized(aInstance));
    instance->GetTimerMilliScheduler().ProcessTimers();

exit:
    return;
}

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
const TimerScheduler::AlarmApi TimerMicroScheduler::sAlarmMicroApi = {&otPlatAlarmMicroStartAt, &otPlatAlarmMicroStop,
                                                                      &otPlatAlarmMicroGetNow};

void TimerMicro::StartAt(uint32_t aT0, uint32_t aDt)
{
    assert(aDt <= kMaxDt);
    mFireTime = aT0 + aDt;
    GetTimerMicroScheduler().Add(*this);
}

void TimerMicro::Stop(void)
{
    GetTimerMicroScheduler().Remove(*this);
}

TimerMicroScheduler &TimerMicro::GetTimerMicroScheduler(void) const
{
    return GetInstance().GetTimerMicroScheduler();
}

extern "C" void otPlatAlarmMicroFired(otInstance *aInstance)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    VerifyOrExit(otInstanceIsInitialized(aInstance));
    instance->GetTimerMicroScheduler().ProcessTimers();

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

} // namespace ot
