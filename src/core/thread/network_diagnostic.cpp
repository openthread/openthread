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

#define WPP_NAME "network_diagnostic.tmh"

#include <openthread/config.h>

#include "network_diagnostic.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "common/encoding.hpp"
#include "mac/mac_frame.hpp"
#include "net/netif.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"
#include "thread/network_diagnostic_tlvs.hpp"

using ot::Encoding::BigEndian::HostSwap16;

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

namespace ot {

namespace NetworkDiagnostic {

NetworkDiagnostic::NetworkDiagnostic(ThreadNetif &aThreadNetif) :
    mDiagnosticGetRequest(OT_URI_PATH_DIAGNOSTIC_GET_REQUEST, &NetworkDiagnostic::HandleDiagnosticGetRequest, this),
    mDiagnosticGetQuery(OT_URI_PATH_DIAGNOSTIC_GET_QUERY, &NetworkDiagnostic::HandleDiagnosticGetQuery, this),
    mDiagnosticGetAnswer(OT_URI_PATH_DIAGNOSTIC_GET_ANSWER, &NetworkDiagnostic::HandleDiagnosticGetAnswer, this),
    mDiagnosticReset(OT_URI_PATH_DIAGNOSTIC_RESET, &NetworkDiagnostic::HandleDiagnosticReset, this),
    mNetif(aThreadNetif),
    mReceiveDiagnosticGetCallback(NULL),
    mReceiveDiagnosticGetCallbackContext(NULL)
{
    mNetif.GetCoap().AddResource(mDiagnosticGetRequest);
    mNetif.GetCoap().AddResource(mDiagnosticGetQuery);
    mNetif.GetCoap().AddResource(mDiagnosticGetAnswer);
    mNetif.GetCoap().AddResource(mDiagnosticReset);
}

otInstance *NetworkDiagnostic::GetInstance(void)
{
    return mNetif.GetInstance();
}

void NetworkDiagnostic::SetReceiveDiagnosticGetCallback(otReceiveDiagnosticGetCallback aCallback,
                                                        void *aCallbackContext)
{
    mReceiveDiagnosticGetCallback = aCallback;
    mReceiveDiagnosticGetCallbackContext = aCallbackContext;
}

otError NetworkDiagnostic::SendDiagnosticGet(const Ip6::Address &aDestination, const uint8_t aTlvTypes[],
                                             uint8_t aCount)
{
    otError error;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;
    otCoapResponseHandler handler = NULL;

    if (aDestination.IsMulticast())
    {
        header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
        header.SetToken(Coap::Header::kDefaultTokenLength);
        header.AppendUriPathOptions(OT_URI_PATH_DIAGNOSTIC_GET_QUERY);
    }
    else
    {
        handler = &NetworkDiagnostic::HandleDiagnosticGetResponse;
        header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
        header.SetToken(Coap::Header::kDefaultTokenLength);
        header.AppendUriPathOptions(OT_URI_PATH_DIAGNOSTIC_GET_REQUEST);
    }

    if (aCount > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = mNetif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo, handler, this));

    otLogInfoNetDiag(GetInstance(), "Sent diagnostic get");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                    const otMessageInfo *aMessageInfo, otError aResult)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetResponse(*static_cast<Coap::Header *>(aHeader),
                                                                            *static_cast<Message *>(aMessage),
                                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                            aResult);
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(Coap::Header &aHeader, Message &aMessage,
                                                    const Ip6::MessageInfo &aMessageInfo,
                                                    otError aResult)
{
    VerifyOrExit(aResult == OT_ERROR_NONE);
    VerifyOrExit(aHeader.GetCode() == OT_COAP_CODE_CHANGED);

    otLogInfoNetDiag(GetInstance(), "Received diagnostic get response");

    if (mReceiveDiagnosticGetCallback)
    {
        mReceiveDiagnosticGetCallback(&aMessage, &aMessageInfo, mReceiveDiagnosticGetCallbackContext);
    }

exit:
    return;
}

void NetworkDiagnostic::HandleDiagnosticGetAnswer(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                  const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetAnswer(*static_cast<Coap::Header *>(aHeader),
                                                                          *static_cast<Message *>(aMessage),
                                                                          *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetAnswer(Coap::Header &aHeader, Message &aMessage,
                                                  const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    otLogInfoNetDiag(GetInstance(), "Diagnostic get answer received");

    if (mReceiveDiagnosticGetCallback)
    {
        mReceiveDiagnosticGetCallback(&aMessage, &aMessageInfo, mReceiveDiagnosticGetCallbackContext);
    }

    SuccessOrExit(mNetif.GetCoap().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoNetDiag(GetInstance(), "Sent diagnostic answer acknowledgment");

exit:
    return;
}

otError NetworkDiagnostic::AppendIp6AddressList(Message &aMessage)
{
    otError error = OT_ERROR_NONE;
    Ip6AddressListTlv tlv;
    uint8_t count = 0;

    tlv.Init();

    for (const Ip6::NetifUnicastAddress *addr = mNetif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        count++;
    }

    tlv.SetLength(count * sizeof(Ip6::Address));
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    for (const Ip6::NetifUnicastAddress *addr = mNetif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        SuccessOrExit(error = aMessage.Append(&addr->GetAddress(), sizeof(Ip6::Address)));
    }

exit:

    return error;
}

otError NetworkDiagnostic::AppendChildTable(Message &aMessage)
{
    otError error = OT_ERROR_NONE;
    uint8_t count = 0;
    uint8_t timeout = 0;
    uint8_t numChildren;
    const Child *children = mNetif.GetMle().GetChildren(&numChildren);
    ChildTableTlv tlv;
    ChildTableEntry entry;

    tlv.Init();

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].GetState() == Neighbor::kStateValid)
        {
            count++;
        }
    }

    tlv.SetLength(count * sizeof(ChildTableEntry));

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(ChildTableTlv)));

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].GetState() == Neighbor::kStateValid)
        {
            timeout = 0;

            while (static_cast<uint32_t>(1 << timeout) < children[i].GetTimeout()) { timeout++; }

            entry.SetReserved(0);
            entry.SetTimeout(timeout + 4);
            entry.SetChildId(mNetif.GetMle().GetChildId(children[i].GetRloc16()));
            entry.SetMode(children[i].GetDeviceMode());

            SuccessOrExit(error = aMessage.Append(&entry, sizeof(ChildTableEntry)));
        }
    }

exit:

    return error;
}

otError NetworkDiagnostic::FillRequestedTlvs(Message &aRequest, Message &aResponse,
                                             NetworkDiagnosticTlv &aNetworkDiagnosticTlv)
{
    otError error = OT_ERROR_NONE;
    uint16_t offset = 0;
    uint8_t type;

    offset = aRequest.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint32_t i = 0; i < aNetworkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aRequest.Read(offset, sizeof(type), &type) == sizeof(type), error = OT_ERROR_DROP);

        otLogInfoNetDiag(GetInstance(), "Received diagnostic get type %d", type);

        switch (type)
        {
        case NetworkDiagnosticTlv::kExtMacAddress:
        {
            ExtMacAddressTlv tlv;
            tlv.Init();
            tlv.SetMacAddr(*mNetif.GetMac().GetExtAddress());
            SuccessOrExit(error = aResponse.Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kAddress16:
        {
            Address16Tlv tlv;
            tlv.Init();
            tlv.SetRloc16(mNetif.GetMle().GetRloc16());
            SuccessOrExit(error = aResponse.Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kMode:
        {
            ModeTlv tlv;
            tlv.Init();
            tlv.SetMode(mNetif.GetMle().GetDeviceMode());
            SuccessOrExit(error = aResponse.Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kTimeout:
        {
            if ((mNetif.GetMle().GetDeviceMode() & ModeTlv::kModeRxOnWhenIdle) == 0)
            {
                TimeoutTlv tlv;
                tlv.Init();
                tlv.SetTimeout(mNetif.GetMle().GetTimeout());
                SuccessOrExit(error = aResponse.Append(&tlv, sizeof(tlv)));
            }

            break;
        }

        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv tlv;
            tlv.Init();
            mNetif.GetMle().FillConnectivityTlv(*reinterpret_cast<Mle::ConnectivityTlv *>(&tlv));
            SuccessOrExit(error = aResponse.Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kRoute:
        {
            RouteTlv tlv;
            tlv.Init();
            mNetif.GetMle().FillRouteTlv(*reinterpret_cast<Mle::RouteTlv *>(&tlv));
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kLeaderData:
        {
            LeaderDataTlv tlv;
            memcpy(&tlv, &mNetif.GetMle().GetLeaderDataTlv(), sizeof(tlv));
            tlv.Init();
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv tlv;
            tlv.Init();
            mNetif.GetMle().FillNetworkDataTlv((*reinterpret_cast<Mle::NetworkDataTlv *>(&tlv)), true);
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
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
            mNetif.GetMac().FillMacCountersTlv(tlv);
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kBatteryLevel:
        {
            // TODO Need more api from driver
            BatteryLevelTlv tlv;
            tlv.Init();
            tlv.SetBatteryLevel(100);
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kSupplyVoltage:
        {
            // TODO Need more api from driver
            SupplyVoltageTlv tlv;
            tlv.Init();
            tlv.SetSupplyVoltage(0);
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kChildTable:
        {
            SuccessOrExit(error = AppendChildTable(aResponse));
            break;
        }

        case NetworkDiagnosticTlv::kChannelPages:
        {
            ChannelPagesTlv tlv;
            tlv.Init();
            tlv.GetChannelPages()[0] = 0;
            tlv.SetLength(1);
            SuccessOrExit(error = aResponse.Append(&tlv, tlv.GetSize()));
            break;
        }

        default:
            ExitNow(error = OT_ERROR_DROP);
        }

        offset += sizeof(type);
    }

exit:
    return error;
}

void NetworkDiagnostic::HandleDiagnosticGetQuery(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                 const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetQuery(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetQuery(Coap::Header &aHeader, Message &aMessage,
                                                 const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Message *message = NULL;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(aHeader.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoNetDiag(GetInstance(), "Received diagnostic get query");

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv),
                                &networkDiagnosticTlv) == sizeof(NetworkDiagnosticTlv)), error = OT_ERROR_DROP);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = OT_ERROR_DROP);

    VerifyOrExit((static_cast<TypeListTlv *>(&networkDiagnosticTlv)->IsValid()), error = OT_ERROR_DROP);

    // DIAG_GET.qry may be sent as a confirmable message.
    if (aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE)
    {
        if (mNetif.GetCoap().SendEmptyAck(aHeader, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoNetDiag(GetInstance(), "Sent diagnostic get query acknowledgment");
        }
    }

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_DIAGNOSTIC_GET_ANSWER);

    if (networkDiagnosticTlv.GetLength() > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = mNetif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    messageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = FillRequestedTlvs(aMessage, *message, networkDiagnosticTlv));

    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo, NULL, this));

    otLogInfoNetDiag(GetInstance(), "Sent diagnostic get answer");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void NetworkDiagnostic::HandleDiagnosticGetRequest(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                   const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetRequest(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticGetRequest(Coap::Header &aHeader, Message &aMessage,
                                                   const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Message *message = NULL;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Coap::Header header;
    Ip6::MessageInfo messageInfo(aMessageInfo);

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoNetDiag(GetInstance(), "Received diagnostic get request");

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv),
                                &networkDiagnosticTlv) == sizeof(NetworkDiagnosticTlv)), error = OT_ERROR_DROP);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = OT_ERROR_DROP);

    VerifyOrExit((static_cast<TypeListTlv *>(&networkDiagnosticTlv)->IsValid()), error = OT_ERROR_DROP);

    header.SetDefaultResponseHeader(aHeader);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = FillRequestedTlvs(aMessage, *message, networkDiagnosticTlv));

    if (message->GetLength() == header.GetLength())
    {
        // Remove Payload Marker if payload is actually empty.
        message->SetLength(header.GetLength() - 1);
    }

    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoNetDiag(GetInstance(), "Sent diagnostic get response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError NetworkDiagnostic::SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[],
                                               uint8_t aCount)
{
    otError error;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_DIAGNOSTIC_RESET);

    if (aCount > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = mNetif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoNetDiag(GetInstance(), "Sent network diagnostic reset");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticReset(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                              const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticReset(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticReset(Coap::Header &aHeader, Message &aMessage,
                                              const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t offset = 0;
    uint8_t type;
    NetworkDiagnosticTlv networkDiagnosticTlv;

    otLogInfoNetDiag(GetInstance(), "Received diagnostic reset request");

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv),
                                &networkDiagnosticTlv) == sizeof(NetworkDiagnosticTlv)));

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList);

    VerifyOrExit((static_cast<TypeListTlv *>(&networkDiagnosticTlv)->IsValid()));

    offset = aMessage.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint8_t i = 0; i < networkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aMessage.Read(offset, sizeof(type), &type) == sizeof(type));

        switch (type)
        {
        case NetworkDiagnosticTlv::kMacCounters:
            mNetif.GetMac().ResetCounters();
            otLogInfoNetDiag(GetInstance(), "Received diagnostic reset type kMacCounters(9)");
            break;

        default:
            otLogInfoNetDiag(GetInstance(), "Received diagnostic reset other type %d not resetable", type);
            break;
        }
    }

    SuccessOrExit(mNetif.GetCoap().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoNetDiag(GetInstance(), "Sent diagnostic reset acknowledgment");

exit:
    return;
}

}  // namespace NetworkDiagnostic

}  // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
