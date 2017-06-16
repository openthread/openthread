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
 *   This file includes definitions for the trickle timer logic.
 */

#ifndef TRICKLE_TIMER_HPP_
#define TRICKLE_TIMER_HPP_

#include "common/context.hpp"
#include "common/timer.hpp"

namespace ot {

/**
 * @addtogroup core-timer-trickle
 *
 * @brief
 *   This module includes definitions for the trickle timer logic.
 *
 * @{
 *
 */

/**
 * This class implements a trickle timer.
 *
 */
class TrickleTimer: public Timer
{
public:

    /**
     * Represents the modes of operation for the TrickleTimer
     */
    typedef enum Mode
    {
        kModeNormal     = 0,  ///< Runs the normal trickle logic.
        kModePlainTimer = 1,  ///< Runs a normal timer between Imin and Imax.
        kModeMPL        = 2,  ///< Runs the trickle logic modified for MPL.
    } Mode;

    /**
     * This function pointer is called when the timer expires.
     *
     * @param[in]  aTimer  A reference to the expired timer.
     *
     * @retval TRUE   If the trickle timer should continue running.
     * @retval FALSE  If the trickle timer should stop running.
     */
    typedef bool (*Handler)(TrickleTimer &aTimer);

    /**
     * This constructor creates a trickle timer instance.
     *
     * @param[in]  aScheduler               A reference to the timer scheduler.
     * @param[in]  aRedundancyConstant      The redundancy constant for the timer, k.
     * @param[in]  aTransmitHandler         A pointer to a function that is called when transmission should occur.
     * @param[in]  aIntervalExpiredHandler  An optional pointer to a function that is called when the interval expires.
     * @param[in]  aContext                 A pointer to arbitrary context information.
     *
     */
    TrickleTimer(TimerScheduler &aScheduler,
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
                 uint32_t aRedundancyConstant,
#endif
                 Handler aTransmitHandler, Handler aIntervalExpiredHandler, void *aContext);

    /**
     * This method indicates whether or not the trickle timer instance is running.
     *
     * @retval TRUE   If the trickle timer is running.
     * @retval FALSE  If the trickle timer is not running.
     */
    bool IsRunning(void) const;

    /**
     * This method start the trickle timer.
     *
     * @param[in]  aIntervalMin  The minimum interval for the timer, Imin.
     * @param[in]  aIntervalMax  The maximum interval for the timer, Imax.
     * @param[in]  aMode         The operating mode for the timer.
     *
     */
    void Start(uint32_t aIntervalMin, uint32_t aIntervalMax, Mode aMode);

    /**
     * This method start the trickle timer.
     *
     * @param[in]  aStartTime    The start time.
     * @param[in]  aIntervalMin  The minimum interval for the timer, Imin.
     * @param[in]  aIntervalMax  The maximum interval for the timer, Imax.
     * @param[in]  aMode         The operating mode for the timer.
     *
     */
    void StartAt(uint32_t aStartTime, uint32_t aIntervalMin, uint32_t aIntervalMax, Mode aMode);

    /**
     * This method stops the trickle timer.
     *
     */
    void Stop(void);

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    /**
     * This method indicates to the trickle timer a 'consistent' state.
     *
     */
    void IndicateConsistent(void);
#endif

    /**
     * This method indicates to the trickle timer an 'inconsistent' state.
     *
     */
    void IndicateInconsistent(void);

private:
    bool TransmitFired(void) { return mTransmitHandler(*this); }
    bool IntervalExpiredFired(void) { return mIntervalExpiredHandler ? mIntervalExpiredHandler(*this) : true; }

    void StartNewInterval(void);

    static void HandleTimerFired(Timer &aTimer);
    void HandleTimerFired(void);

    // Shadow base class method to ensure it is hidden.
    void StartAt(void) { }

    typedef enum Phase
    {
        ///< Indicates we are currently not running
        kPhaseDormant    = 1,
        ///< Indicates that when the timer expires, it should evaluate for transmit callbacks
        kPhaseTransmit   = 2,
        ///< Indicates that when the timer expires, it should process interval expiration callbacks
        kPhaseInterval   = 3,
    } Phase;

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    // Redundancy constant
    const uint32_t k;

    // A counter, keeping track of the number of "consistent" transmissions received
    uint32_t c;
#endif

    // Minimum interval size
    uint32_t Imin;
    // Maximum interval size
    uint32_t Imax;
    // The mode of operation
    Mode mMode;

    // The current interval size (in milliseconds)
    uint32_t I;
    // The time (in milliseconds) into the interval at which we should transmit
    uint32_t t;

    // The current trickle phase for the timer
    Phase mPhase;

    // Callback variables
    Handler mTransmitHandler;
    Handler mIntervalExpiredHandler;
};

/**
 * @}
 *
 */

}  // namespace ot

#endif  // TRICKLE_TIMER_HPP_
