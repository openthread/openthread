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
static constexpr uint32_t kFormNetworkTime = 10 * 1000;

/**
 * Time to advance for a node to join as a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 60 * 1000;

/**
 * Time to advance for the BBR selection to complete, in milliseconds.
 */
static constexpr uint32_t kBbrSelectionTime = 10 * 1000;

/**
 * Time to wait for BR_2 to become Primary BBR and Leader, in milliseconds.
 */
static constexpr uint32_t kWaitBbrTime = 140 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

void HandleMdnsBrowse(otInstance *, const otPlatDnssdBrowseResult *) {}

void TestBbrTc3(void)
{
    /**
     * 5.11.3 BBR-TC-03: mDNS discovery of BBR function
     *
     * 5.11.3.1 Topology
     * - BR_1: BR device initially operating as the Primary BBR and Leader.
     * - BR_2: BR device initially operating as a Secondary BBR.
     * - Host_1 and Host_2 : Test bed devices operating as a non-Thread IPv6 hosts. They are used to send out the
     *                       mDNS queries. Two hosts are used to be able to track their queries more easily during pkt
     *                       verification.
     *
     * 5.11.3.2 Purpose & Description
     * The purpose of this test case is to verify that a BBR Function (both Primary and Secondary) can be discovered
     *   using mDNS and that any relevant changes are reflected in the mDNS data sent by the BBR. Also, to verify that
     *   the mandatory mDNS data fields are present and in the correct format. The BBR Sequence Number updating is not
     *   verified.
     *
     * Spec Reference | V1.2 Section
     * ---------------|-------------
     * BBR Discovery  | 5.11.3
     */

    Core  nexus;
    Node &br1   = nexus.CreateNode();
    Node &br2   = nexus.CreateNode();
    Node &host1 = nexus.CreateNode();
    Node &host2 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    host1.SetName("HOST_1");
    host2.SetName("HOST_2");

    br1.Form();

    {
        MeshCoP::Dataset::Info datasetInfo;
        String<17>             xpanIdString;

        SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        nexus.AddTestVar("NETWORK_NAME", datasetInfo.mNetworkName.m8);
        xpanIdString.AppendHexBytes(datasetInfo.mExtendedPanId.m8, sizeof(datasetInfo.mExtendedPanId.m8));
        nexus.AddTestVar("XPAN_ID", xpanIdString.AsCString());
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation - BR_1");

    /**
     * Step 0
     * - Device: BR_1
     * - Description: Topology formation - BR_1
     * - Pass Criteria:
     *   - N/A
     */
    br1.AllowList(br2);
    br2.AllowList(br1);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);
    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0b: Topology addition - BR_2");

    /**
     * Step 0b
     * - Device: BR_2
     * - Description: Topology addition - BR_2
     * - Pass Criteria:
     *   - N/A
     */
    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);
    SuccessOrQuit(br2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    host1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    SuccessOrQuit(host1.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    host1.Get<Dns::Multicast::Core>().SetAutoEnableMode(false);
    SuccessOrQuit(host1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    host2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    SuccessOrQuit(host2.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    host2.Get<Dns::Multicast::Core>().SetAutoEnableMode(false);
    SuccessOrQuit(host2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    nexus.AdvanceTime(kBbrSelectionTime);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Harness instructs the device to send an mDNS query (per P2).");

    /**
     * Step 1
     * - Device: Host_1
     * - Description: Harness instructs the device to send an mDNS query (per P2).
     * - Pass Criteria:
     *   - N/A
     */
    {
        Dns::Multicast::Core::Browser browser;

        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
        VerifyOrQuit(host1.Get<Dns::Multicast::Core>().IsEnabled());

        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleMdnsBrowse;
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kStabilizationTime);
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    nexus.AddOmrPrefixTestVar("OMR_PREFIX_STEP_0", br1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Automatically responds to the mDNS query.");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Automatically responds to the mDNS query.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST either 1. Unicast an mDNS response message, destined to the UDP source port of the query or to
     *     port 5353, or 2. Multicast an mDNS response message, destined to UDP port 5353, containing the following TXT
     *     records in specified format:
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - TXT record key: rv, TXT record value: 1, Value format: String
     *   - TXT record key: tv, TXT record value: 1.2.0 or higher, Value format: String (5 bytes)
     *   - TXT record key: sb, TXT record value: Verify Bit 3-4: 0b10, Verify Bit 7: 1, Verify Bit 8: 1, Value format:
     *     Binary (4 bytes)
     *   - TXT record key: nn, TXT record value: NetwName1, Value format: String
     *   - TXT record key: xp, TXT record value: <Equal to XPAN ID>, Value format: Binary (8 bytes)
     *   - TXT record key: omr, TXT record value: <byte 0x40 followed by 8 bytes of the OMR prefix created by BR_1>,
     *     Value format: Binary (9 bytes)
     *   - and OPTIONALLY containing vendor-specific data in the following format:
     *   - TXT record key: v<anyname>, TXT record value: <any vendor data>, Value format: <any data up to 64 bytes>
     *   - TXT record key: vo, TXT record value: <vendor OID>, Value format: Binary uint24
     *   - Above, v<anyname> stands for any TXT record key that starts with a lowercase v character. There may be zero,
     *     or multiple, of such keys present. If such vendor-specific data is included, the vo key MUST be included as
     *     well.
     *   - Also, verify that the complete DNS-SD Service Instance Name ends with the string ._meshcop._udp_.local. and
     *     has >1 characters before this prefix.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Automatically responds to the mDNS query.");

    /**
     * Step 3
     * - Device: BR_2
     * - Description: Automatically responds to the mDNS query.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST either 1. Unicast an mDNS response message, destined to the UDP source port of the query or to
     *     port 5353, or 2. Multicast an mDNS response message, destined to UDP port 5353 containing the following TXT
     *     records in specified format:
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - TXT record key: sb, TXT record value: Verify Bit 3-4: 0b10, Verify Bit 7: 1, Verify Bit 8: 0, Value format:
     *     Binary (4 bytes)
     *   - rv,tv,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3b: Harness instructs device to disable the BBR function.");

    /**
     * Step 3b
     * - Device: BR_2
     * - Description: Only if DUT=BR_1: Harness instructs device to disable the BBR function. Note: see 5.10.13 step
     *   34b for details and reason for this.
     * - Pass Criteria:
     *   - N/A
     */
    br2.Get<BackboneRouter::Local>().SetEnabled(false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: The device must be rebooted (reset); wait until it is back online.");

    {
        MeshCoP::Dataset::Info datasetInfo;

        SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        br1.Reset();

        br1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    br1.Get<ThreadNetif>().Up();
    SuccessOrQuit(br1.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kBbrSelectionTime);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Harness instructs the device to send an mDNS query.");

    /**
     * Step 5
     * - Device: Host_1
     * - Description: Harness instructs the device to send an mDNS query.
     * - Pass Criteria:
     *   - N/A
     */
    {
        Dns::Multicast::Core::Browser browser;

        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleMdnsBrowse;
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kStabilizationTime);
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    nexus.AddOmrPrefixTestVar("OMR_PREFIX_STEP_4", br1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Automatically responds to the mDNS query.");

    /**
     * Step 6
     * - Device: BR_1
     * - Description: Automatically responds to the mDNS query.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast or multicast an mDNS response message containing :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - rv,tv,sb,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Automatically responds to the mDNS query.");

    /**
     * Step 7
     * - Device: BR_2
     * - Description: Automatically responds to the mDNS query.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST unicast or multicast an mDNS response message containing :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - sb <as in step 3> Binary (4 bytes)
     *   - rv,tv,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7b: Harness instructs device to enable the BBR function again.");
    br2.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: BR_1 powered down; wait 140s for BR_2 to become Primary BBR and Leader.");
    br1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kWaitBbrTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(br2.Get<BackboneRouter::Local>().IsPrimary());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Harness instructs the device to send an mDNS query.");

    /**
     * Step 9
     * - Device: Host_2
     * - Description: Harness instructs the device to send an mDNS query.
     * - Pass Criteria:
     *   - N/A
     */
    {
        Dns::Multicast::Core::Browser browser;

        SuccessOrQuit(host2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleMdnsBrowse;
        SuccessOrQuit(host2.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kStabilizationTime);
        SuccessOrQuit(host2.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    nexus.AddOmrPrefixTestVar("OMR_PREFIX_STEP_10", br2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9b: Optionally responds to the mDNS query, as a BR with disabled BBR Function.");

    /**
     * Step 9b
     * - Device: BR_1
     * - Description: Optionally responds to the mDNS query, as a BR with disabled BBR Function. Note: the power down
     *   of step 8 is not actual powering down in the TH context; rather the Thread Interface and thereby BBR Function
     *   are disabled during the test via the THCI but the backbone interface remains active typically.
     * - Pass Criteria:
     *   - Optionally unicasts or multicasts an mDNS response message containing at least :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sb, TXT record value: Verify Bit 3-4: 0b00 or 0b01, Verify Bit 7: 0, Verify Bit 8: 0, Value
     *     format: Binary (4 bytes)
     *   - (other fields not verified)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Automatically responds to the mDNS query, as a PBBR.");

    /**
     * Step 10
     * - Device: BR_2
     * - Description: Automatically responds to the mDNS query, as a PBBR.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST unicast or multicast an mDNS response message containing :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - sb <as in step 2> Binary (4 bytes)
     *   - rv,tv,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: BR_1 powered up; wait 30s for it to join the Thread Network.");

    /**
     * Step 11
     * - Device: BR_1
     * - Description: The device must be powered up Afterwards, the harness waits 10 seconds for it to join the Thread
     *   Network.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST join the Partition of BR_2.
     */
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);
    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    br1.Get<ThreadNetif>().Up();
    SuccessOrQuit(br1.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kBbrSelectionTime);
    nexus.AdvanceTime(kStabilizationTime);

    // Reset mDNS state to ensure they respond to Step 12 query
    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
    SuccessOrQuit(br2.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
    nexus.AdvanceTime(1000);
    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(br2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Harness instructs the device to send an mDNS query.");

    /**
     * Step 12
     * - Device: Host_1
     * - Description: Harness instructs the device to send an mDNS query.
     * - Pass Criteria:
     *   - N/A
     */
    {
        Dns::Multicast::Core::Browser browser;

        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleMdnsBrowse;
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kStabilizationTime);
        SuccessOrQuit(host1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    nexus.AddOmrPrefixTestVar("OMR_PREFIX_STEP_11", br1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Automatically Responds to the mDNS query, as a SBBR.");

    /**
     * Step 13
     * - Device: BR_1
     * - Description: Automatically Responds to the mDNS query, as a SBBR.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast or multicast an mDNS response message containing :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - sb <as in step 3> Binary (4 bytes)
     *   - rv,tv,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Automatically responds to the mDNS query, as the PBBR");

    /**
     * Step 14
     * - Device: BR_2
     * - Description: Automatically responds to the mDNS query, as the PBBR
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST unicast or multicast an mDNS response message containing :
     *   - TXT record key: dn, TXT record value: TDN, Value format: String
     *   - TXT record key: bb, TXT record value: 61631 (BB_PORT default), Value format: Binary uint16
     *   - TXT record key: sq, TXT record value: n/a, Value format: Binary uint8
     *   - sb <as in step 2> Binary (4 bytes)
     *   - rv,tv,nn,xp,omr <as in step 2> <as in step 2>
     *   - Verify DNS-SD Service Instance Name as in step 2.
     */

    nexus.SaveTestInfo("test_1_2_BBR_TC_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBbrTc3();
    printf("All tests passed\n");
    return 0;
}
