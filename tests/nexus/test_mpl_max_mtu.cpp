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

#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/udp.h>

#include "mac/data_poll_sender.hpp"
#include "net/checksum.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kStabilizationTime  = 30 * 1000;
static constexpr uint32_t kPropagationTime    = 8 * 1000;
static constexpr uint32_t kSedPollPeriod      = 200;

static constexpr uint16_t kTestUdpPort = 12345;
static constexpr uint16_t kSrcUdpPort  = 54321;

// Exact UDP payload length that produces a 1280-byte IPv6 datagram (40B IPv6 + 8B UDP + 1232B = 1280B).
static constexpr uint16_t kMaxUnencapsulatedUdpPayload =
    Ip6::kMaxDatagramLength - sizeof(Ip6::Header) - sizeof(Ip6::UdpHeader);

// Exact UDP payload length when an 8-byte Hop-by-Hop header is already present in a 1280-byte raw IPv6 datagram.
static constexpr uint16_t kMaxPreHbhUdpPayload = kMaxUnencapsulatedUdpPayload - Ip6::ExtensionHeader::kLengthUnitSize;

static const char kOnMeshPrefix[]    = "2001:db8:1::/64";
static const char kExternalSrcAddr[] = "2001:db8:ffff::1";
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

struct NodeReceiver
{
    Node        *mNode;
    otUdpSocket  mUdpSocket;
    Ip6::Address mRealmLocalMcast;
    Ip6::Address mSiteLocalMcast;
    Ip6::Address mOffMeshAddr;
    uint8_t      mExpectedTag;
    uint16_t     mExpectedUdpLen;
    uint32_t     mUdpRxCount;
    uint32_t     mHostRxCount;
    uint32_t     mOffMeshHostRxCount;
    uint16_t     mLastHostLen;
    uint8_t      mLastHostNextHeader;
    Ip6::Address mLastHostSrc;
    Ip6::Address mLastHostDst;

    void Init(Node               &aNode,
              const Ip6::Address &aRealmMcast,
              const Ip6::Address &aSiteMcast,
              const Ip6::Address &aOffMeshAddr)
    {
        otSockAddr bindAddr;

        ClearAllBytes(*this);
        mNode            = &aNode;
        mRealmLocalMcast = aRealmMcast;
        mSiteLocalMcast  = aSiteMcast;
        mOffMeshAddr     = aOffMeshAddr;

        ClearAllBytes(bindAddr);
        bindAddr.mPort = kTestUdpPort;

        SuccessOrQuit(otUdpOpen(mNode, &mUdpSocket, HandleUdpReceive, this));
        SuccessOrQuit(otUdpBind(mNode, &mUdpSocket, &bindAddr, OT_NETIF_THREAD_INTERNAL));

        otIp6SetReceiveFilterEnabled(mNode, false);
        otIp6SetReceiveCallback(mNode, HandleHostIp6Receive, this);
    }

    void ResetExpectation(uint8_t aExpectedTag, uint16_t aExpectedUdpLen)
    {
        mExpectedTag        = aExpectedTag;
        mExpectedUdpLen     = aExpectedUdpLen;
        mUdpRxCount         = 0;
        mHostRxCount        = 0;
        mOffMeshHostRxCount = 0;
        mLastHostLen        = 0;
        mLastHostNextHeader = 0;
        mLastHostSrc.Clear();
        mLastHostDst.Clear();
    }

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
    {
        static_cast<NodeReceiver *>(aContext)->HandleUdpReceive(*AsCoreTypePtr(aMessage), *AsCoreTypePtr(aMessageInfo));
    }

    void HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        uint16_t payloadLen = aMessage.DetermineLengthAfterOffset();

        VerifyOrQuit((aMessageInfo.GetSockAddr() == mRealmLocalMcast) ||
                     (aMessageInfo.GetSockAddr() == mSiteLocalMcast) ||
                     mNode->Get<ThreadNetif>().HasUnicastAddress(aMessageInfo.GetSockAddr()));
        VerifyOrQuit(payloadLen == mExpectedUdpLen);

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

        static_cast<NodeReceiver *>(aContext)->HandleHostIp6Receive(*messagePtr);
    }

    void HandleHostIp6Receive(const Message &aMessage)
    {
        Ip6::Header header;

        // Every IPv6 packet delivered to the host MUST strictly obey standard 1280-byte MTU (`kMaxDatagramLength`).
        VerifyOrQuit(aMessage.GetLength() <= Ip6::kMaxDatagramLength);
        SuccessOrQuit(header.ParseFrom(aMessage));
        VerifyOrQuit(header.IsValid());

        if ((header.GetDestination() == mRealmLocalMcast) || (header.GetDestination() == mSiteLocalMcast))
        {
            if (mHostRxCount > 0)
            {
                VerifyOrQuit(aMessage.GetLength() == mLastHostLen);
                VerifyOrQuit(header.GetNextHeader() == mLastHostNextHeader);
                VerifyOrQuit(header.GetSource() == mLastHostSrc);
                VerifyOrQuit(header.GetDestination() == mLastHostDst);
            }

            mHostRxCount++;
            mLastHostLen        = aMessage.GetLength();
            mLastHostNextHeader = header.GetNextHeader();
            mLastHostSrc        = header.GetSource();
            mLastHostDst        = header.GetDestination();
        }
        else if (header.GetDestination() == mOffMeshAddr)
        {
            mOffMeshHostRxCount++;
            mLastHostLen        = aMessage.GetLength();
            mLastHostNextHeader = header.GetNextHeader();
            mLastHostSrc        = header.GetSource();
            mLastHostDst        = header.GetDestination();
        }
    }
};

static void ResetAllReceivers(NodeReceiver *aReceivers, uint16_t aCount, uint8_t aExpectedTag, uint16_t aExpectedUdpLen)
{
    for (uint16_t i = 0; i < aCount; i++)
    {
        aReceivers[i].ResetExpectation(aExpectedTag, aExpectedUdpLen);
    }
}

static Message *BuildRawUdpMulticastDatagram(Node               &aNode,
                                             const Ip6::Address &aSource,
                                             const Ip6::Address &aDestination,
                                             const uint8_t      *aExtHeaders,
                                             uint16_t            aExtHeadersLen,
                                             uint8_t             aIp6NextHeader,
                                             uint16_t            aUdpPayloadLen,
                                             uint8_t             aTag)
{
    static constexpr uint16_t kMaxPayloadBuf = 2100;

    uint8_t        payload[kMaxPayloadBuf];
    Message       *udpMsg;
    Message       *ip6Msg;
    Ip6::Header    ip6Header;
    Ip6::UdpHeader udpHeader;

    VerifyOrQuit(aUdpPayloadLen <= sizeof(payload));
    FillPayloadPattern(payload, aUdpPayloadLen, aTag);

    udpHeader.Clear();
    udpHeader.SetSourcePort(kSrcUdpPort);
    udpHeader.SetDestinationPort(kTestUdpPort);
    udpHeader.SetLength(sizeof(udpHeader) + aUdpPayloadLen);

    udpMsg = aNode.Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(udpMsg != nullptr);
    SuccessOrQuit(udpMsg->Append(udpHeader));
    SuccessOrQuit(udpMsg->AppendBytes(payload, aUdpPayloadLen));
    Checksum::UpdateMessageChecksum(*udpMsg, aSource, aDestination, Ip6::kProtoUdp);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(aExtHeadersLen + udpMsg->GetLength());
    ip6Header.SetNextHeader(aIp6NextHeader);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);

    ip6Msg = aNode.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(ip6Msg != nullptr);
    SuccessOrQuit(ip6Msg->Append(ip6Header));

    if (aExtHeadersLen > 0)
    {
        SuccessOrQuit(ip6Msg->AppendBytes(aExtHeaders, aExtHeadersLen));
    }

    SuccessOrQuit(ip6Msg->AppendBytesFromMessage(*udpMsg, 0, udpMsg->GetLength()));
    udpMsg->Free();

    return ip6Msg;
}

static void SendUdpSocketMulticast(Node               &aSender,
                                   otUdpSocket        &aSocket,
                                   const Ip6::Address &aDestination,
                                   uint16_t            aPayloadLen,
                                   uint8_t             aTag,
                                   bool                aMulticastLoop)
{
    static constexpr uint16_t kMaxPayloadBuf = 2100;

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
    messageInfo.mPeerPort      = kTestUdpPort;
    messageInfo.mMulticastLoop = aMulticastLoop;

    SuccessOrQuit(otUdpSend(&aSender, &aSocket, message, &messageInfo));
}

static void VerifyRemoteReceivers(const NodeReceiver *aRemoteReceivers,
                                  uint16_t            aRemoteCount,
                                  const Ip6::Address &aExpectedSrc,
                                  const Ip6::Address &aExpectedDst,
                                  uint8_t             aExpectedHostNextHeader,
                                  const Node         *aLoopbackSender = nullptr)
{
    for (uint16_t i = 0; i < aRemoteCount; i++)
    {
        const NodeReceiver &rx                        = aRemoteReceivers[i];
        bool                routerLoopsRealmLocalRetx = rx.mNode->Get<Mle::Mle>().IsRouterOrLeader() &&
                                         aExpectedDst.IsRealmLocalMulticast() &&
                                         !rx.mNode->Get<Mle::Mle>().IsMeshLocalAddress(aExpectedSrc);
        bool directSedOfLoopbackSender =
            (aLoopbackSender != nullptr) && aExpectedDst.IsMulticastLargerThanRealmLocal() &&
            !rx.mNode->Get<Mle::Mle>().IsRxOnWhenIdle() &&
            (rx.mNode->Get<Mle::Mle>().GetParent().GetExtAddress() == aLoopbackSender->Get<Mac::Mac>().GetExtAddress());

        if (directSedOfLoopbackSender)
        {
            VerifyOrQuit(rx.mUdpRxCount >= 1);
        }
        else
        {
            VerifyOrQuit(rx.mUdpRxCount == 1);
        }

        if (routerLoopsRealmLocalRetx || directSedOfLoopbackSender)
        {
            VerifyOrQuit(rx.mHostRxCount >= 1);
        }
        else
        {
            VerifyOrQuit(rx.mHostRxCount == 1);
        }

        VerifyOrQuit(rx.mLastHostLen == Ip6::kMaxDatagramLength);
        VerifyOrQuit(rx.mLastHostNextHeader == aExpectedHostNextHeader);
        VerifyOrQuit(rx.mLastHostDst == aExpectedDst);
        VerifyOrQuit(rx.mLastHostSrc == aExpectedSrc);
    }
}

void TestMplMaxMtu(void)
{
    /**
     * Multi-hop topology exercising MPL forwarding, IP-in-IP decapsulation, host delivery,
     * and indirect child transmission:
     *
     *   SED_Leader <-> Leader <-> Router1 <-> Router2 <-> MED
     *                                               \ <-> SED
     */

    Core          nexus;
    Node         &leader    = nexus.CreateNode();
    Node         &router1   = nexus.CreateNode();
    Node         &router2   = nexus.CreateNode();
    Node         &med       = nexus.CreateNode();
    Node         &sed       = nexus.CreateNode();
    Node         &sedLeader = nexus.CreateNode();
    Ip6::Address  realmMcast;
    Ip6::Address  siteMcast;
    Ip6::Address  externalSrc;
    NodeReceiver  receivers[6];
    NodeReceiver *remoteReceivers = &receivers[1];
    uint16_t      numReceivers    = GetArrayLength(receivers);
    uint16_t      numRemote       = numReceivers - 1;

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med.SetName("MED");
    sed.SetName("SED");
    sedLeader.SetName("SED_LDR");

    SuccessOrQuit(realmMcast.FromString(kRealmLocalMcast));
    SuccessOrQuit(siteMcast.FromString(kSiteLocalMcast));
    SuccessOrQuit(externalSrc.FromString(kExternalSrcAddr));

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

#if !OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    for (Node &node : nexus.GetNodes())
    {
        SuccessOrQuit(node.Get<Dns::Multicast::Core>().SetEnabled(true, 1));
    }
#endif

    Log("Step 0: Form multi-hop topology and subscribe all nodes to ff03::1234 and ff05::1234");

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

    // Configure a stable SLAAC GUA prefix so all nodes (including SEDs) acquire a global unicast address.
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

    Node *allNodes[] = {&leader, &router1, &router2, &med, &sed, &sedLeader};

    for (uint16_t i = 0; i < numReceivers; i++)
    {
        SuccessOrQuit(allNodes[i]->Get<ThreadNetif>().SubscribeExternalMulticast(realmMcast));
        SuccessOrQuit(allNodes[i]->Get<ThreadNetif>().SubscribeExternalMulticast(siteMcast));
        receivers[i].Init(*allNodes[i], realmMcast, siteMcast, externalSrc);
    }

    // Advance past Child Update Request multicast address registration so parents register SED subscriptions.
    nexus.AdvanceTime(kPropagationTime);

    //-----------------------------------------------------------------------------------------------------------------
    // Scenario 1: Raw IPv6 injection (`otIp6Send` / `Ip6::SendRaw`) of exact 1280-byte Realm-Local multicast.

    Log("Step 1a: SendRaw 1280B realm-local multicast with RLOC source (S=0 MPL + PadN -> 1288B on mesh)");
    {
        const Ip6::Address &srcRloc = leader.Get<Mle::Mle>().GetMeshLocalRloc();
        Message            *msg = BuildRawUdpMulticastDatagram(leader, srcRloc, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                                               kMaxUnencapsulatedUdpPayload, 0x11);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        ResetAllReceivers(receivers, numReceivers, 0x11, kMaxUnencapsulatedUdpPayload);
        SuccessOrQuit(otIp6Send(&leader, msg));
        nexus.AdvanceTime(kPropagationTime);

        VerifyRemoteReceivers(remoteReceivers, numRemote, srcRloc, realmMcast, Ip6::kProtoUdp);
    }

    Log("Step 1b: SendRaw 1280B realm-local multicast with ML-EID source (S=1 MPL -> 1288B on mesh)");
    {
        const Ip6::Address &srcMlEid = leader.Get<Mle::Mle>().GetMeshLocalEid();
        Message            *msg = BuildRawUdpMulticastDatagram(leader, srcMlEid, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                                               kMaxUnencapsulatedUdpPayload, 0x12);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        ResetAllReceivers(receivers, numReceivers, 0x12, kMaxUnencapsulatedUdpPayload);
        SuccessOrQuit(otIp6Send(&leader, msg));
        nexus.AdvanceTime(kPropagationTime);

        VerifyRemoteReceivers(remoteReceivers, numRemote, srcMlEid, realmMcast, Ip6::kProtoUdp);
    }

    Log("Step 1c: SendRaw 1280B realm-local multicast with GUA source (S=1 MPL -> 1288B on mesh)");
    {
        const Ip6::Address &srcGua = leader.FindGlobalAddress();
        Message            *msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                                               kMaxUnencapsulatedUdpPayload, 0x13);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        ResetAllReceivers(receivers, numReceivers, 0x13, kMaxUnencapsulatedUdpPayload);
        SuccessOrQuit(otIp6Send(&leader, msg));
        nexus.AdvanceTime(kPropagationTime);

        VerifyRemoteReceivers(remoteReceivers, numRemote, srcGua, realmMcast, Ip6::kProtoUdp);
    }

    Log("Step 1d: SendRaw 1280B realm-local multicast carrying pre-existing 8B HBH (expanded to 16B HBH -> 1288B)");
    {
        static const uint8_t kPreExistingSkipHbh[] = {Ip6::kProtoUdp, 0x00, 0x1e, 0x04, 0xde, 0xad, 0xbe, 0xef};

        const Ip6::Address &srcGua = leader.FindGlobalAddress();
        Message            *msg =
            BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, kPreExistingSkipHbh, sizeof(kPreExistingSkipHbh),
                                         Ip6::kProtoHopOpts, kMaxPreHbhUdpPayload, 0x14);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        ResetAllReceivers(receivers, numReceivers, 0x14, kMaxPreHbhUdpPayload);
        SuccessOrQuit(otIp6Send(&leader, msg));
        nexus.AdvanceTime(kPropagationTime);

        VerifyRemoteReceivers(remoteReceivers, numRemote, srcGua, realmMcast, Ip6::kProtoHopOpts);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Scenario 2: Raw IPv6 injection (`otIp6Send` / `Ip6::SendRaw`) of exact 1280-byte Larger-than-Realm-Local
    // multicast (`ff05::1234`), exercising 1328-byte IP-in-IP `ff03::fc` MPL encapsulation + sleepy-child clone.

    Log("Step 2: SendRaw 1280B site-local multicast from external GUA (IP-in-IP + MPL -> 1328B on mesh)");
    {
        Message *msg = BuildRawUdpMulticastDatagram(leader, externalSrc, siteMcast, nullptr, 0, Ip6::kProtoUdp,
                                                    kMaxUnencapsulatedUdpPayload, 0x21);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        ResetAllReceivers(receivers, numReceivers, 0x21, kMaxUnencapsulatedUdpPayload);
        SuccessOrQuit(otIp6Send(&leader, msg));
        nexus.AdvanceTime(kPropagationTime);

        VerifyRemoteReceivers(remoteReceivers, numRemote, externalSrc, siteMcast, Ip6::kProtoUdp);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Scenario 3: Native OpenThread `otUdpSend` (`Ip6::SendDatagram`) max-MTU multicast (1232B UDP -> 1280B IPv6).

    Log("Step 3a: otUdpSend 1232B UDP payload to ff03::1234 without MulticastLoop (1288B unfragmented on mesh)");
    ResetAllReceivers(receivers, numReceivers, 0x31, kMaxUnencapsulatedUdpPayload);
    SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, realmMcast, kMaxUnencapsulatedUdpPayload, 0x31,
                           /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(receivers[0].mUdpRxCount == 0);
    VerifyRemoteReceivers(remoteReceivers, numRemote, leader.Get<Mle::Mle>().GetMeshLocalEid(), realmMcast,
                          Ip6::kProtoUdp);

    Log("Step 3b: otUdpSend 1232B UDP payload to ff03::1234 with MulticastLoop=true");
    ResetAllReceivers(receivers, numReceivers, 0x32, kMaxUnencapsulatedUdpPayload);
    SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, realmMcast, kMaxUnencapsulatedUdpPayload, 0x32,
                           /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(receivers[0].mUdpRxCount == 1);
    VerifyRemoteReceivers(remoteReceivers, numRemote, leader.Get<Mle::Mle>().GetMeshLocalEid(), realmMcast,
                          Ip6::kProtoUdp);

    Log("Step 3c: otUdpSend 1232B UDP payload to ff05::1234 with MulticastLoop=false and MulticastLoop=true (1328B)");
    ResetAllReceivers(receivers, numReceivers, 0x33, kMaxUnencapsulatedUdpPayload);
    SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, siteMcast, kMaxUnencapsulatedUdpPayload, 0x33,
                           /* aMulticastLoop */ false);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(receivers[0].mUdpRxCount == 0);
    VerifyRemoteReceivers(remoteReceivers, numRemote, leader.FindGlobalAddress(), siteMcast, Ip6::kProtoUdp);

    ResetAllReceivers(receivers, numReceivers, 0x34, kMaxUnencapsulatedUdpPayload);
    SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, siteMcast, kMaxUnencapsulatedUdpPayload, 0x34,
                           /* aMulticastLoop */ true);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(receivers[0].mUdpRxCount == 1);
    VerifyRemoteReceivers(remoteReceivers, numRemote, leader.FindGlobalAddress(), siteMcast, Ip6::kProtoUdp, &leader);

    //-----------------------------------------------------------------------------------------------------------------
    // Scenario 4: Negative & Security Boundary Tests.

    Log("Step 4a: Verify PassToHost drops 1288B multicast when MPL is replaced with PadN instead of removed");
    {
        // 16-byte HBH where MplOption(S=1, seq=0x99, seedId=0xfe00) precedes a non-padding Skip option, causing
        // `RemoveMplOption()` to take `kReplaceMplWithPad` and leave the packet length at 1288 (> kMinimalMtu).
        static const uint8_t kMplBeforeSkipHbh[] = {
            Ip6::kProtoUdp, 0x01, Ip6::MplOption::kType,
            0x04,           0x40, 0x99,
            0xfe,           0x00, 0x1e,
            0x04,           0x11, 0x22,
            0x33,           0x44, 0x01,
            0x00,
        };
        static constexpr uint16_t kPadCaseUdpPayload = Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize -
                                                       sizeof(Ip6::Header) - sizeof(kMplBeforeSkipHbh) -
                                                       sizeof(Ip6::UdpHeader);

        const Ip6::Address &srcGua = leader.FindGlobalAddress();
        Message            *msg =
            BuildRawUdpMulticastDatagram(router2, srcGua, realmMcast, kMplBeforeSkipHbh, sizeof(kMplBeforeSkipHbh),
                                         Ip6::kProtoHopOpts, kPadCaseUdpPayload, 0x41);

        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        msg->SetOrigin(Message::kOriginThreadNetif);

        ResetAllReceivers(receivers, numReceivers, 0x41, kPadCaseUdpPayload);
        // Inject directly on MED (a non-forwarding subscriber) so we deterministically test PassToHost + Receive.
        IgnoreError(med.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)));
        nexus.AdvanceTime(100);

        // Internal UDP socket receives the 1224B payload, while PassToHost MUST drop the still-1288B datagram.
        VerifyOrQuit(receivers[3].mUdpRxCount == 1);
        VerifyOrQuit(receivers[3].mHostRxCount == 0);
    }

#if !OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    Log("Step 4b: Verify host ingress/egress, local send, decapsulation, and on-mesh ceiling boundaries");
    {
        static constexpr uint16_t kMaxMplEncapSize = sizeof(Ip6::Header) + Ip6::ExtensionHeader::kLengthUnitSize;
        static const uint8_t      kMplS1UdpHbh[]   = {Ip6::kProtoUdp, 0x00, Ip6::MplOption::kType, 0x04, 0x40, 0xa1,
                                                      0xfe,           0x01};
        static const uint8_t      kMplS0Ip6Hbh[]   = {Ip6::kProtoIp6, 0x00, Ip6::MplOption::kType, 0x02, 0x00, 0xa2,
                                                      0x01,           0x00};

        const Ip6::Address &srcGua     = leader.FindGlobalAddress();
        const Ip6::Address &router2Gua = router2.FindGlobalAddress();
        uint16_t            freeBuffers;
        uint8_t             rawBuf[Ip6::kMaxDatagramLength + 16];
        otMessage          *bufMsg;
        Message            *msg;

        // 1) `otIp6NewMessageFromBuffer` (`NewMessageFromData`): accepts 1280B and rejects 1281B (`nullptr`).
        freeBuffers = leader.Get<MessagePool>().GetFreeBufferCount();

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload, 0x42);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        SuccessOrQuit(msg->Read(0, rawBuf, Ip6::kMaxDatagramLength));
        msg->Free();

        bufMsg = otIp6NewMessageFromBuffer(&leader, rawBuf, Ip6::kMaxDatagramLength, nullptr);
        VerifyOrQuit(bufMsg != nullptr);
        otMessageFree(bufMsg);

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + 1, 0x42);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + 1);
        SuccessOrQuit(msg->Read(0, rawBuf, Ip6::kMaxDatagramLength + 1));
        msg->Free();

        VerifyOrQuit(otIp6NewMessageFromBuffer(&leader, rawBuf, Ip6::kMaxDatagramLength + 1, nullptr) == nullptr);
        VerifyOrQuit(leader.Get<MessagePool>().GetFreeBufferCount() == freeBuffers);

        // 2) `SendRaw` (`otIp6Send`): rejects 1281B (unicast & multicast) and 1288B with `kErrorParse`.
        ResetAllReceivers(receivers, numReceivers, 0x42, kMaxUnencapsulatedUdpPayload + 1);

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, router2Gua, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + 1, 0x42);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + 1);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + 1, 0x42);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        msg = BuildRawUdpMulticastDatagram(leader, externalSrc, siteMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + 1, 0x42);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize, 0x42);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        msg = BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, kMplS1UdpHbh, sizeof(kMplS1UdpHbh),
                                           Ip6::kProtoHopOpts, kMaxUnencapsulatedUdpPayload, 0x42);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        // 3) Local `SendDatagram` (`otUdpSend` and `otIcmp6SendEchoRequest`) producing 1288B (`1281..1328B`):
        // returns `kErrorNone`, frees the message without leaking buffers, and does NOT deliver or transmit it.
        {
            static constexpr uint16_t kOversizedUdpPayload =
                kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize;
            static constexpr uint16_t kOversizedIcmpPayload = Ip6::kMaxDatagramLength +
                                                              Ip6::ExtensionHeader::kLengthUnitSize -
                                                              sizeof(Ip6::Header) - sizeof(Ip6::Icmp6Header);

            uint8_t       icmpData[kOversizedIcmpPayload];
            otMessage    *echoMsg;
            otMessageInfo echoInfo;

            ResetAllReceivers(receivers, numReceivers, 0x43, kOversizedUdpPayload);
            freeBuffers = leader.Get<MessagePool>().GetFreeBufferCount();

            SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, router2Gua, kOversizedUdpPayload, 0x43, false);
            SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, realmMcast, kOversizedUdpPayload, 0x43, true);
            SendUdpSocketMulticast(leader, receivers[0].mUdpSocket, siteMcast, kOversizedUdpPayload, 0x43, true);

            FillPayloadPattern(icmpData, sizeof(icmpData), 0x43);

            echoMsg = otIp6NewMessage(&leader, nullptr);
            VerifyOrQuit(echoMsg != nullptr);
            SuccessOrQuit(otMessageAppend(echoMsg, icmpData, sizeof(icmpData)));
            ClearAllBytes(echoInfo);
            echoInfo.mPeerAddr = router2Gua;
            SuccessOrQuit(otIcmp6SendEchoRequest(&leader, echoMsg, &echoInfo, 1));

            echoMsg = otIp6NewMessage(&leader, nullptr);
            VerifyOrQuit(echoMsg != nullptr);
            SuccessOrQuit(otMessageAppend(echoMsg, icmpData, sizeof(icmpData)));
            ClearAllBytes(echoInfo);
            echoInfo.mPeerAddr      = realmMcast;
            echoInfo.mMulticastLoop = true;
            SuccessOrQuit(otIcmp6SendEchoRequest(&leader, echoMsg, &echoInfo, 2));

            // Verify immediate message release with zero leaked buffers and zero transmitted/delivered packets.
            VerifyOrQuit(leader.Get<MessagePool>().GetFreeBufferCount() == freeBuffers);

            nexus.AdvanceTime(kPropagationTime);
            VerifyOrQuit(leader.Get<MessagePool>().GetFreeBufferCount() == freeBuffers);

            for (uint16_t i = 0; i < numReceivers; i++)
            {
                VerifyOrQuit(receivers[i].mUdpRxCount == 0);
                VerifyOrQuit(receivers[i].mHostRxCount == 0);
            }
        }

        // 4) `HandleDatagram` accepts 1328B (`kMaxDatagramLength + kMaxMplEncapSize`) and rejects 1329B
        // (`kErrorParse`).
        {
            Message    *innerMsg = BuildRawUdpMulticastDatagram(med, externalSrc, siteMcast, nullptr, 0, Ip6::kProtoUdp,
                                                                kMaxUnencapsulatedUdpPayload, 0x44);
            Message    *outerMsg = med.Get<MessagePool>().Allocate(Message::kTypeIp6);
            Ip6::Header outerHdr;

            VerifyOrQuit(outerMsg != nullptr);
            outerHdr.Clear();
            outerHdr.InitVersionTrafficClassFlow();
            outerHdr.SetPayloadLength(sizeof(kMplS0Ip6Hbh) + innerMsg->GetLength());
            outerHdr.SetNextHeader(Ip6::kProtoHopOpts);
            outerHdr.SetHopLimit(Ip6::kDefaultHopLimit);
            outerHdr.SetSource(leader.Get<Mle::Mle>().GetMeshLocalRloc());
            outerHdr.SetDestination(Ip6::Address::GetRealmLocalAllMplForwarders());

            SuccessOrQuit(outerMsg->Append(outerHdr));
            SuccessOrQuit(outerMsg->AppendBytes(kMplS0Ip6Hbh, sizeof(kMplS0Ip6Hbh)));
            SuccessOrQuit(outerMsg->AppendBytesFromMessage(*innerMsg, 0, innerMsg->GetLength()));
            innerMsg->Free();

            VerifyOrQuit(outerMsg->GetLength() == Ip6::kMaxDatagramLength + kMaxMplEncapSize);
            outerMsg->SetOrigin(Message::kOriginThreadNetif);

            ResetAllReceivers(receivers, numReceivers, 0x44, kMaxUnencapsulatedUdpPayload);
            SuccessOrQuit(med.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(outerMsg)));
            VerifyOrQuit(receivers[3].mUdpRxCount == 1);
            VerifyOrQuit(receivers[3].mHostRxCount == 1);
            VerifyOrQuit(receivers[3].mLastHostLen == Ip6::kMaxDatagramLength);
        }

        ResetAllReceivers(receivers, numReceivers, 0x45, kMaxUnencapsulatedUdpPayload + 1);

        // Plain 1329B packet -> rejected with `kErrorParse`.
        msg = BuildRawUdpMulticastDatagram(router2, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + kMaxMplEncapSize + 1, 0x45);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + kMaxMplEncapSize + 1);
        msg->SetOrigin(Message::kOriginThreadNetif);
        VerifyOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)) == kErrorParse);

        // 1329B `ff03::fc` IP-in-IP MPL packet wrapping a 1281B inner datagram -> rejected with `kErrorParse`.
        {
            Message    *innerMsg = BuildRawUdpMulticastDatagram(router2, externalSrc, siteMcast, nullptr, 0,
                                                                Ip6::kProtoUdp, kMaxUnencapsulatedUdpPayload + 1, 0x45);
            Message    *outerMsg = router2.Get<MessagePool>().Allocate(Message::kTypeIp6);
            Ip6::Header outerHdr;

            VerifyOrQuit(outerMsg != nullptr);
            outerHdr.Clear();
            outerHdr.InitVersionTrafficClassFlow();
            outerHdr.SetPayloadLength(sizeof(kMplS0Ip6Hbh) + innerMsg->GetLength());
            outerHdr.SetNextHeader(Ip6::kProtoHopOpts);
            outerHdr.SetHopLimit(Ip6::kDefaultHopLimit);
            outerHdr.SetSource(leader.Get<Mle::Mle>().GetMeshLocalRloc());
            outerHdr.SetDestination(Ip6::Address::GetRealmLocalAllMplForwarders());

            SuccessOrQuit(outerMsg->Append(outerHdr));
            SuccessOrQuit(outerMsg->AppendBytes(kMplS0Ip6Hbh, sizeof(kMplS0Ip6Hbh)));
            SuccessOrQuit(outerMsg->AppendBytesFromMessage(*innerMsg, 0, innerMsg->GetLength()));
            innerMsg->Free();

            VerifyOrQuit(outerMsg->GetLength() == Ip6::kMaxDatagramLength + kMaxMplEncapSize + 1);
            outerMsg->SetOrigin(Message::kOriginThreadNetif);
            VerifyOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(outerMsg)) == kErrorParse);
        }

        // 5) 1328B `ff03::fc` IP-in-IP packet (40B outer header with `NextHeader = kProtoIp6`, no HBH) wrapping a
        // 1288B inner IPv6 datagram: outer `ParseFrom` passes, inner `aRecursionDepth == 1` `header.IsValid()` fails
        // with `kErrorParse` and is neither delivered locally nor passed to the host!
        {
            Message *innerMsg = BuildRawUdpMulticastDatagram(
                router2, externalSrc, siteMcast, nullptr, 0, Ip6::kProtoUdp,
                kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize, 0x45);
            Message    *outerMsg = router2.Get<MessagePool>().Allocate(Message::kTypeIp6);
            Ip6::Header outerHdr;

            VerifyOrQuit(innerMsg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
            VerifyOrQuit(outerMsg != nullptr);

            outerHdr.Clear();
            outerHdr.InitVersionTrafficClassFlow();
            outerHdr.SetPayloadLength(innerMsg->GetLength());
            outerHdr.SetNextHeader(Ip6::kProtoIp6);
            outerHdr.SetHopLimit(1);
            outerHdr.SetSource(leader.Get<Mle::Mle>().GetMeshLocalRloc());
            outerHdr.SetDestination(Ip6::Address::GetRealmLocalAllMplForwarders());

            SuccessOrQuit(outerMsg->Append(outerHdr));
            SuccessOrQuit(outerMsg->AppendBytesFromMessage(*innerMsg, 0, innerMsg->GetLength()));
            innerMsg->Free();

            VerifyOrQuit(outerMsg->GetLength() == Ip6::kMaxDatagramLength + kMaxMplEncapSize);
            outerMsg->SetOrigin(Message::kOriginThreadNetif);
            VerifyOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(outerMsg)) == kErrorParse);
        }

        // Also check direct `aRecursionDepth == 1` with 1281B -> `kErrorParse`.
        msg = BuildRawUdpMulticastDatagram(router2, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + 1, 0x45);
        msg->SetOrigin(Message::kOriginThreadNetif);
        VerifyOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg), /* aIsReassembled */ false,
                                                            /* aRecursionDepth */ 1) == kErrorParse);

        nexus.AdvanceTime(kPropagationTime);

        for (uint16_t i = 0; i < numReceivers; i++)
        {
            VerifyOrQuit(receivers[i].mUdpRxCount == 0);
            VerifyOrQuit(receivers[i].mHostRxCount == 0);
            VerifyOrQuit(receivers[i].mOffMeshHostRxCount == 0);
        }

        // 6) On-mesh unencapsulated unicast arriving at BR (`leader`, advertising default route) targeting off-mesh
        // `externalSrc`: 1280B succeeds and is emitted to host; 1288B is dropped by `PassToHost` (`kMinimalMtu`) both
        // on direct BR arrival (`kErrorDrop`) and across multi-hop forwarding (`router2 -> router1 -> leader`).
        ResetAllReceivers(receivers, numReceivers, 0x46, kMaxUnencapsulatedUdpPayload);

        msg = BuildRawUdpMulticastDatagram(router2, router2Gua, externalSrc, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload, 0x46);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength);
        msg->SetOrigin(Message::kOriginThreadNetif);
        SuccessOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)));
        nexus.AdvanceTime(kPropagationTime);
        VerifyOrQuit(receivers[0].mOffMeshHostRxCount == 1);
        VerifyOrQuit(receivers[0].mLastHostLen == Ip6::kMaxDatagramLength);

        ResetAllReceivers(receivers, numReceivers, 0x47,
                          kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize);

        msg = BuildRawUdpMulticastDatagram(leader, router2Gua, externalSrc, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize, 0x47);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        msg->SetOrigin(Message::kOriginThreadNetif);
        VerifyOrQuit(leader.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)) == kErrorDrop);
        VerifyOrQuit(receivers[0].mOffMeshHostRxCount == 0);

        msg = BuildRawUdpMulticastDatagram(router2, router2Gua, externalSrc, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize, 0x47);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        msg->SetOrigin(Message::kOriginThreadNetif);
        SuccessOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)));
        nexus.AdvanceTime(kPropagationTime);

        for (uint16_t i = 0; i < numReceivers; i++)
        {
            VerifyOrQuit(receivers[i].mUdpRxCount == 0);
            VerifyOrQuit(receivers[i].mHostRxCount == 0);
            VerifyOrQuit(receivers[i].mOffMeshHostRxCount == 0);
        }
    }
#else
    Log("Step 4b: Verify host egress kMinimalMtu drop and rejection above kMaxAssembledDatagramLength");
    {
        static const uint8_t      kMplS1UdpHbh[] = {Ip6::kProtoUdp, 0x00, Ip6::MplOption::kType, 0x04, 0x40, 0xa3,
                                                    0xfe,           0x02};
        static constexpr uint16_t kOverAssembled =
            Ip6::kMaxAssembledDatagramLength + 1 - sizeof(Ip6::Header) - sizeof(Ip6::UdpHeader);
        static constexpr uint16_t kOverAssembledHbh = kOverAssembled - sizeof(kMplS1UdpHbh);

        const Ip6::Address &srcGua     = leader.FindGlobalAddress();
        const Ip6::Address &router2Gua = router2.FindGlobalAddress();
        uint8_t             rawBuf[Ip6::kMaxAssembledDatagramLength + 8];
        Message            *msg;

        ResetAllReceivers(receivers, numReceivers, 0x48, kOverAssembled);

        msg =
            BuildRawUdpMulticastDatagram(leader, srcGua, realmMcast, nullptr, 0, Ip6::kProtoUdp, kOverAssembled, 0x48);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxAssembledDatagramLength + 1);
        SuccessOrQuit(msg->Read(0, rawBuf, Ip6::kMaxAssembledDatagramLength + 1));
        VerifyOrQuit(otIp6NewMessageFromBuffer(&leader, rawBuf, Ip6::kMaxAssembledDatagramLength + 1, nullptr) ==
                     nullptr);
        VerifyOrQuit(otIp6Send(&leader, msg) == OT_ERROR_PARSE);

        msg = BuildRawUdpMulticastDatagram(router2, srcGua, realmMcast, kMplS1UdpHbh, sizeof(kMplS1UdpHbh),
                                           Ip6::kProtoHopOpts, kOverAssembledHbh, 0x48);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxAssembledDatagramLength + 1);
        msg->SetOrigin(Message::kOriginThreadNetif);
        VerifyOrQuit(router2.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)) == kErrorParse);

        // Even with IP6 fragmentation enabled, BR `PassToHost` must still drop unencapsulated >1280B unicast packets.
        msg = BuildRawUdpMulticastDatagram(leader, router2Gua, externalSrc, nullptr, 0, Ip6::kProtoUdp,
                                           kMaxUnencapsulatedUdpPayload + Ip6::ExtensionHeader::kLengthUnitSize, 0x48);
        VerifyOrQuit(msg->GetLength() == Ip6::kMaxDatagramLength + Ip6::ExtensionHeader::kLengthUnitSize);
        msg->SetOrigin(Message::kOriginThreadNetif);
        VerifyOrQuit(leader.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(msg)) == kErrorDrop);

        nexus.AdvanceTime(kPropagationTime);

        for (uint16_t i = 0; i < numReceivers; i++)
        {
            VerifyOrQuit(receivers[i].mUdpRxCount == 0);
            VerifyOrQuit(receivers[i].mHostRxCount == 0);
            VerifyOrQuit(receivers[i].mOffMeshHostRxCount == 0);
        }
    }
#endif

    for (uint16_t i = 0; i < numReceivers; i++)
    {
        SuccessOrQuit(otUdpClose(allNodes[i], &receivers[i].mUdpSocket));
        otIp6SetReceiveCallback(allNodes[i], Node::HandleIp6Receive, allNodes[i]);
    }

    nexus.SaveTestInfo("test_mpl_max_mtu.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMplMaxMtu();
    printf("All tests passed\n");
    return 0;
}
