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
#include "thread/child_supervision.hpp"
#include "thread/child_table.hpp"
#include "thread/discover_scanner.hpp"

namespace ot {
namespace Nexus {

void TestChildSupervision(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Check initial supervision interval");

    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == sed.Get<SupervisionListener>().GetInterval());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Change supervision interval on child");

    sed.Get<SupervisionListener>().SetInterval(20);
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetInterval() == 20);
    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == 20);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Change supervision check timeout on the child");

    sed.Get<SupervisionListener>().SetTimeout(25);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetTimeout() == 25);

    // Wait for multiple supervision intervals and ensure that child stays attached
    nexus.AdvanceTime(110 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Disable supervision check on child and block traffic from child on parent");

    sed.Get<SupervisionListener>().SetTimeout(0);

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    // sed is not in allowlist, so it will be blocked.

    uint32_t childTimeout = leader.Get<ChildTable>().GetChildAtIndex(0)->GetTimeout();

    nexus.AdvanceTime((childTimeout + 1) * 1000);
    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == 0);

    // Since supervision check is disabled on the child, it should continue to stay attached (since data polls are acked
    // by radio driver).
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Re-enable supervision check on child");

    sed.Get<SupervisionListener>().SetTimeout(25);

    nexus.AdvanceTime(35 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Disable allowlist on parent and re-attach");

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    nexus.AdvanceTime(30 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    sed.Get<SupervisionListener>().ResetCounter();
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Set the supervision interval to zero on child");

    sed.Get<SupervisionListener>().SetInterval(0);
    sed.Get<SupervisionListener>().SetTimeout(25);
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetInterval() == 0);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetTimeout() == 25);

    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == 0);
    }

    // Wait for multiple check timeouts. The child should still stay attached.
    nexus.AdvanceTime(100 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == 1);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

static void StartDiscoverScan(Node &aChild, Node &aParent)
{
    SuccessOrQuit(aChild.Get<Mle::DiscoverScanner>().Discover(
        Mac::ChannelMask(1UL << aParent.Get<Mac::Mac>().GetPanChannel()), Mac::kPanIdBroadcast, /* aJoiner */ false,
        /* aEnableFiltering */ false, /* aFilterIndexes */ nullptr, nullptr, nullptr));
}

void TestChildSupervisionWithDiscoveryScans(void)
{
    static constexpr uint16_t kCheckTimeout = 25;        // (seconds)
    static constexpr uint32_t kScanPeriod   = 10 * 1000; // (msec)
    static constexpr uint32_t kMaxWaitTime  = 60 * 1000; // (msec)

    Core     nexus;
    uint32_t elapsed = 0;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and attach SED");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));

    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Block traffic from child on parent while child runs periodic discovery scans");

    // Data polls from the child are still acked by the radio, so only the supervision check can detect the lost
    // parent. Scans more frequent than the check timeout must not postpone the detection.

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);

    while (sed.Get<Mle::Mle>().IsChild() && (elapsed < kMaxWaitTime))
    {
        StartDiscoverScan(sed, leader);

        nexus.AdvanceTime(kScanPeriod);
        elapsed += kScanPeriod;
    }

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionChangesDuringDiscoveryScan(void)
{
    static constexpr uint16_t kCheckTimeout = 25;        // (seconds)
    static constexpr uint32_t kScanWaitTime = 10 * 1000; // (msec)

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and attach SED");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));

    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);
    sed.Get<SupervisionListener>().ResetCounter();

    Log("---------------------------------------------------------------------------------------");
    Log("Disable the supervision check during a discovery scan");

    StartDiscoverScan(sed, leader);
    sed.Get<SupervisionListener>().SetTimeout(0);

    // Wait past the fire time the check timeout had before it was disabled.
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) + kScanWaitTime);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Run a discovery scan with the supervision check disabled");

    StartDiscoverScan(sed, leader);
    nexus.AdvanceTime(kScanWaitTime);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Enable the supervision check during a discovery scan");

    // The check timeout starts in full when enabled, so the end of the scan must not fire it from a stale fire time.

    StartDiscoverScan(sed, leader);
    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);
    nexus.AdvanceTime(kScanWaitTime);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Switch the child to rx-on and back to rx-off during a discovery scan");

    // The rx-on mode change stops the listener while paused, and the rx-off one starts it with a full check timeout,
    // so the end of the scan must neither fire a stale check nor leave supervision disabled.

    Mle::DeviceMode sedMode = sed.Get<Mle::Mle>().GetDeviceMode();

    sed.Get<SupervisionListener>().ResetCounter();

    StartDiscoverScan(sed, leader);
    SuccessOrQuit(
        sed.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(sedMode.Get() | Mle::DeviceMode::kModeRxOnWhenIdle)));
    nexus.AdvanceTime(Mle::DiscoverScanner::kDefaultScanDuration / 2);
    VerifyOrQuit(sed.Get<Mle::DiscoverScanner>().IsInProgress());
    SuccessOrQuit(sed.Get<Mle::Mle>().SetDeviceMode(sedMode));

    // Wait just under a full check timeout, past the fire time the check timeout had before the rx-on mode change.
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) - 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    // Supervision must still detect a lost parent after the sequence.
    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) + 10 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionWithBetterParentSearches(void)
{
    static constexpr uint16_t kCheckTimeout = 25;        // (seconds)
    static constexpr uint32_t kSearchPeriod = 10 * 1000; // (msec)
    static constexpr uint32_t kMaxWaitTime  = 40 * 1000; // (msec)

    Core     nexus;
    uint32_t elapsed = 0;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and attach SED");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));
    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Block traffic from child on parent while child runs periodic better parent searches");

    // Searches more frequent than the check timeout must not postpone the detection of the lost parent.

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);

    while (sed.Get<Mle::Mle>().IsChild() && (elapsed < kMaxWaitTime))
    {
        SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());

        nexus.AdvanceTime(kSearchPeriod);
        elapsed += kSearchPeriod;
    }

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionAfterBetterParentFound(void)
{
    static constexpr uint16_t kCheckTimeout        = 25; // (seconds)
    static constexpr uint16_t kSupervisionInterval = 10; // (seconds)

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network, attach SED to leader and router to leader");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // A parent sending supervision messages more often than the check timeout keeps the check from firing, so only
    // losing the parent fires it.
    sed.Get<SupervisionListener>().SetInterval(kSupervisionInterval);

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    router.Join(leader);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));
    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);
    sed.Get<SupervisionListener>().ResetCounter();

    // Advance partway through `kCheckTimeout` before searching, so the wait for the attach below passes the check
    // deadline set while attached to the previous parent.
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) - 5 * 1000);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Degrade the link to the leader and search for a better parent");

    // The search succeeds, so the child leaves the temporary rx-on period through the attach instead of the failure
    // path, and supervision must restart with the full timeout for the new parent.

    SuccessOrQuit(sed.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(),
                                                  GetTypicalRssForLinkQuality(-100, kLinkQuality1)));
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(sed.Get<Mac::Mac>().GetExtAddress(),
                                                     GetTypicalRssForLinkQuality(-100, kLinkQuality1)));

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(10 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().GetParent().GetExtAddress() == router.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Block traffic from child on the new parent");

    router.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) + 10 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionChangesDuringBetterParentSearch(void)
{
    static constexpr uint16_t kCheckTimeout        = 25; // (seconds)
    static constexpr uint16_t kSupervisionInterval = 10; // (seconds)

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and attach SED");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // A parent sending supervision messages more often than the check timeout keeps the check from firing, so only
    // losing the parent fires it.
    sed.Get<SupervisionListener>().SetInterval(kSupervisionInterval);

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));
    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);

    Mle::DeviceMode sedMode = sed.Get<Mle::Mle>().GetDeviceMode();

    Log("---------------------------------------------------------------------------------------");
    Log("Switch the child to rx-on and back to rx-off during a better parent search");

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(100);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsAttaching());

    SuccessOrQuit(
        sed.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(sedMode.Get() | Mle::DeviceMode::kModeRxOnWhenIdle)));
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());
    nexus.AdvanceTime(1000);
    SuccessOrQuit(sed.Get<Mle::Mle>().SetDeviceMode(sedMode));
    nexus.AdvanceTime(10 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!sed.Get<Mac::Mac>().GetRxOnWhenIdle());

    Log("---------------------------------------------------------------------------------------");
    Log("Change the child mode, staying sleepy, during a better parent search");

    // The mode change cancels the search without a re-attach, so sleepy operation and supervision must resume. The
    // rx-on mode change above restarts the listener, so this step runs last to keep a stuck listener detectable.

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(100);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsAttaching());

    SuccessOrQuit(
        sed.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(sedMode.Get() ^ Mle::DeviceMode::kModeFullNetworkData)));
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());
    nexus.AdvanceTime(10 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!sed.Get<Mac::Mac>().GetRxOnWhenIdle());

    Log("---------------------------------------------------------------------------------------");
    Log("Block traffic from child on parent");

    sed.Get<SupervisionListener>().ResetCounter();
    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) + 10 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionTimeoutChangesDuringBetterParentSearch(void)
{
    static constexpr uint16_t kCheckTimeout   = 25;        // (seconds)
    static constexpr uint32_t kSearchWaitTime = 10 * 1000; // (msec)

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and attach SED");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // With the default supervision interval, much longer than `kCheckTimeout`, the parent sends no frame to the child
    // during the steps below, so an armed check timeout fires.

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));

    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);
    sed.Get<SupervisionListener>().ResetCounter();

    Log("---------------------------------------------------------------------------------------");
    Log("Disable the supervision check during a better parent search");

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(100);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsAttaching());
    sed.Get<SupervisionListener>().SetTimeout(0);

    // Wait past the fire time the check timeout had before it was disabled.
    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout) + kSearchWaitTime);

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Run a better parent search with the supervision check disabled");

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(kSearchWaitTime);

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Enable the supervision check during a better parent search");

    // The check timeout starts in full when enabled, so the end of the search must not fire it from a stale fire time,
    // and the check must then fire once the full timeout passes.

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());
    nexus.AdvanceTime(100);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsAttaching());
    sed.Get<SupervisionListener>().SetTimeout(kCheckTimeout);
    nexus.AdvanceTime(kSearchWaitTime);

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    nexus.AdvanceTime(Time::SecToMsec(kCheckTimeout));

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

void TestChildSupervisionModeChangeDuringChildIdRequest(void)
{
    static constexpr uint16_t kSupervisionInterval = 10;  // (seconds)
    static constexpr uint32_t kPollPeriod          = 500; // (msec)
    static constexpr uint32_t kPollWindow          = 5000; // (msec)

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network, attach SED to leader and router to leader");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Get<SupervisionListener>().SetInterval(kSupervisionInterval);

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    router.Join(leader);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    Mle::DeviceMode sedMode = sed.Get<Mle::Mle>().GetDeviceMode();

    Log("---------------------------------------------------------------------------------------");
    Log("Change the child mode, staying sleepy, while sending a Child ID Request to a better parent");

    SuccessOrQuit(sed.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(),
                                                  GetTypicalRssForLinkQuality(-100, kLinkQuality1)));
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(sed.Get<Mac::Mac>().GetExtAddress(),
                                                     GetTypicalRssForLinkQuality(-100, kLinkQuality1)));

    SuccessOrQuit(sed.Get<Mle::Mle>().SearchForBetterParent());

    for (uint32_t elapsed = 0; !sed.Get<Mle::Mle>().GetParentCandidate().IsStateValid(); elapsed++)
    {
        VerifyOrQuit(elapsed < 10 * 1000);
        nexus.AdvanceTime(1);
    }

    // The Child ID Request may still be queued, so its tx done can arrive after the search is cancelled.

    SuccessOrQuit(
        sed.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(sedMode.Get() ^ Mle::DeviceMode::kModeFullNetworkData)));
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsAttaching());

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Check the child polls its parent at its own period");

    // Attach mode polls every `OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD`, which would send several times more
    // polls in the window than `kPollPeriod`.

    uint32_t polls       = sed.Get<Mac::Mac>().GetCounters().mTxDataPoll;
    uint32_t leaderPolls = leader.Get<Mac::Mac>().GetCounters().mRxDataPoll;
    uint32_t routerPolls = router.Get<Mac::Mac>().GetCounters().mRxDataPoll;

    nexus.AdvanceTime(kPollWindow);

    polls       = sed.Get<Mac::Mac>().GetCounters().mTxDataPoll - polls;
    leaderPolls = leader.Get<Mac::Mac>().GetCounters().mRxDataPoll - leaderPolls;
    routerPolls = router.Get<Mac::Mac>().GetCounters().mRxDataPoll - routerPolls;

    Log("Data polls in %lu msec: %lu, leader rx %lu, router rx %lu", ToUlong(kPollWindow), ToUlong(polls),
        ToUlong(leaderPolls), ToUlong(routerPolls));
    VerifyOrQuit(polls > 0);
    VerifyOrQuit(polls <= kPollWindow / kPollPeriod + 1);
    VerifyOrQuit(leaderPolls == polls);
    VerifyOrQuit(routerPolls == 0);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildSupervision();
    ot::Nexus::TestChildSupervisionWithDiscoveryScans();
    ot::Nexus::TestChildSupervisionChangesDuringDiscoveryScan();
    ot::Nexus::TestChildSupervisionWithBetterParentSearches();
    ot::Nexus::TestChildSupervisionAfterBetterParentFound();
    ot::Nexus::TestChildSupervisionChangesDuringBetterParentSearch();
    ot::Nexus::TestChildSupervisionTimeoutChangesDuringBetterParentSearch();
    ot::Nexus::TestChildSupervisionModeChangeDuringChildIdRequest();
    printf("All tests passed\n");
    return 0;
}
