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
 *   This file implements a MeshCoP Leader.
 */

#if OPENTHREAD_FTD

#define WPP_NAME "leader.tmh"

#include "leader.hpp"

#include <stdio.h>

#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

Leader::Leader(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mPetition(OT_URI_PATH_LEADER_PETITION, Leader::HandlePetition, this)
    , mKeepAlive(OT_URI_PATH_LEADER_KEEP_ALIVE, Leader::HandleKeepAlive, this)
    , mTimer(aInstance, HandleTimer, this)
    , mDelayTimerMinimal(DelayTimerTlv::kDelayTimerMinimal)
    , mSessionId(Random::GetUint16())
{
    GetNetif().GetCoap().AddResource(mPetition);
    GetNetif().GetCoap().AddResource(mKeepAlive);
}

void Leader::HandlePetition(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandlePetition(*static_cast<Coap::Message *>(aMessage),
                                                    *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandlePetition(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    CommissioningData data;
    CommissionerIdTlv commissionerId;
    StateTlv::State   state = StateTlv::kReject;

    otLogInfoMeshCoP("received petition");

    VerifyOrExit(GetNetif().GetMle().IsRoutingLocator(aMessageInfo.GetPeerAddr()));
    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerId, sizeof(commissionerId), commissionerId));
    VerifyOrExit(commissionerId.IsValid());

    if (mTimer.IsRunning())
    {
        VerifyOrExit(
            !strncmp(commissionerId.GetCommissionerId(), mCommissionerId.GetCommissionerId(), sizeof(mCommissionerId)));

        ResignCommissioner();
    }

    data.mBorderAgentLocator.Init();
    data.mBorderAgentLocator.SetBorderAgentLocator(HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));

    data.mCommissionerSessionId.Init();
    data.mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    data.mSteeringData.Init();
    data.mSteeringData.SetLength(1);
    data.mSteeringData.Clear();

    SuccessOrExit(
        GetNetif().GetNetworkDataLeader().SetCommissioningData(reinterpret_cast<uint8_t *>(&data), data.GetLength()));

    mCommissionerId = commissionerId;

    state = StateTlv::kAccept;
    mTimer.Start(TimerMilli::SecToMsec(kTimeoutLeaderPetition));

exit:
    SendPetitionResponse(aMessage, aMessageInfo, state);
}

otError Leader::SendPetitionResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    ThreadNetif &            netif = GetNetif();
    otError                  error = OT_ERROR_NONE;
    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;
    Coap::Message *          message;

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);

    message->SetDefaultResponseHeader(aRequest);
    message->SetPayloadMarker();

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    if (mTimer.IsRunning())
    {
        uint16_t len = sizeof(Tlv) + mCommissionerId.GetLength();
        SuccessOrExit(error = message->Append(&mCommissionerId, len));
    }

    if (aState == StateTlv::kAccept)
    {
        sessionId.Init();
        sessionId.SetCommissionerSessionId(mSessionId);
        SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));
    }

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent petition response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Leader::HandleKeepAlive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleKeepAlive(*static_cast<Coap::Message *>(aMessage),
                                                     *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    NetworkData::Leader &    netdata = GetNetif().GetNetworkDataLeader();
    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;
    BorderAgentLocatorTlv *  borderAgentLocator;
    StateTlv::State          responseState;

    otLogInfoMeshCoP("received keep alive");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
    VerifyOrExit(sessionId.IsValid());

    borderAgentLocator =
        static_cast<BorderAgentLocatorTlv *>(netdata.GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));

    if ((borderAgentLocator == NULL) || (sessionId.GetCommissionerSessionId() != mSessionId))
    {
        responseState = StateTlv::kReject;
    }
    else if (state.GetState() != StateTlv::kAccept)
    {
        responseState = StateTlv::kReject;
        ResignCommissioner();
    }
    else
    {
        uint16_t rloc = HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]);

        if (borderAgentLocator->GetBorderAgentLocator() != rloc)
        {
            borderAgentLocator->SetBorderAgentLocator(rloc);
            netdata.IncrementVersion();
        }

        responseState = StateTlv::kAccept;
        mTimer.Start(TimerMilli::SecToMsec(kTimeoutLeaderPetition));
    }

    SendKeepAliveResponse(aMessage, aMessageInfo, responseState);

exit:
    return;
}

otError Leader::SendKeepAliveResponse(const Coap::Message &   aRequest,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      StateTlv::State         aState)
{
    ThreadNetif &  netif = GetNetif();
    otError        error = OT_ERROR_NONE;
    StateTlv       state;
    Coap::Message *message;

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);

    message->SetDefaultResponseHeader(aRequest);
    message->SetPayloadMarker();

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent keep alive response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Leader::SendDatasetChanged(const Ip6::Address &aAddress)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;
    Coap::Message *  message;

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    message->SetToken(Coap::Message::kDefaultTokenLength);
    message->AppendUriPathOptions(OT_URI_PATH_DATASET_CHANGED);

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset changed");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Leader::SetDelayTimerMinimal(uint32_t aDelayTimerMinimal)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit((aDelayTimerMinimal != 0 && aDelayTimerMinimal < DelayTimerTlv::kDelayTimerDefault),
                 error = OT_ERROR_INVALID_ARGS);
    mDelayTimerMinimal = aDelayTimerMinimal;

exit:
    return error;
}

uint32_t Leader::GetDelayTimerMinimal(void) const
{
    return mDelayTimerMinimal;
}

void Leader::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Leader>().HandleTimer();
}

void Leader::HandleTimer(void)
{
    VerifyOrExit(GetNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER);

    ResignCommissioner();

exit:
    return;
}

void Leader::SetEmptyCommissionerData(void)
{
    CommissionerSessionIdTlv mCommissionerSessionId;

    mCommissionerSessionId.Init();
    mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    GetNetif().GetNetworkDataLeader().SetCommissioningData(reinterpret_cast<uint8_t *>(&mCommissionerSessionId),
                                                           sizeof(Tlv) + mCommissionerSessionId.GetLength());
}

void Leader::ResignCommissioner(void)
{
    mTimer.Stop();
    SetEmptyCommissionerData();

    otLogInfoMeshCoP("commissioner inactive");
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
