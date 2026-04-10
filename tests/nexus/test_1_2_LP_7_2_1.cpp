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
 * SED poll period, in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 2000;

/**
 * SSED data packet interval, in milliseconds.
 */
static constexpr uint32_t kSsedDataInterval = 1000;

/**
 * Echo response timeout, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols (160 microseconds).
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

/**
 * Forward Series IDs.
 */
static constexpr uint8_t kSeriesId4  = 4;
static constexpr uint8_t kSeriesId11 = 11;
static constexpr uint8_t kSeriesId16 = 16;

/**
 * Forward Series Flags.
 */
static constexpr uint8_t kSeriesFlagsMacDataRequest = 0x04;
static constexpr uint8_t kSeriesFlagsAllDataFrames  = 0x02;
static constexpr uint8_t kSeriesFlagsClear          = 0x00;

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

void Test1_2_LP_7_2_1(void)
{
    /**
     * 7.2.1 Forward Tracking Series
     *
     * 7.2.1.1 Topology
     * - Leader (DUT)
     * - SED_1
     * - SSED_1
     *
     * 7.2.1.2 Purpose & Definition
     * The purpose of this test case is to ensure that the DUT can successfully respond to a Forward Series Link Metrics
     *   Request from both a SED & SSED.
     *
     * Spec Reference           | V1.2 Section
     * -------------------------|-------------
     * Forward Tracking Series  | 4.11.3.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed1   = nexus.CreateNode();
    Node &ssed1  = nexus.CreateNode();

    leader.SetName("LEADER");
    sed1.SetName("SED_1");
    ssed1.SetName("SSED_1");

    {
        Mac::ExtAddress      addr;
        static const uint8_t kLeaderAddr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
        static const uint8_t kSedAddr[]    = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x89};
        static const uint8_t kSsedAddr[]   = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x8a};

        addr.Set(kLeaderAddr);
        leader.Get<Mac::Mac>().SetExtAddress(addr);
        addr.Set(kSedAddr);
        sed1.Get<Mac::Mac>().SetExtAddress(addr);
        addr.Set(kSsedAddr);
        ssed1.Get<Mac::Mac>().SetExtAddress(addr);
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0: SED_1
     * - Description: Preconditions: SED_1 data poll rate set to 2 seconds.
     * - Pass Criteria: N/A
     */
    Log("Step 0: SED_1 data poll rate set to 2 seconds.");
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
    SuccessOrQuit(ssed1.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, SED_1, SSED_1.
     * - Pass Criteria: N/A
     */
    Log("Step 1: Topology formation: DUT, SED_1, SSED_1.");
    leader.AllowList(sed1);
    leader.AllowList(ssed1);
    sed1.AllowList(leader);
    ssed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    /**
     * Step 2: SED_1, SSED_1
     * - Description: Each device automatically attaches to the DUT.
     * - Pass Criteria: N/A
     */
    Log("Step 2: Each device automatically attaches to the DUT.");
    sed1.Join(leader, Node::kAsSed);
    ssed1.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3: SED_1, SSED_1
     * - Description: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the
     *   DUT.
     * - Pass Criteria:
     *   - 3.1: The DUT MUST send an ICMPv6 Echo Reply to SED_1.
     *   - 3.2: The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
     */
    Log("Step 3: Harness verifies connectivity.");
    nexus.SendAndVerifyEchoRequest(sed1, leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(ssed1, leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4: SED_1
     * - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request to the DUT with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 4
     *     - Forward Series Flags = 0x04
     *       - Bits 0-1 = 0
     *       - Bit 2 = 1 (MAC Data Request command frames)
     *       - Bits 3-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x40
     *       - E = 0x00; L = 0x01
     *       - Type/Average Enum = 0x00 (count)
     *       - Metric Enum = 0x00 (count)
     * - Pass Criteria: N/A
     */
    Log("Step 4: SED_1 requests Forward Series Link Metric ID 4.");
    {
        LinkMetrics::SeriesFlags flags;
        LinkMetrics::Metrics     metrics;

        flags.SetFrom(kSeriesFlagsMacDataRequest);
        metrics.Clear();
        metrics.mPduCount = true;

        sed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
        sMgmtResponseReceived = false;

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId4, flags, &metrics));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */
    Log("Step 5: Leader responds to SED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 6: SED_1
     * - Description: Harness waits for 30 seconds (MAC Data Requests sent every 2 seconds).
     * - Pass Criteria: N/A
     */
    Log("Step 6: Wait for 30 seconds.");
    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 7: SED_1
     * - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the DUT with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 4 [series configured in step 4]
     * - Pass Criteria: N/A
     */
    Log("Step 7: SED_1 retrieves aggregated Forward Series ID 4 results.");
    {
        sed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);
        sReportReceived = false;

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId4,
                                                               nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x01
     *         - Type / Average Enum = 0x00 (count/sum)
     *         - Metric Enum = 0x00 (number of PDUs received)
     *       - Count Value (4 bytes)
     */
    Log("Step 8: Leader responds to SED_1 with a MLE Data Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sReportStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 9: SED_1
     * - Description: Harness instructs the device to clear the Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - Link Metrics Management Request:
     *   - Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID = 4 [series configured in step 4]
     *     - Forward Series Flags = 0x00
     *       - Bits 0 -7 = 0
     * - Pass Criteria: N/A
     */
    Log("Step 9: SED_1 clears Forward Series Link Metric ID 4.");
    {
        LinkMetrics::SeriesFlags flags;
        flags.SetFrom(kSeriesFlagsClear);

        sMgmtResponseReceived = false;

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId4, flags, nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */
    Log("Step 10: Leader responds to SED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 11: SED_1
     * - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID: 11
     *     - Forward Series Flags = 0x04
     *       - Bits 0,1 = 0
     *       - Bit 2 = 1 (MAC Data Request command frames)
     *       - Bits 3-7 = 0
     *     - Concatenation of Link Metric Type ID Flags=0x40
     *       - E= 0x00; L = 0x01
     *       - Metric Enum = 0x00 (count)
     *       - Type/Average Enum = 0x00 (count)
     * - Pass Criteria: N/A
     */
    Log("Step 11: SED_1 requests Forward Series Link Metric ID 11.");
    {
        LinkMetrics::SeriesFlags flags;
        LinkMetrics::Metrics     metrics;

        flags.SetFrom(kSeriesFlagsMacDataRequest);
        metrics.Clear();
        metrics.mPduCount = true;

        sMgmtResponseReceived = false;

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId11, flags, &metrics));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */
    Log("Step 12: Leader responds to SED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 13: SED_1
     * - Description: Harness waits for 30 seconds (MAC Data Requests sent every two seconds).
     * - Pass Criteria: N/A
     */
    Log("Step 13: Wait for 30 seconds.");
    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 14: SED_1
     * - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the DUT with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 11 [series configured in step 11]
     * - Pass Criteria: N/A
     */
    Log("Step 14: SED_1 retrieves aggregated Forward Series ID 11 results.");
    {
        sReportReceived = false;

        SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                               kSeriesId11, nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x01
     *         - Type / Average Enum = 0x00 (count / sum)
     *         - Metric Enum = 0x00 (number of PDUs received)
     *       - Count Value (4 bytes)
     */
    Log("Step 15: Leader responds to SED_1 with a MLE Data Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sReportStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 16: SSED_1
     * - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
     *   Management Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID: 16
     *     - Forward Series Flags = 0x02
     *       - Bit 0 =0
     *       - Bit 1 = 1 (all Link Layer Data Frames)
     *       - Bits 2-7 = 0
     *     - Concatenation of Link Metric Type ID Flags = 0x0a
     *       - E = 0x00; L = 0x00
     *       - Type/Average Enum = 0x01 - Exponential Moving Avg
     *       - Metric Enum = 0x02 (Link Margin)
     * - Pass Criteria: N/A
     */
    Log("Step 16: SSED_1 requests Forward Series Link Metric ID 16.");
    {
        LinkMetrics::SeriesFlags flags;
        LinkMetrics::Metrics     metrics;

        flags.SetFrom(kSeriesFlagsAllDataFrames);
        metrics.Clear();
        metrics.mLinkMargin = true;

        ssed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
        sMgmtResponseReceived = false;

        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId16, flags, &metrics));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 17: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SSED_1, including the following TLVs:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */
    Log("Step 17: Leader responds to SSED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 18: SSED_1
     * - Description: Harness instructs the device to send MAC Data packets every 1 second for 30 seconds.
     * - Pass Criteria: N/A
     */
    Log("Step 18: SSED_1 sends MAC Data packets every 1 second for 30 seconds.");
    for (uint32_t i = 0; i < kWaitTime / kSsedDataInterval; i++)
    {
        ssed1.SendEchoRequest(leader.Get<Mle::Mle>().GetLinkLocalAddress(), 0);
        nexus.AdvanceTime(kSsedDataInterval);
    }

    /**
     * Step 19: SSED_1
     * - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the DUT with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 16 [same value as set in step 16]
     * - Pass Criteria: N/A
     */
    Log("Step 19: SSED_1 retrieves aggregated Forward Series ID 16 results.");
    {
        ssed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);
        sReportReceived = false;

        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                                kSeriesId16, nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 20: Leader (DUT)
     * - Description: Automatically responds with a MLE Data Response.
     * - Pass Criteria: The DUT MUST reply to SSED_1 with a MLE Data Response with the following:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *     - Metric Type ID Flags
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *       - Metric Enum = 0x02 (Link Margin)
     *     - Value (1 byte)
     */
    Log("Step 20: Leader responds to SSED_1 with a MLE Data Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sReportStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 21: SSED_1
     * - Description: Harness instructs device to clear an ongoing Forward Series by sending a Link Metrics Management
     *   Request with the following payload:
     *   - MLE Link Metrics Management TLV
     *   - Forward Probing Registration Sub-TLV
     *     - Type = 3
     *     - Forward Series ID: = 16 [same value as set in step 16]
     *     - Forward Series Flags = 0x00
     * - Pass Criteria: N/A
     */
    Log("Step 21: SSED_1 clears Forward Series ID 16.");
    {
        LinkMetrics::SeriesFlags flags;

        sMgmtResponseReceived = false;

        flags.SetFrom(kSeriesFlagsClear);
        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestForwardTrackingSeries(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), kSeriesId16, flags, nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 22: Leader (DUT)
     * - Description: Automatically responds with a Link Metrics Management Response.
     * - Pass Criteria: The DUT MUST reply to SSED_1 with a Link Metrics Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */
    Log("Step 22: Leader responds to SSED_1 with a Link Metrics Management Response.");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    /**
     * Step 23: SSED_1
     * - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
     *   Request message to the DUT with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 16 [same value as set in step 16]
     *   - This is a negative step
     * - Pass Criteria: N/A
     */
    Log("Step 23: SSED_1 retrieves aggregated Forward Series ID 16 results (negative step).");
    {
        sReportReceived = false;

        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                                kSeriesId16, nullptr));
    }
    nexus.AdvanceTime(kSedPollPeriod);

    /**
     * Step 24: Leader (DUT)
     * - Description: Automatically responds with a MLE Data Response.
     * - Pass Criteria: The DUT MUST reply to SSED_1 with a MLE Data Response with the following:
     *   - Link Metrics Report TLV
     *     - Link Metrics Status Sub-TLV
     *       - Status = 3 (Failure - Series ID not recognized)
     */
    Log("Step 24: Leader responds to SSED_1 with a MLE Data Response (Failure).");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sReportStatus == OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED);

    nexus.SaveTestInfo("test_1_2_LP_7_2_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_7_2_1();
    printf("All tests passed\n");
    return 0;
}
