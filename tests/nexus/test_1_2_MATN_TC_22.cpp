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
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for MLR registration, in milliseconds.
 */
static constexpr uint32_t kMlrRegistrationTime = 5 * 1000;

/**
 * MLR Timeout Minimum value in seconds.
 */
static constexpr uint32_t kMlrTimeoutMin = 300;

/**
 * Low MLR Timeout value in seconds.
 */
static constexpr uint32_t kMlrTimeoutLow = kMlrTimeoutMin / 4;

/**
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

void TestMatnTc22(void)
{
    /**
     * 5.10.18 MATN-TC-22: Use of low MLR timeout defaults to MLR_TIMEOUT_MIN
     *
     * 5.10.18.1 Topology
     * - BR_1
     * - TD (DUT)
     *
     * 5.10.18.2 Purpose & Description
     * The purpose of this test case is to verify that a Primary BBR that is configured with a low value of MLR timeout
     *   (< MLR_TIMEOUT_MIN) is interpreted as using an MLR timeout of MLR_TIMEOUT_MIN by all the Thread Devices (DUT).
     *
     * Valid DUT Roles:
     * - DUT as TD (FED, REED or Router)
     *
     * Required Devices and Topology:
     * - BR_1: Test bed BR device operating as the Primary BBR.
     * - TD (DUT): Device operating as a Thread FTD (FED, REED or Router).
     *
     * Initial Conditions:
     * - P1: BR_1 MLR timeout value to configured to of BR_1 to be (MLR_TIMEOUT_MIN / 4) seconds.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Multicast        | 5.10.18      | 5.10.18
     */

    Core         nexus;
    Node        &br1  = nexus.CreateNode();
    Node        &dut1 = nexus.CreateNode(); // Router
    Node        &dut2 = nexus.CreateNode(); // FED
    Ip6::Address ma1;

    br1.SetName("BR_1");
    dut1.SetName("DUT_Router");
    dut2.SetName("DUT_FED");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1. Topology addition – TD (DUT). Boot the DUT. Configure the DUT to
     *   register multicast address, MA1, at its parent.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation – BR_1. Topology addition – TD (DUT_Router and DUT_FED). DUTs register MA1.");

    br1.AllowList(dut1);
    dut1.AllowList(br1);
    br1.AllowList(dut2);
    dut2.AllowList(br1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    /** Configure BR_1 with low MLR timeout. */
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout = kMlrTimeoutLow;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }

    dut1.Join(br1, Node::kAsFtd);
    dut2.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(dut1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut2.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    nexus.AddTestVar("MA1", kMA1);
    {
        String<16> timeoutString;
        timeoutString.Append("%lu", ToUlong(kMlrTimeoutMin));
        nexus.AddTestVar("MLR_TIMEOUT_MIN", timeoutString.AsCString());
    }

    SuccessOrQuit(dut1.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    SuccessOrQuit(dut2.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 1
     * - Device: TD (DUT)
     * - Description: Within MLR_TIMEOUT_MIN seconds of initial registration, the device automatically re-registers for
     *   multicast address, MA1, at BR_1.
     * - Pass Criteria:
     *   - The DUT MUST re-register for multicast address, MA1, at BR_1.
     *   - The re-registration MUST occur within the time MLR_TIMEOUT_MIN seconds since initial registration.
     *   - No more than 2 re-registrations MUST occur within this time.
     */
    Log("Step 1: Within MLR_TIMEOUT_MIN seconds, DUT re-registers for MA1.");
    nexus.AdvanceTime(kMlrTimeoutMin * 1000);

    nexus.SaveTestInfo("test_1_2_MATN_TC_22.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc22();
    printf("All tests passed\n");
    return 0;
}
