/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_
#define OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"

namespace ot {

class Child;

/**
 * Implements wake-up sequence TX scheduling functionality.
 */
class WakeupTxScheduler : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;

public:
    /**
     * Initializes the wake-up sequence TX scheduler object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     */
    explicit WakeupTxScheduler(Instance &aInstance);

    /**
     * Returns the end of the wake-up sequence time.
     *
     * @returns End of the wake-up sequence time.
     */
    TimeMicro GetTxEndTime(void) const { return mTxEndTimeUs; }

    /**
     * Stops the ongoing wake-up sequence.
     */
    void Stop(void);

    /**
     * Updates the value of `mTxRequestAheadTimeUs`, based on bus speed, bus latency and `Mac::kCslRequestAhead`.
     */
    void UpdateFrameRequestAhead(void);

    /**
     * Returns true if a wake-up sequence is currently running.
     */
    bool IsRunning(void) const { return mIsRunning; }

private:
    constexpr static bool kWakeFrameTxCca = OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_FRAME_TX_CCA_ENABLE;

    // Called by the MAC layer when a wake-up frame transmission is about to be started.
    Mac::TxFrame *PrepareWakeupFrame(Mac::TxFrames &aTxFrames);

    // Called at the start of a sequence and after each Wake Frame is prepared.
    void ScheduleTimer(void);

    void RequestWakeupFrameTransmission(void);

    using WakeupTimer = TimerMicroIn<WakeupTxScheduler, &WakeupTxScheduler::RequestWakeupFrameTransmission>;

    TimeMicro   mTxTimeUs;             // Time of the next Wake Frame TX.
    TimeMicro   mTxEndTimeUs;          // Time at which the burst ends.
    uint32_t    mTxRequestAheadTimeUs; // How far ahead the MAC TX request must be issued.
    uint16_t    mIntervalUs;           // Interval between consecutive Wake Frames.
    WakeupTimer mTimer;
    bool        mIsRunning;
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

#endif // OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_
