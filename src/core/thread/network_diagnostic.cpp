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

const char Server::kVendorName[]      = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME;
const char Server::kVendorModel[]     = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL;
const char Server::kVendorSwVersion[] = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION;
const char Server::kVendorAppUrl[]    = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_APP_URL;

//---------------------------------------------------------------------------------------------------------------------
// Server

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNonPreferredChannels(0)
{
    static_assert(sizeof(kVendorName) <= sizeof(VendorNameTlv::StringType), "VENDOR_NAME is too long");
    static_assert(sizeof(kVendorModel) <= sizeof(VendorModelTlv::StringType), "VENDOR_MODEL is too long");
    static_assert(sizeof(kVendorSwVersion) <= sizeof(VendorSwVersionTlv::StringType), "VENDOR_SW_VERSION is too long");
    static_assert(sizeof(kVendorAppUrl) <= sizeof(VendorAppUrlTlv::StringType), "VENDOR_APP_URL is too long");

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    memcpy(mVendorName, kVendorName, sizeof(kVendorName));
    memcpy(mVendorModel, kVendorModel, sizeof(kVendorModel));
    memcpy(mVendorSwVersion, kVendorSwVersion, sizeof(kVendorSwVersion));
    memcpy(mVendorAppUrl, kVendorAppUrl, sizeof(kVendorAppUrl));
#endif
}

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

Error Server::SetVendorName(const char *aVendorName)
{
    return StringCopy(mVendorName, aVendorName, kStringCheckUtf8Encoding);
}

Error Server::SetVendorModel(const char *aVendorModel)
{
    return StringCopy(mVendorModel, aVendorModel, kStringCheckUtf8Encoding);
}

Error Server::SetVendorSwVersion(const char *aVendorSwVersion)
{
    return StringCopy(mVendorSwVersion, aVendorSwVersion, kStringCheckUtf8Encoding);
}

Error Server::SetVendorAppUrl(const char *aVendorAppUrl)
{
    return StringCopy(mVendorAppUrl, aVendorAppUrl, kStringCheckUtf8Encoding);
}

#endif

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
        uint8_t         timeout = 0;
        ChildTableEntry entry;

        if (++count > kMaxChildEntries)
        {
            break;
        }

        while (static_cast<uint32_t>(1 << timeout) < child.GetTimeout())
        {
            timeout++;
        }

        entry.Clear();
        entry.SetTimeout(timeout + 4);
        entry.SetLinkQuality(child.GetLinkQualityIn());
        entry.SetChildId(Mle::ChildIdFromRloc16(child.GetRloc16()));
        entry.SetMode(child.GetDeviceMode());

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
    Mle::RouterIdSet      routerIdSet;
    EnhancedRouteTlvEntry entry;

    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());

    Get<RouterTable>().GetRouterIdSet(routerIdSet);

    SuccessOrExit(error = Tlv::StartTlv(aMessage, Tlv::kEnhancedRoute, tlvBookmark));

    SuccessOrExit(error = aMessage.Append(routerIdSet));

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (!routerIdSet.Contains(routerId))
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
    Error    error = kErrorNone;
    ChildTlv childTlv;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        childTlv.InitFrom(child);

        SuccessOrExit(error = childTlv.AppendTo(aMessage));
    }

    error = Tlv::AppendEmpty<ChildTlv>(aMessage);

exit:
    return error;
}

Error Server::AppendRouterNeighborTlvs(Message &aMessage)
{
    Error             error = kErrorNone;
    RouterNeighborTlv neighborTlv;

    for (Router &router : Get<RouterTable>())
    {
        if (router.IsStateValid())
        {
            neighborTlv.InitFrom(router);
            SuccessOrExit(error = neighborTlv.AppendTo(aMessage));
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

Error Server::AppendMacCounters(Message &aMessage)
{
    MacCountersTlv       tlv;
    const otMacCounters &counters = Get<Mac::Mac>().GetCounters();

    ClearAllBytes(tlv);

    tlv.Init();
    tlv.SetIfInUnknownProtos(counters.mRxOther);
    tlv.SetIfInErrors(counters.mRxErrNoFrame + counters.mRxErrUnknownNeighbor + counters.mRxErrInvalidSrcAddr +
                      counters.mRxErrSec + counters.mRxErrFcs + counters.mRxErrOther);
    tlv.SetIfOutErrors(counters.mTxErrCca);
    tlv.SetIfInUcastPkts(counters.mRxUnicast);
    tlv.SetIfInBroadcastPkts(counters.mRxBroadcast);
    tlv.SetIfInDiscards(counters.mRxAddressFiltered + counters.mRxDestAddrFiltered + counters.mRxDuplicated);
    tlv.SetIfOutUcastPkts(counters.mTxUnicast);
    tlv.SetIfOutBroadcastPkts(counters.mTxBroadcast);
    tlv.SetIfOutDiscards(counters.mTxErrBusyChannel);

    return tlv.AppendTo(aMessage);
}

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
    Error         error;
    OffsetRange   offsetRange;
    TlvTypeBitSet processedTlvs;

    processedTlvs.Clear();

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRequest, Tlv::kTypeList, offsetRange));

    while (!offsetRange.IsEmpty())
    {
        uint8_t tlvType;

        SuccessOrExit(error = aRequest.Read(offsetRange, tlvType));
        offsetRange.AdvanceOffset(sizeof(tlvType));

        if (processedTlvs.Has(tlvType))
        {
            continue;
        }

        processedTlvs.Add(tlvType);

        SuccessOrExit(error = AppendDiagTlv(tlvType, aResponse));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
Error Server::AppendRequestedTlvsForTcat(const Message &aRequest, Message &aResponse, OffsetRange &aOffsetRange)
{
    Error         error = kErrorNone;
    TlvTypeBitSet processedTlvs;

    processedTlvs.Clear();

    while (!aOffsetRange.IsEmpty())
    {
        uint8_t tlvType;

        SuccessOrExit(error = aRequest.Read(aOffsetRange, tlvType));
        aOffsetRange.AdvanceOffset(sizeof(uint8_t));

        if (processedTlvs.Has(tlvType))
        {
            continue;
        }

        processedTlvs.Add(tlvType);

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
        error = AppendMacCounters(aMessage);
        break;

    case Tlv::kMleCounters:
    {
        MleCountersTlv tlv;

        tlv.Init(Get<Mle::Mle>().GetCounters());
        error = tlv.AppendTo(aMessage);
        break;
    }

    case Tlv::kVendorName:
        error = Tlv::Append<VendorNameTlv>(aMessage, GetVendorName());
        break;

    case Tlv::kVendorModel:
        error = Tlv::Append<VendorModelTlv>(aMessage, GetVendorModel());
        break;

    case Tlv::kVendorSwVersion:
        error = Tlv::Append<VendorSwVersionTlv>(aMessage, GetVendorSwVersion());
        break;

    case Tlv::kVendorAppUrl:
        error = Tlv::Append<VendorAppUrlTlv>(aMessage, GetVendorAppUrl());
        break;

    case Tlv::kThreadStackVersion:
        error = Tlv::Append<ThreadStackVersionTlv>(aMessage, otGetVersionString());
        break;

    case Tlv::kChannelPages:
    {
        ChannelPagesTlv tlv;
        uint8_t         length = 0;

        tlv.Init();

        for (uint8_t page : Radio::kSupportedChannelPages)
        {
            tlv.GetChannelPages()[length++] = page;
        }

        tlv.SetLength(length);
        error = tlv.AppendTo(aMessage);

        break;
    }

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
        ConnectivityTlv tlv;

        tlv.Init();
        Get<Mle::Mle>().FillConnectivityTlv(tlv);
        error = tlv.AppendTo(aMessage);
        break;
    }

    case Tlv::kRoute:
    {
        RouteTlv tlv;

        tlv.Init();
        Get<RouterTable>().FillRouteTlv(tlv);
        SuccessOrExit(error = tlv.AppendTo(aMessage));
        break;
    }

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
    VerifyOrExit(aMsg.mMessage.IsPostRequest());

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetQuery>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    // DIAG_GET.qry may be sent as a confirmable request.
    if (aMsg.mMessage.IsConfirmable())
    {
        IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMsg));
    }

#if OPENTHREAD_MTD
    SendAnswer(aMsg.mMessageInfo.GetPeerAddr(), aMsg.mMessage);
#elif OPENTHREAD_FTD
    PrepareAndSendAnswers(aMsg.mMessageInfo.GetPeerAddr(), aMsg.mMessage);
#endif

exit:
    return;
}

#if OPENTHREAD_MTD

void Server::SendAnswer(const Ip6::Address &aDestination, const Message &aRequest)
{
    Error            error  = kErrorNone;
    Coap::Message   *answer = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    AnswerTlv        answerTlv;
    uint16_t         queryId;

    answer = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticGetAnswer);
    VerifyOrExit(answer != nullptr, error = kErrorNoBufs);

    IgnoreError(answer->SetPriority(aRequest.GetPriority()));

    if (Tlv::Find<QueryIdTlv>(aRequest, queryId) == kErrorNone)
    {
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*answer, queryId));
    }

    SuccessOrExit(error = AppendRequestedTlvs(aRequest, *answer));

    answerTlv.Init(0, AnswerTlv::kIsLast);
    SuccessOrExit(answer->Append(answerTlv));

    PrepareMessageInfoForDest(aDestination, messageInfo);

    error = Get<Tmf::Agent>().SendMessage(*answer, messageInfo);

exit:
    FreeMessageOnError(answer, error);
}

#endif // OPENTHREAD_MTD

#if OPENTHREAD_FTD

Error Server::AllocateAnswer(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    // Allocate an `Answer` message, adds it in `mAnswerQueue`,
    // update the `aInfo.mFirstAnswer` if it is the first allocated
    // messages, and appends `QueryIdTlv` to the message (if needed).

    Error error = kErrorNone;

    aAnswer = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticGetAnswer);
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

void Server::PrepareAndSendAnswers(const Ip6::Address &aDestination, const Message &aRequest)
{
    Coap::Message *answer;
    Error          error;
    AnswerInfo     info;
    OffsetRange    offsetRange;
    AnswerTlv      answerTlv;

    if (Tlv::Find<QueryIdTlv>(aRequest, info.mQueryId) == kErrorNone)
    {
        info.mHasQueryId = true;
    }

    info.mPriority = aRequest.GetPriority();

    SuccessOrExit(error = AllocateAnswer(answer, info));

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRequest, Tlv::kTypeList, offsetRange));

    while (!offsetRange.IsEmpty())
    {
        uint8_t tlvType;

        SuccessOrExit(error = aRequest.Read(offsetRange, tlvType));
        offsetRange.AdvanceOffset(sizeof(tlvType));

        switch (tlvType)
        {
        case ChildTlv::kType:
            SuccessOrExit(error = AppendChildTableAsChildTlvs(answer, info));
            break;
        case ChildIp6AddressListTlv::kType:
            SuccessOrExit(error = AppendChildTableIp6AddressList(answer, info));
            break;
        case RouterNeighborTlv::kType:
            SuccessOrExit(error = AppendRouterNeighborTlvs(answer, info));
            break;
        default:
            SuccessOrExit(error = AppendDiagTlv(tlvType, *answer));
            break;
        }

        SuccessOrExit(error = CheckAnswerLength(answer, info));
    }

    answerTlv.Init(info.mAnswerIndex, AnswerTlv::kIsLast);
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
    // This method checks the length of the `aAnswer` message and if it
    // is above the threshold, it enqueues the message for transmission
    // after appending an Answer TLV with the current index to the
    // message. In this case, it will also allocate a new answer
    // message.

    Error     error = kErrorNone;
    AnswerTlv answerTlv;

    VerifyOrExit(aAnswer->GetLength() >= kAnswerMessageLengthThreshold);

    answerTlv.Init(aInfo.mAnswerIndex++, AnswerTlv::kMoreToFollow);
    SuccessOrExit(error = aAnswer->Append(answerTlv));

    error = AllocateAnswer(aAnswer, aInfo);

exit:
    return error;
}

void Server::SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination)
{
    // This method send the given next `aAnswer` associated with
    // a query to the  `aDestination`.

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

Error Server::AppendChildTableAsChildTlvs(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    Error    error = kErrorNone;
    ChildTlv childTlv;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        childTlv.InitFrom(child);

        SuccessOrExit(error = childTlv.AppendTo(*aAnswer));
        SuccessOrExit(error = CheckAnswerLength(aAnswer, aInfo));
    }

    error = Tlv::AppendEmpty<ChildTlv>(*aAnswer);

exit:
    return error;
}

Error Server::AppendRouterNeighborTlvs(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    Error             error = kErrorNone;
    RouterNeighborTlv neighborTlv;

    for (Router &router : Get<RouterTable>())
    {
        if (!router.IsStateValid())
        {
            continue;
        }

        neighborTlv.InitFrom(router);

        SuccessOrExit(error = neighborTlv.AppendTo(*aAnswer));
        SuccessOrExit(error = CheckAnswerLength(aAnswer, aInfo));
    }

    error = Tlv::AppendEmpty<RouterNeighborTlv>(*aAnswer);

exit:
    return error;
}

Error Server::AppendChildTableIp6AddressList(Coap::Message *&aAnswer, AnswerInfo &aInfo)
{
    Error error = kErrorNone;

    for (const Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        SuccessOrExit(error = AppendChildIp6AddressListTlv(*aAnswer, child));
        SuccessOrExit(error = CheckAnswerLength(aAnswer, aInfo));
    }

    error = Tlv::AppendEmpty<ChildIp6AddressListTlv>(*aAnswer);

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

    VerifyOrExit(aMsg.mMessage.IsConfirmablePostRequest(), error = kErrorDrop);

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetRequest>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    response = Get<Tmf::Agent>().NewResponseMessage(aMsg.mMessage);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    IgnoreError(response->SetPriority(aMsg.mMessage.GetPriority()));
    SuccessOrExit(error = AppendRequestedTlvs(aMsg.mMessage, *response));
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMsg.mMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

template <> void Server::HandleTmf<kUriDiagnosticReset>(Coap::Msg &aMsg)
{
    uint16_t offset = 0;
    uint8_t  type;
    Tlv      tlv;

    VerifyOrExit(aMsg.mMessage.IsConfirmablePostRequest());

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticReset>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    SuccessOrExit(aMsg.mMessage.Read(aMsg.mMessage.GetOffset(), tlv));

    VerifyOrExit(tlv.GetType() == Tlv::kTypeList);

    offset = aMsg.mMessage.GetOffset() + sizeof(Tlv);

    for (uint8_t i = 0; i < tlv.GetLength(); i++)
    {
        SuccessOrExit(aMsg.mMessage.Read(offset + i, type));

        switch (type)
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

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMsg));

exit:
    return;
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
                            &HandleGetResponse, this);
    }

    SuccessOrExit(error);

    mGetCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error Client::SendCommand(Uri                   aUri,
                          Message::Priority     aPriority,
                          const Ip6::Address   &aDestination,
                          const uint8_t         aTlvTypes[],
                          uint8_t               aCount,
                          Coap::ResponseHandler aHandler,
                          void                 *aContext)
{
    Error            error;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    switch (aUri)
    {
    case kUriDiagnosticGetQuery:
        message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(aUri);
        break;

    case kUriDiagnosticGetRequest:
    case kUriDiagnosticReset:
        message = Get<Tmf::Agent>().NewConfirmablePostMessage(aUri);
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

    Get<Server>().PrepareMessageInfoForDest(aDestination, messageInfo);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, aHandler, aContext));

    LogInfo("Sent %s to %s", UriToString(aUri), aDestination.ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Client::HandleGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    SuccessOrExit(aResult);
    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged, aResult = kErrorFailed);

exit:
    mGetCallback.InvokeIfSet(aResult, aMessage, aMessageInfo);
}

template <> void Client::HandleTmf<kUriDiagnosticGetAnswer>(Coap::Msg &aMsg)
{
    VerifyOrExit(aMsg.mMessage.IsConfirmablePostRequest());

    LogInfo("Received %s from %s", ot::UriToString<kUriDiagnosticGetAnswer>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
    // Let the `MeshDiag` process the message first.
    if (!Get<Utils::MeshDiag>().HandleDiagnosticGetAnswer(aMsg.mMessage, aMsg.mMessageInfo))
#endif
    {
        mGetCallback.InvokeIfSet(kErrorNone, &aMsg.mMessage, &aMsg.mMessageInfo);
    }

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMsg));

exit:
    return;
}

Error Client::SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount)
{
    return SendCommand(kUriDiagnosticReset, Message::kPriorityNormal, aDestination, aTlvTypes, aCount);
}

static void ParseRoute(const RouteTlv &aRouteTlv, otNetworkDiagRoute &aNetworkDiagRoute)
{
    uint8_t routeCount = 0;

    for (uint8_t i = 0; i <= Mle::kMaxRouterId; ++i)
    {
        if (!aRouteTlv.IsRouterIdSet(i))
        {
            continue;
        }
        aNetworkDiagRoute.mRouteData[routeCount].mRouterId       = i;
        aNetworkDiagRoute.mRouteData[routeCount].mRouteCost      = aRouteTlv.GetRouteCost(routeCount);
        aNetworkDiagRoute.mRouteData[routeCount].mLinkQualityIn  = aRouteTlv.GetLinkQualityIn(routeCount);
        aNetworkDiagRoute.mRouteData[routeCount].mLinkQualityOut = aRouteTlv.GetLinkQualityOut(routeCount);
        ++routeCount;
    }
    aNetworkDiagRoute.mRouteCount = routeCount;
    aNetworkDiagRoute.mIdSequence = aRouteTlv.GetRouterIdSequence();
}

static Error ParseEnhancedRoute(const Message &aMessage, uint16_t aOffset, otNetworkDiagEnhRoute &aNetworkDiagEnhRoute)
{
    Error            error;
    OffsetRange      offsetRange;
    Tlv              tlv;
    Mle::RouterIdSet routerIdSet;
    uint8_t          index;

    SuccessOrExit(error = aMessage.Read(aOffset, tlv));

    VerifyOrExit(!tlv.IsExtended(), error = kErrorParse);
    VerifyOrExit(tlv.GetType() == Tlv::kEnhancedRoute, error = kErrorParse);

    aOffset += sizeof(tlv);
    offsetRange.Init(aOffset, tlv.GetLength());

    SuccessOrExit(error = aMessage.Read(offsetRange, routerIdSet));
    offsetRange.AdvanceOffset(sizeof(routerIdSet));

    index = 0;

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        EnhancedRouteTlvEntry entry;

        if (!routerIdSet.Contains(routerId))
        {
            continue;
        }

        SuccessOrExit(error = aMessage.Read(offsetRange, entry));
        offsetRange.AdvanceOffset(sizeof(entry));

        aNetworkDiagEnhRoute.mRouteData[index].mRouterId = routerId;
        entry.Parse(aNetworkDiagEnhRoute.mRouteData[index]);

        index++;
    }

    aNetworkDiagEnhRoute.mRouteCount = index;

exit:
    return error;
}

void Client::ParseMacCounters(const MacCountersTlv &aMacCountersTlv, otNetworkDiagMacCounters &aMacCounters)
{
    aMacCounters.mIfInUnknownProtos  = aMacCountersTlv.GetIfInUnknownProtos();
    aMacCounters.mIfInErrors         = aMacCountersTlv.GetIfInErrors();
    aMacCounters.mIfOutErrors        = aMacCountersTlv.GetIfOutErrors();
    aMacCounters.mIfInUcastPkts      = aMacCountersTlv.GetIfInUcastPkts();
    aMacCounters.mIfInBroadcastPkts  = aMacCountersTlv.GetIfInBroadcastPkts();
    aMacCounters.mIfInDiscards       = aMacCountersTlv.GetIfInDiscards();
    aMacCounters.mIfOutUcastPkts     = aMacCountersTlv.GetIfOutUcastPkts();
    aMacCounters.mIfOutBroadcastPkts = aMacCountersTlv.GetIfOutBroadcastPkts();
    aMacCounters.mIfOutDiscards      = aMacCountersTlv.GetIfOutDiscards();
}

void Client::ParseIp6AddrList(Ip6AddrList &aIp6Addrs, const Message &aMessage, OffsetRange aOffsetRange)
{
    aIp6Addrs.mCount = 0;

    while (aOffsetRange.Contains(sizeof(Ip6::Address)) && (aIp6Addrs.mCount < GetArrayLength(aIp6Addrs.mList)))
    {
        IgnoreError(aMessage.Read(aOffsetRange, aIp6Addrs.mList[aIp6Addrs.mCount]));
        aOffsetRange.AdvanceOffset(sizeof(Ip6::Address));

        aIp6Addrs.mCount++;
    }
}

Error Client::GetNextDiagTlv(const Coap::Message &aMessage, Iterator &aIterator, DiagTlv &aDiagTlv)
{
    Error           error;
    uint16_t        offset = (aIterator == 0) ? aMessage.GetOffset() : aIterator;
    Tlv::ParsedInfo tlvInfo;

    while (offset < aMessage.GetLength())
    {
        bool skipTlv = false;

        SuccessOrExit(error = tlvInfo.ParseFrom(aMessage, offset));

        switch (tlvInfo.mType)
        {
        case Tlv::kExtMacAddress:
            SuccessOrExit(error =
                              Tlv::Read<ExtMacAddressTlv>(aMessage, offset, AsCoreType(&aDiagTlv.mData.mExtAddress)));
            break;

        case Tlv::kAddress16:
            SuccessOrExit(error = Tlv::Read<Address16Tlv>(aMessage, offset, aDiagTlv.mData.mAddr16));
            break;

        case Tlv::kMode:
        {
            uint8_t mode;

            SuccessOrExit(error = Tlv::Read<ModeTlv>(aMessage, offset, mode));
            Mle::DeviceMode(mode).Get(aDiagTlv.mData.mMode);
            break;
        }

        case Tlv::kTimeout:
            SuccessOrExit(error = Tlv::Read<TimeoutTlv>(aMessage, offset, aDiagTlv.mData.mTimeout));
            break;

        case Tlv::kConnectivity:
        {
            ConnectivityTlv connectivityTlv;

            VerifyOrExit(!tlvInfo.mIsExtended, error = kErrorParse);
            SuccessOrExit(error = aMessage.Read(offset, connectivityTlv));
            VerifyOrExit(connectivityTlv.IsValid(), error = kErrorParse);
            connectivityTlv.GetConnectivity(aDiagTlv.mData.mConnectivity);
            break;
        }

        case Tlv::kRoute:
        {
            RouteTlv routeTlv;
            uint16_t bytesToRead = Min<uint16_t>(tlvInfo.GetSize(), sizeof(routeTlv));

            VerifyOrExit(!tlvInfo.mIsExtended, error = kErrorParse);
            SuccessOrExit(error = aMessage.Read(offset, &routeTlv, bytesToRead));
            VerifyOrExit(routeTlv.IsValid(), error = kErrorParse);
            ParseRoute(routeTlv, aDiagTlv.mData.mRoute);
            break;
        }

        case Tlv::kEnhancedRoute:
            SuccessOrExit(error = ParseEnhancedRoute(aMessage, offset, aDiagTlv.mData.mEnhRoute));
            break;

        case Tlv::kLeaderData:
        {
            LeaderDataTlvValue tlvValue;

            SuccessOrExit(error = Tlv::Read<LeaderDataTlv>(aMessage, offset, tlvValue));
            tlvValue.Get(AsCoreType(&aDiagTlv.mData.mLeaderData));
            break;
        }

        case Tlv::kNetworkData:
            static_assert(sizeof(aDiagTlv.mData.mNetworkData.m8) >= NetworkData::NetworkData::kMaxSize,
                          "NetworkData array in `otNetworkDiagTlv` is too small");

            VerifyOrExit(tlvInfo.mValueOffsetRange.GetLength() <= NetworkData::NetworkData::kMaxSize,
                         error = kErrorParse);
            aDiagTlv.mData.mNetworkData.mCount = static_cast<uint8_t>(tlvInfo.mValueOffsetRange.GetLength());
            aMessage.ReadBytes(tlvInfo.mValueOffsetRange, aDiagTlv.mData.mNetworkData.m8);
            break;

        case Tlv::kIp6AddressList:
            ParseIp6AddrList(aDiagTlv.mData.mIp6AddrList, aMessage, tlvInfo.mValueOffsetRange);
            break;

        case Tlv::kMacCounters:
        {
            MacCountersTlv macCountersTlv;

            SuccessOrExit(error = aMessage.Read(offset, macCountersTlv));
            VerifyOrExit(macCountersTlv.IsValid(), error = kErrorParse);
            ParseMacCounters(macCountersTlv, aDiagTlv.mData.mMacCounters);
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
            SuccessOrExit(error = Tlv::Read<BatteryLevelTlv>(aMessage, offset, aDiagTlv.mData.mBatteryLevel));
            break;

        case Tlv::kSupplyVoltage:
            SuccessOrExit(error = Tlv::Read<SupplyVoltageTlv>(aMessage, offset, aDiagTlv.mData.mSupplyVoltage));
            break;

        case Tlv::kChildTable:
        {
            uint16_t   childInfoLength = GetArrayLength(aDiagTlv.mData.mChildTable.mTable);
            ChildInfo *childInfo       = &aDiagTlv.mData.mChildTable.mTable[0];
            uint8_t   &childCount      = aDiagTlv.mData.mChildTable.mCount;

            VerifyOrExit((tlvInfo.mValueOffsetRange.GetLength() % sizeof(ChildTableEntry)) == 0, error = kErrorParse);

            // `DiagTlv` has a fixed array Child Table entries. If there
            // are more entries in the message, we read and return as
            // many as can fit in array and ignore the rest.

            childCount = 0;

            while (!tlvInfo.mValueOffsetRange.IsEmpty() && (childCount < childInfoLength))
            {
                ChildTableEntry entry;

                SuccessOrExit(error = aMessage.Read(tlvInfo.mValueOffsetRange, entry));

                childInfo->mTimeout     = entry.GetTimeout();
                childInfo->mLinkQuality = entry.GetLinkQuality();
                childInfo->mChildId     = entry.GetChildId();
                entry.GetMode().Get(childInfo->mMode);

                childCount++;
                childInfo++;
                tlvInfo.mValueOffsetRange.AdvanceOffset(sizeof(ChildTableEntry));
            }

            break;
        }

        case Tlv::kChannelPages:
            aDiagTlv.mData.mChannelPages.mCount = static_cast<uint8_t>(
                Min(tlvInfo.mValueOffsetRange.GetLength(), GetArrayLength(aDiagTlv.mData.mChannelPages.m8)));
            aMessage.ReadBytes(tlvInfo.mValueOffsetRange.GetOffset(), aDiagTlv.mData.mChannelPages.m8,
                               aDiagTlv.mData.mChannelPages.mCount);
            break;

        case Tlv::kMaxChildTimeout:
            SuccessOrExit(error = Tlv::Read<MaxChildTimeoutTlv>(aMessage, offset, aDiagTlv.mData.mMaxChildTimeout));
            break;

        case Tlv::kEui64:
            SuccessOrExit(error = Tlv::Read<Eui64Tlv>(aMessage, offset, AsCoreType(&aDiagTlv.mData.mEui64)));
            break;

        case Tlv::kVersion:
            SuccessOrExit(error = Tlv::Read<VersionTlv>(aMessage, offset, aDiagTlv.mData.mVersion));
            break;

        case Tlv::kVendorName:
            SuccessOrExit(error = Tlv::Read<VendorNameTlv>(aMessage, offset, aDiagTlv.mData.mVendorName));
            break;

        case Tlv::kVendorModel:
            SuccessOrExit(error = Tlv::Read<VendorModelTlv>(aMessage, offset, aDiagTlv.mData.mVendorModel));
            break;

        case Tlv::kVendorSwVersion:
            SuccessOrExit(error = Tlv::Read<VendorSwVersionTlv>(aMessage, offset, aDiagTlv.mData.mVendorSwVersion));
            break;

        case Tlv::kVendorAppUrl:
            SuccessOrExit(error = Tlv::Read<VendorAppUrlTlv>(aMessage, offset, aDiagTlv.mData.mVendorAppUrl));
            break;

        case Tlv::kThreadStackVersion:
            SuccessOrExit(error =
                              Tlv::Read<ThreadStackVersionTlv>(aMessage, offset, aDiagTlv.mData.mThreadStackVersion));
            break;

        case Tlv::kNonPreferredChannels:
            SuccessOrExit(error = MeshCoP::ChannelMaskTlv::ParseValue(aMessage, tlvInfo.mValueOffsetRange,
                                                                      aDiagTlv.mData.mNonPreferredChannels));
            break;

        case Tlv::kBrState:
        {
            uint8_t state;

            SuccessOrExit(error = Tlv::Read<BrStateTlv>(aMessage, offset, state));
            aDiagTlv.mData.mBrState = static_cast<BrState>(state);
            break;
        }

        case Tlv::kBrIfAddrs:
            ParseIp6AddrList(aDiagTlv.mData.mBrIfAddrList, aMessage, tlvInfo.mValueOffsetRange);
            break;

        case Tlv::kBrLocalOmrPrefix:
        case Tlv::kBrLocalOnlinkPrefix:
        case Tlv::kBrFavoredOnLinkPrefix:
        case Tlv::kBrDhcp6PdOmrPrefix:
            SuccessOrExit(error = aMessage.Read(tlvInfo.mValueOffsetRange, aDiagTlv.mData.mBrPrefix));
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
            aDiagTlv.mType = tlvInfo.mType;
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
