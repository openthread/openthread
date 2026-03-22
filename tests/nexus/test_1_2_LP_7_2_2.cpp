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

#include "mac/data_poll_sender.hpp"
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
 * Time to advance for CSL synchronization to complete, in milliseconds.
 */
static constexpr uint32_t kCslSyncTime = 5 * 1000;

/**
 * Time to wait for metrics collection, in milliseconds.
 */
static constexpr uint32_t kWaitTime = 30 * 1000;

/**
 * Echo response timeout, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Poll rates in milliseconds.
 */
static constexpr uint32_t kPollRate1s = 1000;
static constexpr uint32_t kPollRate3s = 3000;
static constexpr uint32_t kPollRate5s = 5000;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols (160 microseconds).
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

/**
 * CSL Timeout in seconds.
 */
static constexpr uint32_t kCslTimeout = 30;

/**
 * Forward Series IDs.
 */
static constexpr uint8_t kSeriesId4  = 4;
static constexpr uint8_t kSeriesId6  = 6;
static constexpr uint8_t kSeriesId8  = 8;
static constexpr uint8_t kSeriesId10 = 10;

/**
 * Forward Series Flags.
 */
static constexpr uint8_t kSeriesFlagsMacDataRequest = 0x04;
static constexpr uint8_t kSeriesFlagsAllDataFrames  = 0x02;

struct CallbackContext
{
    otLinkMetricsStatus mStatus   = OT_LINK_METRICS_STATUS_OTHER_ERROR;
    bool                mReceived = false;

    void Reset(void) { mReceived = false; }
};

static void HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus, void *aContext)
{
    OT_UNUSED_VARIABLE(aAddress);
    auto *context      = static_cast<CallbackContext *>(aContext);
    context->mStatus   = aStatus;
    context->mReceived = true;
}

static void HandleReport(const otIp6Address        *aSource,
                         const otLinkMetricsValues *aMetricsValues,
                         otLinkMetricsStatus        aStatus,
                         void                      *aContext)
{
    OT_UNUSED_VARIABLE(aSource);
    OT_UNUSED_VARIABLE(aMetricsValues);
    auto *context      = static_cast<CallbackContext *>(aContext);
    context->mStatus   = aStatus;
    context->mReceived = true;
}

void Test1_2_LP_7_2_2(void)
{
    /**
     * 7.2.2 Forward Series Probing With Single Shot
     *
     * 7.2.2.1 Topology
     * - Leader (DUT)
     * - SED_1
     * - SED_2
     * - SED_3
     * - SSED_1
     * - SSED_2
     * - SSED_3
     *
     * 7.2.2.2 Purpose & Definition
     * The purpose of this test case is to ensure that the DUT can successfully support a minimum of 6 simultaneous
     *   children performing link metrics.
     *
     * Spec Reference                          | V1.2 Section
     * ----------------------------------------|-------------
     * Forward Series Probing With Single Shot | 4.11.3.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed1   = nexus.CreateNode();
    Node &sed2   = nexus.CreateNode();
    Node &sed3   = nexus.CreateNode();
    Node &ssed1  = nexus.CreateNode();
    Node &ssed2  = nexus.CreateNode();
    Node &ssed3  = nexus.CreateNode();

    leader.SetName("LEADER");
    sed1.SetName("SED_1");
    sed2.SetName("SED_2");
    sed3.SetName("SED_3");
    ssed1.SetName("SSED_1");
    ssed2.SetName("SSED_2");
    ssed3.SetName("SSED_3");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    CallbackContext context;

    /**
     * Step 0: SSED_1,2,3 / SED_1,2,3
     * - Description: Preconditions: SSED_1,2,3 - set CSLTimeout = 30s; SED_1 - set poll rate = 1s; SED_2 = set poll
     * rate = 3s; SED_3 = set poll rate = 5s.
     * - Pass Criteria: N/A.
     */
    Log("Step 0: Preconditions: Set poll rates and CSL timeout.");
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kPollRate1s));
    SuccessOrQuit(sed2.Get<DataPollSender>().SetExternalPollPeriod(kPollRate3s));
    SuccessOrQuit(sed3.Get<DataPollSender>().SetExternalPollPeriod(kPollRate5s));
    SuccessOrQuit(ssed1.Get<DataPollSender>().SetExternalPollPeriod(kPollRate5s));
    SuccessOrQuit(ssed2.Get<DataPollSender>().SetExternalPollPeriod(kPollRate5s));
    SuccessOrQuit(ssed3.Get<DataPollSender>().SetExternalPollPeriod(kPollRate5s));
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);
    ssed2.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);
    ssed3.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, SED_1, SED_2, SED_3, SSED_1, SSED_2, SSED_3. SSED_1,2,3 Preconditions:
     * Set polling rate = 5s,.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: Topology formation: DUT, SED_1, SED_2, SED_3, SSED_1, SSED_2, SSED_3.");
    leader.AllowList(sed1);
    leader.AllowList(sed2);
    leader.AllowList(sed3);
    leader.AllowList(ssed1);
    leader.AllowList(ssed2);
    leader.AllowList(ssed3);
    sed1.AllowList(leader);
    sed2.AllowList(leader);
    sed3.AllowList(leader);
    ssed1.AllowList(leader);
    ssed2.AllowList(leader);
    ssed3.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    /**
     * Step 2: SED_1,2,3 / SSED_1,2,3
     * - Description: Each device automatically attaches to the Leader (DUT).
     * - Pass Criteria: N/A.
     */
    Log("Step 2: Each device automatically attaches to the Leader (DUT).");
    sed1.Join(leader, Node::kAsSed);
    sed2.Join(leader, Node::kAsSed);
    sed3.Join(leader, Node::kAsSed);
    ssed1.Join(leader, Node::kAsSed);
    ssed2.Join(leader, Node::kAsSed);
    ssed3.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachTime);

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    ssed2.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    ssed3.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3: SED_1,2,3 / SSED_1,2,3
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request from each device to the Leader
     *   (DUT).
     * - Pass Criteria:
     *   - 3.1: The DUT MUST send an ICMPv6 Echo Reply to SED_1.
     *   - 3.2: The DUT MUST send an ICMPv6 Echo Reply to SED_2.
     *   - 3.3: The DUT MUST send an ICMPv6 Echo Reply to SED_3.
     *   - 3.4: The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
     *   - 3.5: The DUT MUST send an ICMPv6 Echo Reply to SSED_2.
     *   - 3.6: The DUT MUST send an ICMPv6 Echo Reply to SSED_3.
     */
    Log("Step 3: Harness verifies connectivity.");
    Node *nodes[] = {&sed1, &sed2, &sed3, &ssed1, &ssed2, &ssed3};
    for (Node *node : nodes)
    {
        nexus.SendAndVerifyEchoRequest(*node, leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0, Ip6::kDefaultHopLimit,
                                       kEchoTimeout);
    }

    /**
     * Step 4: SED_1
     * - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 4
     *     - Forward Series Flags = 0x04
     *       - Bits 0,1 = 0
     *       - Bit 2 = 1 (Track MAC Data Request command frames)
     *       - Bits 3-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x09
     *       - E = 0x00; L = 0x00
     *       - Type/Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 1 (Layer 2 LQI)
     * - Pass Criteria: N/A.
     */
    Log("Step 4: SED_1 requests a Forward Series Link Metric ID 4.");
    {
        LinkMetrics::SeriesFlags flags;
        flags.SetFrom(kSeriesFlagsMacDataRequest);
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mLqi = true;

        sed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId4, flags, &metrics));
    }

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SED_1 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 5: Leader responds to SED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate1s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 6: SED_2
     * - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 6
     *     - Forward Series Flags = 0x02
     *       - Bit 0 = 0
     *       - Bit 1 = 1 (Track Link Layer data frames)
     *       - Bits 2-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x40
     *       - E = 0x00; L = 0x01
     *       - Type/Average Enum = 0 (count)
     *       - Metric Enum = 0 (# of PDUs received)
     * - Pass Criteria: N/A.
     */
    Log("Step 6: SED_2 requests a Forward Series Link Metric ID 6.");
    {
        LinkMetrics::SeriesFlags flags;
        flags.SetFrom(kSeriesFlagsAllDataFrames);
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mPduCount = true;

        sed2.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(sed2.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId6, flags, &metrics));
    }

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SED_2 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 7: Leader responds to SED_2 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate3s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 8: SSED_1
     * - Description: Harness instructs device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 8
     *     - Forward Series Flags = 0x02
     *       - Bit 0 = 0
     *       - Bit 1 = 1 (Track Link Layer data frames)
     *       - Bits 2-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x0a
     *       - E = 0x00; L = 0x00
     *       - Type/Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     * - Pass Criteria: N/A.
     */
    Log("Step 8: SSED_1 requests a Forward Series Link Metric ID 8.");
    {
        LinkMetrics::SeriesFlags flags;
        flags.SetFrom(kSeriesFlagsAllDataFrames);
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mLinkMargin = true;

        ssed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId8, flags, &metrics));
    }

    /**
     * Step 9: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SSED_1 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 9: Leader responds to SSED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 10: SSED_2
     * - Description: Harness instructs device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 10
     *     - Forward Series Flags = 0x04
     *       - Bits 0,1 = 0
     *       - Bit 2 = 1 (MAC Data Request command frames)
     *       - Bits 3-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x0b
     *       - E = 0x00; L = 0x00
     *       - Type/Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A.
     */
    Log("Step 10: SSED_2 requests a Forward Series Link Metric ID 10.");
    {
        LinkMetrics::SeriesFlags flags;
        flags.SetFrom(kSeriesFlagsMacDataRequest);
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mRssi = true;

        ssed2.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(ssed2.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId10, flags, &metrics));
    }

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SSED_2 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 11: Leader responds to SSED_2 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 12: SED_3
     * - Description: Harness instructs device to enable IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link
     *   Metrics Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Type = 7
     *     - Enh-ACK Flags = 1
     *     - Concatenation of Link Metrics Type ID Flags=0x0a
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     * - Pass Criteria: N/A.
     */
    Log("Step 12: SED_3 enables Enhanced ACK based Probing.");
    {
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mLinkMargin = true;

        sed3.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(sed3.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metrics));
    }

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SED_3 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 13: Leader responds to SED_3 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 14: SSED_3
     * - Description: Harness instructs device to enable IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link
     *   Metrics Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Type = 7
     *     - Enh-ACK Flags = 1
     *     - Concatenation of Link Metrics Type ID Flags=0x09, 0x0b
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 1 (Layer 2 LQI)
     *       - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A.
     */
    Log("Step 14: SSED_3 enables Enhanced ACK based Probing.");
    {
        LinkMetrics::Metrics metrics;
        metrics.Clear();
        metrics.mLqi  = true;
        metrics.mRssi = true;

        ssed3.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, &context);
        context.Reset();

        SuccessOrQuit(ssed3.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metrics));
    }

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST respond to SSED_3 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success).
     */
    Log("Step 15: Leader responds to SSED_3 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 16: SED_1, SSED_2
     * - Description: Harness configures devices to send MAC Data Requests periodically.
     * - Pass Criteria: N/A.
     */
    Log("Step 16: Configuration already done in Step 0.");

    /**
     * Step 17: SED_2, SSED_1
     * - Description: Harness instructs devices to send MAC Data packets every 1 second for 30 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 17: Devices send data packets.");
    for (uint32_t i = 0; i < kWaitTime / kPollRate1s; i++)
    {
        sed2.SendEchoRequest(leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0);
        ssed1.SendEchoRequest(leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0);
        nexus.AdvanceTime(kPollRate1s);
    }

    /**
     * Step 18: SED_1
     * - Description: Harness instructs device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the Leader (DUT) with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 4 [series configured in step 4]
     * - Pass Criteria: N/A.
     */
    Log("Step 18: SED_1 retrieves aggregated Forward Series ID 4 results.");
    {
        sed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, &context);
        context.Reset();

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId4,
                                                               nullptr));
    }

    /**
     * Step 19: Leader (DUT)
     * - Description: Automatically responds with a MLE Data Response.
     * - Pass Criteria: The DUT MUST reply to SED_1 with a MLE Data Response with the following:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 1
     *         - Metric Enum = 1
     *       - Value (1 bytes)
     */
    Log("Step 19: Leader responds to SED_1 with a MLE Data Response.");
    nexus.AdvanceTime(kPollRate1s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 20: SED_2
     * - Description: Harness instructs device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the Leader (DUT) with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 6 [series configured in step 6]
     * - Pass Criteria: N/A.
     */
    Log("Step 20: SED_2 retrieves aggregated Forward Series ID 6 results.");
    {
        sed2.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, &context);
        context.Reset();

        SuccessOrQuit(sed2.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId6,
                                                               nullptr));
    }

    /**
     * Step 21: Leader (DUT)
     * - Description: Automatically responds with a MLE Data Response.
     * - Pass Criteria: The DUT MUST reply to SED_2 with a MLE Data Response with the following:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x01
     *         - Type / Average Enum = 0 (Count)
     *         - Metric Enum = 0 (Number of PDUs received)
     *       - Count Value (4 bytes)
     */
    Log("Step 21: Leader responds to SED_2 with a MLE Data Response.");
    nexus.AdvanceTime(kPollRate3s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 22: SED_3
     * - Description: Harness makes a Single Probe Link Metric query for Link Margin by instructing the device to send a
     *   MAC Data message with the Frame Version subfield within the MAC header of the message to set 0b10.
     * - Pass Criteria: N/A.
     */
    Log("Step 22: SED_3 makes a Single Probe Link Metric query.");
    sed3.SendEchoRequest(leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0);

    /**
     * Step 23: Leader (DUT)
     * - Description: Automatically responds with an Enhanced Acknowledgement containing link metrics.
     * - Pass Criteria: The DUT MUST reply to SED_3 with Enhanced ACK containing the following:
     *   - Header IE
     *     - Element ID = 0x00
     *     - Vendor CID = 0xeab89b (Thread Group)
     *     - Vendor Specific Information
     *       - 1st byte = 0 (Enhanced ACK Link Metrics)
     *       - 2nd byte … Link Margin data
     */
    Log("Step 23: Leader responds with an Enhanced Acknowledgement.");
    nexus.AdvanceTime(kPollRate5s);

    /**
     * Step 24: SSED_1
     * - Description: Harness instructs device to retrieve aggregated Forward Series Results by sending a MLE Data
     *   Request message to the Leader (DUT) with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 8 [series configured in step 8]
     *   - The Link Metrics Query Options Sub-TLV is NOT sent
     * - Pass Criteria: N/A.
     */
    Log("Step 24: SSED_1 retrieves aggregated Forward Series ID 8 results.");
    {
        ssed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, &context);
        context.Reset();

        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                                kSeriesId8, nullptr));
    }

    /**
     * Step 25: Leader (DUT)
     * - Description: Automatically responds with MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 1
     *         - Metric Enum = 2
     *       - Value (1 byte)
     */
    Log("Step 25: Leader responds to SSED_1 with a MLE Data Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 26: SSED_2
     * - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the Leader (DUT) with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 10 [series configured in step 10]
     * - Pass Criteria: N/A.
     */
    Log("Step 26: SSED_2 retrieves aggregated Forward Series ID 10 results.");
    {
        ssed2.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, &context);
        context.Reset();

        SuccessOrQuit(ssed2.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                                kSeriesId10, nullptr));
    }

    /**
     * Step 27: Leader (DUT)
     * - Description: Automatically responds with a MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_2, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 1
     *         - Metric Enum = 3
     *       - Value (1 byte)
     */
    Log("Step 27: Leader responds to SSED_2 with a MLE Data Response.");
    nexus.AdvanceTime(kPollRate5s);
    VerifyOrQuit(context.mReceived);
    VerifyOrQuit(context.mStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 28: SSED_3
     * - Description: Harness makes a Single Probe Link Metric query for Link Margin by instructing the device to send a
     *   MAC Data message with the Frame Version subfield within the MAC header of the message to 0b10.
     * - Pass Criteria: N/A.
     */
    Log("Step 28: SSED_3 makes a Single Probe Link Metric query.");
    ssed3.SendEchoRequest(leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0);

    /**
     * Step 29: Leader (DUT)
     * - Description: Automatically responds with an Enhanced Acknowledgement containing link metrics.
     * - Pass Criteria: The DUT MUST reply to SSED_3 with Enhanced ACK containing:
     *   - Header IE
     *     - Element ID = 0x00
     *     - Vendor CID = 0xeab89b (Thread Group)
     *     - Vendor Specific Information
     *       - 1st byte = 0 (Enhanced ACK Link Metrics)
     *       - 2nd byte …LQI
     *       - 3rd byte … RSSI
     */
    Log("Step 29: Leader responds with an Enhanced Acknowledgement.");
    nexus.AdvanceTime(kPollRate5s);

    nexus.SaveTestInfo("test_1_2_LP_7_2_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_7_2_2();
    printf("All tests passed\n");
    return 0;
}
