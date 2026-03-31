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
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 20 * 1000;

/**
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 20 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Active Timestamp components.
 */
static constexpr uint64_t kActiveTimestampSeconds = 0x666af1;
static constexpr uint16_t kActiveTimestampTicks   = 0x1880;

/**
 * Service name parameters.
 */
static const char kMeshCoPServiceType[] = "_meshcop._udp";

void Test_1_3_GEN_TC_2(const char *aJsonFileName)
{
    /**
     * 9.2. [1.3] [CERT] Border Router mDNS discovery of v1.3/v1.4 TXT records
     *
     * 9.2.1. Purpose
     *   - This test verifies that the Border Router DUT implements TXT records that were added in the v1.3 / v1.4
     *     specifications: “omr”, “xa”, and “id”. It also verifies changed rules in the v1.4.0 specification on use of
     *     vendor-specific records. It also validates, to the extent possible, that BR vendors provide correct values
     *     for TXT record keys.
     *
     * 9.2.2. Topology
     *   - BR_1: Thread Border Router DUT and Leader
     *   - Eth_1: Reference device
     *
     * Spec Reference | V1.3.0 Section
     * ---------------|---------------
     * Thread 1.3.0   | 9.2
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    eth1.SetName("Eth_1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    br1.mSettings.Wipe();
    eth1.mSettings.Wipe();
    br1.Reset();
    eth1.Reset();

    for (uint8_t i = 0; i < 2; i++)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Iteration %u", i);

        /**
         * Step 0
         *   - Device: BR_1 (DUT) / Harness
         *   - Description (GEN-9.2): Harness configures DUT with Active Operational Dataset, either via THCI or via
         *     the vendor specified setup method for the BR. This includes the following: Network Name: "Thread Cert
         *     9.2", Active Timestamp (uint64): 0x000000666AF13100. Note: this timestamp can be set using the OT CLI
         *     command "dataset activetimestamp 1718284593"
         */
        Log("Step 0: Harness configures DUT with Active Operational Dataset.");
        {
            MeshCoP::Dataset::Info datasetInfo;
            SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().CreateNewNetwork(datasetInfo));

            datasetInfo.mActiveTimestamp.mSeconds             = kActiveTimestampSeconds;
            datasetInfo.mActiveTimestamp.mTicks               = kActiveTimestampTicks;
            datasetInfo.mActiveTimestamp.mAuthoritative       = false;
            datasetInfo.mComponents.mIsActiveTimestampPresent = true;

            {
                MeshCoP::NetworkName networkName;
                SuccessOrQuit(networkName.Set("Thread Cert 9.2"));
                datasetInfo.Set<MeshCoP::Dataset::kNetworkName>(networkName);
            }

            br1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

            SuccessOrQuit(br1.Get<MeshCoP::BorderAgent::Manager>().SetServiceBaseName("OpenThread "));

            {
                uint8_t vendorTxtData[] = {
                    14,  'v', 'n', '=', 'N', 'e', 'x', 'u', 's', 'V', 'e', 'n', 'd', 'o', 'r', 13,   'm',  'n',
                    '=', 'N', 'e', 'x', 'u', 's', 'M', 'o', 'd', 'e', 'l', 6,   'v', 'o', '=', 0x00, 0x00, 0x01,
                };
                br1.Get<MeshCoP::BorderAgent::TxtData>().SetVendorData(vendorTxtData, sizeof(vendorTxtData));
            }

            br1.Get<MeshCoP::BorderAgent::Manager>().SetEnabled(true);
        }

        /**
         * Step 1
         *   - Device: BR_1 (DUT)
         *   - Description (GEN-9.2): Form topology. DUT becomes Leader of its Thread Network. It configures an OMR ULA
         *     prefix OMR_1 and sends the prefix OMR_1 in ND RA multicast messages on the AIL. Harness records the
         *     value of OMR_1 for validation in future test steps.
         */
        Log("Step 1: Form topology. DUT becomes Leader. Configures OMR prefix.");
        br1.Get<ThreadNetif>().Up();
        SuccessOrQuit(br1.Get<Mle::Mle>().Start());

        br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
        br1.Get<BorderRouter::RoutingManager>().Init();
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
        nexus.AdvanceTime(kFormNetworkTime);

        // Wait for BR to advertise prefixes and for verification script to catch RAs
        nexus.AdvanceTime(kBrActionTime);

        VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        /**
         * Step 2
         *   - Device: Eth_1
         *   - Description (GEN-9.2): Harness instructs the device to send via mDNS a DNS-SD QType=PTR query for
         *     services “_meshcop._udp.local”.
         */
        Log("Step 2: Eth_1 sends mDNS PTR query for _meshcop._udp.local.");
        Dns::Multicast::Core::Browser browser;
        browser.mCallback     = [](otInstance *, const Dns::Multicast::Core::BrowseResult *) {};
        browser.mServiceType  = kMeshCoPServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));

        /**
         * Step 3
         *   - Device: BR_1 (DUT)
         *   - Description (GEN-9.2): Automatically responds with mDNS response with PTR records, and optionally
         *     Additional Records (SRV/TXT).
         */
        Log("Step 3: BR_1 (DUT) responds with mDNS response with PTR records.");

        /**
         * Step 4
         *   - Device: Eth_1
         *   - Description (GEN-9.2): Harness instructs the device to send via mDNS a query QType=TXT, with the DNS
         *     name set to the service instance name that is included in the PTR record in step 3.
         */
        Log("Step 4: Eth_1 sends mDNS TXT query for the service instance.");
        {
            Dns::Multicast::Core::TxtResolver resolver;
            char                              serviceInstance[64];

            snprintf(serviceInstance, sizeof(serviceInstance), "OpenThread%s",
                     br1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());

            Log("Querying TXT for instance: %s", serviceInstance);

            resolver.mCallback = [](otInstance *, const Dns::Multicast::Core::TxtResult *aResult) {
                Log("TXT Callback: instance=%s", aResult->mServiceInstance);
            };
            resolver.mServiceType     = kMeshCoPServiceType;
            resolver.mInfraIfIndex    = kInfraIfIndex;
            resolver.mServiceInstance = serviceInstance;

            SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartTxtResolver(resolver));
            nexus.AdvanceTime(kDnsQueryTime);
            SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopTxtResolver(resolver));
        }

        /**
         * Step 5
         *   - Device: BR_1 (DUT)
         *   - Description (GEN-9.2): Automatically responds with mDNS response with TXT record.
         */
        Log("Step 5: BR_1 (DUT) responds with mDNS response with TXT record.");

        /**
         * Step 6
         *   - Device: BR_1 (DUT) / Harness
         *   - Description (GEN-9.2): Verify that the DUT responded with the right data in the TXT record keys. This
         *     uses the TXT record found in step 5.
         */
        Log("Step 6: Verify TXT record keys.");

        {
            char varName[64];
            snprintf(varName, sizeof(varName), "BR_1_EXT_ADDR_%u", i);
            nexus.AddTestVar(varName, br1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());

            Ip6::Prefix                   omrPrefix;
            BorderRouter::RoutePreference preference;
            if (br1.Get<BorderRouter::RoutingManager>().GetFavoredOmrPrefix(omrPrefix, preference) == kErrorNone)
            {
                snprintf(varName, sizeof(varName), "BR_1_OMR_PREFIX_%u", i);
                nexus.AddTestVar(varName, omrPrefix.ToString().AsCString());
                Log("Iteration %u OMR Prefix: %s", i, omrPrefix.ToString().AsCString());
            }
        }

        if (i == 0)
        {
            /**
             * Step 7
             *   - Device: BR_1 (DUT)
             *   - Description (GEN-9.2): Factory reset the device.
             */
            Log("Step 7: Factory reset the device.");
            SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(false));
            br1.mSettings.Wipe();
            br1.Reset();

            SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));

            // Wait for reset and restart
            nexus.AdvanceTime(20 * 1000);
        }
        else
        {
            /**
             * Step 8
             *   - Device: BR_1 (DUT)
             *   - Description (GEN-9.2): Repeat steps 0 to 6 again.
             */
            Log("Step 8: Repeat steps 0 to 6 again.");
        }
    }

    Log("All steps completed.");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_GEN_TC_2((argc > 2) ? argv[2] : "test_1_3_gen_tc_2.json");
    printf("All tests passed\n");
    return 0;
}
