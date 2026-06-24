/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for Network Diagnostics helper types and classes.
 */

#ifndef OT_CORE_THREAD_NET_DIAG_TYPES_HPP_
#define OT_CORE_THREAD_NET_DIAG_TYPES_HPP_

#include "openthread-core-config.h"

#include "net_diag_tlvs.hpp"
#include "coap/coap_message.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/time.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetDiag {

/**
 * Represents an Answer type (used in `AnswerBuilder`).
 */
enum AnswerType : uint8_t
{
    kAnswerTypeNetDiag, ///< A `NetDiag` answer.
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    kAnswerTypeHistoryTracker, ///< A `HistoryTracker` answer.
#endif
};

/**
 * Represents an object for tracking and managing `NetDiag` or `HistoryTracker` answer messages.
 *
 * This class is used when the response to a query requires multiple CoAP answer messages. It manages the inclusion of
 * the Query ID and the Answer TLV (providing message indexing and "more-to-follow" flags) in each allocated answer
 * message, while maintaining all answer messages in a queue.
 *
 * The `AnswerType` determines the specific TLV types and the CoAP URI used for the allocated answer messages, either
 * the Network Diagnostics TLVs or the History Tracker TLVs are used.
 */
class AnswerBuilder : public InstanceLocator
{
public:
    /**
     * Initializes the `AnswerBuilder`.
     *
     * @param[in] aInstance  The OpenThread instance.
     * @param[in] aType      The `AnswerType`.
     */
    AnswerBuilder(Instance &aInstance, AnswerType aType);

    /**
     * Destructor for `AnswerBuilder`.
     *
     * Any remaining messages in the answers queue are freed.
     */
    ~AnswerBuilder(void);

    /**
     * Starts the building of answer messages based on a given request.
     *
     * This method searches for a Query ID TLV within @p aRequest. If present, the Query ID is captured and
     * automatically included in all allocated answer messages. It also captures the priority from @p aRequest and
     * uses it for all answer messages.
     *
     * @param[in] aRequest  The request message.
     *
     * @retval kErrorNone   Successfully started.
     * @retval kErrorNoBufs Insufficient message buffers to allocate the first answer.
     */
    Error Start(const Coap::Message &aRequest);

    /**
     * Gets the current answer message.
     *
     * @returns The current answer message.
     */
    Coap::Message &GetAnswer(void) { return *mAnswer; }

    /**
     * Checks the current answer message length and allocates a new one if it exceeds the threshold.
     *
     * If the current answer message length is above the threshold, this method appends an Answer TLV with the
     * "more to follow" flag set, and then allocates a new answer message, adding it to the answer queue.
     *
     * @retval kErrorNone   Successfully checked and handled (possibly allocating a new message).
     * @retval kErrorNoBufs Insufficient message buffers to append Answer TLV or allocate a new message.
     */
    Error CheckAnswerLength(void);

    /**
     * Finishes the answer generation.
     *
     * This method appends an Answer TLV to the current answer message with the "is last" flag set.
     *
     * @retval kErrorNone   Successfully finished.
     * @retval kErrorNoBufs Insufficient message buffers to append the Answer TLV.
     */
    Error Finish(void);

    /**
     * Gets the queue containing all answer messages.
     *
     * @returns The message queue containing all answer messages.
     */
    Coap::MessageQueue &GetAnswers(void) { return mAnswers; }

private:
    static constexpr uint16_t kAnswerMessageLengthThreshold = 800;

    Error Allocate(void);
    Error AppendAnswerTlv(AnswerTlvValue::IsLastFlag aFlag);

    Coap::Message     *mAnswer;
    Coap::MessageQueue mAnswers;
    uint16_t           mAnswerIndex;
    uint16_t           mQueryId;
    AnswerType         mType;
    Message::Priority  mPriority;
    bool               mHasQueryId;
};

/**
 * Represents an `AnswerSender` for sending Network Diagnostics or History Tracker answer messages.
 *
 * This class takes ownership of the answer messages generated by an `AnswerBuilder` and manages sending them to a
 * destination address one by one. It handles confirmable CoAP POST messages and tracks the responses. If any message
 * fails to send or receives an error response, all remaining related answers are automatically freed.
 */
class AnswerSender : public InstanceLocator
{
public:
    /**
     * Initializes the `AnswerSender`.
     *
     * @param[in] aInstance  The OpenThread instance.
     * @param[in] aType      The `AnswerType` specifying either Network Diagnostics or History Tracker.
     */
    AnswerSender(Instance &aInstance, AnswerType aType);

    /**
     * Takes over all answer messages from @p aAnswerBuilder and starts sending them to @p aDestination.
     *
     * The `AnswerSender` enqueues all messages from the builder into its own queue. It then begins sending the first
     * answer message in the queue.
     *
     * @param[in] aAnswerBuilder  The `AnswerBuilder` containing the generated answer messages.
     * @param[in] aDestination    The destination IP address to send the answers to.
     */
    void Send(AnswerBuilder &aAnswerBuilder, const Ip6::Address &aDestination);

private:
    bool IsLast(const Coap::Message &aAnswer) const;
    void FreeAllRelated(Coap::Message &aAnswer);
    void SendNext(Coap::Message &aAnswer, const Ip6::Address &aDestination);
    void HandleResponse(Coap::Message &aNextAnswer, Coap::Msg *aResponse, Error aResult);

    template <AnswerType kType> static void          HandleResponse(void *aContext, Coap::Msg *aMsg, Error aResult);
    template <AnswerType kType> static AnswerSender &GetSender(Instance &aInstance);

    Coap::MessageQueue mQueue;
    AnswerType         mType;
};

// Declare template specializations.

#if OPENTHREAD_FTD
template <> AnswerSender &AnswerSender::GetSender<kAnswerTypeNetDiag>(Instance &aInstance);
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
template <> AnswerSender &AnswerSender::GetSender<kAnswerTypeHistoryTracker>(Instance &aInstance);
#endif

} // namespace NetDiag
} // namespace ot

#endif // OT_CORE_THREAD_NET_DIAG_TYPES_HPP_
