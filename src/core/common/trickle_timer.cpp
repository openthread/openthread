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

#include <common/code_utils.hpp>
#include <common/trickle_timer.hpp>
#include <platform/random.h>

namespace Thread {

TrickleTimer::TrickleTimer(
    TimerScheduler &aScheduler, uint32_t aRedundancyConstant, TrickleTimerMode aMode,
    Handler aTransmitHandler, Handler aIntervalExpiredHandler, void *aContext)
    :
    mTimer(aScheduler, HandleTimerFired, this),
    k(aRedundancyConstant),
    mMode(aMode),
    mPhase(kTricklePhaseDormant),
    mTransmitHandler(aTransmitHandler),
    mIntervalExpiredHandler(aIntervalExpiredHandler),
    mContext(aContext)
{
}

bool TrickleTimer::IsRunning(void) const
{
    return mPhase != kTricklePhaseDormant;
}

void TrickleTimer::Start(uint32_t aIntervalMin, uint32_t aIntervalMax)
{
    assert(!IsRunning());

    // Set the interval limits
    Imin = aIntervalMin;
    Imax = aIntervalMax;

    // Initialize I to [Imin, Imax]
    I = Imin + otPlatRandomGet() % (Imax - Imin);

    // Start a new interval
    StartNewInterval();
}

void TrickleTimer::Stop(void)
{
    mPhase = kTricklePhaseDormant;
    mTimer.Stop();
}

void TrickleTimer::IndicateConsistent(void)
{
    // Increment counter
    c++;
}

void TrickleTimer::IndicateInconsistent(void)
{
    // Only relavent if we aren't already at 'I' == 'Imin'
    if (IsRunning() && I != Imin)
    {
        // Reset I to Imin
        I = Imin;

        // Stop the existing timer
        mTimer.Stop();

        // Start a new interval
        StartNewInterval();
    }
}

void TrickleTimer::StartNewInterval(void)
{
    // Reset the counter and timer phase
    c = 0;
    mPhase = kTricklePhaseTransmit;

    // Initialize t
    if (I == 0)
    {
        // Immediate interval, just set t to 0
        t = 0;
    }
    else if (mMode == kTrickleTimerModeMPL)
    {
        // Initialize t to random value between (0, I]
        t = otPlatRandomGet() % I;
    }
    else
    {
        // Initialize t to random value between (I/2, I]
        t = (I / 2) + otPlatRandomGet() % (I / 2);
    }

    // Start the timer for 't' milliseconds from now
    mTimer.Start(t);
}

void TrickleTimer::HandleTimerFired(void *aContext)
{
    TrickleTimer *obj = reinterpret_cast<TrickleTimer *>(aContext);
    obj->HandleTimerFired();
}

void TrickleTimer::HandleTimerFired(void)
{
    bool ShouldContinue = true;

    // We have just reached time 't'
    if (mPhase == kTricklePhaseTransmit)
    {
        // Are we not using reduncancy or is the counter still less than it?
        if (k == 0 || c < k)
        {
            // Invoke the transmission callback
            ShouldContinue = TransmitFired();
        }

        // Wait for the rest of the interval to elapse
        if (ShouldContinue)
        {
            // Start next phase of the timer
            mPhase = kTricklePhaseInterval;

            // Start the time for 'I - t' milliseconds
            mTimer.Start(I - t);
        }
    }
    // We have just reached time 'I'
    else if (mPhase == kTricklePhaseInterval)
    {
        // Double 'I' to get the new interval length
        uint32_t newI = I == 0 ? 1 : I << 1;

        if (newI > Imax) { newI = Imax; }

        I = newI;

        // Invoke the interval expiration callback
        ShouldContinue = IntervalExpiredFired();

        if (ShouldContinue)
        {
            // Start a new interval
            StartNewInterval();
        }
    }
    else
    {
        assert(false);
    }

    // If we aren't still running, we go dormant
    if (!ShouldContinue)
    {
        mPhase = kTricklePhaseDormant;
    }
}

}  // namespace Thread
