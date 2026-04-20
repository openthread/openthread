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
 * Verifies SRP client auto-start functionality.
 *
 * This test replicates the logic from `tests/scripts/thread-cert/test_srp_auto_start_mode.py`.
 *
 * Topology:
 *   Four routers, one acting as SRP client, others as SRP server.
 *
 */
void TestSrpAutoStart(void)
{
    Core  nexus;
    Node &client  = nexus.CreateNode();
    Node &server1 = nexus.CreateNode();
    Node &server2 = nexus.CreateNode();
    Node &server3 = nexus.CreateNode();

    client.SetName("client");
    server1.SetName("server1");
    server2.SetName("server2");
    server3.SetName("server3");

    Log("Forming network");
    client.Get<Srp::Server>().SetEnabled(false);
    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    client.Form();
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Mle::Mle>().IsLeader());

    server1.Join(client);
    server2.Join(client);
    server3.Join(client);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(server1.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(server2.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(server3.Get<Mle::Mle>().IsFullThreadDevice());

    Ip6::Address server1Mleid = server1.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address server2Mleid = server2.Get<Mle::Mle>().GetMeshLocalEid();
    uint16_t     anycastPort  = 53;

    // Enable server1 with unicast address mode
    Log("Enable server1 with unicast address mode");
    SuccessOrQuit(server1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

    // Check auto start mode on client and check that server1 is selected
    Log("Check auto start mode on client and check that server1 is selected");
    VerifyOrQuit(client.Get<Srp::Client>().IsAutoStartModeEnabled());
    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server1Mleid);

    // Disable server1 and check client is stopped/disabled.
    Log("Disable server1 and check client is stopped/disabled.");
    server1.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(!client.Get<Srp::Client>().IsRunning());

    // Enable server2 with unicast address mode and check client starts again.
    Log("Enable server2 with unicast address mode and check client starts again.");
    SuccessOrQuit(server2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server2.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server2Mleid);

    // Enable server1 and check that client stays with server2
    Log("Enable server1 and check that client stays with server2");
    server1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server2Mleid);

    // Disable server2 and check client switches to server1.
    Log("Disable server2 and check client switches to server1.");
    server2.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server1Mleid);

    // Enable server2 with anycast mode seq-num 1, and check that client switched to it.
    Log("Enable server2 with anycast mode seq-num 1, and check that client switched to it.");
    SuccessOrQuit(server2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeAnycast));
    SuccessOrQuit(server2.Get<Srp::Server>().SetAnycastModeSequenceNumber(1));
    server2.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

    VerifyOrQuit(server2.Get<Srp::Server>().GetAnycastModeSequenceNumber() == 1);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Enable server3 with anycast mode seq-num 2, and check that client switched to it since seq number is higher.
    Log("Enable server3 with anycast mode seq-num 2, and check that client switched to it since seq number is higher.");
    SuccessOrQuit(server3.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeAnycast));
    SuccessOrQuit(server3.Get<Srp::Server>().SetAnycastModeSequenceNumber(2));
    server3.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

    VerifyOrQuit(server3.Get<Srp::Server>().GetAnycastModeSequenceNumber() == 2);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Disable server3 and check that client goes back to server2.
    Log("Disable server3 and check that client goes back to server2.");
    server3.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Enable server3 with anycast mode seq-num 0 (which is smaller than server2 seq-num 1) and check that client stays
    // with server2.
    Log("Enable server3 with anycast mode seq-num 0 and check that client stays with server2.");
    SuccessOrQuit(server3.Get<Srp::Server>().SetAnycastModeSequenceNumber(0));
    server3.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(server3.Get<Srp::Server>().GetAnycastModeSequenceNumber() == 0);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Disable server2 and check that client goes back to server3.
    Log("Disable server2 and check that client goes back to server3.");
    server2.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Disable server3 and check that client goes back to server1 with unicast address.
    Log("Disable server3 and check that client goes back to server1 with unicast address.");
    server3.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server1Mleid);

    // Enable server2 with anycast mode seq-num 5, and check that client switched to it.
    Log("Enable server2 with anycast mode seq-num 5, and check that client switched to it.");
    SuccessOrQuit(server2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeAnycast));
    SuccessOrQuit(server2.Get<Srp::Server>().SetAnycastModeSequenceNumber(5));
    server2.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(server2.Get<Srp::Server>().GetAnycastModeSequenceNumber() == 5);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Publish an entry on server3 with specific unicast address
    // This entry should be now preferred over anycast of server2.
    Log("Publish an entry on server3 with specific unicast address");
    Ip6::Address unicastAddr3;
    uint16_t     unicastPort3 = 1234;
    SuccessOrQuit(unicastAddr3.FromString("fd00:0:0:0:0:3333:beef:cafe"));
    server3.Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(unicastAddr3, unicastPort3, 1);
    nexus.AdvanceTime(65 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == unicastAddr3);
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == unicastPort3);

    // Publish an entry on server1 with specific unicast address
    // Client should still stay with server3 which it originally selected.
    Log("Publish an entry on server1 with specific unicast address");
    Ip6::Address unicastAddr1;
    uint16_t     unicastPort1 = 10203;
    SuccessOrQuit(unicastAddr1.FromString("fd00:0:0:0:0:2222:beef:cafe"));
    server1.Get<Srp::Server>().SetEnabled(false);
    server1.Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(unicastAddr1, unicastPort1, 1);
    nexus.AdvanceTime(65 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == unicastAddr3);
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == unicastPort3);

    // Unpublish the entry on server3. Now client should switch to entry from server1.
    Log("Unpublish the entry on server3.");
    server3.Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    nexus.AdvanceTime(65 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == unicastAddr1);
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == unicastPort1);

    // Unpublish the entry on server1 and check client goes back to anycast entry from server2.
    Log("Unpublish the entry on server1.");
    server1.Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    nexus.AdvanceTime(65 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress().GetIid().IsAnycastServiceLocator());
    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == anycastPort);

    // Finally disable server2, and check that client is disabled.
    Log("Finally disable server2, and check that client is disabled.");
    server2.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(!client.Get<Srp::Client>().IsRunning());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpAutoStart();
    printf("All tests passed\n");
    return 0;
}
