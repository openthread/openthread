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

#include "coap/coap.hpp"
#include "coap/coap_message.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime      = 10 * 1000;
static constexpr uint32_t kAttachAsRouterTime   = 200 * 1000;
static constexpr uint32_t kAttachAsSedTime      = 10 * 1000;
static constexpr uint32_t kMlrRegistrationTime  = 10 * 1000;
static constexpr uint32_t kSettleTime           = 5 * 1000;
static constexpr uint32_t kIdleCheckWindow      = 3 * 1000;
static constexpr uint32_t kShortIdleCheckWindow = 500;
static constexpr uint32_t kChildUpdateWaitTime  = 150;
static constexpr uint32_t kFastPollPeriod       = 188;

static uint32_t GetTxDataPollCount(Node &aNode) { return aNode.Get<Mac::Mac>().GetCounters().mTxDataPoll; }

struct Context
{
    Context(void)
        : mMlrReqCount(0)
        , mSourceRloc16(Mle::kInvalidRloc16)
        , mDropMlrReq(false)
    {
    }

    uint32_t     mMlrReqCount;
    uint16_t     mSourceRloc16;
    bool         mDropMlrReq;
    Ip6::Address mTargetAddress;
};

static Error BrCoapInterceptor(void *aContext, const Coap::Msg &aMsg)
{
    Error                              error = kErrorNone;
    Mlr::AddressArray                  addresses;
    Context                           *context = static_cast<Context *>(aContext);
    Coap::Message::UriPathStringBuffer uriPath;

    VerifyOrQuit(context != nullptr);

    VerifyOrExit(aMsg.IsPostRequest());

    SuccessOrExit(aMsg.mMessage.ReadUriPathOptions(uriPath));
    VerifyOrExit(UriFromPath(uriPath) == kUriMlr);

    if (context->mSourceRloc16 != Mle::kInvalidRloc16)
    {
        VerifyOrExit(aMsg.mMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator() &&
                     (aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator() == context->mSourceRloc16));
    }

    SuccessOrExit(Ip6AddressesTlv::FindIn(aMsg.mMessage, addresses));

    Log("   Received MLR.req with %u addresses:", addresses.GetLength());

    for (const Ip6::Address &address : addresses)
    {
        Log("   - %s", address.ToString().AsCString());
    }

    if (addresses.Contains(context->mTargetAddress))
    {
        context->mMlrReqCount++;
    }

    if (context->mDropMlrReq)
    {
        error = kErrorDrop;
    }

exit:
    return error;
}

void TestMlrRedundant(void)
{
    /**
     * This test verifies that the MLR manager does not send redundant registration
     * requests when multiple entities (a parent router and its children) subscribe
     * to the same multicast address. It sets up a scenario where the total number
     * of subscribed addresses exceeds the capacity of a single MLR.req CoAP message,
     * ensuring that the state is properly managed and shared addresses are registered
     * exactly once with the Primary Backbone Router.
     */

    Core         nexus;
    Node        &br     = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &sed1   = nexus.CreateNode();
    Node        &sed2   = nexus.CreateNode();
    Node        &sed3   = nexus.CreateNode();
    Ip6::Address sharedAddress;
    Context      context;

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form topology");

    AllowLinkBetween(br, router);
    AllowLinkBetween(router, sed1);
    AllowLinkBetween(router, sed2);
    AllowLinkBetween(router, sed3);

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    router.Join(br, Node::kAsFtd);
    nexus.AdvanceTime(kAttachAsRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    sed1.Join(router, Node::kAsSed);
    sed2.Join(router, Node::kAsSed);
    sed3.Join(router, Node::kAsSed);

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(5 * Time::kOneSecondInMsec));
    SuccessOrQuit(sed2.Get<DataPollSender>().SetExternalPollPeriod(5 * Time::kOneSecondInMsec));
    SuccessOrQuit(sed3.Get<DataPollSender>().SetExternalPollPeriod(5 * Time::kOneSecondInMsec));

    nexus.AdvanceTime(kAttachAsSedTime);

    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed2.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed3.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sharedAddress.FromString("ff04::1"));
    context.mTargetAddress = sharedAddress;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set an interceptor on leader to count incoming MLR.req messages");
    br.Get<Tmf::Agent>().SetInterceptor(BrCoapInterceptor, &context);

    Log("Router and SEDs subscribe to the same multicast address");
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(sharedAddress));
    SuccessOrQuit(sed1.Get<Ip6::Netif>().SubscribeExternalMulticast(sharedAddress));
    SuccessOrQuit(sed2.Get<Ip6::Netif>().SubscribeExternalMulticast(sharedAddress));
    SuccessOrQuit(sed3.Get<Ip6::Netif>().SubscribeExternalMulticast(sharedAddress));

    Log("Router subscribes to 14 more unique multicast addresses to trigger MLR IP6 Addresses TLV limits");

    for (uint8_t i = 2; i <= 15; i++)
    {
        Ip6::Address ma;

        ma                = sharedAddress;
        ma.mFields.m8[15] = i;

        SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma));
    }

    nexus.AdvanceTime(15 * Time::kOneSecondInMsec);

    Log("Enable BBR on wait for MLR registrations");
    br.Get<BackboneRouter::Local>().SetEnabled(true);

    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("Verify the shared address was registered exactly once (no redundant registrations)");

    VerifyOrQuit(context.mMlrReqCount == 1);
}

void TestMlrState(void)
{
    /**
     * This test verifies the state machine transitions and timing behavior of the MLR manager.
     */

    Core                   nexus;
    Node                  &br     = nexus.CreateNode();
    Node                  &router = nexus.CreateNode();
    Ip6::Address           firstAddress;
    Ip6::Address           newAddress;
    BackboneRouter::Config config;
    Context                context;

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form topology");

    AllowLinkBetween(br, router);
    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    router.Join(br, Node::kAsFtd);
    nexus.AdvanceTime(kAttachAsRouterTime);

    SuccessOrQuit(firstAddress.FromString("ff04::1"));
    context.mTargetAddress = firstAddress;
    br.Get<Tmf::Agent>().SetInterceptor(BrCoapInterceptor, &context);

    Log("Enable BBR and verify initial registration");
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(firstAddress));

    nexus.AdvanceTime(kMlrRegistrationTime);
    VerifyOrQuit(context.mMlrReqCount == 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Add new address and verify quick registration");

    SuccessOrQuit(newAddress.FromString("ff04::100"));
    context.mTargetAddress = newAddress;
    context.mMlrReqCount   = 0;

    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(newAddress));

    // Verify it is registered quickly (within short aggregation window)
    nexus.AdvanceTime(1000);
    VerifyOrQuit(context.mMlrReqCount == 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Update MLR Timeout and verify renewal time update");

    config = br.Get<BackboneRouter::Leader>().GetConfig();

    Log("Initial MLR timeout: %lu", ToUlong(br.Get<BackboneRouter::Leader>().GetConfig().GetMlrTimeout()));

    // We set a short MLR timeout (300 seconds is the minimum).
    config.mReregistrationDelay = 10;
    config.mMlrTimeout          = 300;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));

    context.mTargetAddress = firstAddress;
    context.mMlrReqCount   = 0;

    Log("Wait for renewal registration");

    // Renewal happens between (0.5 * 300) = 150s and (300 - 9) = 291s.
    nexus.AdvanceTime(300 * Time::kOneSecondInMsec);

    VerifyOrQuit(context.mMlrReqCount >= 1);
    Log("Renewal registration successful");
}

void TestMedMulticastRegistrationOnAttach(void)
{
    /**
     * This test verifies that an rx-on-when-idle MED includes subscribed
     * larger-than-realm-local multicast addresses in its MLE Child ID Request
     * during initial attach and re-attach to a Thread 1.2+ parent.
     */

    Core         nexus;
    Node        &leader = nexus.CreateNode();
    Node        &med    = nexus.CreateNode();
    Ip6::Address mcastAddr1;
    Ip6::Address mcastAddr2;
    Ip6::Address realmLocalMcastAddr;
    const Child *child;

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, med);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(mcastAddr1.FromString("ff05::1234"));
    SuccessOrQuit(mcastAddr2.FromString("ff05::abcd"));
    SuccessOrQuit(realmLocalMcastAddr.FromString("ff03::5678"));

    Log("Subscribe MED to multicast addresses before initial attach");
    SuccessOrQuit(med.Get<Ip6::Netif>().SubscribeExternalMulticast(mcastAddr1));
    SuccessOrQuit(med.Get<Ip6::Netif>().SubscribeExternalMulticast(realmLocalMcastAddr));

    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().IsMinimalEndDevice());

    child = leader.Get<ChildTable>().FindChild(med.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(child->HasIp6Address(mcastAddr1));
    VerifyOrQuit(!child->HasIp6Address(realmLocalMcastAddr));

    Log("Detach MED, change multicast subscription while detached, and verify re-attach registration");
    SuccessOrQuit(med.Get<Mle::Mle>().BecomeDetached());
    SuccessOrQuit(med.Get<Ip6::Netif>().UnsubscribeExternalMulticast(mcastAddr1));
    SuccessOrQuit(med.Get<Ip6::Netif>().SubscribeExternalMulticast(mcastAddr2));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    child = leader.Get<ChildTable>().FindChild(med.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(!child->HasIp6Address(mcastAddr1));
    VerifyOrQuit(child->HasIp6Address(mcastAddr2));
    VerifyOrQuit(!child->HasIp6Address(realmLocalMcastAddr));
}

void TestSedMlrFastPolls(void)
{
    /**
     * This test verifies that a Thread 1.2 SED attached to a Thread 1.1 parent
     * requests fast data polls when sending `MLR.req` and cancels
     * `kRequesterMlrManager` both when receiving `MLR.rsp` and when leaving
     * `kStateRegistering` on PBBR removal or update while staying attached.
     */

    Core                   nexus;
    Node                  &br  = nexus.CreateNode();
    Node                  &sed = nexus.CreateNode();
    Ip6::Address           mcastAddr1;
    Ip6::Address           mcastAddr2;
    BackboneRouter::Config config;
    Context                context;
    uint32_t               pollsBefore;
    uint32_t               pollsAfter;

    br.SetName("BR");
    sed.SetName("SED");

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(br, sed);

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    sed.Join(br, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSedTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsRxOnWhenIdle());

    // Simulate a Thread 1.1 parent so the Thread 1.2 SED sends MLR.req itself.
    sed.Get<Mle::Mle>().GetParent().SetVersion(kThreadVersion1p1);

    SuccessOrQuit(mcastAddr1.FromString("ff04::1"));
    SuccessOrQuit(mcastAddr2.FromString("ff04::2"));

    SuccessOrQuit(sed.Get<Ip6::Netif>().SubscribeExternalMulticast(mcastAddr1));
    nexus.AdvanceTime(kSettleTime);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    context.mSourceRloc16  = sed.Get<Mle::Mle>().GetRloc16();
    context.mTargetAddress = mcastAddr1;
    context.mDropMlrReq    = false;
    br.Get<Tmf::Agent>().SetInterceptor(BrCoapInterceptor, &context);

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mReregistrationDelay = 1;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(Time::kOneSecondInMsec);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 1: SED requests fast polls on MLR.req and cancels them on MLR.rsp");

    SuccessOrQuit(sed.Get<DataPollSender>().SendDataPoll());
    nexus.AdvanceTime(500);
    VerifyOrQuit(sed.Get<BackboneRouter::Leader>().HasPrimary());

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(context.mMlrReqCount == 1);

    pollsAfter = GetTxDataPollCount(sed);
    VerifyOrQuit(pollsAfter > pollsBefore);
    VerifyOrQuit(pollsAfter - pollsBefore < DataPollSender::kDefaultFastPolls);

    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsAfter);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 2: Leaving kStateRegistering on PBBR removal cancels kRequesterMlrManager");

    context.mTargetAddress = mcastAddr2;
    context.mMlrReqCount   = 0;
    context.mDropMlrReq    = true;

    SuccessOrQuit(sed.Get<Ip6::Netif>().SubscribeExternalMulticast(mcastAddr2));

    for (uint32_t elapsed = 0; (elapsed < 1000) && (context.mMlrReqCount == 0); elapsed += 10)
    {
        nexus.AdvanceTime(10);
    }

    VerifyOrQuit(context.mMlrReqCount == 1);

    // Let the MLE Child Update exchange triggered by multicast subscription complete.
    nexus.AdvanceTime(kChildUpdateWaitTime);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    br.Get<BackboneRouter::Local>().SetEnabled(false);

    for (uint32_t elapsed = 0; (elapsed < 2000) && sed.Get<BackboneRouter::Leader>().HasPrimary(); elapsed += 10)
    {
        nexus.AdvanceTime(10);
    }

    VerifyOrQuit(!sed.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    // Let the MLE Child Update triggered by Network Data change complete.
    nexus.AdvanceTime(kChildUpdateWaitTime);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 3: Leaving kStateRegistering on PBBR update cancels kRequesterMlrManager");

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(Time::kOneSecondInMsec);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());

    context.mTargetAddress = mcastAddr1;
    context.mMlrReqCount   = 0;
    context.mDropMlrReq    = true;

    SuccessOrQuit(sed.Get<DataPollSender>().SendDataPoll());

    for (uint32_t elapsed = 0; (elapsed < 2000) && (context.mMlrReqCount == 0); elapsed += 10)
    {
        nexus.AdvanceTime(10);
    }

    VerifyOrQuit(context.mMlrReqCount == 1);

    // Add a concurrent fast poll requester before triggering PBBR update.
    sed.Get<DataPollSender>().RequestFastPolls(DataPollSender::kRequesterUser, 10);

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mSequenceNumber++;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));

    for (uint32_t elapsed = 0; (elapsed < 2000) && (sed.Get<BackboneRouter::Leader>().GetConfig().GetSequenceNumber() !=
                                                    config.mSequenceNumber);
         elapsed += 10)
    {
        nexus.AdvanceTime(10);
    }

    VerifyOrQuit(sed.Get<BackboneRouter::Leader>().GetConfig().GetSequenceNumber() == config.mSequenceNumber);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    // Let the MLE Child Update triggered by Network Data change complete.
    nexus.AdvanceTime(kChildUpdateWaitTime);

    // Verify kRequesterUser still keeps fast polling active after MLR manager cancelled its request.
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    // Cancelling kRequesterUser must stop fast polling immediately without being blocked by kRequesterMlrManager.
    sed.Get<DataPollSender>().CancelFastPolls(DataPollSender::kRequesterUser);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kShortIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    // Allow the subsequent re-registration to complete cleanly.
    context.mDropMlrReq  = false;
    context.mMlrReqCount = 0;
    nexus.AdvanceTime(kMlrRegistrationTime);
    VerifyOrQuit(context.mMlrReqCount == 1);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMlrRedundant();
    ot::Nexus::TestMlrState();
    ot::Nexus::TestMedMulticastRegistrationOnAttach();
    ot::Nexus::TestSedMlrFastPolls();
    printf("All tests passed\n");
    return 0;
}
