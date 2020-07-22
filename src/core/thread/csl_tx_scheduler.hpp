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

#ifndef CSL_TX_SCHEDULER_HPP_
#define CSL_TX_SCHEDULER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/time.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
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

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

class Child;

class CslTxScheduler : public InstanceLocator
{
    friend class Mac::Mac;
    friend class IndirectSender;

public:
    enum
    {
        kMaxCslTriggeredTxAttempts     = OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS,
        kCslFrameRequestAheadThreshold = 2000 / OT_US_PER_TEN_SYMBOLS,
    };

    /**
     * This class defines all the child info required for scheduling CSL transmissions.
     *
     * `Child` class publicly inherits from this class.
     *
     */
    class ChildInfo
    {
    public:
        uint8_t GetCslTxAttempts(void) const { return mCslTxAttempts; }
        void    SetCslTxAttempts(uint8_t aCslTxAttempts) { mCslTxAttempts = aCslTxAttempts; }
        void    IncrementCslTxAttempts(void) { mCslTxAttempts++; }
        void    ResetCslTxAttempts(void) { SetCslTxAttempts(0); }

        bool IsCslSynchronized(void) const { return mCslSynchronized && mCslPeriod > 0; }
        void SetCslSynchronized(bool aCslSynchronized) { mCslSynchronized = aCslSynchronized; }

        uint8_t GetCslChannel(void) const { return mCslChannel; }
        void    SetCslChannel(uint8_t aChannel) { mCslChannel = aChannel; }

        uint32_t GetCslSyncTimeout(void) const { return mCslSyncTimeout; }
        void     SetCslSyncTimeout(uint32_t aTimeout) { mCslSyncTimeout = aTimeout; }

        uint16_t GetCslPeriod(void) const { return mCslPeriod; }
        void     SetCslPeriod(uint16_t aPeriod) { mCslPeriod = aPeriod; }

        uint16_t GetCslPhase(void) const { return mCslPhase; }
        void     SetCslPhase(uint16_t aPhase) { mCslPhase = aPhase; }

        TimeMilli GetCslLastHeard(void) const { return mCslLastHeard; }
        void      SetCslLastHeard(TimeMilli aCslLastHeard) { mCslLastHeard = aCslLastHeard; }

    private:
        uint8_t   mCslTxAttempts : 7;   // Number of csl triggered tx attempts.
        bool      mCslSynchronized : 1; ///< Indicates whether or not the child is CSL synchronized.
        uint8_t   mCslChannel;          ///< The channel the device will listen on.
        uint32_t  mCslSyncTimeout;      ///< The sync timeout, in microseconds.
        uint16_t  mCslPeriod;           ///< The CSL sample period in 10 symbols(160 microseconds)
        uint16_t  mCslPhase;            ///< The time when the next CSL sample will start.
        TimeMilli mCslLastHeard;        ///< Time when last frame containing CSL IE heard.

        static_assert(kMaxCslTriggeredTxAttempts < (1 << 5), "mCslTxAttempts cannot fit max!");
    };

    /**
     * This class defines the callbacks used by the `CslTxScheduler`.
     *
     */
    class Callbacks : public InstanceLocator
    {
        friend class CslTxScheduler;

    private:
        typedef IndirectSenderBase::FrameContext FrameContext;

        /**
         * This constructor initializes the callbacks object.
         *
         * @param[in]  aInstance   A reference to the OpenThread instance.
         *
         */
        explicit Callbacks(Instance &aInstance);

        otError PrepareFrameForChild(Mac::TxFrame &aFrame, FrameContext &aContext, Child &aChild);

        void HandleSentFrameToChild(const Mac::TxFrame &aFrame,
                                    const FrameContext &aContext,
                                    otError             aError,
                                    Child &             aChild);
    };
    /**
     * This constructor initializes the csl tx scheduler object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     *
     */
    explicit CslTxScheduler(Instance &aInstance);

    void Update(void);

    void Clear(void);

private:
    /**
     * This method always finds the most recent csl tx among all children,
     * and request `Mac` to do csl tx at specific time. It shouldn't be called
     * when `Mac` is already starting to do the csl tx(indicated by `mBusy`).
     *
     */
    void RescheduleCslTx(void);

    uint32_t GetNextCslTransmissionDelay(const Child &aChild, uint64_t aRadioNow);

    otError HandleFrameRequest(Mac::TxFrame &aFrame);

    void HandleSentFrame(const Mac::TxFrame &aFrame, otError aError);

    void HandleSentFrame(const Mac::TxFrame &aFrame, otError aError, Child &aChild);

    Child *                 mCslTxChild;
    bool                    mBusy;
    Callbacks::FrameContext mFrameContext;
    Callbacks               mCallbacks;
};

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

/**
 * @}
 *
 */

} // namespace ot

#endif // CSL_TX_SCHEDULER_HPP_
