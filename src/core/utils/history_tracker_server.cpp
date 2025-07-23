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

template <> void Server::HandleTmf<kUriHistoryQuery>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aMessage.IsPostRequest());

    LogInfo("Received %s from %s", UriToString<kUriHistoryQuery>(), aMessageInfo.GetPeerAddr().ToString().AsCString());

    if (aMessage.IsConfirmable())
    {
        IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));
    }

    PrepareAndSendAnswers(aMessageInfo.GetPeerAddr(), aMessage);

exit:
    return;
}

Error Server::AllocateAnswer(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    // Allocates an `Answer` message, adds it to `mAnswerQueue`,
    // updates the `aInfo.mFirstAnswer` if it is the first allocated
    // messages, and appends `QueryIdTlv` to the message (if needed).

    Error error = kErrorNone;

    aAnswer = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriHistoryAnswer);
    VerifyOrExit(aAnswer != nullptr, error = kErrorNoBufs);
    IgnoreError(aAnswer->SetPriority(aInfo.mPriority));

    mAnswerQueue.Enqueue(*aAnswer);

    if (aInfo.mFirstAnswer == nullptr)
    {
        aInfo.mFirstAnswer = aAnswer;
    }

    if (aInfo.mHasQueryId)
    {
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*aAnswer, aInfo.mQueryId));
    }

exit:
    return error;
}

bool Server::IsLastAnswer(const Coap::Message &aAnswer) const
{
    // Indicates whether `aAnswer` is the last one associated with
    // the same query.

    bool      isLast = true;
    AnswerTlv answerTlv;

    // If there is no Answer TLV, we assume it is the last answer.

    SuccessOrExit(Tlv::FindTlv(aAnswer, answerTlv));
    isLast = answerTlv.IsLast();

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

void Server::PrepareAndSendAnswers(const Ip6::Address &aDestination, const Message &aRequest)
{
    Coap::Message *answer;
    Error          error;
    AnswerInfo     info;
    OffsetRange    offsetRange;
    Tlv            tlv;
    RequestTlv     requestTlv;
    AnswerTlv      answerTlv;

    if (Tlv::Find<QueryIdTlv>(aRequest, info.mQueryId) == kErrorNone)
    {
        info.mHasQueryId = true;
    }

    info.mPriority = aRequest.GetPriority();

    SuccessOrExit(error = AllocateAnswer(answer, info));

    offsetRange.InitFromMessageOffsetToEnd(aRequest);

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(tlv.GetSize()))
    {
        SuccessOrExit(error = aRequest.Read(offsetRange, tlv));
        VerifyOrExit(offsetRange.Contains(tlv.GetSize()), error = kErrorParse);

        if (tlv.GetType() == Tlv::kRequest)
        {
            SuccessOrExit(error = aRequest.Read(offsetRange, requestTlv));
            VerifyOrExit(requestTlv.IsValid(), error = kErrorParse);

            switch (requestTlv.GetTlvType())
            {
            case Tlv::kNetworkInfo:
                SuccessOrExit(error = AppendNetworkInfo(answer, info, requestTlv));
                break;

            default:
                break;
            }

            SuccessOrExit(error = CheckAnswerLength(answer, info));
        }
    }

    answerTlv.Init(info.mAnswerIndex, /* aIsLast */ true);
    SuccessOrExit(error = answer->Append(answerTlv));

    SendNextAnswer(*info.mFirstAnswer, aDestination);

exit:
    if ((error != kErrorNone) && (info.mFirstAnswer != nullptr))
    {
        FreeAllRelatedAnswers(*info.mFirstAnswer);
    }
}

Error Server::CheckAnswerLength(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    // Checks the length of the `aAnswer` message and if it is above
    // the threshold, it enqueues the message for transmission after
    // appending an Answer TLV with the current index to the message.
    // In this case, it will also allocate a new answer message.

    Error     error = kErrorNone;
    AnswerTlv answerTlv;

    VerifyOrExit(aAnswer->GetLength() >= kAnswerMessageLengthThreshold);

    answerTlv.Init(aInfo.mAnswerIndex++, /* aIsLast */ false);
    SuccessOrExit(error = aAnswer->Append(answerTlv));

    error = AllocateAnswer(aAnswer, aInfo);

exit:
    return error;
}

void Server::SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination)
{
    Error            error      = kErrorNone;
    Coap::Message   *nextAnswer = IsLastAnswer(aAnswer) ? nullptr : aAnswer.GetNextCoapMessage();
    Tmf::MessageInfo messageInfo(GetInstance());

    mAnswerQueue.Dequeue(aAnswer);

    PrepareMessageInfoForDest(aDestination, messageInfo);

    // When sending the message, we pass `nextAnswer` as `aContext`
    // to be used when invoking callback `HandleAnswerResponse()`.

    error = Get<Tmf::Agent>().SendMessage(aAnswer, messageInfo, HandleAnswerResponse, nextAnswer);

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

void Server::PrepareMessageInfoForDest(const Ip6::Address &aDestination, Tmf::MessageInfo &aMessageInfo) const
{
    if (aDestination.IsMulticast())
    {
        aMessageInfo.SetMulticastLoop(true);
    }

    if (aDestination.IsLinkLocalUnicastOrMulticast())
    {
        aMessageInfo.SetSockAddr(Get<Mle::Mle>().GetLinkLocalAddress());
    }
    else
    {
        aMessageInfo.SetSockAddrToRloc();
    }

    aMessageInfo.SetPeerAddr(aDestination);
}

void Server::HandleAnswerResponse(void                *aContext,
                                  otMessage           *aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  otError              aResult)
{
    Coap::Message *nextAnswer = static_cast<Coap::Message *>(aContext);

    VerifyOrExit(nextAnswer != nullptr);

    nextAnswer->Get<Server>().HandleAnswerResponse(*nextAnswer, AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                   aResult);

exit:
    return;
}

void Server::HandleAnswerResponse(Coap::Message          &aNextAnswer,
                                  Coap::Message          *aResponse,
                                  const Ip6::MessageInfo *aMessageInfo,
                                  Error                   aResult)
{
    Error error = aResult;

    SuccessOrExit(error);
    VerifyOrExit(aResponse != nullptr && aMessageInfo != nullptr, error = kErrorDrop);
    VerifyOrExit(aResponse->GetCode() == Coap::kCodeChanged, error = kErrorDrop);

    SendNextAnswer(aNextAnswer, aMessageInfo->GetPeerAddr());

exit:
    if (error != kErrorNone)
    {
        FreeAllRelatedAnswers(aNextAnswer);
    }
}

Error Server::AppendNetworkInfo(Coap::Message *&aAnswer, AnswerInfo &aInfo, const RequestTlv &aRequestTlv)
{
    Error    error = kErrorNone;
    Iterator iterator;
    uint32_t maxEntryAge = aRequestTlv.GetMaxEntryAge();
    uint16_t maxCount    = aRequestTlv.GetNumEntries();

    iterator.Init(aInfo.mNow);

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

        SuccessOrExit(error = aAnswer->Append(networkInfoTlv));
        SuccessOrExit(error = CheckAnswerLength(aAnswer, aInfo));
    }

    SuccessOrExit(error = AppendEmptyTlv(*aAnswer, Tlv::kNetworkInfo));

exit:
    return error;
}

Error Server::AppendEmptyTlv(Coap::Message &aAnswer, Tlv::Type aTlvType)
{
    Tlv tlv;

    tlv.SetType(aTlvType);
    tlv.SetLength(0);

    return aAnswer.Append(tlv);
}

} // namespace HistoryTracker
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_SERVER_ENABLE
