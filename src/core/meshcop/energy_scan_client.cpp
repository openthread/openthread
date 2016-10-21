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

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <platform/random.h>
#include <meshcop/energy_scan_client.hpp>
#include <thread/meshcop_tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

EnergyScanClient::EnergyScanClient(ThreadNetif &aThreadNetif) :
    mEnergyScan(OPENTHREAD_URI_ENERGY_REPORT, &EnergyScanClient::HandleReport, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mCoapClient(aThreadNetif.GetCoapClient()),
    mNetif(aThreadNetif)
{
    mContext = NULL;
    mCallback = NULL;
    mCoapServer.AddResource(mEnergyScan);
}

ThreadError EnergyScanClient::SendQuery(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                        uint16_t aScanDuration, const Ip6::Address &aAddress,
                                        otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMaskTlv channelMask;
    union
    {
        MeshCoP::ChannelMaskEntry channelMaskEntry;
        uint8_t channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + sizeof(aChannelMask)];
    };
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

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    channelMask.Init();
    channelMask.SetLength(sizeof(channelMaskBuf));
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    channelMaskEntry.SetChannelPage(0);
    channelMaskEntry.SetMaskLength(sizeof(aChannelMask));

    for (size_t i = 0; i < sizeof(aChannelMask); i++)
    {
        channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + i] = (aChannelMask >> (8 * i)) & 0xff;
    }

    SuccessOrExit(error = message->Append(channelMaskBuf, sizeof(channelMaskBuf)));

    count.Init();
    count.SetCount(aCount);
    SuccessOrExit(error = message->Append(&count, sizeof(count)));

    period.Init();
    period.SetPeriod(aPeriod);
    SuccessOrExit(error = message->Append(&period, sizeof(period)));

    scanDuration.Init();
    scanDuration.SetScanDuration(aScanDuration);
    SuccessOrExit(error = message->Append(&scanDuration, sizeof(scanDuration)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aAddress;
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent energy scan query");

    mCallback = aCallback;
    mContext = aContext;

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void EnergyScanClient::HandleReport(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                    const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<EnergyScanClient *>(aContext)->HandleReport(aHeader, aMessage, aMessageInfo);
}

void EnergyScanClient::HandleReport(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::ChannelMaskTlv tlv;
        MeshCoP::ChannelMaskEntry header;
        uint32_t mask;
    } OT_TOOL_PACKED_END channelMask;

    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::EnergyListTlv tlv;
        uint8_t list[OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS];
    } OT_TOOL_PACKED_END energyList;

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, ;);

    otLogInfoMeshCoP("received energy scan report");

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask.tlv));
    VerifyOrExit(channelMask.tlv.IsValid() &&
                 channelMask.header.GetChannelPage() == 0 &&
                 channelMask.header.GetMaskLength() >= sizeof(uint32_t), ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kEnergyList, sizeof(energyList), energyList.tlv));
    VerifyOrExit(energyList.tlv.IsValid(), ;);

    if (mCallback != NULL)
    {
        mCallback(HostSwap32(channelMask.mask), energyList.list, energyList.tlv.GetLength(), mContext);
    }

    SendResponse(aHeader, aMessageInfo);

exit:
    return;
}

ThreadError EnergyScanClient::SendResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aRequestInfo)
{
    ThreadError error = kThreadError_None;
    Message *message;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    memcpy(&responseInfo, &aRequestInfo, sizeof(responseInfo));
    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = mCoapServer.SendMessage(*message, responseInfo));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

}  // namespace Thread
