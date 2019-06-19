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
