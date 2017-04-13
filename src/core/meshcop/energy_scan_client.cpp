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
 *   This file implements the Energy Scan Client.
 */

#define WPP_NAME "energy_scan_client.tmh"

#include "openthread/platform/random.h"

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <meshcop/energy_scan_client.hpp>
#include <meshcop/meshcop.hpp>
#include <meshcop/tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

EnergyScanClient::EnergyScanClient(ThreadNetif &aThreadNetif) :
    mEnergyScan(OPENTHREAD_URI_ENERGY_REPORT, &EnergyScanClient::HandleReport, this),
    mNetif(aThreadNetif)
{
    mContext = NULL;
    mCallback = NULL;
    mNetif.GetCoapServer().AddResource(mEnergyScan);
}

otInstance *EnergyScanClient::GetInstance(void)
{
    return mNetif.GetInstance();
}

ThreadError EnergyScanClient::SendQuery(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                        uint16_t aScanDuration, const Ip6::Address &aAddress,
                                        otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;
    MeshCoP::ScanDurationTlv scanDuration;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(aAddress.IsMulticast() ? kCoapTypeNonConfirmable : kCoapTypeConfirmable,
                kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_ENERGY_SCAN);
    header.SetPayloadMarker();

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(mNetif.GetCoapClient(), header)) != NULL,
                 error = kThreadError_NoBufs);

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

    scanDuration.Init();
    scanDuration.SetScanDuration(aScanDuration);
    SuccessOrExit(error = message->Append(&scanDuration, sizeof(scanDuration)));

    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent energy scan query");

    mCallback = aCallback;
    mContext = aContext;

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void EnergyScanClient::HandleReport(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                    const otMessageInfo *aMessageInfo)
{
    static_cast<EnergyScanClient *>(aContext)->HandleReport(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void EnergyScanClient::HandleReport(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::ChannelMask0Tlv channelMask;
    Ip6::MessageInfo responseInfo(aMessageInfo);

    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::EnergyListTlv tlv;
        uint8_t list[OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS];
    } OT_TOOL_PACKED_END energyList;

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost);

    otLogInfoMeshCoP(GetInstance(), "received energy scan report");

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kEnergyList, sizeof(energyList), energyList.tlv));
    VerifyOrExit(energyList.tlv.IsValid());

    if (mCallback != NULL)
    {
        mCallback(channelMask.GetMask(), energyList.list, energyList.tlv.GetLength(), mContext);
    }

    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(mNetif.GetCoapServer().SendEmptyAck(aHeader, responseInfo));

    otLogInfoMeshCoP(GetInstance(), "sent energy scan report response");

exit:
    return;
}

}  // namespace Thread
