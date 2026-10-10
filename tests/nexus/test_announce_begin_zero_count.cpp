/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kPeriod                = 1000;
static constexpr uint8_t  kBoundedCount          = 4;
static constexpr uint16_t kCommissionerSessionId = 0x1234;

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinTime        = 10 * 1000;
static constexpr uint32_t kResponseTime    = 100;
static constexpr uint32_t kIndefiniteWait  = 200 * kPeriod;

struct ResponseContext
{
    Coap::Code mExpectedCode;
    bool       mReceived;
};

static void HandleResponse(void *aContext, Coap::Msg *aMsg, Error aError)
{
    ResponseContext &context = *static_cast<ResponseContext *>(aContext);

    SuccessOrQuit(aError);
    VerifyOrQuit(aMsg != nullptr);
    VerifyOrQuit(aMsg->mMessage.ReadCode() == context.mExpectedCode);
    context.mReceived = true;
}

static void SendAnnounceBeginRequest(Node      &aSender,
                                     Node      &aReceiver,
                                     uint32_t   aMask,
                                     uint8_t    aCount,
                                     uint16_t   aPeriod,
                                     Coap::Code aExpectedCode,
                                     bool       aExpectedRunning)
{
    Tmf::Agent     &agent   = aSender.Get<Tmf::Agent>();
    Coap::Message  *message = agent.AllocateAndInitPriorityConfirmablePostMessage(kUriAnnounceBegin);
    ResponseContext context = {aExpectedCode, false};

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, kCommissionerSessionId));
    SuccessOrQuit(MeshCoP::ChannelMaskTlv::AppendTo(*message, aMask));
    SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::CountTlv>(*message, aCount));
    SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::PeriodTlv>(*message, aPeriod));
    SuccessOrQuit(
        agent.SendMessageTo(*message, aReceiver.Get<Mle::Mle>().GetMeshLocalRloc(), HandleResponse, &context));

    Core::Get().AdvanceTime(kResponseTime);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(aReceiver.Get<AnnounceBeginServer>().IsRunning() == aExpectedRunning);
}

void Test(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Form a network and join the receiver");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsAttached());

    uint32_t mask            = static_cast<uint32_t>(1) << router.Get<Mac::Mac>().GetPanChannel();
    uint32_t unsupportedMask = router.Get<Mac::Mac>().GetSupportedChannelMask().GetMask() & ~mask;

    VerifyOrQuit(unsupportedMask != 0);
    router.Get<Mac::Mac>().SetSupportedChannelMask(Mac::ChannelMask(mask));

    Log("Reject a zero Count");
    SendAnnounceBeginRequest(leader, router, mask, 0, kPeriod, Coap::kCodeBadRequest, false);
    nexus.AdvanceTime(kIndefiniteWait);
    VerifyOrQuit(!router.Get<AnnounceBeginServer>().IsRunning());

    Log("Reject a zero Period");
    SendAnnounceBeginRequest(leader, router, mask, kBoundedCount, 0, Coap::kCodeBadRequest, false);

    Log("Reject an empty channel mask");
    SendAnnounceBeginRequest(leader, router, 0, kBoundedCount, kPeriod, Coap::kCodeBadRequest, false);

    Log("Reject a mask with no supported channels");
    SendAnnounceBeginRequest(leader, router, unsupportedMask, kBoundedCount, kPeriod, Coap::kCodeBadRequest, false);

    Log("Accept a non-zero Count and preserve the sender while busy");
    SendAnnounceBeginRequest(leader, router, mask, kBoundedCount, kPeriod, Coap::kCodeChanged, true);
    SendAnnounceBeginRequest(leader, router, 0, 0, 0, Coap::kCodeServiceUnavailable, true);

    nexus.AdvanceTime((kBoundedCount - 1) * kPeriod - 3 * kResponseTime);
    VerifyOrQuit(router.Get<AnnounceBeginServer>().IsRunning());
    nexus.AdvanceTime(2 * kResponseTime);
    VerifyOrQuit(!router.Get<AnnounceBeginServer>().IsRunning());

    Log("Accept a mixed channel mask after the sender stops");
    SendAnnounceBeginRequest(leader, router, mask | unsupportedMask, 1, kPeriod, Coap::kCodeChanged, false);

    nexus.AdvanceTime(kIndefiniteWait);
    VerifyOrQuit(!router.Get<AnnounceBeginServer>().IsRunning());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test();
    printf("All tests passed\n");
    return 0;
}
