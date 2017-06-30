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

#include <openthread/config.h>

#include "leader.hpp"

#include <stdio.h>

#include <openthread/platform/random.h>

#include "openthread-instance.h"
#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace MeshCoP {

Leader::Leader(ThreadNetif &aThreadNetif):
    ThreadNetifLocator(aThreadNetif),
    mPetition(OT_URI_PATH_LEADER_PETITION, Leader::HandlePetition, this),
    mKeepAlive(OT_URI_PATH_LEADER_KEEP_ALIVE, Leader::HandleKeepAlive, this),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this),
    mDelayTimerMinimal(DelayTimerTlv::kDelayTimerMinimal),
    mSessionId(0xffff)
{
    aThreadNetif.GetCoap().AddResource(mPetition);
    aThreadNetif.GetCoap().AddResource(mKeepAlive);
}

void Leader::HandlePetition(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                            const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandlePetition(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandlePetition(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    CommissioningData data;
    CommissionerIdTlv commissionerId;
    StateTlv::State state = StateTlv::kReject;

    otLogInfoMeshCoP(GetInstance(), "received petition");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerId, sizeof(commissionerId), commissionerId));
    VerifyOrExit(commissionerId.IsValid());

    if (mTimer.IsRunning())
    {
        VerifyOrExit(!strncmp(commissionerId.GetCommissionerId(), mCommissionerId.GetCommissionerId(),
                              sizeof(mCommissionerId)));

        ResignCommissioner();
    }

    data.mBorderAgentLocator.Init();
    data.mBorderAgentLocator.SetBorderAgentLocator(HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));

    data.mCommissionerSessionId.Init();
    data.mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    data.mSteeringData.Init();
    data.mSteeringData.SetLength(1);
    data.mSteeringData.Clear();

    SuccessOrExit(GetNetif().GetNetworkDataLeader().SetCommissioningData(reinterpret_cast<uint8_t *>(&data),
                                                                         data.GetLength()));

    mCommissionerId = commissionerId;

    state = StateTlv::kAccept;
    mTimer.Start(Timer::SecToMsec(kTimeoutLeaderPetition));

exit:
    OT_UNUSED_VARIABLE(aMessageInfo);
    SendPetitionResponse(aHeader, aMessageInfo, state);
}

otError Leader::SendPetitionResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State aState)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;
    Message *message;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

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

    otLogInfoMeshCoP(GetInstance(), "sent petition response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Leader::HandleKeepAlive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                             const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleKeepAlive(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleKeepAlive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    StateTlv state;
    CommissionerSessionIdTlv sessionId;
    StateTlv::State responseState;

    otLogInfoMeshCoP(GetInstance(), "received keep alive");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
    VerifyOrExit(sessionId.IsValid());

    if (sessionId.GetCommissionerSessionId() != mSessionId)
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
        responseState = StateTlv::kAccept;
        mTimer.Start(Timer::SecToMsec(kTimeoutLeaderPetition));
    }

    SendKeepAliveResponse(aHeader, aMessageInfo, responseState);

exit:
    return;
}

otError Leader::SendKeepAliveResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                      StateTlv::State aState)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    StateTlv state;
    Message *message;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent keep alive response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Leader::SendDatasetChanged(const Ip6::Address &aAddress)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_DATASET_CHANGED);

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset changed");

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
    GetOwner(aTimer).HandleTimer();
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

    otLogInfoMeshCoP(GetInstance(), "commissioner inactive");
}

Leader &Leader::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Leader &leader = *static_cast<Leader *>(aContext.GetContext());
#else
    Leader &leader = otGetThreadNetif().GetLeader();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return leader;
}

}  // namespace MeshCoP
}  // namespace ot

#endif // OPENTHREAD_FTD

