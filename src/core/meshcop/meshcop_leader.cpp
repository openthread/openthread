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

#include "meshcop_leader.hpp"

#if OPENTHREAD_FTD

#include <stdio.h>

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("MeshCoPLeader");

Leader::Leader(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mPetition(UriPath::kLeaderPetition, Leader::HandlePetition, this)
    , mKeepAlive(UriPath::kLeaderKeepAlive, Leader::HandleKeepAlive, this)
    , mTimer(aInstance, HandleTimer)
    , mDelayTimerMinimal(DelayTimerTlv::kDelayTimerMinimal)
    , mSessionId(Random::NonCrypto::GetUint16())
{
    Get<Tmf::Agent>().AddResource(mPetition);
    Get<Tmf::Agent>().AddResource(mKeepAlive);
}

void Leader::HandlePetition(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandlePetition(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void Leader::HandlePetition(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    CommissioningData data;
    CommissionerIdTlv commissionerId;
    StateTlv::State   state = StateTlv::kReject;

    LogInfo("received petition");

    VerifyOrExit(Get<Mle::MleRouter>().IsRoutingLocator(aMessageInfo.GetPeerAddr()));
    SuccessOrExit(Tlv::FindTlv(aMessage, commissionerId));

    if (mTimer.IsRunning())
    {
        VerifyOrExit((commissionerId.GetCommissionerIdLength() == mCommissionerId.GetCommissionerIdLength()) &&
                     (!strncmp(commissionerId.GetCommissionerId(), mCommissionerId.GetCommissionerId(),
                               commissionerId.GetCommissionerIdLength())));

        ResignCommissioner();
    }

    data.mBorderAgentLocator.Init();
    data.mBorderAgentLocator.SetBorderAgentLocator(aMessageInfo.GetPeerAddr().GetIid().GetLocator());

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
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));

    if (mTimer.IsRunning())
    {
        SuccessOrExit(error = mCommissionerId.AppendTo(*message));
    }

    if (aState == StateTlv::kAccept)
    {
        SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, mSessionId));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

    LogInfo("sent petition response");

exit:
    FreeMessageOnError(message, error);
    LogError("send petition response", error);
}

void Leader::HandleKeepAlive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleKeepAlive(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void Leader::HandleKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint8_t                state;
    uint16_t               sessionId;
    BorderAgentLocatorTlv *borderAgentLocator;
    StateTlv::State        responseState;

    LogInfo("received keep alive");

    SuccessOrExit(Tlv::Find<StateTlv>(aMessage, state));

    SuccessOrExit(Tlv::Find<CommissionerSessionIdTlv>(aMessage, sessionId));

    borderAgentLocator =
        As<BorderAgentLocatorTlv>(Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));

    if ((borderAgentLocator == nullptr) || (sessionId != mSessionId))
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
        uint16_t rloc = aMessageInfo.GetPeerAddr().GetIid().GetLocator();

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
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

    LogInfo("sent keep alive response");

exit:
    FreeMessageOnError(message, error);
    LogError("send keep alive response", error);
}

void Leader::SendDatasetChanged(const Ip6::Address &aAddress)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message *  message;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kDatasetChanged);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aAddress);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("sent dataset changed");

exit:
    FreeMessageOnError(message, error);
    LogError("send dataset changed", error);
}

Error Leader::SetDelayTimerMinimal(uint32_t aDelayTimerMinimal)
{
    Error error = kErrorNone;
    VerifyOrExit((aDelayTimerMinimal != 0 && aDelayTimerMinimal < DelayTimerTlv::kDelayTimerDefault),
                 error = kErrorInvalidArgs);
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
    aTimer.Get<Leader>().HandleTimer();
}

void Leader::HandleTimer(void)
{
    VerifyOrExit(Get<Mle::MleRouter>().IsLeader());

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

    LogInfo("commissioner inactive");
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
