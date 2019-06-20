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
#include "mac/mac_frame.hpp"
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

class Child;

/**
 * This class implements indirect transmission.
 *
 */
class IndirectSender : public InstanceLocator
{
    friend class Instance;

public:
    /**
     * This class defines all the child info required for indirect transmission.
     *
     * `Child` class publicly inherits from this class.
     */
    class ChildInfo
    {
        friend class IndirectSender;
        friend class SourceMatchController;

    public:
        /**
         * This method returns the number of queued messages for the child.
         *
         * @returns Number of queued messages for the child.
         *
         */
        uint16_t GetIndirectMessageCount(void) const { return mQueuedMessageCount; }

    private:
        bool IsDataRequestPending(void) const { return mDataRequestPendig; }
        void SetDataRequestPending(bool aPending) { mDataRequestPendig = aPending; }

        Message *GetIndirectMessage(void) { return mIndirectMessage; }
        void     SetIndirectMessage(Message *aMessage) { mIndirectMessage = aMessage; }

        uint16_t GetIndirectFragmentOffset(void) const { return mIndirectFragmentOffset; }
        void     SetIndirectFragmentOffset(uint16_t aFragmentOffset) { mIndirectFragmentOffset = aFragmentOffset; }

        bool GetIndirectTxSuccess(void) const { return mIndirectTxSuccess; }
        void SetIndirectTxSuccess(bool aTxStatus) { mIndirectTxSuccess = aTxStatus; }

        uint32_t GetIndirectFrameCounter(void) const { return mIndirectFrameCounter; }
        void     SetIndirectFrameCounter(uint32_t aFrameCounter) { mIndirectFrameCounter = aFrameCounter; }

        uint8_t GetIndirectKeyId(void) const { return mIndirectKeyId; }
        void    SetIndirectKeyId(uint8_t aKeyId) { mIndirectKeyId = aKeyId; }

        uint8_t GetIndirectTxAttempts(void) const { return mIndirectTxAttempts; }
        void    ResetIndirectTxAttempts(void) { mIndirectTxAttempts = 0; }
        void    IncrementIndirectTxAttempts(void) { mIndirectTxAttempts++; }

        uint8_t GetIndirectDataSequenceNumber(void) const { return mIndirectDsn; }
        void    SetIndirectDataSequenceNumber(uint8_t aDsn) { mIndirectDsn = aDsn; }

        bool IsIndirectSourceMatchShort(void) const { return mUseShortAddress; }
        void SetIndirectSourceMatchShort(bool aShort) { mUseShortAddress = aShort; }

        bool IsIndirectSourceMatchPending(void) const { return mSourceMatchPending; }
        void SetIndirectSourceMatchPending(bool aPending) { mSourceMatchPending = aPending; }

        void IncrementIndirectMessageCount(void) { mQueuedMessageCount++; }
        void DecrementIndirectMessageCount(void) { mQueuedMessageCount--; }
        void ResetIndirectMessageCount(void) { mQueuedMessageCount = 0; }

        const Mac::Address &GetMacAddress(Mac::Address &aMacAddress) const;

        Message *mIndirectMessage;             // Current indirect message.
        uint16_t mIndirectFragmentOffset : 15; // 6LoWPAN fragment offset for the indirect message.
        bool     mIndirectTxSuccess : 1;       // Indicates tx success/failure of current indirect message.
        uint16_t mQueuedMessageCount : 13;     // Number of queued indirect messages for the child.
        bool     mUseShortAddress : 1;         // Indicates whether to use short or extended address.
        bool     mSourceMatchPending : 1;      // Indicates whether or not pending to add to src match table.
        bool     mDataRequestPendig : 1;       // Indicates whether or not a Data Poll was received,
        uint32_t mIndirectFrameCounter;        // Frame counter for current indirect message (used fore retx).
        uint8_t  mIndirectKeyId;               // Key Id for current indirect message (used for retx).
        uint8_t  mIndirectTxAttempts;          // Number of data poll triggered tx attempts.
        uint8_t  mIndirectDsn;                 // MAC level Data Sequence Number (DSN) for retx attempts.

        OT_STATIC_ASSERT(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < 8192, "mQueuedMessageCount cannot fit max required!");
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit IndirectSender(Instance &aInstance);

    /**
     * This method enables indirect transmissions.
     *
     */
    void Start(void) { mEnabled = true; }

    /**
     * This method disables indirect transmission.
     *
     * Any previously scheduled indirect transmission is canceled.
     *
     */
    void Stop(void);

    /**
     * This method adds a message for indirect transmission to a sleepy child.
     *
     * @param[in] aMessage  The message to add.
     * @param[in] aChild    The (sleepy) child for indirect transmission.
     *
     * @retval OT_ERROR_NONE           Successfully added the message for indirect transmission.
     * @retval OT_ERROR_ALREADY        The message was already added for indirect transmission to same child.
     * @retval OT_ERROR_INVALID_STATE  The child is not sleepy.
     *
     */
    otError AddMessageForSleepyChild(Message &aMessage, Child &aChild);

    /**
     * This method removes a message for indirect transmission to a sleepy child.
     *
     * @param[in] aMessage  The message to update.
     * @param[in] aChild    The (sleepy) child for indirect transmission.
     *
     * @retval OT_ERROR_NONE           Successfully removed the message for indirect transmission.
     * @retval OT_ERROR_NOT_FOUND      The message was not scheduled for indirect transmission to the child.
     *
     */
    otError RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild);

    /**
     * This method removes all added messages for a specific child and frees message (with no indirect/direct tx).
     *
     * @param[in]  aChild  A reference to a child whose messages shall be removed.
     *
     */
    void ClearAllMessagesForSleepyChild(Child &aChild);

    void    HandleDataPoll(const Mac::Frame &aFrame, const Mac::Address &aMacSource, const otThreadLinkInfo &aLinkInfo);
    otError GetIndirectTransmission(void);
    void    HandleSentFrameToChild(const Mac::Frame &aFrame, otError aError, const Mac::Address &aMacDest);

    // Callbacks from MAC layer
    void    HandleSentFrame(Mac::Frame &aFrame, otError aError);
    otError HandleFrameRequest(Mac::Frame &aFrame);

private:
    enum
    {
        /**
         * Maximum number of tx attempts by `MeshForwarder` for an outbound indirect frame (for a sleepy child). The
         * `MeshForwader` attempts occur following the reception of a new data request command (a new data poll) from
         * the sleepy child.
         *
         */
        kMaxPollTriggeredTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS,
    };

    Message *GetIndirectTransmission(Child &aChild);
    void     PrepareIndirectTransmission(Message &aMessage, const Child &aChild);
    void     UpdateIndirectMessages(void);

    bool mEnabled;

    SourceMatchController mSourceMatchController;
    Child *               mIndirectStartingChild;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // INDIRECT_SENDER_HPP_
