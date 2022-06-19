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

#include "panid_query_server.hpp"

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

RegisterLogModule("MeshCoP");

PanIdQueryServer::PanIdQueryServer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMask(0)
    , mPanId(Mac::kPanIdBroadcast)
    , mTimer(aInstance, PanIdQueryServer::HandleTimer)
    , mPanIdQuery(UriPath::kPanIdQuery, &PanIdQueryServer::HandleQuery, this)
{
    Get<Tmf::Agent>().AddResource(mPanIdQuery);
}

void PanIdQueryServer::HandleQuery(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleQuery(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void PanIdQueryServer::HandleQuery(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t         panId;
    Ip6::MessageInfo responseInfo(aMessageInfo);
    uint32_t         mask;

    VerifyOrExit(aMessage.IsPostRequest());
    VerifyOrExit((mask = MeshCoP::ChannelMaskTlv::GetChannelMask(aMessage)) != 0);

    SuccessOrExit(Tlv::Find<MeshCoP::PanIdTlv>(aMessage, panId));

    mChannelMask  = mask;
    mCommissioner = aMessageInfo.GetPeerAddr();
    mPanId        = panId;
    mTimer.Start(kScanDelay);

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, responseInfo));
        LogInfo("sent panid query response");
    }

exit:
    return;
}

void PanIdQueryServer::HandleScanResult(Mac::ActiveScanResult *aScanResult, void *aContext)
{
    static_cast<PanIdQueryServer *>(aContext)->HandleScanResult(aScanResult);
}

void PanIdQueryServer::HandleScanResult(Mac::ActiveScanResult *aScanResult)
{
    if (aScanResult != nullptr)
    {
        if (aScanResult->mPanId == mPanId)
        {
            mChannelMask |= 1 << aScanResult->mChannel;
        }
    }
    else if (mChannelMask != 0)
    {
        SendConflict();
    }
}

void PanIdQueryServer::SendConflict(void)
{
    Error                   error = kErrorNone;
    MeshCoP::ChannelMaskTlv channelMask;
    Tmf::MessageInfo        messageInfo(GetInstance());
    Coap::Message *         message;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kPanIdConflict);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    channelMask.Init();
    channelMask.SetChannelMask(mChannelMask);
    SuccessOrExit(error = channelMask.AppendTo(*message));

    SuccessOrExit(error = Tlv::Append<MeshCoP::PanIdTlv>(*message, mPanId));

    messageInfo.SetSockAddrToRlocPeerAddrTo(mCommissioner);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("sent panid conflict");

exit:
    FreeMessageOnError(message, error);
    MeshCoP::LogError("send panid conflict", error);
}

void PanIdQueryServer::HandleTimer(Timer &aTimer)
{
    aTimer.Get<PanIdQueryServer>().HandleTimer();
}

void PanIdQueryServer::HandleTimer(void)
{
    IgnoreError(Get<Mac::Mac>().ActiveScan(mChannelMask, 0, HandleScanResult, this));
    mChannelMask = 0;
}

} // namespace ot
