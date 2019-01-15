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

#include "panid_query_server.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {

PanIdQueryServer::PanIdQueryServer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMask(0)
    , mPanId(Mac::kPanIdBroadcast)
    , mTimer(aInstance, &PanIdQueryServer::HandleTimer, this)
    , mPanIdQuery(OT_URI_PATH_PANID_QUERY, &PanIdQueryServer::HandleQuery, this)
{
    GetNetif().GetCoap().AddResource(mPanIdQuery);
}

void PanIdQueryServer::HandleQuery(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleQuery(*static_cast<Coap::Message *>(aMessage),
                                                           *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PanIdQueryServer::HandleQuery(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::PanIdTlv       panId;
    MeshCoP::ChannelMaskTlv channelMask;
    Ip6::MessageInfo        responseInfo(aMessageInfo);

    VerifyOrExit(aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid() && channelMask.GetChannelPage() == OT_RADIO_CHANNEL_PAGE);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPanId, sizeof(panId), panId));
    VerifyOrExit(panId.IsValid());

    mChannelMask  = channelMask.GetMask();
    mCommissioner = aMessageInfo.GetPeerAddr();
    mPanId        = panId.GetPanId();
    mTimer.Start(kScanDelay);

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(GetNetif().GetCoap().SendEmptyAck(aMessage, responseInfo));
        otLogInfoMeshCoP("sent panid query response");
    }

exit:
    return;
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

otError PanIdQueryServer::SendConflict(void)
{
    ThreadNetif &           netif = GetNetif();
    otError                 error = OT_ERROR_NONE;
    MeshCoP::ChannelMaskTlv channelMask;
    MeshCoP::PanIdTlv       panId;
    Ip6::MessageInfo        messageInfo;
    Coap::Message *         message;

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(netif.GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    message->SetToken(Coap::Message::kDefaultTokenLength);
    message->AppendUriPathOptions(OT_URI_PATH_PANID_CONFLICT);
    message->SetPayloadMarker();

    channelMask.Init();
    channelMask.SetChannelPage(OT_RADIO_CHANNEL_PAGE);
    channelMask.SetMask(mChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    panId.Init();
    panId.SetPanId(mPanId);
    SuccessOrExit(error = message->Append(&panId, sizeof(panId)));

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(mCommissioner);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent panid conflict");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryServer::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<PanIdQueryServer>().HandleTimer();
}

void PanIdQueryServer::HandleTimer(void)
{
    GetNetif().GetMac().ActiveScan(mChannelMask, 0, HandleScanResult, this);
    mChannelMask = 0;
}

} // namespace ot
