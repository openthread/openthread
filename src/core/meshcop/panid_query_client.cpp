/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the PAN ID Query Client.
 */

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <meshcop/panid_query_client.hpp>
#include <thread/meshcop_tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {

PanIdQueryClient::PanIdQueryClient(ThreadNetif &aThreadNetif) :
    mPanIdQuery(OPENTHREAD_URI_PANID_CONFLICT, &HandleConflict, this),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &HandleTimer, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mNetif(aThreadNetif)
{
    mCoapServer.AddResource(mPanIdQuery);
    mSocket.Open(HandleUdpReceive, this);
}

ThreadError PanIdQueryClient::SendQuery(uint16_t aPanId, uint32_t aChannelMask, const Ip6::Address &aAddress,
                                        otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMaskTlv channelMask;
    union
    {
        MeshCoP::ChannelMaskEntry channelMaskEntry;
        uint8_t channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + sizeof(aChannelMask)];
    };
    MeshCoP::PanIdTlv panId;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init();
    header.SetVersion(1);
    header.SetType(aAddress.IsMulticast() ? Coap::Header::kTypeNonConfirmable : Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(0);
    header.SetToken(NULL, 0);
    header.AppendUriPathOptions(OPENTHREAD_URI_PANID_QUERY);
    header.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    channelMask.Init();
    channelMask.SetLength(sizeof(channelMaskBuf));
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    channelMaskEntry.SetChannelPage(0);
    channelMaskEntry.SetMaskLength(sizeof(aChannelMask));

    for (size_t i = 0; i < sizeof(aChannelMask); i++)
    {
        channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + i] = (aChannelMask >> (8 * i)) & 0xff;
    }

    SuccessOrExit(error = message->Append(channelMaskBuf, sizeof(channelMaskBuf)));

    panId.Init();
    panId.SetPanId(aPanId);
    SuccessOrExit(error = message->Append(&panId, sizeof(panId)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aAddress;
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent panid query\r\n");

    mCallback = aCallback;
    mContext = aContext;

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryClient::HandleConflict(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<PanIdQueryClient *>(aContext)->HandleConflict(aHeader, aMessage, aMessageInfo);
}

void PanIdQueryClient::HandleConflict(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::PanIdTlv panId;
    union
    {
        MeshCoP::ChannelMaskEntry channelMaskEntry;
        uint8_t channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + sizeof(uint32_t)];
    };
    uint32_t channelMask;
    uint16_t offset;
    uint16_t length;

    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodePost, ;);

    otLogInfoMeshCoP("received panid conflict\r\n");

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPanId, sizeof(panId), panId));
    VerifyOrExit(panId.IsValid(), ;);

    SuccessOrExit(MeshCoP::Tlv::GetValueOffset(aMessage, MeshCoP::Tlv::kChannelMask, offset, length));
    aMessage.Read(offset, sizeof(channelMaskBuf), channelMaskBuf);
    VerifyOrExit(channelMaskEntry.GetChannelPage() == 0 &&
                 channelMaskEntry.GetMaskLength() == sizeof(uint32_t), ;);

    channelMask =
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 0]) << 0) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 1]) << 8) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 2]) << 16) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 3]) << 24);

    if (mCallback != NULL)
    {
        mCallback(panId.GetPanId(), channelMask, mContext);
    }

    SendConflictResponse(aHeader, aMessageInfo);

exit:
    return;
}

ThreadError PanIdQueryClient::SendConflictResponse(const Coap::Header &aRequestHeader,
                                                   const Ip6::MessageInfo &aRequestInfo)
{
    ThreadError error = kThreadError_None;
    Message *message;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.Init();
    responseHeader.SetVersion(1);
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    memcpy(&responseInfo, &aRequestInfo, sizeof(responseInfo));
    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = mCoapServer.SendMessage(*message, responseInfo));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryClient::HandleTimer(void *aContext)
{
    static_cast<PanIdQueryClient *>(aContext)->HandleTimer();
}

void PanIdQueryClient::HandleTimer(void)
{
}

void PanIdQueryClient::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    otLogInfoMeshCoP("received panid query response\r\n");
    (void)aContext;
    (void)aMessage;
    (void)aMessageInfo;
}

}  // namespace Thread
