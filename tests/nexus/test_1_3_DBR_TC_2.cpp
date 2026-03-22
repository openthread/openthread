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
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 100 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingResponseTime = 5 * 1000;

/**
 * Time to advance for leader timeout (NETWORK_ID_TIMEOUT + 10s), in milliseconds.
 */
static constexpr uint32_t kLeaderTimeoutTime = (120 + 10) * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0xabcd;

/**
 * Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void Test_1_3_DBR_TC_2(void)
{
    /**
     * 1.2. [1.3] [CERT] Reachability - Multiple BRs - Single Thread / Single Infrastructure Link
     *
     * 1.2.1. Purpose
     * To test the following:
     * 1. Bi-directional reachability between Thread devices and infrastructure devices
     * 2. No existing IPv6 infrastructure
     * 3. Multiple BRS
     * 4. DUT BR adopts existing ULA and OMR prefixes
     *
     * 1.2.2. Topology
     * - BR 1 (DUT) - Thread Border Router
     * - BR 2-Test Bed device operating as a Thread Border Router Device and the Leader
     * - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth 1-Test bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference | V1.1 Section | V1.3.0 Section
     * ---------------|--------------|---------------
     * Reachability   | N/A          | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &br2  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: Eth 1, BR 2 Description (DBR-1.2): Form topology. Wait for BR_2 to...");

    /**
     * Step 1
     * - Device: Eth 1, BR 2
     * - Description (DBR-1.2): Form topology. Wait for BR_2 to: 1. Register as border router in Thread Network Data 2.
     *   Send multicast ND RAS PIO with ULA prefix (ULA_init) RIO with OMR prefix (OMR_init)
     * - Pass Criteria: N/A
     */

    eth1.mInfraIf.Init(eth1);

    br2.AllowList(br1);

    br2.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omrInit;
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrInit));
    nexus.AddTestVar("OMR_INIT", omrInit.ToString().AsCString());

    Ip6::Prefix ulaInit;
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(ulaInit));
    nexus.AddTestVar("ULA_INIT", ulaInit.ToString().AsCString());

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(br2.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        const MeshCoP::ExtendedPanId &extPanId = datasetInfo.Get<MeshCoP::Dataset::kExtendedPanId>();
        nexus.AddTestVar("EXT_PAN_ID_VAR", extPanId.ToString().AsCString());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR 1 (DUT) Description (DBR-1.2): Enable: turn on device.");

    /**
     * Step 2
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.2): Enable: turn on device.
     * - Pass Criteria: N/A
     */

    br1.AllowList(br2);
    br1.AllowList(ed1);

    br1.Join(br2);
    nexus.AdvanceTime(kJoinNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR 1 (DUT) Description (DBR-1.2): Automatically registers itself as a border router.");

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.2): Automatically registers itself as a border router in the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT MUST NOT register a new OMR Prefix in the Thread Network Data.
     */

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: BR_1 (DUT) Description (DBR-1.2): Automatically multicasts ND RAs on AIL.");

    /**
     * Step 4
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.2): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAS, including the following
     *   - IPv6 destination MUST be ff02::1
     *   - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
     *   - Route Information Option (RIO) Prefix OMR prefix = OMR_init.
     *   - MUST NOT include Prefix Information Option (PIO)
     *   - Any ND RA messages MUST NOT include the following: Route Information Option (RIO) Prefix::/0 (the zero-length
     *     prefix)
     */

    // Time already advanced.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4b: Device: ED 1 Description (DBR-1.2): Enable device. It attaches to the DUT.");

    /**
     * Step 4b
     * - Device: ED 1
     * - Description (DBR-1.2): Enable device. It attaches to the DUT.
     * - Pass Criteria:
     *   - Verify the DUT still adheres to step 3 pass criteria for the Network Data when applied to the Thread
     *     Network Data that is sent to the Child ED 1.
     */

    ed1.AllowList(br1);
    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: Eth 1 Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request.");

    /**
     * Step 5
     * - Device: Eth 1
     * - Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request to ED 1 via BR 1 Thread link.
     *   1. IPv6 Source: its address starting with prefix ULA_init 2. IPv6 Destination: ED_1 OMR address starting with
     *   prefix OMR init
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *   - 1. IPv6 Source: ED_1 OMR address starting with prefix OMR init
     *   - 2. IPv6 Destination: Eth_1 ULA address starting with prefix ULA init
     */

    const Ip6::Address &eth1Ula = eth1.mInfraIf.FindMatchingAddress(ulaInit.ToString().AsCString());
    const Ip6::Address &ed1Omr  = ed1.FindMatchingAddress(omrInit.ToString().AsCString());

    nexus.AddTestVar("ETH_1_ULA_ADDR", eth1Ula.ToString().AsCString());
    nexus.AddTestVar("ED_1_OMR_ADDR", ed1Omr.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Ula, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: ED_1 Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request.");

    /**
     * Step 6
     * - Device: ED_1
     * - Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request to Eth 1. 1. IPv6 Source: its
     *   OMR address starting with prefix OMR init) 2. IPv6 Destination: Eth_1 ULA address starting with prefix ULA init
     * - Pass Criteria:
     *   - ED_1 receives an ICMPv6 Echo Reply from Eth_1 via BR_1 Thread link
     *   - 1. IPv6 Source: Eth_1 ULA address starting with prefix ULA init
     *   - 2. IPv6 Destination: ED_1 OMR address starting with prefix OMR init
     */

    ed1.SendEchoRequest(eth1Ula, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: BR 2 Description (DBR-1.2): Harness disables the device.");

    /**
     * Step 7
     * - Device: BR 2
     * - Description (DBR-1.2): Harness disables the device. Note: this will eventually lead to the timing out and
     *   removal of the OMR init prefix in the Thread Network Data. However, during the short time period in which the
     *   next 3 test steps are executed this advertised OMR_init prefix remains valid and BR_1 keeps operating as is.
     * - Pass Criteria: N/A
     */

    br2.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: BR 1 (DUT) Description (DBR-1.2): Repeat Step 4");

    /**
     * Step 8
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.2): Repeat Step 4
     * - Pass Criteria:
     *   - Repeat Step 4
     */

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: Eth 1 Description (DBR-1.2): Repeat Step 5");

    /**
     * Step 9
     * - Device: Eth 1
     * - Description (DBR-1.2): Repeat Step 5
     * - Pass Criteria:
     *   - Repeat Step 5
     */

    eth1.mInfraIf.SendEchoRequest(eth1Ula, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: ED 1 Description (DBR-1.2): Repeat Step 6");

    /**
     * Step 10
     * - Device: ED 1
     * - Description (DBR-1.2): Repeat Step 6
     * - Pass Criteria:
     *   - Repeat Step 6
     */

    ed1.SendEchoRequest(eth1Ula, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Device: N/A Description (DBR-1.2): Harness waits for Leader timeout to occur.");

    /**
     * Step 11
     * - Device: N/A
     * - Description (DBR-1.2): Harness waits for Leader timeout to occur, after loss of BR_2 Leader. Note: this may
     *   be implemented by waiting at least ( NETWORK_ID_TIMEOUT + 10) seconds.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kLeaderTimeoutTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Device: BR 1 (DUT) Description (DBR-1.2): Automatically becomes Leader and advertises...");

    /**
     * Step 12
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.2): Automatically becomes Leader and advertises its own OMR prefix, as well as a ULA prefix
     *   for the adjacent infrastructure link (AIL).
     * - Pass Criteria:
     *   - The DUT MUST become Leader of a new Partition, and MUST register a new OMR prefix OMR 1 in the Thread Network
     *     Data.
     *   - OMR 1 MUST NOT be equal to OMR_init.
     *   - DUT MUST advertise a route in Network Data as follows: Prefix TLV Prefix fc00::/7 Has Route sub-TLV Prf
     *     'Medium' (00) or 'Low' ( 11)
     */

    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    Ip6::Prefix omr1;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr1));
    nexus.AddTestVar("OMR_1", omr1.ToString().AsCString());

    Ip6::Prefix ula1;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(ula1));
    nexus.AddTestVar("ULA_1", ula1.ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Device: BR_1 (DUT) Description (DBR-1.2): Automatically multicasts ND RAs on AIL.");

    /**
     * Step 13
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.2): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAS, including the following
     *   - IPv6 destination MUST be ff02::1
     *   - M bit and O bit MUST be '0'
     *   - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
     *   - MUST include Route Information Option (RIO) Prefix OMR_1 Prf 'Medium' (00) or 'Low' (11)
     *   - MUST include Prefix Information Option (PIO) Prefix ULA 1 A bit MUST be '1'
     *   - ULA 1 MUST contain the Extended PAN ID as follows:
     *   - Global ID equals the 40 most significant bits of the Extended PAN ID
     *   - Subnet ID equals the 16 least significant bits of the Extended PAN ID
     */

    nexus.AdvanceTime(kBrActionTime);

    nexus.SaveTestInfo("test_1_3_DBR_TC_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_2();
    printf("All tests passed\n");
    return 0;
}
