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
 *   This file implements the trickle timer logic.
 */

#include "trickle_timer.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"

namespace ot {

TrickleTimer::TrickleTimer(Instance &aInstance,
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
                           uint32_t aRedundancyConstant,
#endif
                           Handler aTransmitHandler,
                           Handler aIntervalExpiredHandler,
                           void *  aOwner)
    : TimerMilli(aInstance, &TrickleTimer::HandleTimer, aOwner)
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    , mRedundancyConstant(aRedundancyConstant)
    , mCounter(0)
#endif
    , mIntervalMin(0)
    , mIntervalMax(0)
    , mInterval(0)
    , mTimeInInterval(0)
    , mTransmitHandler(aTransmitHandler)
    , mIntervalExpiredHandler(aIntervalExpiredHandler)
    , mMode(kModeNormal)
    , mIsRunning(false)
    , mInTransmitPhase(false)
{
    assert(aTransmitHandler != NULL);
}

otError TrickleTimer::Start(uint32_t aIntervalMin, uint32_t aIntervalMax, Mode aMode)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIntervalMax >= aIntervalMin, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aIntervalMin != 0 || aIntervalMax != 0, error = OT_ERROR_INVALID_ARGS);

    mIntervalMin = aIntervalMin;
    mIntervalMax = aIntervalMax;
    mMode        = aMode;
    mIsRunning   = true;

    // Select interval randomly from range [Imin, Imax].
    mInterval = Random::GetUint32InRange(mIntervalMin, mIntervalMax + 1);

    StartNewInterval();

exit:
    return error;
}

void TrickleTimer::Stop(void)
{
    mIsRunning = false;
    TimerMilli::Stop();
}

void TrickleTimer::IndicateInconsistent(void)
{
    // If interval is equal to minimum when an "inconsistent" event
    // is received, do nothing.
    VerifyOrExit(mIsRunning && (mInterval != mIntervalMin));

    mInterval = mIntervalMin;
    StartNewInterval();

exit:
    return;
}

void TrickleTimer::StartNewInterval(void)
{
    uint32_t halfInterval;

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    mCounter = 0;
#endif

    mInTransmitPhase = true;

    switch (mMode)
    {
    case kModeNormal:
        halfInterval = mInterval / 2;
        VerifyOrExit(halfInterval < mInterval, mTimeInInterval = halfInterval);

        // Select a random point in the interval taken from the range [I/2, I).
        mTimeInInterval = Random::GetUint32InRange(halfInterval, mInterval);
        break;

    case kModePlainTimer:
        mTimeInInterval = mInterval;
        break;

    case kModeMPL:
        // Select a random point in interval taken from the range [0, I].
        mTimeInInterval = Random::GetUint32InRange(0, mInterval + 1);
        break;
    }

exit:
    TimerMilli::Start(mTimeInInterval);
}

void TrickleTimer::HandleTimer(Timer &aTimer)
{
    static_cast<TrickleTimer *>(&aTimer)->HandleTimer();
}

void TrickleTimer::HandleTimer(void)
{
    if (mInTransmitPhase)
    {
        HandleEndOfTimeInInterval();
    }
    else
    {
        HandleEndOfInterval();
    }
}

void TrickleTimer::HandleEndOfTimeInInterval(void)
{
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    // Trickle transmits if and only if the counter `c` is less
    // than the redundancy constant `k`.
    if (mRedundancyConstant == 0 || mCounter < mRedundancyConstant)
#endif
    {
        bool shouldContinue = mTransmitHandler(*this);
        VerifyOrExit(shouldContinue, Stop());
    }

    switch (mMode)
    {
    case kModePlainTimer:
        // Select a random interval in [Imin, Imax] and restart.
        mInterval = Random::GetUint32InRange(mIntervalMin, mIntervalMax + 1);
        StartNewInterval();
        break;

    case kModeNormal:
    case kModeMPL:
        // Waiting for the rest of the interval to elapse.
        mInTransmitPhase = false;
        TimerMilli::Start(mInterval - mTimeInInterval);
        break;
    }

exit:
    return;
}

void TrickleTimer::HandleEndOfInterval(void)
{
    // Double the interval and ensure result is below max.
    if (mInterval == 0)
    {
        mInterval = 1;
    }
    else if (mInterval <= mIntervalMax - mInterval)
    {
        mInterval *= 2;
    }
    else
    {
        mInterval = mIntervalMax;
    }

    if (mIntervalExpiredHandler)
    {
        bool shouldContinue = mIntervalExpiredHandler(*this);
        VerifyOrExit(shouldContinue, Stop());
    }

    StartNewInterval();

exit:
    return;
}

} // namespace ot
