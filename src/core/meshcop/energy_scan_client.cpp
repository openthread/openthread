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

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

RegisterLogModule("EnergyScanClnt");

EnergyScanClient::EnergyScanClient(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error EnergyScanClient::SendQuery(uint32_t                           aChannelMask,
                                  uint8_t                            aCount,
                                  uint16_t                           aPeriod,
                                  uint16_t                           aScanDuration,
                                  const Ip6::Address                &aAddress,
                                  otCommissionerEnergyReportCallback aCallback,
                                  void                              *aContext)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message   *message = nullptr;

    VerifyOrExit(Get<MeshCoP::Commissioner>().IsActive(), error = kErrorInvalidState);
    VerifyOrExit((message = Get<Tmf::Agent>().NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->InitAsPost(aAddress, kUriEnergyScan));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(
        error = Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, Get<MeshCoP::Commissioner>().GetSessionId()));

    SuccessOrExit(error = MeshCoP::ChannelMaskTlv::AppendTo(*message, aChannelMask));

    SuccessOrExit(error = Tlv::Append<MeshCoP::CountTlv>(*message, aCount));
    SuccessOrExit(error = Tlv::Append<MeshCoP::PeriodTlv>(*message, aPeriod));
    SuccessOrExit(error = Tlv::Append<MeshCoP::ScanDurationTlv>(*message, aScanDuration));

    messageInfo.SetSockAddrToRlocPeerAddrTo(aAddress);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s", UriToString<kUriEnergyScan>());

    mCallback.Set(aCallback, aContext);

exit:
    FreeMessageOnError(message, error);
    return error;
}

template <>
void EnergyScanClient::HandleTmf<kUriEnergyReport>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint32_t               mask;
    MeshCoP::EnergyListTlv energyListTlv;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    LogInfo("Received %s", UriToString<kUriEnergyReport>());

    SuccessOrExit(MeshCoP::ChannelMaskTlv::FindIn(aMessage, mask));

    SuccessOrExit(MeshCoP::Tlv::FindTlv(aMessage, MeshCoP::Tlv::kEnergyList, sizeof(energyListTlv), energyListTlv));

    mCallback.InvokeIfSet(mask, energyListTlv.GetEnergyList(), energyListTlv.GetEnergyListLength());

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    LogInfo("Sent %s ack", UriToString<kUriEnergyReport>());

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
