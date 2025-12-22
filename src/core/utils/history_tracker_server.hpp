/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions to support History Tracker Server (TMF).
 */

#ifndef OT_CORE_UTILS_HISTORY_TRACKER_SERVER_HPP_
#define OT_CORE_UTILS_HISTORY_TRACKER_SERVER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE

#include <openthread/history_tracker.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "thread/tmf.hpp"
#include "utils/history_tracker.hpp"
#include "utils/history_tracker_tlvs.hpp"

namespace ot {
namespace HistoryTracker {

/**
 * Represents History Tracker Server.
 */
class Server : public InstanceLocator
{
    friend class Tmf::Agent;

public:
    explicit Server(Instance &aInstance);

private:
    static constexpr uint16_t kAnswerMessageLengthThreshold = 800;

    struct AnswerInfo
    {
        AnswerInfo(void)
            : mNow(TimerMilli::GetNow())
            , mAnswerIndex(0)
            , mQueryId(0)
            , mHasQueryId(false)
            , mFirstAnswer(nullptr)
        {
        }

        TimeMilli         mNow;
        uint16_t          mAnswerIndex;
        uint16_t          mQueryId;
        bool              mHasQueryId;
        Message::Priority mPriority;
        Coap::Message    *mFirstAnswer;
    };

    Error AllocateAnswer(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    bool  IsLastAnswer(const Coap::Message &aAnswer) const;
    void  FreeAllRelatedAnswers(Coap::Message &aFirstAnswer);
    void  PrepareAndSendAnswers(const Ip6::Address &aDestination, const Message &aRequest);
    Error CheckAnswerLength(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    void  SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination);
    void  PrepareMessageInfoForDest(const Ip6::Address &aDestination, Tmf::MessageInfo &aMessageInfo) const;
    Error AppendNetworkInfo(Coap::Message *&aAnswer, AnswerInfo &aInfo, const RequestTlv &aRequestTlv);
    Error AppendEmptyTlv(Coap::Message &aAnswer, Tlv::Type aTlvType);

    static void HandleAnswerResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     otError              aResult);
    void        HandleAnswerResponse(Coap::Message          &aNextAnswer,
                                     Coap::Message          *aResponse,
                                     const Ip6::MessageInfo *aMessageInfo,
                                     Error                   aResult);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Coap::MessageQueue mAnswerQueue;
};

DeclareTmfHandler(Server, kUriHistoryQuery);

} // namespace HistoryTracker
} // namespace ot

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE

#endif // OT_CORE_UTILS_HISTORY_TRACKER_SERVER_HPP_
