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

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

namespace ot {

PanIdQueryClient::PanIdQueryClient(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCallback(nullptr)
    , mContext(nullptr)
    , mPanIdQuery(UriPath::kPanIdConflict, &PanIdQueryClient::HandleConflict, this)
{
    Get<Tmf::TmfAgent>().AddResource(mPanIdQuery);
}

otError PanIdQueryClient::SendQuery(uint16_t                            aPanId,
                                    uint32_t                            aChannelMask,
                                    const Ip6::Address &                aAddress,
                                    otCommissionerPanIdConflictCallback aCallback,
                                    void *                              aContext)
{
    otError                 error = OT_ERROR_NONE;
    MeshCoP::ChannelMaskTlv channelMask;
    Ip6::MessageInfo        messageInfo;
    Coap::Message *         message = nullptr;

    VerifyOrExit(Get<MeshCoP::Commissioner>().IsActive(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsPost(aAddress, UriPath::kPanIdQuery));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, MeshCoP::Tlv::kCommissionerSessionId,
                                               Get<MeshCoP::Commissioner>().GetSessionId()));

    channelMask.Init();
    channelMask.SetChannelMask(aChannelMask);
    SuccessOrExit(error = channelMask.AppendTo(*message));

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, MeshCoP::Tlv::kPanId, aPanId));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent panid query");

    mCallback = aCallback;
    mContext  = aContext;

exit:
    FreeMessageOnError(message, error);
    return error;
}

void PanIdQueryClient::HandleConflict(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PanIdQueryClient *>(aContext)->HandleConflict(*static_cast<Coap::Message *>(aMessage),
                                                              *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PanIdQueryClient::HandleConflict(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t         panId;
    Ip6::MessageInfo responseInfo(aMessageInfo);
    uint32_t         mask;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), OT_NOOP);

    otLogInfoMeshCoP("received panid conflict");

    SuccessOrExit(Tlv::FindUint16Tlv(aMessage, MeshCoP::Tlv::kPanId, panId));

    VerifyOrExit((mask = MeshCoP::ChannelMaskTlv::GetChannelMask(aMessage)) != 0, OT_NOOP);

    if (mCallback != nullptr)
    {
        mCallback(panId, mask, mContext);
    }

    SuccessOrExit(Get<Tmf::TmfAgent>().SendEmptyAck(aMessage, responseInfo));

    otLogInfoMeshCoP("sent panid query conflict response");

exit:
    return;
}

} // namespace ot

#endif //  OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
