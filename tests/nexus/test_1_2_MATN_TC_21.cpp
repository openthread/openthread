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

#include "common/as_core_type.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

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
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Multicast address MA1 (admin-local).
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * Multicast address MA3 (global).
 */
static const char kMA3[] = "ff0e::1234:777a:3";

/**
 * Multicast address MA5 (mesh-local).
 */
static const char kMA5[] = "ff03::1234:777a:5";

/**
 * Multicast address MA6 (link-local).
 */
static const char kMA6[] = "ff02::1";

/**
 * Invalid multicast address MAe1 (unicast).
 */
static const char kMAe1[] = "fd0e::1234:777a:1";

/**
 * Invalid multicast address MAe2 (unspecified).
 */
static const char kMAe2[] = "::";

/**
 * Invalid multicast address MAe3 (unicast).
 */
static const char kMAe3[] = "cafe::e0ff";

/**
 * Helper to send MLR.req with arbitrary address data.
 */
void SendMlrRequest(Node &aSource, const Ip6::Address &aDestination, const void *aAddressesData, uint16_t aDataLength)
{
    Coap::Message *message = aSource.Get<Tmf::Agent>().AllocateAndInitPriorityConfirmablePostMessage(kUriMlr);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<Ip6AddressesTlv>(*message, static_cast<const uint8_t *>(aAddressesData), aDataLength));

    SuccessOrQuit(aSource.Get<Tmf::Agent>().SendMessageTo(*message, aDestination));
}

void TestMatnTc21(void)
{
    /**
     * 5.10.17 MATN-TC-21: Incorrect Multicast registrations by Thread Device
     *
     * 5.10.17.1 Topology
     * - BR_1 (DUT)
     * - BR_2
     * - Router
     * - Host
     *
     * 5.10.17.2 Purpose & Description
     * The purpose of this test case is to verify that a BR_1 can correctly handle incorrect or invalid multicast
     *   registrations from a Thread device, whether that is due to invalid TLV values or other reasons enumerated
     *   below.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Multicast        | 5.10.17
     */

    Core         nexus;
    Node        &br1    = nexus.CreateNode();
    Node        &br2    = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &host   = nexus.CreateNode();
    Ip6::Address ma1;
    Ip6::Address ma3;
    Ip6::Address ma5;
    Ip6::Address ma6;
    Ip6::Address mae1;
    Ip6::Address mae2;
    Ip6::Address mae3;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("Router");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));
    SuccessOrQuit(ma3.FromString(kMA3));
    SuccessOrQuit(ma5.FromString(kMA5));
    SuccessOrQuit(ma6.FromString(kMA6));
    SuccessOrQuit(mae1.FromString(kMAe1));
    SuccessOrQuit(mae2.FromString(kMAe2));
    SuccessOrQuit(mae3.FromString(kMAe3));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1, BR_2, Router
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation – BR_1, BR_2, Router");

    br1.AllowList(br2);
    br1.AllowList(router);
    br2.AllowList(br1);
    br2.AllowList(router);
    router.AllowList(br1);
    router.AllowList(br2);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    router.Join(br1, Node::kAsFtd);
    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    host.mInfraIf.Init(host);
    host.mInfraIf.AddAddress(host.mInfraIf.GetLinkLocalAddress());

    nexus.AdvanceTime(kStabilizationTime * 2);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("MA3", kMA3);
    nexus.AddTestVar("MA5", kMA5);
    nexus.AddTestVar("MA6", kMA6);
    nexus.AddTestVar("MAe1", kMAe1);
    nexus.AddTestVar("MAe2", kMAe2);
    nexus.AddTestVar("MAe3", kMAe3);

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register an invalid multicast address at BR_1. Automatically
     *   unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload
     *   contains: IPv6 Addresses TLV: MAe1
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: Router registers an invalid multicast address MAe1 at BR_1.");
    SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), &mae1, sizeof(mae1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Does not update its multicast listener table.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 2: BR_1 does not update its multicast listener table.");

    /**
     * Step 3
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with an error status
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains:
     *   - Status TLV: 2 [ST_MLR_INVALID]
     *   - IPv6 Addresses TLV: The invalid IPv6 address included in step 1
     */
    Log("Step 3: BR_1 responds to the multicast registration with an error status ST_MLR_INVALID.");

    /**
     * Step 4
     * - Device: Router
     * - Description: Harness instructs the device to register two invalid multicast addresses at BR_1.
     *   Automatically unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the
     *   payload contains: IPv6 Addresses TLV: MAe2, MAe3
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: Router registers two invalid multicast addresses MAe2, MAe3 at BR_1.");
    {
        Ip6::Address addresses[2];

        addresses[0] = mae2;
        addresses[1] = mae3;
        SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), addresses, sizeof(addresses));
    }
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 5
     * - Device: BR_1
     * - Description: Does not update its multicast listener table.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 5: BR_1 does not update its multicast listener table.");

    /**
     * Step 6
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with an error status
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains:
     *   - Status TLV: 2 [ST_MLR_INVALID]
     *   - IPv6 Addresses TLV: The two invalid IPv6 addresses included in step 4
     */
    Log("Step 6: BR_1 responds to the multicast registration with an error status ST_MLR_INVALID.");

    /**
     * Step 7
     * - Device: Router
     * - Description: Harness instructs the device to register a link local multicast address (MA6) at BR_1.
     *   Automatically unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the
     *   payload contains: IPv6 Addresses TLV: MA6
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 7: Router registers a link local multicast address MA6 at BR_1.");
    SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), &ma6, sizeof(ma6));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Does not update its multicast listener table.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 8: BR_1 does not update its multicast listener table.");

    /**
     * Step 9
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with an error.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains:
     *   - Status TLV: 2 [ST_MLR_INVALID]
     *   - IPv6 Addresses TLV: MA6
     */
    Log("Step 9: BR_1 responds to the multicast registration with an error ST_MLR_INVALID.");

    /**
     * Step 10
     * - Device: Router
     * - Description: Harness instructs the device to register a mesh-local multicast address (MA5) at BR_1.
     *   Automatically unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the
     *   payload contains: IPv6 Addresses TLV: MA5
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: Router registers a mesh-local multicast address MA5 at BR_1.");
    SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), &ma5, sizeof(ma5));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 11
     * - Device: BR_1
     * - Description: Does not update its multicast listener table.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 11: BR_1 does not update its multicast listener table.");

    /**
     * Step 12
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with an error.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains:
     *   - Status TLV: 2 [ST_MLR_INVALID]
     *   - IPv6 Addresses TLV: MA5
     */
    Log("Step 12: BR_1 responds to the multicast registration with an error ST_MLR_INVALID.");

    /**
     * Step 13
     * - Device: Router
     * - Description: Harness instructs the device to register one Link local (MA6) and one Admin-local (MA1)
     *   address at BR_1. Automatically unicasts an MLR.req CoAP request to BR_2 as follows:
     *   coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV MA6, MA1
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 13: Router registers one Link local MA6 and one Admin-local MA1 address at BR_1.");
    {
        Ip6::Address addresses[2];

        addresses[0] = ma6;
        addresses[1] = ma1;
        SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), addresses, sizeof(addresses));
    }
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 14
     * - Device: BR_1
     * - Description: Automatically updates its multicast listener table with MA1 only.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 14: BR_1 automatically updates its multicast listener table with MA1 only.");

    /**
     * Step 15
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains:
     *   - Status TLV: 2 [ST_MLR_INVALID]
     *   - IPv6 Addresses TLV: MA6 (only)
     */
    Log("Step 15: BR_1 responds to the multicast registration with an error ST_MLR_INVALID and MA6.");

    /**
     * Step 16
     * - Device: Host
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1, on the backbone link.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Host sends a ICMPv6 Echo Request to MA1 on the backbone link.");
    host.mInfraIf.SendEchoRequest(host.mInfraIf.GetLinkLocalAddress(), ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    /**
     * Step 17
     * - Device: BR_1
     * - Description: Automatically forwards the ping packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST forward the ping packet with multicast address, MA1, to its Thread Network encapsulated in
     *     an MPL packet, where:
     *   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
     */
    Log("Step 17: BR_1 automatically forwards the ping packet to its Thread Network.");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 18
     * - Device: Router
     * - Description: Harness instructs the device to register one valid multicast address at BR_1 and one
     *   invalid address (with a length that is too short). Due to the short length of the last address, the IPv6
     *   Addresses TLV is invalid i.e. message format error. Automatically unicasts an MLR.req CoAP request to
     *   BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MA3 (16
     *   bytes), MAe4 (14 bytes)
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 18: Router registers one valid address MA3 and one invalid address MAe4 (14 bytes) at BR_1.");
    {
        uint8_t data[sizeof(ma3) + 14];
        memcpy(data, &ma3, sizeof(ma3));
        memset(data + sizeof(ma3), 0xee, 14); // MAe4 as 14 bytes of 0xee
        SendMlrRequest(router, br1.Get<Mle::Mle>().GetMeshLocalRloc(), data, sizeof(data));
    }
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 19
     * - Device: BR_1
     * - Description: MAY automatically update its multicast listener table with MA3 only.
     * - Pass Criteria:
     *   - None.
     */
    Log("Step 19: BR_1 MAY automatically update its multicast listener table with MA3 only.");

    /**
     * Step 20
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with an error status
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST respond in one of the following three ways:
     *   - 1) The DUT responds with CoAP error response 4.00 or 5.00 (in this case payload is not verified by Test
     *     Harness – it will Fail and inform the user of the situation.
     *   - 2) The DUT unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains:
     *     Status TLV: 2 [ST_MLR_INVALID], IPv6 Addresses TLV: MAe4 (stored in 14 or 16 bytes) or The entire TLV
     *     as it is in the request (Step 18)
     *   - 3) The DUT unicasts an MLR.rsp CoAP response to TD as follows: 2.04 Changed Where the payload contains:
     *     Status TLV: 6 [ST_MLR_GENERAL_FAILURE]
     */
    Log("Step 20: BR_1 responds to the multicast registration with an error status.");

    /**
     * Step 21
     * - Device: Router
     * - Description: Harness instructs the device to register another multicast address at BR_2 (instead of
     *   BR_1). Automatically unicasts an MLR.req CoAP request to BR_2 as follows:
     *   coap://[<BR_2 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MA1
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 21: Router registers another multicast address MA1 at BR_2 (Secondary BBR).");
    SendMlrRequest(router, br2.Get<Mle::Mle>().GetMeshLocalRloc(), &ma1, sizeof(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 22
     * - Device: BR_2
     * - Description: Automatically responds to the multicast registration with an error.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST unicast a MLR.rsp CoAP response as below, to TD. 2.04 Changed
     *   - Where the payload contains:
     *   - Status TLV: 5 [ST_MLR_BBR_NOT_PRIMARY]
     */
    Log("Step 22: BR_2 responds to the multicast registration with an error ST_MLR_BBR_NOT_PRIMARY.");

    nexus.SaveTestInfo("test_1_2_MATN_TC_21.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc21();
    printf("All tests passed\n");
    return 0;
}
