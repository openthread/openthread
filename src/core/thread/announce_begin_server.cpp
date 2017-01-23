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
 *   This file implements the Announce Begin Server.
 */

#define WPP_NAME "announce_begin_server.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <meshcop/tlvs.hpp>
#include <platform/radio.h>
#include <thread/announce_begin_server.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

AnnounceBeginServer::AnnounceBeginServer(ThreadNetif &aThreadNetif) :
    mChannelMask(0),
    mPeriod(0),
    mCount(0),
    mChannel(0),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &AnnounceBeginServer::HandleTimer, this),
    mAnnounceBegin(OPENTHREAD_URI_ANNOUNCE_BEGIN, &AnnounceBeginServer::HandleRequest, this),
    mNetif(aThreadNetif)
{
    mNetif.GetCoapServer().AddResource(mAnnounceBegin);
}

ThreadError AnnounceBeginServer::SendAnnounce(uint32_t aChannelMask)
{
    return SendAnnounce(aChannelMask, kDefaultCount, kDefaultPeriod);
}

ThreadError AnnounceBeginServer::SendAnnounce(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod)
{
    ThreadError error = kThreadError_None;

    mChannelMask = aChannelMask;
    mCount = aCount;
    mPeriod = aPeriod;
    mChannel = kPhyMinChannel;

    while ((mChannelMask & (1 << mChannel)) == 0)
    {
        mChannel++;
        VerifyOrExit(mChannel <= kPhyMaxChannel,);
    }

    mTimer.Start(mPeriod);

exit:
    return error;
}

void AnnounceBeginServer::HandleRequest(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                        const otMessageInfo *aMessageInfo)
{
    static_cast<AnnounceBeginServer *>(aContext)->HandleRequest(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}
void AnnounceBeginServer::HandleRequest(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;
    Ip6::MessageInfo responseInfo(aMessageInfo);

    VerifyOrExit(aHeader.GetCode() == kCoapRequestPost, ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid(),);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kCount, sizeof(count), count));
    VerifyOrExit(count.IsValid(), ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPeriod, sizeof(period), period));
    VerifyOrExit(period.IsValid(), ;);

    SendAnnounce(channelMask.GetMask(), count.GetCount(), period.GetPeriod());

    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(mNetif.GetCoapServer().SendEmptyAck(aHeader, responseInfo));

    otLogInfoMeshCoP("sent announce begin response");

exit:
    return;
}

void AnnounceBeginServer::HandleTimer(void *aContext)
{
    static_cast<AnnounceBeginServer *>(aContext)->HandleTimer();
}

void AnnounceBeginServer::HandleTimer(void)
{
    mNetif.GetMle().SendAnnounce(mChannel++, false);

    while (mCount > 0)
    {
        if (mChannelMask & (1 << mChannel))
        {
            mTimer.Start(mPeriod);
            break;
        }

        mChannel++;

        if (mChannel > kPhyMaxChannel)
        {
            mChannel = kPhyMinChannel;
            mCount--;
        }
    }
}

}  // namespace Thread
