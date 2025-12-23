/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the radio sample scheduler.
 */

#include "radio_sample_scheduler.hpp"

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("SampleSched");

RadioSampleScheduler::RadioSampleScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
    for (Scheduler &scheduler : mSchedulers)
    {
        scheduler.mIsEnabled = false;
    }
}

void RadioSampleScheduler::Start(SchedulerId aSchedulerId, uint8_t aChannel, uint32_t aDuration, uint32_t aPeriod)
{
    OT_ASSERT(aSchedulerId < kNumSchedulers);

    uint32_t   timeAhead = 0;
    uint32_t   timeAfter = 0;
    Scheduler &scheduler = mSchedulers[aSchedulerId];

    scheduler.mChannel    = aChannel;
    scheduler.mDuration   = aDuration;
    scheduler.mPeriod     = aPeriod;
    scheduler.mFireTime   = TimeMicro(0);
    scheduler.mIsEnabled  = true;
    scheduler.mIsSampling = false;
    scheduler.mRadioStart = Get<Radio>().GetNow();
    scheduler.mLocalStart = TimerMicro::GetNow();

    GetWindowEdges(scheduler, timeAhead, timeAfter);

    scheduler.mRadioTime = scheduler.mRadioStart + timeAhead;
    scheduler.mLocalTime = scheduler.mLocalStart + timeAhead;

    LogInfo("%s Start : %s, timeAhead=%lu, timeAfter=%lu", GetSchedulerName(scheduler),
            scheduler.ToString().AsCString(), ToUlong(timeAhead), ToUlong(timeAfter));

    HandleRadioSample(scheduler, timeAhead, timeAfter);
}

void RadioSampleScheduler::Stop(void)
{
    for (Scheduler &scheduler : mSchedulers)
    {
        Stop(scheduler);
    }
}

void RadioSampleScheduler::Stop(SchedulerId aSchedulerId)
{
    OT_ASSERT(aSchedulerId < kNumSchedulers);
    Stop(mSchedulers[aSchedulerId]);
}

void RadioSampleScheduler::Stop(Scheduler &aScheduler)
{
    VerifyOrExit(aScheduler.mIsEnabled);

    LogInfo("%s Stop()", GetSchedulerName(aScheduler));

    aScheduler.mIsEnabled = false;
    aScheduler.mPeriod    = 0;

    UpdateTimer();

    if (Get<SubMac>().RadioSupportsReceiveTiming())
    {
        UpdateRadioSampleState();
    }

exit:
    return;
}

bool RadioSampleScheduler::IsRadioSampleEnabled(void) const
{
    bool ret = false;

    for (const Scheduler &scheduler : mSchedulers)
    {
        VerifyOrExit(!scheduler.mIsEnabled, ret = true);
    }

exit:
    return ret;
}

void RadioSampleScheduler::UpdateTimer(void)
{
    bool      foundTime = false;
    TimeMicro fireTime;

    for (Scheduler &scheduler : mSchedulers)
    {
        if (!scheduler.mIsEnabled)
        {
            continue;
        }

        if (!foundTime || scheduler.mFireTime < fireTime)
        {
            foundTime = true;
            fireTime  = scheduler.mFireTime;
        }
    }

    if (foundTime)
    {
        mTimer.FireAt(fireTime);
    }
    else
    {
        mTimer.Stop();
    }
}

void RadioSampleScheduler::HandleTimer(void)
{
    uint32_t  timeAhead, timeAfter;
    TimeMicro now = TimerMicro::GetNow();

    for (Scheduler &scheduler : mSchedulers)
    {
        if (!scheduler.mIsEnabled || (now < scheduler.mFireTime))
        {
            continue;
        }

        GetWindowEdges(scheduler, timeAhead, timeAfter);
        HandleRadioSample(scheduler, timeAhead, timeAfter);
    }
}

void RadioSampleScheduler::HandleRadioSample(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter)
{
    if (Get<SubMac>().RadioSupportsReceiveTiming())
    {
        HandleReceiveAt(aScheduler, aTimeAhead, aTimeAfter);
    }
    else
    {
        HandleReceiveOrSleep(aScheduler, aTimeAhead, aTimeAfter);
    }
}

void RadioSampleScheduler::HandleReceiveAt(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter)
{
    /*
     * When the radio supports receive-timing:
     *   The handler will be called once per sample period. When the handler is called, it will set the timer to
     *   fire at the next sample time and call `Radio::ReceiveAt` to start sampling for the current sample period.
     *   The timer fires some time before the actual sample time. After `Radio::ReceiveAt` is called, the radio will
     *   remain in sleep state until the actual sample time.
     *   Note that it never call `Radio::Sleep` explicitly. The radio will fall into sleep after `ReceiveAt` ends. This
     *   will be done by the platform as part of the `otPlatRadioReceiveAt` API.
     *
     *   Timer fires                                         Timer fires
     *       ^                                                    ^
     *       x-|------------|-------------------------------------x-|------------|---------------------------------------|
     *            sample                   sleep                        sample                    sleep
     */
    uint32_t periodUs = aScheduler.mPeriod;
    uint32_t winStart;
    uint32_t winDuration;

    aScheduler.mFireTime = aScheduler.mLocalTime - aTimeAhead + periodUs;
    aTimeAhead -= SubMac::kCslReceiveTimeAhead;
    winStart    = aScheduler.mRadioTime - aTimeAhead;
    winDuration = aTimeAhead + aScheduler.mDuration + aTimeAfter;

    aScheduler.mRadioTime += periodUs;
    aScheduler.mLocalTime += periodUs;

    UpdateSampleTime(aScheduler);

    // Schedule reception window for any state except RX - so that sample RX Window has lower priority
    // than scanning or RX after the data poll.
    if ((Get<SubMac>().mState != SubMac::kStateDisabled) && (Get<SubMac>().mState != SubMac::kStateReceive))
    {
        IgnoreError(Get<Radio>().ReceiveAt(aScheduler.mChannel, winStart, winDuration));
    }

    UpdateTimer();
}

void RadioSampleScheduler::HandleReceiveOrSleep(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter)
{
    /*
     * When the radio doesn't support receive-timing:
     *   The handler will be called twice per sample period: at the beginning of sample and sleep. When the handler is
     *   called, it will explicitly change the radio state due to the current state by calling `Radio::Receive` or
     *   `Radio::Sleep`.
     *
     *   Timer fires  Timer fires                            Timer fires  Timer fires
     *       ^            ^                                       ^            ^
     *       |------------|---------------------------------------|------------|---------------------------------------|
     *          sample                   sleep                        sample                    sleep
     */

    aScheduler.mIsSampling = !aScheduler.mIsSampling;

    if (!aScheduler.mIsSampling)
    {
        aScheduler.mFireTime = aScheduler.mLocalTime - aTimeAhead;
    }
    else
    {
        uint32_t periodUs = aScheduler.mPeriod;

        aScheduler.mFireTime = aScheduler.mLocalTime + aScheduler.mDuration + aTimeAfter;
        aScheduler.mRadioTime += periodUs;
        aScheduler.mLocalTime += periodUs;

        UpdateSampleTime(aScheduler);
    }

    UpdateRadioSampleState();
    UpdateTimer();
}

void RadioSampleScheduler::UpdateSampleTime(Scheduler &aScheduler)
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (IsCslScheduler(aScheduler))
    {
        Get<Radio>().UpdateCslSampleTime(aScheduler.mRadioTime);
    }
#endif

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    if (IsECslScheduler(aScheduler))
    {
        Get<Radio>().SetECslSampleTime(aScheduler.mRadioTime);
    }
#endif
}

/*
 * The radio state (receive/sleep) is determined by the request from both CSL and WED:
 * <1> If both CSL and WED request to enter sleep state, the radio is set to sleep state.
 * <2> If either CSL or WED requests to enter the receive state and the other requests to enter sleep state, the radio
 *     is set to receive state using the channel that is requested to enter the receive state.
 * <3> If both CSL and WED request to enter the receive state, the radio is set to the receive state using the CSL
 *     channel.
 *
 * The diagram below illustrates how to set the radio state based on the request of WED and CSL.
 *
 * CSL   ------========------------========------------========------------========---
 *             ^       ^
 *             |       |
 *             | mIsCslSampling=false
 *     mIsCslSampling=true
 *
 * WED   -----------++++++++----------------++++++++----------------++++++++----------
 *                  ^       ^
 *                  |       |
 *                  | mIsWedSampling=false
 *         mIsWedSampling=true
 *
 * Radio ------========+++++-------========-++++++++---========-----+++++++========---
 *             ^       ^    ^
 *             |       |    |
 *             |       | Radio::Sleep()
 *             |  Radio::Receive(WedCh)
 *      Radio::Receive(CslCh)
 */
void RadioSampleScheduler::UpdateRadioSampleState(void)
{
    VerifyOrExit((Get<SubMac>().mState == SubMac::kStateRadioSample) ||
                 (Get<SubMac>().mState == SubMac::kStateReceive));

    for (Scheduler &scheduler : mSchedulers)
    {
        if (!scheduler.mIsEnabled)
        {
            continue;
        }

        if (scheduler.mIsSampling)
        {
            IgnoreError(Get<Radio>().Receive(scheduler.mChannel));
            ExitNow();
        }
    }

    if (Get<SubMac>().mState == SubMac::kStateReceive)
    {
        IgnoreError(Get<Radio>().Receive(Get<SubMac>().mPanChannel));
    }
    else
    {
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
        IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif
    }

exit:
    return;
}

void RadioSampleScheduler::GetWindowEdges(const Scheduler &aScheduler, uint32_t &aAhead, uint32_t &aAfter)
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (IsCslScheduler(aScheduler))
    {
        Get<SubMac>().GetCslWindowEdges(aAhead, aAfter);
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    if (IsWedScheduler(aScheduler))
    {
        Get<SubMac>().GetWedWindowEdges(aAhead, aAfter);
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    if (IsECslScheduler(aScheduler))
    {
        Get<SubMac>().GetECslWindowEdges(aAhead, aAfter);
        ExitNow();
    }
#endif

exit:
    return;
}

uint32_t RadioSampleScheduler::GetLocalTime(void) const
{
    uint32_t now;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    now = TimerMicro::GetNow().GetValue();
#else
    now = static_cast<uint32_t>(Get<Radio>().GetNow());
#endif

    return now;
}

RadioSampleScheduler::SchedulerId RadioSampleScheduler::GetSchedulerId(const Scheduler &aScheduler) const
{
    return static_cast<SchedulerId>(&aScheduler - mSchedulers);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
void RadioSampleScheduler::LogWindow(uint32_t aWinStart, uint32_t aWinDuration, uint8_t aChannel)
{
    LogDebg("CSL window start %lu, duration %lu, channel %u", ToUlong(aWinStart), ToUlong(aWinDuration), aChannel);
}
#else
void RadioSampleScheduler::LogWindow(uint32_t, uint32_t, uint8_t) {}
#endif

const char *RadioSampleScheduler::GetSchedulerName(const Scheduler &aScheduler) const
{
    static const char *const kSchedulerNames[] = {
#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
        "Esl", // (0) kECslSchedulerId
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        "Csl", // (1) kCslSchedulerId
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        "Wed", // (2) kWedSchedulerId
#endif
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
        ValidateNextEnum(kECslSchedulerId);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        ValidateNextEnum(kCslSchedulerId);
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        ValidateNextEnum(kWedSchedulerId);
#endif
    };

    return kSchedulerNames[GetSchedulerId(aScheduler)];
}

//----------------------------------------------------------------------------------------

RadioSampleScheduler::Scheduler::InfoString RadioSampleScheduler::Scheduler::ToString(void) const
{
    InfoString string;

    if (mIsEnabled)
    {
        string.Append("RadioTime:%lu, LocalTime:%lu, FireTime:%lu, IsSampling:%u, Duration:%lu, Period:%lu, Channel:%u",
                      ToUlong(mRadioTime - mRadioStart), ToUlong(mLocalTime.GetValue() - mLocalStart.GetValue()),
                      ToUlong(mFireTime.GetValue() - mLocalStart.GetValue()), mIsSampling, ToUlong(mDuration),
                      ToUlong(mPeriod), mChannel);
    }
    else
    {
        string.Append("Disabled");
    }

    return string;
}

} // namespace Mac
} // namespace ot
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
