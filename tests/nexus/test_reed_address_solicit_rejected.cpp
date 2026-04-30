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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

void TestReedAddressSolicitRejected(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();

    leader.SetName("Leader");
    reed.SetName("REED");

    nexus.AdvanceTime(0);

    // Leader sets RouterUpgradeThreshold to 1 to reject further routers
    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);

    Log("Step 1: Form network and join REED");
    AllowLinkBetween(leader, reed);
    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    reed.Join(leader);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    Log("Step 2: REED adds a service and registers it");
    {
        uint32_t                 enterpriseNumber = 123;
        NetworkData::ServiceData serviceData;
        NetworkData::ServerData  serverData;
        uint8_t                  serviceDataBytes[] = {0x12, 0x34};
        uint8_t                  serverDataBytes[]  = {0xab, 0xcd};

        serviceData.Init(serviceDataBytes, sizeof(serviceDataBytes));
        serverData.Init(serverDataBytes, sizeof(serverDataBytes));

        SuccessOrQuit(reed.Get<NetworkData::Local>().AddService(enterpriseNumber, serviceData, true, serverData));
        reed.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    // REED sends a Server Data Notification to the leader, the leader
    // updates its network data and broadcasts the update to the
    // network. We advance time enough to ensure all these steps are
    // completed.
    nexus.AdvanceTime(15 * 1000);

    Log("Step 3: Verify REED has the Service ALOC");
    {
        bool hasAloc = false;
        for (const Ip6::Netif::UnicastAddress &addr : reed.Get<ThreadNetif>().GetUnicastAddresses())
        {
            if (addr.GetAddress().GetIid().IsAnycastServiceLocator() &&
                (addr.GetAddress().GetIid().GetLocator() == 0xfc10))
            {
                hasAloc = true;
                break;
            }
        }
        VerifyOrQuit(hasAloc, "REED should have the Service ALOC");
    }

    Log("Step 4: Force REED to try to become a router");
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(16);
    VerifyOrQuit(reed.Get<Mle::Mle>().SetRouterSelectionJitter(120) == kErrorNone);

    // We advance time enough to cover jitter and router upgrade attempt.
    nexus.AdvanceTime(130 * 1000);

    Log("Step 5: Verify REED remains a child and still has the Service ALOC");
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild(), "REED should remain a child");

    {
        bool hasAloc = false;
        for (const Ip6::Netif::UnicastAddress &addr : reed.Get<ThreadNetif>().GetUnicastAddresses())
        {
            if (addr.GetAddress().GetIid().IsAnycastServiceLocator() &&
                (addr.GetAddress().GetIid().GetLocator() == 0xfc10))
            {
                hasAloc = true;
                break;
            }
        }
        VerifyOrQuit(hasAloc, "REED should still have the Service ALOC");
    }

    nexus.SaveTestInfo("test_reed_address_solicit_rejected.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestReedAddressSolicitRejected();
    printf("All tests passed\n");
    return 0;
}
