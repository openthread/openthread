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
#include "thread/link_metrics.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 5 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Forward Series IDs.
 */
static constexpr uint8_t kSeriesId1 = 1;
static constexpr uint8_t kSeriesId2 = 2;
static constexpr uint8_t kSeriesId3 = 3;

/**
 * Forward Series Flags.
 */
static constexpr uint8_t kSeriesFlagsAllDataFrames = 0x02;
static constexpr uint8_t kSeriesFlagsClear         = 0x00;

static otLinkMetricsStatus sMgmtStatus;
static bool                sMgmtResponseReceived;
static otLinkMetricsStatus sReportStatus;
static bool                sReportReceived;

static void HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus, void *aContext)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aContext);

    sMgmtStatus           = aStatus;
    sMgmtResponseReceived = true;
}

static void HandleReport(const otIp6Address        *aSource,
                         const otLinkMetricsValues *aMetricsValues,
                         otLinkMetricsStatus        aStatus,
                         void                      *aContext)
{
    OT_UNUSED_VARIABLE(aSource);
    OT_UNUSED_VARIABLE(aMetricsValues);
    OT_UNUSED_VARIABLE(aContext);

    sReportStatus   = aStatus;
    sReportReceived = true;
}

static void SendForwardTrackingSeries(Node &aInitiator, Node &aSubject, uint8_t aSeriesId, uint8_t aFlags)
{
    LinkMetrics::SeriesFlags flags;
    LinkMetrics::Metrics     metrics;

    flags.SetFrom(aFlags);
    metrics.Clear();
    metrics.mPduCount = true;

    sMgmtResponseReceived = false;

    SuccessOrQuit(aInitiator.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
        aSubject.Get<Mle::Mle>().GetLinkLocalAddress(), aSeriesId, flags,
        (aFlags == kSeriesFlagsClear) ? nullptr : &metrics));

    Core::Get().AdvanceTime(kStabilizationTime);

    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);
}

static void QueryForwardTrackingSeries(Node &aInitiator, Node &aSubject, uint8_t aSeriesId, otLinkMetricsStatus aStatus)
{
    sReportReceived = false;

    SuccessOrQuit(aInitiator.Get<LinkMetrics::Initiator>().Query(aSubject.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                                 aSeriesId, nullptr));

    Core::Get().AdvanceTime(kStabilizationTime);

    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sReportStatus == aStatus);
}

void TestLinkMetricsSeriesRouterUpgrade(void)
{
    // Checks that the Forward Tracking Series tracked for a neighbor
    // stay with that neighbor when a child becomes a router: on the
    // parent (child entry copied to a router entry) and on the child
    // itself (parent entry copied to a router entry).

    Core                     nexus;
    Node                    &leader = nexus.CreateNode();
    Node                    &router = nexus.CreateNode();
    Child                   *childEntry;
    Router                  *routerEntry;
    LinkMetrics::SeriesInfo *seriesInfo;
    LinkMetrics::SeriesInfo *parentSeriesInfo;

    nexus.AdvanceTime(0);

    Log("Form network");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Join as router-ineligible child");
    router.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsChild());

    router.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
    router.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);
    leader.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
    leader.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);

    Log("Register Forward Tracking Series as child");
    SendForwardTrackingSeries(router, leader, kSeriesId1, kSeriesFlagsAllDataFrames);

    childEntry = leader.Get<ChildTable>().FindChild(router.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(childEntry != nullptr);
    seriesInfo = childEntry->GetForwardTrackingSeriesInfo(kSeriesId1);
    VerifyOrQuit(seriesInfo != nullptr);

    Log("Register Forward Tracking Series on child");
    SendForwardTrackingSeries(leader, router, kSeriesId3, kSeriesFlagsAllDataFrames);
    parentSeriesInfo = router.Get<Mle::Mle>().GetParent().GetForwardTrackingSeriesInfo(kSeriesId3);
    VerifyOrQuit(parentSeriesInfo != nullptr);

    Log("Become router");
    SuccessOrQuit(router.Get<Mle::Mle>().SetRouterEligible(true));
    SuccessOrQuit(router.Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    VerifyOrQuit(leader.Get<ChildTable>().FindChild(router.Get<Mac::Mac>().GetExtAddress(),
                                                    Child::kInStateAnyExceptInvalid) == nullptr);

    routerEntry = leader.Get<RouterTable>().FindRouter(router.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(routerEntry != nullptr);
    VerifyOrQuit(routerEntry->IsStateValid());
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId1) == seriesInfo);

    routerEntry = router.Get<RouterTable>().FindRouter(leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(routerEntry != nullptr);
    VerifyOrQuit(routerEntry->IsStateValid());
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId3) == parentSeriesInfo);
    VerifyOrQuit(router.Get<Mle::Mle>().GetParent().GetForwardTrackingSeriesInfo(kSeriesId3) == nullptr);

    QueryForwardTrackingSeries(leader, router, kSeriesId3, OT_LINK_METRICS_STATUS_SUCCESS);

    Log("Register a second Forward Tracking Series as router");
    SendForwardTrackingSeries(router, leader, kSeriesId2, kSeriesFlagsAllDataFrames);

    routerEntry = leader.Get<RouterTable>().FindRouter(router.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(routerEntry != nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId1) != nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId2) != nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId1) !=
                 routerEntry->GetForwardTrackingSeriesInfo(kSeriesId2));

    Log("Query both series");
    QueryForwardTrackingSeries(router, leader, kSeriesId1, OT_LINK_METRICS_STATUS_SUCCESS);
    QueryForwardTrackingSeries(router, leader, kSeriesId2, OT_LINK_METRICS_STATUS_SUCCESS);

    Log("Clear all series");
    SendForwardTrackingSeries(router, leader, kSeriesId1, kSeriesFlagsClear);
    SendForwardTrackingSeries(router, leader, kSeriesId2, kSeriesFlagsClear);
    SendForwardTrackingSeries(leader, router, kSeriesId3, kSeriesFlagsClear);

    routerEntry = leader.Get<RouterTable>().FindRouter(router.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(routerEntry != nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId1) == nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId2) == nullptr);

    routerEntry = router.Get<RouterTable>().FindRouter(leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(routerEntry != nullptr);
    VerifyOrQuit(routerEntry->GetForwardTrackingSeriesInfo(kSeriesId3) == nullptr);

    QueryForwardTrackingSeries(router, leader, kSeriesId1, OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED);
    QueryForwardTrackingSeries(router, leader, kSeriesId2, OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED);
    QueryForwardTrackingSeries(leader, router, kSeriesId3, OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestLinkMetricsSeriesRouterUpgrade();
    printf("All tests passed\n");
    return 0;
}
