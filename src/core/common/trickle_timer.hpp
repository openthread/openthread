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

#include "openthread-core-config.h"

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
class TrickleTimer : public TimerMilli
{
public:
    /**
     * This enumeration defines the modes of operation for the `TrickleTimer`.
     *
     */
    enum Mode
    {
        kModeNormal,     ///< Runs the normal trickle logic (as per RFC6206).
        kModePlainTimer, ///< Runs a plain timer with random interval selected between min/max intervals.
        kModeMPL,        ///< Runs the trickle logic modified for MPL.
    };

    /**
     * This function pointer is called when the timer expires.
     *
     * @param[in]  aTimer  A reference to the trickle timer.
     *
     * @retval TRUE   If the trickle timer should continue running.
     * @retval FALSE  If the trickle timer should stop running.
     *
     */
    typedef bool (*Handler)(TrickleTimer &aTimer);

    /**
     * This constructor initializes a `TrickleTimer` instance.
     *
     * @param[in]  aInstance                A reference to the OpenThread instance.
     * @param[in]  aRedundancyConstant      The redundancy constant for the timer, also known as `k`.
     * @param[in]  aTransmitHandler         A pointer to a function that is called when transmission should occur.
     * @param[in]  aIntervalExpiredHandler  An optional pointer to a function that is called when the interval expires.
     * @param[in]  aOwner                   A pointer to owner of the `TrickleTimer` object.
     *
     */
    TrickleTimer(Instance &aInstance,
#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
                 uint32_t aRedundancyConstant,
#endif
                 Handler aTransmitHandler,
                 Handler aIntervalExpiredHandler,
                 void *  aOwner);

    /**
     * This method indicates whether or not the trickle timer instance is running.
     *
     * @retval TRUE   If the trickle timer is running.
     * @retval FALSE  If the trickle timer is not running.
     *
     */
    bool IsRunning(void) const { return mIsRunning; }

    /**
     * This method starts the trickle timer.
     *
     * @param[in]  aIntervalMin  The minimum interval for the timer in milliseconds.
     * @param[in]  aIntervalMax  The maximum interval for the timer in milliseconds.
     * @param[in]  aMode         The operating mode for the timer.
     *
     * @retval OT_ERROR_NONE           The timer started successfully.
     * @retval OT_ERROR_INVALID_ARGS   The given parameters are invalid (i.e., max interval is smaller than min).
     *
     */
    otError Start(uint32_t aIntervalMin, uint32_t aIntervalMax, Mode aMode);

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
    void IndicateConsistent(void) { mCounter++; }
#endif

    /**
     * This method indicates to the trickle timer an 'inconsistent' state.
     *
     */
    void IndicateInconsistent(void);

private:
    void        StartNewInterval(void);
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);
    void        HandleEndOfTimeInInterval(void);
    void        HandleEndOfInterval(void);
    void        StartAt(void) {} // Shadow base class `TimerMilli` method to ensure it is hidden.

#ifdef ENABLE_TRICKLE_TIMER_SUPPRESSION_SUPPORT
    const uint32_t mRedundancyConstant; // Redundancy constant (aka 'k').
    uint32_t       mCounter;            // A counter for number of "consistent" transmissions (aka 'c').
#endif

    uint32_t mIntervalMin;            // Minimum interval (aka `Imin`).
    uint32_t mIntervalMax;            // Maximum interval (aka `Imax`).
    uint32_t mInterval;               // Current interval (aka `I`).
    uint32_t mTimeInInterval;         // Time in interval (aka `t`).
    Handler  mTransmitHandler;        // Transmit handler callback.
    Handler  mIntervalExpiredHandler; // Interval expired handler callback.
    Mode     mMode;                   // Trickle timer mode.
    bool     mIsRunning : 1;          // Indicates if the trickle timer is running.
    bool     mInTransmitPhase : 1;    // Indicates if in transmit phase (before time `t` in current interval `I`).
};

/**
 * @}
 *
 */

} // namespace ot

#endif // TRICKLE_TIMER_HPP_
