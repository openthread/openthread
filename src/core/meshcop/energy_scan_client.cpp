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

#include "energy_scan_client.hpp"

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

namespace ot {

EnergyScanClient::EnergyScanClient(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnergyScan(OT_URI_PATH_ENERGY_REPORT, &EnergyScanClient::HandleReport, this)
{
    mContext  = NULL;
    mCallback = NULL;
    Get<Coap::Coap>().AddResource(mEnergyScan);
}

otError EnergyScanClient::SendQuery(uint32_t                           aChannelMask,
                                    uint8_t                            aCount,
                                    uint16_t                           aPeriod,
                                    uint16_t                           aScanDuration,
                                    const Ip6::Address &               aAddress,
                                    otCommissionerEnergyReportCallback aCallback,
                                    void *                             aContext)
{
    otError                           error = OT_ERROR_NONE;
    MeshCoP::CommissionerSessionIdTlv sessionId;
    MeshCoP::ChannelMaskTlv           channelMask;
    MeshCoP::CountTlv                 count;
    MeshCoP::PeriodTlv                period;
    MeshCoP::ScanDurationTlv          scanDuration;
    Ip6::MessageInfo                  messageInfo;
    Coap::Message *                   message = NULL;

    VerifyOrExit(Get<MeshCoP::Commissioner>().IsActive(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error =
                      message->Init(aAddress.IsMulticast() ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE,
                                    OT_COAP_CODE_POST, OT_URI_PATH_ENERGY_SCAN));
    SuccessOrExit(error = message->SetPayloadMarker());

    sessionId.Init();
    sessionId.SetCommissionerSessionId(Get<MeshCoP::Commissioner>().GetSessionId());
    SuccessOrExit(error = sessionId.AppendTo(*message));

    channelMask.Init();
    channelMask.SetChannelMask(aChannelMask);
    SuccessOrExit(error = channelMask.AppendTo(*message));

    count.Init();
    count.SetCount(aCount);
    SuccessOrExit(error = count.AppendTo(*message));

    period.Init();
    period.SetPeriod(aPeriod);
    SuccessOrExit(error = period.AppendTo(*message));

    scanDuration.Init();
    scanDuration.SetScanDuration(aScanDuration);
    SuccessOrExit(error = scanDuration.AppendTo(*message));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent energy scan query");

    mCallback = aCallback;
    mContext  = aContext;

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void EnergyScanClient::HandleReport(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<EnergyScanClient *>(aContext)->HandleReport(*static_cast<Coap::Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void EnergyScanClient::HandleReport(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint32_t mask;

    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::EnergyListTlv tlv;
        uint8_t                list[OPENTHREAD_CONFIG_TMF_ENERGY_SCAN_MAX_RESULTS];
    } OT_TOOL_PACKED_END energyList;

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMeshCoP("received energy scan report");

    VerifyOrExit((mask = MeshCoP::ChannelMaskTlv::GetChannelMask(aMessage)) != 0);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kEnergyList, sizeof(energyList), energyList.tlv));
    VerifyOrExit(energyList.tlv.IsValid());

    if (mCallback != NULL)
    {
        mCallback(mask, energyList.list, energyList.tlv.GetLength(), mContext);
    }

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    otLogInfoMeshCoP("sent energy scan report response");

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
