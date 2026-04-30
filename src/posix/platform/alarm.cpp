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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "mainloop.hpp"
#include "common/code_utils.hpp"

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;
#endif

static uint32_t sSpeedUpFactor = 1;

#ifdef __linux__

#include <signal.h>
#include <time.h>

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE && !OPENTHREAD_POSIX_VIRTUAL_TIME
static timer_t sMicroTimer;
static int     sRealTimeSignal = 0;

static void microTimerHandler(int aSignal, siginfo_t *aSignalInfo, void *aUserContext)
{
    assert(aSignal == sRealTimeSignal);
    assert(aSignalInfo->si_value.sival_ptr == &sMicroTimer);
    OT_UNUSED_VARIABLE(aSignal);
    OT_UNUSED_VARIABLE(aSignalInfo);
    OT_UNUSED_VARIABLE(aUserContext);
}
#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE && !OPENTHREAD_POSIX_VIRTUAL_TIME
#endif // __linux__

#ifdef CLOCK_MONOTONIC_RAW
#define OT_POSIX_CLOCK_ID CLOCK_MONOTONIC_RAW
#else
#define OT_POSIX_CLOCK_ID CLOCK_MONOTONIC
#endif

static bool IsExpired(uint32_t aTime, uint32_t aNow)
{
    // Determine whether or not `aTime` is before or same as `aNow`.

    uint32_t diff = aNow - aTime;

    return (diff & (1U << 31)) == 0;
}

static uint32_t CalculateDuration(uint32_t aTime, uint32_t aNow)
{
    // Return the time duration from `aNow` to `aTime` if `aTime` is
    // after `aNow`, otherwise return zero.

    return IsExpired(aTime, aNow) ? 0 : aTime - aNow;
}

#if !OPENTHREAD_POSIX_VIRTUAL_TIME
uint64_t otPlatTimeGet(void)
{
    struct timespec now;

    VerifyOrDie(clock_gettime(OT_POSIX_CLOCK_ID, &now) == 0, OT_EXIT_FAILURE);

    return static_cast<uint64_t>(now.tv_sec) * OT_US_PER_S + static_cast<uint64_t>(now.tv_nsec) / OT_NS_PER_US;
}
#endif // !OPENTHREAD_POSIX_VIRTUAL_TIME

static uint64_t platformAlarmGetNow(void) { return otPlatTimeGet() * sSpeedUpFactor; }

void platformAlarmInit(uint32_t aSpeedUpFactor, int aRealTimeSignal)
{
    sSpeedUpFactor = aSpeedUpFactor;

    if (aRealTimeSignal == 0)
    {
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
        otLogWarnPlat("Real time signal not enabled, microsecond timers may be inaccurate!");
#endif
    }
#ifdef __linux__
    else if (aRealTimeSignal >= SIGRTMIN && aRealTimeSignal <= SIGRTMAX)
    {
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE && !OPENTHREAD_POSIX_VIRTUAL_TIME
        struct sigaction sa;
        struct sigevent  sev;

        sa.sa_flags     = SA_SIGINFO;
        sa.sa_sigaction = microTimerHandler;
        sigemptyset(&sa.sa_mask);

        VerifyOrDie(sigaction(aRealTimeSignal, &sa, nullptr) != -1, OT_EXIT_ERROR_ERRNO);

        sev.sigev_notify          = SIGEV_SIGNAL;
        sev.sigev_signo           = aRealTimeSignal;
        sev.sigev_value.sival_ptr = &sMicroTimer;

        VerifyOrDie(timer_create(CLOCK_MONOTONIC, &sev, &sMicroTimer) != -1, OT_EXIT_ERROR_ERRNO);

        sRealTimeSignal = aRealTimeSignal;
#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE && !OPENTHREAD_POSIX_VIRTUAL_TIME
    }
#endif // __linux__
    else
    {
        DieNow(OT_EXIT_INVALID_ARGUMENTS);
    }
}

uint32_t otPlatAlarmMilliGetNow(void) { return static_cast<uint32_t>(platformAlarmGetNow() / OT_US_PER_MS); }

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMsAlarm     = aT0 + aDt;
    sIsMsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsMsRunning = false;
}

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
uint32_t otPlatAlarmMicroGetNow(void) { return static_cast<uint32_t>(platformAlarmGetNow()); }

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sUsAlarm     = aT0 + aDt;
    sIsUsRunning = true;

#if defined(__linux__) && !OPENTHREAD_POSIX_VIRTUAL_TIME
    if (sRealTimeSignal != 0)
    {
        struct itimerspec its;
        uint32_t          diff = sUsAlarm - otPlatAlarmMicroGetNow();

        its.it_value.tv_sec  = diff / OT_US_PER_S;
        its.it_value.tv_nsec = (diff % OT_US_PER_S) * OT_NS_PER_US;

        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = 0;

        if (-1 == timer_settime(sMicroTimer, 0, &its, nullptr))
        {
            otLogWarnPlat("Failed to update microsecond timer: %s", strerror(errno));
        }
    }
#endif // defined( __linux__) && !OPENTHREAD_POSIX_VIRTUAL_TIME
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsUsRunning = false;

#if defined(__linux__) && !OPENTHREAD_POSIX_VIRTUAL_TIME
    if (sRealTimeSignal != 0)
    {
        struct itimerspec its = {{0, 0}, {0, 0}};

        if (-1 == timer_settime(sMicroTimer, 0, &its, nullptr))
        {
            otLogWarnPlat("Failed to stop microsecond timer: %s", strerror(errno));
        }
    }
#endif // defined( __linux__) && !OPENTHREAD_POSIX_VIRTUAL_TIME
}
#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    uint64_t remaining = INT32_MAX;
    uint64_t now       = platformAlarmGetNow();

    assert(aTimeout != nullptr);

    if (sIsMsRunning)
    {
        uint32_t nowMs        = static_cast<uint32_t>(now / OT_US_PER_MS);
        uint32_t leftoverUsec = static_cast<uint32_t>(now % OT_US_PER_MS);

        remaining = CalculateDuration(sMsAlarm, nowMs);

        if (remaining > 0)
        {
            remaining *= OT_US_PER_MS;
            remaining -= leftoverUsec;
        }
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (sIsUsRunning && (remaining > 0))
    {
        uint32_t usRemaining = CalculateDuration(sUsAlarm, static_cast<uint32_t>(now));

        if (usRemaining < remaining)
        {
            remaining = usRemaining;
        }
    }
#endif

    if (remaining == 0)
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
    else
    {
        uint64_t timeout;

        remaining /= sSpeedUpFactor;

        if (remaining == 0)
        {
            remaining = 1;
        }

        timeout = static_cast<uint64_t>(aTimeout->tv_sec) * OT_US_PER_S + static_cast<uint64_t>(aTimeout->tv_usec);

        if (remaining < timeout)
        {
            aTimeout->tv_sec  = static_cast<time_t>(remaining / OT_US_PER_S);
            aTimeout->tv_usec = static_cast<suseconds_t>(remaining % OT_US_PER_S);
        }
    }
}

void platformAlarmProcess(otInstance *aInstance)
{
    if (sIsMsRunning && IsExpired(sMsAlarm, otPlatAlarmMilliGetNow()))
    {
        sIsMsRunning = false;

#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagAlarmFired(aInstance);
        }
        else
#endif
        {
            otPlatAlarmMilliFired(aInstance);
        }
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

    if (sIsUsRunning && IsExpired(sUsAlarm, otPlatAlarmMicroGetNow()))
    {
        sIsUsRunning = false;

        otPlatAlarmMicroFired(aInstance);
    }

#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
}
