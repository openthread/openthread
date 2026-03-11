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
    , mIsRunning(false)
    , mTimer(aInstance)
{
}

template <> void PanIdQueryServer::HandleTmf<kUriPanIdQuery>(Coap::Msg &aMsg)
{
    Error    error;
    uint16_t panId;
    uint32_t mask;

    VerifyOrExit(!mIsRunning, error = kErrorBusy);

    SuccessOrExit(error = MeshCoP::ChannelMaskTlv::FindIn(aMsg.mMessage, mask));

    SuccessOrExit(error = Tlv::Find<MeshCoP::PanIdTlv>(aMsg.mMessage, panId));

    mChannelMask  = mask;
    mCommissioner = aMsg.mMessageInfo.GetPeerAddr();
    mPanId        = panId;
    mIsRunning    = true;
    mTimer.Start(kScanDelay);

exit:
    IgnoreError(Get<Tmf::Agent>().SendAckResponseIfUnicastRequest(aMsg, error));
}

void PanIdQueryServer::HandleScanResult(const ScanResult *aScanResult)
{
    if (aScanResult != nullptr)
    {
        if (aScanResult->mPanId == mPanId)
        {
            SetBit<uint32_t>(mChannelMask, aScanResult->mChannel);
        }
    }
    else if (mChannelMask != 0)
    {
        mIsRunning = false;
        SendConflict();
    }
}

void PanIdQueryServer::SendConflict(void)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().AllocateAndInitPriorityConfirmablePostMessage(kUriPanIdConflict);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = MeshCoP::ChannelMaskTlv::AppendTo(*message, mChannelMask));

    SuccessOrExit(error = Tlv::Append<MeshCoP::PanIdTlv>(*message, mPanId));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, mCommissioner));

    LogInfo("Sent %s", UriToString<kUriPanIdConflict>());

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send panid conflict");
}

void PanIdQueryServer::HandleTimer(void)
{
    if (Get<Mac::Mac>().ActiveScan(mChannelMask, 0, HandleScanResult, this) != kErrorNone)
    {
        mIsRunning = false;
    }

    mChannelMask = 0;
}

} // namespace ot
