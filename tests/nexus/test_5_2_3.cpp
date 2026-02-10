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

#include <stdarg.h>
#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * 5.2.3 Leader rejects CoAP Address Solicit (2-hops from Leader)
 *
 * 5.2.3.1 Topology
 * - Build a topology with the DUT as the Leader and have a total of 32 routers, including the Leader.
 * - Attempt to attach a 33rd router, two hops from the leader.
 *
 * 5.2.3.2 Purpose & Description
 * The purpose of this test case is to show that the DUT will only allow 32 active routers on the network and
 * reject the Address Solicit Request from a 33rd router - that is 2-hops away - with a No Address Available status.
 *
 * Spec Reference                               | V1.1 Section    | V1.3.0 Section
 * ---------------------------------------------|-----------------|-----------------
 * Attaching to a Parent / Router ID Assignment | 4.7.1 / 5.9.10  | 4.5.1 / 5.9.10
 */

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 * This duration accounts for MLE attach process and ROUTER_SELECTION_JITTER.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a node to try to become a router.
 */
static constexpr uint32_t kWaitTime = 10 * 1000;

static constexpr uint8_t kMaxRouters = 32;

void Test5_2_3(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node *routers[kMaxRouters];

    leader.SetName("Leader");

    for (uint8_t i = 0; i < kMaxRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
        routers[i]->SetName("Router", i + 1);
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(kMaxRouters);
    for (uint8_t i = 0; i < kMaxRouters; i++)
    {
        routers[i]->Get<Mle::Mle>().SetRouterUpgradeThreshold(kMaxRouters);
    }

    // Topology:
    // Leader <-> Router 1..31
    // Router 32 <-> Router 1
    for (uint8_t i = 0; i < kMaxRouters - 1; i++)
    {
        leader.AllowList(*routers[i]);
        routers[i]->AllowList(leader);
    }

    routers[kMaxRouters - 1]->AllowList(*routers[0]);
    routers[0]->AllowList(*routers[kMaxRouters - 1]);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1: All
     * - Description: Begin wireless sniffer and ensure topology is created and connectivity between nodes.
     * - Pass Criteria: Topology is created, the DUT is the Leader of the network and there is a total of 32
     *   active routers, including the Leader.
     */
    Log("Step 1: All");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (uint8_t i = 0; i < kMaxRouters - 1; i++)
    {
        routers[i]->Join(leader);
    }

    nexus.AdvanceTime(kAttachToRouterTime);

    for (uint8_t i = 0; i < kMaxRouters - 1; i++)
    {
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouter());
    }

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2: Router_31
     * - Description: The harness causes Router_31 to attach to the network and send an Address Solicit Request to
     *   become an active router.
     * - Pass Criteria: N/A
     */
    Log("Step 2: Router_31");
    // Handled in Step 1 for simplicity as Router 31 is one of the 31 routers joining the Leader.

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3: Leader (DUT)
     * - Description: The DUT receives the Address Solicit Request and automatically replies with an Address
     *   Solicit Response.
     * - Pass Criteria:
     *   - The DUT MUST reply to the Address Solicit Request with an Address Solicit Response containing:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload:
     *       - Status TLV (value = Success)
     *       - RLOC16 TLV
     *       - Router Mask TLV
     */
    Log("Step 3: Leader (DUT)");
    // Handled in Step 1.

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends MLE Advertisements.
     * - Pass Criteria: The DUT’s MLE Advertisements MUST contain the Route64 TLV with 32 assigned Router IDs.
     */
    Log("Step 4: Leader (DUT)");
    // Handled in Step 1.

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5: Router_32
     * - Description: The harness causes Router_32 to attach to any of the active routers, 2-hops from the leader,
     *   and to send an Address Solicit Request to become an active router.
     * - Pass Criteria: N/A
     */
    Log("Step 5: Router_32");
    routers[kMaxRouters - 1]->Join(*routers[0]);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(routers[kMaxRouters - 1]->Get<Mle::Mle>().IsChild());

    // Force Router 32 to try to become a router
    routers[kMaxRouters - 1]->Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 6: Leader (DUT)
     * - Description: The DUT receives the Address Solicit Request and automatically replies with an Address
     *   Solicit Response.
     * - Pass Criteria:
     *   - The DUT MUST reply to the Address Solicit Request with an Address Solicit Response containing:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload:
     *       - Status TLV (value = No Address Available)
     */
    Log("Step 6: Leader (DUT)");
    nexus.AdvanceTime(kWaitTime);
    VerifyOrQuit(routers[kMaxRouters - 1]->Get<Mle::Mle>().IsChild());

    nexus.SaveTestInfo("test_5_2_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_2_3();
    printf("All tests passed\n");
    return 0;
}
