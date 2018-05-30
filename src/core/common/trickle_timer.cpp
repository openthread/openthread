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
    : TimerMilli(aInstance, HandleTimerFired, aOwner)
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    , k(aRedundancyConstant)
    , c(0)
#endif
    , Imin(0)
    , Imax(0)
    , mMode(kModeNormal)
    , I(0)
    , t(0)
    , mPhase(kPhaseDormant)
    , mTransmitHandler(aTransmitHandler)
    , mIntervalExpiredHandler(aIntervalExpiredHandler)
{
}

bool TrickleTimer::IsRunning(void) const
{
    return mPhase != kPhaseDormant;
}

void TrickleTimer::Start(uint32_t aIntervalMin, uint32_t aIntervalMax, Mode aMode)
{
    assert(!IsRunning());

    // Set the interval limits and mode
    Imin  = aIntervalMin;
    Imax  = aIntervalMax;
    mMode = aMode;

    // Initialize I to [Imin, Imax]
    if (Imin == Imax)
    {
        I = Imin;
    }
    else
    {
        I = Random::GetUint32InRange(Imin, Imax);
    }

    // Start a new interval
    StartNewInterval();
}

void TrickleTimer::Stop(void)
{
    mPhase = kPhaseDormant;
    TimerMilli::Stop();
}

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
void TrickleTimer::IndicateConsistent(void)
{
    // Increment counter
    c++;
}
#endif

void TrickleTimer::IndicateInconsistent(void)
{
    // Only relevant if we aren't already at 'I' == 'Imin'
    if (IsRunning() && I != Imin)
    {
        // Reset I to Imin
        I = Imin;

        // Stop the existing timer
        TimerMilli::Stop();

        // Start a new interval
        StartNewInterval();
    }
}

void TrickleTimer::StartNewInterval(void)
{
// Reset the counter and timer phase

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    c = 0;
#endif
    mPhase = kPhaseTransmit;

    // Initialize t
    if (I < 2)
    {
        // Immediate interval, just set t to 0
        t = 0;
    }
    else if (mMode == kModeMPL)
    {
        // Initialize t to random value between (0, I]
        t = Random::GetUint32InRange(0, I);
    }
    else if (mMode == kModePlainTimer)
    {
        // Initialize t to I, which has already been randomized in Start
        t = I;
    }
    else
    {
        // Initialize t to random value between (I/2, I]
        t = Random::GetUint32InRange(I / 2, I);
    }

    // Start the timer for 't' milliseconds from now
    TimerMilli::Start(t);
}

void TrickleTimer::HandleTimerFired(Timer &aTimer)
{
    static_cast<TrickleTimer *>(&aTimer)->HandleTimerFired();
}

void TrickleTimer::HandleTimerFired(void)
{
    Phase curPhase       = mPhase;
    bool  shouldContinue = true;

    // Default the current state to Dormant
    mPhase = kPhaseDormant;

    switch (curPhase)
    {
    // We have just reached time 't'
    case kPhaseTransmit:
    {
    // Are we not using redundancy or is the counter still less than it?
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
        if (k == 0 || c < k)
#endif
        {
            // Invoke the transmission callback
            shouldContinue = TransmitFired();
        }

        // Wait for the rest of the interval to elapse
        if (shouldContinue)
        {
            // If we are in plain timer mode, just randomize I and restart the interval
            if (mMode == kModePlainTimer)
            {
                // Initialize I to [Imin, Imax]
                I = Random::GetUint32InRange(Imin, Imax);

                // Start a new interval
                StartNewInterval();
            }
            else
            {
                // Start next phase of the timer
                mPhase = kPhaseInterval;

                // Start the time for 'I - t' milliseconds
                TimerMilli::Start(I - t);
            }
        }

        break;
    }

    // We have just reached time 'I'
    case kPhaseInterval:
    {
        // Double 'I' to get the new interval length
        uint32_t newI = I == 0 ? 1 : I << 1;

        if (newI > Imax)
        {
            newI = Imax;
        }

        I = newI;

        // Invoke the interval expiration callback
        shouldContinue = IntervalExpiredFired();

        if (shouldContinue)
        {
            // Start a new interval
            StartNewInterval();
        }

        break;
    }

    default:
        assert(false);
    }
}

} // namespace ot
