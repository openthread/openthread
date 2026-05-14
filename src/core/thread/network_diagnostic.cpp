/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements Thread's Network Diagnostic processing.
 */

#include "network_diagnostic.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("NetDiag");

namespace NetworkDiagnostic {

//---------------------------------------------------------------------------------------------------------------------
// Server

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNonPreferredChannels(0)
{
}

Error Server::AppendIp6AddressList(Message &aMessage)
{
    Error         error;
    Tlv::Bookmark tlvBookmark;

    SuccessOrExit(error = Tlv::StartTlv(aMessage, Tlv::kIp6AddressList, tlvBookmark));

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        SuccessOrExit(error = aMessage.Append(addr.GetAddress()));
        SuccessOrExit(error = Tlv::AdjustTlv(aMessage, tlvBookmark));
    }

    error = Tlv::EndTlv(aMessage, tlvBookmark);

exit:
    return error;
}

#if OPENTHREAD_FTD
Error Server::AppendChildTable(Message &aMessage)
{
    Error         error = kErrorNone;
    uint16_t      count = 0;
    Tlv::Bookmark tlvBookmark;

    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());

    SuccessOrExit(error = Tlv::StartTlv(aMessage, Tlv::kChildTable, tlvBookmark));

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        ChildTableTlvEntry entry;

        if (++count > kMaxChildEntries)
        {
            break;
        }

        entry.InitFrom(child);

        SuccessOrExit(error = aMessage.Append(entry));
        SuccessOrExit(error = Tlv::AdjustTlv(aMessage, tlvBookmark));
    }

    error = Tlv::EndTlv(aMessage, tlvBookmark);

exit:
    return error;
}

Error Server::AppendEnhancedRoute(Message &aMessage)
{
    Error                 error = kErrorNone;
    Tlv::Bookmark         tlvBookmark;
    Mle::RouterIdMask     routerIdMask;
    EnhancedRouteTlvEntry entry;

    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());

    Get<RouterTable>().GetRouterIdMask(routerIdMask);

    SuccessOrExit(error = Tlv::StartTlv(aMessage, Tlv::kEnhancedRoute, tlvBookmark));

    SuccessOrExit(error = routerIdMask.AppendMaskTo(aMessage));

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (!routerIdMask.IsAllocated(routerId))
        {
            continue;
        }

        if (Get<Mle::Mle>().MatchesRouterId(routerId))
        {
            entry.InitAsSelf();
        }
        else
        {
            entry.InitFrom(*Get<RouterTable>().FindRouterById(routerId));
        }

        SuccessOrExit(error = aMessage.Append(entry));
    }

    error = Tlv::EndTlv(aMessage, tlvBookmark);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
Error Server::AppendChildTableAsChildTlvs(Message &aMessage)
{
    Error         error = kErrorNone;
    ChildTlvValue childTlvValue;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        childTlvValue.InitFrom(child);

        SuccessOrExit(error = Tlv::Append<ChildTlv>(aMessage, childTlvValue));
    }

    error = Tlv::AppendEmpty<ChildTlv>(aMessage);

exit:
    return error;
}

Error Server::AppendRouterNeighborTlvs(Message &aMessage)
{
    Error                  error = kErrorNone;
    RouterNeighborTlvValue neighborTlvValue;

    for (Router &router : Get<RouterTable>())
    {
        if (router.IsStateValid())
        {
            neighborTlvValue.InitFrom(router);
            SuccessOrExit(error = Tlv::Append<RouterNeighborTlv>(aMessage, neighborTlvValue));
        }
    }

    error = Tlv::AppendEmpty<RouterNeighborTlv>(aMessage);

exit:
    return error;
}

Error Server::AppendChildTableIp6AddressList(Message &aMessage)
{
    Error error = kErrorNone;

    for (const Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        SuccessOrExit(error = AppendChildIp6AddressListTlv(aMessage, child));
    }

    error = Tlv::AppendEmpty<ChildIp6AddressListTlv>(aMessage);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

Error Server::AppendBorderRouterIfAddrs(Message &aMessage)
{
    Error                             error;
    Tlv::Bookmark                     tlvBookmark;
    BorderRouter::PrefixTableIterator iterator;
    BorderRouter::IfAddrEntry         ifAddr;

    SuccessOrExit(error = Tlv::StartTlv(aMessage, Tlv::kBrIfAddrs, tlvBookmark));

    Get<BorderRouter::RxRaTracker>().InitIterator(iterator);

    while (Get<BorderRouter::RxRaTracker>().GetNextIfAddrEntry(iterator, ifAddr) == kErrorNone)
    {
        SuccessOrExit(error = aMessage.Append(ifAddr.mAddress));
        SuccessOrExit(error = Tlv::AdjustTlv(aMessage, tlvBookmark));
    }

    error = Tlv::EndTlv(aMessage, tlvBookmark);

exit:
    return error;
}

Error Server::AppendBrPrefixTlv(uint8_t aTlvType, Message &aMessage)
{
    Ip6::Prefix        prefix;
    Ip6::NetworkPrefix netPrefix;

    netPrefix.Clear();

    switch (aTlvType)
    {
    case Tlv::kBrLocalOmrPrefix:
        SuccessOrExit(Get<BorderRouter::RoutingManager>().GetOmrPrefix(prefix));
        break;
    case Tlv::kBrLocalOnlinkPrefix:
        SuccessOrExit(Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(prefix));
        break;
    case Tlv::kBrFavoredOnLinkPrefix:
        SuccessOrExit(Get<BorderRouter::RoutingManager>().GetFavoredOnLinkPrefix(prefix));
        break;
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    case Tlv::kBrDhcp6PdOmrPrefix:
    {
        BorderRouter::Dhcp6PdPrefix pdPrefix;

        SuccessOrExit(Get<BorderRouter::RoutingManager>().GetDhcp6PdOmrPrefix(pdPrefix));
        prefix = AsCoreType(&pdPrefix.mPrefix);
        break;
    }
#endif
    default:
        ExitNow();
    }

    IgnoreError(netPrefix.SetFrom(prefix));

exit:
    return Tlv::AppendTlv(aMessage, aTlvType, &netPrefix, sizeof(netPrefix));
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

Error Server::AppendRequestedTlvs(const Message &aRequest, Message &aResponse)
{
    Error               error;
    TlvTypeListIterator iterator;
    uint8_t             tlvType;

    SuccessOrExit(error = iterator.InitForTypeListTlv(aRequest));

    while (iterator.ReadNextTlvType(tlvType) == kErrorNone)
    {
        SuccessOrExit(error = AppendDiagTlv(tlvType, aResponse));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
Error Server::AppendRequestedTlvsForTcat(const Message &aRequest, Message &aResponse, OffsetRange &aOffsetRange)
{
    Error               error = kErrorNone;
    TlvTypeListIterator iterator;
    uint8_t             tlvType;

    iterator.Init(aRequest, aOffsetRange);

    while (iterator.ReadNextTlvType(tlvType) == kErrorNone)
    {
#if OPENTHREAD_FTD
        switch (tlvType)
        {
        case ChildTlv::kType:
            SuccessOrExit(error = AppendChildTableAsChildTlvs(aResponse));
            break;

        case ChildIp6AddressListTlv::kType:
            SuccessOrExit(error = AppendChildTableIp6AddressList(aResponse));
            break;

        case RouterNeighborTlv::kType:
            SuccessOrExit(error = AppendRouterNeighborTlvs(aResponse));
            break;

        default:
            SuccessOrExit(error = AppendDiagTlv(tlvType, aResponse));
            break;
        }

#elif OPENTHREAD_MTD
        SuccessOrExit(error = AppendDiagTlv(tlvType, aResponse));
#endif
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

Error Server::AppendDiagTlv(uint8_t aTlvType, Message &aMessage)
{
    Error error = kErrorNone;

    switch (aTlvType)
    {
    case Tlv::kExtMacAddress:
        error = Tlv::Append<ExtMacAddressTlv>(aMessage, Get<Mac::Mac>().GetExtAddress());
        break;

    case Tlv::kAddress16:
        error = Tlv::Append<Address16Tlv>(aMessage, Get<Mle::Mle>().GetRloc16());
        break;

    case Tlv::kMode:
        error = Tlv::Append<ModeTlv>(aMessage, Get<Mle::Mle>().GetDeviceMode().Get());
        break;

    case Tlv::kEui64:
    {
        Mac::ExtAddress eui64;

        Get<Radio>().GetIeeeEui64(eui64);
        error = Tlv::Append<Eui64Tlv>(aMessage, eui64);
        break;
    }

    case Tlv::kVersion:
        error = Tlv::Append<VersionTlv>(aMessage, kThreadVersion);
        break;

    case Tlv::kTimeout:
        VerifyOrExit(!Get<Mle::Mle>().IsRxOnWhenIdle());
        error = Tlv::Append<TimeoutTlv>(aMessage, Get<Mle::Mle>().GetTimeout());
        break;

    case Tlv::kLeaderData:
        error = Tlv::Append<LeaderDataTlv>(aMessage, LeaderDataTlvValue(Get<Mle::Mle>().GetLeaderData()));
        break;

    case Tlv::kNetworkData:
        error = Tlv::Append<NetworkDataTlv>(aMessage, Get<NetworkData::Leader>().GetBytes(),
                                            Get<NetworkData::Leader>().GetLength());
        break;

    case Tlv::kIp6AddressList:
        error = AppendIp6AddressList(aMessage);
        break;

    case Tlv::kMacCounters:
    {
        MacCountersTlv tlv;

        tlv.Init(Get<Mac::Mac>().GetCounters());
        error = tlv.AppendTo(aMessage);
        break;
    }

    case Tlv::kMleCounters:
    {
        MleCountersTlv tlv;

        tlv.Init(Get<Mle::Mle>().GetCounters());
        error = tlv.AppendTo(aMessage);
        break;
    }

    case Tlv::kVendorName:
        error = Tlv::Append<VendorNameTlv>(aMessage, Get<VendorInfo>().GetName());
        break;

    case Tlv::kVendorModel:
        error = Tlv::Append<VendorModelTlv>(aMessage, Get<VendorInfo>().GetModel());
        break;

    case Tlv::kVendorSwVersion:
        error = Tlv::Append<VendorSwVersionTlv>(aMessage, Get<VendorInfo>().GetSwVersion());
        break;

    case Tlv::kVendorAppUrl:
        error = Tlv::Append<VendorAppUrlTlv>(aMessage, Get<VendorInfo>().GetAppUrl());
        break;

    case Tlv::kThreadStackVersion:
        error = Tlv::Append<ThreadStackVersionTlv>(aMessage, otGetVersionString());
        break;

    case Tlv::kChannelPages:
        error = Tlv::Append<ChannelPagesTlv>(aMessage, Radio::kSupportedChannelPages, Radio::kNumChannelPages);
        break;

    case Tlv::kNonPreferredChannels:
    {
        MeshCoP::ChannelMaskTlv::Value value;

        MeshCoP::ChannelMaskTlv::PrepareValue(value, mNonPreferredChannels, /* aIncludeZeroPageMasks */ true);
        error = Tlv::AppendTlv(aMessage, Tlv::kNonPreferredChannels, value.mData, value.mLength);
        break;
    }

#if OPENTHREAD_FTD

    case Tlv::kConnectivity:
    {
        ConnectivityTlvValue tlvValue;

        Get<Mle::Mle>().FillConnectivityTlvValue(tlvValue);
        error = Tlv::Append<ConnectivityTlv>(aMessage, &tlvValue, sizeof(tlvValue));
        break;
    }

    case Tlv::kRoute:
        SuccessOrExit(error = Get<RouterTable>().AppendRouteTlv(aMessage, Tlv::kRoute));
        break;

    case Tlv::kEnhancedRoute:
        error = AppendEnhancedRoute(aMessage);
        break;

    case Tlv::kChildTable:
        error = AppendChildTable(aMessage);
        break;

    case Tlv::kMaxChildTimeout:
    {
        uint32_t maxTimeout;

        SuccessOrExit(Get<Mle::Mle>().GetMaxChildTimeout(maxTimeout));
        error = Tlv::Append<MaxChildTimeoutTlv>(aMessage, maxTimeout);
        break;
    }

#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

    case Tlv::kBrState:
        error = Tlv::Append<BrStateTlv>(aMessage, Get<BorderRouter::RoutingManager>().GetState());
        break;

    case Tlv::kBrIfAddrs:
        error = AppendBorderRouterIfAddrs(aMessage);
        break;

    case Tlv::kBrLocalOmrPrefix:
    case Tlv::kBrDhcp6PdOmrPrefix:
    case Tlv::kBrLocalOnlinkPrefix:
    case Tlv::kBrFavoredOnLinkPrefix:
        error = AppendBrPrefixTlv(aTlvType, aMessage);
        break;

#endif

    default:
        break;
    }

exit:
    return error;
}

template <> void Server::HandleTmf<kUriDiagnosticGetQuery>(Coap::Msg &aMsg)
{
    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetQuery>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    // DIAG_GET.qry may be sent as a confirmable request.
    if (aMsg.IsConfirmable())
    {
        IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg));
    }

#if OPENTHREAD_MTD
    SendAnswer(aMsg.mMessageInfo.GetPeerAddr(), aMsg.mMessage);
#elif OPENTHREAD_FTD
    PrepareAndSendAnswers(aMsg.mMessageInfo.GetPeerAddr(), aMsg.mMessage);
#endif
}

#if OPENTHREAD_MTD

void Server::SendAnswer(const Ip6::Address &aDestination, const Message &aRequest)
{
    Error          error  = kErrorNone;
    Coap::Message *answer = nullptr;
    AnswerTlvValue answerTlvValue;
    uint16_t       queryId;

    answer = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriDiagnosticGetAnswer);
    VerifyOrExit(answer != nullptr, error = kErrorNoBufs);

    IgnoreError(answer->SetPriority(aRequest.GetPriority()));

    if (Tlv::Find<QueryIdTlv>(aRequest, queryId) == kErrorNone)
    {
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*answer, queryId));
    }

    SuccessOrExit(error = AppendRequestedTlvs(aRequest, *answer));

    answerTlvValue.Init(0, AnswerTlvValue::kIsLast);
    SuccessOrExit(error = Tlv::Append<AnswerTlv>(*answer, answerTlvValue));

    error = Get<Tmf::Agent>().SendMessageAllowMulticastLoop(*answer, aDestination);

exit:
    FreeMessageOnError(answer, error);
}

#endif // OPENTHREAD_MTD

#if OPENTHREAD_FTD

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
    // This method dequeues and frees all answer messages related to
    // same query as `aFirstAnswer`. Note that related answers are
    // enqueued in order.

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
    AnswerBuilder  answerBuilder(GetInstance());
    Coap::Message *firstAnswer;

    SuccessOrExit(PrepareAnswers(aRequest, answerBuilder));

    firstAnswer = answerBuilder.GetAnswers().GetHead();
    mAnswerQueue.EnqueueAllFrom(answerBuilder.GetAnswers());

    SendNextAnswer(*firstAnswer, aDestination);

exit:
    return;
}

Error Server::PrepareAnswers(const Coap::Message &aRequest, AnswerBuilder &aAnswerBuilder)
{
    Error               error;
    uint8_t             tlvType;
    TlvTypeListIterator iterator;

    SuccessOrExit(error = aAnswerBuilder.Start(aRequest));

    SuccessOrExit(error = iterator.InitForTypeListTlv(aRequest));

    while (iterator.ReadNextTlvType(tlvType) == kErrorNone)
    {
        switch (tlvType)
        {
        case ChildTlv::kType:
            SuccessOrExit(error = AppendChildTableAsChildTlvs(aAnswerBuilder));
            break;
        case ChildIp6AddressListTlv::kType:
            SuccessOrExit(error = AppendChildTableIp6AddressList(aAnswerBuilder));
            break;
        case RouterNeighborTlv::kType:
            SuccessOrExit(error = AppendRouterNeighborTlvs(aAnswerBuilder));
            break;
        default:
            SuccessOrExit(error = AppendDiagTlv(tlvType, aAnswerBuilder.GetAnswer()));
            break;
        }

        SuccessOrExit(error = aAnswerBuilder.CheckAnswerLength());
    }

    SuccessOrExit(error = aAnswerBuilder.Finish());

exit:
    return error;
}

void Server::SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination)
{
    // This method send the given next `aAnswer` associated with
    // a query to the  `aDestination`.

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

Error Server::AppendChildTableAsChildTlvs(AnswerBuilder &aAnswerBuilder)
{
    Error         error = kErrorNone;
    ChildTlvValue childTlvValue;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        childTlvValue.InitFrom(child);

        SuccessOrExit(error = Tlv::Append<ChildTlv>(aAnswerBuilder.GetAnswer(), childTlvValue));
        SuccessOrExit(error = aAnswerBuilder.CheckAnswerLength());
    }

    error = Tlv::AppendEmpty<ChildTlv>(aAnswerBuilder.GetAnswer());

exit:
    return error;
}

Error Server::AppendRouterNeighborTlvs(AnswerBuilder &aAnswerBuilder)
{
    Error                  error = kErrorNone;
    RouterNeighborTlvValue neighborTlvValue;

    for (Router &router : Get<RouterTable>())
    {
        if (!router.IsStateValid())
        {
            continue;
        }

        neighborTlvValue.InitFrom(router);

        SuccessOrExit(error = Tlv::Append<RouterNeighborTlv>(aAnswerBuilder.GetAnswer(), neighborTlvValue));
        SuccessOrExit(error = aAnswerBuilder.CheckAnswerLength());
    }

    error = Tlv::AppendEmpty<RouterNeighborTlv>(aAnswerBuilder.GetAnswer());

exit:
    return error;
}

Error Server::AppendChildTableIp6AddressList(AnswerBuilder &aAnswerBuilder)
{
    Error error = kErrorNone;

    for (const Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        SuccessOrExit(error = AppendChildIp6AddressListTlv(aAnswerBuilder.GetAnswer(), child));
        SuccessOrExit(error = aAnswerBuilder.CheckAnswerLength());
    }

    error = Tlv::AppendEmpty<ChildIp6AddressListTlv>(aAnswerBuilder.GetAnswer());

exit:
    return error;
}

Error Server::AppendChildIp6AddressListTlv(Message &aAnswer, const Child &aChild)
{
    Error                       error = kErrorNone;
    Tlv::Bookmark               tlvBookmark;
    ChildIp6AddressListTlvValue tlvValue;
    Ip6::Address                mlEid;

    if (aChild.GetMeshLocalIp6Address(mlEid) != kErrorNone)
    {
        mlEid.Clear();
        VerifyOrExit(!aChild.GetIp6Addresses().IsEmpty());
    }

    SuccessOrExit(error = Tlv::StartTlv(aAnswer, Tlv::kChildIp6AddressList, tlvBookmark));

    tlvValue.SetRloc16(aChild.GetRloc16());

    SuccessOrExit(error = aAnswer.Append(tlvValue));

    if (!mlEid.IsUnspecified())
    {
        SuccessOrExit(error = aAnswer.Append(mlEid));
    }

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        SuccessOrExit(error = aAnswer.Append(address));
        SuccessOrExit(error = Tlv::AdjustTlv(aAnswer, tlvBookmark));
    }

    error = Tlv::EndTlv(aAnswer, tlvBookmark);

exit:
    return error;
}

#endif // OPENTHREAD_FTD

template <> void Server::HandleTmf<kUriDiagnosticGetRequest>(Coap::Msg &aMsg)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    VerifyOrExit(aMsg.IsConfirmable(), error = kErrorDrop);

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetRequest>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    response = Get<Tmf::Agent>().AllocateAndInitResponseFor(aMsg.mMessage);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    IgnoreError(response->SetPriority(aMsg.mMessage.GetPriority()));
    SuccessOrExit(error = AppendRequestedTlvs(aMsg.mMessage, *response));
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMsg.mMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

template <> void Server::HandleTmf<kUriDiagnosticReset>(Coap::Msg &aMsg)
{
    TlvTypeListIterator iterator;
    uint8_t             tlvType;

    VerifyOrExit(aMsg.IsConfirmable());

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticReset>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    SuccessOrExit(iterator.InitForTypeListTlv(aMsg.mMessage));

    while (iterator.ReadNextTlvType(tlvType) == kErrorNone)
    {
        switch (tlvType)
        {
        case Tlv::kMacCounters:
            Get<Mac::Mac>().ResetCounters();
            break;

        case Tlv::kMleCounters:
            Get<Mle::Mle>().ResetCounters();
            break;

        case Tlv::kNonPreferredChannels:
            mNonPreferredChannelsResetCallback.InvokeIfSet();
            break;

        default:
            break;
        }
    }

    IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg));

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Server::TlvTypeListIterator

void Server::TlvTypeListIterator::Init(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    mMessage     = &aMessage;
    mOffsetRange = aOffsetRange;
    mProcessedTlvs.Clear();
}

Error Server::TlvTypeListIterator::InitForTypeListTlv(const Message &aMessage)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, Tlv::kTypeList, mOffsetRange));
    mMessage = &aMessage;
    mProcessedTlvs.Clear();

exit:
    return error;
}

Error Server::TlvTypeListIterator::ReadNextTlvType(uint8_t &aTlvType)
{
    Error error;

    while (!mOffsetRange.IsEmpty())
    {
        SuccessOrExit(error = mMessage->ReadAndAdvance(mOffsetRange, aTlvType));

        if (!mProcessedTlvs.Has(aTlvType))
        {
            mProcessedTlvs.Add(aTlvType);
            error = kErrorNone;
            ExitNow();
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mQueryId(Random::NonCrypto::GetUint16())
{
}

Error Client::SendDiagnosticGet(const Ip6::Address &aDestination,
                                const uint8_t       aTlvTypes[],
                                uint8_t             aCount,
                                GetCallback         aCallback,
                                void               *aContext)
{
    Error error;

    if (aDestination.IsMulticast())
    {
        error = SendCommand(kUriDiagnosticGetQuery, Message::kPriorityNormal, aDestination, aTlvTypes, aCount);
    }
    else
    {
        error = SendCommand(kUriDiagnosticGetRequest, Message::kPriorityNormal, aDestination, aTlvTypes, aCount,
                            HandleGetResponse, this);
    }

    SuccessOrExit(error);

    mGetCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error Client::SendCommand(Uri                 aUri,
                          Message::Priority   aPriority,
                          const Ip6::Address &aDestination,
                          const uint8_t       aTlvTypes[],
                          uint8_t             aCount)
{
    return SendCommand(aUri, aPriority, aDestination, aTlvTypes, aCount, nullptr, nullptr);
}

Error Client::SendCommand(Uri                   aUri,
                          Message::Priority     aPriority,
                          const Ip6::Address   &aDestination,
                          const uint8_t         aTlvTypes[],
                          uint8_t               aCount,
                          Coap::ResponseHandler aHandler,
                          void                 *aContext)
{
    Error          error;
    Coap::Message *message = nullptr;

    switch (aUri)
    {
    case kUriDiagnosticGetQuery:
        message = Get<Tmf::Agent>().AllocateAndInitNonConfirmablePostMessage(aUri);
        break;

    case kUriDiagnosticGetRequest:
    case kUriDiagnosticReset:
        message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(aUri);
        break;

    default:
        OT_ASSERT(false);
    }

    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    IgnoreError(message->SetPriority(aPriority));

    if (aCount > 0)
    {
        SuccessOrExit(error = Tlv::Append<TypeListTlv>(*message, aTlvTypes, aCount));
    }

    if (aUri == kUriDiagnosticGetQuery)
    {
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*message, ++mQueryId));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageAllowMulticastLoop(*message, aDestination, aHandler, aContext));

    LogInfo("Sent %s to %s", UriToString(aUri), aDestination.ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Client::HandleGetResponse(Coap::Msg *aMsg, Error aResult)
{
    SuccessOrExit(aResult);
    VerifyOrExit(aMsg->GetCode() == Coap::kCodeChanged, aResult = kErrorFailed);

exit:
    mGetCallback.InvokeIfSet(aResult, (aMsg == nullptr) ? nullptr : &aMsg->mMessage,
                             (aMsg == nullptr) ? nullptr : &aMsg->mMessageInfo);
}

template <> void Client::HandleTmf<kUriDiagnosticGetAnswer>(Coap::Msg &aMsg)
{
    VerifyOrExit(aMsg.IsConfirmable());

    LogInfo("Received %s from %s", ot::UriToString<kUriDiagnosticGetAnswer>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
    // Let the `MeshDiag` process the message first.
    if (!Get<Utils::MeshDiag>().HandleDiagnosticGetAnswer(aMsg.mMessage, aMsg.mMessageInfo))
#endif
    {
        mGetCallback.InvokeIfSet(kErrorNone, &aMsg.mMessage, &aMsg.mMessageInfo);
    }

    IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg));

exit:
    return;
}

Error Client::SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount)
{
    return SendCommand(kUriDiagnosticReset, Message::kPriorityNormal, aDestination, aTlvTypes, aCount);
}

void Client::GetRouteInfo(const RouteTlv::Data &aRouteTlvData, RouteInfo &aNetDiagRouteInfo)
{
    uint8_t routeCount = 0;

    aNetDiagRouteInfo.mIdSequence = aRouteTlvData.GetRouterIdSequence();

    for (const RouteTlv::Data::Entry &entry : aRouteTlvData.GetEntries())
    {
        aNetDiagRouteInfo.mRouteData[routeCount].mRouterId       = entry.GetRouterId();
        aNetDiagRouteInfo.mRouteData[routeCount].mRouteCost      = entry.GetRouteCost();
        aNetDiagRouteInfo.mRouteData[routeCount].mLinkQualityIn  = entry.GetLinkQualityIn();
        aNetDiagRouteInfo.mRouteData[routeCount].mLinkQualityOut = entry.GetLinkQualityOut();
        routeCount++;
    }

    aNetDiagRouteInfo.mRouteCount = routeCount;
}

static Error ParseEnhancedRoute(const Message &aMessage, uint16_t aOffset, otNetworkDiagEnhRoute &aNetworkDiagEnhRoute)
{
    Error             error;
    OffsetRange       offsetRange;
    Tlv               tlv;
    Mle::RouterIdMask routerIdMask;
    uint8_t           index;

    SuccessOrExit(error = aMessage.Read(aOffset, tlv));

    VerifyOrExit(!tlv.IsExtended(), error = kErrorParse);
    VerifyOrExit(tlv.GetType() == Tlv::kEnhancedRoute, error = kErrorParse);

    aOffset += sizeof(tlv);
    offsetRange.Init(aOffset, tlv.GetLength());

    SuccessOrExit(error = routerIdMask.ReadMaskFrom(aMessage, offsetRange));
    offsetRange.AdvanceOffset(Mle::RouterIdMask::kMaskSize);

    index = 0;

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        EnhancedRouteTlvEntry entry;

        if (!routerIdMask.IsAllocated(routerId))
        {
            continue;
        }

        SuccessOrExit(error = aMessage.ReadAndAdvance(offsetRange, entry));

        aNetworkDiagEnhRoute.mRouteData[index].mRouterId = routerId;
        entry.Parse(aNetworkDiagEnhRoute.mRouteData[index]);

        index++;
    }

    aNetworkDiagEnhRoute.mRouteCount = index;

exit:
    return error;
}

void Client::ReadDiagData(DiagData &aDiagData, const Message &aMessage, const Tlv::Info &aTlvInfo)
{
    OffsetRange offsetRange = aTlvInfo.GetValueOffsetRange();

    offsetRange.ShrinkLength(GetArrayLength(aDiagData.m8));
    aDiagData.mCount = static_cast<uint8_t>(aMessage.ReadBytes(offsetRange, aDiagData.m8));
}

Error Client::ParseChildTable(ChildTable &aChildTable, const Message &aMessage, OffsetRange aOffsetRange)
{
    Error error = kErrorNone;

    // `ChildTable` has a fixed array of Child Table entries. If there
    // are more entries in the message, we read and return as many as
    // can fit in array and ignore the rest.

    aChildTable.mCount = 0;

    while (!aOffsetRange.IsEmpty() && (aChildTable.mCount < GetArrayLength(aChildTable.mTable)))
    {
        ChildTableTlvEntry entry;

        SuccessOrExit(error = aMessage.ReadAndAdvance(aOffsetRange, entry));

        entry.Parse(aChildTable.mTable[aChildTable.mCount]);
        aChildTable.mCount++;
    }

exit:
    return error;
}

void Client::ParseIp6AddrList(Ip6AddrList &aIp6Addrs, const Message &aMessage, OffsetRange aOffsetRange)
{
    aIp6Addrs.mCount = 0;

    while (aOffsetRange.Contains(sizeof(Ip6::Address)) && (aIp6Addrs.mCount < GetArrayLength(aIp6Addrs.mList)))
    {
        IgnoreError(aMessage.ReadAndAdvance(aOffsetRange, aIp6Addrs.mList[aIp6Addrs.mCount]));

        aIp6Addrs.mCount++;
    }
}

Error Client::GetNextDiagTlv(const Coap::Message &aMessage, Iterator &aIterator, DiagTlv &aDiagTlv)
{
    Error     error;
    uint16_t  offset = (aIterator == 0) ? aMessage.GetOffset() : aIterator;
    Tlv::Info tlvInfo;

    while (offset < aMessage.GetLength())
    {
        bool skipTlv = false;

        SuccessOrExit(error = tlvInfo.ParseFrom(aMessage, offset));

        switch (tlvInfo.GetType())
        {
        case Tlv::kExtMacAddress:
            SuccessOrExit(error = tlvInfo.Read<ExtMacAddressTlv>(aMessage, AsCoreType(&aDiagTlv.mData.mExtAddress)));
            break;

        case Tlv::kAddress16:
            SuccessOrExit(error = tlvInfo.Read<Address16Tlv>(aMessage, aDiagTlv.mData.mAddr16));
            break;

        case Tlv::kMode:
        {
            uint8_t mode;

            SuccessOrExit(error = tlvInfo.Read<ModeTlv>(aMessage, mode));
            Mle::DeviceMode(mode).Get(aDiagTlv.mData.mMode);
            break;
        }

        case Tlv::kTimeout:
            SuccessOrExit(error = tlvInfo.Read<TimeoutTlv>(aMessage, aDiagTlv.mData.mTimeout));
            break;

        case Tlv::kConnectivity:
        {
            ConnectivityTlvValue tlvValue;

            SuccessOrExit(error = tlvValue.ParseFrom(aMessage, tlvInfo.GetValueOffsetRange()));
            tlvValue.GetConnectivity(AsCoreType(&aDiagTlv.mData.mConnectivity));
            break;
        }

        case Tlv::kRoute:
        {
            RouteTlv::Data routeTlvData;

            SuccessOrExit(error = routeTlvData.ParseFrom(aMessage, tlvInfo.GetValueOffsetRange()));
            GetRouteInfo(routeTlvData, aDiagTlv.mData.mRoute);
            break;
        }

        case Tlv::kEnhancedRoute:
            SuccessOrExit(error = ParseEnhancedRoute(aMessage, offset, aDiagTlv.mData.mEnhRoute));
            break;

        case Tlv::kLeaderData:
        {
            LeaderDataTlvValue tlvValue;

            SuccessOrExit(error = tlvInfo.Read<LeaderDataTlv>(aMessage, tlvValue));
            tlvValue.Get(AsCoreType(&aDiagTlv.mData.mLeaderData));
            break;
        }

        case Tlv::kNetworkData:
            static_assert(sizeof(aDiagTlv.mData.mNetworkData.m8) >= NetworkData::NetworkData::kMaxSize,
                          "NetworkData array in `otNetworkDiagTlv` is too small");

            VerifyOrExit(tlvInfo.GetLength() <= NetworkData::NetworkData::kMaxSize, error = kErrorParse);
            ReadDiagData(aDiagTlv.mData.mNetworkData, aMessage, tlvInfo);
            break;

        case Tlv::kIp6AddressList:
            ParseIp6AddrList(aDiagTlv.mData.mIp6AddrList, aMessage, tlvInfo.GetValueOffsetRange());
            break;

        case Tlv::kMacCounters:
        {
            MacCountersTlv macCountersTlv;

            SuccessOrExit(error = aMessage.Read(offset, macCountersTlv));
            VerifyOrExit(macCountersTlv.IsValid(), error = kErrorParse);
            macCountersTlv.Read(aDiagTlv.mData.mMacCounters);
            break;
        }

        case Tlv::kMleCounters:
        {
            MleCountersTlv mleCoutersTlv;

            SuccessOrExit(error = aMessage.Read(offset, mleCoutersTlv));
            VerifyOrExit(mleCoutersTlv.IsValid(), error = kErrorParse);
            mleCoutersTlv.Read(aDiagTlv.mData.mMleCounters);
            break;
        }

        case Tlv::kBatteryLevel:
            SuccessOrExit(error = tlvInfo.Read<BatteryLevelTlv>(aMessage, aDiagTlv.mData.mBatteryLevel));
            break;

        case Tlv::kSupplyVoltage:
            SuccessOrExit(error = tlvInfo.Read<SupplyVoltageTlv>(aMessage, aDiagTlv.mData.mSupplyVoltage));
            break;

        case Tlv::kChildTable:
            SuccessOrExit(error = ParseChildTable(aDiagTlv.mData.mChildTable, aMessage, tlvInfo.GetValueOffsetRange()));
            break;

        case Tlv::kChannelPages:
            ReadDiagData(aDiagTlv.mData.mChannelPages, aMessage, tlvInfo);
            break;

        case Tlv::kMaxChildTimeout:
            SuccessOrExit(error = tlvInfo.Read<MaxChildTimeoutTlv>(aMessage, aDiagTlv.mData.mMaxChildTimeout));
            break;

        case Tlv::kEui64:
            SuccessOrExit(error = tlvInfo.Read<Eui64Tlv>(aMessage, AsCoreType(&aDiagTlv.mData.mEui64)));
            break;

        case Tlv::kVersion:
            SuccessOrExit(error = tlvInfo.Read<VersionTlv>(aMessage, aDiagTlv.mData.mVersion));
            break;

        case Tlv::kVendorName:
            SuccessOrExit(error = tlvInfo.Read<VendorNameTlv>(aMessage, aDiagTlv.mData.mVendorName));
            break;

        case Tlv::kVendorModel:
            SuccessOrExit(error = tlvInfo.Read<VendorModelTlv>(aMessage, aDiagTlv.mData.mVendorModel));
            break;

        case Tlv::kVendorSwVersion:
            SuccessOrExit(error = tlvInfo.Read<VendorSwVersionTlv>(aMessage, aDiagTlv.mData.mVendorSwVersion));
            break;

        case Tlv::kVendorAppUrl:
            SuccessOrExit(error = tlvInfo.Read<VendorAppUrlTlv>(aMessage, aDiagTlv.mData.mVendorAppUrl));
            break;

        case Tlv::kThreadStackVersion:
            SuccessOrExit(error = tlvInfo.Read<ThreadStackVersionTlv>(aMessage, aDiagTlv.mData.mThreadStackVersion));
            break;

        case Tlv::kNonPreferredChannels:
            SuccessOrExit(error = MeshCoP::ChannelMaskTlv::ParseValue(aMessage, tlvInfo.GetValueOffsetRange(),
                                                                      aDiagTlv.mData.mNonPreferredChannels));
            break;

        case Tlv::kBrState:
        {
            uint8_t state;

            SuccessOrExit(error = tlvInfo.Read<BrStateTlv>(aMessage, state));
            aDiagTlv.mData.mBrState = static_cast<BrState>(state);
            break;
        }

        case Tlv::kBrIfAddrs:
            ParseIp6AddrList(aDiagTlv.mData.mBrIfAddrList, aMessage, tlvInfo.GetValueOffsetRange());
            break;

        case Tlv::kBrLocalOmrPrefix:
        case Tlv::kBrLocalOnlinkPrefix:
        case Tlv::kBrFavoredOnLinkPrefix:
        case Tlv::kBrDhcp6PdOmrPrefix:
            SuccessOrExit(error = aMessage.Read(tlvInfo.GetValueOffsetRange(), aDiagTlv.mData.mBrPrefix));
            break;

        default:
            // Skip unrecognized TLVs.
            skipTlv = true;
            break;
        }

        offset += tlvInfo.GetSize();

        if (!skipTlv)
        {
            // Exit if a TLV is recognized and parsed successfully.
            aDiagTlv.mType = tlvInfo.GetType();
            aIterator      = offset;
            error          = kErrorNone;
            ExitNow();
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Client::UriToString(Uri aUri)
{
    const char *str = "";

    switch (aUri)
    {
    case kUriDiagnosticGetQuery:
        str = ot::UriToString<kUriDiagnosticGetQuery>();
        break;
    case kUriDiagnosticGetRequest:
        str = ot::UriToString<kUriDiagnosticGetRequest>();
        break;
    case kUriDiagnosticReset:
        str = ot::UriToString<kUriDiagnosticReset>();
        break;
    default:
        break;
    }

    return str;
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

#endif // OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

} // namespace NetworkDiagnostic

} // namespace ot
