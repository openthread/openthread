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
 *   This file implements the History Tracker Server.
 */

#include "history_tracker_server.hpp"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace HistoryTracker {

RegisterLogModule("HistoryServer");

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

template <> void Server::HandleTmf<kUriHistoryQuery>(Coap::Msg &aMsg)
{
    LogInfo("Received %s from %s", UriToString<kUriHistoryQuery>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    if (aMsg.IsConfirmable())
    {
        IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg));
    }

    PrepareAndSendAnswers(aMsg.mMessageInfo.GetPeerAddr(), aMsg.mMessage);
}

bool Server::IsLastAnswer(const Coap::Message &aAnswer) const
{
    // Indicates whether `aAnswer` is the last one associated with
    // the same query.

    bool           isLast = true;
    AnswerTlvValue answerTlvValue;

    // If there is no Answer TLV, we assume it is the last answer.

    SuccessOrExit(Tlv::Find<AnswerTlv>(aAnswer, answerTlvValue));
    isLast = answerTlvValue.IsLast();

exit:
    return isLast;
}

void Server::FreeAllRelatedAnswers(Coap::Message &aFirstAnswer)
{
    // Dequeues and frees all answer messages related to the same query
    // as `aFirstAnswer`. Note that related answers are enqueued in
    // order.

    Coap::Message *answer = &aFirstAnswer;

    while (answer != nullptr)
    {
        Coap::Message *next = IsLastAnswer(*answer) ? nullptr : answer->GetNextCoapMessage();

        mAnswerQueue.DequeueAndFree(*answer);
        answer = next;
    }
}

void Server::PrepareAndSendAnswers(const Ip6::Address &aDestination, const Coap::Message &aRequest)
{
    Error          error;
    AnswerBuilder  answerBuilder(GetInstance(), NetDiag::kAnswerTypeHistoryTracker);
    OffsetRange    offsetRange;
    Tlv::Info      tlvInfo;
    RequestTlv     requestTlv;
    TimeMilli      now = TimerMilli::GetNow();
    Coap::Message *firstAnswer;

    SuccessOrExit(error = answerBuilder.Start(aRequest));

    offsetRange.InitFromMessageOffsetToEnd(aRequest);

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(tlvInfo.GetSize()))
    {
        SuccessOrExit(error = tlvInfo.ParseFrom(aRequest, offsetRange));

        if (tlvInfo.IsExtended())
        {
            continue;
        }

        if (tlvInfo.GetType() == Tlv::kRequest)
        {
            SuccessOrExit(error = aRequest.Read(offsetRange, requestTlv));
            VerifyOrExit(requestTlv.IsValid(), error = kErrorParse);

            switch (requestTlv.GetTlvType())
            {
            case Tlv::kNetworkInfo:
                SuccessOrExit(error = AppendNetworkInfo(answerBuilder, requestTlv, now));
                break;

            default:
                break;
            }

            SuccessOrExit(error = answerBuilder.CheckAnswerLength());
        }
    }

    SuccessOrExit(error = answerBuilder.Finish());

    firstAnswer = answerBuilder.GetAnswers().GetHead();
    mAnswerQueue.EnqueueAllFrom(answerBuilder.GetAnswers());

    SendNextAnswer(*firstAnswer, aDestination);

exit:
    return;
}

void Server::SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination)
{
    Error          error      = kErrorNone;
    Coap::Message *nextAnswer = IsLastAnswer(aAnswer) ? nullptr : aAnswer.GetNextCoapMessage();

    mAnswerQueue.Dequeue(aAnswer);

    // When sending the message, we pass `nextAnswer` as `aContext`
    // to be used when invoking callback `HandleAnswerResponse()`.

    error = Get<Tmf::Agent>().SendMessageAllowMulticastLoop(aAnswer, aDestination, HandleAnswerResponse, nextAnswer);

    if (error != kErrorNone)
    {
        // If the `SendMessage()` fails, we `Free` the dequeued
        // `aAnswer` and all the related next answers in the queue.

        aAnswer.Free();

        if (nextAnswer != nullptr)
        {
            FreeAllRelatedAnswers(*nextAnswer);
        }
    }
}

void Server::HandleAnswerResponse(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    Coap::Message *nextAnswer = static_cast<Coap::Message *>(aContext);

    VerifyOrExit(nextAnswer != nullptr);

    nextAnswer->Get<Server>().HandleAnswerResponse(*nextAnswer, aMsg, aResult);

exit:
    return;
}

void Server::HandleAnswerResponse(Coap::Message &aNextAnswer, Coap::Msg *aResponse, Error aResult)
{
    Error error = aResult;

    SuccessOrExit(error);
    VerifyOrExit(aResponse != nullptr, error = kErrorDrop);
    VerifyOrExit(aResponse->GetCode() == Coap::kCodeChanged, error = kErrorDrop);

    SendNextAnswer(aNextAnswer, aResponse->mMessageInfo.GetPeerAddr());

exit:
    if (error != kErrorNone)
    {
        FreeAllRelatedAnswers(aNextAnswer);
    }
}

Error Server::AppendNetworkInfo(AnswerBuilder &aAnswerBuilder, const RequestTlv &aRequestTlv, TimeMilli aNow)
{
    Error    error = kErrorNone;
    Iterator iterator;
    uint32_t maxEntryAge = aRequestTlv.GetMaxEntryAge();
    uint16_t maxCount    = aRequestTlv.GetNumEntries();

    iterator.Init(aNow);

    for (uint16_t count = 0; (maxCount == 0) || (count < maxCount); count++)
    {
        const NetworkInfo *networkInfo;
        uint32_t           entryAge;
        NetworkInfoTlv     networkInfoTlv;

        networkInfo = Get<Local>().IterateNetInfoHistory(iterator, entryAge);

        if (networkInfo == nullptr)
        {
            break;
        }

        if ((maxEntryAge != 0) && (entryAge > maxEntryAge))
        {
            break;
        }

        networkInfoTlv.InitFrom(*networkInfo, entryAge);

        SuccessOrExit(error = aAnswerBuilder.GetAnswer().Append(networkInfoTlv));
        SuccessOrExit(error = aAnswerBuilder.CheckAnswerLength());
    }

    SuccessOrExit(error = Tlv::AppendEmpty<NetworkInfoTlv>(aAnswerBuilder.GetAnswer()));

exit:
    return error;
}

} // namespace HistoryTracker
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
