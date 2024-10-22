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

#ifndef ENH_CSL_SENDER_HPP_
#define ENH_CSL_SENDER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/time.hpp"
#include "config/mac.h"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "thread/indirect_sender_frame_context.hpp"

namespace ot {

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   Includes definitions for Enhanced CSL transmission.
 * @{
 */

class Neighbor;

/**
 * Implements enhanced CSL tx functionality.
 */
class EnhCslSender : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;

public:
    static constexpr uint8_t kMaxEnhCslTriggeredTxAttempts = OPENTHREAD_CONFIG_MAC_ENH_CSL_TX_ATTEMPTS;

    /**
     * Defines all the peer info required for scheduling enhanced CSL transmissions.
     *
     * `Neighbor` class publicly inherits from this class.
     */
    class EnhCslPeerInfo
    {
    public:
        uint8_t GetEnhCslTxAttempts(void) const { return mCslTxAttempts; }
        void    IncrementEnhCslTxAttempts(void) { mCslTxAttempts++; }
        void    ResetEnhCslTxAttempts(void) { mCslTxAttempts = 0; }

        uint8_t GetIndirectDataSequenceNumber(void) const { return mIndirectDsn; }
        void    SetIndirectDataSequenceNumber(uint8_t aDsn) { mIndirectDsn = aDsn; }

        bool IsEnhCslSynchronized(void) const { return mCslSynchronized && mCslPeriod > 0; }
        void SetEnhCslSynchronized(bool aCslSynchronized) { mCslSynchronized = aCslSynchronized; }

        bool IsEnhCslPrevSnValid(void) const { return mCslPrevSnValid; }
        void SetEnhCslPrevSnValid(bool aCslPrevSnValid) { mCslPrevSnValid = aCslPrevSnValid; }

        uint8_t GetEnhCslPrevSn(void) const { return mCslPrevSn; }
        void    SetEnhCslPrevSn(uint8_t aCslPrevSn) { mCslPrevSn = aCslPrevSn; }

        uint16_t GetEnhCslPeriod(void) const { return mCslPeriod; }
        void     SetEnhCslPeriod(uint16_t aPeriod) { mCslPeriod = aPeriod; }

        uint16_t GetEnhCslPhase(void) const { return mCslPhase; }
        void     SetEnhCslPhase(uint16_t aPhase) { mCslPhase = aPhase; }

        TimeMilli GetEnhCslLastHeard(void) const { return mCslLastHeard; }
        void      SetEnhCslLastHeard(TimeMilli aCslLastHeard) { mCslLastHeard = aCslLastHeard; }

        uint64_t GetEnhLastRxTimestamp(void) const { return mLastRxTimestamp; }
        void     SetEnhLastRxTimestamp(uint64_t aLastRxTimestamp) { mLastRxTimestamp = aLastRxTimestamp; }

        uint32_t GetIndirectFrameCounter(void) const { return mIndirectFrameCounter; }
        void     SetIndirectFrameCounter(uint32_t aFrameCounter) { mIndirectFrameCounter = aFrameCounter; }

        uint8_t GetIndirectKeyId(void) const { return mIndirectKeyId; }
        void    SetIndirectKeyId(uint8_t aKeyId) { mIndirectKeyId = aKeyId; }

        Message *GetIndirectMessage(void) { return mIndirectMessage; }
        void     SetIndirectMessage(Message *aMessage) { mIndirectMessage = aMessage; }

        uint16_t GetIndirectMessageCount(void) const { return mQueuedMessageCount; }
        void     IncrementIndirectMessageCount(void) { mQueuedMessageCount++; }
        void     DecrementIndirectMessageCount(void) { mQueuedMessageCount--; }
        void     ResetIndirectMessageCount(void) { mQueuedMessageCount = 0; }

        uint16_t GetIndirectFragmentOffset(void) const { return mIndirectFragmentOffset; }
        void     SetIndirectFragmentOffset(uint16_t aFragmentOffset) { mIndirectFragmentOffset = aFragmentOffset; }

        uint8_t GetEnhCslMaxTxAttempts(void) const
        {
            return mCslMaxTxAttempts != 0 ? mCslMaxTxAttempts : EnhCslSender::kMaxEnhCslTriggeredTxAttempts;
        }
        void SetEnhCslMaxTxAttempts(uint8_t txAttempts) { mCslMaxTxAttempts = txAttempts; }
        void ResetEnhCslMaxTxAttempts() { mCslMaxTxAttempts = 0; }

    private:
        uint8_t   mCslTxAttempts : 6;   ///< Number of enhanced CSL triggered tx attempts.
        bool      mCslSynchronized : 1; ///< Indicates whether or not the peer is enhanced CSL synchronized.
        bool      mCslPrevSnValid : 1;  ///< Indicates whether or not the previous MAC frame sequence number was set.
        uint8_t   mCslMaxTxAttempts;    ///< Override for the maximum number of enhanced CSL triggered tx attempts.
        uint16_t  mCslPeriod;    ///< Enhanced CSL sampled listening period in units of 10 symbols (160 microseconds).
        uint16_t  mCslPhase;     ///< The time when the next CSL sample will start.
        TimeMilli mCslLastHeard; ///< Time when last frame containing CSL IE was heard.
        uint64_t  mLastRxTimestamp; ///< Time when last frame containing CSL IE was received, in microseconds.

        uint8_t  mCslPrevSn;            ///< The previous MAC frame sequence number (for MAC-level frame deduplication).
        uint8_t  mIndirectDsn;          // MAC level Data Sequence Number (DSN) for retx attempts.
        uint8_t  mIndirectKeyId;        // Key Id for current indirect frame (used for retx).
        uint32_t mIndirectFrameCounter; // Frame counter for current indirect frame (used for retx).

        Message *mIndirectMessage;        // Current indirect message.
        uint16_t mQueuedMessageCount;     // Number of queued indirect messages for the peer.
        uint16_t mIndirectFragmentOffset; // 6LoWPAN fragment offset for the indirect message.
    };

    /**
     * This constructor initializes the enhanced CSL sender object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     */
    explicit EnhCslSender(Instance &aInstance);

    /**
     * Adds a message for enhanced CSL transmission to a neighbor.
     *
     * @param[in] aMessage     The message to add.
     * @param[in] aNeighbor    The neighbor for enhanced CSL transmission.
     */
    void AddMessageForCslPeer(Message &aMessage, Neighbor &aNeighbor);

    /**
     * Removes all added messages for a specific neighbor.
     *
     * @param[in] aNeighbor    The neighbor for enhanced CSL transmission.
     */
    void ClearAllMessagesForCslPeer(Neighbor &aNeighbor);

    /**
     * Updates the next CSL transmission (finds the nearest neighbor).
     *
     * It would then request the `Mac` to do the CSL tx. If the last CSL tx has been fired at `Mac` but hasn't been
     * done yet, and it's aborted, this method would set `mCslTxNeighbor` to `nullptr` to notify the
     * `HandleTransmitDone` that the operation has been aborted.
     */
    void Update(void);

    /**
     * Returns the current parent or parent candidate.
     *
     * @returns The current parent or parent candidate.
     */
    Neighbor *GetParent(void) const;

private:
    // Guard time in usec to add when checking delay while preparaing the CSL frame for tx.
    static constexpr uint32_t kFramePreparationGuardInterval = 1500;

    typedef IndirectSenderBase::FrameContext FrameContext;

    void     InitFrameRequestAhead(void);
    void     RescheduleCslTx(void);
    uint32_t GetNextCslTransmissionDelay(Neighbor &aNeighbor, uint32_t &aDelayFromLastRx, uint32_t aAheadUs) const;
    Error    PrepareFrameForNeighbor(Mac::TxFrame &aFrame, FrameContext &aContext, Neighbor &aNeighbor);
    uint16_t PrepareDataFrame(Mac::TxFrame &aFrame, Neighbor &aNeighbor, Message &aMessage);
    void     HandleSentFrameToNeighbor(const Mac::TxFrame &aFrame,
                                       const FrameContext &aContext,
                                       otError             aError,
                                       Neighbor           &aNeighbor);

    // Callbacks from `Mac`
    Mac::TxFrame *HandleFrameRequest(Mac::TxFrames &aTxFrames);
    void          HandleSentFrame(const Mac::TxFrame &aFrame, Error aError);

    void HandleSentFrame(const Mac::TxFrame &aFrame, Error aError, Neighbor &aNeighbor);

    Neighbor    *mCslTxNeigh;
    Message     *mCslTxMessage;
    uint32_t     mCslFrameRequestAheadUs;
    FrameContext mFrameContext;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#endif // ENH_CSL_SENDER_HPP_
