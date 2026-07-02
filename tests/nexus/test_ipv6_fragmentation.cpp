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

#include <openthread/error.h>
#include <openthread/message.h>
#include <openthread/platform/radio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "utils/history_tracker.hpp"

namespace ot {
namespace Nexus {

/** Time to advance for a node to form a network and become leader, in milliseconds. */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/** Time to advance for a node to join as a child and upgrade to a router, in milliseconds. */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/** Time to wait for ICMPv6 Echo response, in milliseconds. */
static constexpr uint32_t kEchoTimeout = 10 * 1000;

/** Time to wait for IPv6 fragmentation TX to complete or fail, in milliseconds. */
static constexpr uint32_t kFragTxTimeout = 10 * 1000;

/** ICMP payload size that requires at least three IPv6 fragments. */
static constexpr uint16_t kMultiFragmentPayloadSize = 2600;

static bool     gForwardedPacketReceived = false;
static uint16_t gForwardedPacketLen      = 0;

struct IcmpResponseContext
{
    explicit IcmpResponseContext(uint16_t aIdentifier)
        : mIdentifier(aIdentifier)
        , mResponseReceived(false)
    {
    }

    uint16_t mIdentifier;
    bool     mResponseReceived;
};

struct Ip6FragTxDoneContext
{
    Ip6FragTxDoneContext(void)
        : mParentMessage(nullptr)
        , mInvoked(false)
        , mError(OT_ERROR_FAILED)
        , mInvokeCount(0)
    {
    }

    otMessage *mParentMessage;
    bool       mInvoked;
    otError    mError;
    uint32_t   mInvokeCount;
};

static void HandleIcmpResponse(void *aContext, otMessage *, const otMessageInfo *, const otIcmp6Header *aIcmpHeader)
{
    IcmpResponseContext    *context = static_cast<IcmpResponseContext *>(aContext);
    const Ip6::Icmp6Header *header  = AsCoreTypePtr(aIcmpHeader);

    if ((header->GetType() == Ip6::Icmp6Header::kTypeEchoReply) && (header->GetId() == context->mIdentifier))
    {
        context->mResponseReceived = true;
    }
}

static void HandleIp6FragTxDone(const otMessage *aMessage, otError aError, void *aContext)
{
    Ip6FragTxDoneContext *context = static_cast<Ip6FragTxDoneContext *>(aContext);

    VerifyOrQuit(context != nullptr);

    if (context->mParentMessage != nullptr)
    {
        VerifyOrQuit(aMessage == context->mParentMessage, "TX done callback invoked on a fragment message");
        VerifyOrQuit(AsCoreType(aMessage).GetSubType() != Message::kSubTypeIp6Fragment,
                     "TX done callback invoked on a fragment message");
    }

    context->mInvoked = true;
    context->mError   = aError;
    context->mInvokeCount++;
}

static void SendEchoRequestWithOptionalTxDoneCallback(Node               &aSender,
                                                      const Ip6::Address &aDestination,
                                                      uint16_t            aIdentifier,
                                                      uint16_t            aPayloadSize,
                                                      uint8_t             aHopLimit,
                                                      otMessageTxCallback aCallback,
                                                      void               *aCallbackContext)
{
    Message         *message = nullptr;
    Ip6::MessageInfo messageInfo;

    message = aSender.Get<Ip6::Icmp>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->SetLength(aPayloadSize));

    if (aCallback != nullptr)
    {
        otMessageRegisterTxCallback(message, aCallback, aCallbackContext);

        if (aCallbackContext != nullptr)
        {
            static_cast<Ip6FragTxDoneContext *>(aCallbackContext)->mParentMessage = message;
        }
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetHopLimit(aHopLimit);
    messageInfo.mAllowZeroHopLimit = true;

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, aIdentifier));
}

static void SendAndVerifyEchoWithTxDoneCallback(Core                 &aNexus,
                                                Node                 &aSender,
                                                const Ip6::Address   &aDestination,
                                                uint16_t              aIdentifier,
                                                uint16_t              aPayloadSize,
                                                uint8_t               aHopLimit,
                                                Ip6FragTxDoneContext &aTxDoneContext,
                                                bool                  aRegisterTxDoneCallback,
                                                bool                  aExpectTxDoneInvoked,
                                                bool                  aExpectTxDoneSuccess,
                                                bool                  aExpectEchoReply,
                                                uint32_t              aWaitTime)
{
    IcmpResponseContext icmpContext(aIdentifier);
    Ip6::Icmp::Handler  icmpHandler(HandleIcmpResponse, &icmpContext);

    aTxDoneContext.mInvoked     = false;
    aTxDoneContext.mError       = OT_ERROR_FAILED;
    aTxDoneContext.mInvokeCount = 0;

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    SendEchoRequestWithOptionalTxDoneCallback(aSender, aDestination, aIdentifier, aPayloadSize, aHopLimit,
                                              aRegisterTxDoneCallback ? HandleIp6FragTxDone : nullptr, &aTxDoneContext);

    aNexus.AdvanceTime(aWaitTime);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    if (aExpectTxDoneInvoked)
    {
        VerifyOrQuit(aTxDoneContext.mInvoked, "Message TX callback was not invoked");
        VerifyOrQuit(aTxDoneContext.mInvokeCount == 1, "Message TX callback invoked more than once");

        if (aExpectTxDoneSuccess)
        {
            VerifyOrQuit(aTxDoneContext.mError == OT_ERROR_NONE, "Expected successful message TX callback");
        }
        else
        {
            VerifyOrQuit(aTxDoneContext.mError != OT_ERROR_NONE, "Expected failed message TX callback");
        }
    }
    else
    {
        VerifyOrQuit(!aTxDoneContext.mInvoked, "Message TX callback invoked unexpectedly");
        VerifyOrQuit(aTxDoneContext.mInvokeCount == 0, "Message TX callback invoked unexpectedly");
    }

    if (aExpectEchoReply)
    {
        VerifyOrQuit(icmpContext.mResponseReceived, "Expected Echo Reply was not received");
    }
    else
    {
        VerifyOrQuit(!icmpContext.mResponseReceived, "Received unexpected Echo Reply");
    }
}

static uint32_t CountIp6FragmentMeshTx(const Node &aNode, const Ip6::Address &aDestination, bool aTxSuccess)
{
    HistoryTracker::Iterator           iter;
    uint32_t                           age;
    uint32_t                           count = 0;
    const HistoryTracker::MessageInfo *info;

    iter.Init();

    while ((info = aNode.Get<HistoryTracker::Local>().IterateTxHistory(iter, age)) != nullptr)
    {
        if ((info->mIpProto == Ip6::kProtoFragment) && (AsCoreType(&info->mDestination.mAddress) == aDestination) &&
            (info->mTxSuccess == aTxSuccess))
        {
            count++;
        }
    }

    return count;
}

static void SendAndVerifyThreeFragAbortOnSecondNoAck(Core                 &aNexus,
                                                     Node                 &aSender,
                                                     Node                 &aReceiver,
                                                     const Ip6::Address   &aDestination,
                                                     uint16_t              aIdentifier,
                                                     uint16_t              aPayloadSize,
                                                     uint8_t               aHopLimit,
                                                     Ip6FragTxDoneContext &aTxDoneContext,
                                                     uint32_t              aWaitTime)
{
    IcmpResponseContext icmpContext(aIdentifier);
    Ip6::Icmp::Handler  icmpHandler(HandleIcmpResponse, &icmpContext);
    const uint32_t      baselineSuccessFragTx = CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ true);
    const uint32_t      baselineFailedFragTx  = CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ false);
    const uint32_t      baselineFragTx        = baselineSuccessFragTx + baselineFailedFragTx;
    bool                routerAsleep          = false;
    uint32_t            elapsed               = 0;

    aTxDoneContext.mParentMessage = nullptr;
    aTxDoneContext.mInvoked       = false;
    aTxDoneContext.mError         = OT_ERROR_FAILED;
    aTxDoneContext.mInvokeCount   = 0;

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    SendEchoRequestWithOptionalTxDoneCallback(aSender, aDestination, aIdentifier, aPayloadSize, aHopLimit,
                                              HandleIp6FragTxDone, &aTxDoneContext);

    while (!aTxDoneContext.mInvoked && (elapsed < aWaitTime))
    {
        const uint32_t fragTxCount = CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ true) +
                                     CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ false);

        if (!routerAsleep && ((fragTxCount - baselineFragTx) >= 1))
        {
            SuccessOrQuit(otPlatRadioSleep(&aReceiver.GetInstance()));
            routerAsleep = true;
        }

        aNexus.AdvanceTime(1);
        elapsed++;
    }

    if (routerAsleep)
    {
        SuccessOrQuit(otPlatRadioReceive(&aReceiver.GetInstance(), aReceiver.Get<Mac::Mac>().GetPanChannel()));
    }

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(aTxDoneContext.mInvoked, "Message TX callback was not invoked");
    VerifyOrQuit(aTxDoneContext.mInvokeCount == 1, "Message TX callback invoked more than once");
    VerifyOrQuit(aTxDoneContext.mError == OT_ERROR_NO_ACK, "Expected OT_ERROR_NO_ACK on second fragment failure");
    VerifyOrQuit(!icmpContext.mResponseReceived, "Received unexpected Echo Reply");

    {
        const uint32_t successFragTxDelta =
            CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ true) - baselineSuccessFragTx;
        const uint32_t failedFragTxDelta =
            CountIp6FragmentMeshTx(aSender, aDestination, /* aTxSuccess */ false) - baselineFailedFragTx;

        VerifyOrQuit(successFragTxDelta == 1, "Expected one successful IPv6 fragment mesh TX");
        VerifyOrQuit(failedFragTxDelta == 1, "Expected one failed IPv6 fragment mesh TX");
        VerifyOrQuit((successFragTxDelta + failedFragTxDelta) == 2,
                     "Expected exactly two IPv6 fragment mesh TX attempts");
        VerifyOrQuit((successFragTxDelta + failedFragTxDelta) < 3, "Third IPv6 fragment was transmitted");
    }
}

static void SendAndVerifyConcurrentFragTx(Core                 &aNexus,
                                          Node                 &aSender,
                                          Node                 &aReceiver,
                                          Ip6FragTxDoneContext &aTxDoneContext1,
                                          Ip6FragTxDoneContext &aTxDoneContext2,
                                          uint32_t              aWaitTime)
{
    static constexpr uint16_t kPingId1 = 0x7890;
    static constexpr uint16_t kPingId2 = 0x7891;

    IcmpResponseContext icmp1(kPingId1);
    IcmpResponseContext icmp2(kPingId2);
    Ip6::Icmp::Handler  icmpHandler1(HandleIcmpResponse, &icmp1);
    Ip6::Icmp::Handler  icmpHandler2(HandleIcmpResponse, &icmp2);

    aTxDoneContext1.mParentMessage = nullptr;
    aTxDoneContext1.mInvoked       = false;
    aTxDoneContext1.mError         = OT_ERROR_FAILED;
    aTxDoneContext1.mInvokeCount   = 0;

    aTxDoneContext2.mParentMessage = nullptr;
    aTxDoneContext2.mInvoked       = false;
    aTxDoneContext2.mError         = OT_ERROR_FAILED;
    aTxDoneContext2.mInvokeCount   = 0;

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler1));
    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler2));

    // Start both fragmented datagrams before either completes.
    SendEchoRequestWithOptionalTxDoneCallback(aSender, aReceiver.Get<Mle::Mle>().GetMeshLocalEid(), kPingId1,
                                              kMultiFragmentPayloadSize, 64, HandleIp6FragTxDone, &aTxDoneContext1);
    SendEchoRequestWithOptionalTxDoneCallback(aSender, aReceiver.Get<Mle::Mle>().GetMeshLocalEid(), kPingId2, 1952, 64,
                                              HandleIp6FragTxDone, &aTxDoneContext2);

    aNexus.AdvanceTime(aWaitTime);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler1));
    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler2));

    VerifyOrQuit(aTxDoneContext1.mInvoked, "First concurrent message TX callback was not invoked");
    VerifyOrQuit(aTxDoneContext1.mInvokeCount == 1, "First concurrent message TX callback invoked more than once");
    VerifyOrQuit(aTxDoneContext1.mError == OT_ERROR_NONE, "First concurrent message TX callback failed");

    VerifyOrQuit(aTxDoneContext2.mInvoked, "Second concurrent message TX callback was not invoked");
    VerifyOrQuit(aTxDoneContext2.mInvokeCount == 1, "Second concurrent message TX callback invoked more than once");
    VerifyOrQuit(aTxDoneContext2.mError == OT_ERROR_NONE, "Second concurrent message TX callback failed");

    VerifyOrQuit(aTxDoneContext1.mParentMessage != nullptr);
    VerifyOrQuit(aTxDoneContext2.mParentMessage != nullptr);
    VerifyOrQuit(aTxDoneContext1.mParentMessage != aTxDoneContext2.mParentMessage,
                 "Concurrent fragmented sends reused the same parent message");

    VerifyOrQuit(icmp1.mResponseReceived, "Did not receive Echo Reply for first concurrent ping");
    VerifyOrQuit(icmp2.mResponseReceived, "Did not receive Echo Reply for second concurrent ping");
}

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
    outerHeader.SetPayloadLength(48); // 8 bytes (Fragment Header) + 40 bytes (Inner IPv6 Header)
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

    // 3. Inner IPv6 Header (Payload of Fragment 1)
    Ip6::Header innerHeader;
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetPayloadLength(600); // 600 bytes inner payload to stay under MTU limit
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetHopLimit(64);
    innerHeader.SetSource(aLeaderEid);
    innerHeader.SetDestination(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    SuccessOrQuit(message->Append(innerHeader));

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
     * Validates IPv6 fragmentation TX, reassembly, TX done callbacks, and negative cases.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    Ip6FragTxDoneContext txDoneContext;
    Ip6FragTxDoneContext concurrentTxDoneContext1;
    Ip6FragTxDoneContext concurrentTxDoneContext2;

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

    Log("Step 4: Message TX callback on fragmented datagram success");
    SendAndVerifyEchoWithTxDoneCallback(nexus, leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0x2345, 1952, 64,
                                        txDoneContext, /* aRegisterTxDoneCallback */ true,
                                        /* aExpectTxDoneInvoked */ true,
                                        /* aExpectTxDoneSuccess */ true,
                                        /* aExpectEchoReply */ true, kFragTxTimeout);

    Log("Step 5: Message TX callback on non-fragmented datagram");
    SendAndVerifyEchoWithTxDoneCallback(nexus, leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0x3456, 32, 64,
                                        txDoneContext, /* aRegisterTxDoneCallback */ true,
                                        /* aExpectTxDoneInvoked */ true,
                                        /* aExpectTxDoneSuccess */ true,
                                        /* aExpectEchoReply */ true, kEchoTimeout);

    Log("Step 6: Multi-fragment echo request (3+ IPv6 fragments)");
    SendAndVerifyEchoWithTxDoneCallback(nexus, leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0x4567,
                                        kMultiFragmentPayloadSize, 64, txDoneContext,
                                        /* aRegisterTxDoneCallback */ true,
                                        /* aExpectTxDoneInvoked */ true,
                                        /* aExpectTxDoneSuccess */ true,
                                        /* aExpectEchoReply */ true, kFragTxTimeout);

    Log("Step 7: Concurrent fragmented echo requests from same node");
    SendAndVerifyConcurrentFragTx(nexus, leader, router, concurrentTxDoneContext1, concurrentTxDoneContext2,
                                  kFragTxTimeout);

    Log("Step 8: Second IPv6 fragment NO_ACK aborts remaining fragments");
    SendAndVerifyThreeFragAbortOnSecondNoAck(nexus, leader, router, router.Get<Mle::Mle>().GetMeshLocalEid(), 0x6789,
                                             kMultiFragmentPayloadSize, 64, txDoneContext, kFragTxTimeout);

    Log("Step 9: TX failure aborts fragmented transmission on first fragment");
    SuccessOrQuit(otPlatRadioSleep(&router.GetInstance()));
    SendAndVerifyEchoWithTxDoneCallback(nexus, leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0x5678, 1952, 64,
                                        txDoneContext, /* aRegisterTxDoneCallback */ true,
                                        /* aExpectTxDoneInvoked */ true,
                                        /* aExpectTxDoneSuccess */ false,
                                        /* aExpectEchoReply */ false, kFragTxTimeout);
    SuccessOrQuit(otPlatRadioReceive(&router.GetInstance(), router.Get<Mac::Mac>().GetPanChannel()));

    Log("Step 10: Fragment reassembly gap check");
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

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIPv6Fragmentation();
    printf("All tests passed\n");
    return 0;
}
