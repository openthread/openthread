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

#include "coap/coap_message.hpp"
#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "mac/mac.hpp"
#include "net/netif.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/version.hpp"

namespace ot {

RegisterLogModule("NetDiag");

namespace NetworkDiagnostic {

//---------------------------------------------------------------------------------------------------------------------
// Server

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void Server::PrepareMessageInfoForDest(const Ip6::Address &aDestination, Tmf::MessageInfo &aMessageInfo) const
{
    if (aDestination.IsMulticast())
    {
        aMessageInfo.SetMulticastLoop(true);
    }

    if (aDestination.IsLinkLocal() || aDestination.IsLinkLocalMulticast())
    {
        aMessageInfo.SetSockAddr(Get<Mle::MleRouter>().GetLinkLocalAddress());
    }
    else
    {
        aMessageInfo.SetSockAddrToRloc();
    }

    aMessageInfo.SetPeerAddr(aDestination);
}

Error Server::AppendIp6AddressList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        OT_UNUSED_VARIABLE(addr);
        count++;
    }

    if (count * Ip6::Address::kSize <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kIp6AddressList);
        tlv.SetLength(static_cast<uint8_t>(count * Ip6::Address::kSize));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv extTlv;

        extTlv.SetType(Tlv::kIp6AddressList);
        extTlv.SetLength(count * Ip6::Address::kSize);
        SuccessOrExit(error = aMessage.Append(extTlv));
    }

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        SuccessOrExit(error = aMessage.Append(addr.GetAddress()));
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
Error Server::AppendChildTable(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count;

    VerifyOrExit(Get<Mle::MleRouter>().IsRouterOrLeader());

    count = Min(Get<ChildTable>().GetNumChildren(Child::kInStateValid), kMaxChildEntries);

    if (count * sizeof(ChildTableEntry) <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kChildTable);
        tlv.SetLength(static_cast<uint8_t>(count * sizeof(ChildTableEntry)));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv extTlv;

        extTlv.SetType(Tlv::kChildTable);
        extTlv.SetLength(count * sizeof(ChildTableEntry));
        SuccessOrExit(error = aMessage.Append(extTlv));
    }

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        uint8_t         timeout = 0;
        ChildTableEntry entry;

        VerifyOrExit(count--);

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
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

Error Server::AppendMacCounters(Message &aMessage)
{
    MacCountersTlv       tlv;
    const otMacCounters &counters = Get<Mac::Mac>().GetCounters();

    memset(&tlv, 0, sizeof(tlv));

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

Error Server::AppendRequestedTlvs(const Message &aRequest, Message &aResponse)
{
    Error    error;
    uint16_t offset;
    uint16_t length;
    uint16_t endOffset;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aRequest, Tlv::kTypeList, offset, length));
    endOffset = offset + length;

    for (; offset < endOffset; offset++)
    {
        uint8_t tlvType;

        SuccessOrExit(error = aRequest.Read(offset, tlvType));
        SuccessOrExit(error = AppendDiagTlv(tlvType, aResponse));
    }

exit:
    return error;
}

Error Server::AppendDiagTlv(uint8_t aTlvType, Message &aMessage)
{
    Error error = kErrorNone;

    switch (aTlvType)
    {
    case Tlv::kExtMacAddress:
        error = Tlv::Append<ExtMacAddressTlv>(aMessage, Get<Mac::Mac>().GetExtAddress());
        break;

    case Tlv::kAddress16:
        error = Tlv::Append<Address16Tlv>(aMessage, Get<Mle::MleRouter>().GetRloc16());
        break;

    case Tlv::kMode:
        error = Tlv::Append<ModeTlv>(aMessage, Get<Mle::MleRouter>().GetDeviceMode().Get());
        break;

    case Tlv::kVersion:
        error = Tlv::Append<VersionTlv>(aMessage, kThreadVersion);
        break;

    case Tlv::kTimeout:
        VerifyOrExit(!Get<Mle::MleRouter>().IsRxOnWhenIdle());
        error = Tlv::Append<TimeoutTlv>(aMessage, Get<Mle::MleRouter>().GetTimeout());
        break;

    case Tlv::kLeaderData:
    {
        LeaderDataTlv tlv;

        tlv.Init();
        tlv.Set(Get<Mle::MleRouter>().GetLeaderData());
        error = tlv.AppendTo(aMessage);
        break;
    }

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

    case Tlv::kChannelPages:
    {
        ChannelPagesTlv tlv;
        uint8_t         length = 0;

        tlv.Init();

        for (uint8_t page = 0; page < sizeof(Radio::kSupportedChannelPages) * CHAR_BIT; page++)
        {
            if (Radio::kSupportedChannelPages & (1 << page))
            {
                tlv.GetChannelPages()[length++] = page;
            }
        }

        tlv.SetLength(length);
        error = tlv.AppendTo(aMessage);

        break;
    }

#if OPENTHREAD_FTD

    case Tlv::kConnectivity:
    {
        ConnectivityTlv tlv;

        tlv.Init();
        Get<Mle::MleRouter>().FillConnectivityTlv(tlv);
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

    case Tlv::kChildTable:
        error = AppendChildTable(aMessage);
        break;

    case Tlv::kMaxChildTimeout:
    {
        uint32_t maxTimeout;

        SuccessOrExit(Get<Mle::MleRouter>().GetMaxChildTimeout(maxTimeout));
        error = Tlv::Append<MaxChildTimeoutTlv>(aMessage, maxTimeout);
        break;
    }

#endif // OPENTHREAD_FTD

    default:
        break;
    }

exit:
    return error;
}

template <>
void Server::HandleTmf<kUriDiagnosticGetQuery>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error            error    = kErrorNone;
    Coap::Message   *response = nullptr;
    Tmf::MessageInfo responseInfo(GetInstance());

    VerifyOrExit(aMessage.IsPostRequest(), error = kErrorDrop);

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetQuery>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    // DIAG_GET.qry may be sent as a confirmable request.
    if (aMessage.IsConfirmable())
    {
        IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));
    }

    response = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticGetAnswer);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendRequestedTlvs(aMessage, *response));

    PrepareMessageInfoForDest(aMessageInfo.GetPeerAddr(), responseInfo);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, responseInfo));

exit:
    FreeMessageOnError(response, error);
}

template <>
void Server::HandleTmf<kUriDiagnosticGetRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = kErrorDrop);

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticGetRequest>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    response = Get<Tmf::Agent>().NewResponseMessage(aMessage);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendRequestedTlvs(aMessage, *response));
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

template <> void Server::HandleTmf<kUriDiagnosticReset>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t offset = 0;
    uint8_t  type;
    Tlv      tlv;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticReset>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), tlv));

    VerifyOrExit(tlv.GetType() == Tlv::kTypeList);

    offset = aMessage.GetOffset() + sizeof(Tlv);

    for (uint8_t i = 0; i < tlv.GetLength(); i++)
    {
        SuccessOrExit(aMessage.Read(offset + i, type));

        switch (type)
        {
        case Tlv::kMacCounters:
            Get<Mac::Mac>().ResetCounters();
            break;

        default:
            break;
        }
    }

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

exit:
    return;
}

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
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
        error = SendCommand(kUriDiagnosticGetQuery, aDestination, aTlvTypes, aCount);
    }
    else
    {
        error = SendCommand(kUriDiagnosticGetRequest, aDestination, aTlvTypes, aCount, &HandleGetResponse, this);
    }

    SuccessOrExit(error);

    mGetCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error Client::SendCommand(Uri                   aUri,
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

    if (aCount > 0)
    {
        SuccessOrExit(error = Tlv::Append<TypeListTlv>(*message, aTlvTypes, aCount));
    }

    Get<Server>().PrepareMessageInfoForDest(aDestination, messageInfo);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, aHandler, aContext));

    LogInfo("Sent %s to %s", UriToString(aUri), aDestination.ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Client::HandleGetResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, Error aResult)
{
    static_cast<Client *>(aContext)->HandleGetResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                       aResult);
}

void Client::HandleGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    SuccessOrExit(aResult);
    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged, aResult = kErrorFailed);

exit:
    mGetCallback.InvokeIfSet(aResult, aMessage, aMessageInfo);
}

template <>
void Client::HandleTmf<kUriDiagnosticGetAnswer>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    LogInfo("Received %s from %s", ot::UriToString<kUriDiagnosticGetAnswer>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    mGetCallback.InvokeIfSet(kErrorNone, &aMessage, &aMessageInfo);

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

exit:
    return;
}

Error Client::SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount)
{
    return SendCommand(kUriDiagnosticReset, aDestination, aTlvTypes, aCount);
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

static inline void ParseMacCounters(const MacCountersTlv &aMacCountersTlv, otNetworkDiagMacCounters &aMacCounters)
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

Error Client::GetNextDiagTlv(const Coap::Message &aMessage, Iterator &aIterator, TlvInfo &aTlvInfo)
{
    Error    error;
    uint16_t offset = (aIterator == 0) ? aMessage.GetOffset() : aIterator;

    while (offset < aMessage.GetLength())
    {
        bool     skipTlv = false;
        uint16_t valueOffset;
        uint16_t tlvLength;
        union
        {
            Tlv         tlv;
            ExtendedTlv extTlv;
        };

        SuccessOrExit(error = aMessage.Read(offset, tlv));

        if (tlv.IsExtended())
        {
            SuccessOrExit(error = aMessage.Read(offset, extTlv));
            valueOffset = offset + sizeof(ExtendedTlv);
            tlvLength   = extTlv.GetLength();
        }
        else
        {
            valueOffset = offset + sizeof(Tlv);
            tlvLength   = tlv.GetLength();
        }

        VerifyOrExit(offset + tlv.GetSize() <= aMessage.GetLength(), error = kErrorParse);

        switch (tlv.GetType())
        {
        case Tlv::kExtMacAddress:
            SuccessOrExit(error =
                              Tlv::Read<ExtMacAddressTlv>(aMessage, offset, AsCoreType(&aTlvInfo.mData.mExtAddress)));
            break;

        case Tlv::kAddress16:
            SuccessOrExit(error = Tlv::Read<Address16Tlv>(aMessage, offset, aTlvInfo.mData.mAddr16));
            break;

        case Tlv::kMode:
        {
            uint8_t mode;

            SuccessOrExit(error = Tlv::Read<ModeTlv>(aMessage, offset, mode));
            Mle::DeviceMode(mode).Get(aTlvInfo.mData.mMode);
            break;
        }

        case Tlv::kTimeout:
            SuccessOrExit(error = Tlv::Read<TimeoutTlv>(aMessage, offset, aTlvInfo.mData.mTimeout));
            break;

        case Tlv::kConnectivity:
        {
            ConnectivityTlv connectivityTlv;

            VerifyOrExit(!tlv.IsExtended(), error = kErrorParse);
            SuccessOrExit(error = aMessage.Read(offset, connectivityTlv));
            VerifyOrExit(connectivityTlv.IsValid(), error = kErrorParse);
            connectivityTlv.GetConnectivity(aTlvInfo.mData.mConnectivity);
            break;
        }

        case Tlv::kRoute:
        {
            RouteTlv routeTlv;
            uint16_t bytesToRead = static_cast<uint16_t>(Min(tlv.GetSize(), static_cast<uint32_t>(sizeof(routeTlv))));

            VerifyOrExit(!tlv.IsExtended(), error = kErrorParse);
            SuccessOrExit(error = aMessage.Read(offset, &routeTlv, bytesToRead));
            VerifyOrExit(routeTlv.IsValid(), error = kErrorParse);
            ParseRoute(routeTlv, aTlvInfo.mData.mRoute);
            break;
        }

        case Tlv::kLeaderData:
        {
            LeaderDataTlv leaderDataTlv;

            VerifyOrExit(!tlv.IsExtended(), error = kErrorParse);
            SuccessOrExit(error = aMessage.Read(offset, leaderDataTlv));
            VerifyOrExit(leaderDataTlv.IsValid(), error = kErrorParse);
            leaderDataTlv.Get(AsCoreType(&aTlvInfo.mData.mLeaderData));
            break;
        }

        case Tlv::kNetworkData:
            static_assert(sizeof(aTlvInfo.mData.mNetworkData.m8) >= NetworkData::NetworkData::kMaxSize,
                          "NetworkData array in `otNetworkDiagTlv` is too small");

            VerifyOrExit(tlvLength <= NetworkData::NetworkData::kMaxSize, error = kErrorParse);
            aTlvInfo.mData.mNetworkData.mCount = static_cast<uint8_t>(tlvLength);
            aMessage.ReadBytes(valueOffset, aTlvInfo.mData.mNetworkData.m8, tlvLength);
            break;

        case Tlv::kIp6AddressList:
        {
            uint16_t      addrListLength = GetArrayLength(aTlvInfo.mData.mIp6AddrList.mList);
            Ip6::Address *addrEntry      = AsCoreTypePtr(&aTlvInfo.mData.mIp6AddrList.mList[0]);
            uint8_t      &addrCount      = aTlvInfo.mData.mIp6AddrList.mCount;

            VerifyOrExit((tlvLength % Ip6::Address::kSize) == 0, error = kErrorParse);

            // `TlvInfo` has a fixed array for IPv6 addresses. If there
            // are more addresses in the message, we read and return as
            // many as can fit in array and ignore the rest.

            addrCount = 0;

            while ((tlvLength > 0) && (addrCount < addrListLength))
            {
                SuccessOrExit(error = aMessage.Read(valueOffset, *addrEntry));
                addrCount++;
                addrEntry++;
                valueOffset += Ip6::Address::kSize;
                tlvLength -= Ip6::Address::kSize;
            }

            break;
        }

        case Tlv::kMacCounters:
        {
            MacCountersTlv macCountersTlv;

            SuccessOrExit(error = aMessage.Read(offset, macCountersTlv));
            VerifyOrExit(macCountersTlv.IsValid(), error = kErrorParse);
            ParseMacCounters(macCountersTlv, aTlvInfo.mData.mMacCounters);
            break;
        }

        case Tlv::kBatteryLevel:
            SuccessOrExit(error = Tlv::Read<BatteryLevelTlv>(aMessage, offset, aTlvInfo.mData.mBatteryLevel));
            break;

        case Tlv::kSupplyVoltage:
            SuccessOrExit(error = Tlv::Read<SupplyVoltageTlv>(aMessage, offset, aTlvInfo.mData.mSupplyVoltage));
            break;

        case Tlv::kChildTable:
        {
            uint16_t   childInfoLength = GetArrayLength(aTlvInfo.mData.mChildTable.mTable);
            ChildInfo *childInfo       = &aTlvInfo.mData.mChildTable.mTable[0];
            uint8_t   &childCount      = aTlvInfo.mData.mChildTable.mCount;

            VerifyOrExit((tlvLength % sizeof(ChildTableEntry)) == 0, error = kErrorParse);

            // `TlvInfo` has a fixed array Child Table entries. If there
            // are more entries in the message, we read and return as
            // many as can fit in array and ignore the rest.

            childCount = 0;

            while ((tlvLength > 0) && (childCount < childInfoLength))
            {
                ChildTableEntry entry;

                SuccessOrExit(error = aMessage.Read(valueOffset, entry));

                childInfo->mTimeout     = entry.GetTimeout();
                childInfo->mLinkQuality = entry.GetLinkQuality();
                childInfo->mChildId     = entry.GetChildId();
                entry.GetMode().Get(childInfo->mMode);

                childCount++;
                childInfo++;
                tlvLength -= sizeof(ChildTableEntry);
                valueOffset += sizeof(ChildTableEntry);
            }

            break;
        }

        case Tlv::kChannelPages:
            aTlvInfo.mData.mChannelPages.mCount =
                static_cast<uint8_t>(Min(tlvLength, GetArrayLength(aTlvInfo.mData.mChannelPages.m8)));
            aMessage.ReadBytes(valueOffset, aTlvInfo.mData.mChannelPages.m8, aTlvInfo.mData.mChannelPages.mCount);
            break;

        case Tlv::kMaxChildTimeout:
            SuccessOrExit(error = Tlv::Read<MaxChildTimeoutTlv>(aMessage, offset, aTlvInfo.mData.mMaxChildTimeout));
            break;

        case Tlv::kVersion:
            SuccessOrExit(error = Tlv::Read<VersionTlv>(aMessage, offset, aTlvInfo.mData.mVersion));
            break;

        default:
            // Skip unrecognized TLVs.
            skipTlv = true;
            break;
        }

        offset += tlv.GetSize();

        if (!skipTlv)
        {
            // Exit if a TLV is recognized and parsed successfully.
            aTlvInfo.mType = tlv.GetType();
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
