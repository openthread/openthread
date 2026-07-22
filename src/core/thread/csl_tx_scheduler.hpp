/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#ifndef OT_CORE_THREAD_CSL_TX_SCHEDULER_HPP_
#define OT_CORE_THREAD_CSL_TX_SCHEDULER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"
#include "thread/indirect_sender_frame_context.hpp"

namespace ot {

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   This module includes definitions for CSL transmission scheduling.
 *
 * @{
 */

class CslNeighbor;

/**
 * Implements CSL tx scheduling functionality.
 */
class CslTxScheduler : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;
    friend class Radio::Callbacks;
    friend class IndirectSender;

public:
    static constexpr uint8_t kMaxCslTriggeredTxAttempts = OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS;

    /**
     * Defines all the neighbor info required for scheduling CSL transmissions.
     *
     * `CslNeighbor` publicly inherits from this class.
     */
    class NeighborInfo
    {
    public:
        /**
         * Returns the number of CSL triggered tx attempts.
         *
         * @returns The number of CSL triggered tx attempts.
         */
        uint8_t GetCslTxAttempts(void) const { return mCslTxAttempts; }

        /**
         * Increments the number of CSL triggered tx attempts.
         */
        void IncrementCslTxAttempts(void) { mCslTxAttempts++; }

        /**
         * Resets the number of CSL triggered tx attempts to zero.
         */
        void ResetCslTxAttempts(void) { mCslTxAttempts = 0; }

        /**
         * Indicates whether or not the neighbor is CSL synchronized.
         *
         * @retval TRUE   If the neighbor is CSL synchronized.
         * @retval FALSE  If the neighbor is not CSL synchronized.
         */
        bool IsCslSynchronized(void) const { return mCslSynchronized && mCslPeriod > 0; }

        /**
         * Sets whether or not the neighbor is CSL synchronized.
         *
         * @param[in] aCslSynchronized  TRUE if the neighbor is CSL synchronized, FALSE otherwise.
         */
        void SetCslSynchronized(bool aCslSynchronized) { mCslSynchronized = aCslSynchronized; }

        /**
         * Returns the CSL channel the neighbor listens on.
         *
         * @returns The CSL channel.
         */
        uint8_t GetCslChannel(void) const { return mCslChannel; }

        /**
         * Sets the CSL channel the neighbor listens on.
         *
         * @param[in] aChannel  The CSL channel.
         */
        void SetCslChannel(uint8_t aChannel) { mCslChannel = aChannel; }

        /**
         * Returns the CSL sync timeout in seconds.
         *
         * @returns The CSL sync timeout in seconds.
         */
        uint32_t GetCslTimeout(void) const { return mCslTimeout; }

        /**
         * Sets the CSL sync timeout in seconds.
         *
         * @param[in] aTimeout  The CSL sync timeout in seconds.
         */
        void SetCslTimeout(uint32_t aTimeout) { mCslTimeout = aTimeout; }

        /**
         * Returns the CSL sampled listening period in units of 10 symbols (160 microseconds).
         *
         * @returns The CSL sampled listening period.
         */
        uint16_t GetCslPeriod(void) const { return mCslPeriod; }

        /**
         * Sets the CSL sampled listening period in units of 10 symbols (160 microseconds).
         *
         * @param[in] aPeriod  The CSL sampled listening period.
         */
        void SetCslPeriod(uint16_t aPeriod) { mCslPeriod = aPeriod; }

        /**
         * Returns the CSL phase in units of 10 symbols (160 microseconds).
         *
         * @returns The CSL phase.
         */
        uint16_t GetCslPhase(void) const { return mCslPhase; }

        /**
         * Sets the CSL phase in units of 10 symbols (160 microseconds).
         *
         * @param[in] aPhase  The CSL phase.
         */
        void SetCslPhase(uint16_t aPhase) { mCslPhase = aPhase; }

        /**
         * Returns the time (local milliseconds `TimeMilli`) when the last frame containing a CSL IE was heard.
         *
         * @returns The last heard time.
         */
        TimeMilli GetCslLastHeard(void) const { return mCslLastHeard; }

        /**
         * Sets the time (local milliseconds `TimeMilli`) when the last frame containing a CSL IE was heard.
         *
         * @param[in] aCslLastHeard  The last heard time.
         */
        void SetCslLastHeard(TimeMilli aCslLastHeard) { mCslLastHeard = aCslLastHeard; }

        /**
         * Returns the radio clock timestamp (microseconds `Radio::Time64`) when the last frame containing a CSL IE
         * was received.
         *
         * @returns The last RX timestamp.
         */
        Radio::Time64 GetLastRxTimestamp(void) const { return mLastRxTimestamp; }

        /**
         * Sets the radio clock timestamp (microseconds `Radio::Time64`) when the last frame containing a CSL IE
         * was received.
         *
         * @param[in] aLastRxTimestamp  The last RX timestamp.
         */
        void SetLastRxTimestamp(Radio::Time64 aLastRxTimestamp) { mLastRxTimestamp = aLastRxTimestamp; }

    private:
        uint8_t       mCslTxAttempts : 7;
        bool          mCslSynchronized : 1;
        uint8_t       mCslChannel;
        uint32_t      mCslTimeout;      // In seconds
        uint16_t      mCslPeriod;       // In units of 10 symbols
        uint16_t      mCslPhase;        // In units of 10 symbols
        TimeMilli     mCslLastHeard;    // Local milliseconds clock time
        Radio::Time64 mLastRxTimestamp; // Radio microseconds clock time

        static_assert(kMaxCslTriggeredTxAttempts < (1 << 7), "mCslTxAttempts cannot fit max!");
    };

    /**
     * Initializes the CSL tx scheduler object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     */
    explicit CslTxScheduler(Instance &aInstance);

    /**
     * Updates the CSL transmission schedule.
     *
     * If no CSL transmission is currently in progress, this method finds the CSL neighbor with the earliest
     * upcoming receive window that also has a pending indirect message, and schedules its frame transmission.
     *
     * If a CSL transmission is ongoing at the MAC layer but the corresponding indirect message being transmitted
     * is modified or removed, this method updates the scheduler's internal state to indicate that the active CSL
     * transmission has been aborted.
     */
    void Update(void);

    /**
     * Clears all the states inside `CslTxScheduler` and the related states in each child.
     */
    void Clear(void);

private:
    // Guard time in usec to add when checking delay while preparing the CSL frame for tx.
    static constexpr uint32_t kFramePreparationGuardInterval = 1500;

    typedef IndirectSenderBase::FrameContext FrameContext;

    void RescheduleCslTx(void);
    void HandleTimer(void);

    uint32_t GetNextCslTransmissionDelay(const CslNeighbor &aCslNeighbor,
                                         uint32_t          &aDelayFromLastRx,
                                         uint32_t           aAheadUs) const;

    // Callbacks from `Mac`
    Mac::TxFrame *HandleFrameRequest(Mac::TxFrames &aTxFrames);
    void          HandleSentFrame(const Mac::TxFrame &aFrame, Error aError);

    // Callback from `Radio`
    void HandleRadioBusLatencyChanged(void);

    using CslTxTimer = TimerMilliIn<CslTxScheduler, &CslTxScheduler::HandleTimer>;

    uint32_t     mCslFrameRequestAheadUs;
    CslNeighbor *mCslTxNeighbor;
    Message     *mCslTxMessage;
    FrameContext mFrameContext;
    CslTxTimer   mTimer;
};

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#endif // OT_CORE_THREAD_CSL_TX_SCHEDULER_HPP_
