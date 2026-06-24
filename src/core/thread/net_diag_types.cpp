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
 *   This file implements methods for Network Diagnostics helper types and classes.
 */

#include "net_diag_types.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace NetDiag {

//---------------------------------------------------------------------------------------------------------------------
// AnswerBuilder

AnswerBuilder::AnswerBuilder(Instance &aInstance, AnswerType aType)
    : InstanceLocator(aInstance)
    , mAnswer(nullptr)
    , mAnswerIndex(0)
    , mQueryId(0)
    , mType(aType)
    , mPriority(Message::kPriorityNormal)
    , mHasQueryId(false)
{
}

AnswerBuilder::~AnswerBuilder(void) { mAnswers.DequeueAndFreeAll(); }

Error AnswerBuilder::Start(const Coap::Message &aRequest)
{
    mAnswerIndex = 0;
    mPriority    = aRequest.GetPriority();
    mHasQueryId  = false;

    switch (mType)
    {
    case kAnswerTypeNetDiag:
        SuccessOrExit(Tlv::Find<QueryIdTlv>(aRequest, mQueryId));
        break;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    case kAnswerTypeHistoryTracker:
        SuccessOrExit(Tlv::Find<HistoryTracker::QueryIdTlv>(aRequest, mQueryId));
        break;
#endif
    }

    mHasQueryId = true;

exit:
    return Allocate();
}

Error AnswerBuilder::Finish(void) { return AppendAnswerTlv(AnswerTlvValue::kIsLast); }

Error AnswerBuilder::Allocate(void)
{
    Error error = kErrorNone;
    Uri   uri   = kUriDiagnosticGetAnswer;

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    if (mType == kAnswerTypeHistoryTracker)
    {
        uri = kUriHistoryAnswer;
    }
#endif

    mAnswer = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(uri);
    VerifyOrExit(mAnswer != nullptr, error = kErrorNoBufs);

    IgnoreError(mAnswer->SetPriority(mPriority));

    mAnswers.Enqueue(*mAnswer);

    VerifyOrExit(mHasQueryId);

    switch (mType)
    {
    case kAnswerTypeNetDiag:
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*mAnswer, mQueryId));
        break;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    case kAnswerTypeHistoryTracker:
        SuccessOrExit(error = Tlv::Append<HistoryTracker::QueryIdTlv>(*mAnswer, mQueryId));
        break;
#endif
    }

exit:
    return error;
}

Error AnswerBuilder::AppendAnswerTlv(AnswerTlvValue::IsLastFlag aFlag)
{
    Error          error = kErrorNone;
    AnswerTlvValue value;

    value.Init(mAnswerIndex++, aFlag);

    switch (mType)
    {
    case kAnswerTypeNetDiag:
        error = Tlv::Append<AnswerTlv>(*mAnswer, value);
        break;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    case kAnswerTypeHistoryTracker:
        error = Tlv::Append<HistoryTracker::AnswerTlv>(*mAnswer, value);
        break;
#endif
    }

    return error;
}

Error AnswerBuilder::CheckAnswerLength(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mAnswer->GetLength() >= kAnswerMessageLengthThreshold);

    SuccessOrExit(error = AppendAnswerTlv(AnswerTlvValue::kMoreToFollow));

    error = Allocate();

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// AnswerSender

AnswerSender::AnswerSender(Instance &aInstance, AnswerType aType)
    : InstanceLocator(aInstance)
    , mType(aType)
{
}

void AnswerSender::Send(AnswerBuilder &aAnswerBuilder, const Ip6::Address &aDestination)
{
    Coap::Message *firstAnswer = aAnswerBuilder.GetAnswers().GetHead();

    VerifyOrExit(firstAnswer != nullptr);

    mQueue.EnqueueAllFrom(aAnswerBuilder.GetAnswers());
    SendNext(*firstAnswer, aDestination);

exit:
    return;
}

bool AnswerSender::IsLast(const Coap::Message &aAnswer) const
{
    // Indicates whether `aAnswer` is the last one associated with
    // the same query.

    bool           isLast = true;
    Error          error  = kErrorNotFound;
    AnswerTlvValue answerTlvValue;

    switch (mType)
    {
    case kAnswerTypeNetDiag:
        error = Tlv::Find<AnswerTlv>(aAnswer, answerTlvValue);
        break;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    case kAnswerTypeHistoryTracker:
        error = Tlv::Find<HistoryTracker::AnswerTlv>(aAnswer, answerTlvValue);
        break;
#endif
    }

    // If there is no Answer TLV, we assume it is the last answer.

    SuccessOrExit(error);

    isLast = answerTlvValue.IsLast();

exit:
    return isLast;
}

void AnswerSender::FreeAllRelated(Coap::Message &aAnswer)
{
    // This method dequeues and frees all answer messages related to
    // same query as `aAnswer`. Note that related answers are always
    // enqueued in order.

    Coap::Message *answer = &aAnswer;

    while (answer != nullptr)
    {
        Coap::Message *next = IsLast(*answer) ? nullptr : answer->GetNextCoapMessage();

        mQueue.DequeueAndFree(*answer);
        answer = next;
    }
}

void AnswerSender::SendNext(Coap::Message &aAnswer, const Ip6::Address &aDestination)
{
    // This method sends the given next `aAnswer` associated with
    // a query to the  `aDestination`.

    Error                 error           = kErrorFailed;
    Coap::Message        *nextAnswer      = IsLast(aAnswer) ? nullptr : aAnswer.GetNextCoapMessage();
    Coap::ResponseHandler responseHandler = nullptr;

    mQueue.Dequeue(aAnswer);

    switch (mType)
    {
    case kAnswerTypeNetDiag:
#if OPENTHREAD_FTD
        responseHandler = AnswerSender::HandleResponse<kAnswerTypeNetDiag>;
#endif
        break;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
    case kAnswerTypeHistoryTracker:
        responseHandler = AnswerSender::HandleResponse<kAnswerTypeHistoryTracker>;
        break;
#endif
    }

    VerifyOrExit(responseHandler != nullptr);

    // When sending the message, we pass `nextAnswer` as `aContext`
    // to be used when invoking callback `responseHandler`

    error = Get<Tmf::Agent>().SendMessageAllowMulticastLoop(aAnswer, aDestination, responseHandler, nextAnswer);

exit:
    if (error != kErrorNone)
    {
        // If the `SendMessage()` fails, we `Free` the dequeued
        // `aAnswer` and all the related next answers in the queue.

        aAnswer.Free();

        if (nextAnswer != nullptr)
        {
            FreeAllRelated(*nextAnswer);
        }
    }
}

#if OPENTHREAD_FTD
template <> AnswerSender &AnswerSender::GetSender<kAnswerTypeNetDiag>(Instance &aInstance)
{
    return aInstance.Get<Server>().mAnswerSender;
}
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
template <> AnswerSender &AnswerSender::GetSender<kAnswerTypeHistoryTracker>(Instance &aInstance)
{
    return aInstance.Get<HistoryTracker::Server>().mAnswerSender;
}
#endif

template <AnswerType kType> void AnswerSender::HandleResponse(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    Coap::Message *nextAnswer = static_cast<Coap::Message *>(aContext);

    VerifyOrExit(nextAnswer != nullptr);
    GetSender<kType>(nextAnswer->GetInstance()).HandleResponse(*nextAnswer, aMsg, aResult);

exit:
    return;
}

void AnswerSender::HandleResponse(Coap::Message &aNextAnswer, Coap::Msg *aResponse, Error aResult)
{
    Error error = aResult;

    SuccessOrExit(error);
    VerifyOrExit(aResponse != nullptr, error = kErrorDrop);
    VerifyOrExit(aResponse->GetCode() == Coap::kCodeChanged, error = kErrorDrop);

    SendNext(aNextAnswer, aResponse->mMessageInfo.GetPeerAddr());

exit:
    if (error != kErrorNone)
    {
        FreeAllRelated(aNextAnswer);
    }
}

} // namespace NetDiag
} // namespace ot
