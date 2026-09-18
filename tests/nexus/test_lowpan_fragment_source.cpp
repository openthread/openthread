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
#include <vector>

#include <openthread/platform/radio.h>

#include "coap/coap.hpp"
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

static uint32_t AddChecksumBytes(uint32_t aSum, const uint8_t *aData, size_t aLength)
{
    for (size_t index = 0; index + 1 < aLength; index += 2)
    {
        aSum += (static_cast<uint16_t>(aData[index]) << 8) | aData[index + 1];
    }

    if (aLength & 1)
    {
        aSum += static_cast<uint16_t>(aData[aLength - 1]) << 8;
    }

    return aSum;
}

static uint16_t ComputeUdpChecksum(const Ip6::Address   &aSource,
                                   const Ip6::Address   &aDestination,
                                   const Ip6::UdpHeader &aHeader,
                                   const uint8_t        *aPayload,
                                   uint16_t              aPayloadLength)
{
    uint32_t sum       = 0;
    uint32_t udpLength = sizeof(Ip6::UdpHeader) + aPayloadLength;
    uint8_t  tail[8]   = {static_cast<uint8_t>(udpLength >> 24),
                          static_cast<uint8_t>(udpLength >> 16),
                          static_cast<uint8_t>(udpLength >> 8),
                          static_cast<uint8_t>(udpLength),
                          0,
                          0,
                          0,
                          Ip6::kProtoUdp};

    sum = AddChecksumBytes(sum, aSource.GetBytes(), sizeof(otIp6Address));
    sum = AddChecksumBytes(sum, aDestination.GetBytes(), sizeof(otIp6Address));
    sum = AddChecksumBytes(sum, tail, sizeof(tail));
    sum = AddChecksumBytes(sum, reinterpret_cast<const uint8_t *>(&aHeader), sizeof(aHeader));
    sum = AddChecksumBytes(sum, aPayload, aPayloadLength);

    while (sum >> 16)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    uint16_t checksum = static_cast<uint16_t>(~sum);

    return checksum ? checksum : 0xffff;
}

static std::vector<uint8_t> BuildDiagnosticGetPayload(Node &aNode)
{
    Coap::Message *coap = aNode.Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriDiagnosticGetRequest);

    VerifyOrQuit(coap != nullptr);

    std::vector<uint8_t> types(kLargeTypeCount, kUnknownDiagType);
    types[0] = NetDiag::Tlv::kVersion;
    SuccessOrQuit(Tlv::Append<NetDiag::TypeListTlv>(*coap, types.data(), kLargeTypeCount));

    std::vector<uint8_t> bytes(coap->GetLength());
    VerifyOrQuit(coap->ReadBytes(0, bytes.data(), coap->GetLength()) == coap->GetLength());
    coap->Free();

    return bytes;
}

static Message *BuildIpMessage(Node                       &aOwner,
                               const Ip6::Address         &aSource,
                               const Ip6::Address         &aDestination,
                               const std::vector<uint8_t> &aPayload)
{
    Message *message = aOwner.Get<MessagePool>().Allocate(Message::kTypeIp6);

    VerifyOrQuit(message != nullptr);

    Ip6::Header ip6Header;
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetPayloadLength(sizeof(Ip6::UdpHeader) + aPayload.size());
    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetHopLimit(Ip6::kDefaultHopLimit);
    ip6Header.SetSource(aSource);
    ip6Header.SetDestination(aDestination);

    Ip6::UdpHeader udpHeader;
    udpHeader.Clear();
    udpHeader.SetSourcePort(Tmf::kUdpPort);
    udpHeader.SetDestinationPort(Tmf::kUdpPort);
    udpHeader.SetLength(sizeof(Ip6::UdpHeader) + aPayload.size());
    udpHeader.SetChecksum(0);
    udpHeader.SetChecksum(ComputeUdpChecksum(aSource, aDestination, udpHeader, aPayload.data(), aPayload.size()));

    SuccessOrQuit(message->Append(ip6Header));
    SuccessOrQuit(message->Append(udpHeader));
    SuccessOrQuit(message->AppendBytes(aPayload.data(), aPayload.size()));
    message->SetLinkSecurityEnabled(true);
    message->SetOffset(0);

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

    std::vector<uint8_t> payload = BuildDiagnosticGetPayload(*relay);
    const Ip6::Address  &source  = originA->Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address  &dest    = receiver->Get<Mle::Mle>().GetMeshLocalRloc();
    OwnedPtr<Message>    message(BuildIpMessage(*relay, source, dest, payload));
    Mac::Addresses       addresses       = MakeRelayAddresses(*relay, *receiver);
    uint16_t             meshSource      = originA->Get<Mac::Mac>().GetShortAddress();
    uint16_t             meshDestination = receiver->Get<Mac::Mac>().GetShortAddress();
    uint32_t             txBefore        = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;
    uint32_t             fragmentCount =
        SendAllFragments(nexus, *relay, *receiver, *message, addresses, meshSource, meshDestination);
    uint32_t txAfter = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;

    VerifyOrQuit(fragmentCount > 1);
    VerifyOrQuit(txAfter > txBefore);
}

static void TestDifferentOriginatorRejected(void)
{
    // A forwarded fragment is authenticated by the immediate relay. The Mesh Header originator still identifies
    // the fragmented datagram source, so changing it between FRAG1 and FRAGN must not complete reassembly.
    Core  nexus;
    Node *receiver;
    Node *relay;
    Node *originA;
    Node *originB;

    InitializeNodes(nexus, receiver, relay, originA, originB);

    std::vector<uint8_t> payload = BuildDiagnosticGetPayload(*relay);
    const Ip6::Address  &source  = originA->Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address  &dest    = receiver->Get<Mle::Mle>().GetMeshLocalRloc();
    OwnedPtr<Message>    firstMessage(BuildIpMessage(*relay, source, dest, payload));
    OwnedPtr<Message>    nextMessage(BuildIpMessage(*relay, source, dest, payload));
    Mac::Addresses       addresses       = MakeRelayAddresses(*relay, *receiver);
    uint16_t             originAShort    = originA->Get<Mac::Mac>().GetShortAddress();
    uint16_t             originBShort    = originB->Get<Mac::Mac>().GetShortAddress();
    uint16_t             meshDestination = receiver->Get<Mac::Mac>().GetShortAddress();
    uint32_t             txBefore        = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;

    uint16_t nextOffset =
        PrepareAndDeliverMesh(*relay, *receiver, *firstMessage, addresses, originAShort, meshDestination);

    VerifyOrQuit(nextOffset < firstMessage->GetLength());

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

    uint32_t txAfter = receiver->Get<Mac::Mac>().GetCounters().mTxTotal;

    VerifyOrQuit(originAShort != originBShort);
    VerifyOrQuit(fragmentCount > 1);
    VerifyOrQuit(txAfter == txBefore);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSameOriginatorAccepted();
    ot::Nexus::TestDifferentOriginatorRejected();
    printf("All tests passed\n");
    return 0;
}
