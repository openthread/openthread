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
 * Time to advance for Link Metrics management response to be received, in milliseconds.
 */
static constexpr uint32_t kMgmtResponseTime = 2 * 1000;

/**
 * Time to advance for Enhanced ACK to be received after soliciting it, in milliseconds.
 */
static constexpr uint32_t kEnhAckTime = 1 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Identifier for ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols (160 microseconds).
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

static otLinkMetricsStatus sMgmtStatus;
static bool                sMgmtResponseReceived;
static bool                sEnhAckReportReceived;

static void HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus, void *aContext)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aContext);

    sMgmtStatus           = aStatus;
    sMgmtResponseReceived = true;
}

static void HandleEnhAckProbingReport(otShortAddress             aShortAddress,
                                      const otExtAddress        *aExtAddress,
                                      const otLinkMetricsValues *aMetricsValues,
                                      void                      *aContext)
{
    OT_UNUSED_VARIABLE(aShortAddress);
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aMetricsValues);
    OT_UNUSED_VARIABLE(aContext);

    sEnhAckReportReceived = true;
}

void Test1_2_LP_7_1_1(void)
{
    /**
     * 7.1.1 Single Probe Link Metrics with Enhanced ACKs
     *
     * 7.1.1.1 Topology
     * (No specific topology text description is provided other than an implicit
     *   reference to a diagram with DUT as Leader, SED_1, and SSED_1).
     *
     * 7.1.1.2 Purpose & Definition
     * The purpose of this test case is to ensure that the DUT can successfully
     *   respond to a Single Probe Link Metrics Request from a SED & a SSED
     *   with Enhanced ACKs.
     * - 4 - SED, Enhanced ACK, type/avg enum=1, metric enum=2
     * - 6 - SSED, Enhanced ACK, type/avg enum=1, metric enum =2 and 3
     * - 8 - SSED requests link metrics report via Enhanced ACK
     * - 10 - SED requests link metrics report via Enhanced ACK
     * - 12 - Clear SSED Enhanced ACK config
     * - 14 - SSED requests link metrics report via Enhanced ACK (negative test)
     * - 16 - SSED, Enhanced ACK, type/avg enum=1, metric enum= 1, 2, and 3
     *   (negative test)
     * - 18 - SSED, Enhanced ACK, type/avg enum=2, metric enum=2 (negative test)
     *
     * Specification Reference: 4.11.3
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

    Log("Step 1: Topology formation: DUT, SED_1, SSED_1.");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, SED_1, SSED_1.
     * - Pass Criteria: N/A.
     */

    leader.AllowList(sed1);
    leader.AllowList(ssed1);
    sed1.AllowList(leader);
    ssed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Step 2: Each device automatically attaches to the Leader (DUT).");

    /**
     * Step 2: SED_1, SSED_1
     * - Description: Each device automatically attaches to the Leader (DUT).
     * - Pass Criteria: Successfully attaches to DUT.
     */

    sed1.Join(leader, Node::kAsMed);
    ssed1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);

    Log("Step 3: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT.");

    /**
     * Step 3: SED_1, SSED_1
     * - Description: Harness verifies connectivity by instructing each device to
     *   send an ICMPv6 Echo Request to the DUT.
     * - Pass Criteria:
     *   - 3.1 : The DUT MUST send an ICMPv6 Echo Reply to SED_1.
     *   - 3.2 : The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
     */

    nexus.SendAndVerifyEchoRequest(sed1, leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);
    nexus.SendAndVerifyEchoRequest(ssed1, leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("Step 4: SED_1 enable IEEE 802.15.4-2015 Enhanced ACK-based Probing.");

    /**
     * Step 4: SED_1
     * - Description: Harness instructs the device to enable
     *   IEEE 802.15.4-2015 Enhanced ACK-based Probing by sending a
     *   Link Metrics Management Request to the DUT.
     *   - Link Metrics Management TLV payload:
     *     - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Enh-ACK Flags = 0x01 (register a configuration)
     *     - Concatenation of Link Metric Type ID Flags = 0x0a
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     * - Pass Criteria: N/A.
     */

    LinkMetrics::Metrics metrics;
    metrics.mPduCount   = false;
    metrics.mLqi        = false;
    metrics.mLinkMargin = true;
    metrics.mRssi       = false;

    sed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
    sMgmtResponseReceived = false;

    SuccessOrQuit(sed1.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
        leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metrics));

    Log("Step 5: Leader (DUT) automatically responds to SED_1 with a Link Metrics Management Response.");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically responds to SED_1 with a Link Metrics
     *   Management Response.
     * - Pass Criteria: The DUT MUST send a Link Metrics Management Response to
     *   SED_1 containing the following TLVs:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */

    nexus.AdvanceTime(kMgmtResponseTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    Log("Step 6: SSED_1 enable IEEE 802.15.4-2015 Enhanced ACK-based Probing.");

    /**
     * Step 6: SSED_1
     * - Description: Harness instructs the device to enable
     *   IEEE 802.15.4-2015 Enhanced ACK-based Probing by sending a
     *   Link Metrics Management Request to the DUT.
     *   - MLE Link Metrics Management TLV Payload:
     *     - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Enh-ACK Flags = 1 (register a configuration)
     *     - Concatenation of Link Metric Type ID Flags = 0x0a0b
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     *       - E = 0x00
     *       - L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A.
     */

    metrics.mPduCount   = false;
    metrics.mLqi        = false;
    metrics.mLinkMargin = true;
    metrics.mRssi       = true;

    ssed1.Get<LinkMetrics::Initiator>().SetMgmtResponseCallback(HandleMgmtResponse, nullptr);
    ssed1.Get<LinkMetrics::Initiator>().SetEnhAckProbingCallback(HandleEnhAckProbingReport, nullptr);
    sMgmtResponseReceived = false;

    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
        leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metrics));

    Log("Step 7: Leader (DUT) automatically responds to SSED_1 with a Link Metrics Management Response.");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with a Link Metrics
     *   Management Response.
     * - Pass Criteria: The DUT MUST send Link Metrics Management Response to
     *   SSED_1 containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */

    nexus.AdvanceTime(kMgmtResponseTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    Log("Step 8: SSED_1 solicit an Enhanced ACK from the DUT.");

    /**
     * Step 8: SSED_1
     * - Description: Harness instructs the device to solicit an Enhanced ACK from
     *   the DUT by sending a MAC Data Message with the Frame Version subfield
     *   within the MAC header of the message set to 0b10.
     * - Pass Criteria: N/A.
     */

    sEnhAckReportReceived = false;
    ssed1.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kEnhAckTime);

    Log("Step 9: Leader (DUT) automatically responds to SSED_1 with an Enhanced ACK containing link metrics.");

    /**
     * Step 9: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with an Enhanced
     *   Acknowledgement containing link metrics.
     * - Pass Criteria: The DUT MUST reply to SSED_1 with an Enhanced ACK,
     *   containing the following:
     *   - Frame Control Field
     *   - Security Enabled = True
     *   - Header IE
     *     - Element ID = 0x00
     *     - Vendor CID = 0xeab89b (Thread Group)
     *     - Vendor Specific Information
     *       - 1st byte = 0 (Enhanced ACK Link Metrics)
     *       - 2nd byte ... Link Margin data
     *       - 3rd byte ... RSSI data
     */

    VerifyOrQuit(sEnhAckReportReceived);

    Log("Step 10: SED_1 solicit an Enhanced ACK from the DUT.");

    /**
     * Step 10: SED_1
     * - Description: Harness instructs the device to solicit an Enhanced ACK from
     *   the DUT by sending a MAC Data Message with the Frame Version subfield
     *   within the MAC header of the message set to 0b10.
     * - Pass Criteria: N/A.
     */

    sed1.Get<LinkMetrics::Initiator>().SetEnhAckProbingCallback(HandleEnhAckProbingReport, nullptr);
    sEnhAckReportReceived = false;
    sed1.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kEnhAckTime);

    Log("Step 11: Leader (DUT) automatically responds to SED_1 with an Enhanced ACK containing link metrics.");

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically responds to SED_1 with an Enhanced
     *   Acknowledgement containing link metrics.
     * - Pass Criteria: The DUT MUST reply to SED_1 with an Enhanced ACK,
     *   containing the following:
     *   - Header IE
     *   - Element ID = 0x00
     *   - Vendor CID = 0xeab89b (Thread Group)
     *   - Vendor Specific Information
     *     - 1st byte = 0 (Enhanced ACK Link Metrics)
     *     - 2nd byte ... Link Margin data
     */

    VerifyOrQuit(sEnhAckReportReceived);

    Log("Step 12: SSED_1 clear its Enhanced ACK link metrics configuration.");

    /**
     * Step 12: SSED_1
     * - Description: Harness instructs the device to clear its Enhanced ACK link
     *   metrics configuration by instructing it to send a Link Metrics Management
     *   Request to the DUT.
     *   - MLE Link Metrics Management TLV Payload:
     *     - Enhanced ACK Configuration Sub-TLV
     *     - Enh-ACK Flags = 0 (clear enhanced ACK link metric config)
     * - Pass Criteria: N/A.
     */

    sMgmtResponseReceived = false;
    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
        leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckClear, nullptr));

    Log("Step 13: Leader (DUT) automatically responds to SSED_1 with a Link Metrics Management Response.");

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with a Link Metrics
     *   Management Response.
     * - Pass Criteria: The DUT MUST send Link Metrics Management Response to
     *   SSED_1 containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 0 (Success)
     */

    nexus.AdvanceTime(kMgmtResponseTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_SUCCESS);

    Log("Step 14: SSED_1 solicit an Enhanced ACK from the DUT.");

    /**
     * Step 14: SSED_1
     * - Description: Harness instructs the device to solicit an Enhanced ACK from
     *   the DUT by sending a MAC Data Message with the Frame Version subfield
     *   within the MAC header of the message set to 0b10.
     * - Pass Criteria: N/A.
     */

    sEnhAckReportReceived = false;
    ssed1.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kEnhAckTime);

    Log("Step 15: Leader (DUT) automatically responds to SSED_1 with an immediate ACK.");

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically responds to SSED_1 with an immediate ACK.
     * - Pass Criteria: The DUT MUST NOT include a Link Metrics Report in the ACK.
     */

    VerifyOrQuit(!sEnhAckReportReceived);

    Log("Step 16: SSED_1 request 3 metric types for Enhanced ACKs.");

    /**
     * Step 16: SSED_1
     * - Description: Harness verifies that Enhanced ACKs cannot be enabled while
     *   requesting 3 metric types by instructing the device to send the following
     *   Link Metrics Management Request to the DUT.
     *   - MLE Link Metrics Management TLV Payload:
     *     - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Enh-ACK Flags = 1 (register a configuration)
     *     - Concatenation of Link Metric Type ID Flags = 0x090a0b
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 1 (Layer 2 LQI)
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 2 (Link Margin)
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 1 (Exponential Moving Avg)
     *       - Metric Enum = 3 (RSSI)
     * - Pass Criteria: N/A.
     */

    metrics.mPduCount   = false;
    metrics.mLqi        = true;
    metrics.mLinkMargin = true;
    metrics.mRssi       = true;

    sMgmtResponseReceived = false;
    SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
        leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metrics));

    Log("Step 17: Leader (DUT) automatically responds to SSED_1 with a failure.");

    /**
     * Step 17: Leader (DUT)
     * - Description: Automatically responds to the invalid query from SSED_1
     *   with a failure.
     * - Pass Criteria: The DUT MUST send Link Metrics Management Response to
     *   SSED_1 containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 254 (Failure)
     */

    nexus.AdvanceTime(kMgmtResponseTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_OTHER_ERROR);

    Log("Step 18: SSED_1 tries to register reserved Type/Average Enum.");

    /**
     * Step 18: SSED_1
     * - Description: Harness verifies that Enhanced ACKs cannot be enabled while
     *   requesting a reserved Type/Average Enum of value 2 by instructing the
     *   device to send the following Link Metrics Management Request to the DUT.
     *   - MLE Link Metrics Management TLV Payload:
     *     - Enhanced ACK Link Metrics Configuration Sub-TLV
     *     - Enh-ACK Flags = 1 (register a configuration)
     *     - Link Metric Type ID Flags = 0x12
     *       - E = 0x00; L = 0x00
     *       - Type / Average Enum = 2 (Reserved)
     *       - Metric Enum = 2 (Link Margin)
     * - Pass Criteria: N/A.
     */

    {
        LinkMetrics::Metrics metricsReserved;

        metricsReserved.Clear();
        metricsReserved.mLinkMargin = true;
        metricsReserved.mReserved   = true;

        sMgmtResponseReceived = false;
        SuccessOrQuit(ssed1.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(
            leader.Get<Mle::Mle>().GetLinkLocalAddress(), LinkMetrics::kEnhAckRegister, &metricsReserved));
    }

    Log("Step 19: Leader (DUT) automatically responds to the invalid SSED_1 query with a failure.");

    /**
     * Step 19: Leader (DUT)
     * - Description: Automatically responds to the invalid SSED_1 query
     *   with a failure.
     * - Pass Criteria: The DUT MUST respond to SSED_1 with a Link Metrics
     *   Management Response containing the following:
     *   - MLE Link Metrics Management TLV
     *   - Link Metrics Status Sub-TLV = 254 (Failure)
     */

    nexus.AdvanceTime(kMgmtResponseTime);
    VerifyOrQuit(sMgmtResponseReceived);
    VerifyOrQuit(sMgmtStatus == OT_LINK_METRICS_STATUS_OTHER_ERROR);

    nexus.SaveTestInfo("test_1_2_LP_7_1_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_7_1_1();
    printf("All tests passed\n");
    return 0;
}
