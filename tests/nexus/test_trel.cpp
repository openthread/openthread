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

static constexpr ot::Trel::Peer::DnssdState kDnssdResolved = ot::Trel::Peer::kDnssdResolved;
static constexpr ot::Trel::Peer::DnssdState kDnssdRemoved  = ot::Trel::Peer::kDnssdRemoved;

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

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    for (Node &node : nexus.GetNodes())
    {
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
            VerifyOrQuit(peer.GetExtPanId() == node.Get<MeshCoP::NetworkIdentity>().GetExtPanId());

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

void TestTrelUserDisableReenable(void)
{
    // Validates user disabling of TREL interface persists after
    // OpenThread state changes such as Thread netif down and up.

    Core  nexus;
    Node &node = nexus.CreateNode();

    Log("---------------------------------------------------------------------------------------");
    Log("TestTrelUserDisableReenable()");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));
    SuccessOrQuit(node.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form network");

    node.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    VerifyOrQuit(node.Get<ot::Trel::Interface>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable TREL interface by `kRequesterUser`");

    node.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
    VerifyOrQuit(!node.Get<ot::Trel::Interface>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Bring down Thread netif and stop MAC and all radio links");

    node.Get<ThreadNetif>().Down();
    node.Get<Mac::Mac>().SetEnabled(false);

    VerifyOrQuit(node.Get<Mle::Mle>().IsDisabled());
    VerifyOrQuit(!node.Get<ot::Trel::Interface>().IsEnabled());

    nexus.AdvanceTime(1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Bring up the Thread netif down and restart MLE and Thread operation");

    node.Get<ThreadNetif>().Up();
    SuccessOrQuit(node.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Ensure TREL interface remains disabled since it was explicitly disabled by `kRequesterUser`");

    VerifyOrQuit(!node.Get<ot::Trel::Interface>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable TREL interface by `kRequesterUser` ");

    node.Get<ot::Trel::Interface>().SetEnabled(true, ot::Trel::Interface::kRequesterUser);

    VerifyOrQuit(node.Get<ot::Trel::Interface>().IsEnabled());
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

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    for (Node &node : nexus.GetNodes())
    {
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
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node2.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetNext() == nullptr);

    // Check peer on `node2` to match `node1` info.
    peer = node2.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node1.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node1.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node1.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node1.mInfraIf.GetLinkLocalAddress());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable TREL Interface (and `PeerDiscoverer`) on `node2`");

    node2.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `PeerTable` is properly updated on `node1`");

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == kDnssdRemoved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() == nullptr);
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 0);
    VerifyOrQuit(peer->GetNext() == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-enable TREL Interface (and `PeerDiscoverer`) on `node2`");

    node2.Get<ot::Trel::Interface>().SetEnabled(true, ot::Trel::Interface::kRequesterUser);
    nexus.AdvanceTime(15 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that peer tables again updated on both nodes");

    VerifyOrQuit(node1.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);
    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 1);

    // Check peer on `node1` to match `node2` info.
    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node2.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node2.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetNext() == nullptr);

    // Check peer on `node2` to match `node1` info.
    peer = node2.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == ot::Trel::Peer::kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node1.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), node1.Get<ot::Trel::PeerDiscoverer>().GetServiceName()));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == node1.mTrel.mUdpPort);
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node1.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node1.mInfraIf.GetLinkLocalAddress());

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    inactiveDuration = peer->DetermineSecondsSinceLastInteraction();
    VerifyOrQuit(inactiveDuration > 0);
    Log("- peer has been inactive for %lu seconds", ToUlong(inactiveDuration));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable TREL Interface (and `PeerDiscoverer`) on `node2` again and signal its removal on mDNS");

    node2.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
    VerifyOrQuit(node2.Get<ot::Trel::PeerTable>().IsEmpty());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that peer entry for `node2` is properly switched to `kDnssdRemoved` state");

    nexus.AdvanceTime(10 * 1000 + 500);

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == kDnssdRemoved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mInfraIf.GetLinkLocalAddress());

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

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `conflictNode`");

    conflictNode.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
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
            VerifyOrQuit(peer.GetExtPanId() == node1.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
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

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(!node.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `node2`");

    node2.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    Log("Manually register a TREL service on `node2` with proper TXT data");

    SuccessOrQuit(encoder.AppendEntry("xa", node2.Get<Mac::Mac>().GetExtAddress()));
    SuccessOrQuit(encoder.AppendEntry("xp", node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId()));

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
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(peer->GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress());

    VerifyOrQuit(peer->GetServiceName() != nullptr);
    VerifyOrQuit(StringMatch(peer->GetServiceName(), service.mServiceInstance));
    VerifyOrQuit(peer->GetHostName() != nullptr);
    VerifyOrQuit(StringStartsWith(peer->GetHostName(), "ot"));
    VerifyOrQuit(StringEndsWith(peer->GetHostName(), node2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));
    VerifyOrQuit(peer->GetSockAddr().GetPort() == service.mPort);

    VerifyOrQuit(peer->GetSockAddr().GetAddress() == node2.mInfraIf.GetLinkLocalAddress());
    VerifyOrQuit(peer->GetHostAddresses().GetLength() == 1);
    VerifyOrQuit(peer->GetHostAddresses()[0] == node2.mInfraIf.GetLinkLocalAddress());

    VerifyOrQuit(peer->GetNext() == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Update the advertised local host addresses on `node2`");

    node2.mInfraIf.RemoveAllAddresses();

    SuccessOrQuit(guaAddr.FromString("2001:cafe::4567"));
    node2.mInfraIf.AddAddress(guaAddr);

    SuccessOrQuit(ulaAddr.FromString("fd00:abba::1234"));
    node2.mInfraIf.AddAddress(ulaAddr);

    SuccessOrQuit(linkLocalAddr.FromString("fe80::bd2c:a124"));
    node2.mInfraIf.AddAddress(linkLocalAddr);

    nexus.AdvanceTime(3 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate peer table on `node1` discovers all the new addresses");

    peer = node1.Get<ot::Trel::PeerTable>().GetHead();
    VerifyOrQuit(peer != nullptr);

    VerifyOrQuit(peer->GetDnssdState() == kDnssdResolved);
    VerifyOrQuit(peer->GetExtPanId() == node2.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
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
        VerifyOrQuit(node2.mInfraIf.HasAddress(hostAddress));
    }

    for (const Ip6::Address &ifAddress : node2.mInfraIf.GetAddresses())
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
    Dns::Multicast::Core::Service services[3];
    uint8_t                       txtData[kMaxTxtDataSize];
    Ip6::Address                  address;

    Log("---------------------------------------------------------------------------------------");
    Log("TestMultiServiceSameHost()");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    for (Node &nodeEntry : nexus.GetNodes())
    {
        VerifyOrQuit(!nodeEntry.Get<Dns::Multicast::Core>().IsEnabled());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Disable TREL interface but enable mDNS on `multiServiceNode`");

    multiServiceNode.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
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

    for (const ot::Trel::Peer &peerEntry : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        VerifyOrQuit(peerEntry.GetDnssdState() == kDnssdResolved);
        VerifyOrQuit(peerEntry.GetServiceName() != nullptr);
        VerifyOrQuit(peerEntry.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peerEntry.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peerEntry.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peerEntry.GetSockAddr().GetAddress() == multiServiceNode.mInfraIf.GetLinkLocalAddress());
        VerifyOrQuit(peerEntry.GetHostAddresses().GetLength() == 1);
        VerifyOrQuit(peerEntry.GetHostAddresses()[0] == multiServiceNode.mInfraIf.GetLinkLocalAddress());

        for (const Dns::Multicast::Core::Service &service : services)
        {
            if (StringMatch(peerEntry.GetServiceName(), service.mServiceInstance))
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

    for (const ot::Trel::Peer &peerEntry : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        if (peerEntry.GetDnssdState() != kDnssdResolved)
        {
            continue;
        }

        VerifyOrQuit(peerEntry.GetServiceName() != nullptr);
        VerifyOrQuit(peerEntry.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peerEntry.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peerEntry.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peerEntry.GetSockAddr().GetAddress() == multiServiceNode.mInfraIf.GetLinkLocalAddress());
        VerifyOrQuit(peerEntry.GetHostAddresses().GetLength() == 1);
        VerifyOrQuit(peerEntry.GetHostAddresses()[0] == multiServiceNode.mInfraIf.GetLinkLocalAddress());

        for (uint16_t index = 0; index < 2; index++)
        {
            if (StringMatch(peerEntry.GetServiceName(), services[index].mServiceInstance))
            {
                found = true;
            }
        }

        VerifyOrQuit(found);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Update the local host addresses on `multiServiceNode`");

    SuccessOrQuit(address.FromString("fd00:abba::1234"));
    multiServiceNode.mInfraIf.AddAddress(address);

    nexus.AdvanceTime(5 * 1000);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate all peers get the updated list");

    VerifyOrQuit(node.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 3);

    for (const ot::Trel::Peer &peerEntry : node.Get<ot::Trel::PeerTable>())
    {
        bool found = false;

        if (peerEntry.GetDnssdState() != kDnssdResolved)
        {
            continue;
        }

        VerifyOrQuit(peerEntry.GetServiceName() != nullptr);
        VerifyOrQuit(peerEntry.GetHostName() != nullptr);
        VerifyOrQuit(StringStartsWith(peerEntry.GetHostName(), "ot"));
        VerifyOrQuit(StringEndsWith(peerEntry.GetHostName(),
                                    multiServiceNode.Get<Mac::Mac>().GetExtAddress().ToString().AsCString()));

        VerifyOrQuit(peerEntry.GetSockAddr().GetAddress() == multiServiceNode.mInfraIf.GetLinkLocalAddress());
        VerifyOrQuit(peerEntry.GetHostAddresses().GetLength() == 2);
        VerifyOrQuit(peerEntry.GetHostAddresses()[0] == multiServiceNode.mInfraIf.GetLinkLocalAddress());
        VerifyOrQuit(peerEntry.GetHostAddresses()[1] == multiServiceNode.mInfraIf.GetAddresses()[1]);

        for (uint16_t index = 0; index < 2; index++)
        {
            if (StringMatch(peerEntry.GetServiceName(), services[index].mServiceInstance))
            {
                found = true;
            }
        }

        VerifyOrQuit(found);
    }
}

#if OPENTHREAD_CONFIG_MULTI_RADIO
void TestMultiRadioRxFragmentTag(void)
{
    Core      nexus;
    Node     &node1 = nexus.CreateNode();
    Node     &node2 = nexus.CreateNode();
    Neighbor *neighbor;

    Log("---------------------------------------------------------------------------------------");
    Log("TestMultiRadioRxFragmentTag()");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    node1.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());

    node2.Join(node1, Node::kAsFed);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(node2.Get<Mle::Mle>().IsChild());

    neighbor = node1.Get<NeighborTable>().FindNeighbor(node2.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(neighbor != nullptr);

    neighbor->ClearLastRxFragmentTag();

    // Exact sequence from TREL-TC-4 out-of-order multi-radio delivery:
    // Tag 8826 received on TREL, then duplicate 8826 and new 8829 on 15.4,
    // followed by 8827, 8828 (TREL-only unicasts) and duplicate 8829 on TREL.
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(8826));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(8826));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(8829));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(8827));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(8828));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(8829));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(8826));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(8827));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(8828));

    // Test bitmap boundary (delta == 32 and delta > 32)
    neighbor->ClearLastRxFragmentTag();
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(99));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(100));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(132)); // delta == 32 with non-zero bitmap
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(100));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(101)); // delta == 31
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(101));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(131)); // delta == 1
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(131));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(99)); // delta == 33 (> 32)

    // Test large advance (> 32) clearing bitmap
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(200)); // delta == 68 (> 32)
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(199));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(199));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(200));

    // Test 16-bit tag wrap-around (0xfffe, 0xffff, 0x0001, 0x0002)
    neighbor->ClearLastRxFragmentTag();
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(0xfffe));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(0x0002));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(0xfffe));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(0xffff));
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(0x0001));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(0xffff));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(0x0001));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(0x0002));

    // Test tag timeout expiration
    nexus.AdvanceTime(OPENTHREAD_CONFIG_MULTI_RADIO_FRAG_TAG_TIMEOUT + 1);
    VerifyOrQuit(!neighbor->IsRxFragmentTagDuplicate(0x0002));
    VerifyOrQuit(neighbor->IsRxFragmentTagDuplicate(0x0002));
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
void TestMultiRadioTimeSync(void)
{
    static constexpr uint16_t kTimeSyncPeriod = 10;

    Core     nexus;
    Node    &leader     = nexus.CreateNode();
    Node    &router     = nexus.CreateNode();
    Node    &fed        = nexus.CreateNode();
    Node    &sed        = nexus.CreateNode();
    Node    &fed154     = nexus.CreateNode();
    Node    &sed154     = nexus.CreateNode();
    Node    &routerTrel = nexus.CreateNode();
    Node    &fedTrel    = nexus.CreateNode();
    uint64_t leaderTime;
    uint64_t nodeTime;

    Log("---------------------------------------------------------------------------------------");
    Log("TestMultiRadioTimeSync()");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    for (Node &node : nexus.GetNodes())
    {
        SuccessOrQuit(otNetworkTimeSetSyncPeriod(&node.GetInstance(), kTimeSyncPeriod));
    }

    SuccessOrQuit(leader.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(router.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(fed.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(sed.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(routerTrel.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(fedTrel.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    fed154.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);
    sed154.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);

    // Filter 802.15.4 on `routerTrel` and `fedTrel` so they only communicate over TREL.
    routerTrel.Get<Mac::Mac>().SetRadioFilterEnabled(true);
    fedTrel.Get<Mac::Mac>().SetRadioFilterEnabled(true);

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(otNetworkTimeGet(&leader.GetInstance(), &leaderTime) == OT_NETWORK_TIME_SYNCHRONIZED);
    VerifyOrQuit(leader.Get<TimeSync>().GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ);

    // Join both a multi-radio SED and an 802.15.4-only SED.
    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(1000));
    SuccessOrQuit(sed154.Get<DataPollSender>().SetExternalPollPeriod(1000));
    sed.Join(leader, Node::kAsSed);
    sed154.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed154.Get<Mle::Mle>().IsChild());

    router.Join(leader);
    fed.Join(leader, Node::kAsFed);
    fed154.Join(leader, Node::kAsFed);
    routerTrel.Join(leader);
    fedTrel.Join(leader, Node::kAsFed);

    nexus.AdvanceTime(150 * 1000);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed154.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed154.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(routerTrel.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fedTrel.Get<Mle::Mle>().IsChild());

    VerifyOrQuit(leader.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 5);
    VerifyOrQuit(router.Get<ot::Trel::PeerTable>().GetNumberOfPeers() == 5);

    // Verify `routerTrel` and `fedTrel` are TREL-only neighbors and remain unsynchronized
    // because Time Sync IE is only carried over 802.15.4.
    {
        Node *const trelOnlyNodes[] = {&routerTrel, &fedTrel};

        for (Node *trelNode : trelOnlyNodes)
        {
            Neighbor::MultiRadioInfo info;
            Neighbor                *neighbor;

            neighbor = leader.Get<NeighborTable>().FindNeighbor(trelNode->Get<Mac::Mac>().GetExtAddress());
            VerifyOrQuit(neighbor != nullptr);
            neighbor->PopulateMultiRadioInfo(info);
            VerifyOrQuit(!info.mSupportsIeee802154);
            VerifyOrQuit(info.mSupportsTrelUdp6);

            VerifyOrQuit(otNetworkTimeGet(&trelNode->GetInstance(), &nodeTime) == OT_NETWORK_TIME_UNSYNCHRONIZED);
        }
    }

    VerifyOrQuit(otNetworkTimeGet(&leader.GetInstance(), &leaderTime) == OT_NETWORK_TIME_SYNCHRONIZED);

    {
        Node *const rxOnNodes[] = {&leader, &router, &fed, &fed154};

        for (Node *rxOnNode : rxOnNodes)
        {
            VerifyOrQuit(otNetworkTimeGet(&rxOnNode->GetInstance(), &nodeTime) == OT_NETWORK_TIME_SYNCHRONIZED);
            VerifyOrQuit(nodeTime == leaderTime);
            VerifyOrQuit(rxOnNode->Get<TimeSync>().GetTimeSyncSeq() == leader.Get<TimeSync>().GetTimeSyncSeq());
        }
    }

    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(router, fed.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(leader, sed.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, 3000);
    nexus.SendAndVerifyEchoRequest(leader, sed154.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, 3000);
    nexus.SendAndVerifyEchoRequest(router, fed154.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(leader, routerTrel.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(leader, fedTrel.Get<Mle::Mle>().GetMeshLocalEid());

    // Re-enable 802.15.4 on `fedTrel` and verify it synchronizes upon the next 802.15.4 Time Sync.
    fedTrel.Get<Mac::Mac>().SetRadioFilterEnabled(false);
    nexus.AdvanceTime((kTimeSyncPeriod + 5) * 1000);

    VerifyOrQuit(otNetworkTimeGet(&leader.GetInstance(), &leaderTime) == OT_NETWORK_TIME_SYNCHRONIZED);
    VerifyOrQuit(otNetworkTimeGet(&fedTrel.GetInstance(), &nodeTime) == OT_NETWORK_TIME_SYNCHRONIZED);
    VerifyOrQuit(nodeTime == leaderTime);
    VerifyOrQuit(fedTrel.Get<TimeSync>().GetTimeSyncSeq() == leader.Get<TimeSync>().GetTimeSyncSeq());
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#endif // OPENTHREAD_CONFIG_MULTI_RADIO

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ot::Nexus::TestTrelBasic();
    ot::Nexus::TestTrelUserDisableReenable();
    ot::Nexus::TestTrelDelayedMdnsStartAndPeerRemovalDelay();
    ot::Nexus::TestServiceNameConflict();
    ot::Nexus::TestHostAddressChange();
    ot::Nexus::TestMultiServiceSameHost();
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ot::Nexus::TestMultiRadioRxFragmentTag();
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    ot::Nexus::TestMultiRadioTimeSync();
#endif
#endif
    printf("All tests passed\n");
#else
    printf("TREL is not enabled - test skipped\n");
#endif

    return 0;
}
