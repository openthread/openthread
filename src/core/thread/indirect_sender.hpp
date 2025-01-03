/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for handling indirect transmission.
 */

#ifndef INDIRECT_SENDER_HPP_
#define INDIRECT_SENDER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "mac/data_poll_handler.hpp"
#include "mac/mac_frame.hpp"
#include "thread/csl_tx_scheduler.hpp"
#include "thread/indirect_sender_frame_context.hpp"
#include "thread/mle_types.hpp"
#include "thread/src_match_controller.hpp"

namespace ot {

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   This module includes definitions for handling indirect transmissions.
 *
 * @{
 */

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

class CslNeighbor;
#if OPENTHREAD_FTD
class Child;
#endif

/**
 * Implements indirect transmission.
 */
class IndirectSender : public InstanceLocator, public IndirectSenderBase, private NonCopyable
{
    friend class Instance;
#if OPENTHREAD_FTD
    friend class DataPollHandler;
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    friend class CslTxScheduler;
#endif

public:
    /**
     * Defines all the neighbor info required for indirect (CSL or data-poll) transmission.
     *
     * Sub-classes of `Neighbor`, e.g., `CslNeighbor` or `Child` publicly inherits from this class.
     */
    class NeighborInfo
    {
        friend class IndirectSender;
#if OPENTHREAD_FTD
        friend class DataPollHandler;
        friend class SourceMatchController;
#endif
        friend class CslTxScheduler;

    public:
        /**
         * Returns the number of queued messages for the child.
         *
         * @returns Number of queued messages for the child.
         */
        uint16_t GetIndirectMessageCount(void) const { return mQueuedMessageCount; }

    private:
        Message *GetIndirectMessage(void) { return mIndirectMessage; }
        void     SetIndirectMessage(Message *aMessage) { mIndirectMessage = aMessage; }

        uint16_t GetIndirectFragmentOffset(void) const { return mIndirectFragmentOffset; }
        void     SetIndirectFragmentOffset(uint16_t aFragmentOffset) { mIndirectFragmentOffset = aFragmentOffset; }

        bool GetIndirectTxSuccess(void) const { return mIndirectTxSuccess; }
        void SetIndirectTxSuccess(bool aTxStatus) { mIndirectTxSuccess = aTxStatus; }

        bool IsIndirectSourceMatchShort(void) const { return mUseShortAddress; }
        void SetIndirectSourceMatchShort(bool aShort) { mUseShortAddress = aShort; }

        bool IsIndirectSourceMatchPending(void) const { return mSourceMatchPending; }
        void SetIndirectSourceMatchPending(bool aPending) { mSourceMatchPending = aPending; }

        void IncrementIndirectMessageCount(void) { mQueuedMessageCount++; }
        void DecrementIndirectMessageCount(void) { mQueuedMessageCount--; }
        void ResetIndirectMessageCount(void) { mQueuedMessageCount = 0; }

        bool IsWaitingForMessageUpdate(void) const { return mWaitingForMessageUpdate; }
        void SetWaitingForMessageUpdate(bool aNeedsUpdate) { mWaitingForMessageUpdate = aNeedsUpdate; }

        const Mac::Address &GetMacAddress(Mac::Address &aMacAddress) const;

        Message *mIndirectMessage;             // Current indirect message.
        uint16_t mIndirectFragmentOffset : 14; // 6LoWPAN fragment offset for the indirect message.
        bool     mIndirectTxSuccess : 1;       // Indicates tx success/failure of current indirect message.
        bool     mWaitingForMessageUpdate : 1; // Indicates waiting for updating the indirect message.
        uint16_t mQueuedMessageCount : 14;     // Number of queued indirect messages for the child.
        bool     mUseShortAddress : 1;         // Indicates whether to use short or extended address.
        bool     mSourceMatchPending : 1;      // Indicates whether or not pending to add to src match table.

        static_assert(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < (1UL << 14),
                      "mQueuedMessageCount cannot fit max required!");
    };

    /**
     * Represents a predicate function for checking if a given `Message` meets specific criteria.
     *
     * @param[in] aMessage The message to evaluate.
     *
     * @retval TRUE   If the @p aMessage satisfies the predicate condition.
     * @retval FALSE  If the @p aMessage does not satisfy the predicate condition.
     */
    typedef bool (&MessageChecker)(const Message &aMessage);

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit IndirectSender(Instance &aInstance);

    /**
     * Enables indirect transmissions.
     */
    void Start(void) { mEnabled = true; }

    /**
     * Disables indirect transmission.
     *
     * Any previously scheduled indirect transmission is canceled.
     */
    void Stop(void);

#if OPENTHREAD_FTD
    /**
     * Adds a message for indirect transmission to a sleepy child.
     *
     * @param[in] aMessage  The message to add.
     * @param[in] aChild    The (sleepy) child for indirect transmission.
     */
    void AddMessageForSleepyChild(Message &aMessage, Child &aChild);

    /**
     * Removes a message for indirect transmission to a sleepy child.
     *
     * @param[in] aMessage  The message to update.
     * @param[in] aChild    The (sleepy) child for indirect transmission.
     *
     * @retval kErrorNone          Successfully removed the message for indirect transmission.
     * @retval kErrorNotFound      The message was not scheduled for indirect transmission to the child.
     */
    Error RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild);

    /**
     * Removes all added messages for a specific child and frees message (with no indirect/direct tx).
     *
     * @param[in]  aChild  A reference to a child whose messages shall be removed.
     */
    void ClearAllMessagesForSleepyChild(Child &aChild);

    /**
     * Finds the first queued message for a given sleepy child that also satisfies the conditions of a given
     * `MessageChecker`.
     *
     * The caller MUST ensure that @p aChild is sleepy.
     *
     * @param[in] aChild     The sleepy child to check.
     * @param[in] aChecker   The predicate function to apply.
     *
     * @returns A pointer to the matching queued message, or `nullptr` if none is found.
     */
    Message *FindQueuedMessageForSleepyChild(const Child &aChild, MessageChecker aChecker)
    {
        return AsNonConst(AsConst(this)->FindQueuedMessageForSleepyChild(aChild, aChecker));
    }

    /**
     * Finds the first queued message for a given sleepy child that also satisfies the conditions of a given
     * `MessageChecker`.
     *
     * The caller MUST ensure that @p aChild is sleepy.
     *
     * @param[in] aChild     The sleepy child to check.
     * @param[in] aChecker   The predicate function to apply.
     *
     * @returns A pointer to the matching queued message, or `nullptr` if none is found.
     */
    const Message *FindQueuedMessageForSleepyChild(const Child &aChild, MessageChecker aChecker) const;

    /**
     * Indicates whether there is any queued message for a given sleepy child that also satisfies the conditions of a
     * given `MessageChecker`.
     *
     * The caller MUST ensure that @p aChild is sleepy.
     *
     * @param[in] aChild    The sleepy child to check for.
     * @param[in] aChecker  The predicate function to apply.
     *
     * @retval TRUE   There is a queued message satisfying @p aChecker for sleepy child @p aChild.
     * @retval FALSE  There is no queued message satisfying @p aChecker for sleepy child @p aChild.
     */
    bool HasQueuedMessageForSleepyChild(const Child &aChild, MessageChecker aChecker) const
    {
        return (FindQueuedMessageForSleepyChild(aChild, aChecker) != nullptr);
    }

    /**
     * Sets whether to use the extended or short address for a child.
     *
     * @param[in] aChild            A reference to the child.
     * @param[in] aUseShortAddress  `true` to use short address, `false` to use extended address.
     */
    void SetChildUseShortAddress(Child &aChild, bool aUseShortAddress);

    /**
     * Handles a child mode change and updates any queued messages for the child accordingly.
     *
     * @param[in]  aChild    The child whose device mode was changed.
     * @param[in]  aOldMode  The old device mode of the child.
     */
    void HandleChildModeChange(Child &aChild, Mle::DeviceMode aOldMode);

#endif // OPENTHREAD_FTD

private:
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    // Callbacks from `CslTxScheduler`
    Error PrepareFrameForCslNeighbor(Mac::TxFrame &aFrame, FrameContext &aContext, CslNeighbor &aCslNeighbor);
    void  HandleSentFrameToCslNeighbor(const Mac::TxFrame &aFrame,
                                       const FrameContext &aContext,
                                       Error               aError,
                                       CslNeighbor        &aCslNeighbor);
#endif

#if OPENTHREAD_FTD
    // Callbacks from `DataPollHandler`
    Error PrepareFrameForChild(Mac::TxFrame &aFrame, FrameContext &aContext, Child &aChild);
    void  HandleSentFrameToChild(const Mac::TxFrame &aFrame, const FrameContext &aContext, Error aError, Child &aChild);
    void  HandleFrameChangeDone(Child &aChild);

    void     UpdateIndirectMessage(Child &aChild);
    void     RequestMessageUpdate(Child &aChild);
    uint16_t PrepareDataFrame(Mac::TxFrame &aFrame, Child &aChild, Message &aMessage);
    void     PrepareEmptyFrame(Mac::TxFrame &aFrame, Child &aChild, bool aAckRequest);
    void     ClearMessagesForRemovedChildren(void);

    static bool AcceptAnyMessage(const Message &aMessage);
    static bool AcceptSupervisionMessage(const Message &aMessage);
#endif // OPENTHREAD_FTD

    bool mEnabled;
#if OPENTHREAD_FTD
    SourceMatchController mSourceMatchController;
    DataPollHandler       mDataPollHandler;
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    CslTxScheduler mCslTxScheduler;
#endif
};

#endif // #if OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

/**
 * @}
 */

} // namespace ot

#endif // INDIRECT_SENDER_HPP_
