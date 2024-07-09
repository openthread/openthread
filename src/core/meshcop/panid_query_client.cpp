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
 *   This file implements the PAN ID Query Client.
 */

#include "panid_query_client.hpp"

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("PanIdQueryClnt");

PanIdQueryClient::PanIdQueryClient(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error PanIdQueryClient::SendQuery(uint16_t                            aPanId,
                                  uint32_t                            aChannelMask,
                                  const Ip6::Address                 &aAddress,
                                  otCommissionerPanIdConflictCallback aCallback,
                                  void                               *aContext)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message   *message = nullptr;

    VerifyOrExit(Get<MeshCoP::Commissioner>().IsActive(), error = kErrorInvalidState);
    VerifyOrExit((message = Get<Tmf::Agent>().NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->InitAsPost(aAddress, kUriPanIdQuery));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(
        error = Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, Get<MeshCoP::Commissioner>().GetSessionId()));

    SuccessOrExit(error = MeshCoP::ChannelMaskTlv::AppendTo(*message, aChannelMask));

    SuccessOrExit(error = Tlv::Append<MeshCoP::PanIdTlv>(*message, aPanId));

    messageInfo.SetSockAddrToRlocPeerAddrTo(aAddress);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s", UriToString<kUriPanIdQuery>());

    mCallback.Set(aCallback, aContext);

exit:
    FreeMessageOnError(message, error);
    return error;
}

template <>
void PanIdQueryClient::HandleTmf<kUriPanIdConflict>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t panId;
    uint32_t mask;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    LogInfo("Received %s", UriToString<kUriPanIdConflict>());

    SuccessOrExit(Tlv::Find<MeshCoP::PanIdTlv>(aMessage, panId));

    SuccessOrExit(MeshCoP::ChannelMaskTlv::FindIn(aMessage, mask));

    mCallback.InvokeIfSet(panId, mask);

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    LogInfo("Sent %s response", UriToString<kUriPanIdConflict>());

exit:
    return;
}

} // namespace ot

#endif //  OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
