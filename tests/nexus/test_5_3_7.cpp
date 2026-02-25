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

#include "common/preference.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * The 2001::/64 prefix used in the test.
 */
static const char kPrefix[] = "2001::/64";

/**
 * The address used for duplicate address detection.
 */
static const char kDuplicateAddress[] = "2001::1";

/**
 * The identifier used for Echo Request.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

void Test5_3_7(void)
{
    /**
     * 5.3.7 Duplicate Address Detection
     *
     * 5.3.7.1 Topology
     * - Leader (DUT)
     * - Router_1
     * - Router_2
     * - MED_1 (Attached to Router_1)
     * - MED_2 (Attached to Leader)
     * - SED_1 (Attached to Router_2)
     *
     * 5.3.7.2 Purpose & Description
     * The purpose of this test case is to validate the DUT’s ability to perform duplicate address detection.
     *
     * Spec Reference                   | V1.1 Section | V1.3.0 Section
     * ---------------------------------|--------------|---------------
     * Duplicate IPv6 Address Detection | 5.6          | 5.6
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &med2    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med1.SetName("MED_1");
    med2.SetName("MED_2");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, router2);
    nexus.AllowLinkBetween(leader, med2);

    nexus.AllowLinkBetween(router1, router2);
    nexus.AllowLinkBetween(router1, med1);

    nexus.AllowLinkBetween(router2, sed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    med2.Join(leader, Node::kAsMed);
    sed1.Join(router2, Node::kAsSed);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med2.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader (DUT)");

    /**
     * Step 2: Leader (DUT)
     * - Description: Transmit MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_2");

    /**
     * Step 3: Router_2
     * - Description: Harness configures the following On-Mesh Prefix on the device:
     *   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     * - Pass Criteria: N/A
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix));
        config.mStable       = true;
        config.mDefaultRoute = true;
        config.mSlaac        = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mPreference   = Preference::kMedium;

        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: MED_1, SED_1");

    /**
     * Step 4: MED_1, SED_1
     * - Description: Harness configures both devices with the same 2001:: address.
     * - Pass Criteria: N/A
     */

    {
        Ip6::Netif::UnicastAddress unicastAddress;

        unicastAddress.InitAsThreadOriginGlobalScope();
        SuccessOrQuit(unicastAddress.GetAddress().FromString(kDuplicateAddress));

        SuccessOrQuit(med1.Get<ThreadNetif>().AddExternalUnicastAddress(unicastAddress));
        SuccessOrQuit(sed1.Get<ThreadNetif>().AddExternalUnicastAddress(unicastAddress));
    }

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: MED_2");

    /**
     * Step 5: MED_2
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the address configured on MED_1 and
     *   SED_1 with Prefix 2001::
     * - Pass Criteria:
     *   - The DUT MUST multicast an Address Query message to the Realm-Local All-Routers address (FF03::2):
     *     - ADDR_QRY.req (/aq) - Address Query Request
     *     - CoAP URI-Path: NON POST coap://[<FF03::2>]:MM/a/aq
     *     - CoAP Payload:
     *       - Target EID TLV
     */

    {
        Ip6::Address destination;

        SuccessOrQuit(destination.FromString(kDuplicateAddress));
        med2.SendEchoRequest(destination, kEchoIdentifier);
    }

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1, Router_2");

    /**
     * Step 6: Router_1, Router_2
     * - Description: Automatically respond with Address Notification message with matching Target TLVs.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically sends a Multicast Address Error Notification.
     * - Pass Criteria:
     *   - The DUT MUST issue an Address Error Notification message to the Realm-Local All-Routers multicast address
     *     (FF03::2):
     *     - ADDR_ERR.ntf(/ae) - Address Error Notification
     *     - CoAP URI-Path: NON POST coap://[<peer address>]:MM/a/ae
     *     - CoAP Payload:
     *       - Target EID TLV
     *       - ML-EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator,,,
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_3_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_7();
    printf("All tests passed\n");
    return 0;
}
