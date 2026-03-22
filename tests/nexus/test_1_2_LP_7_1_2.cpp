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

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 20 * 1000;

/**
 * Time to advance for CSL synchronization to complete, in milliseconds.
 */
static constexpr uint32_t kCslSyncTime = 5 * 1000;

/**
 * Time to advance for Link Metrics report to be received, in milliseconds.
 */
static constexpr uint32_t kReportResponseTime = 2 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols (160 microseconds).
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

/**
 * RSSI value to simulate high transmission power.
 */
static constexpr int8_t kRssiHigh = -40;

/**
 * RSSI value to simulate low transmission power.
 */
static constexpr int8_t kRssiLow = -70;

/**
 * Query ID for Single Probe Query.
 */
static constexpr uint8_t kSingleProbeQueryId = 0;

static otLinkMetricsValues sMetricsValues;
static bool                sReportReceived;

static void HandleReport(const otIp6Address        *aAddress,
                         const otLinkMetricsValues *aMetricsValues,
                         otLinkMetricsStatus        aStatus,
                         void                      *aContext)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aStatus);
    OT_UNUSED_VARIABLE(aContext);

    if (aMetricsValues != nullptr)
    {
        sMetricsValues = *aMetricsValues;
    }

    sReportReceived = true;
}

void Test1_2_LP_7_1_2(void)
{
    /**
     * 7.1.2 Single Probe Link Metrics without Enhanced ACK
     *
     * 7.1.2.1 Topology
     * (Note: A specific topology diagram is not explicitly provided in the text, but the test involves the Leader
     *   (DUT), SED_1, and SSED_1.)
     *
     * 7.1.2.2 Purpose & Definition
     * The purpose of this test case is to ensure that the DUT can successfully respond to a Single Probe Link Metrics
     *   Request from SEDs without Enhanced ACKs.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Link Metrics     | 4.11.3
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed1   = nexus.CreateNode();
    Node &ssed1  = nexus.CreateNode();

    leader.SetName("LEADER");
    sed1.SetName("SED_1");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Topology formation: DUT, SED_1, SSED_1.");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, SED_1, SSED_1.
     * - Pass Criteria: N/A
     */

    leader.AllowList(sed1);
    leader.AllowList(ssed1);
    sed1.AllowList(leader);
    ssed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Each device automatically attaches to the DUT.");

    /**
     * Step 2: SED_1, SSED_1
     * - Description: Each device automatically attaches to the DUT.
     * - Pass Criteria: N/A
     */

    sed1.Join(leader, Node::kAsSed);
    ssed1.Join(leader, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(500));
    SuccessOrQuit(ssed1.Get<DataPollSender>().SetExternalPollPeriod(500));
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT.");

    /**
     * Step 3: SED_1, SSED_1
     * - Description: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the
     *   DUT.
     * - Pass Criteria:
     *   - 3.1 : The DUT MUST send an ICMPv6 Echo Reply to SED_1.
     *   - 3.2 : The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
     */

    nexus.SendAndVerifyEchoRequest(sed1, leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);
    nexus.SendAndVerifyEchoRequest(ssed1, leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: SED_1 configures TxPower = 'High' and sends Single Probe Link Metric for RSSI.");

    /**
     * Step 4: SED_1
     * - Description: Harness configures the device with TxPower = 'High'. Harness instructs the device to send a
     *   Single Probe Link Metric for RSSI using MLE Data Request with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 0x00 (Single Probe Query)
     *     - Link Metrics Query Options Sub-TLV
     *       - Concatenation of Link Metric Type ID Flags = 0x0b
     *         - E = 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A
     */

    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(sed1.Get<Mac::Mac>().GetExtAddress(), kRssiHigh));

    LinkMetrics::Metrics metrics;
    metrics.Clear();
    metrics.mRssi = true;

    sed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);
    sReportReceived = false;

    SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                           kSingleProbeQueryId, &metrics));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader (DUT) automatically responds to SED_1 with MLE Data Response.");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically responds to SED_1 with MLE Data Response.
     * - Pass Criteria: The DUT MUST reply to SED_1 with MLE Data Response with the following:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *         - Metric Enum = 0x03 (RSSI)
     *       - RSSI Value (1-byte)
     */

    nexus.AdvanceTime(kReportResponseTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sMetricsValues.mMetrics.mRssi);

    uint8_t rssiHighRaw = sMetricsValues.mRssiValue;

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: SED_1 configures TxPower = 'Low' and sends Single Probe Link Metric for RSSI.");

    /**
     * Step 6: SED_1
     * - Description: Harness configures the device with TxPower = 'Low'. Harness instructs the device to send a
     *   Single Probe Link Metric for RSSI using MLE Data Request with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 0x00 (Single Probe Query)
     *     - Link Metrics Query Options Sub-TLV
     *       - Concatenation of Link Metric Type ID Flags = 0x0b
     *         - E = 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A
     */

    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(sed1.Get<Mac::Mac>().GetExtAddress(), kRssiLow));

    sReportReceived = false;
    SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                           kSingleProbeQueryId, &metrics));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT) automatically responds to SED_1 with MLE Data Response.");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds to SED_1 with MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1 with a MLE Data Response including the
     *   following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *         - Metric Enum = 0x03 (RSSI)
     *       - RSSI value (1-byte)
     */

    nexus.AdvanceTime(kReportResponseTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sMetricsValues.mMetrics.mRssi);

    uint8_t rssiLowRaw = sMetricsValues.mRssiValue;

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: There should be a difference in the RSSI reported in steps 5 & 7.");

    /**
     * Step 8: Leader (DUT)
     * - Description: There should be a difference in the RSSI reported in steps 5 & 7.
     * - Pass Criteria: The DUT MUST report different RSSI values in the Link Metrics Report Sub-TLV from steps 5
     *   and 7.
     */

    VerifyOrQuit(rssiHighRaw != rssiLowRaw);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: SSED_1 sends Single Probe Link Metrics for Layer 2 LQI.");

    /**
     * Step 9: SSED_1
     * - Description: Harness instructs the device to send a Single Probe Link Metrics for Layer 2 LQI using MLE Data
     *   Request with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 0x00 (Single Probe Query)
     *     - Link Metrics Query Options Sub-TLV
     *       - Concatenation of Link Metric Type ID Flags = 0x09
     *         - E = 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 1 (Layer 2 LQI)
     * - Pass Criteria: N/A
     */

    metrics.Clear();
    metrics.mLqi = true;

    ssed1.Get<LinkMetrics::Initiator>().SetReportCallback(HandleReport, nullptr);
    sReportReceived = false;
    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                            kSingleProbeQueryId, &metrics));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1 including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *         - Metric Enum = 0x01 (Layer 2 LQI)
     *       - Layer 2 LQI value (1-byte)
     */

    nexus.AdvanceTime(kReportResponseTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sMetricsValues.mMetrics.mLqi);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: SSED_1 sends Single Probe Link Metrics for Link Margin.");

    /**
     * Step 11: SSED_1
     * - Description: Harness instructs the device to send a Single Probe Link Metrics for Link Margin using MLE
     *   Data Request with the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 0 (Single Probe Query)
     *     - Link Metrics Query Options Sub-TLV
     *     - Concatenation of Link Metric Type ID Flags = 0x0a
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     * - Pass Criteria: N/A
     */

    metrics.Clear();
    metrics.mLinkMargin = true;

    sReportReceived = false;
    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                            kSingleProbeQueryId, &metrics));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *         - Metric Enum = 0x02 (Link Margin)
     *       - Link Margin value (1-byte)
     */

    nexus.AdvanceTime(kReportResponseTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sMetricsValues.mMetrics.mLinkMargin);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: SSED_1 sends Single Probe Link Metrics for LQI, Link Margin, and RSSI.");

    /**
     * Step 13: SSED_1
     * - Description: Harness instructs the device to send a Single Probe Link Metrics using MLE Data Request with
     *   the following payload:
     *   - TLV Request TLV (Link Metrics Report TLV specified)
     *   - Link Metrics Query TLV
     *     - Link Metrics Query ID Sub-TLV
     *       - Query ID = 0 (Single Probe Query)
     *     - Link Metrics Query Options Sub-TLV
     *       - Concatenation of Link Metric Type ID Flags = 0x090a0b
     *         - E= 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 1 (Layer 2 LQI)
     *         - E= 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 2 (Link Margin)
     *         - E= 0x00; L = 0x00
     *         - Type / Average Enum = 1 (Exponential Moving Avg)
     *         - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A
     */

    metrics.Clear();
    metrics.mLqi        = true;
    metrics.mLinkMargin = true;
    metrics.mRssi       = true;

    sReportReceived = false;
    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().Query(leader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                            kSingleProbeQueryId, &metrics));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.");

    /**
     * Step 14: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
     *   - Link Metrics Report TLV
     *     - Link Metrics Report Sub-TLV
     *       - Metric Type ID Flags
     *         - E = 0x00
     *         - L = 0x00
     *         - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *         - Metric Enum = 0x01 (Layer 2 LQI)
     *       - Layer 2 LQI value (1-byte)
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *       - Metric Enum = 0x02 (Link Margin)
     *       - Link Margin value (1-byte)
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 0x01 (Exponential Moving Avg)
     *       - Metric Enum = 0x03 (RSSI)
     *       - RSSI value (1-byte)
     */

    nexus.AdvanceTime(kReportResponseTime);
    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(sMetricsValues.mMetrics.mLqi);
    VerifyOrQuit(sMetricsValues.mMetrics.mLinkMargin);
    VerifyOrQuit(sMetricsValues.mMetrics.mRssi);

    nexus.SaveTestInfo("test_1_2_LP_7_1_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_7_1_2();
    printf("All tests passed\n");
    return 0;
}
