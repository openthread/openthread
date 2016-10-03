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

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <mac/mac_frame.hpp>
#include <net/netif.hpp>
#include <platform/random.h>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <thread/network_diag.hpp>
#include <thread/network_diag_tlvs.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

namespace NetworkDiagnostic {

NetworkDiagnostic::NetworkDiagnostic(ThreadNetif &aThreadNetif) :
    mDiagnosticGet(OPENTHREAD_URI_DIAGNOSTIC_GET, &NetworkDiagnostic::HandleDiagnosticGet, this),
    mDiagnosticReset(OPENTHREAD_URI_DIAGNOSTIC_RESET, &NetworkDiagnostic::HandleDiagnosticReset, this),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mMle(aThreadNetif.GetMle()),
    mNetif(aThreadNetif)
{
    mCoapServer.AddResource(mDiagnosticGet);
    mCoapServer.AddResource(mDiagnosticReset);
    mCoapMessageId = static_cast<uint8_t>(otPlatRandomGet());
}

ThreadError NetworkDiagnostic::SendDiagnosticGet(const Ip6::Address &aDestination, uint8_t aTlvTypes[], uint8_t aCount)
{
    ThreadError error;
    Ip6::SockAddr sockaddr;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    sockaddr.mPort = kCoapUdpPort;
    mSocket.Open(&NetworkDiagnostic::HandleUdpReceive, this);
    mSocket.Bind(sockaddr);

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodeGet);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_DIAGNOSTIC_GET);
    header.Finalize();

    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aDestination;
    messageInfo.GetSockAddr() = *mMle.GetMeshLocal16();
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic get\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    NetworkDiagnostic *obj = static_cast<NetworkDiagnostic *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;

    SuccessOrExit(header.FromMessage(aMessage));
    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == mCoapMessageId &&
                 header.GetTokenLength() == sizeof(mCoapToken) &&
                 memcmp(mCoapToken, header.GetToken(), sizeof(mCoapToken)) == 0, ;);

    otLogInfoNetDiag("Network Diagnostic message acknowledged\n");

exit:
    (void)aMessageInfo;
}

ThreadError NetworkDiagnostic::SendDiagnosticReset(const Ip6::Address &aDestination, uint8_t aTlvTypes[],
                                                   uint8_t aCount)
{
    ThreadError error;
    Ip6::SockAddr sockaddr;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    sockaddr.mPort = kCoapUdpPort;
    mSocket.Open(&NetworkDiagnostic::HandleUdpReceive, this);
    mSocket.Bind(sockaddr);

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_DIAGNOSTIC_RESET);
    header.Finalize();

    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aDestination;
    messageInfo.GetSockAddr() = *mMle.GetMeshLocal16();
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoNetDiag("Sent network diagnostic reset\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGet(void *aContext, Coap::Header &aHeader,
                                            Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    NetworkDiagnostic *obj = reinterpret_cast<NetworkDiagnostic *>(aContext);
    obj->HandleDiagnosticGet(aHeader, aMessage, aMessageInfo);
}

ThreadError NetworkDiagnostic::AppendIPv6AddressList(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    IPv6AddressListTlv tlv;
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

ThreadError NetworkDiagnostic::AppendChildTable(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    uint8_t numChildren;
    const Child *children = mMle.GetChildren(&numChildren);

    {
        uint8_t count = 0;

        for (int i = 0; i < numChildren; i++)
        {
            if (children[i].mState != Neighbor::kStateInvalid)
            {
                continue;
            }

            count++;
        }

        ChildTableTlv tlv;
        tlv.Init();
        tlv.SetLength(count * sizeof(ChildTableEntry));

        SuccessOrExit(error = aMessage.Append(&tlv, sizeof(ChildTableTlv)));
    }

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].mState != Neighbor::kStateInvalid)
        {
            continue;
        }

        const Child &child = children[i];
        ChildTableEntry entry;
        uint8_t timeout = 0;

        while (static_cast<uint32_t>(1 << timeout) < child.mTimeout) { timeout++; }

        entry.SetTimeout(timeout + 4);
        entry.SetChildId(child.mValid.mRloc16);
        entry.SetMode(child.mMode);

        SuccessOrExit(error = aMessage.Append(&entry, sizeof(ChildTableEntry)));
    }

exit:

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGet(Coap::Header &aHeader, Message &aMessage,
                                            const Ip6::MessageInfo &aMessageInfo)
{
    uint8_t tlvTypeSet[kNumTlvTypes];
    uint16_t numTlvTypes;
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodeGet, error = kThreadError_Drop);

    otLogInfoNetDiag("Received diagnostic get request\n");

    header.Init();
    header.SetType(Coap::Header::kTypeAcknowledgment);
    header.SetCode(Coap::Header::kCodeChanged);
    header.SetMessageId(aHeader.GetMessageId());
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    numTlvTypes = aMessage.Read(aMessage.GetOffset(), kNumTlvTypes, tlvTypeSet);

    for (uint8_t i = 0; i < numTlvTypes; i++)
    {
        otLogInfoNetDiag("Received diagnostic get type %d\n", tlvTypeSet[i]);

        switch (tlvTypeSet[i])
        {
        case NetworkDiagnosticTlv::kExtMacAddress:
        {
            ExtMacAddressTlv tlv;
            tlv.Init();
            tlv.SetMacAddr(*mNetif.GetMac().GetExtAddress());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kAddress16:
        {
            Address16Tlv tlv;
            tlv.Init();
            tlv.SetRloc16(mMle.GetRloc16());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kMode:
        {
            ModeTlv tlv;
            tlv.Init();
            tlv.SetMode(mMle.GetDeviceMode());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kTimeout:
        {
            if ((mMle.GetDeviceMode() & ModeTlv::kModeRxOnWhenIdle) == 0)
            {
                TimeoutTlv tlv;
                tlv.Init();
                tlv.SetTimeout(mMle.GetTimeout());
                SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            }

            break;
        }

        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv tlv;
            tlv.Init();
            mMle.FillConnectivityTlv(*reinterpret_cast<Mle::ConnectivityTlv *>(&tlv));
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kRoute:
        {
            RouteTlv tlv;
            tlv.Init();
            mMle.FillRouteTlv(*reinterpret_cast<Mle::RouteTlv *>(&tlv));
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kLeaderData:
        {
            LeaderDataTlv tlv;
            memcpy(&tlv, &mMle.GetLeaderDataTlv(), sizeof(tlv));
            tlv.Init();
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv tlv;
            tlv.Init();
            mMle.FillNetworkDataTlv((*reinterpret_cast<Mle::NetworkDataTlv *>(&tlv)), true);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kIPv6AddressList:
        {
            SuccessOrExit(error = AppendIPv6AddressList(*message));
            break;
        }

        case NetworkDiagnosticTlv::kMacCounters:
        {
            MacCountersTlv tlv;
            memset(&tlv, 0, sizeof(tlv));
            tlv.Init();
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kBatteryLevel:
        {
            // TODO Need more api from driver
            BatteryLevelTlv tlv;
            tlv.Init();
            tlv.SetBatteryLevel(100);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kSupplyVoltage:
        {
            // TODO Need more api from driver
            SupplyVoltageTlv tlv;
            tlv.Init();
            tlv.SetSupplyVoltage(0);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kChildTable:
        {
            SuccessOrExit(error = AppendChildTable(*message));
            break;
        }

        case NetworkDiagnosticTlv::kChannelPages:
        {
            ChannelPagesTlv tlv;
            tlv.Init();
            tlv.GetChannelPages()[0] = 0;
            tlv.SetLength(1);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        default:
            ExitNow();
        }
    }

    memcpy(&messageInfo, &aMessageInfo, sizeof(messageInfo));
    memset(&messageInfo.mSockAddr, 0, sizeof(messageInfo.mSockAddr));
    otLogInfoNetDiag("Sending diagnostic get acknowledgment\n");
    SuccessOrExit(error = mCoapServer.SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic get acknowledgment\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void NetworkDiagnostic::HandleDiagnosticReset(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                              const Ip6::MessageInfo &aMessageInfo)
{
    NetworkDiagnostic *obj = reinterpret_cast<NetworkDiagnostic *>(aContext);
    obj->HandleDiagnosticReset(aHeader, aMessage, aMessageInfo);
}

void NetworkDiagnostic::HandleDiagnosticReset(Coap::Header &aHeader, Message &aMessage,
                                              const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    uint8_t tlvTypeSet[kNumResetTlvTypes];
    uint16_t numTlvTypes;
    Message *message = NULL;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;
    otLogInfoNetDiag("Received diagnostic reset request\n");

    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodePost, error = kThreadError_Drop);

    otLogInfoNetDiag("Received diagnostic reset request\n");
    numTlvTypes = aMessage.Read(aMessage.GetOffset(), kNumResetTlvTypes, tlvTypeSet);

    otLogInfoNetDiag("Received diagnostic reset request\n");

    for (uint8_t i = 0; i < numTlvTypes; i++)
    {
        switch (tlvTypeSet[i])
        {
        case NetworkDiagnosticTlv::kMacCounters:
            break;

        default:
            ExitNow();
        }
    }

    otLogInfoNetDiag("Received diagnostic reset request\n");
    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    otLogInfoNetDiag("Received diagnostic reset request\n");
    header.Init();
    header.SetType(Coap::Header::kTypeAcknowledgment);
    header.SetCode(Coap::Header::kCodeChanged);
    header.SetMessageId(aHeader.GetMessageId());
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.Finalize();
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    memcpy(&messageInfo, &aMessageInfo, sizeof(messageInfo));
    memset(&messageInfo.mSockAddr, 0, sizeof(messageInfo.mSockAddr));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic reset acknowledgment\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

}  // namespace NetworkDiagnostic

}  // namespace Thread
