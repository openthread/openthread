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
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 * This duration accounts for MLE attach process and ROUTER_SELECTION_JITTER.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachToChildTime = 10 * 1000;

/**
 * Time interval for advancing time in a loop.
 */
static constexpr uint32_t kOneSecond = 1000;

/**
 * Delay between node joins to ensure stability.
 */
static constexpr uint32_t kJoinDelay = 10 * 1000;

/**
 * Number of active routers in the topology.
 */
static constexpr uint16_t kNumRouters = 16;

void Test5_2_7(void)
{
    /**
     * 5.2.7 REED Synchronization
     *
     * 5.2.7.1 Topology
     * - Topology A
     * - Topology B
     * - Build a topology that has a total of 16 active routers, including the Leader, with no communication
     *   constraints.
     *
     * 5.2.7.2 Purpose & Description
     * The purpose of this test case is to validate the REED’s Synchronization procedure after attaching to a network
     *   with multiple Routers. A REED MUST process incoming Advertisements and perform a one-way frame-counter
     *   synchronization with at least 3 neighboring Routers. When Router receives unicast MLE Link Request from REED,
     *   it replies with MLE Link Accept.
     *
     * Spec Reference                     | V1.1 Section | V1.3.0 Section
     * -----------------------------------|--------------|---------------
     * REED and FED Synchronization       | 4.7.7.4      | 4.7.1.4
     */

    Core  nexus;
    Node *routers[kNumRouters];

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
        if (i == 0)
        {
            routers[i]->SetName("LEADER");
        }
        else
        {
            routers[i]->SetName("ROUTER", i);
        }
    }

    Node &reed1 = nexus.CreateNode();
    reed1.SetName("REED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology formation
     *   - The REED device is added last
     *   - If DUT=REED
     *     - the DUT may attach to any router
     *   - If DUT=Router
     *     - the REED is not allowed to attach to the DUT
     *     - the REED is limited to 2 neighbors, including the DUT
     * - Pass Criteria: N/A
     */
    routers[0]->Form();
    nexus.AdvanceTime(kFormNetworkTime);

    for (uint16_t i = 1; i < kNumRouters; i++)
    {
        routers[i]->Join(*routers[0]);
        nexus.AdvanceTime(kJoinDelay);
    }

    for (uint32_t time = 0; time < kAttachToRouterTime; time += kOneSecond)
    {
        nexus.AdvanceTime(kOneSecond);
        bool allRouters = true;
        for (uint16_t i = 0; i < kNumRouters; i++)
        {
            if (!routers[i]->Get<Mle::Mle>().IsRouterOrLeader())
            {
                allRouters = false;
                break;
            }
        }
        if (allRouters)
        {
            break;
        }
    }

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        if (!routers[i]->Get<Mle::Mle>().IsRouterOrLeader())
        {
            Log("Node %u (name %s) is NOT a router, role %s", i, routers[i]->GetName(),
                Mle::RoleToString(routers[i]->Get<Mle::Mle>().GetRole()));
        }
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouterOrLeader());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: REED_1");

    /**
     * Step 2: REED_1
     * - Description: Automatically joins the topology
     * - Pass Criteria:
     *   - For DUT = REED: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request
     */
    reed1.Join(*routers[0]);
    nexus.AdvanceTime(kAttachToChildTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!reed1.Get<Mle::Mle>().IsRouterOrLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: REED_1");

    /**
     * Step 3: REED_1
     * - Description: Automatically sends Link Request to neighboring Routers
     * - Pass Criteria:
     *   - For DUT = REED: The DUT MUST send a unicast Link Request to at least three neighbors
     *   - The following TLVs MUST be present in the Link Request:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - Source Address TLV
     *     - Version TLV
     */
    nexus.AdvanceTime(kAttachToChildTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Automatically sends Link Accept to REED_1
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST send Link Accept to the REED; the DUT MUST NOT send a Link Accept And Request
     *     message.
     *   - The following TLVs MUST be present in the Link Accept message:
     *     - Link-layer Frame Counter TLV
     *     - Source Address TLV
     *     - Response TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     */
    nexus.AdvanceTime(kAttachToChildTime);

    nexus.SaveTestInfo("test_5_2_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_2_7();
    printf("All tests passed\n");
    return 0;
}
