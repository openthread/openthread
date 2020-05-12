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

#include "meshcop_leader.hpp"

#include <stdio.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
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
    , mSessionId(Random::NonCrypto::GetUint16())
{
    Get<Coap::Coap>().AddResource(mPetition);
    Get<Coap::Coap>().AddResource(mKeepAlive);
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

    VerifyOrExit(Get<Mle::MleRouter>().IsRoutingLocator(aMessageInfo.GetPeerAddr()), OT_NOOP);
    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerId, sizeof(commissionerId), commissionerId));

    if (mTimer.IsRunning())
    {
        VerifyOrExit((commissionerId.GetCommissionerIdLength() == mCommissionerId.GetCommissionerIdLength()) &&
                         (!strncmp(commissionerId.GetCommissionerId(), mCommissionerId.GetCommissionerId(),
                                   commissionerId.GetCommissionerIdLength())),
                     OT_NOOP);

        ResignCommissioner();
    }

    data.mBorderAgentLocator.Init();
    data.mBorderAgentLocator.SetBorderAgentLocator(aMessageInfo.GetPeerAddr().GetLocator());

    data.mCommissionerSessionId.Init();
    data.mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    data.mSteeringData.Init();
    data.mSteeringData.SetLength(1);
    data.mSteeringData.Clear();

    SuccessOrExit(
        Get<NetworkData::Leader>().SetCommissioningData(reinterpret_cast<uint8_t *>(&data), data.GetLength()));

    mCommissionerId = commissionerId;

    if (mCommissionerId.GetLength() > CommissionerIdTlv::kMaxLength)
    {
        mCommissionerId.SetLength(CommissionerIdTlv::kMaxLength);
    }

    state = StateTlv::kAccept;
    mTimer.Start(Time::SecToMsec(kTimeoutLeaderPetition));

exit:
    SendPetitionResponse(aMessage, aMessageInfo, state);
}

void Leader::SendPetitionResponse(const Coap::Message &   aRequest,
                                  const Ip6::MessageInfo &aMessageInfo,
                                  StateTlv::State         aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, Tlv::kState, static_cast<uint8_t>(aState)));

    if (mTimer.IsRunning())
    {
        SuccessOrExit(error = mCommissionerId.AppendTo(*message));
    }

    if (aState == StateTlv::kAccept)
    {
        SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kCommissionerSessionId, mSessionId));
    }

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent petition response");

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoMeshCoP("Failed to send petition response: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }
}

void Leader::HandleKeepAlive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleKeepAlive(*static_cast<Coap::Message *>(aMessage),
                                                     *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint8_t                state;
    uint16_t               sessionId;
    BorderAgentLocatorTlv *borderAgentLocator;
    StateTlv::State        responseState;

    otLogInfoMeshCoP("received keep alive");

    SuccessOrExit(Tlv::ReadUint8Tlv(aMessage, Tlv::kState, state));

    SuccessOrExit(Tlv::ReadUint16Tlv(aMessage, Tlv::kCommissionerSessionId, sessionId));

    borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(
        Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));

    if ((borderAgentLocator == NULL) || (sessionId != mSessionId))
    {
        responseState = StateTlv::kReject;
    }
    else if (state != StateTlv::kAccept)
    {
        responseState = StateTlv::kReject;
        ResignCommissioner();
    }
    else
    {
        uint16_t rloc = aMessageInfo.GetPeerAddr().GetLocator();

        if (borderAgentLocator->GetBorderAgentLocator() != rloc)
        {
            borderAgentLocator->SetBorderAgentLocator(rloc);
            Get<NetworkData::Leader>().IncrementVersion();
        }

        responseState = StateTlv::kAccept;
        mTimer.Start(Time::SecToMsec(kTimeoutLeaderPetition));
    }

    SendKeepAliveResponse(aMessage, aMessageInfo, responseState);

exit:
    return;
}

void Leader::SendKeepAliveResponse(const Coap::Message &   aRequest,
                                   const Ip6::MessageInfo &aMessageInfo,
                                   StateTlv::State         aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, Tlv::kState, static_cast<uint8_t>(aState)));

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent keep alive response");

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send keep alive response: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }
}

void Leader::SendDatasetChanged(const Ip6::Address &aAddress)
{
    otError          error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;
    Coap::Message *  message;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_DATASET_CHANGED));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset changed");

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send dataset changed: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }
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
    VerifyOrExit(Get<Mle::MleRouter>().IsLeader(), OT_NOOP);

    ResignCommissioner();

exit:
    return;
}

void Leader::SetEmptyCommissionerData(void)
{
    CommissionerSessionIdTlv mCommissionerSessionId;

    mCommissionerSessionId.Init();
    mCommissionerSessionId.SetCommissionerSessionId(++mSessionId);

    IgnoreError(Get<NetworkData::Leader>().SetCommissioningData(reinterpret_cast<uint8_t *>(&mCommissionerSessionId),
                                                                sizeof(Tlv) + mCommissionerSessionId.GetLength()));
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
