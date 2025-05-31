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
using EphemeralKeyManager  = MeshCoP::BorderAgent::EphemeralKeyManager;
using HistoryTracker       = Utils::HistoryTracker;
using EpskcEvent           = HistoryTracker::EpskcEvent;
using Iterator             = HistoryTracker::Iterator;
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

EpskcEvent GetNewestEpskcEvent(Node &aNode)
{
    const EpskcEvent *epskcEvent = nullptr;
    Iterator          iter;
    uint32_t          age;
    iter.Init();

    epskcEvent = aNode.Get<HistoryTracker>().IterateEpskcEventHistory(iter, age);

    VerifyOrQuit(epskcEvent != nullptr);
    return *epskcEvent;
}

void TestHistoryTrackerBorderAgentEpskcEvent(void)
{
    static const char kEphemeralKey[] = "nexus1234";

    static constexpr uint16_t kEphemeralKeySize = sizeof(kEphemeralKey) - 1;
    static constexpr uint16_t kUdpPort          = 49155;

    Core           nexus;
    Node          &node0 = nexus.CreateNode();
    Node          &node1 = nexus.CreateNode();
    Ip6::SockAddr  sockAddr;
    Coap::Message *message;
    EpskcEvent     epskcEvent;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestHistoryTrackerBorderAgentEpskcEvent");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - node0 leader acting as Border Agent.
    // - node1 acting as candidate

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());
    sockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(kUdpPort);

    SuccessOrQuit(node1.Get<Mac::Mac>().SetPanChannel(node0.Get<Mac::Mac>().GetPanChannel()));
    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    // Enable the Ephemeral Key feature.
    node0.Get<EphemeralKeyManager>().SetEnabled(true);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 1: Epskc Mode is activated, then deactivated");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    node0.Get<EphemeralKeyManager>().Stop();
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_LOCAL_CLOSE);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 2: Epskc Mode is activated, then deactivated after max failed connection "
        "attempts");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    SuccessOrQuit(node0.Get<Ip6::Filter>().AddUnsecurePort(kUdpPort));

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    nexus.AdvanceTime(0);

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open());
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

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_MAX_ATTEMPTS);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 3: The session is connected successfully, then disconnected by API");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    SuccessOrQuit(
        node1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(kEphemeralKey), kEphemeralKeySize));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    node0.Get<EphemeralKeyManager>().Stop();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_LOCAL_CLOSE);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 4: The session is connected successfully. And the candidate petitioned as an "
        "active commissioner. The session is disconnected by the remote.");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED);

    node1.Get<Tmf::SecureAgent>().Disconnect();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_REMOTE_CLOSE);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 5: The session is connected successfully. And the active dataset is fetched. "
        "The session is disconnected by the remote.");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriActiveGet);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_ACTIVE_DATASET);

    node1.Get<Tmf::SecureAgent>().Disconnect();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_REMOTE_CLOSE);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 6: The session is connected successfully. And the pending dataset is fetched. "
        "The session is disconnected by the remote.");

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriActiveGet);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_ACTIVE_DATASET);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriPendingGet);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_PENDING_DATASET);

    node1.Get<Tmf::SecureAgent>().Disconnect();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_REMOTE_CLOSE);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 7: The session is connected successfully. And the pending dataset is fetched. "
        "The session is due to session timeout.");
    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriActiveGet);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_ACTIVE_DATASET);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriPendingGet);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_RETRIEVED_PENDING_DATASET);

    static constexpr uint32_t kKeepAliveTimeout = 50 * 1000; // Timeout to reject a commissioner (in msec)
    nexus.AdvanceTime(kKeepAliveTimeout);                    // Wait for the session timeout
    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_SESSION_TIMEOUT);

    Log("------------------------------------------------------------------------------------------------------");
    Log("  Check Ephemeral Key Journey 8: The session is connected successfully. The epskc mode is deactivated due to "
        "timeout.");
    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_ACTIVATED);

    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_CONNECTED);

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerPetition);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_PETITIONED);

    // Send a KeepAlive message every 30s until ePSKc mode timed out
    for (uint8_t i = 0; i < EphemeralKeyManager::kDefaultTimeout / (30 * Time::kOneSecondInMsec) - 1; i++)
    {
        if (!node1.Get<Tmf::SecureAgent>().IsConnected())
        {
            break;
        }
        message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerKeepAlive);
        VerifyOrQuit(message != nullptr);
        SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
        SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));
        nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

        epskcEvent = GetNewestEpskcEvent(node0);
        VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_KEEP_ALIVE);
    }

    message = node1.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriCommissionerKeepAlive);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerIdTlv>(*message, "node1"));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message));
    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    epskcEvent = GetNewestEpskcEvent(node0);
    VerifyOrQuit(epskcEvent == OT_HISTORY_TRACKER_BORDER_AGENT_EPSKC_EVENT_DEACTIVATED_EPSKC_TIMEOUT);
}

//----------------------------------------------------------------------------------------------------------------------

struct TxtData
{
    void Init(const uint8_t *aData, uint16_t aLength) { mData = aData, mLength = aLength; }

    void ValidateFormat(void)
    {
        TxtEntry::Iterator iter;
        TxtEntry           txtEntry;

        iter.Init(mData, mLength);

        while (true)
        {
            Error error = iter.GetNextEntry(txtEntry);

            if (error == kErrorNotFound)
            {
                break;
            }

            SuccessOrQuit(error);
            VerifyOrQuit(txtEntry.mKey != nullptr);
        }
    }

    void LogAllTxtEntries(void)
    {
        static constexpr uint16_t kValueStringSize = 256;

        char               valueString[kValueStringSize];
        TxtEntry::Iterator iter;
        TxtEntry           txtEntry;

        Log("TXT data - length %u", mLength);

        iter.Init(mData, mLength);

        while (true)
        {
            Error        error = iter.GetNextEntry(txtEntry);
            StringWriter writer(valueString, sizeof(valueString));

            if (error == kErrorNotFound)
            {
                break;
            }

            SuccessOrQuit(error);
            VerifyOrQuit(txtEntry.mKey != nullptr);

            writer.AppendHexBytes(txtEntry.mValue, txtEntry.mValueLength);
            Log("   %s -> [%s] (len:%u)", txtEntry.mKey, valueString, txtEntry.mValueLength);
        }
    }

    bool ContainsKey(const char *aKey) const
    {
        TxtEntry txtEntry;

        return FindTxtEntry(aKey, txtEntry);
    }

    void ValidateKey(const char *aKey, const void *aValue, uint16_t aValueLength) const
    {
        TxtEntry txtEntry;

        VerifyOrQuit(FindTxtEntry(aKey, txtEntry));
        VerifyOrQuit(txtEntry.mValueLength == aValueLength);
        VerifyOrQuit(memcmp(txtEntry.mValue, aValue, aValueLength) == 0);
    }

    template <typename ObjectType> void ValidateKey(const char *aKey, const ObjectType &aObject) const
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        ValidateKey(aKey, &aObject, sizeof(aObject));
    }

    void ValidateKey(const char *aKey, const char *aString) const { ValidateKey(aKey, aString, strlen(aString)); }

    uint32_t ReadUint32Key(const char *aKey) const
    {
        TxtEntry txtEntry;

        VerifyOrQuit(FindTxtEntry(aKey, txtEntry));
        VerifyOrQuit(txtEntry.mValueLength == sizeof(uint32_t));
        return BigEndian::ReadUint32(txtEntry.mValue);
    }

    bool FindTxtEntry(const char *aKey, TxtEntry &aTxtEntry) const
    {
        bool               found = false;
        TxtEntry::Iterator iter;

        iter.Init(mData, mLength);

        while (iter.GetNextEntry(aTxtEntry) == kErrorNone)
        {
            VerifyOrQuit(aTxtEntry.mKey != nullptr);

            if (StringMatch(aTxtEntry.mKey, aKey))
            {
                found = true;
                break;
            }
        }

        return found;
    }

    const uint8_t *mData;
    uint16_t       mLength;
};

void ValidateMeshCoPTxtData(TxtData &aTxtData, Node &aNode)
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

    BorderAgent::Id id;
    uint32_t        stateBitmap;
    uint32_t        threadIfStatus;
    uint32_t        threadRole;

    aTxtData.ValidateFormat();
    aTxtData.LogAllTxtEntries();

    SuccessOrQuit(aNode.Get<BorderAgent>().GetId(id));
    aTxtData.ValidateKey("id", id);
    aTxtData.ValidateKey("rv", "1");
    aTxtData.ValidateKey("nn", aNode.Get<NetworkNameManager>().GetNetworkName().GetAsCString());
    aTxtData.ValidateKey("xp", aNode.Get<ExtendedPanIdManager>().GetExtPanId());
    aTxtData.ValidateKey("tv", kThreadVersionString);
    aTxtData.ValidateKey("xa", aNode.Get<Mac::Mac>().GetExtAddress());

    if (aNode.Get<Mle::Mle>().IsAttached())
    {
        aTxtData.ValidateKey("pt", BigEndian::HostSwap32(aNode.Get<Mle::Mle>().GetLeaderData().GetPartitionId()));
        aTxtData.ValidateKey("at", aNode.Get<ActiveDatasetManager>().GetTimestamp());
    }
    else
    {
        VerifyOrQuit(!aTxtData.ContainsKey("pt"));
        VerifyOrQuit(!aTxtData.ContainsKey("at"));
    }

    stateBitmap = aTxtData.ReadUint32Key("sb");

    VerifyOrQuit((stateBitmap & kMaskConnectionMode) == aNode.Get<BorderAgent>().IsRunning() ? kConnectionModePskc
                                                                                             : kConnectionModeDisabled);
    switch (aNode.Get<Mle::Mle>().GetRole())
    {
    case Mle::DeviceRole::kRoleDisabled:
        threadIfStatus = kThreadIfStatusNotInitialized;
        threadRole     = kThreadRoleDisabledOrDetached;
        break;
    case Mle::DeviceRole::kRoleDetached:
        threadIfStatus = kThreadIfStatusInitialized;
        threadRole     = kThreadRoleDisabledOrDetached;
        break;
    case Mle::DeviceRole::kRoleChild:
        threadIfStatus = kThreadIfStatusActive;
        threadRole     = kThreadRoleChild;
        break;
    case Mle::DeviceRole::kRoleRouter:
        threadIfStatus = kThreadIfStatusActive;
        threadRole     = kThreadRoleRouter;
        break;
    case Mle::DeviceRole::kRoleLeader:
        threadIfStatus = kThreadIfStatusActive;
        threadRole     = kThreadRoleLeader;
        break;
    }

    VerifyOrQuit((stateBitmap & kMaskThreadIfStatus) == threadIfStatus);
    VerifyOrQuit((stateBitmap & kMaskThreadRole) == threadRole);

    if (aNode.Get<BorderAgent>().Get<EphemeralKeyManager>().GetState() !=
        BorderAgent::EphemeralKeyManager::kStateDisabled)
    {
        VerifyOrQuit(stateBitmap & kFlagEpskcSupported);
    }
    else
    {
        VerifyOrQuit(!(stateBitmap & kFlagEpskcSupported));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void HandleServiceChanged(void *aContext) // Callback used in `TestBorderAgentTxtDataCallback().`
{
    // `aContext` is a boolean `callbackInvoked`
    VerifyOrQuit(aContext != nullptr);
    *static_cast<bool *>(aContext) = true;
}

void ReadAndValidateMeshCoPTxtData(Node &aNode)
{
    BorderAgent::ServiceTxtData serviceTxtData;
    TxtData                     txtData;

    SuccessOrQuit(aNode.Get<BorderAgent>().PrepareServiceTxtData(serviceTxtData));
    txtData.Init(serviceTxtData.mData, serviceTxtData.mLength);

    ValidateMeshCoPTxtData(txtData, aNode);
}

void TestBorderAgentTxtDataCallback(void)
{
    Core            nexus;
    Node           &node0           = nexus.CreateNode();
    bool            callbackInvoked = false;
    BorderAgent::Id newId;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentTxtDataCallback");

    nexus.AdvanceTime(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Set MeshCoP service change callback. Will get initial values.
    Log("Set MeshCoP service change callback and check initial values");
    node0.Get<BorderAgent>().SetServiceChangedCallback(HandleServiceChanged, &callbackInvoked);
    nexus.AdvanceTime(1);

    // Check the initial TXT entries
    ReadAndValidateMeshCoPTxtData(node0);

    // Check the Border Agent state
    VerifyOrQuit(!node0.Get<BorderAgent>().IsRunning());
    VerifyOrQuit(node0.Get<BorderAgent>().GetUdpPort() == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Join Thread network and check updated values and states.
    callbackInvoked = false;
    Log("Join Thread network and check updated Txt data and states");
    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(callbackInvoked);
    ReadAndValidateMeshCoPTxtData(node0);

    VerifyOrQuit(node0.Get<BorderAgent>().IsRunning());
    VerifyOrQuit(node0.Get<BorderAgent>().GetUdpPort() != 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Change the Border Agent ID and validate that TXT data changed and callback is invoked");

    newId.GenerateRandom();

    callbackInvoked = false;
    SuccessOrQuit(node0.Get<BorderAgent>().SetId(newId));

    nexus.AdvanceTime(1);
    ReadAndValidateMeshCoPTxtData(node0);

    // Validate that setting the ID to the same value as before is
    // correctly detected and does not trigger the callback.

    callbackInvoked = false;
    SuccessOrQuit(node0.Get<BorderAgent>().SetId(newId));
    nexus.AdvanceTime(1);
    VerifyOrQuit(!callbackInvoked);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable EphemeralKeyManager and validate that TXT data state bitmap indicates this");

    callbackInvoked = false;
    node0.Get<EphemeralKeyManager>().SetEnabled(false);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateDisabled);

    nexus.AdvanceTime(1);
    VerifyOrQuit(callbackInvoked);
    ReadAndValidateMeshCoPTxtData(node0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable the MLE operation and validate the TXT data state bitmap");

    callbackInvoked = false;
    SuccessOrQuit(node0.Get<Mle::Mle>().Disable());

    nexus.AdvanceTime(1);
    VerifyOrQuit(callbackInvoked);
    ReadAndValidateMeshCoPTxtData(node0);
}

//----------------------------------------------------------------------------------------------------------------------

static constexpr uint32_t kInfraIfIndex   = 1;
static constexpr uint16_t kMaxEntries     = 5;
static constexpr uint16_t kMaxTxtDataSize = 128;

typedef Dns::Name::Buffer DnsName;

struct BrowseOutcome
{
    DnsName  mServiceInstance;
    uint32_t mTtl;
};

struct SrvOutcome
{
    DnsName  mHostName;
    uint16_t mPort;
    uint32_t mTtl;
};

struct TxtOutcome
{
    uint8_t  mTxtData[kMaxTxtDataSize];
    uint16_t mTxtDataLength;
    uint32_t mTtl;
};

static Array<BrowseOutcome, kMaxEntries> sBrowseOutcomes;
static Array<SrvOutcome, kMaxEntries>    sSrvOutcomes;
static Array<TxtOutcome, kMaxEntries>    sTxtOutcomes;

void HandleBrowseCallback(otInstance *aInstance, const Dns::Multicast::Core::BrowseResult *aResult)
{
    BrowseOutcome *outcome;

    VerifyOrQuit(aInstance != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    outcome = sBrowseOutcomes.PushBack();
    VerifyOrQuit(outcome != nullptr);

    SuccessOrQuit(StringCopy(outcome->mServiceInstance, aResult->mServiceInstance));
    outcome->mTtl = aResult->mTtl;
}

void HandleSrvCallback(otInstance *aInstance, const Dns::Multicast::Core::SrvResult *aResult)
{
    SrvOutcome *outcome;

    VerifyOrQuit(aInstance != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    outcome = sSrvOutcomes.PushBack();
    VerifyOrQuit(outcome != nullptr);

    SuccessOrQuit(StringCopy(outcome->mHostName, aResult->mHostName));
    outcome->mPort = aResult->mPort;
    outcome->mTtl  = aResult->mTtl;
}

void HandleTxtCallback(otInstance *aInstance, const Dns::Multicast::Core::TxtResult *aResult)
{
    TxtOutcome *outcome;

    VerifyOrQuit(aInstance != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    outcome = sTxtOutcomes.PushBack();
    VerifyOrQuit(outcome != nullptr);

    VerifyOrQuit(aResult->mTxtDataLength < kMaxTxtDataSize);
    memcpy(outcome->mTxtData, aResult->mTxtData, aResult->mTxtDataLength);
    outcome->mTxtDataLength = aResult->mTxtDataLength;
    outcome->mTtl           = aResult->mTtl;
}

void ValidateRegisteredServiceData(Dns::Multicast::Core::Service &aService, Node &aNode)
{
    TxtData txtData;

    txtData.Init(aService.mTxtData, aService.mTxtDataLength);
    ValidateMeshCoPTxtData(txtData, aNode);
}

void TestBorderAgentServiceRegistration(void)
{
    static const char    kDefaultServiceBaseName[] = OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME;
    static const char    kEphemeralKey[]           = "nexus1234";
    static const uint8_t kVendorTxtData[]          = {8, 'v', 'n', '=', 'n', 'e', 'x', 'u', 's'};

    static constexpr uint32_t kUdpPort = 49155;

    Core                              nexus;
    Node                             &node0 = nexus.CreateNode();
    Node                             &node1 = nexus.CreateNode();
    Dns::Multicast::Core::Iterator   *iterator;
    Dns::Multicast::Core::Service     service;
    Dns::Multicast::Core::EntryState  entryState;
    Dns::Multicast::Core::Browser     browser;
    Dns::Multicast::Core::SrvResolver srvResolver;
    Dns::Multicast::Core::TxtResolver txtResolver;
    TxtData                           txtData;
    uint16_t                          txtDataLengthWithNoVendorData;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentServiceRegistration");

    nexus.AdvanceTime(0);

    node0.Form();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable Border Agent function on node1
    node1.Get<MeshCoP::BorderAgent>().SetEnabled(false);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Enable mDNS
    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().IsEnabled());
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    VerifyOrQuit(node1.Get<Dns::Multicast::Core>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by Border Agent");

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));

    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, kDefaultServiceBaseName));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the service can be resolved on other nodes");

    ClearAllBytes(browser);
    browser.mServiceType  = "_meshcop._udp";
    browser.mInfraIfIndex = kInfraIfIndex;
    browser.mCallback     = HandleBrowseCallback;
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().StartBrowser(browser));

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 1);
    VerifyOrQuit(StringStartsWith(sBrowseOutcomes[0].mServiceInstance, kDefaultServiceBaseName));
    VerifyOrQuit(sBrowseOutcomes[0].mTtl > 0);

    ClearAllBytes(srvResolver);
    srvResolver.mServiceInstance = sBrowseOutcomes[0].mServiceInstance;
    srvResolver.mServiceType     = browser.mServiceType;
    srvResolver.mInfraIfIndex    = kInfraIfIndex;
    srvResolver.mCallback        = HandleSrvCallback;
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().StartSrvResolver(srvResolver));

    ClearAllBytes(txtResolver);
    txtResolver.mServiceInstance = sBrowseOutcomes[0].mServiceInstance;
    txtResolver.mServiceType     = browser.mServiceType;
    txtResolver.mInfraIfIndex    = kInfraIfIndex;
    txtResolver.mCallback        = HandleTxtCallback;
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().StartTxtResolver(txtResolver));

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(sSrvOutcomes.GetLength() == 1);
    VerifyOrQuit(StringStartsWith(sSrvOutcomes[0].mHostName, "ot"));
    VerifyOrQuit(sSrvOutcomes[0].mTtl > 0);
    VerifyOrQuit(sSrvOutcomes[0].mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());

    VerifyOrQuit(sTxtOutcomes.GetLength() == 1);
    VerifyOrQuit(sTxtOutcomes[0].mTtl > 0);
    txtData.Init(sTxtOutcomes[0].mTxtData, sTxtOutcomes[0].mTxtDataLength);
    ValidateMeshCoPTxtData(txtData, node0);

    sBrowseOutcomes.Clear();
    sSrvOutcomes.Clear();
    sTxtOutcomes.Clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable and start ephemeral key");

    node0.Get<EphemeralKeyManager>().SetEnabled(true);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStopped);
    node0.Get<EphemeralKeyManager>().SetCallback(HandleEphemeralKeyChange, &node0);

    SuccessOrQuit(node0.Get<EphemeralKeyManager>().Start(kEphemeralKey, /* aTimeout */ 0, kUdpPort));

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetState() == EphemeralKeyManager::kStateStarted);
    VerifyOrQuit(node0.Get<EphemeralKeyManager>().GetUdpPort() == kUdpPort);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the registered services");

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    for (uint8_t num = 2; num > 0; num--)
    {
        SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
        Log("- - - - - - - - - - - - - - - - -");
        Log("  HostName: %s", service.mHostName);
        Log("  ServiceInstance: %s", service.mServiceInstance);
        Log("  ServiceType: %s", service.mServiceType);
        Log("  Port: %u", service.mPort);
        Log("  TTL: %lu", ToUlong(service.mTtl));

        VerifyOrQuit(StringStartsWith(service.mServiceInstance, kDefaultServiceBaseName));
        VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
        VerifyOrQuit(service.mSubTypeLabelsLength == 0);
        VerifyOrQuit(service.mTtl > 0);
        VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
        VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);

        if (StringMatch(service.mServiceType, "_meshcop._udp"))
        {
            VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
            ValidateRegisteredServiceData(service, node0);
        }
        else if (StringMatch(service.mServiceType, "_meshcop-e._udp"))
        {
            VerifyOrQuit(service.mPort == kUdpPort);
            VerifyOrQuit(service.mTxtDataLength == 1);
            VerifyOrQuit(service.mTxtData[0] == 0);
        }
        else
        {
            // Unexpected service type
            VerifyOrQuit(false);
        }
    }

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 0);
    VerifyOrQuit(sSrvOutcomes.GetLength() == 0);
    VerifyOrQuit(sTxtOutcomes.GetLength() == 0);

    Log("Wait for the ephemeral key to expire and validate the registered service is removed");

    nexus.AdvanceTime(5 * Time::kOneMinuteInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, kDefaultServiceBaseName));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Change the base service name and validate the new service");

    SuccessOrQuit(node0.Get<MeshCoP::BorderAgent>().SetServiceBaseName("OpenThreadAgent"));

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, "OpenThreadAgent"));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    // Check browser/resolver outcome on node1

    VerifyOrQuit(sSrvOutcomes.GetLength() == 1);
    VerifyOrQuit(sSrvOutcomes[0].mTtl == 0);
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().StopSrvResolver(srvResolver));

    VerifyOrQuit(sTxtOutcomes.GetLength() == 1);
    VerifyOrQuit(sTxtOutcomes[0].mTtl == 0);
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().StopTxtResolver(txtResolver));

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 2);
    VerifyOrQuit(sBrowseOutcomes[0].mTtl == 0);
    VerifyOrQuit(StringStartsWith(sBrowseOutcomes[0].mServiceInstance, kDefaultServiceBaseName));

    VerifyOrQuit(sBrowseOutcomes[1].mTtl > 0);
    VerifyOrQuit(StringStartsWith(sBrowseOutcomes[1].mServiceInstance, "OpenThreadAgent"));

    sBrowseOutcomes.Clear();
    sSrvOutcomes.Clear();
    sTxtOutcomes.Clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable Border Agent and validate that registered service is removed");

    node0.Get<MeshCoP::BorderAgent>().SetEnabled(false);
    VerifyOrQuit(!node0.Get<MeshCoP::BorderAgent>().IsEnabled());

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 1);
    VerifyOrQuit(sBrowseOutcomes[0].mTtl == 0);
    VerifyOrQuit(StringStartsWith(sBrowseOutcomes[0].mServiceInstance, "OpenThreadAgent"));

    sBrowseOutcomes.Clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-enable Border Agent and validate that service is registered again");

    node0.Get<MeshCoP::BorderAgent>().SetEnabled(true);
    VerifyOrQuit(node0.Get<MeshCoP::BorderAgent>().IsEnabled());

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, "OpenThreadAgent"));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);
    txtDataLengthWithNoVendorData = service.mTxtDataLength;

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 1);
    VerifyOrQuit(sBrowseOutcomes[0].mTtl > 0);
    VerifyOrQuit(StringStartsWith(sBrowseOutcomes[0].mServiceInstance, "OpenThreadAgent"));

    sBrowseOutcomes.Clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set vendor TXT data and validate that it is included in the registered mDNS service");

    node0.Get<BorderAgent>().SetVendorTxtData(kVendorTxtData, sizeof(kVendorTxtData));
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, "OpenThreadAgent"));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);

    // Check that vendor TXT data is included at the end of
    // the registered service TXT data.
    VerifyOrQuit(service.mTxtDataLength > txtDataLengthWithNoVendorData);
    VerifyOrQuit(service.mTxtDataLength > sizeof(kVendorTxtData));
    VerifyOrQuit(!memcmp(&service.mTxtData[service.mTxtDataLength - sizeof(kVendorTxtData)], kVendorTxtData,
                         sizeof(kVendorTxtData)));

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Clear vendor TXT data and validate that the registered mDNS service is updated accordingly");

    node0.Get<BorderAgent>().SetVendorTxtData(nullptr, 0);
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    iterator = node0.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));
    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);
    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, "OpenThreadAgent"));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    VerifyOrQuit(service.mPort == node0.Get<MeshCoP::BorderAgent>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);
    ValidateRegisteredServiceData(service, node0);
    VerifyOrQuit(service.mTxtDataLength == txtDataLengthWithNoVendorData);

    // Check that there is no more registered mDNS service
    VerifyOrQuit(node0.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    node0.Get<Dns::Multicast::Core>().FreeIterator(*iterator);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAgent();
    ot::Nexus::TestBorderAgentEphemeralKey();
    ot::Nexus::TestHistoryTrackerBorderAgentEpskcEvent();
    ot::Nexus::TestBorderAgentTxtDataCallback();
    ot::Nexus::TestBorderAgentServiceRegistration();
    printf("All tests passed\n");
    return 0;
}
