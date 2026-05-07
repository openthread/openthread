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
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Maximum interval between Router advertisements.
 */
static constexpr uint32_t kMaximumRouterAdvertisementInterval = 32 * 1000;

/**
 * Device mode to configure as a Full Thread Device (FTD).
 */
static const Mle::DeviceMode kFtdDeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle |
                                            Mle::DeviceMode::kModeFullThreadDevice |
                                            Mle::DeviceMode::kModeFullNetworkData);

/**
 * Device mode to configure as a Minimal End Device (MED).
 */
static const Mle::DeviceMode kMedDeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);

void TestMleRouterRoleAllowed(void)
{
    // This test validates that correctly-formatted transmissions occur based on whether the Router
    // role is allowed.  This includes Advertisements (only from REEDs and Active Routers),
    // which should not occur for devices with Router role disallowed.
    // This test verifies Router role disallowed on FED and MED devices, with a duplicate
    // pair used to demonstrate that these transmissions begin in the correct format when
    // re-configured so that the Router role becomes allowed.

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &router       = nexus.CreateNode();
    Node &reed         = nexus.CreateNode();
    Node &fedOnly      = nexus.CreateNode();
    Node &medOnly      = nexus.CreateNode();
    Node &fedAndRouter = nexus.CreateNode();
    Node &medAndRouter = nexus.CreateNode();

    leader.SetName("Leader");
    router.SetName("Router");
    reed.SetName("REED");
    fedOnly.SetName("FED_ONLY");
    medOnly.SetName("MED_ONLY");
    fedAndRouter.SetName("FED_Router");
    medAndRouter.SetName("MED_Router");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form topology");
    // Child links are only set up with the leader and router,
    // so that the REED will not become a router due to a parent request.

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, reed);
    AllowLinkBetween(leader, fedOnly);
    AllowLinkBetween(leader, medOnly);
    AllowLinkBetween(leader, fedAndRouter);
    AllowLinkBetween(leader, medAndRouter);

    AllowLinkBetween(router, reed);
    AllowLinkBetween(router, fedOnly);
    AllowLinkBetween(router, medOnly);
    AllowLinkBetween(router, fedAndRouter);
    AllowLinkBetween(router, medAndRouter);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1.1: Leader");
    // All Leader advertisements are expected to have a router source address

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1.2: Join Router");
    // All Router advertisements are expected to have a router source address

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1.3: Join end devices (only) - reed, fed, & med");

    // All REED advertisements should have a child source address
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
    reed.Join(leader);
    // Router role is allowed for the REED, but should not upgrade due to the updated threshold
    VerifyOrQuit(reed.Get<Mle::Mle>().IsRouterRoleAllowed());

    fedOnly.Join(leader);
    // Attach as an FTD, but not configured as router-eligible
    SuccessOrQuit(fedOnly.Get<Mle::Mle>().SetRouterEligible(false));
    VerifyOrQuit(!fedOnly.Get<Mle::Mle>().IsRouterRoleAllowed());

    medOnly.Join(leader, Node::kAsMed);
    VerifyOrQuit(!medOnly.Get<Mle::Mle>().IsRouterRoleAllowed());

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medOnly.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1.4: Join end devices that will have router role allowed - fed & med");

    fedAndRouter.Join(leader);
    // Attach as an FTD, but not configured as router-eligible
    SuccessOrQuit(fedAndRouter.Get<Mle::Mle>().SetRouterEligible(false));
    VerifyOrQuit(fedAndRouter.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(!fedAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    medAndRouter.Join(leader, Node::kAsMed);
    VerifyOrQuit(!medAndRouter.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(!medAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1.5: Child attachment");
    // FED and MED nodes with router role disallowed should not advertise during this time

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedAndRouter.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medAndRouter.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Set FED & MED Router Role allowed");

    // Now router eligible so that the former FED is allowed to take the Router role
    SuccessOrQuit(fedAndRouter.Get<Mle::Mle>().SetRouterEligible(true));
    VerifyOrQuit(fedAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    // Enabling FTD mode so that the former MED is allowed to take the Router role
    SuccessOrQuit(medAndRouter.Get<Mle::Mle>().SetDeviceMode(kFtdDeviceMode));
    VerifyOrQuit(medAndRouter.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(medAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    // FED and MED nodes should advertise as Routers at least once during this time
    nexus.AdvanceTime(kAttachToRouterTime);
    nexus.AdvanceTime(kMaximumRouterAdvertisementInterval);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedAndRouter.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(medAndRouter.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Set FED & MED back to their original configuration");

    // Setting the router role by configuration is expected to immediately detach
    SuccessOrQuit(fedAndRouter.Get<Mle::Mle>().SetRouterEligible(false));
    VerifyOrQuit(!fedAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    // Setting the mode by configuration is expected to immediately detach
    SuccessOrQuit(medAndRouter.Get<Mle::Mle>().SetDeviceMode(kMedDeviceMode));
    VerifyOrQuit(!medAndRouter.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(!medAndRouter.Get<Mle::Mle>().IsRouterRoleAllowed());

    // FED and MED nodes should attach as children and send no further advertisements
    nexus.AdvanceTime(kAttachToRouterTime);
    nexus.AdvanceTime(kMaximumRouterAdvertisementInterval);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medOnly.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fedAndRouter.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(medAndRouter.Get<Mle::Mle>().IsChild());

    nexus.SaveTestInfo("test_mle_router_role_allowed.json", &leader);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMleRouterRoleAllowed();
    printf("All tests passed\n");
    return 0;
}
