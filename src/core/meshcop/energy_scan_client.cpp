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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "energy_scan_client.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

EnergyScanClient::EnergyScanClient(ThreadNetif &aThreadNetif) :
    mEnergyScan(OT_URI_PATH_ENERGY_REPORT, &EnergyScanClient::HandleReport, this),
    mNetif(aThreadNetif)
{
    mContext = NULL;
    mCallback = NULL;
    mNetif.GetCoap().AddResource(mEnergyScan);
}

otInstance *EnergyScanClient::GetInstance(void)
{
    return mNetif.GetInstance();
}

otError EnergyScanClient::SendQuery(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                    uint16_t aScanDuration, const Ip6::Address &aAddress,
                                    otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMask0Tlv channelMask;
    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;
    MeshCoP::ScanDurationTlv scanDuration;
    Ip6::MessageInfo messageInfo;
    Message *message = NULL;

    VerifyOrExit(mNetif.GetCommissioner().IsActive(), error = OT_ERROR_INVALID_STATE);

    header.Init(aAddress.IsMulticast() ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE,
                OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_ENERGY_SCAN);
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

    scanDuration.Init();
    scanDuration.SetScanDuration(aScanDuration);
    SuccessOrExit(error = message->Append(&scanDuration, sizeof(scanDuration)));

    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());
    SuccessOrExit(error = mNetif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent energy scan query");

    mCallback = aCallback;
    mContext = aContext;

exit:

    if (error != OT_ERROR_NONE && message != NULL)
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

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMeshCoP(GetInstance(), "received energy scan report");

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kEnergyList, sizeof(energyList), energyList.tlv));
    VerifyOrExit(energyList.tlv.IsValid());

    if (mCallback != NULL)
    {
        mCallback(channelMask.GetMask(), energyList.list, energyList.tlv.GetLength(), mContext);
    }

    SuccessOrExit(mNetif.GetCoap().SendEmptyAck(aHeader, responseInfo));

    otLogInfoMeshCoP(GetInstance(), "sent energy scan report response");

exit:
    return;
}

}  // namespace ot

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

