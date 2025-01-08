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

void TestBorderAgent(void)
{
    Core           nexus;
    Node          &node0 = nexus.CreateNode();
    Node          &node1 = nexus.CreateNode();
    Ip6::SockAddr  sockAddr;
    Pskc           pskc;
    Coap::Message *message;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgent");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - node0 leader acting as Border Agent,
    // - node1 staying disconnected (acting as candidate)

   // node0.GetInstance().SetLogLevel(kLogLevelInfo);

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent initial state");

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);

    SuccessOrQuit(node0.Get<Ip6::Filter>().AddUnsecurePort(node0.Get<MeshCoP::BorderAgent>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection to Border Agent");

    sockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(node0.Get<MeshCoP::BorderAgent>().GetUdpPort());

    node0.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disconnect from candidate side");

    node1.Get<Tmf::SecureAgent>().Close();

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a secure connection again");

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Commissioner Petition` TMF command to become full commissioner");

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateAccepted);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Commissioner Keep Alive` and check timeout behavior");

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateAccepted);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerKeepAlive);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    Log("  Wait for 49 seconds (TIMEOUT_LEAD_PET is 50 seconds) and check BA state");

    nexus.AdvanceTime(49 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateAccepted);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    Log("   Wait for additional 5 seconds (ensuring TIMEOUT_LEAD_PET and session disconnect guard time expires)");

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);
    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a secure session again and petition to become commissioner");

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateAccepted);
}

static bool sEphemeralKeyCallbackCalled = false;

void HandleEphemeralKeyChange(void *aContext)
{
    Node *node;

    VerifyOrQuit(aContext != nullptr);
    node = reinterpret_cast<Node *>(aContext);

    Log("  EphemeralKeyCallback() active:%u connected:%u", node->Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive(),
        node->Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);
    sEphemeralKeyCallbackCalled = true;
}

void TestBorderAgentEphemeralKey(void)
{
    static const char         kEphemeralKey[]   = "nexus1234";
    static constexpr uint16_t kEphemeralKeySize = sizeof(kEphemeralKey) - 1;
    static constexpr uint16_t kUdpPort          = 49155;

    Core           nexus;
    Node          &node0 = nexus.CreateNode();
    Node          &node1 = nexus.CreateNode();
    Ip6::SockAddr  sockAddr;
    Coap::Message *message;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentEphemeralKey");

    nexus.AdvanceTime(0);

    //node0.GetInstance().SetLogLevel(kLogLevelInfo);

    // Form the topology:
    // - node0 leader acting as Border Agent,
    // - node1 staying disconnected (acting as candidate)

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent ephemeral key initial state");

    sEphemeralKeyCallbackCalled = false;
    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(!sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set and start ephemeral key on Border Agent");

    node0.Get<MeshCoP::BorderAgent>().SetEphemeralKeyCallback(HandleEphemeralKeyChange, &node0);

    SuccessOrQuit(node0.Get<MeshCoP::BorderAgent>().SetEphemeralKey(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(node0.Get<Ip6::Filter>().AddUnsecurePort(kUdpPort));

    nexus.AdvanceTime(0);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a secure connection with BA using the ephemeral key");

    sEphemeralKeyCallbackCalled = false;
    sockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(kUdpPort);

    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disconnect from candidate side, check that ephemeral key use is stopped");

    sEphemeralKeyCallbackCalled = false;

    node1.Get<Tmf::SecureAgent>().Close();

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);

    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start using ephemeral key again with short timeout (20 seconds) and establish a connection");

    SuccessOrQuit(
        node0.Get<MeshCoP::BorderAgent>().SetEphemeralKey(kEphemeralKey, 20 * Time::kOneSecondInMsec, kUdpPort));

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateConnected);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());

    Log("  Check the ephemeral key timeout behavior");

    sEphemeralKeyCallbackCalled = false;
    nexus.AdvanceTime(25 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start using ephemeral key without any candidate connecting, check timeout");

    SuccessOrQuit(node0.Get<MeshCoP::BorderAgent>().SetEphemeralKey(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetUdpPort() == kUdpPort);

    Log("  Wait more than 120 seconds (default ephemeral key timeout)");
    sEphemeralKeyCallbackCalled = false;
    nexus.AdvanceTime(122 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that ephemeral key use is stopped after max failed connection attempts");

    SuccessOrQuit(node0.Get<MeshCoP::BorderAgent>().SetEphemeralKey(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() == MeshCoP::BorderAgent::kStateStarted);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize - 2));

    for (uint8_t numAttempts = 1; numAttempts < 10; numAttempts++)
    {
        Log("  Attempt %u to connect with the wrong key", numAttempts);

        SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
        VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().GetState() != MeshCoP::BorderAgent::kStateConnected);
        VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    }

    Log("  Attempt 10 (final attempt) to connect with the wrong key, check that ephemeral key use is stopped");

    sEphemeralKeyCallbackCalled = false;
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive());
    VerifyOrQuit(sEphemeralKeyCallbackCalled);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAgent();
    ot::Nexus::TestBorderAgentEphemeralKey();
    printf("All tests passed\n");
    return 0;
}
