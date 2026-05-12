/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

void TestBorderAgentTracker(void)
{
    static constexpr uint32_t kInfraIfIndex = 1;

    Core                                     nexus;
    Node                                    &node0 = nexus.CreateNode();
    Node                                    &node1 = nexus.CreateNode();
    Node                                    &node2 = nexus.CreateNode();
    Node                                    &node3 = nexus.CreateNode();
    MeshCoP::BorderAgent::Tracker::Iterator  iterator;
    MeshCoP::BorderAgent::Tracker::AgentInfo agent;
    Dns::Multicast::Core::Service            service;
    uint16_t                                 count;
    bool                                     found;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentTracker");

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(node3.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    node0.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    node1.Join(node0);
    node2.Join(node0);
    node3.Form();
    nexus.AdvanceTime(600 * 1000);

    VerifyOrQuit(node1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(node2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(node3.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent Tracker's initial state");

    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent::Tracker>().IsRunning());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Agent Tracker");

    node0.Get<MeshCoP::BorderAgent::Tracker>().SetEnabled(true, MeshCoP::BorderAgent::Tracker::kRequesterUser);
    nexus.AdvanceTime(10);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent::Tracker>().IsRunning());

    nexus.AdvanceTime(5000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the tracked agents");

    iterator.Init(node0.GetInstance());
    count = 0;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);
    }

    VerifyOrQuit(count == 4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable BA function on node0, ensure that it is removed from the `BorderAgentTracker` list");

    node0.Get<MeshCoP::BorderAgent::Manager>().SetEnabled(false);
    nexus.AdvanceTime(5000);

    iterator.Init(node0.GetInstance());
    count = 0;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);
    }

    VerifyOrQuit(count == 3);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-enable BA function on node0, ensure that it is added again in the `BorderAgentTracker` list");

    node0.Get<MeshCoP::BorderAgent::Manager>().SetEnabled(true);
    nexus.AdvanceTime(5000);

    iterator.Init(node0.GetInstance());
    count = 0;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);
    }

    VerifyOrQuit(count == 4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable Border Agent Tracker");

    node0.Get<MeshCoP::BorderAgent::Tracker>().SetEnabled(false, MeshCoP::BorderAgent::Tracker::kRequesterUser);
    nexus.AdvanceTime(10);

    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent::Tracker>().IsRunning());

    iterator.Init(node0.GetInstance());
    VerifyOrQuit(iterator.GetNextAgentInfo(agent) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-enable BA tracker and ensure all agents are discovered again");

    node0.Get<MeshCoP::BorderAgent::Tracker>().SetEnabled(true, MeshCoP::BorderAgent::Tracker::kRequesterUser);
    nexus.AdvanceTime(10);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent::Tracker>().IsRunning());

    nexus.AdvanceTime(5000);

    iterator.Init(node0.GetInstance());
    count = 0;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);
    }

    VerifyOrQuit(count == 4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Manually register a second `_meshcop._udp` service on node3");

    ClearAllBytes(service);
    service.mServiceInstance = "extra";
    service.mServiceType     = "_meshcop._udp";
    service.mPort            = 1234;

    SuccessOrQuit(node3.Get<Dns::Multicast::Core>().RegisterService(service, /* aRequestId */ 0, nullptr));

    nexus.AdvanceTime(5 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that both agent services from node3 are discovered and tracked correctly");

    iterator.Init(node0.GetInstance());
    count = 0;
    found = false;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);

        if (StringMatch(agent.mServiceName, service.mServiceInstance, kStringCaseInsensitiveMatch))
        {
            VerifyOrQuit(agent.mPort == service.mPort);
            found = true;
        }
    }

    VerifyOrQuit(count == 5);
    VerifyOrQuit(found);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Unregister the manually added second `_meshcop._udp` service");

    SuccessOrQuit(node3.Get<Dns::Multicast::Core>().UnregisterService(service));

    nexus.AdvanceTime(5 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that tracked agents is updated");

    iterator.Init(node0.GetInstance());
    count = 0;
    found = false;

    while (iterator.GetNextAgentInfo(agent) == kErrorNone)
    {
        count++;
        Log("- %u) \"%s\", host:\"%s\", port:%u", count, agent.mServiceName, agent.mHostName, agent.mPort);

        VerifyOrQuit(agent.mHostName != nullptr);
        VerifyOrQuit(agent.mPort != 0);
        VerifyOrQuit(agent.mTxtData != nullptr);
        VerifyOrQuit(agent.mAddresses != nullptr);

        if (StringMatch(agent.mServiceName, service.mServiceInstance, kStringCaseInsensitiveMatch))
        {
            found = true;
        }
    }

    VerifyOrQuit(count == 4);
    VerifyOrQuit(!found);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAgentTracker();
    printf("All tests passed\n");
    return 0;
}
