/*
 *  Copyright (c) 2019-2026, The OpenThread Authors.
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
 *   This file includes definitions for OpenThread radio statistics.
 */

#ifndef OT_CORE_RADIO_RADIO_STATISTICS_HPP_
#define OT_CORE_RADIO_RADIO_STATISTICS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)

#include <openthread/radio_stats.h>

#include "common/error.hpp"
#include "common/non_copyable.hpp"
#include "common/time.hpp"

namespace ot {
namespace Radio {

/**
 * @addtogroup core-radio
 *
 * @{
 */

class Radio;
class Callbacks;

/**
 * Implements the radio statistics logic.
 *
 * The radio statistics are the time when the radio in TX/RX/radio state.
 * Since this class collects these statistics from pure software level
 * and no platform API is involved, a simplified model is used to
 * calculate the time of different radio states. The data may not be very
 * accurate, but it's sufficient to provide a general understanding of
 * the proportion of time a device is in different radio states.
 *
 * The simplified model is:
 * - The radio statistics is only aware of 2 states: RX and sleep.
 * - Each time `Radio::Receive` or `Radio::Sleep` is called, it will check
 *   the current state and add the time since last time the methods were
 *   called. For example, `Sleep` is first called and `Receive` is called
 *   after 1 second, then 1 second will be added to SleepTime and the
 *   current state switches to `Receive`.
 * - The time of TX will be calculated from the callback of TransmitDone.
 *   If TX returns kErrorNone or kErrorNoAck, the tx time will be added
 *   according to the number of bytes sent. And the SleepTime or RxTime
 *   will be reduced accordingly.
 * - When `GetStats` is called, an operation will be executed to calculate
 *   the time for the last state. And the result will be returned.
 */
class Statistics : private NonCopyable
{
    friend class Radio;
    friend class Callbacks;

public:
    using TimeStats = otRadioTimeStats; ///< Radio statistics (time spend in each state).

    /**
     * Retrieves the current radio statistics.
     *
     * @return The current time statistics.
     */
    const TimeStats &GetStats(void);

    /**
     * Resets the radio statistics.
     */
    void ResetTime(void);

private:
    enum Status : uint8_t
    {
        kDisabled,
        kSleep,
        kReceive,
    };

    Statistics(void);
    void RecordStateChange(Status aStatus);
    void HandleReceiveAt(uint32_t aDurationUs);
    void RecordTxDone(Error aError, uint16_t aPsduLength);
    void RecordRxDone(Error aError);
    void UpdateTime(void);

    Status    mStatus;
    TimeStats mTimeStats;
    TimeMicro mLastUpdateTime;
};

/**
 * @}
 */

} // namespace Radio
} // namespace ot

#endif // OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)

#endif // OT_CORE_RADIO_RADIO_STATISTICS_HPP_
