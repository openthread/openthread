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

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <platform/random.h>
#include <meshcop/announce_begin_client.hpp>
#include <thread/meshcop_tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {

AnnounceBeginClient::AnnounceBeginClient(ThreadNetif &aThreadNetif) :
    mSocket(aThreadNetif.GetIp6().mUdp),
    mNetif(aThreadNetif)
{
    mSocket.Open(HandleUdpReceive, this);
    mCoapMessageId = static_cast<uint8_t>(otPlatRandomGet());
}

ThreadError AnnounceBeginClient::SendRequest(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                             const Ip6::Address &aAddress)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;

    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::ChannelMaskTlv tlv;
        MeshCoP::ChannelMaskEntry header;
        uint32_t mask;
    } OT_TOOL_PACKED_END channelMask;

    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;

    Ip6::MessageInfo messageInfo;
    Message *message;

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetType(aAddress.IsMulticast() ? Coap::Header::kTypeNonConfirmable : Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_ANNOUNCE_BEGIN);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    channelMask.tlv.Init();
    channelMask.tlv.SetLength(sizeof(channelMask.header) + sizeof(channelMask.mask));
    channelMask.header.SetChannelPage(0);
    channelMask.header.SetMaskLength(sizeof(channelMask.mask));
    channelMask.mask = HostSwap32(aChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    count.Init();
    count.SetCount(aCount);
    SuccessOrExit(error = message->Append(&count, sizeof(count)));

    period.Init();
    period.SetPeriod(aPeriod);
    SuccessOrExit(error = message->Append(&period, sizeof(period)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aAddress;
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent announce begin query\r\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void AnnounceBeginClient::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    otLogInfoMeshCoP("received announce begin response\r\n");
    (void)aContext;
    (void)aMessage;
    (void)aMessageInfo;
}

}  // namespace Thread
