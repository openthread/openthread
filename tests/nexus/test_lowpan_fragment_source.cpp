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

#include <openthread/platform/radio.h>

#include "coap/coap.hpp"
#include "net/checksum.hpp"
#include "net/ip6_headers.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/message_framer.hpp"
#include "thread/net_diag_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

static constexpr uint8_t kLargeTypeCount  = 180;
static constexpr uint8_t kUnknownDiagType = 0xfe;

static Coap::Message *BuildDiagnosticGetPayload(Node &aNode)
{
    Coap::Message *message = aNode.Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriDiagnosticGetRequest);
    uint8_t        types[kLargeTypeCount];

    VerifyOrQuit(message != nullptr);

    for (uint8_t &type : types)
    {
        type = kUnknownDiagType;
    }

    types[0] = NetDiag::Tlv::kVersion;
    SuccessOrQuit(Tlv::Append<NetDiag::TypeListTlv>(*message, types, kLargeTypeCount));

    return message;
}

static Message *BuildIpMessage(Node               &aOwner,
                               const Ip6::Address &aSource,
                               const Ip6::Address &aDestination,
                               const Message      &aPayload)
{
    Message *message = aOwner.Get<MessagePool>().Allocate(Message::kTypeIp6);

    VerifyOrQuit(message != nullptr);

    uint16_t payloadLength = aPayload.GetLength();

    Ip6::Header ip6Header;
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + payloadLength));
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);

    Ip6::UdpHeader udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(Tmf::kUdpPort);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(static_cast<uint16_t>(sizeof(Ip6::UdpHeader) + payloadLength));
    udpHeader.SetChecksum(0);

    SuccessOrQuit(message->Append(ip6Header));
    SuccessOrQuit(message->Append(udpHeader));
    SuccessOrQuit(message->AppendBytesFromMessage(aPayload, 0, payloadLength));

    message->SetOffset(sizeof(Ip6::Header));
    Checksum::UpdateMessageChecksum(*message, aSource, aDestination, Ip6::kProtoUdp);
    message->SetOffset(0);
    message->SetLinkSecurityEnabled(true);

    return message;
}

static uint16_t PrepareAndDeliverMesh(Node                 &aRelay,
                                      Node                 &aReceiver,
                                      Message              &aMessage,
                                      const Mac::Addresses &aMacAddresses,
                                      uint16_t              aMeshSource,
                                      uint16_t              aMeshDestination)
{
    Radio::Frame frame;
    uint16_t     nextOffset =
        aRelay.Get<MessageFramer>().PrepareFrame(frame, aMessage, aMacAddresses, true, aMeshSource, aMeshDestination);

    VerifyOrQuit(nextOffset > aMessage.GetOffset());
    SuccessOrQuit(otMacFrameProcessTxSfd(&frame, Core::Get().GetNowMicro64(), &aRelay.mRadio.mRadioContext));
    frame.UpdateFcs();

    Radio::Frame rxFrame(frame);
    rxFrame.mInfo.mRxInfo.mTimestamp = Core::Get().GetNowMicro64();
    rxFrame.mInfo.mRxInfo.mRssi      = -20;
    rxFrame.mInfo.mRxInfo.mLqi       = 255;
    otPlatRadioReceiveDone(&aReceiver.GetInstance(), &rxFrame, kErrorNone);

    return nextOffset;
}

static uint16_t PrepareAndDeliverDirect(Node                 &aSender,
                                        Node                 &aReceiver,
                                        Message              &aMessage,
                                        const Mac::Addresses &aMacAddresses)
{
    Radio::Frame frame;
    uint16_t     nextOffset = aSender.Get<MessageFramer>().PrepareFrame(frame, aMessage, aMacAddresses);

    VerifyOrQuit(nextOffset > aMessage.GetOffset());
    SuccessOrQuit(otMacFrameProcessTxSfd(&frame, Core::Get().GetNowMicro64(), &aSender.mRadio.mRadioContext));
    frame.UpdateFcs();

    Radio::Frame rxFrame(frame);
    rxFrame.mInfo.mRxInfo.mTimestamp = Core::Get().GetNowMicro64();
    rxFrame.mInfo.mRxInfo.mRssi      = -20;
    rxFrame.mInfo.mRxInfo.mLqi       = 255;
    otPlatRadioReceiveDone(&aReceiver.GetInstance(), &rxFrame, kErrorNone);

    return nextOffset;
}

static void InitializeNodes(Core &aNexus, Node *&aReceiver, Node *&aRelay, Node *&aOriginA, Node *&aOriginB)
{
    aReceiver = &aNexus.CreateNode();
    aRelay    = &aNexus.CreateNode();
    aOriginA  = &aNexus.CreateNode();
    aOriginB  = &aNexus.CreateNode();

    aReceiver->Form();
    aNexus.AdvanceTime(15 * 1000);
    aRelay->Join(*aReceiver, Node::kAsFtd);
    aOriginA->Join(*aReceiver, Node::kAsFtd);
    aOriginB->Join(*aReceiver, Node::kAsFtd);
    aNexus.AdvanceTime(120 * 1000);

    VerifyOrQuit(aReceiver->Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(aRelay->Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(aOriginA->Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(aOriginB->Get<Mle::Mle>().IsAttached());
}

static Mac::Addresses MakeRelayAddresses(Node &aRelay, Node &aReceiver)
{
    Mac::Addresses addresses;

    addresses.mSource.SetShort(aRelay.Get<Mac::Mac>().GetShortAddress());
    addresses.mDestination.SetShort(aReceiver.Get<Mac::Mac>().GetShortAddress());

    return addresses;
}

static uint32_t SendAllFragments(Core                 &aNexus,
                                 Node                 &aRelay,
                                 Node                 &aReceiver,
                                 Message              &aMessage,
                                 const Mac::Addresses &aMacAddresses,
                                 uint16_t              aMeshSource,
                                 uint16_t              aMeshDestination)
{
    uint32_t fragmentCount = 0;

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t nextOffset =
            PrepareAndDeliverMesh(aRelay, aReceiver, aMessage, aMacAddresses, aMeshSource, aMeshDestination);

        aMessage.SetOffset(nextOffset);
        fragmentCount++;
    }

    aNexus.AdvanceTime(50);
    return fragmentCount;
}

static void TestSameOriginatorAccepted(void)
{
    Core  nexus;
    Node *receiver;
    Node *relay;
    Node *originA;
    Node *originB;

    InitializeNodes(nexus, receiver, relay, originA, originB);
    OT_UNUSED_VARIABLE(originB);

    OwnedPtr<Coap::Message> payload(BuildDiagnosticGetPayload(*relay));
    const Ip6::Address  &source  = originA->Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address  &dest    = receiver->Get<Mle::Mle>().GetMeshLocalRloc();
    OwnedPtr<Message>    message(BuildIpMessage(*relay, source, dest, *payload));
    Mac::Addresses       addresses       = MakeRelayAddresses(*relay, *receiver);
    uint16_t             meshSource      = originA->Get<Mac::Mac>().GetShortAddress();
    uint16_t             meshDestination = receiver->Get<Mac::Mac>().GetShortAddress();
    uint32_t             txBefore        = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;
    uint32_t             rxBefore        = receiver->Get<MeshForwarder>().GetCounters().mRxSuccess;
    uint32_t             fragmentCount =
        SendAllFragments(nexus, *relay, *receiver, *message, addresses, meshSource, meshDestination);
    uint32_t txAfter = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;
    uint32_t rxAfter = receiver->Get<MeshForwarder>().GetCounters().mRxSuccess;

    VerifyOrQuit(fragmentCount > 1);
    VerifyOrQuit(txAfter > txBefore);
    VerifyOrQuit(rxAfter > rxBefore);
}

static void TestDifferentOriginatorRejected(void)
{
    Core  nexus;
    Node *receiver;
    Node *relay;
    Node *originA;
    Node *originB;

    InitializeNodes(nexus, receiver, relay, originA, originB);

    OwnedPtr<Coap::Message> payload(BuildDiagnosticGetPayload(*relay));
    const Ip6::Address  &source  = originA->Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address  &dest    = receiver->Get<Mle::Mle>().GetMeshLocalRloc();
    OwnedPtr<Message>    firstMessage(BuildIpMessage(*relay, source, dest, *payload));
    OwnedPtr<Message>    nextMessage(BuildIpMessage(*relay, source, dest, *payload));
    Mac::Addresses       addresses       = MakeRelayAddresses(*relay, *receiver);
    uint16_t             originAShort    = originA->Get<Mac::Mac>().GetShortAddress();
    uint16_t             originBShort    = originB->Get<Mac::Mac>().GetShortAddress();
    uint16_t             meshDestination = receiver->Get<Mac::Mac>().GetShortAddress();
    uint32_t             txBefore        = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;
    uint32_t             rxBefore        = receiver->Get<MeshForwarder>().GetCounters().mRxSuccess;

    uint16_t nextOffset =
        PrepareAndDeliverMesh(*relay, *receiver, *firstMessage, addresses, originAShort, meshDestination);

    VerifyOrQuit(nextOffset < firstMessage->GetLength());
    firstMessage->SetOffset(nextOffset);

    nextMessage->SetDatagramTag(firstMessage->GetDatagramTag());
    nextMessage->SetOffset(nextOffset);

    uint32_t fragmentCount = 1;

    while (nextMessage->GetOffset() < nextMessage->GetLength())
    {
        nextOffset = PrepareAndDeliverMesh(*relay, *receiver, *nextMessage, addresses, originBShort, meshDestination);
        nextMessage->SetOffset(nextOffset);
        fragmentCount++;
    }

    nexus.AdvanceTime(50);

    VerifyOrQuit(originAShort != originBShort);
    VerifyOrQuit(fragmentCount > 1);
    VerifyOrQuit(receiver->Get<Mac::Mac>().GetCounters().mTxTotal == txBefore);
    VerifyOrQuit(receiver->Get<MeshForwarder>().GetCounters().mRxSuccess == rxBefore);

    while (firstMessage->GetOffset() < firstMessage->GetLength())
    {
        nextOffset = PrepareAndDeliverMesh(*relay, *receiver, *firstMessage, addresses, originAShort, meshDestination);
        firstMessage->SetOffset(nextOffset);
    }

    nexus.AdvanceTime(50);

    VerifyOrQuit(receiver->Get<Mac::Mac>().GetCounters().mTxTotal > txBefore);
    VerifyOrQuit(receiver->Get<MeshForwarder>().GetCounters().mRxSuccess > rxBefore);
}

static void TestAddressTransitionAccepted(void)
{
    Core  nexus;
    Node *receiver;
    Node *relay;
    Node *originA;
    Node *originB;

    InitializeNodes(nexus, receiver, relay, originA, originB);
    OT_UNUSED_VARIABLE(relay);
    OT_UNUSED_VARIABLE(originB);

    OwnedPtr<Coap::Message> payload(BuildDiagnosticGetPayload(*originA));
    const Ip6::Address     &source = originA->Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address     &dest   = receiver->Get<Mle::Mle>().GetMeshLocalRloc();
    OwnedPtr<Message>       firstMessage(BuildIpMessage(*originA, source, dest, *payload));
    OwnedPtr<Message>       nextMessage(BuildIpMessage(*originA, source, dest, *payload));
    Mac::Addresses          extendedAddresses;
    Mac::Addresses          shortAddresses;
    uint16_t                originAShort = originA->Get<Mac::Mac>().GetShortAddress();
    uint16_t                receiverShort = receiver->Get<Mac::Mac>().GetShortAddress();
    uint32_t                rxBefore = receiver->Get<MeshForwarder>().GetCounters().mRxSuccess;

    extendedAddresses.mSource.SetExtended(originA->Get<Mac::Mac>().GetExtAddress());
    extendedAddresses.mDestination.SetShort(receiverShort);
    shortAddresses.mSource.SetShort(originAShort);
    shortAddresses.mDestination.SetShort(receiverShort);

    uint16_t nextOffset = PrepareAndDeliverDirect(*originA, *receiver, *firstMessage, extendedAddresses);

    VerifyOrQuit(nextOffset < firstMessage->GetLength());
    firstMessage->SetOffset(nextOffset);

    nextMessage->SetDatagramTag(firstMessage->GetDatagramTag());
    nextMessage->SetOffset(nextOffset);

    while (nextMessage->GetOffset() < nextMessage->GetLength())
    {
        nextOffset = PrepareAndDeliverDirect(*originA, *receiver, *nextMessage, shortAddresses);
        nextMessage->SetOffset(nextOffset);
    }

    nexus.AdvanceTime(50);

    VerifyOrQuit(receiver->Get<MeshForwarder>().GetCounters().mRxSuccess > rxBefore);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSameOriginatorAccepted();
    ot::Nexus::TestDifferentOriginatorRejected();
    ot::Nexus::TestAddressTransitionAccepted();
    printf("All tests passed\n");
    return 0;
}
