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

#ifndef WAKEUP_TX_SCHEDULER_HPP_
#define WAKEUP_TX_SCHEDULER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE

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
     * Initiates the wake-up sequence to a Wake-up End Device.
     *
     * @param[in] aWedAddress The extended address of the Wake-up End Device.
     * @param[in] aIntervalUs An interval between consecutive wake-up frames (in microseconds).
     * @param[in] aDurationMs Duration of the wake-up sequence (in milliseconds).
     *
     * @retval kErrorNone         Successfully started the wake-up sequence.
     * @retval kErrorInvalidState This or another device is currently being woken-up.
     */
    Error WakeUp(const Mac::ExtAddress &aWedAddress, uint16_t aIntervalUs, uint16_t aDurationMs);

    /**
     * Returns the connection window used by this device.
     *
     * The connection window is amount of time that this device waits for an initial link establishment message after
     * sending the last wake-up frame.
     *
     * @returns Connection window in the units of microseconds.
     */
    uint32_t GetConnectionWindowUs(void) const
    {
        return mIntervalUs * kConnectionRetryInterval * kConnectionRetryCount;
    }

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

private:
    constexpr static uint8_t  kConnectionRetryInterval = OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_CONNECTION_RETRY_INTERVAL;
    constexpr static uint8_t  kConnectionRetryCount    = OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_CONNECTION_RETRY_COUNT;
    constexpr static uint32_t kWakeupFrameLength       = 54; // Includes SHR
    constexpr static bool     kWakeupFrameTxCca        = OPENTHREAD_CONFIG_WAKEUP_FRAME_TX_CCA_ENABLE;
    constexpr static uint32_t kParentRequestLength     = 78; // Includes SHR

    // Called by the MAC layer when a wake-up frame transmission is about to be started.
    Mac::TxFrame *PrepareWakeupFrame(Mac::TxFrames &aTxFrames);

    // Called at the beginning of a wake-up sequence and right after a wake-up frame has been prepared for transmission.
    void ScheduleTimer(void);

    void RequestWakeupFrameTransmission(void);

    using WakeupTimer = TimerMicroIn<WakeupTxScheduler, &WakeupTxScheduler::RequestWakeupFrameTransmission>;

    Mac::ExtAddress mWedAddress;
    TimeMicro       mTxTimeUs;             // Point in time when the next TX occurs.
    TimeMicro       mTxEndTimeUs;          // Point in time when the wake-up sequence is over.
    uint16_t        mTxRequestAheadTimeUs; // How much ahead the TX MAC operation needs to be requested.
    uint16_t        mIntervalUs;           // Interval between consecutive wake-up frames.
    WakeupTimer     mTimer;
    bool            mIsRunning;
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE

#endif // WAKEUP_TX_SCHEDULER_HPP_
