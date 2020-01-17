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
    VerifyOrExit(aResult == OT_ERROR_NONE);
    VerifyOrExit(aMessage && aMessage->GetCode() == OT_COAP_CODE_CHANGED);

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
    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

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
        VerifyOrExit(count--);

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

        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv tlv;
            tlv.Init();
            Get<Mle::MleRouter>().FillConnectivityTlv(reinterpret_cast<Mle::ConnectivityTlv &>(tlv));
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

#if OPENTHREAD_FTD
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
            LeaderDataTlv tlv(reinterpret_cast<const LeaderDataTlv &>(Get<Mle::MleRouter>().GetLeaderDataTlv()));
            tlv.Init();
            SuccessOrExit(error = tlv.AppendTo(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv tlv;
            tlv.Init();

            Get<Mle::MleRouter>().FillNetworkDataTlv((reinterpret_cast<Mle::NetworkDataTlv &>(tlv)), false);
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

        case NetworkDiagnosticTlv::kChildTable:
        {
            // Thread 1.1.1 Specification Section 10.11.2.2:
            // If a Thread device is unable to supply a specific Diagnostic TLV, that TLV is omitted.
            // Here only Leader or Router may have children.
            if (Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER ||
                Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_ROUTER)
            {
                SuccessOrExit(error = AppendChildTable(aResponse));
            }
            break;
        }

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
    if (aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE)
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

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_DROP);

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

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv), &networkDiagnosticTlv) ==
                  sizeof(NetworkDiagnosticTlv)));

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList);

    offset = aMessage.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint8_t i = 0; i < networkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aMessage.Read(offset, sizeof(type), &type) == sizeof(type));

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

} // namespace NetworkDiagnostic

} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
