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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 20 * 1000;

/**
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Global prefix string.
 */
static constexpr char kGuaPrefixStr[] = "910b:1234::/64";

/**
 * TREL service instance name captured from mDNS browsing.
 */
static char sTrelInstanceName[Dns::Name::kMaxNameSize];

/**
 * Callback for mDNS browsing to capture TREL service instance name.
 */
static void HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    if (aResult->mTtl > 0)
    {
        // Save the instance name if it is not from Eth_1 itself.
        if (!StringEndsWith(aResult->mServiceInstance,
                            Node::From(aInstance).Get<Mac::Mac>().GetExtAddress().ToString().AsCString()))
        {
            IgnoreError(StringCopy(sTrelInstanceName, aResult->mServiceInstance));
        }
    }
}

void Test_1_4_TREL_TC_6(void)
{
    /**
     * 8.6. [1.4] [CERT] mDNS Discovery of TREL Service
     *
     * 8.6.1. Purpose
     * This test covers mDNS discovery of TREL Service on the BR DUT, and validation of the DNS-SD parameters and fields
     *   advertised.
     *
     * 8.6.2. Topology
     * - 1. BR Leader (DUT) - Support multi-radio (TREL and 15.4)
     * - 2. Eth_1 - Reference device on the Adjacent Infrastructure Link performing mDNS discovery
     *
     * Spec Reference   | Section
     * -----------------|----------
     * TREL Discovery   | 15.6.3.1
     */

    Core nexus;

    Node &br   = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br.SetName("BR_DUT");
    eth1.SetName("Eth_1");

    eth1.mTrel.Disable();

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1
     * - Device: Eth_1, BR (DUT)
     * - Description (TREL-8.6): Form the topology. Eth_1 advertises an on-link prefix using multicast ND RA PIO, such
     *   as 910b:1234::/64. Wait for the DUT to become Leader. The DUT automatically configures an IPv6 address on the
     *   AIL based on the advertised prefix.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: Form the topology");

    eth1.mInfraIf.Init(eth1);
    Ip6::Prefix guaPrefix;
    SuccessOrQuit(guaPrefix.FromString(kGuaPrefixStr));
    eth1.mInfraIf.StartRouterAdvertisement(guaPrefix);

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br.mInfraIf.Init(br);
    br.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    SuccessOrQuit(br.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    nexus.AdvanceTime(0);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2
     * - Device: Eth_1
     * - Description (TREL-8.6): Harness instructs device to send multicast mDNS query QType=PTR for name
     *   “_trel._udp.local”. Note: for test reliability purposes, it's allowed to retransmit this query one additional
     *   time.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth_1 sends mDNS query PTR for _trel._udp.local");

    eth1.mInfraIf.Init(eth1);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    {
        Dns::Multicast::Core::Browser browser;

        ClearAllBytes(browser);
        browser.mCallback     = HandleBrowseResult;
        browser.mServiceType  = "_trel._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3
     * - Device: BR (DUT)
     * - Description (TREL-8.6): Automatically responds with mDNS response. Note: the response can be unicast or
     *   multicast.
     * - Pass Criteria:
     *   - The DUT MUST send an mDNS response (unicast and/or multicast).
     *   - Eth_1 MUST receive the mDNS response containing the TREL service, as follows, in the Answer Record section:
     *     $ORIGIN local. _trel._udp PTR <name>._trel._udp
     *   - The response MAY contain the following in the Additional Record section or in the Answer Record section:
     *     $ORIGIN local. <name>._trel._udp ( SRV <val1> <val2> <port> <hostname> TXT xa=<extaddr>;xp=<xpanid> )
     *     <hostname> AAAA <link-local address> AAAA <global address>
     *   - Where: <name> is a service instance name selected by BR DUT.
     *   - Where: <val1> is a value >= 0.
     *   - Where: <val2> is a value >= 0.
     *   - Where: <port> is a port number selected by BR DUT.
     *   - Where: <hostname> is a hostname selected by BR DUT.
     *   - Where: <global address> is an address starting with prefix 910b:1234::/64.
     *   - Where: <link-local address> is an IPv6 address starting with fe80::/10.
     *   - Where: <extaddr> is the extended address of BR DUT, binary encoded. It MUST NOT be ASCII hexadecimal encoded.
     *   - Where: <xpanid> is the extended PAN ID of the Thread Network, binary encoded. It MUST NOT be ASCII
     *     hexadecimal encoded.
     *   - Verify that <xpanid> in the TXT record equals the configured Extended PAN ID (XPANID) of the Thread Network.
     */
    Log("Step 3: BR (DUT) responds with mDNS response");

    VerifyOrQuit(sTrelInstanceName[0] != '\0');
    nexus.AddTestVar("TREL_INSTANCE_NAME", sTrelInstanceName);
    nexus.AddTestVar("BR_EXT_ADDR", br.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
    nexus.AddTestVar("XPANID", br.Get<MeshCoP::NetworkIdentity>().GetExtPanId().ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4
     * - Device: Eth_1
     * - Description (TREL-8.6): Harness instructs device to send multicast mDNS query QType=SRV for name
     *   “<name>._trel._udp.local”, the service found in the previous step. Note: <name> is taken from the previous
     *   step response for the PTR record. Note: for test reliability purposes, it's allowed to retransmit this query
     *   one additional time.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: Eth_1 sends mDNS query SRV for service instance");

    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
    nexus.AdvanceTime(1000);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = sTrelInstanceName;
        resolver.mServiceType     = "_trel._udp";
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5
     * - Device: BR (DUT)
     * - Description (TREL-8.6): Automatically responds with mDNS response. Note: the response can be unicast or
     *   multicast.
     * - Pass Criteria:
     *   - The DUT MUST send mDNS response (unicast and/or multicast)
     *   - Eth_1 MUST receive the mDNS response containing the TREL service, as follows, in the Answer Record section:
     *     $ORIGIN local. <name>._trel._udp ( SRV <val1> <val2> <port> <hostname> )
     *   - It MAY contain in the Additional Record section or in the Answer Record section the following: <hostname>
     *     AAAA <link-local address> AAAA <global address>
     *   - Also it MAY contain in the Additional Record section or the Answer Record section the following:
     *     <name>._trel._udp TXT xa=<extaddr>;xp=<xpanid>
     *   - Where: all the names in pointy brackets are as specified in step 3. For all items present, the same
     *     validation as in step 3 MUST be performed.
     */
    Log("Step 5: BR (DUT) responds with mDNS response");

    nexus.SaveTestInfo("test_1_4_TREL_TC_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_4_TREL_TC_6();
    printf("All tests passed\n");
    return 0;
}
