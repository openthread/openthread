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

#define WPP_NAME "panid_query_client.tmh"

#include <openthread/config.h>

#include "panid_query_client.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

namespace ot {

PanIdQueryClient::PanIdQueryClient(ThreadNetif &aThreadNetif) :
    ThreadNetifLocator(aThreadNetif),
    mCallback(NULL),
    mContext(NULL),
    mPanIdQuery(OT_URI_PATH_PANID_CONFLICT, &PanIdQueryClient::HandleConflict, this)
{
    aThreadNetif.GetCoap().AddResource(mPanIdQuery);
}

otError PanIdQueryClient::SendQuery(uint16_t aPanId, uint32_t aChannelMask, const Ip6::Address &aAddress,
                                    otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::PanIdTlv panId;
    Ip6::MessageInfo messageInfo;
    Message *message = NULL;

    VerifyOrExit(netif.GetCommissioner().IsActive(), error = OT_ERROR_INVALID_STATE);

    header.Init(aAddress.IsMulticast() ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE,
                OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_PANID_QUERY);
    header.SetPayloadMarker();

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(netif.GetCoap(), header)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    sessionId.Init();
    sessionId.SetCommissionerSessionId(netif.GetCommissioner().GetSessionId());
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    channelMask.Init();
    channelMask.SetMask(aChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    panId.Init();
    panId.SetPanId(aPanId);
    SuccessOrExit(error = message->Append(&panId, sizeof(panId)));

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent panid query");

    mCallback = aCallback;
    mContext = aContext;

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void PanIdQueryClient::HandleConflict(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo)
{
    static_cast<PanIdQueryClient *>(aContext)->HandleConflict(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PanIdQueryClient::HandleConflict(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    MeshCoP::PanIdTlv panId;
    MeshCoP::ChannelMask0Tlv channelMask;
    Ip6::MessageInfo responseInfo(aMessageInfo);

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMeshCoP(GetInstance(), "received panid conflict");

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPanId, sizeof(panId), panId));
    VerifyOrExit(panId.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid());

    if (mCallback != NULL)
    {
        mCallback(panId.GetPanId(), channelMask.GetMask(), mContext);
    }

    SuccessOrExit(netif.GetCoap().SendEmptyAck(aHeader, responseInfo));

    otLogInfoMeshCoP(GetInstance(), "sent panid query conflict response");

exit:
    return;
}

}  // namespace ot

#endif //  OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

