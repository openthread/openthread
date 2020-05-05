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
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "mac/mac.hpp"
#include "net/netif.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

namespace ot {

namespace NetworkDiagnostic {

NetworkDiagnostic::NetworkDiagnostic(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mDiagnosticGetRequest(OT_URI_PATH_DIAGNOSTIC_GET_REQUEST, &NetworkDiagnostic::HandleDiagnosticGetRequest, this)
    , mDiagnosticGetQuery(OT_URI_PATH_DIAGNOSTIC_GET_QUERY, &NetworkDiagnostic::HandleDiagnosticGetQuery, this)
    , mDiagnosticGetAnswer(OT_URI_PATH_DIAGNOSTIC_GET_ANSWER, &NetworkDiagnostic::HandleDiagnosticGetAnswer, this)
    , mDiagnosticReset(OT_URI_PATH_DIAGNOSTIC_RESET, &NetworkDiagnostic::HandleDiagnosticReset, this)
    , mReceiveDiagnosticGetCallback(NULL)
    , mReceiveDiagnosticGetCallbackContext(NULL)
{
    Get<Coap::Coap>().AddResource(mDiagnosticGetRequest);
    Get<Coap::Coap>().AddResource(mDiagnosticGetQuery);
    Get<Coap::Coap>().AddResource(mDiagnosticGetAnswer);
    Get<Coap::Coap>().AddResource(mDiagnosticReset);
}

void NetworkDiagnostic::SetReceiveDiagnosticGetCallback(otReceiveDiagnosticGetCallback aCallback,
                                                        void *                         aCallbackContext)
{
    mReceiveDiagnosticGetCallback        = aCallback;
    mReceiveDiagnosticGetCallbackContext = aCallbackContext;
}

otError NetworkDiagnostic::SendDiagnosticGet(const Ip6::Address &aDestination,
                                             const uint8_t       aTlvTypes[],
                                             uint8_t             aCount)
{
    otError               error;
    Coap::Message *       message = NULL;
    Ip6::MessageInfo      messageInfo;
    otCoapResponseHandler handler = NULL;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    if (aDestination.IsMulticast())
    {
        SuccessOrExit(
            error = message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_DIAGNOSTIC_GET_QUERY));
    }
    else
    {
        handler = &NetworkDiagnostic::HandleDiagnosticGetResponse;
        SuccessOrExit(
            error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_DIAGNOSTIC_GET_REQUEST));
    }

    if (aCount > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    if (aCount > 0)
    {
        TypeListTlv tlv;
        tlv.Init();
        tlv.SetLength(aCount);

        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvTypes, aCount));
    }

    if (aDestination.IsLinkLocal() || aDestination.IsLinkLocalMulticast())
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetLinkLocalAddress());
    }
    else
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo, handler, this));

    otLogInfoNetDiag("Sent diagnostic get");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(void *               aContext,
                                                    otMessage *          aMessage,
                                                    const otMessageInfo *aMessageInfo,
                                                    otError              aResult)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(Coap::Message *         aMessage,
                                                    const Ip6::MessageInfo *aMessageInfo,
                                                    otError                 aResult)
{
    VerifyOrExit(aResult == OT_ERROR_NONE, OT_NOOP);
    VerifyOrExit(aMessage && aMessage->GetCode() == OT_COAP_CODE_CHANGED, OT_NOOP);

    otLogInfoNetDiag("Received diagnostic get response");

    if (mReceiveDiagnosticGetCallback)
    {
        mReceiveDiagnosticGetCallback(aMessage, aMessageInfo, mReceiveDiagnosticGetCallbackContext);
    }

exit:
    return;
}

void NetworkDiagnostic::HandleDiagnosticGetAnswer(void *               aContext,
                                                  otMessage *          aMessage,
                                                  const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetAnswer(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aMessage.IsConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, OT_NOOP);

    otLogInfoNetDiag("Diagnostic get answer received");

    if (mReceiveDiagnosticGetCallback)
    {
        mReceiveDiagnosticGetCallback(&aMessage, &aMessageInfo, mReceiveDiagnosticGetCallbackContext);
    }

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    otLogInfoNetDiag("Sent diagnostic answer acknowledgment");

exit:
    return;
}

otError NetworkDiagnostic::AppendIp6AddressList(Message &aMessage)
{
    otError           error = OT_ERROR_NONE;
    Ip6AddressListTlv tlv;
    uint8_t           count = 0;

    tlv.Init();

    for (const Ip6::NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        count++;
    }

    tlv.SetLength(count * sizeof(Ip6::Address));
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    for (const Ip6::NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        SuccessOrExit(error = aMessage.Append(&addr->GetAddress(), sizeof(Ip6::Address)));
    }

exit:

    return error;
}

#if OPENTHREAD_FTD
otError NetworkDiagnostic::AppendChildTable(Message &aMessage)
{
    otError         error   = OT_ERROR_NONE;
    uint16_t        count   = 0;
    uint8_t         timeout = 0;
    ChildTableTlv   tlv;
    ChildTableEntry entry;

    tlv.Init();

    count = Get<ChildTable>().GetNumChildren(Child::kInStateValid);

    // The length of the Child Table TLV may exceed the outgoing link's MTU (1280B).
    // As a workaround we limit the number of entries in the Child Table TLV,
    // also to avoid using extended TLV format. The issue is processed by the
    // Thread Group (SPEC-894).
    if (count > (Tlv::kBaseTlvMaxLength / sizeof(ChildTableEntry)))
    {
        count = Tlv::kBaseTlvMaxLength / sizeof(ChildTableEntry);
    }

    tlv.SetLength(static_cast<uint8_t>(count * sizeof(ChildTableEntry)));

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(ChildTableTlv)));

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        VerifyOrExit(count--, OT_NOOP);

        Child &child = *iter.GetChild();

        timeout = 0;

        while (static_cast<uint32_t>(1 << timeout) < child.GetTimeout())
        {
            timeout++;
        }

        entry.SetReserved(0);
        entry.SetTimeout(timeout + 4);

        entry.SetChildId(Mle::Mle::ChildIdFromRloc16(child.GetRloc16()));
        entry.SetMode(child.GetDeviceMode());

        SuccessOrExit(error = aMessage.Append(&entry, sizeof(ChildTableEntry)));
    }

exit:

    return error;
}
#endif // OPENTHREAD_FTD

void NetworkDiagnostic::FillMacCountersTlv(MacCountersTlv &aMacCountersTlv)
{
    const otMacCounters &macCounters = Get<Mac::Mac>().GetCounters();

    aMacCountersTlv.SetIfInUnknownProtos(macCounters.mRxOther);
    aMacCountersTlv.SetIfInErrors(macCounters.mRxErrNoFrame + macCounters.mRxErrUnknownNeighbor +
                                  macCounters.mRxErrInvalidSrcAddr + macCounters.mRxErrSec + macCounters.mRxErrFcs +
                                  macCounters.mRxErrOther);
    aMacCountersTlv.SetIfOutErrors(macCounters.mTxErrCca);
    aMacCountersTlv.SetIfInUcastPkts(macCounters.mRxUnicast);
    aMacCountersTlv.SetIfInBroadcastPkts(macCounters.mRxBroadcast);
    aMacCountersTlv.SetIfInDiscards(macCounters.mRxAddressFiltered + macCounters.mRxDestAddrFiltered +
                                    macCounters.mRxDuplicated);
    aMacCountersTlv.SetIfOutUcastPkts(macCounters.mTxUnicast);
    aMacCountersTlv.SetIfOutBroadcastPkts(macCounters.mTxBroadcast);
    aMacCountersTlv.SetIfOutDiscards(macCounters.mTxErrBusyChannel);
}

otError NetworkDiagnostic::FillRequestedTlvs(Message &             aRequest,
                                             Message &             aResponse,
                                             NetworkDiagnosticTlv &aNetworkDiagnosticTlv)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t offset = 0;
    uint8_t  type;

    offset = aRequest.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint32_t i = 0; i < aNetworkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aRequest.Read(offset, sizeof(type), &type) == sizeof(type), error = OT_ERROR_PARSE);

        otLogInfoNetDiag("Type %d", type);

        switch (type)
        {
        case NetworkDiagnosticTlv::kExtMacAddress:
        {
            ExtMacAddressTlv tlv;
            tlv.Init();
            tlv.SetMacAddr(Get<Mac::Mac>().GetExtAddress());
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kAddress16:
        {
            Address16Tlv tlv;
            tlv.Init();
            tlv.SetRloc16(Get<Mle::MleRouter>().GetRloc16());
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kMode:
        {
            ModeTlv tlv;
            tlv.Init();
            tlv.SetMode(Get<Mle::MleRouter>().GetDeviceMode());
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kTimeout:
        {
            if (!Get<Mle::MleRouter>().IsRxOnWhenIdle())
            {
                TimeoutTlv tlv;
                tlv.Init();
                tlv.SetTimeout(Get<Mle::MleRouter>().GetTimeout());
                SuccessOrExit(error = tlv.AppendTo(aResponse));
            }

            break;
        }

#if OPENTHREAD_FTD
        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv tlv;
            tlv.Init();
            Get<Mle::MleRouter>().FillConnectivityTlv(reinterpret_cast<Mle::ConnectivityTlv &>(tlv));
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kRoute:
        {
            RouteTlv tlv;
            tlv.Init();
            Get<Mle::MleRouter>().FillRouteTlv(reinterpret_cast<Mle::RouteTlv &>(tlv));
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }
#endif

        case NetworkDiagnosticTlv::kLeaderData:
        {
            LeaderDataTlv          tlv;
            const Mle::LeaderData &leaderData = Get<Mle::MleRouter>().GetLeaderData();

            tlv.Init();
            tlv.SetPartitionId(leaderData.GetPartitionId());
            tlv.SetWeighting(leaderData.GetWeighting());
            tlv.SetDataVersion(leaderData.GetDataVersion());
            tlv.SetStableDataVersion(leaderData.GetStableDataVersion());
            tlv.SetLeaderRouterId(leaderData.GetLeaderRouterId());

            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv tlv;
            uint8_t        length;

            tlv.Init();

            length = sizeof(NetworkDataTlv) - sizeof(Tlv); // sizeof( NetworkDataTlv::mNetworkData )
            Get<NetworkData::Leader>().GetNetworkData(/* aStableOnly */ false, tlv.GetNetworkData(), length);
            tlv.SetLength(length);

            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kIp6AddressList:
        {
            SuccessOrExit(error = AppendIp6AddressList(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kMacCounters:
        {
            MacCountersTlv tlv;
            memset(&tlv, 0, sizeof(tlv));
            tlv.Init();
            FillMacCountersTlv(tlv);
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kBatteryLevel:
        {
            // Thread 1.1.1 Specification Section 10.11.4.2:
            // Omitted if the battery level is not measured, is unknown or the device does not
            // operate on battery power.
            break;
        }

        case NetworkDiagnosticTlv::kSupplyVoltage:
        {
            // Thread 1.1.1 Specification Section 10.11.4.3:
            // Omitted if the supply voltage is not measured, is unknown.
            break;
        }

#if OPENTHREAD_FTD
        case NetworkDiagnosticTlv::kChildTable:
        {
            // Thread 1.1.1 Specification Section 10.11.2.2:
            // If a Thread device is unable to supply a specific Diagnostic TLV, that TLV is omitted.
            // Here only Leader or Router may have children.
            if (Get<Mle::MleRouter>().IsRouterOrLeader())
            {
                SuccessOrExit(error = AppendChildTable(aResponse));
            }
            break;
        }
#endif

        case NetworkDiagnosticTlv::kChannelPages:
        {
            uint8_t         length   = 0;
            uint8_t         pageMask = Radio::kSupportedChannelPages;
            ChannelPagesTlv tlv;

            tlv.Init();
            for (uint8_t page = 0; page < sizeof(pageMask) * 8; page++)
            {
                if (pageMask & (1 << page))
                {
                    tlv.GetChannelPages()[length++] = page;
                }
            }

            tlv.SetLength(length);
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

#if OPENTHREAD_FTD
        case NetworkDiagnosticTlv::kMaxChildTimeout:
        {
            uint32_t maxTimeout = 0;

            if (Get<Mle::MleRouter>().GetMaxChildTimeout(maxTimeout) == OT_ERROR_NONE)
            {
                MaxChildTimeoutTlv tlv;
                tlv.Init();
                tlv.SetTimeout(maxTimeout);
                SuccessOrExit(error = tlv.AppendTo(aResponse));
            }

            break;
        }
#endif

        default:
            ExitNow(error = OT_ERROR_PARSE);
        }

        offset += sizeof(type);
    }

exit:
    return error;
}

void NetworkDiagnostic::HandleDiagnosticGetQuery(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetQuery(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetQuery(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError              error   = OT_ERROR_NONE;
    Coap::Message *      message = NULL;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Ip6::MessageInfo     messageInfo;

    VerifyOrExit(aMessage.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoNetDiag("Received diagnostic get query");

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv), &networkDiagnosticTlv) ==
                  sizeof(NetworkDiagnosticTlv)),
                 error = OT_ERROR_PARSE);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = OT_ERROR_PARSE);

    // DIAG_GET.qry may be sent as a confirmable message.
    if (aMessage.IsConfirmable())
    {
        if (Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoNetDiag("Sent diagnostic get query acknowledgment");
        }
    }

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error =
                      message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_DIAGNOSTIC_GET_ANSWER));

    if (networkDiagnosticTlv.GetLength() > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    if (aMessageInfo.GetSockAddr().IsLinkLocal() || aMessageInfo.GetSockAddr().IsLinkLocalMulticast())
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetLinkLocalAddress());
    }
    else
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    }

    messageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = FillRequestedTlvs(aMessage, *message, networkDiagnosticTlv));

    if (message->GetLength() == message->GetOffset())
    {
        // Remove Payload Marker if payload is actually empty.
        message->SetLength(message->GetLength() - 1);
    }

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo, NULL, this));

    otLogInfoNetDiag("Sent diagnostic get answer");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void NetworkDiagnostic::HandleDiagnosticGetRequest(void *               aContext,
                                                   otMessage *          aMessage,
                                                   const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetRequest(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetRequest(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError              error   = OT_ERROR_NONE;
    Coap::Message *      message = NULL;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Ip6::MessageInfo     messageInfo(aMessageInfo);

    VerifyOrExit(aMessage.IsConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoNetDiag("Received diagnostic get request");

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv), &networkDiagnosticTlv) ==
                  sizeof(NetworkDiagnosticTlv)),
                 error = OT_ERROR_PARSE);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = OT_ERROR_PARSE);

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aMessage));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = FillRequestedTlvs(aMessage, *message, networkDiagnosticTlv));

    if (message->GetLength() == message->GetOffset())
    {
        // Remove Payload Marker if payload is actually empty.
        message->SetLength(message->GetOffset() - 1);
    }

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic get response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError NetworkDiagnostic::SendDiagnosticReset(const Ip6::Address &aDestination,
                                               const uint8_t       aTlvTypes[],
                                               uint8_t             aCount)
{
    otError          error;
    Coap::Message *  message = NULL;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_DIAGNOSTIC_RESET));

    if (aCount > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    if (aCount > 0)
    {
        TypeListTlv tlv;
        tlv.Init();
        tlv.SetLength(aCount);

        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvTypes, aCount));
    }

    if (aDestination.IsLinkLocal() || aDestination.IsLinkLocalMulticast())
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetLinkLocalAddress());
    }
    else
    {
        messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent network diagnostic reset");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticReset(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticReset(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticReset(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t             offset = 0;
    uint8_t              type;
    NetworkDiagnosticTlv networkDiagnosticTlv;

    otLogInfoNetDiag("Received diagnostic reset request");

    VerifyOrExit(aMessage.IsConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, OT_NOOP);

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv), &networkDiagnosticTlv) ==
                  sizeof(NetworkDiagnosticTlv)),
                 OT_NOOP);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, OT_NOOP);

    offset = aMessage.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint8_t i = 0; i < networkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aMessage.Read(offset, sizeof(type), &type) == sizeof(type), OT_NOOP);

        switch (type)
        {
        case NetworkDiagnosticTlv::kMacCounters:
            Get<Mac::Mac>().ResetCounters();
            otLogInfoNetDiag("Received diagnostic reset type kMacCounters(9)");
            break;

        default:
            otLogInfoNetDiag("Received diagnostic reset other type %d not resetable", type);
            break;
        }
    }

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    otLogInfoNetDiag("Sent diagnostic reset acknowledgment");

exit:
    return;
}

static inline void ParseMode(const Mle::DeviceMode &aMode, otLinkModeConfig &aLinkModeConfig)
{
    aLinkModeConfig.mRxOnWhenIdle       = aMode.IsRxOnWhenIdle();
    aLinkModeConfig.mSecureDataRequests = aMode.IsSecureDataRequest();
    aLinkModeConfig.mDeviceType         = aMode.IsFullThreadDevice();
    aLinkModeConfig.mNetworkData        = aMode.IsFullNetworkData();
}

static inline void ParseConnectivity(const ConnectivityTlv &    aConnectivityTlv,
                                     otNetworkDiagConnectivity &aNetworkDiagConnectivity)
{
    aNetworkDiagConnectivity.mParentPriority   = aConnectivityTlv.GetParentPriority();
    aNetworkDiagConnectivity.mLinkQuality3     = aConnectivityTlv.GetLinkQuality3();
    aNetworkDiagConnectivity.mLinkQuality2     = aConnectivityTlv.GetLinkQuality2();
    aNetworkDiagConnectivity.mLinkQuality1     = aConnectivityTlv.GetLinkQuality1();
    aNetworkDiagConnectivity.mLeaderCost       = aConnectivityTlv.GetLeaderCost();
    aNetworkDiagConnectivity.mIdSequence       = aConnectivityTlv.GetIdSequence();
    aNetworkDiagConnectivity.mActiveRouters    = aConnectivityTlv.GetActiveRouters();
    aNetworkDiagConnectivity.mSedBufferSize    = aConnectivityTlv.GetSedBufferSize();
    aNetworkDiagConnectivity.mSedDatagramCount = aConnectivityTlv.GetSedDatagramCount();
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

static inline void ParseLeaderData(const LeaderDataTlv &aLeaderDataTlv, otLeaderData &aLeaderData)
{
    aLeaderData.mPartitionId       = aLeaderDataTlv.GetPartitionId();
    aLeaderData.mWeighting         = aLeaderDataTlv.GetWeighting();
    aLeaderData.mDataVersion       = aLeaderDataTlv.GetDataVersion();
    aLeaderData.mStableDataVersion = aLeaderDataTlv.GetStableDataVersion();
    aLeaderData.mLeaderRouterId    = aLeaderDataTlv.GetLeaderRouterId();
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

static inline void ParseChildEntry(const ChildTableEntry &aChildTableTlvEntry, otNetworkDiagChildEntry &aChildEntry)
{
    aChildEntry.mTimeout = aChildTableTlvEntry.GetTimeout();
    aChildEntry.mChildId = aChildTableTlvEntry.GetChildId();
    ParseMode(aChildTableTlvEntry.GetMode(), aChildEntry.mMode);
}

otError NetworkDiagnostic::GetNextDiagTlv(const otMessage &      aMessage,
                                          otNetworkDiagIterator &aIterator,
                                          otNetworkDiagTlv &     aNetworkDiagTlv)
{
    otError              error   = OT_ERROR_PARSE;
    const Coap::Message &message = static_cast<const Coap::Message &>(aMessage);
    uint16_t             offset  = message.GetOffset();
    NetworkDiagnosticTlv tlv;

    offset += aIterator;

    while (true)
    {
        uint16_t tlvTotalLength;

        VerifyOrExit(message.Read(offset, sizeof(tlv), &tlv) == sizeof(tlv), error = OT_ERROR_NOT_FOUND);

        switch (tlv.GetType())
        {
        case NetworkDiagnosticTlv::kExtMacAddress:
        {
            ExtMacAddressTlv extMacAddr;

            tlvTotalLength = sizeof(extMacAddr);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &extMacAddr) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(extMacAddr.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mExtAddress = *extMacAddr.GetMacAddr();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kAddress16:
        {
            Address16Tlv addr16;

            tlvTotalLength = sizeof(addr16);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &addr16) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(addr16.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mAddr16 = addr16.GetRloc16();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kMode:
        {
            ModeTlv linkMode;

            tlvTotalLength = sizeof(linkMode);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &linkMode) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(linkMode.IsValid(), OT_NOOP);

            ParseMode(linkMode.GetMode(), aNetworkDiagTlv.mData.mMode);
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kTimeout:
        {
            TimeoutTlv timeout;

            tlvTotalLength = sizeof(timeout);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &timeout) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(timeout.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mTimeout = timeout.GetTimeout();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv connectivity;

            tlvTotalLength = sizeof(connectivity);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &connectivity) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(connectivity.IsValid(), OT_NOOP);

            ParseConnectivity(connectivity, aNetworkDiagTlv.mData.mConnectivity);
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kRoute:
        {
            RouteTlv route;

            tlvTotalLength = sizeof(tlv) + tlv.GetLength();
            VerifyOrExit(tlvTotalLength <= sizeof(route), OT_NOOP);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &route) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(route.IsValid(), OT_NOOP);

            ParseRoute(route, aNetworkDiagTlv.mData.mRoute);
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kLeaderData:
        {
            LeaderDataTlv leaderData;

            tlvTotalLength = sizeof(leaderData);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &leaderData) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(leaderData.IsValid(), OT_NOOP);

            ParseLeaderData(leaderData, aNetworkDiagTlv.mData.mLeaderData);
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv networkData;

            tlvTotalLength = sizeof(tlv) + tlv.GetLength();
            VerifyOrExit(tlvTotalLength <= sizeof(networkData), OT_NOOP);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &networkData) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(networkData.IsValid(), OT_NOOP);
            VerifyOrExit(sizeof(aNetworkDiagTlv.mData.mNetworkData.m8) >= networkData.GetLength(), OT_NOOP);

            memcpy(aNetworkDiagTlv.mData.mNetworkData.m8, networkData.GetNetworkData(), networkData.GetLength());
            aNetworkDiagTlv.mData.mNetworkData.mCount = networkData.GetLength();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kIp6AddressList:
        {
            Ip6AddressListTlv &ip6AddrList = static_cast<Ip6AddressListTlv &>(tlv);

            VerifyOrExit(ip6AddrList.IsValid(), OT_NOOP);
            VerifyOrExit(sizeof(aNetworkDiagTlv.mData.mIp6AddrList.mList) >= ip6AddrList.GetLength(), OT_NOOP);
            VerifyOrExit(message.Read(offset + sizeof(ip6AddrList), ip6AddrList.GetLength(),
                                      aNetworkDiagTlv.mData.mIp6AddrList.mList) == ip6AddrList.GetLength(),
                         OT_NOOP);

            aNetworkDiagTlv.mData.mIp6AddrList.mCount = ip6AddrList.GetLength() / OT_IP6_ADDRESS_SIZE;
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kMacCounters:
        {
            MacCountersTlv macCounters;

            tlvTotalLength = sizeof(MacCountersTlv);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &macCounters) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(macCounters.IsValid(), OT_NOOP);

            ParseMacCounters(macCounters, aNetworkDiagTlv.mData.mMacCounters);
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kBatteryLevel:
        {
            BatteryLevelTlv batteryLevel;

            tlvTotalLength = sizeof(BatteryLevelTlv);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &batteryLevel) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(batteryLevel.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mBatteryLevel = batteryLevel.GetBatteryLevel();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kSupplyVoltage:
        {
            SupplyVoltageTlv supplyVoltage;

            tlvTotalLength = sizeof(SupplyVoltageTlv);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &supplyVoltage) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(supplyVoltage.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mSupplyVoltage = supplyVoltage.GetSupplyVoltage();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kChildTable:
        {
            ChildTableTlv &childTable = static_cast<ChildTableTlv &>(tlv);

            VerifyOrExit(childTable.IsValid(), OT_NOOP);
            VerifyOrExit(childTable.GetNumEntries() <= OT_ARRAY_LENGTH(aNetworkDiagTlv.mData.mChildTable.mTable),
                         OT_NOOP);

            for (uint8_t i = 0; i < childTable.GetNumEntries(); ++i)
            {
                ChildTableEntry childEntry;
                VerifyOrExit(childTable.ReadEntry(childEntry, message, offset, i) == OT_ERROR_NONE, OT_NOOP);
                ParseChildEntry(childEntry, aNetworkDiagTlv.mData.mChildTable.mTable[i]);
            }
            aNetworkDiagTlv.mData.mChildTable.mCount = childTable.GetNumEntries();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kChannelPages:
        {
            VerifyOrExit(sizeof(aNetworkDiagTlv.mData.mChannelPages.m8) >= tlv.GetLength(), OT_NOOP);
            VerifyOrExit(message.Read(offset + sizeof(tlv), tlv.GetLength(), aNetworkDiagTlv.mData.mChannelPages.m8) ==
                             tlv.GetLength(),
                         OT_NOOP);

            aNetworkDiagTlv.mData.mChannelPages.mCount = tlv.GetLength();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        case NetworkDiagnosticTlv::kMaxChildTimeout:
        {
            MaxChildTimeoutTlv maxChildTimeout;

            tlvTotalLength = sizeof(maxChildTimeout);
            VerifyOrExit(message.Read(offset, tlvTotalLength, &maxChildTimeout) == tlvTotalLength, OT_NOOP);
            VerifyOrExit(maxChildTimeout.IsValid(), OT_NOOP);

            aNetworkDiagTlv.mData.mMaxChildTimeout = maxChildTimeout.GetTimeout();
            ExitNow(error = OT_ERROR_NONE);
            OT_UNREACHABLE_CODE(break);
        }

        default:
            // Ignore unrecognized Network Diagnostic TLV silently.
            break;
        }

        // The actual TLV size may exceeds tlvTotalLength.
        offset += tlv.GetSize();
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        aNetworkDiagTlv.mType = tlv.GetType();
        aIterator             = static_cast<uint16_t>(offset - message.GetOffset() + tlv.GetSize());
    }
    return error;
}

} // namespace NetworkDiagnostic

} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
