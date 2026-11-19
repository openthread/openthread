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

static constexpr uint32_t kFormNetworkTime      = 13 * 1000;
static constexpr uint32_t kAttachAsChildTime    = 10 * 1000;
static constexpr uint32_t kSettleTime           = 5 * 1000;
static constexpr uint32_t kIdleCheckWindow      = 3 * 1000;
static constexpr uint32_t kStepTime             = 10;
static constexpr uint32_t kMaxWaitForUpdateTime = 1000;
static constexpr uint32_t kMaxWaitForTimeout    = 3 * 1000;
static constexpr uint32_t kFirstFastPollWait    = 100;
static constexpr uint32_t kFastPollPeriod       = 188;
static constexpr uint32_t kFastPollCheckWindow  = 300; // Mid-cycle between 2nd (~198 ms) and 3rd (~386 ms) fast poll.
static constexpr uint16_t kSilentServerPort     = 12340;
static constexpr uint16_t kSrpServicePort1      = 12345;
static constexpr uint16_t kSrpServicePort2      = 12346;

static const char kSrpHostName[]      = "my-host";
static const char kSrpHostAddress[]   = "2001::1";
static const char kSrpServiceType[]   = "_ipps._tcp";
static const char kSrpInstanceName1[] = "my-service-1";
static const char kSrpInstanceName2[] = "my-service-2";

struct TimeoutContext
{
    Node    *mSed;
    bool     mTimedOut;
    uint32_t mPollsAtTimeout;
};

static uint32_t GetTxDataPollCount(Node &aNode) { return aNode.Get<Mac::Mac>().GetCounters().mTxDataPoll; }

static void HandleSilentServerReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    (*static_cast<uint32_t *>(aContext))++;
}

static void HandleSrpClientCallback(otError                    aError,
                                    const otSrpClientHostInfo *aHostInfo,
                                    const otSrpClientService  *aServices,
                                    const otSrpClientService  *aRemovedServices,
                                    void                      *aContext)
{
    OT_UNUSED_VARIABLE(aHostInfo);
    OT_UNUSED_VARIABLE(aServices);
    OT_UNUSED_VARIABLE(aRemovedServices);

    TimeoutContext *context = static_cast<TimeoutContext *>(aContext);

    if (aError == kErrorResponseTimeout)
    {
        DataPollSender &sender = context->mSed->Get<DataPollSender>();

        context->mTimedOut = true;

        // Cancel `kRequesterUser` before calling `Stop()`. If `HandleTimer()` already cancelled
        // `kRequesterSrpClient`, this clears `mRemainingFastPolls` to 0 so the subsequent 1-poll
        // request schedules exactly 1 poll rather than preserving the larger remaining count.
        sender.CancelFastPolls(DataPollSender::kRequesterUser);
        sender.RequestFastPolls(DataPollSender::kRequesterUser, 1);
        context->mPollsAtTimeout = GetTxDataPollCount(*context->mSed);

        // Stop client after re-requesting `kRequesterUser` so the scheduled retry does not
        // interfere and `Stop()` cannot mask a missing cancel in `HandleTimer()`.
        context->mSed->Get<Srp::Client>().Stop();
    }
}

static void WaitForSrpUpdate(Core &aNexus, const uint32_t &aRxCount)
{
    uint32_t initialCount = aRxCount;

    for (uint32_t elapsed = 0; elapsed < kMaxWaitForUpdateTime; elapsed += kStepTime)
    {
        aNexus.AdvanceTime(kStepTime);

        if (aRxCount > initialCount)
        {
            break;
        }
    }

    VerifyOrQuit(aRxCount > initialCount);
}

static void PrepareService(Srp::Client::Service &aService, const char *aInstanceName, uint16_t aPort)
{
    ClearAllBytes(aService);
    aService.mName         = kSrpServiceType;
    aService.mInstanceName = aInstanceName;
    aService.mPort         = aPort;
}

void TestSrpClientFastPolls(void)
{
    Core                 nexus;
    Node                &leader        = nexus.CreateNode();
    Node                &sed           = nexus.CreateNode();
    Srp::Client         &srpClient     = sed.Get<Srp::Client>();
    DataPollSender      &sender        = sed.Get<DataPollSender>();
    uint32_t             rxUpdateCount = 0;
    Ip6::Udp::Socket     silentServer(leader, HandleSilentServerReceive, &rxUpdateCount);
    Ip6::SockAddr        silentServerSockAddr;
    Ip6::SockAddr        srpServerSockAddr;
    Ip6::Address         hostAddress;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    TimeoutContext       timeoutContext;
    uint32_t             pollsBefore;

    leader.SetName("LEADER");
    sed.SetName("SED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    AllowLinkBetween(leader, sed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(silentServer.Open(Ip6::kNetifThreadInternal));
    SuccessOrQuit(silentServer.Bind(kSilentServerPort));

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    srpClient.DisableAutoStartMode();

    nexus.AdvanceTime(kSettleTime);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    silentServerSockAddr.Clear();
    silentServerSockAddr.SetAddress(leader.Get<Mle::Mle>().GetMeshLocalEid());
    silentServerSockAddr.SetPort(kSilentServerPort);

    SuccessOrQuit(hostAddress.FromString(kSrpHostAddress));
    PrepareService(service1, kSrpInstanceName1, kSrpServicePort1);
    PrepareService(service2, kSrpInstanceName2, kSrpServicePort2);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 1: Stop() while in kStateUpdating cancels kRequesterSrpClient immediately");

    SuccessOrQuit(srpClient.SetHostName(kSrpHostName));
    SuccessOrQuit(srpClient.SetHostAddresses(&hostAddress, 1));
    SuccessOrQuit(srpClient.AddService(service1));
    SuccessOrQuit(srpClient.Start(silentServerSockAddr));

    WaitForSrpUpdate(nexus, rxUpdateCount);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    srpClient.Stop();
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    // Verify `Stop()` clears only `kRequesterSrpClient` and does not block `kRequesterUser`.
    SuccessOrQuit(srpClient.Start(silentServerSockAddr));
    WaitForSrpUpdate(nexus, rxUpdateCount);
    nexus.AdvanceTime(kFirstFastPollWait);

    sender.RequestFastPolls(DataPollSender::kRequesterUser, 15);
    srpClient.Stop();

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(2 * kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 2: ClearHostAndServices() while in kStateUpdating cancels kRequesterSrpClient immediately");

    SuccessOrQuit(srpClient.Start(silentServerSockAddr));
    WaitForSrpUpdate(nexus, rxUpdateCount);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    sender.RequestFastPolls(DataPollSender::kRequesterUser, 15);
    srpClient.ClearHostAndServices();

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(2 * kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    // Also verify `ClearHostAndServices()` stops fast polls immediately with no other active requester.
    SuccessOrQuit(srpClient.SetHostName(kSrpHostName));
    SuccessOrQuit(srpClient.SetHostAddresses(&hostAddress, 1));
    PrepareService(service1, kSrpInstanceName1, kSrpServicePort1);
    SuccessOrQuit(srpClient.AddService(service1));

    WaitForSrpUpdate(nexus, rxUpdateCount);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    srpClient.ClearHostAndServices();
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 3: kStateUpdating timeout in HandleTimer() cancels kRequesterSrpClient immediately");

    ClearAllBytes(timeoutContext);
    timeoutContext.mSed = &sed;
    srpClient.SetCallback(HandleSrpClientCallback, &timeoutContext);

    SuccessOrQuit(srpClient.SetHostName(kSrpHostName));
    SuccessOrQuit(srpClient.SetHostAddresses(&hostAddress, 1));
    PrepareService(service1, kSrpInstanceName1, kSrpServicePort1);
    SuccessOrQuit(srpClient.AddService(service1));

    WaitForSrpUpdate(nexus, rxUpdateCount);

    sender.RequestFastPolls(DataPollSender::kRequesterUser, 20);

    for (uint32_t elapsed = 0; elapsed < kMaxWaitForTimeout; elapsed += kStepTime)
    {
        nexus.AdvanceTime(kStepTime);

        if (timeoutContext.mTimedOut)
        {
            break;
        }
    }

    VerifyOrQuit(timeoutContext.mTimedOut);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == timeoutContext.mPollsAtTimeout + 1);

    srpClient.SetCallback(nullptr, nullptr);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 4: Adding a service while in kStateUpdating keeps kRequesterSrpClient active");

    SuccessOrQuit(srpClient.Start(silentServerSockAddr));
    WaitForSrpUpdate(nexus, rxUpdateCount);

    // Wait for the first fast poll to finish so no poll is in flight when probing `mRemainingFastPolls`.
    nexus.AdvanceTime(kFirstFastPollWait);

    sender.RequestFastPolls(DataPollSender::kRequesterUser, 20);

    // Transitions `kStateUpdating -> kStateToUpdate` via `UpdateState()` while the first update is in flight.
    SuccessOrQuit(srpClient.AddService(service2));

    // Probe immediately before the `kStateToUpdate` TX jitter timer can fire: `CancelFastPolls(kRequesterUser)`
    // must keep `mRemainingFastPolls == 20` because `kRequesterSrpClient` is still active.
    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    sender.RequestFastPolls(DataPollSender::kRequesterUser, 1);
    pollsBefore = GetTxDataPollCount(sed);
    srpClient.Stop();

    nexus.AdvanceTime(kSettleTime);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 20);

    SuccessOrQuit(silentServer.Close());

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 5: Receiving an SRP response cancels kRequesterSrpClient immediately");

    leader.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kSettleTime);

    // Drain any queued Network Data update from `leader` before starting the fast-poll check.
    SuccessOrQuit(sender.SendDataPoll());
    nexus.AdvanceTime(kSettleTime);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    srpServerSockAddr.Clear();
    srpServerSockAddr.SetAddress(leader.Get<Mle::Mle>().GetMeshLocalEid());
    srpServerSockAddr.SetPort(leader.Get<Srp::Server>().GetPort());

    sender.RequestFastPolls(DataPollSender::kRequesterUser, 15);
    SuccessOrQuit(srpClient.Start(srpServerSockAddr));

    for (uint32_t elapsed = 0; elapsed < kMaxWaitForTimeout; elapsed += kStepTime)
    {
        nexus.AdvanceTime(kStepTime);

        if (srpClient.GetHostInfo().GetState() == Srp::Client::kRegistered)
        {
            break;
        }
    }

    VerifyOrQuit(srpClient.GetHostInfo().GetState() == Srp::Client::kRegistered);

    // Advance mid-cycle after the poll that delivered the SRP response so no poll is in flight when cancelling.
    nexus.AdvanceTime(kFirstFastPollWait);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(2 * kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) > pollsBefore);

    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpClientFastPolls();

    printf("All tests passed\n");
    return 0;
}
