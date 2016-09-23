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

#include <stdio.h>

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <meshcop/leader.hpp>
#include <platform/random.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

#ifdef WINDOWS_LOGGING
#include "leader.tmh"
#endif

namespace Thread {
namespace MeshCoP {

Leader::Leader(ThreadNetif &aThreadNetif):
    mPetition(OPENTHREAD_URI_LEADER_PETITION, HandlePetition, this),
    mKeepAlive(OPENTHREAD_URI_LEADER_KEEP_ALIVE, HandleKeepAlive, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mNetworkData(aThreadNetif.GetNetworkDataLeader()),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this)
{
    mCoapServer.AddResource(mPetition);
    mCoapServer.AddResource(mKeepAlive);
}

void Leader::HandlePetition(void *aContext, Coap::Header &aHeader, Message &aMessage,
                            const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandlePetition(aHeader, aMessage, aMessageInfo);
}

void Leader::HandlePetition(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    CommissioningData data;
    CommissionerIdTlv commissionerId;
    StateTlv::State state = StateTlv::kReject;

    otLogInfoMeshCoP("received petition\r\n");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerId, sizeof(commissionerId), commissionerId));

    VerifyOrExit(!mTimer.IsRunning(), ;);

    data.mBorderAgentLocator.Init();
    data.mBorderAgentLocator.SetBorderAgentLocator(HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));

    data.mCommissionerSessionId.Init();
    data.mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    data.mSteeringData.Init();
    data.mSteeringData.SetLength(1);
    data.mSteeringData.Clear();

    SuccessOrExit(mNetworkData.SetCommissioningData(reinterpret_cast<uint8_t *>(&data), data.GetLength()));

    mCommissionerId = commissionerId;

    state = StateTlv::kAccept;
    mTimer.Start(Timer::SecToMsec(kTimeoutLeaderPetition));

exit:
    (void)aMessageInfo;
    SendPetitionResponse(aHeader, aMessageInfo, state);
}

ThreadError Leader::SendPetitionResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                         StateTlv::State aState)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;
    Message *message;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    responseHeader.Init();
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    if (mTimer.IsRunning())
    {
        SuccessOrExit(error = message->Append(&mCommissionerId, sizeof(mCommissionerId)));
    }

    if (aState == StateTlv::kAccept)
    {
        sessionId.Init();
        sessionId.SetCommissionerSessionId(mSessionId);
        SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    }

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent petition response\r\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Leader::HandleKeepAlive(void *aContext, Coap::Header &aHeader, Message &aMessage,
                             const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleKeepAlive(aHeader, aMessage, aMessageInfo);
}

void Leader::HandleKeepAlive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    StateTlv state;
    CommissionerSessionIdTlv sessionId;
    StateTlv::State responseState;

    otLogInfoMeshCoP("received keep alive\r\n");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kState, sizeof(state), state));
    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));

    if (sessionId.GetCommissionerSessionId() != mSessionId)
    {
        responseState = StateTlv::kReject;
    }
    else if (state.GetState() != StateTlv::kAccept)
    {
        responseState = StateTlv::kReject;
        mTimer.Stop();
        mNetworkData.SetCommissioningData(NULL, 0);
        otLogInfoMeshCoP("commissioner inactive\r\n");
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

ThreadError Leader::SendKeepAliveResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                          StateTlv::State aState)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    StateTlv state;
    Message *message;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    responseHeader.Init();
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent keep alive response\r\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Leader::HandleTimer(void *aContext)
{
    static_cast<Leader *>(aContext)->HandleTimer();
}

void Leader::HandleTimer(void)
{
    otLogInfoMeshCoP("commissioner inactive\r\n");
    mNetworkData.SetCommissioningData(NULL, 0);
}

}  // namespace MeshCoP
}  // namespace Thread
