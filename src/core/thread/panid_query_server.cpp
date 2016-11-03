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
 *   This file implements the PAN ID Query Server.
 */

#define WPP_NAME "panid_query_server.tmh"

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <platform/random.h>
#include <thread/meshcop_tlvs.hpp>
#include <thread/panid_query_server.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {

PanIdQueryServer::PanIdQueryServer(ThreadNetif &aThreadNetif) :
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &PanIdQueryServer::HandleTimer, this),
    mPanIdQuery(OPENTHREAD_URI_PANID_QUERY, &PanIdQueryServer::HandleQuery, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mCoapClient(aThreadNetif.GetCoapClient()),
    mNetif(aThreadNetif)
{
    mCoapServer.AddResource(mPanIdQuery);
}

void PanIdQueryServer::HandleQuery(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                   const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleQuery(aHeader, aMessage, aMessageInfo);
}

void PanIdQueryServer::HandleQuery(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::PanIdTlv panId;
    MeshCoP::ChannelMask0Tlv channelMask;

    VerifyOrExit(aHeader.GetCode() == kCoapRequestPost, ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid(),);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPanId, sizeof(panId), panId));
    VerifyOrExit(panId.IsValid(), ;);

    mChannelMask = channelMask.GetMask();
    mCommissioner = aMessageInfo.GetPeerAddr();
    mPanId = panId.GetPanId();
    mTimer.Start(kScanDelay);

    SendQueryResponse(aHeader, aMessageInfo);

exit:
    return;
}

ThreadError PanIdQueryServer::SendQueryResponse(const Coap::Header &aRequestHeader,
                                                const Ip6::MessageInfo &aRequestInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo;

    VerifyOrExit(aRequestHeader.GetType() == kCoapTypeConfirmable, ;);

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    memcpy(&responseInfo, &aRequestInfo, sizeof(responseInfo));
    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = mCoapServer.SendMessage(*message, responseInfo));

    otLogInfoMeshCoP("sent panid query response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryServer::HandleScanResult(void *aContext, Mac::Frame *aFrame)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleScanResult(aFrame);
}

void PanIdQueryServer::HandleScanResult(Mac::Frame *aFrame)
{
    uint16_t panId;

    if (aFrame != NULL)
    {
        aFrame->GetSrcPanId(panId);

        if (panId == mPanId)
        {
            mChannelMask |= 1 << aFrame->GetChannel();
        }
    }
    else if (mChannelMask != 0)
    {
        SendConflict();
    }
}

ThreadError PanIdQueryServer::SendConflict(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::PanIdTlv panId;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_PANID_CONFLICT);
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    channelMask.Init();
    channelMask.SetMask(mChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    panId.Init();
    panId.SetPanId(mPanId);
    SuccessOrExit(error = message->Append(&panId, sizeof(panId)));

    messageInfo.SetPeerAddr(mCommissioner);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent panid conflict");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryServer::HandleTimer(void *aContext)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleTimer();
}

void PanIdQueryServer::HandleTimer(void)
{
    mNetif.GetMac().ActiveScan(mChannelMask, 0, HandleScanResult, this);
    mChannelMask = 0;
}

}  // namespace Thread
