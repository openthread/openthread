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

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

static constexpr uint32_t kInfraIfIndex   = 1;
static constexpr uint16_t kMaxTxtDataSize = 128;

static constexpr ot::Trel::Peer::DnssdState kDnssdResolved  = ot::Trel::Peer::kDnssdResolved;
static constexpr ot::Trel::Peer::DnssdState kDnssdRemoved   = ot::Trel::Peer::kDnssdRemoved;
static constexpr ot::Trel::Peer::DnssdState kDnssdResolving = ot::Trel::Peer::kDnssdResolving;

void TestTrelBasic(void)
{
    // Validate basic operations, forming a network and
    // joining as router, FED, MED, and SED using TREL.
    // Also check TREL peer table on all devices.

    Core  nexus;
    Node &leader  = nexus.CreateNode();
    Node &fed     = nexus.CreateNode();
    Node &sed     = nexus.CreateNode();
    Node &med     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    Log("---------------------------------------------------------------------------------------");
    Log("TestTrelBasic()");

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelWarn);
        SuccessOrQuit(node.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    fed.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    router1.Join(leader);
    router2.Join(leader);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check all nodes roles and device modes");

    nexus.AdvanceTime(300 * 1000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    VerifyOrQuit(fed.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsFullThreadDevice());

    VerifyOrQuit(med.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(med.Get<Mle::Mle>().IsMinimalEndDevice());

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsFullThreadDevice());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check TREL peer table on all nodes");

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(node.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 5);

        for (const ot::Trel::Peer &peer : node.Get<ot::Trel::PeerTable>())
        {
            bool found = false;

            VerifyOrQuit(peer.GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
            VerifyOrQuit(peer.GetExtPanId() == node.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());

            for (Node &otherNode : nexus.GetNodes())
            {
                if (&otherNode == &node)
                {
                    continue;
                }

                if (peer.GetExtAddress() == otherNode.Get<Mac::Mac>().GetExtAddress())
                {
                    Ip6::SockAddr otherSockAddr;

                    found = true;
                    otherNode.GetTrelSockAddr(otherSockAddr);
                    VerifyOrQuit(peer.GetSockAddr() == otherSockAddr);

                    VerifyOrQuit(peer.GetServiceName() != nullptr);
                    VerifyOrQuit(
                        StringMatch(peer.GetServiceName(), otherNode.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));

                    VerifyOrQuit(peer.GetHostName() != nullptr);
                    VerifyOrQuit(StringStartsWith(peer.GetHostName(), "ot"));
                    VerifyOrQuit(StringEndsWith(peer.GetHostName(),
                                                otherNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

                    VerifyOrQuit(peer.GetHostAddresses().GetLength() == 1);
                    VerifyOrQuit(peer.GetHostAddresses()[0] == otherSockAddr.GetAddress());
                    break;
                }
            }

            VerifyOrQuit(found);

            // Check the format of the TREL service name
            VerifyOrQuit(StringStartsWith(node.Get<ot::Trel::PeerDiscoverer>().GetServiceName(), "otTREL"));
            VerifyOrQuit(StringEndsWith(node.Get<ot::Trel::PeerDiscoverer>().GetServiceName(),
                                        node.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
        }
    }
}

void TestTrelDelayedMdnsStartAndPeerRemovalDelay(void)
{
    Core                  nexus;
    Node                 &node1 = nexus.CreateNode();
    Node                 &node2 = nexus.CreateNode();
    const ot::Trel::Peer *peer;
    uint32_t              inactiveDuration;

    Log("---------------------------------------------------------------------------------------");
    Log("TestTrelDelayedMdnsStartAndPeerRemovalDelay()");

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelWarn);
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start first network on node1");

    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    node1.Form();
    nexus.AdvanceTime(45 * 1000);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());

    // Check that `node1` did not discover any `TREL` peer,
    // as `mDNS` is not yet enabled on other nodes.
    // Additionally, the `TREL` peer table must exclude the device itself.

    VerifyOrQuit(node1.Get<ot::Trel::PeerTable>().IsEmpty());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable mDNS on `node2`, form a new network");

    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    node2.Form();
    nexus.AdvanceTime(45 * 1000);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that peer tables are correctly updated on both `node1` and `node2`");

    VerifyOrQuit(node1.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);
    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);

    // Check peer on `node1` to match `node2` info.
    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node2.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetNext() == nullptr);

    // Check peer on `node2` to match `node1` info.
    peer = node2.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node1.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node1.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node1.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node1.mMdns.mIfAddresses[0]);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable TREL Interface (and `PeerDiscoverer`) on `node2`");

    node2.Get<ot::Trel::Interface>().Disable();
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `PeerTable` is properly updated on `node1`");

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdRemoved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() == nullptr);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 0);
    VerifyOrQuit(peer->GetNext() == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-enable TREL Interface (and `PeerDiscoverer`) on `node2`");

    node2.Get<ot::Trel::Interface>().Enable();
    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that peer tables again updated on both nodes");

    VerifyOrQuit(node1.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);
    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);

    // Check peer on `node1` to match `node2` info.
    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node2.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetNext() == nullptr);

    // Check peer on `node2` to match `node1` info.
    peer = node2.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node1.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node1.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node1.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node1.mMdns.mIfAddresses[0]);

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    inactiveDuration = peer->DetermineSecondsSinceLastInteraction();
    VerifyOrQuit(inactiveDuration > 0);
    Log("- peer has been inactive for %lu seconds", ToUlong(inactiveDuration));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable TREL Interface (and `PeerDiscoverer`) on `node2` again and signal its removal on mDNS");

    node2.Get<ot::Trel::Interface>().Disable();
    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().IsEmpty());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that peer entry for `node2` is properly switched to `kDnssdRemoved` state");

    nexus.AdvanceTime(10 * 1000 + 500);

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdRemoved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mMdns.mIfAddresses[0]);

    Log("Validate the `DetermineSecondsSinceLastInteraction()` is properly tracked");

    VerifyOrQuit(peer->DetermineSecondsSinceLastInteraction() - inactiveDuration >= 10);

    inactiveDuration = peer->DetermineSecondsSinceLastInteraction();
    VerifyOrQuit(inactiveDuration > 0);
    Log("- peer has been inactive for %lu seconds", ToUlong(inactiveDuration));

    Log("Validate that peer is deleted from list after 450 second inactivity");

    nexus.AdvanceTime((451 - inactiveDuration) * 1000);

    VerifyOrQuit(node1.Get<ot::Trel::PeerTable>().IsEmpty());

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer == nullptr);
}

void TestServiceNameConflict(void)
{
    Core                          nexus;
    Node                         &node1        = nexus.CreateNode();
    Node                         &node2        = nexus.CreateNode();
    Node                         &conflictNode = nexus.CreateNode();
    Dns::Multicast::Core::Service service;
    Dns::Name::Buffer             conflictName;
    bool                          peersValidated;

    Log("---------------------------------------------------------------------------------------");
    Log("TestServiceNameConflict()");

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelWarn);
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `conflictNode`");

    conflictNode.Get<ot::Trel::Interface>().Disable();
    SuccessOrQuit(conflictNode.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    Log("Register a service on `conflictNode` with same name that `node1` would use");

    SuccessOrQuit(StringCopy(conflictName, node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));

    ClearAllBytes(service);
    service.mServiceType     = "_trel._udp";
    service.mServiceInstance = conflictName;
    service.mPort            = 12345;

    SuccessOrQuit(conflictNode.Get<Dns::Multicast::Core>().RegisterService(service, /* aRequestId */ 0, nullptr));

    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable mDNS on `node1` and `node2` and form a new network");

    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    node1.Form();
    nexus.AdvanceTime(45 * 1000);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());

    node2.Join(node1, Node::kAsFed);
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(node2.Get<Mle::Mle>().IsChild());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `node1` correctly detected the name conflict and appended a (1) to its service name");

    VerifyOrQuit(StringStartsWith(node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName(), conflictName));
    VerifyOrQuit(StringEndsWith(node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName(), "(1)"));

    Log("Check peer table on `node2` to match `node1` info");

    peersValidated = false;

    for (const ot::Trel::Peer &peer : node2.Get<ot::Trel::PeerTable>())
    {
        if (peer.GetDnssdState() == ot::Trel::Peer::kDnssdResolved)
        {
            VerifyOrQuit(peer.GetExtPanId() == node1.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
            VerifyOrQuit(peer.GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress());
            VerifyOrQuit(peer.GetServiceName() != nullptr);
            VerifyOrQuit(StringMatch(peer.GetServiceName(), node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
            peersValidated = true;
            break;
        }
    }

    VerifyOrQuit(peersValidated);
}

void TestHostAddressChange(void)
{
    Core                          nexus;
    Node                         &node1 = nexus.CreateNode();
    Node                         &node2 = nexus.CreateNode();
    const ot::Trel::Peer         *peer;
    Dns::Multicast::Core::Service service;
    uint8_t                       txtData[kMaxTxtDataSize];
    Dns::TxtDataEncoder           encoder(txtData, sizeof(txtData));
    Ip6::Address                  linkLocalAddr;
    Ip6::Address                  guaAddr;
    Ip6::Address                  ulaAddr;

    Log("---------------------------------------------------------------------------------------");
    Log("TestHostAddressChange()");

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelWarn);
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `node2`");

    node2.Get<ot::Trel::Interface>().Disable();
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    Log("Manually register a TREL service on `node2` with proper TXT data");

    SuccessOrQuit(encoder.AppendEntry("xa", node2.Get<Mac::Mac>().GetExtAddress()));
    SuccessOrQuit(encoder.AppendEntry("xp", node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    ClearAllBytes(service);
    service.mServiceType     = "_trel._udp";
    service.mServiceInstance = "node2";
    service.mTxtData         = txtData;
    service.mTxtDataLength   = encoder.GetLength();
    service.mPort            = 3333;

    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().RegisterService(service, /* aRequestId */ 0, nullptr));

    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form a new network on `node1`");

    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    node1.Form();
    nexus.AdvanceTime(45 * 1000);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate peer table on `node1` and `node2` is discovered properly");

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());

    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), service.mServiceInstance));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == service.mPort);

    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mMdns.mIfAddresses[0]);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mMdns.mIfAddresses[0]);

    VerifyOrQuit(peer->GetNext() == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Update the advertised local host addresses on `node2`");

    node2.mMdns.mIfAddresses.Clear();

    SuccessOrQuit(guaAddr.FromString("2001:cafe::4567"));
    SuccessOrQuit(node2.mMdns.mIfAddresses.PushBack(guaAddr));

    SuccessOrQuit(ulaAddr.FromString("fd00:abba::1234"));
    SuccessOrQuit(node2.mMdns.mIfAddresses.PushBack(ulaAddr));

    SuccessOrQuit(linkLocalAddr.FromString("fe80::bd2c:a124"));
    SuccessOrQuit(node2.mMdns.mIfAddresses.PushBack(linkLocalAddr));

    node2.mMdns.SignalIfAddresses(node2.GetInstance());

    nexus.AdvanceTime(3 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate peer table on `node1` discovers all the new addresses");

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());

    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), service.mServiceInstance));

    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == service.mPort);

    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 3);

    for (const Ip6::Address &hostAddress : peer->GetHostAddresses())
    {
        VerifyOrQuit(node2.mMdns.mIfAddresses.Contains(hostAddress));
    }

    for (const Ip6::Address &ifAddress : node2.mMdns.mIfAddresses)
    {
        VerifyOrQuit(peer->GetHostAddresses().Contains(ifAddress));
    }

    // Check the order of addresses in list:

    VerifyOrQuit(peer->GetHostAddresses()[0] == linkLocalAddr);
    VerifyOrQuit(peer->GetHostAddresses()[1] == guaAddr);
    VerifyOrQuit(peer->GetHostAddresses()[2] == ulaAddr);

    Log("Validate the peer `SockAddr` is correctly updated based on new discovered host addresses");

    VerifyOrQuit(peer->GetSockAddr().GetAddress() == linkLocalAddr);
    VerifyOrQuit(peer->GetSockAddr().GetPort() == service.mPort);

    VerifyOrQuit(peer->GetNext() == nullptr);
}

void TestMultiServiceSameHost(void)
{
    Core                          nexus;
    Node                         &node             = nexus.CreateNode();
    Node                         &multiServiceNode = nexus.CreateNode();
    const ot::Trel::Peer         *peer;
    Dns::Multicast::Core::Service services[3];
    uint8_t                       txtData[kMaxTxtDataSize];
    Ip6::Address                  address;

    Log("---------------------------------------------------------------------------------------");
    Log("TestMultiServiceSameHost()");

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelInfo);
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `multiServiceNode`");

    multiServiceNode.Get<ot::Trel::Interface>().Disable();
    SuccessOrQuit(multiServiceNode.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    Log("Manually register three TREL services on the `multiServiceNode`");

    {
        Dns::TxtDataEncoder encoder(txtData, sizeof(txtData));

        SuccessOrQuit(encoder.AppendEntry("xa", "0102030405060708"));
        SuccessOrQuit(encoder.AppendEntry("xp", "0102030405060708"));
        ClearAllBytes(services[0]);
        services[0].mServiceType     = "_trel._udp";
        services[0].mServiceInstance = "service0";
        services[0].mTxtData         = txtData;
        services[0].mTxtDataLength   = encoder.GetLength();
        services[0].mPort            = 11111;
    }

    SuccessOrQuit(
        multiServiceNode.Get<Dns::Multicast::Core>().RegisterService(services[0], /* aRequestId */ 0, nullptr));

    {
        Dns::TxtDataEncoder encoder(txtData, sizeof(txtData));
        SuccessOrQuit(encoder.AppendEntry("xa", "1122334455667788"));
        SuccessOrQuit(encoder.AppendEntry("xp", "1122334455667788"));
        ClearAllBytes(services[1]);
        services[1].mServiceType     = "_trel._udp";
        services[1].mServiceInstance = "service1";
        services[1].mTxtData         = txtData;
        services[1].mTxtDataLength   = encoder.GetLength();
        services[1].mPort            = 2222;
    }

    SuccessOrQuit(
        multiServiceNode.Get<Dns::Multicast::Core>().RegisterService(services[1], /* aRequestId */ 0, nullptr));

    {
        Dns::TxtDataEncoder encoder(txtData, sizeof(txtData));

        SuccessOrQuit(encoder.AppendEntry("xa", "1020304050607080"));
        SuccessOrQuit(encoder.AppendEntry("xp", "1020304050607080"));
        ClearAllBytes(services[2]);
        services[2].mServiceType     = "_trel._udp";
        services[2].mServiceInstance = "service2";
        services[2].mTxtData         = txtData;
        services[2].mTxtDataLength   = encoder.GetLength();
        services[2].mPort            = 3333;
    }

    SuccessOrQuit(
        multiServiceNode.Get<Dns::Multicast::Core>().RegisterService(services[2], /* aRequestId */ 0, nullptr));

    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form a new network on `node`");

    SuccessOrQuit(node.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    node.Form();
    nexus.AdvanceTime(45 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate peer table on `node` and all services are discovered from the same host");

    VerifyOrQuit(node.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 3);

    for (const ot::Trel::Peer &peer : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        VerifyOrQuit(peer.GetDnssdState() == kDnssdResolved);
        VerifyOrQuit(peer.GetServiceName() != nullptr);
        VerifyOrQuit(peer.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peer.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peer.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peer.GetSockAddr().GetAddress() == multiServiceNode.mMdns.mIfAddresses[0]);
        VerifyOrQuit(peer.GetHostAddresses().GetLength() == 1);
        VerifyOrQuit(peer.GetHostAddresses()[0] == multiServiceNode.mMdns.mIfAddresses[0]);

        for (const Dns::Multicast::Core::Service &service : services)
        {
            if (StringMatch(peer.GetServiceName(), service.mServiceInstance))
            {
                found = true;
            }
        }

        VerifyOrQuit(found);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Unregister the third service on `multiServiceNode`");

    SuccessOrQuit(multiServiceNode.Get<Dns::Multicast::Core>().UnregisterService(services[2]));

    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate peer table on `node`");

    VerifyOrQuit(node.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 3);

    for (const ot::Trel::Peer &peer : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        if (peer.GetDnssdState() != kDnssdResolved)
        {
            continue;
        }

        VerifyOrQuit(peer.GetServiceName() != nullptr);
        VerifyOrQuit(peer.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peer.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peer.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peer.GetSockAddr().GetAddress() == multiServiceNode.mMdns.mIfAddresses[0]);
        VerifyOrQuit(peer.GetHostAddresses().GetLength() == 1);
        VerifyOrQuit(peer.GetHostAddresses()[0] == multiServiceNode.mMdns.mIfAddresses[0]);

        for (uint16_t index = 0; index < 2; index++)
        {
            if (StringMatch(peer.GetServiceName(), services[index].mServiceInstance))
            {
                found = true;
            }
        }

        VerifyOrQuit(found);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Update the local host addresses on `multiServiceNode`");

    SuccessOrQuit(address.FromString("fd00:abba::1234"));
    SuccessOrQuit(multiServiceNode.mMdns.mIfAddresses.PushBack(address));

    multiServiceNode.mMdns.SignalIfAddresses(multiServiceNode.GetInstance());

    nexus.AdvanceTime(5 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate all peers get the updated list");

    VerifyOrQuit(node.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 3);

    for (const ot::Trel::Peer &peer : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        if (peer.GetDnssdState() != kDnssdResolved)
        {
            continue;
        }

        VerifyOrQuit(peer.GetServiceName() != nullptr);
        VerifyOrQuit(peer.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peer.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peer.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peer.GetSockAddr().GetAddress() == multiServiceNode.mMdns.mIfAddresses[0]);
        VerifyOrQuit(peer.GetHostAddresses().GetLength() == 2);
        VerifyOrQuit(peer.GetHostAddresses()[0] == multiServiceNode.mMdns.mIfAddresses[0]);
        VerifyOrQuit(peer.GetHostAddresses()[1] == multiServiceNode.mMdns.mIfAddresses[1]);

        for (uint16_t index = 0; index < 2; index++)
        {
            if (StringMatch(peer.GetServiceName(), services[index].mServiceInstance))
            {
                found = true;
            }
        }

        VerifyOrQuit(found);
    }
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ot::Nexus::TestTrelBasic();
    ot::Nexus::TestTrelDelayedMdnsStartAndPeerRemovalDelay();
    ot::Nexus::TestServiceNameConflict();
    ot::Nexus::TestHostAddressChange();
    ot::Nexus::TestMultiServiceSameHost();
    printf("All tests passed\n");
#else
    printf("TREL is not enabled - test skipped\n");
#endif

    return 0;
}
