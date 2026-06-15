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

/**
 * @file
 *   This file includes definitions for the Thread Direct Wake Initiator TX scheduler.
 */

#ifndef OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_
#define OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

#include <openthread/thread_direct.h>

#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"

namespace ot {

/**
 * Implements Wake Initiator (WI) wake frame TX scheduling.
 *
 * Transmits periodic IEEE 802.15.4 MAC Command Wake Frames on the configured wake
 * channel at a fixed interval for the specified burst duration, then opens a
 * connection window during which a TD Link Command from the Wake Listener is expected.
 */
class WakeupTxScheduler : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;

public:
    /**
     * Initializes the Wake Frame TX scheduler.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     */
    explicit WakeupTxScheduler(Instance &aInstance);

    /**
     * Starts a Thread Direct wake burst targeting @p aWlExtAddress.
     *
     * When @p aWakeType is Frame::kWakeFrameTypeDirectLink, a connection window opens after
     * the burst; OT_THREAD_DIRECT_EVENT_LINK_FAILED fires on expiry without a response.
     *
     * @param[in] aWlExtAddress  Extended address of the Wake Listener.
     * @param[in] aWakeType      Wake frame type.
     * @param[in] aIntervalUs    Interval between consecutive Wake Frames in us.
     * @param[in] aDurationMs    Wake burst duration in ms.
     * @param[in] aKeyIndex      Key index for Wake Frame security.
     *
     * @retval kErrorNone          Wake burst started.
     * @retval kErrorInvalidArgs   @p aIntervalUs or @p aDurationMs is invalid.
     * @retval kErrorInvalidState  A wake burst is already in progress.
     */
    Error StartWakeup(const Mac::ExtAddress    &aWlExtAddress,
                      Mac::Frame::WakeFrameType aWakeType,
                      uint16_t                  aIntervalUs,
                      uint16_t                  aDurationMs,
                      uint8_t                   aKeyIndex);

    /**
     * Stops the running wake burst and cancels the connection window.
     *
     * The completion callback is not invoked.
     */
    void Stop(void);

    bool IsRunning(void) const { return mIsRunning || (mTdWakeState != kTdWakeIdle); }

    /**
     * Updates `mTxRequestAheadTimeUs` based on bus speed and latency.
     */
    void UpdateFrameRequestAhead(void);

private:
    constexpr static uint16_t kDefaultWakeIntervalUs   = OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US;
    constexpr static uint16_t kDefaultWakeDurationMs   = OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS;
    constexpr static uint8_t  kConnectionRetryInterval = OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_INTERVAL;
    constexpr static uint8_t  kConnectionRetryCount    = OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_COUNT;
    constexpr static bool     kWakeFrameTxCca          = OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_FRAME_TX_CCA_ENABLE;

    // Returns the connection window duration in microseconds.
    // The WI keeps the connection window open long enough for the WL to send
    // its TD Link Command across all configured retry attempts.
    static constexpr uint32_t GetConnectionWindowUs(void)
    {
        return static_cast<uint32_t>(kConnectionRetryCount + 1) * kConnectionRetryInterval * kDefaultWakeIntervalUs;
    }

    enum TdWakeState : uint8_t
    {
        kTdWakeIdle, ///< No wake burst in progress.
        kTdWaking,   ///< Burst done; connection window open, awaiting WL TD Link Command.
    };

    // Starts the Wake Frame burst timer without opening the connection window.
    Error WakeUp(const Mac::ExtAddress    &aPeerExtAddress,
                 Mac::Frame::WakeFrameType aWakeType,
                 uint16_t                  aIntervalUs,
                 uint16_t                  aDurationMs);

    // Called by the MAC layer when a Wake Frame TX is about to start.
    Mac::TxFrame *PrepareWakeupFrame(Mac::TxFrames &aTxFrames);

    // Called at the start of a sequence and after each Wake Frame is prepared.
    void ScheduleTimer(void);

    void RequestWakeupFrameTransmission(void);

    // Fires when the connection window expires without a TD Link Command from the WL.
    void HandleConnectionWindowTimer(void);

    using WakeupTimer           = TimerMicroIn<WakeupTxScheduler, &WakeupTxScheduler::RequestWakeupFrameTransmission>;
    using ConnectionWindowTimer = TimerMicroIn<WakeupTxScheduler, &WakeupTxScheduler::HandleConnectionWindowTimer>;

    Mac::ExtAddress           mPeerExtAddress;       // Extended address of the WL being woken.
    Mac::Frame::WakeFrameType mWakeFrameType;        // Wake Frame type for the current burst.
    TimeMicro                 mTxTimeUs;             // Time of the next Wake Frame TX.
    TimeMicro                 mTxEndTimeUs;          // Time at which the burst ends.
    uint32_t                  mTxRequestAheadTimeUs; // How far ahead the MAC TX request must be issued.
    uint16_t                  mIntervalUs;           // Interval between consecutive Wake Frames.
    uint8_t                   mKeyIndex;             // Key index used for Wake Frame security in this burst.
    WakeupTimer               mTimer;
    ConnectionWindowTimer     mConnectionWindowTimer;
    TdWakeState               mTdWakeState;
    bool                      mIsRunning;
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

#endif // OT_CORE_MAC_WAKEUP_TX_SCHEDULER_HPP_
