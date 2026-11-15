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

#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/udp.h>

#include "mac/data_poll_sender.hpp"
#include "net/checksum.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/** Time to advance for a node to form a network and become leader, in milliseconds. */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/** Time to advance for a node to join as a child and upgrade to a router, in milliseconds. */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
/** Time to wait for ICMPv6 Echo response, in milliseconds. */
static constexpr uint32_t kEchoTimeout = 10 * 1000;

static bool     gForwardedPacketReceived = false;
static uint16_t gForwardedPacketLen      = 0;

static void MyCustomIp6ReceiveCallback(otMessage *aMessage, void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);
    Message    &message = *AsCoreTypePtr(aMessage);
    Ip6::Header header;
    if (header.ParseFrom(message) == kErrorNone)
    {
        if (header.GetNextHeader() == Ip6::kProtoUdp && message.GetLength() == 640)
        {
            gForwardedPacketReceived = true;
            gForwardedPacketLen      = message.GetLength();
            Log("MyCustomIp6ReceiveCallback: Received forwarded packet of length %u", gForwardedPacketLen);
        }
    }
}

static void SendGapFragment1(Node &aRouter, const Ip6::Address &aLeaderEid, uint32_t aIdentification)
{
    Message *message = aRouter.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Outer IPv6 Header
    Ip6::Header outerHeader;
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetPayloadLength(56); // 8 bytes (Fragment Header) + 40 bytes (Inner IPv6) + 8 bytes (UDP Header)
    outerHeader.SetNextHeader(Ip6::kProtoFragment);
    outerHeader.SetHopLimit(64);
    outerHeader.SetSource(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    outerHeader.SetDestination(aLeaderEid);
    SuccessOrQuit(message->Append(outerHeader));

    // 2. Fragment Header
    Ip6::FragmentHeader fragmentHeader;
    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(Ip6::kProtoIp6);
    fragmentHeader.SetOffset(0);
    fragmentHeader.SetMoreFlag();
    fragmentHeader.SetIdentification(aIdentification);
    SuccessOrQuit(message->Append(fragmentHeader));

    // 3. Inner IPv6 Header and UDP Header (Payload of Fragment 1)
    Ip6::Header innerHeader;
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetPayloadLength(600); // 600 bytes inner payload to stay under MTU limit
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetHopLimit(64);
    innerHeader.SetSource(aLeaderEid);
    innerHeader.SetDestination(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    SuccessOrQuit(message->Append(innerHeader));

    Ip6::UdpHeader udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(1234);
    udpHeader.SetDestinationPort(12345);
    udpHeader.SetLength(600);
    SuccessOrQuit(message->Append(udpHeader));

    // Set message priority/etc
    SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));

    // Send it
    SuccessOrQuit(aRouter.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message)));
}

static void SendGapFragment2(Node &aRouter, const Ip6::Address &aLeaderEid, uint32_t aIdentification)
{
    Message *message = aRouter.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Outer IPv6 Header
    Ip6::Header outerHeader;
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetPayloadLength(8); // 8 bytes (Fragment Header)
    outerHeader.SetNextHeader(Ip6::kProtoFragment);
    outerHeader.SetHopLimit(64);
    outerHeader.SetSource(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    outerHeader.SetDestination(aLeaderEid);
    SuccessOrQuit(message->Append(outerHeader));

    // 2. Fragment Header
    Ip6::FragmentHeader fragmentHeader;
    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(Ip6::kProtoIp6);
    fragmentHeader.SetOffset(80);   // 80 * 8 = 640 bytes (under MTU limit)
    fragmentHeader.ClearMoreFlag(); // M = 0
    fragmentHeader.SetIdentification(aIdentification);
    SuccessOrQuit(message->Append(fragmentHeader));

    // Set message priority/etc
    SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));

    // Send it
    SuccessOrQuit(aRouter.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message)));
}

void TestIPv6Fragmentation(void)
{
    /**
     * Test IPv6 Fragmentation
     *
     * Topology:
     * - Leader
     * - Router
     *
     * Description:
     * The purpose of this test case is to validate IPv6 fragmentation and reassembly.
     * Large ICMPv6 Echo Requests (exceeding 1280 bytes MTU) are sent between nodes.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Form network");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("Step 2: Large Echo Request from Leader to Router (1952 bytes payload)");
    // 1952 bytes payload + 8 bytes ICMP header + 40 bytes IPv6 header = 2000 bytes.
    // This exceeds the 1280 bytes MTU and triggers IPv6 fragmentation.
    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 1952, 64, kEchoTimeout);

    Log("Step 3: Large Echo Request from Router to Leader (1831 bytes payload)");
    // 1831 bytes payload + 8 bytes ICMP header + 40 bytes IPv6 header = 1879 bytes.
    // This exceeds the 1280 bytes MTU and triggers IPv6 fragmentation.
    nexus.SendAndVerifyEchoRequest(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), 1831, 64, kEchoTimeout);

    Log("Step 4: Fragment Reassembly Gap Check Test");
    gForwardedPacketReceived = false;
    gForwardedPacketLen      = 0;

    // Register the custom receive callback on Router to intercept the forwarded packet.
    router.Get<Ip6::Ip6>().SetReceiveCallback(MyCustomIp6ReceiveCallback, nullptr);

    {
        uint32_t identification = 0x12345678;
        Log("Sending Fragment 1 (offset=0, M=1)");
        SendGapFragment1(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), identification);
        nexus.AdvanceTime(100); // Short wait

        Log("Sending Fragment 2 (offset=640, M=0)");
        SendGapFragment2(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), identification);
        nexus.AdvanceTime(1000); // Wait for reassembly and forwarding
    }

    // Check if the packet was received (Expect no packet received in patched version!)
    VerifyOrQuit(!gForwardedPacketReceived, "Reassembly gap check failed: forwarded packet was received!");
    Log("Success: No forwarded packet was received. Reassembly gap is properly handled!");

    // Restore default receive callback on Router
    router.Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, &router);

    nexus.SaveTestInfo("test_ipv6_fragmentation.json");
}
#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

static constexpr uint32_t kStabilizationTime   = 30 * 1000;
static constexpr uint32_t kPropagationTime     = 8 * 1000;
static constexpr uint32_t kFragmentSpacingTime = 300;
static constexpr uint32_t kSedPollPeriod       = 200;

static constexpr uint16_t kMulticastUdpPort     = 12346;
static constexpr uint16_t kMulticastSrcPort     = 54321;
static constexpr uint16_t kMulticastPayloadSize = 1500;
static constexpr uint16_t kRawFrag1PayloadSize  = 800;
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
static constexpr uint16_t kBoundaryPayloadSize = 1233;
#endif

static const char kOnMeshPrefix[]    = "2001:db8:1::/64";
static const char kRealmLocalMcast[] = "ff03::1234";
static const char kSiteLocalMcast[]  = "ff05::1234";

static uint8_t ComputePatternByte(uint8_t aTag, uint16_t aIndex)
{
    return static_cast<uint8_t>(aTag ^ static_cast<uint8_t>((aIndex * 37u) + (aIndex >> 8)));
}

static void FillPayloadPattern(uint8_t *aBuffer, uint16_t aLength, uint8_t aTag)
{
    for (uint16_t i = 0; i < aLength; i++)
    {
        aBuffer[i] = ComputePatternByte(aTag, i);
    }
}

struct MulticastReceiver
{
    Node        *mNode;
    otUdpSocket  mUdpSocket;
    Ip6::Address mExpectedDst;
    uint8_t      mExpectedTag;
    uint16_t     mExpectedPayloadLen;
    uint32_t     mUdpRxCount;
    uint32_t     mHostRxCount;
    uint32_t     mHostFragRxCount;

    void Init(Node &aNode)
    {
        otSockAddr bindAddr;

        ClearAllBytes(*this);
        mNode = &aNode;

        ClearAllBytes(bindAddr);
        bindAddr.mPort = kMulticastUdpPort;

        SuccessOrQuit(otUdpOpen(mNode, &mUdpSocket, HandleUdpReceive, this));
        SuccessOrQuit(otUdpBind(mNode, &mUdpSocket, &bindAddr, OT_NETIF_THREAD_INTERNAL));

        otIp6SetReceiveFilterEnabled(mNode, false);
        otIp6SetReceiveCallback(mNode, HandleHostIp6Receive, this);
    }

    void Close(void)
    {
        SuccessOrQuit(otUdpClose(mNode, &mUdpSocket));
        mNode->Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, mNode);
    }

    void ResetExpectation(const Ip6::Address &aExpectedDst, uint8_t aExpectedTag, uint16_t aExpectedPayloadLen)
    {
        mExpectedDst        = aExpectedDst;
        mExpectedTag        = aExpectedTag;
        mExpectedPayloadLen = aExpectedPayloadLen;
        mUdpRxCount         = 0;
        mHostRxCount        = 0;
        mHostFragRxCount    = 0;
    }

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
    {
        static_cast<MulticastReceiver *>(aContext)->HandleUdpReceive(*AsCoreTypePtr(aMessage),
                                                                     *AsCoreTypePtr(aMessageInfo));
    }

    void HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        uint16_t payloadLen = aMessage.DetermineLengthAfterOffset();

        VerifyOrQuit(aMessageInfo.GetSockAddr() == mExpectedDst);
        VerifyOrQuit(payloadLen == mExpectedPayloadLen);

        for (uint16_t i = 0; i < payloadLen; i++)
        {
            uint8_t byte = 0;

            SuccessOrQuit(aMessage.Read(aMessage.GetOffset() + i, byte));
            VerifyOrQuit(byte == ComputePatternByte(mExpectedTag, i));
        }

        mUdpRxCount++;
    }

    static void HandleHostIp6Receive(otMessage *aMessage, void *aContext)
    {
        OwnedPtr<Message> messagePtr(AsCoreTypePtr(aMessage));

        static_cast<MulticastReceiver *>(aContext)->HandleHostIp6Receive(*messagePtr);
    }

    void HandleHostIp6Receive(const Message &aMessage)
    {
        Ip6::Header header;

        VerifyOrQuit(aMessage.GetLength() <= Ip6::kMaxDatagramLength);
        SuccessOrQuit(header.ParseFrom(aMessage));
        VerifyOrQuit(header.IsValid());

        if (header.GetDestination() == mExpectedDst)
        {
            mHostRxCount++;

            if (header.GetNextHeader() == Ip6::kProtoFragment)
            {
                mHostFragRxCount++;
            }
        }
    }
};

static void ResetReceivers(MulticastReceiver  *aReceivers,
                           uint16_t            aCount,
                           const Ip6::Address &aExpectedDst,
                           uint8_t             aExpectedTag,
                           uint16_t            aExpectedPayloadLen)
{
    for (uint16_t i = 0; i < aCount; i++)
    {
        aReceivers[i].ResetExpectation(aExpectedDst, aExpectedTag, aExpectedPayloadLen);
    }
}

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
static void SendUdpMulticast(Node               &aSender,
                             otUdpSocket        &aSocket,
                             const Ip6::Address &aDestination,
                             uint16_t            aPayloadLen,
                             uint8_t             aTag,
                             bool                aMulticastLoop)
{
    static constexpr uint16_t kMaxPayloadBuf = 2000;

    uint8_t       payload[kMaxPayloadBuf];
    otMessage    *message;
    otMessageInfo messageInfo;

    VerifyOrQuit(aPayloadLen <= sizeof(payload));
    FillPayloadPattern(payload, aPayloadLen, aTag);

    message = otUdpNewMessage(&aSender, nullptr);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(otMessageAppend(message, payload, aPayloadLen));

    ClearAllBytes(messageInfo);
    messageInfo.mPeerAddr      = aDestination;
    messageInfo.mPeerPort      = kMulticastUdpPort;
    messageInfo.mMulticastLoop = aMulticastLoop;

    SuccessOrQuit(otUdpSend(&aSender, &aSocket, message, &messageInfo));
}
#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

static Message *BuildIp6FragmentMessage(Node               &aSender,
                                        const Ip6::Address &aSource,
                                        const Ip6::Address &aDestination,
                                        uint8_t             aNextHeader,
                                        const Message      &aUpperPayload,
                                        uint16_t            aOffsetBytes,
                                        uint16_t            aFragPayloadLen,
                                        bool                aMoreFragments,
                                        uint32_t            aIdentification,
                                        bool                aMulticastLoop)
{
    Message            *fragment;
    Ip6::Header         ip6Header;
    Ip6::FragmentHeader fragmentHeader;

    VerifyOrQuit((aOffsetBytes % 8) == 0);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(sizeof(fragmentHeader) + aFragPayloadLen);
    ip6Header.SetNextHeader(Ip6::kProtoFragment);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);

    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(aNextHeader);
    fragmentHeader.SetOffset(Ip6::FragmentHeader::BytesToFragmentOffset(aOffsetBytes));
    fragmentHeader.SetIdentification(aIdentification);

    if (aMoreFragments)
    {
        fragmentHeader.SetMoreFlag();
    }

    fragment = aSender.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(fragment != nullptr);
    SuccessOrQuit(fragment->SetPriority(Message::kPriorityNormal));
    SuccessOrQuit(fragment->Append(ip6Header));
    SuccessOrQuit(fragment->Append(fragmentHeader));
    SuccessOrQuit(fragment->AppendBytesFromMessage(aUpperPayload, aOffsetBytes, aFragPayloadLen));
    otMessageSetMulticastLoopEnabled(fragment, aMulticastLoop);

    return fragment;
}

static void SendRawFragmentedUdpMulticast(Core               &aNexus,
                                          Node               &aSender,
                                          const Ip6::Address &aSource,
                                          const Ip6::Address &aDestination,
                                          uint16_t            aPayloadLen,
                                          uint8_t             aTag,
                                          uint32_t            aIdentification,
                                          bool                aMulticastLoop)
{
    static constexpr uint16_t kMaxPayloadBuf = 2000;

    uint8_t        payload[kMaxPayloadBuf];
    Message       *udpMessage;
    Message       *frag1;
    Message       *frag2;
    Ip6::UdpHeader udpHeader;
    uint16_t       totalUdpLen;
    uint16_t       frag2Len;

    VerifyOrQuit(aPayloadLen <= sizeof(payload));
    FillPayloadPattern(payload, aPayloadLen, aTag);

    totalUdpLen = sizeof(udpHeader) + aPayloadLen;
    VerifyOrQuit(totalUdpLen > kRawFrag1PayloadSize);
    frag2Len = totalUdpLen - kRawFrag1PayloadSize;

    udpHeader.Clear();
    udpHeader.SetSourcePort(kMulticastSrcPort);
    udpHeader.SetDestinationPort(kMulticastUdpPort);
    udpHeader.SetLength(totalUdpLen);

    udpMessage = aSender.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(udpMessage != nullptr);
    SuccessOrQuit(udpMessage->Append(udpHeader));
    SuccessOrQuit(udpMessage->AppendBytes(payload, aPayloadLen));
    Checksum::UpdateMessageChecksum(*udpMessage, aSource, aDestination, Ip6::kProtoUdp);

    frag1 = BuildIp6FragmentMessage(aSender, aSource, aDestination, Ip6::kProtoUdp, *udpMessage, /* aOffsetBytes */ 0,
                                    kRawFrag1PayloadSize, /* aMoreFragments */ true, aIdentification, aMulticastLoop);
    SuccessOrQuit(otIp6Send(&aSender, frag1));

    // Space fragments beyond the MPL Trickle interval so multi-hop routers forward them in order.
    aNexus.AdvanceTime(kFragmentSpacingTime);

    frag2 = BuildIp6FragmentMessage(aSender, aSource, aDestination, Ip6::kProtoUdp, *udpMessage, kRawFrag1PayloadSize,
                                    frag2Len, /* aMoreFragments */ false, aIdentification, aMulticastLoop);
    SuccessOrQuit(otIp6Send(&aSender, frag2));

    udpMessage->Free();
}

void TestMulticastFragmentation(void)
{
    /**
     * Test IPv6 Multicast Fragmentation and Reassembly
     *
     * Topology:
     *   SED_Leader <-> Leader <-> Router1 <-> Router2 <-> MED
     *                                               \ <-> SED
     *
     * Validates locally originated (`otUdpSend` / `SendDatagram` -> `FragmentDatagram`)
     * and raw-injected (`otIp6Send` / `SendRaw`) fragmented multicast datagrams (> 1280 bytes)
     * for both realm-local (`ff03::1234`) and larger-than-realm-local (`ff05::1234`) scopes.
     */

    Core              nexus;
    Node             &leader    = nexus.CreateNode();
    Node             &router1   = nexus.CreateNode();
    Node             &router2   = nexus.CreateNode();
    Node             &med       = nexus.CreateNode();
    Node             &sed       = nexus.CreateNode();
    Node             &sedLeader = nexus.CreateNode();
    Ip6::Address      realmMcast;
    Ip6::Address      siteMcast;
    MulticastReceiver receivers[6];
    uint16_t          numReceivers = GetArrayLength(receivers);

    MulticastReceiver &leaderRx    = receivers[0];
    MulticastReceiver &router1Rx   = receivers[1];
    MulticastReceiver &router2Rx   = receivers[2];
    MulticastReceiver &medRx       = receivers[3];
    MulticastReceiver &sedRx       = receivers[4];
    MulticastReceiver &sedLeaderRx = receivers[5];

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med.SetName("MED");
    sed.SetName("SED");
    sedLeader.SetName("SED_LDR");

    SuccessOrQuit(realmMcast.FromString(kRealmLocalMcast));
    SuccessOrQuit(siteMcast.FromString(kSiteLocalMcast));

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Multicast Frag Step 0: Form multi-hop topology and subscribe all nodes to ff03::1234 and ff05::1234");

    AllowLinkBetween(leader, router1);
    AllowLinkBetween(leader, sedLeader);
    AllowLinkBetween(router1, router2);
    AllowLinkBetween(router2, med);
    AllowLinkBetween(router2, sed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    router2.Join(router1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    med.Join(router2, Node::kAsMed);
    sed.Join(router2, Node::kAsSed);
    sedLeader.Join(leader, Node::kAsSed);

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
    SuccessOrQuit(sedLeader.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));

    {
        NetworkData::OnMeshPrefixConfig prefixConfig;

        prefixConfig.Clear();
        SuccessOrQuit(prefixConfig.GetPrefix().FromString(kOnMeshPrefix));
        prefixConfig.mPreferred    = true;
        prefixConfig.mSlaac        = true;
        prefixConfig.mOnMesh       = true;
        prefixConfig.mDefaultRoute = true;
        prefixConfig.mStable       = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(prefixConfig));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sedLeader.Get<Mle::Mle>().IsChild());

    {
        Node *allNodes[] = {&leader, &router1, &router2, &med, &sed, &sedLeader};

        for (uint16_t i = 0; i < numReceivers; i++)
        {
            SuccessOrQuit(allNodes[i]->Get<ThreadNetif>().SubscribeExternalMulticast(realmMcast));
            SuccessOrQuit(allNodes[i]->Get<ThreadNetif>().SubscribeExternalMulticast(siteMcast));
            receivers[i].Init(*allNodes[i]);
        }
    }

    nexus.AdvanceTime(kPropagationTime);

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    Log("Multicast Frag Step 1a: otUdpSend 1500B to ff03::1234 from Leader (mMulticastLoop = false)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x21, kMulticastPayloadSize);
    SendUdpMulticast(leader, leaderRx.mUdpSocket, realmMcast, kMulticastPayloadSize, 0x21, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);

    Log("Multicast Frag Step 1b: otUdpSend 1500B to ff03::1234 from Leader (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x22, kMulticastPayloadSize);
    SendUdpMulticast(leader, leaderRx.mUdpSocket, realmMcast, kMulticastPayloadSize, 0x22, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);

    Log("Multicast Frag Step 1c: otUdpSend 1500B to ff03::1234 from Router2 (mMulticastLoop = false)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x23, kMulticastPayloadSize);
    SendUdpMulticast(router2, router2Rx.mUdpSocket, realmMcast, kMulticastPayloadSize, 0x23,
                     /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 1d: otUdpSend 1500B to ff03::1234 from Router2 (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x24, kMulticastPayloadSize);
    SendUdpMulticast(router2, router2Rx.mUdpSocket, realmMcast, kMulticastPayloadSize, 0x24, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 1e: otUdpSend 1233B (1281B IPv6 boundary) to ff03::1234 from Router2");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x25, kBoundaryPayloadSize);
    SendUdpMulticast(router2, router2Rx.mUdpSocket, realmMcast, kBoundaryPayloadSize, 0x25, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 2a: otUdpSend 1500B to ff05::1234 from Leader (mMulticastLoop = false)");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x31, kMulticastPayloadSize);
    SendUdpMulticast(leader, leaderRx.mUdpSocket, siteMcast, kMulticastPayloadSize, 0x31, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);

    Log("Multicast Frag Step 2b: otUdpSend 1500B to ff05::1234 from Leader (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x32, kMulticastPayloadSize);
    SendUdpMulticast(leader, leaderRx.mUdpSocket, siteMcast, kMulticastPayloadSize, 0x32, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);

    Log("Multicast Frag Step 2c: otUdpSend 1500B to ff05::1234 from Router2 (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x33, kMulticastPayloadSize);
    SendUdpMulticast(router2, router2Rx.mUdpSocket, siteMcast, kMulticastPayloadSize, 0x33, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 2d: otUdpSend 1233B (1281B IPv6 boundary) to ff05::1234 from Router2");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x34, kBoundaryPayloadSize);
    SendUdpMulticast(router2, router2Rx.mUdpSocket, siteMcast, kBoundaryPayloadSize, 0x34, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);
#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

    Log("Multicast Frag Step 3a: Multi-hop SendRaw 2-fragment 1500B to ff03::1234 (mMulticastLoop = false)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x41, kMulticastPayloadSize);
    SendRawFragmentedUdpMulticast(nexus, leader, leader.Get<Mle::Mle>().GetMeshLocalEid(), realmMcast,
                                  kMulticastPayloadSize, 0x41, 0x41000001, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);
#else
    VerifyOrQuit(router1Rx.mUdpRxCount == 0);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mUdpRxCount == 0);
    VerifyOrQuit(sedRx.mUdpRxCount == 0);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 0);
#endif
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 3b: Multi-hop SendRaw 2-fragment 1500B to ff03::1234 (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, realmMcast, 0x42, kMulticastPayloadSize);
    SendRawFragmentedUdpMulticast(nexus, leader, leader.Get<Mle::Mle>().GetMeshLocalEid(), realmMcast,
                                  kMulticastPayloadSize, 0x42, 0x42000002, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    VerifyOrQuit(leaderRx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);
#else
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 0);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mUdpRxCount == 0);
    VerifyOrQuit(sedRx.mUdpRxCount == 0);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);
#endif

    Log("Multicast Frag Step 3c: Multi-hop SendRaw 2-fragment 1500B to ff05::1234 (mMulticastLoop = false)");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x43, kMulticastPayloadSize);
    SendRawFragmentedUdpMulticast(nexus, leader, leader.FindGlobalAddress(), siteMcast, kMulticastPayloadSize, 0x43,
                                  0x43000003, /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);
#else
    VerifyOrQuit(router1Rx.mUdpRxCount == 0);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mUdpRxCount == 0);
    VerifyOrQuit(sedRx.mUdpRxCount == 0);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 0);
#endif
    VerifyOrQuit(medRx.mHostRxCount == 2);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);

    Log("Multicast Frag Step 3d: Multi-hop SendRaw 2-fragment 1500B to ff05::1234 (mMulticastLoop = true)");
    ResetReceivers(receivers, numReceivers, siteMcast, 0x44, kMulticastPayloadSize);
    SendRawFragmentedUdpMulticast(nexus, leader, leader.FindGlobalAddress(), siteMcast, kMulticastPayloadSize, 0x44,
                                  0x44000004, /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    VerifyOrQuit(leaderRx.mUdpRxCount == 1);
    VerifyOrQuit(router1Rx.mUdpRxCount == 1);
    VerifyOrQuit(router2Rx.mUdpRxCount == 1);
    VerifyOrQuit(medRx.mUdpRxCount == 1);
    VerifyOrQuit(sedRx.mUdpRxCount == 1);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);
#else
    VerifyOrQuit(leaderRx.mUdpRxCount == 0);
    VerifyOrQuit(router1Rx.mUdpRxCount == 0);
    VerifyOrQuit(router2Rx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mUdpRxCount == 0);
    VerifyOrQuit(sedRx.mUdpRxCount == 0);
    VerifyOrQuit(sedLeaderRx.mUdpRxCount == 0);
    VerifyOrQuit(medRx.mHostFragRxCount == 2);
#endif

    Log("Multicast Frag Step 4a: Subscribed router drops disallowed ICMPv6 multicast first fragment");
    {
        static const uint8_t kIcmpBody[8] = {0};

        const Ip6::Address &srcAddr = leader.Get<Mle::Mle>().GetMeshLocalEid();
        Ip6::Icmp6Header    icmpHeader;
        Message            *icmpMessage;
        Message            *frag;

        ResetReceivers(receivers, numReceivers, realmMcast, 0x51, 0);

        icmpHeader.Clear();
        icmpHeader.SetType(Ip6::Icmp6Header::kTypeNeighborSolicit);

        icmpMessage = leader.Get<MessagePool>().Allocate(Message::kTypeOther);
        VerifyOrQuit(icmpMessage != nullptr);
        SuccessOrQuit(icmpMessage->Append(icmpHeader));
        SuccessOrQuit(icmpMessage->AppendBytes(kIcmpBody, sizeof(kIcmpBody)));
        Checksum::UpdateMessageChecksum(*icmpMessage, srcAddr, realmMcast, Ip6::kProtoIcmp6);

        frag = BuildIp6FragmentMessage(leader, srcAddr, realmMcast, Ip6::kProtoIcmp6, *icmpMessage,
                                       /* aOffsetBytes */ 0, sizeof(icmpHeader) + sizeof(kIcmpBody),
                                       /* aMoreFragments */ true, 0x51000001, /* aMulticastLoop */ true);
        VerifyOrQuit(otIp6Send(&leader, frag) == OT_ERROR_DROP);
        icmpMessage->Free();

        nexus.AdvanceTime(kPropagationTime);
        VerifyOrQuit(medRx.mHostFragRxCount == 0);
    }

    Log("Multicast Frag Step 4b: Subscribed router forwards allowed ICMPv6 first fragment and 4B last fragment");
    {
        static constexpr uint16_t kFrag1Len        = 16;
        static constexpr uint16_t kShortFrag2Len   = 4;
        static const uint8_t      kEchoPayload[12] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                                      0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};

        const Ip6::Address &srcAddr = leader.Get<Mle::Mle>().GetMeshLocalEid();
        Ip6::Icmp6Header    icmpHeader;
        Message            *icmpMessage;
        Message            *frag1;
        Message            *frag2;

        ResetReceivers(receivers, numReceivers, realmMcast, 0x52, 0);

        icmpHeader.Clear();
        icmpHeader.SetType(Ip6::Icmp6Header::kTypeEchoRequest);

        icmpMessage = leader.Get<MessagePool>().Allocate(Message::kTypeOther);
        VerifyOrQuit(icmpMessage != nullptr);
        SuccessOrQuit(icmpMessage->Append(icmpHeader));
        SuccessOrQuit(icmpMessage->AppendBytes(kEchoPayload, sizeof(kEchoPayload)));
        Checksum::UpdateMessageChecksum(*icmpMessage, srcAddr, realmMcast, Ip6::kProtoIcmp6);

        frag1 = BuildIp6FragmentMessage(leader, srcAddr, realmMcast, Ip6::kProtoIcmp6, *icmpMessage,
                                        /* aOffsetBytes */ 0, kFrag1Len, /* aMoreFragments */ true, 0x52000002,
                                        /* aMulticastLoop */ true);
        SuccessOrQuit(otIp6Send(&leader, frag1));

        nexus.AdvanceTime(kFragmentSpacingTime);

        frag2 = BuildIp6FragmentMessage(leader, srcAddr, realmMcast, Ip6::kProtoIcmp6, *icmpMessage, kFrag1Len,
                                        kShortFrag2Len, /* aMoreFragments */ false, 0x52000002,
                                        /* aMulticastLoop */ true);
        SuccessOrQuit(otIp6Send(&leader, frag2));
        icmpMessage->Free();

        nexus.AdvanceTime(kPropagationTime);
        VerifyOrQuit(router1Rx.mHostFragRxCount == 2);
        VerifyOrQuit(medRx.mHostFragRxCount == 2);
    }

    Log("Multicast Frag Step 4c: Atomic fragment (offset = 0, M = 0) delivered locally and forwarded");
    {
        static constexpr uint16_t kAtomicPayloadSize = 32;

        const Ip6::Address &srcAddr = leader.Get<Mle::Mle>().GetMeshLocalEid();
        uint8_t             payload[kAtomicPayloadSize];
        Ip6::UdpHeader      udpHeader;
        Message            *udpMessage;
        Message            *atomicFrag;

        ResetReceivers(receivers, numReceivers, realmMcast, 0x53, kAtomicPayloadSize);
        FillPayloadPattern(payload, kAtomicPayloadSize, 0x53);

        udpHeader.Clear();
        udpHeader.SetSourcePort(kMulticastSrcPort);
        udpHeader.SetDestinationPort(kMulticastUdpPort);
        udpHeader.SetLength(sizeof(udpHeader) + kAtomicPayloadSize);

        udpMessage = leader.Get<MessagePool>().Allocate(Message::kTypeOther);
        VerifyOrQuit(udpMessage != nullptr);
        SuccessOrQuit(udpMessage->Append(udpHeader));
        SuccessOrQuit(udpMessage->AppendBytes(payload, kAtomicPayloadSize));
        Checksum::UpdateMessageChecksum(*udpMessage, srcAddr, realmMcast, Ip6::kProtoUdp);

        atomicFrag = BuildIp6FragmentMessage(leader, srcAddr, realmMcast, Ip6::kProtoUdp, *udpMessage,
                                             /* aOffsetBytes */ 0, sizeof(udpHeader) + kAtomicPayloadSize,
                                             /* aMoreFragments */ false, 0x53000003, /* aMulticastLoop */ true);
        SuccessOrQuit(otIp6Send(&leader, atomicFrag));
        udpMessage->Free();

        nexus.AdvanceTime(kPropagationTime);
        VerifyOrQuit(leaderRx.mUdpRxCount == 1);
        VerifyOrQuit(router1Rx.mUdpRxCount == 1);
        VerifyOrQuit(router2Rx.mUdpRxCount == 1);
        VerifyOrQuit(medRx.mUdpRxCount == 1);
        VerifyOrQuit(sedRx.mUdpRxCount == 1);
        VerifyOrQuit(sedLeaderRx.mUdpRxCount == 1);
    }

    for (uint16_t i = 0; i < numReceivers; i++)
    {
        receivers[i].Close();
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    ot::Nexus::TestIPv6Fragmentation();
#endif
    ot::Nexus::TestMulticastFragmentation();
    printf("All tests passed\n");
    return 0;
}
