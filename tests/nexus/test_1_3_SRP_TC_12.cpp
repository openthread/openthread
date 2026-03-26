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
 * Time to advance for SRP server information to be updated in Network Data.
 */
static constexpr uint32_t kSrpServerInfoUpdateTime = 20 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * Time to advance for the DUT to withdraw its Unicast Dataset.
 */
static constexpr uint32_t kWithdrawTime = 35 * 1000;

/**
 * SRP Anycast sequence numbers.
 */
static constexpr uint8_t kBr1AnycastSeqNum = 100;
static constexpr uint8_t kBr2AnycastSeqNum = 255;

/**
 * SRP Version.
 */
static constexpr uint8_t kSrpVersion = 1;

/**
 * Fake SRP Server addresses and ports.
 */
static const char *const  kBr2UnicastAddr = "fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";
static constexpr uint16_t kBr2UnicastPort = 55555;
static const char *const  kBr3UnicastAddr = "fd80::1";
static constexpr uint16_t kBr3UnicastPort = 55556;

void Test_1_3_SRP_TC_12(const char *aJsonFileName)
{
    /**
     * 2.12. [1.3] [CERT] SRP Service Advertisement - Multiple BRs
     *
     * 2.12.1. Purpose
     * To test the following:
     * - 1. DNS/SRP service advertisement by all BRs in the Thread Network.
     * - 2. DUT must advertise its own DNS/SRP service initially.
     * - 3. DUT as Leader integrates all the advertised DNS/SRP services.
     *
     * 2.12.2. Topology
     * - BR 1 (DUT) - Thread Border Router, and the Leader
     * - BR 2 - Border Router
     * - BR 3 - Border Router
     *
     * Spec Reference | V1.1 Section | V1.3.0 Section
     * ---------------|--------------|---------------
     * SRP Server     | N/A          | 1.3
     */

    Core nexus;

    Node &br1 = nexus.CreateNode();
    Node &br2 = nexus.CreateNode();
    Node &br3 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    br3.SetName("BR_3");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.12): Enable; switch on.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) Enable; switch on.");
    br1.Form();
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    SuccessOrQuit(br1.Get<Srp::Server>().SetAnycastModeSequenceNumber(kBr1AnycastSeqNum));
    br1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.12): Automatically assumes the Leader role Automatically adds SRP Server
     *     information in the Thread Network Data. Automatically registers itself as a border router
     *     in the Thread Network Data.
     *   Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data. It MUST advertise at
     *     least one Dataset of the below:
     *     - 1. DNS/SRP Anycast Dataset
     *     - 2. DNS/SRP Unicast Dataset
     *   - Specifically, in case of DNS/SRP Unicast Dataset as per [ThreadSpec] 14.3.2.2: S_service_data
     *     Length MUST be 1 or 19
     *   - Specifically, in case of DNS/SRP Anycast Dataset as per [ThreadSpec] 14.3.2.1: S_service_data
     *     Length MUST be 2 S_server_data Length MUST be 0.
     */
    Log("Step 2: BR 1 (DUT) adds SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 3
     *   Device: BR 2
     *   Description (SRP-2.12): Enable.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 3: BR 2 Enable.");
    br2.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Step 4
     *   Device: BR_2
     *   Description (SRP-2.12): Harness instructs device to add its SRP Server information in the
     *     Thread Network Data with a (faked) high numerical address: DNS/SRP Unicast Dataset Service
     *     TLV S_service_data Length = 1 S service data: 0x5D Server TLV S_server_data Length = 18
     *     S_server_16: (default RLOC of itself) S server data: Address: fdffffffffffffffffff::
     *     S_server data: Port: <auto-selected by BR_2> Automatically registers itself as a border
     *     router in the Thread Network Data.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 4: BR 2 adds its SRP Server information with a (faked) high numerical address.");
    {
        Ip6::Address br2Addr;
        SuccessOrQuit(br2Addr.FromString(kBr2UnicastAddr));
        SuccessOrQuit(br2.Get<NetworkData::Service::Manager>().AddDnsSrpUnicastServiceWithAddrInServerData(
            br2Addr, kBr2UnicastPort, kSrpVersion));
        br2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 5
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.12): Automatically integrates BR_2's SRP Server information in the Thread
     *     Network Data. Note: BR 1 will not withdraw its own Unicast Dataset if it advertised that
     *     type, because that has a numerically lower IPv6 address w.r.t. the BR 2 Dataset.
     *   Pass Criteria:
     *   - At least 2 Datasets MUST be present in in the Thread Network Data:
     *     - 1. BR 1 (Anycast and/or Unicast) Dataset as in Step 2.
     *     - 2. BR 2 Unicast Dataset as defined in Step 4.
     */
    Log("Step 5: BR 1 (DUT) integrates BR 2's SRP Server information.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 6
     *   Device: BR 3
     *   Description (SRP-2.12): Enable.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 6: BR 3 Enable.");
    br3.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Step 7
     *   Device: BR 3
     *   Description (SRP-2.12): Harness instructs device to add its SRP Server information in the
     *     Thread Network Data with a (faked) low numerical address: DNS/SRP Unicast Dataset Service
     *     TLV S_service_data Length = 19 S_service data: 0x5D S_service_data: Address: fd80:::1
     *     S_service_data: Port: <auto-selected by BR 3> Server TLV: S_server 16: (default own RLOC
     *     address) Automatically registers itself as a border router in the Thread Network Data.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 7: BR 3 adds its SRP Server information with a (faked) low numerical address.");
    {
        Ip6::Address br3Addr;
        SuccessOrQuit(br3Addr.FromString(kBr3UnicastAddr));
        SuccessOrQuit(br3.Get<NetworkData::Service::Manager>().AddDnsSrpUnicastServiceWithAddrInServiceData(
            br3Addr, kBr3UnicastPort, kSrpVersion));
        br3.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 8
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.12): Automatically integrates BR 3's SRP Server information in the Thread
     *     Network Data and withdraws its own Unicast Dataset if it had any. Note: for a Thread 1.3
     *     DUT, the reason to withdraw is that there are more than NETDATA NUM_UNICAST_DNS SRP DATASET
     *     ENTRIES (2) Unicast Datasets in the network data, so above target. Note: for a Thread 1.4
     *     DUT, the reason to withdraw is that there are is a Unicast Dataset of a more preferred
     *     type (the one added by BR_3) than its own automatically added type.
     *   Pass Criteria:
     *   - The DUT now includes 3 or 4 Datasets in its Thread Network Data. (2 unicast, 1 Anycast,
     *     optionally 1 Anycast of its own)
     *   - If the DUT advertised a Unicast Dataset, it MUST withdraw it. So after some time (up to 32
     *     seconds) the DUT MUST NOT advertise any Unicast Dataset in the Thread Network Data that
     *     Includes the DUT's RLOC in a Server TLV. Includes the DUT's ML-EID in a Service TLV as
     *     address.
     *   - Thread Network Data MUST contain Unicast Datasets of BR_2 and BR_3 as specified above in
     *     step 4 and 7.
     */
    Log("Step 8: BR 1 (DUT) integrates BR 3's SRP Server information and withdraws its own Unicast Dataset.");
    nexus.AdvanceTime(kWithdrawTime);

    /**
     * Step 9
     *   Device: BR 2
     *   Description (SRP-2.12): Harness instructs device to register an additional Dataset with the
     *     Leader, as follows: DNS/SRP Anycast Dataset S_service_data Length = 2 Service TLV S service
     *     data: 0x5C S_service data: SRP-SN: 255 Server TLV S_server_16: (default RLOC of itself)
     *     Note: an OT-BR reference device can only be configured with one DNS/SRP dataset when using
     *     the regular CLI commands. To force the above addition, the commands: service add 44978 5cff
     *     netdata register may be used. Note that the first command requires an OT CLI update
     *     provided by PR #10377. If this PR is not merged yet into the reference device code, as a
     *     workaround the argument 00' can be added temporarily.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 9: BR 2 adds an additional Anycast Dataset.");
    {
        SuccessOrQuit(br2.Get<NetworkData::Service::Manager>().AddDnsSrpAnycastService(kBr2AnycastSeqNum, kSrpVersion));
        br2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 10
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.12): Automatically integrates BR_2's new Anycast Dataset and MAY withdraw
     *     its own Anycast Dataset if it had any. Whether it withdraws or not depends on its
     *     advertised SRP-SN.
     *   Pass Criteria:
     *   - Thread Network Data MUST contain Unicast Datasets of BR_2 and BR 3 as specified above in
     *     step 8.
     *   - Thread Network Data MUST NOT contain Unicast Dataset of BR 1.
     *   - Thread Network Data MUST contain Anycast Dataset of BR_2 defined in step 9
     *   - In case the DUT advertised Anycast Dataset in step 2, the Test Harness now identifies the
     *     SRP-SN (final byte of service data) that the DUT advertised in step 2, and: If SRP-SN <=
     *     128, then DUT MUST withdraw its Anycast Dataset. If SRP-SN >= 129, then DUT MUST NOT
     *     withdraw its Anycast Dataset.
     */
    Log("Step 10: BR 1 (DUT) integrates BR 2's new Anycast Dataset.");
    nexus.AdvanceTime(kWithdrawTime);

    Log("All steps completed.");

    {
        String<10> string;

        string.Clear();
        string.Append("%u", kBr2UnicastPort);
        nexus.AddTestVar("BR_2_UNICAST_PORT", string.AsCString());

        string.Clear();
        string.Append("%u", kBr3UnicastPort);
        nexus.AddTestVar("BR_3_UNICAST_PORT", string.AsCString());

        string.Clear();
        string.Append("%u", kBr1AnycastSeqNum);
        nexus.AddTestVar("BR_1_ANYCAST_SEQ", string.AsCString());

        string.Clear();
        string.Append("%u", kBr2AnycastSeqNum);
        nexus.AddTestVar("BR_2_ANYCAST_SEQ", string.AsCString());
    }

    nexus.AddTestVar("BR_2_UNICAST_ADDR", kBr2UnicastAddr);
    nexus.AddTestVar("BR_3_UNICAST_ADDR", kBr3UnicastAddr);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_12((argc > 2) ? argv[2] : "test_1_3_SRP_TC_12.json");

    printf("All tests passed\n");
    return 0;
}
