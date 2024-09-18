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

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("MeshCoP");

PanIdQueryServer::PanIdQueryServer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMask(0)
    , mPanId(Mac::kPanIdBroadcast)
    , mTimer(aInstance)
{
}

template <>
void PanIdQueryServer::HandleTmf<kUriPanIdQuery>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t panId;
    uint32_t mask;

    VerifyOrExit(aMessage.IsPostRequest());
    SuccessOrExit(MeshCoP::ChannelMaskTlv::FindIn(aMessage, mask));

    SuccessOrExit(Tlv::Find<MeshCoP::PanIdTlv>(aMessage, panId));

    mChannelMask  = mask;
    mCommissioner = aMessageInfo.GetPeerAddr();
    mPanId        = panId;
    mTimer.Start(kScanDelay);

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));
        LogInfo("Sent %s ack", UriToString<kUriPanIdQuery>());
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
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message   *message;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriPanIdConflict);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = MeshCoP::ChannelMaskTlv::AppendTo(*message, mChannelMask));

    SuccessOrExit(error = Tlv::Append<MeshCoP::PanIdTlv>(*message, mPanId));

    messageInfo.SetSockAddrToRlocPeerAddrTo(mCommissioner);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s", UriToString<kUriPanIdConflict>());

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send panid conflict");
}

void PanIdQueryServer::HandleTimer(void)
{
    IgnoreError(Get<Mac::Mac>().ActiveScan(mChannelMask, 0, HandleScanResult, this));
    mChannelMask = 0;
}

} // namespace ot
