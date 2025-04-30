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

using ActiveDatasetManager = MeshCoP::ActiveDatasetManager;
using BorderAgent          = MeshCoP::BorderAgent;
using EphemeralKeyManager  = ot::MeshCoP::BorderAgent::EphemeralKeyManager;
using ExtendedPanIdManager = MeshCoP::ExtendedPanIdManager;
using NameData             = MeshCoP::NameData;
using NetworkNameManager   = MeshCoP::NetworkNameManager;
using TxtEntry             = Dns::TxtEntry;

void TestBorderAgent(void)
{
    Core                         nexus;
    Node                        &node0 = nexus.CreateNode();
    Node                        &node1 = nexus.CreateNode();
    Node                        &node2 = nexus.CreateNode();
    Node                        &node3 = nexus.CreateNode();
    Ip6::SockAddr                sockAddr;
    Pskc                         pskc;
    Coap::Message               *message;
    BorderAgent::SessionIterator iter;
    BorderAgent::SessionInfo     sessionInfo;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgent");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - node0 leader acting as Border Agent,
    // - node1-3 stay disconnected (acting as candidate)

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    SuccessOrQuit(node2.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node2.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node2.Get<ThreadNetif>().Up();

    SuccessOrQuit(node3.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node3.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node3.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent initial state");

    VerifyOrQuit(node0.Get<BorderAgent>().IsEnabled());
    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check disabling and re-enabling of Border Agent");

    node0.Get<BorderAgent>().SetEnabled(false);
    VerifyOrQuit(!node0.Get<BorderAgent>().IsEnabled());
    VerifyOrQuit(!node0.Get<BorderAgent>().IsRunning());

    node0.Get<BorderAgent>().SetEnabled(true);
    VerifyOrQuit(node0.Get<BorderAgent>().IsEnabled());
    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());

    SuccessOrQuit(node0.Get<Ip6::Filter>().AddUnsecurePort(node0.Get<BorderAgent>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection to Border Agent");

    sockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(node0.Get<BorderAgent>().GetUdpPort());

    node0.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcSecureSessionSuccesses == 1);

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disconnect from candidate side");

    node1.Get<Tmf::SecureAgent>().Close();

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());

    iter.Init(node0.GetInstance());
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a secure connection again");

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcSecureSessionSuccesses == 2);

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Commissioner Petition` TMF command to become full commissioner");

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcSecureSessionSuccesses == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcCommissionerPetitions == 1);

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(sessionInfo.mIsCommissioner);
    VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Commissioner Keep Alive` and check timeout behavior");

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(sessionInfo.mIsCommissioner);
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerKeepAlive);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    Log("  Wait for 49 seconds (TIMEOUT_LEAD_PET is 50 seconds) and check BA state");

    nexus.AdvanceTime(49 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    Log("   Wait for additional 5 seconds (ensuring TIMEOUT_LEAD_PET and session disconnect guard time expires)");

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());
    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a secure session again and petition to become commissioner");

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcSecureSessionSuccesses == 3);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcCommissionerPetitions == 2);

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(sessionInfo.mIsCommissioner);
    VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish two more secure sessions while the first session is still active");

    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));
    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().Connect(sockAddr));

    SuccessOrQuit(node3.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));
    SuccessOrQuit(node3.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node3.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node3.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcSecureSessionSuccesses == 5);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mPskcCommissionerPetitions == 2);

    iter.Init(node0.GetInstance());

    for (uint8_t numSessions = 0; numSessions < 3; numSessions++)
    {
        SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
        VerifyOrQuit(sessionInfo.mIsConnected);

        if (node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)))
        {
            VerifyOrQuit(sessionInfo.mIsCommissioner);
        }
        else
        {
            VerifyOrQuit(node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)) ||
                         node3.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
            VerifyOrQuit(!sessionInfo.mIsCommissioner);
        }
    }

    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disconnect from candidate side on node 1");

    node1.Get<Tmf::SecureAgent>().Close();

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());

    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node3.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());

    for (uint8_t numSessions = 0; numSessions < 2; numSessions++)
    {
        SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
        VerifyOrQuit(sessionInfo.mIsConnected);
        VerifyOrQuit(!sessionInfo.mIsCommissioner);
        VerifyOrQuit(node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)) ||
                     node3.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    }

    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Connect again from node 1 after 25 seconds");

    nexus.AdvanceTime(25 * Time::kOneSecondInMsec);

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node3.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());

    for (uint8_t numSessions = 0; numSessions < 3; numSessions++)
    {
        SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
        VerifyOrQuit(sessionInfo.mIsConnected);
        VerifyOrQuit(!sessionInfo.mIsCommissioner);
        VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)) ||
                     node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)) ||
                     node3.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    }

    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait for the first two sessions to timeout, check that only the recent node1 session is connected");

    nexus.AdvanceTime(28 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node2.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node3.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());

    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node1.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));

    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait for the node1 session to also timeout, validate that its disconnected");

    nexus.AdvanceTime(25 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node2.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(!node3.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());

    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);
}

//----------------------------------------------------------------------------------------------------------------------

static bool sEphemeralKeyCallbackCalled = false;

void HandleEphemeralKeyChange(void *aContext)
{
    Node *node;

    VerifyOrQuit(aContext != nullptr);
    node = reinterpret_cast<Node *>(aContext);

    Log("  EphemeralKeyCallback() state:%s",
        EphemeralKeyManager::StateToString(node->Get<EphemeralKeyManager>().GetState()));
    sEphemeralKeyCallbackCalled = true;
}

void TestBorderAgentEphemeralKey(void)
{
    static const char kEphemeralKey[]         = "nexus1234";
    static const char kTooShortEphemeralKey[] = "abcde";
    static const char kTooLongEphemeralKey[]  = "012345678901234567890123456789012";

    static constexpr uint16_t kEphemeralKeySize = sizeof(kEphemeralKey) - 1;
    static constexpr uint16_t kUdpPort          = 49155;

    Core                         nexus;
    Node                        &node0 = nexus.CreateNode();
    Node                        &node1 = nexus.CreateNode();
    Node                        &node2 = nexus.CreateNode();
    Ip6::SockAddr                sockAddr;
    Ip6::SockAddr                baSockAddr;
    Pskc                         pskc;
    BorderAgent::SessionIterator iter;
    BorderAgent::SessionInfo     sessionInfo;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentEphemeralKey");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - node0 leader acting as Border Agent,
    // - node1 and node2 staying disconnected (acting as candidate)

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    SuccessOrQuit(node2.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node2.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node2.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent ephemeral key counter's initial value");

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationMaxAttempts == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationClears == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcInvalidArgsErrors == 0);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcInvalidBaStateErrors == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent ephemeral key feature enabled");

    node0.Get<EphemeralKeyManager>().SetEnabled(false);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateDisabled);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort) ==
                 kErrorInvalidState);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcInvalidBaStateErrors == 1);

    node0.Get<EphemeralKeyManager>().SetEnabled(true);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Agent ephemeral key initial state");

    sEphemeralKeyCallbackCalled = false;
    VerifyOrQuit(!sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set and start ephemeral key on Border Agent");

    node0.Get<EphemeralKeyManager>().SetCallback(HandleEphemeralKeyChange, &node0);

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 1);

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
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateConnected);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 1);

    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disconnect from candidate side, check that ephemeral key use is stopped");

    sEphemeralKeyCallbackCalled = false;

    node1.Get<Tmf::SecureAgent>().Close();

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start using ephemeral key again with short timeout (20 seconds) and establish a connection");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, 20 * Time::kOneSecondInMsec, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateConnected);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);

    Log("  Check the ephemeral key timeout behavior");

    sEphemeralKeyCallbackCalled = false;
    nexus.AdvanceTime(25 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start using ephemeral key without any candidate connecting, check timeout");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    Log("  Wait more than 120 seconds (default ephemeral key timeout)");
    sEphemeralKeyCallbackCalled = false;
    nexus.AdvanceTime(122 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 3);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that ephemeral key use is stopped after max failed connection attempts");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize - 2));

    for (uint8_t numAttempts = 1; numAttempts < 10; numAttempts++)
    {
        Log("  Attempt %u to connect with the wrong key", numAttempts);

        SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
        VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    }

    Log("  Attempt 10 (final attempt) to connect with the wrong key, check that ephemeral key use is stopped");

    sEphemeralKeyCallbackCalled = false;
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 4);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationMaxAttempts == 1);

    node1.Get<Tmf::SecureAgent>().Close();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that disabling ephemeral key will stop and disconnect an active session");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateConnected);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    sEphemeralKeyCallbackCalled = false;
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateConnected);
    node0.Get<EphemeralKeyManager>().SetEnabled(false);

    nexus.AdvanceTime(0);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateDisabled);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 5);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 3);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationMaxAttempts == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationClears == 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check ephemeral key can be used along with PSKc");

    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());
    SuccessOrQuit(node0.Get<Ip6::Filter>().AddUnsecurePort(node0.Get<BorderAgent>().GetUdpPort()));

    node0.Get<EphemeralKeyManager>().SetEnabled(true);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    Log("  Establish a secure session using PSKc (from node2)");

    baSockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    baSockAddr.SetPort(node0.Get<BorderAgent>().GetUdpPort());

    node0.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));
    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node2.Get<Tmf::SecureAgent>().Connect(baSockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    Log("  Establish a secure session using ephemeral key (from node1)");

    node1.Get<Tmf::SecureAgent>().Close();
    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateConnected);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    Log("  Stop the secure session using ephemeral key");

    node0.Get<EphemeralKeyManager>().Stop();

    sEphemeralKeyCallbackCalled = false;

    nexus.AdvanceTime(0);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    VerifyOrQuit(sEphemeralKeyCallbackCalled);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 6);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcSecureSessionSuccesses == 4);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationTimeouts == 2);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationDisconnects == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationMaxAttempts == 1);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcDeactivationClears == 2);

    Log("  Check the BA session using PSKc is still connected");

    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node2.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);

    iter.Init(node0.GetInstance());
    SuccessOrQuit(iter.GetNextSessionInfo(sessionInfo));
    VerifyOrQuit(sessionInfo.mIsConnected);
    VerifyOrQuit(!sessionInfo.mIsCommissioner);
    VerifyOrQuit(node2.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo.mPeerSockAddr.mAddress)));
    VerifyOrQuit(iter.GetNextSessionInfo(sessionInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check proper error is returned with invalid ephemeral key (too short or long)");

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().Start(kTooShortEphemeralKey, /* aTimeout */ 0, kUdpPort) ==
                 kErrorInvalidArgs);

    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 6);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcInvalidArgsErrors == 1);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().Start(kTooLongEphemeralKey, /* aTimeout */ 0, kUdpPort) ==
                 kErrorInvalidArgs);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcActivations == 6);
    VerifyOrQuit(node0.Get<BorderAgent>().GetCounters().mEpskcInvalidArgsErrors == 2);
}

//----------------------------------------------------------------------------------------------------------------------

class TxtDataTester
{
public:
    TxtDataTester(BorderAgent &aBorderAgent)
        : mBorderAgent(aBorderAgent)
        , mIsRunning(false)
        , mUdpPort(0)
        , mCallbackInvoked(false)
    {
    }

    static void HandleServiceChanged(void *aContext) { static_cast<TxtDataTester *>(aContext)->HandleServiceChanged(); }

    void HandleServiceChanged(void)
    {
        mIsRunning = mBorderAgent.IsRunning();
        mUdpPort   = mBorderAgent.GetUdpPort();
        SuccessOrQuit(mBorderAgent.PrepareServiceTxtData(mTxtData));
        mCallbackInvoked = true;
    }

    bool FindTxtEntry(const char *aKey, TxtEntry &aTxtEntry)
    {
        bool               found = false;
        TxtEntry::Iterator iter;

        iter.Init(mTxtData.mData, mTxtData.mLength);
        while (iter.GetNextEntry(aTxtEntry) == kErrorNone)
        {
            if (strcmp(aTxtEntry.mKey, aKey) == 0)
            {
                found = true;
                break;
            }
        }

        return found;
    }

    BorderAgent                &mBorderAgent;
    BorderAgent::ServiceTxtData mTxtData;
    bool                        mIsRunning;
    uint16_t                    mUdpPort;
    bool                        mCallbackInvoked;
};

template <typename ObjectType> bool CheckObjectSameAsTxtEntryData(const TxtEntry &aTxtEntry, const ObjectType &aObject)
{
    static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

    return aTxtEntry.mValueLength == sizeof(ObjectType) && memcmp(aTxtEntry.mValue, &aObject, sizeof(ObjectType)) == 0;
}

template <> bool CheckObjectSameAsTxtEntryData<NameData>(const TxtEntry &aTxtEntry, const NameData &aNameData)
{
    return aTxtEntry.mValueLength == aNameData.GetLength() &&
           memcmp(aTxtEntry.mValue, aNameData.GetBuffer(), aNameData.GetLength()) == 0;
}

void TestBorderAgentTxtDataCallback(void)
{
    // State bitmap masks and field values
    static constexpr uint32_t kMaskConnectionMode           = 7 << 0;
    static constexpr uint32_t kConnectionModeDisabled       = 0 << 0;
    static constexpr uint32_t kConnectionModePskc           = 1 << 0;
    static constexpr uint32_t kMaskThreadIfStatus           = 3 << 3;
    static constexpr uint32_t kThreadIfStatusNotInitialized = 0 << 3;
    static constexpr uint32_t kThreadIfStatusInitialized    = 1 << 3;
    static constexpr uint32_t kThreadIfStatusActive         = 2 << 3;
    static constexpr uint32_t kMaskThreadRole               = 3 << 9;
    static constexpr uint32_t kThreadRoleDisabledOrDetached = 0 << 9;
    static constexpr uint32_t kThreadRoleChild              = 1 << 9;
    static constexpr uint32_t kThreadRoleRouter             = 2 << 9;
    static constexpr uint32_t kThreadRoleLeader             = 3 << 9;
    static constexpr uint32_t kFlagEpskcSupported           = 1 << 11;

    Core          nexus;
    Node         &node0 = nexus.CreateNode();
    TxtDataTester txtDataTester(node0.Get<BorderAgent>());
    TxtEntry      txtEntry;
    uint32_t      stateBitmap;
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    BorderAgent::Id id;
    BorderAgent::Id newId;
#endif

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentTxtDataCallback");

    nexus.AdvanceTime(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // 1. Set MeshCoP service change callback. Will get initial values.
    Log("Set MeshCoP service change callback and check initial values");
    node0.Get<BorderAgent>().SetServiceChangedCallback(TxtDataTester::HandleServiceChanged, &txtDataTester);
    nexus.AdvanceTime(1);

    // 1.1 Check the initial TXT entries
    VerifyOrQuit(txtDataTester.mCallbackInvoked);
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    VerifyOrQuit(txtDataTester.FindTxtEntry("id", txtEntry));
    VerifyOrQuit(node0.Get<BorderAgent>().GetId(id) == kErrorNone);
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, id));
#endif
    VerifyOrQuit(txtDataTester.FindTxtEntry("nn", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<NetworkNameManager>().GetNetworkName().GetAsData()));
    VerifyOrQuit(txtDataTester.FindTxtEntry("xp", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<ExtendedPanIdManager>().GetExtPanId()));
    VerifyOrQuit(txtDataTester.FindTxtEntry("tv", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, NameData(kThreadVersionString, strlen(kThreadVersionString))));
    VerifyOrQuit(txtDataTester.FindTxtEntry("xa", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<Mac::Mac>().GetExtAddress()));

    VerifyOrQuit(txtDataTester.FindTxtEntry("sb", txtEntry));
    VerifyOrQuit(txtEntry.mValueLength == sizeof(uint32_t));
    stateBitmap = BigEndian::ReadUint32(txtEntry.mValue);
    VerifyOrQuit((stateBitmap & kMaskConnectionMode) == kConnectionModeDisabled);
    VerifyOrQuit((stateBitmap & kMaskThreadIfStatus) == kThreadIfStatusNotInitialized);
    VerifyOrQuit((stateBitmap & kMaskThreadRole) == kThreadRoleDisabledOrDetached);
    VerifyOrQuit(stateBitmap & kFlagEpskcSupported);

    VerifyOrQuit(txtDataTester.FindTxtEntry("pt", txtEntry) == false);
    VerifyOrQuit(txtDataTester.FindTxtEntry("at", txtEntry) == false);

    // 1.2 Check the Border Agent state
    VerifyOrQuit(txtDataTester.mIsRunning == false);
    VerifyOrQuit(txtDataTester.mUdpPort == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // 2. Join Thread network and check updated values and states.
    txtDataTester.mCallbackInvoked = false;
    Log("Join Thread network and check updated Txt data and states");
    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    // 2.1 Check the initial TXT entries
    VerifyOrQuit(txtDataTester.mCallbackInvoked);
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    VerifyOrQuit(txtDataTester.FindTxtEntry("id", txtEntry));
    VerifyOrQuit(node0.Get<BorderAgent>().GetId(id) == kErrorNone);
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, id));
#endif
    VerifyOrQuit(txtDataTester.FindTxtEntry("nn", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<NetworkNameManager>().GetNetworkName().GetAsData()));
    VerifyOrQuit(txtDataTester.FindTxtEntry("xp", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<ExtendedPanIdManager>().GetExtPanId()));
    VerifyOrQuit(txtDataTester.FindTxtEntry("tv", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, NameData(kThreadVersionString, strlen(kThreadVersionString))));
    VerifyOrQuit(txtDataTester.FindTxtEntry("xa", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<Mac::Mac>().GetExtAddress()));
    VerifyOrQuit(txtDataTester.FindTxtEntry("pt", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(
        txtEntry, BigEndian::HostSwap32(node0.Get<Mle::Mle>().GetLeaderData().GetPartitionId())));
    VerifyOrQuit(txtDataTester.FindTxtEntry("at", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, node0.Get<ActiveDatasetManager>().GetTimestamp()));

    VerifyOrQuit(txtDataTester.FindTxtEntry("sb", txtEntry));
    VerifyOrQuit(txtEntry.mValueLength == sizeof(uint32_t));
    stateBitmap = BigEndian::ReadUint32(txtEntry.mValue);
    VerifyOrQuit((stateBitmap & kMaskConnectionMode) == kConnectionModePskc);
    VerifyOrQuit((stateBitmap & kMaskThreadIfStatus) == kThreadIfStatusActive);
    VerifyOrQuit((stateBitmap & kMaskThreadRole) == kThreadRoleLeader);
    VerifyOrQuit(stateBitmap & kFlagEpskcSupported);

    // 2.2 Check the Border Agent state
    VerifyOrQuit(txtDataTester.mIsRunning == true);
    VerifyOrQuit(txtDataTester.mUdpPort != 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    Log("Change the Border Agent ID and validate that TXT data changed and callback is invoked");

    newId.GenerateRandom();
    VerifyOrQuit(newId != id);

    txtDataTester.mCallbackInvoked = false;
    SuccessOrQuit(node0.Get<BorderAgent>().SetId(newId));

    nexus.AdvanceTime(1);

    VerifyOrQuit(txtDataTester.mCallbackInvoked);
    VerifyOrQuit(txtDataTester.FindTxtEntry("id", txtEntry));
    VerifyOrQuit(CheckObjectSameAsTxtEntryData(txtEntry, newId));

    // Validate that setting the ID to the same value as before is
    // correctly detected and does not trigger the callback.

    txtDataTester.mCallbackInvoked = false;
    SuccessOrQuit(node0.Get<BorderAgent>().SetId(newId));
    nexus.AdvanceTime(1);
    VerifyOrQuit(!txtDataTester.mCallbackInvoked);

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable EphemeralKeyManager and validate that TXT data state bitmap indicates this");

    txtDataTester.mCallbackInvoked = false;
    node0.Get<EphemeralKeyManager>().SetEnabled(false);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateDisabled);

    nexus.AdvanceTime(1);
    VerifyOrQuit(txtDataTester.mCallbackInvoked);

    VerifyOrQuit(txtDataTester.FindTxtEntry("sb", txtEntry));
    VerifyOrQuit(txtEntry.mValueLength == sizeof(uint32_t));
    stateBitmap = BigEndian::ReadUint32(txtEntry.mValue);
    VerifyOrQuit((stateBitmap & kMaskConnectionMode) == kConnectionModePskc);
    VerifyOrQuit((stateBitmap & kMaskThreadIfStatus) == kThreadIfStatusActive);
    VerifyOrQuit((stateBitmap & kMaskThreadRole) == kThreadRoleLeader);
    VerifyOrQuit(!(stateBitmap & kFlagEpskcSupported));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable the MLE operation and validate the TXT data state bitmap");

    txtDataTester.mCallbackInvoked = false;
    SuccessOrQuit(node0.Get<Mle::Mle>().Disable());

    nexus.AdvanceTime(1);
    VerifyOrQuit(txtDataTester.mCallbackInvoked);

    VerifyOrQuit(txtDataTester.FindTxtEntry("sb", txtEntry));
    VerifyOrQuit(txtEntry.mValueLength == sizeof(uint32_t));
    stateBitmap = BigEndian::ReadUint32(txtEntry.mValue);
    VerifyOrQuit((stateBitmap & kMaskConnectionMode) == kConnectionModeDisabled);
    VerifyOrQuit((stateBitmap & kMaskThreadIfStatus) == kThreadIfStatusNotInitialized);
    VerifyOrQuit((stateBitmap & kMaskThreadRole) == kThreadRoleDisabledOrDetached);
    VerifyOrQuit(!(stateBitmap & kFlagEpskcSupported));
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAgent();
    ot::Nexus::TestBorderAgentEphemeralKey();
    ot::Nexus::TestBorderAgentTxtDataCallback();
    printf("All tests passed\n");
    return 0;
}
