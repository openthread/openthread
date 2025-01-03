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
 *   This file includes definitions for handling of data polls and indirect frame transmission.
 */

#ifndef DATA_POLL_HANDLER_HPP_
#define DATA_POLL_HANDLER_HPP_

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "thread/indirect_sender_frame_context.hpp"

namespace ot {

/**
 * @addtogroup core-data-poll-handler
 *
 * @brief
 *   This module includes definitions for data poll handler.
 *
 * @{
 */

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#if OPENTHREAD_FTD
class Child;
#endif

/**
 * Implements the data poll (mac data request command) handler.
 */
class DataPollHandler : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;

public:
    static constexpr uint8_t kMaxPollTriggeredTxAttempts = OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS;

    /**
     * Defines frame change request types used as input to `RequestFrameChange()`.
     */
    enum FrameChange : uint8_t
    {
        kPurgeFrame,   ///< Indicates that previous frame should be purged. Any ongoing indirect tx should be aborted.
        kReplaceFrame, ///< Indicates that previous frame needs to be replaced with a new higher priority one.
    };

    /**
     * Defines all the neighbor info required for handling of data polls and indirect frame transmissions.
     *
     * `Child` (and `CslNeighbor`) class publicly inherits from this class.
     */
    class NeighborInfo
    {
        friend class DataPollHandler;
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        friend class CslTxScheduler;
#endif

    private:
        bool IsDataPollPending(void) const { return mDataPollPending; }
        void SetDataPollPending(bool aPending) { mDataPollPending = aPending; }

        uint32_t GetIndirectFrameCounter(void) const { return mIndirectFrameCounter; }
        void     SetIndirectFrameCounter(uint32_t aFrameCounter) { mIndirectFrameCounter = aFrameCounter; }

        uint8_t GetIndirectKeyId(void) const { return mIndirectKeyId; }
        void    SetIndirectKeyId(uint8_t aKeyId) { mIndirectKeyId = aKeyId; }

        uint8_t GetIndirectTxAttempts(void) const { return mIndirectTxAttempts; }
        void    ResetIndirectTxAttempts(void) { mIndirectTxAttempts = 0; }
        void    IncrementIndirectTxAttempts(void) { mIndirectTxAttempts++; }

        uint8_t GetIndirectDataSequenceNumber(void) const { return mIndirectDsn; }
        void    SetIndirectDataSequenceNumber(uint8_t aDsn) { mIndirectDsn = aDsn; }

        bool IsFramePurgePending(void) const { return mFramePurgePending; }
        void SetFramePurgePending(bool aPurgePending) { mFramePurgePending = aPurgePending; }

        bool IsFrameReplacePending(void) const { return mFrameReplacePending; }
        void SetFrameReplacePending(bool aReplacePending) { mFrameReplacePending = aReplacePending; }

#if OPENTHREAD_CONFIG_MULTI_RADIO
        Mac::RadioType GetLastPollRadioType(void) const { return mLastPollRadioType; }
        void           SetLastPollRadioType(Mac::RadioType aRadioType) { mLastPollRadioType = aRadioType; }
#endif

        uint32_t mIndirectFrameCounter;    // Frame counter for current indirect frame (used for retx).
        uint8_t  mIndirectKeyId;           // Key Id for current indirect frame (used for retx).
        uint8_t  mIndirectDsn;             // MAC level Data Sequence Number (DSN) for retx attempts.
        uint8_t  mIndirectTxAttempts : 5;  // Number of data poll triggered tx attempts.
        bool     mDataPollPending : 1;     // Indicates whether or not a Data Poll was received.
        bool     mFramePurgePending : 1;   // Indicates a pending purge request for the current indirect frame.
        bool     mFrameReplacePending : 1; // Indicates a pending replace request for the current indirect frame.
#if OPENTHREAD_CONFIG_MULTI_RADIO
        Mac::RadioType mLastPollRadioType; // The radio link last data poll frame was received on.
#endif

        static_assert(kMaxPollTriggeredTxAttempts < (1 << 5), "mIndirectTxAttempts cannot fit max!");
    };

#if OPENTHREAD_FTD

    /**
     * Initializes the data poll handler object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     */
    explicit DataPollHandler(Instance &aInstance);

    /**
     * Clears any state/info saved per child for indirect frame transmission.
     */
    void Clear(void);

    /**
     * Requests a frame change for a given child.
     *
     * Two types of frame change requests are supported:
     *
     * 1) "Purge Frame" which indicates that the previous frame should be purged and any ongoing indirect tx aborted.
     * 2) "Replace Frame" which indicates that the previous frame needs to be replaced with a new higher priority one.
     *
     * If there is no ongoing indirect frame transmission to the child, the request will be handled immediately and the
     * callback `HandleFrameChangeDone()` is called directly from this method itself. This callback notifies the next
     * layer (`IndirectSender`) that the indirect frame/message for the child can be safely updated.
     *
     * If there is an ongoing indirect frame transmission to this child, the request can not be handled immediately.
     * The following options can happen based on the request type:
     *
     * 1) In case of "purge" request, the ongoing indirect transmission is aborted and upon completion of the abort the
     *    callback `HandleFrameChangeDone()` is invoked.
     *
     * 2) In case of "replace" request, the ongoing indirect transmission is allowed to finish (current tx attempt).
     *    2.a) If the tx attempt is successful, the `IndirectSender::HandleSentFrameToChild()` in invoked which
     *         indicates the "replace" could not happen (in this case `HandleFrameChangeDone()` is no longer called).
     *    2.b) If the ongoing tx attempt is unsuccessful, then callback `HandleFrameChangeDone()` is invoked to allow
     *         the next layer to update the frame/message for the child.
     *
     * If there is a pending request, a subsequent call to this method is ignored except for the case where pending
     * request is for "replace frame" and new one is for "purge frame" where the "purge" overrides the "replace"
     * request.
     *
     * @param[in]  aChange    The frame change type.
     * @param[in]  aChild     The child to process its frame change.
     */
    void RequestFrameChange(FrameChange aChange, Child &aChild);

private:
    typedef IndirectSenderBase::FrameContext FrameContext;

    // Callbacks from MAC
    void          HandleDataPoll(Mac::RxFrame &aFrame);
    Mac::TxFrame *HandleFrameRequest(Mac::TxFrames &aTxFrames);
    void          HandleSentFrame(const Mac::TxFrame &aFrame, Error aError);

    void HandleSentFrame(const Mac::TxFrame &aFrame, Error aError, Child &aChild);
    void ProcessPendingPolls(void);
    void ResetTxAttempts(Child &aChild);

    Child       *mIndirectTxChild; // The child being handled (`nullptr` indicates no active indirect tx).
    FrameContext mFrameContext;    // Context for the prepared frame for the current indirect tx (if any)

#endif // OPENTHREAD_FTD
};

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

/**
 * @}
 */

} // namespace ot

#endif // DATA_POLL_HANDLER_HPP_
