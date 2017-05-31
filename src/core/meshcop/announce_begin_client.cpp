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
 *   This file implements the Announce Begin Client.
 */

#define WPP_NAME "announce_begin_client.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "announce_begin_client.hpp"

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

AnnounceBeginClient::AnnounceBeginClient(ThreadNetif &aThreadNetif) :
    mNetif(aThreadNetif)
{
}

otInstance *AnnounceBeginClient::GetInstance(void)
{
    return mNetif.GetInstance();
}

otError AnnounceBeginClient::SendRequest(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                         const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;

    Ip6::MessageInfo messageInfo;
    Message *message = NULL;

    VerifyOrExit(mNetif.GetCommissioner().IsActive(), error = OT_ERROR_INVALID_STATE);

    header.Init(aAddress.IsMulticast() ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE,
                OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_ANNOUNCE_BEGIN);
    header.SetPayloadMarker();

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(mNetif.GetCoap(), header)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    channelMask.Init();
    channelMask.SetMask(aChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    count.Init();
    count.SetCount(aCount);
    SuccessOrExit(error = message->Append(&count, sizeof(count)));

    period.Init();
    period.SetPeriod(aPeriod);
    SuccessOrExit(error = message->Append(&period, sizeof(period)));

    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent announce begin query");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

}  // namespace ot

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

