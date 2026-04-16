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

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

using Manager = MeshCoP::BorderAgent::Manager;

struct InspectorResponseContext : public Clearable<InspectorResponseContext>
{
    void Clear(void)
    {
        mReceived = false;
        mError    = kErrorNone;
        mResponse.Free();
    }

    bool              mReceived;
    Error             mError;
    OwnedPtr<Message> mResponse;
};

void HandleInspectorResponse(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    OT_UNUSED_VARIABLE(aMsg);

    InspectorResponseContext *context = static_cast<InspectorResponseContext *>(aContext);

    VerifyOrQuit(context != nullptr);
    context->mReceived = true;
    context->mError    = aResult;

    if (aResult == kErrorNone)
    {
        Log("  Received Inspector response");

        VerifyOrQuit(aMsg != nullptr);
        context->mResponse.Reset(aMsg->mMessage.Clone<kNoReservedHeader>());
        VerifyOrQuit(context->mResponse != nullptr);
    }
    else
    {
        Log("  Inspector response error: %s", ErrorToString(aResult));
    }
}

struct InspectorResourceContext : public Clearable<InspectorResourceContext>
{
    void Clear(void)
    {
        mAnswerCount = 0;
        mAnswers.DequeueAndFreeAll();
    }

    uint16_t     mAnswerCount;
    MessageQueue mAnswers;
};

bool HandleInspectorResource(void *aContext, Uri aUri, Coap::Msg &aMsg)
{
    OT_UNUSED_VARIABLE(aMsg);

    Message *answer;

    InspectorResourceContext *context   = static_cast<InspectorResourceContext *>(aContext);
    bool                      didHandle = false;

    VerifyOrQuit(context != nullptr);

    switch (aUri)
    {
    case kUriDiagnosticGetAnswer:
        answer = aMsg.mMessage.Clone<kNoReservedHeader>();
        VerifyOrQuit(answer != nullptr);
        context->mAnswers.Enqueue(*answer);
        context->mAnswerCount++;
        Log("  Received `DiagnosticGetAnswer` (%u)", context->mAnswerCount);
        didHandle = true;
        break;

    default:
        break;
    }

    return didHandle;
}

void TestBorderAgentInspector(void)
{
    static const uint8_t kTlvTypes[] = {NetworkDiagnostic::Tlv::kAddress16, NetworkDiagnostic::Tlv::kMode};

    static constexpr uint8_t kManylvTypes[] = {
        NetworkDiagnostic::Tlv::kExtMacAddress,
        NetworkDiagnostic::Tlv::kAddress16,
        NetworkDiagnostic::Tlv::kMode,
        NetworkDiagnostic::Tlv::kConnectivity,
        NetworkDiagnostic::Tlv::kRoute,
        NetworkDiagnostic::Tlv::kLeaderData,
        NetworkDiagnostic::Tlv::kNetworkData,
        NetworkDiagnostic::Tlv::kIp6AddressList,
        NetworkDiagnostic::Tlv::kMacCounters,
        NetworkDiagnostic::Tlv::kChildTable,
        NetworkDiagnostic::Tlv::kChannelPages,
        NetworkDiagnostic::Tlv::kMaxChildTimeout,
        NetworkDiagnostic::Tlv::kEui64,
        NetworkDiagnostic::Tlv::kVersion,
        NetworkDiagnostic::Tlv::kVendorName,
        NetworkDiagnostic::Tlv::kVendorModel,
        NetworkDiagnostic::Tlv::kVendorSwVersion,
        NetworkDiagnostic::Tlv::kThreadStackVersion,
        NetworkDiagnostic::Tlv::kChild,
        NetworkDiagnostic::Tlv::kChildIp6AddressList,
        NetworkDiagnostic::Tlv::kRouterNeighbor,
        NetworkDiagnostic::Tlv::kMleCounters,
        NetworkDiagnostic::Tlv::kVendorAppUrl,
        NetworkDiagnostic::Tlv::kEnhancedRoute,
    };

    static constexpr uint16_t kNumChildren = 16;

    Core                     nexus;
    Node                    &node0 = nexus.CreateNode(); // Border Agent
    Node                    &node1 = nexus.CreateNode(); // Inspector
    Node                    *children[kNumChildren];
    Ip6::SockAddr            sockAddr;
    Pskc                     pskc;
    Coap::Message           *message;
    InspectorResponseContext responseContext;
    InspectorResourceContext resourceContext;
    Tlv::Info                tlvInfo;
    uint16_t                 numTlvs;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAgentInspector");

    nexus.AdvanceTime(0);

    // Form the topology: node0 leader acting as Border Agent
    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    for (Node *&child : children)
    {
        child = &nexus.CreateNode();
        child->Join(node0, Node::kAsMed);
        nexus.AdvanceTime(5 * Time::kOneSecondInMsec);
        VerifyOrQuit(child->Get<Mle::Mle>().IsChild());
    }

    node1.Get<Mac::Mac>().SetPanId(node0.Get<Mac::Mac>().GetPanId());
    node1.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection to Border Agent");

    sockAddr.SetAddress(node0.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(node0.Get<Manager>().GetUdpPort());

    node0.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Open(0));
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Diagnostic Get Request` from Inspector");

    message = node1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriDiagnosticGetRequest);
    VerifyOrQuit(message != nullptr);

    // Append some requested TLV types (Address16, Mode)

    SuccessOrQuit(Tlv::Append<NetworkDiagnostic::TypeListTlv>(*message, kTlvTypes, sizeof(kTlvTypes)));

    responseContext.Clear();
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message, HandleInspectorResponse, &responseContext));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the received `Diagnostic Get Request` response by Inspector");

    VerifyOrQuit(responseContext.mReceived);
    SuccessOrQuit(responseContext.mError);

    for (uint8_t tlvType : kTlvTypes)
    {
        SuccessOrQuit(tlvInfo.FindIn(*responseContext.mResponse, tlvType));
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `Inspector Keep Alive` and check timeout behavior");

    // Wait for 30 seconds (keep-alive is usually 50s for commissioner, same for inspector session)
    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    message = node1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriInspectorKeepAlive);
    VerifyOrQuit(message != nullptr);

    responseContext.Clear();
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message, HandleInspectorResponse, &responseContext));

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    VerifyOrQuit(responseContext.mReceived);
    SuccessOrQuit(responseContext.mError);

    Log("  Wait for another 45 seconds and validate session is still active");
    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    Log("  Wait for additional 10 seconds and validate session is timed out");
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);
    VerifyOrQuit(!node1.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish session again and test `Diagnostic Get Query` (multiple answers)");

    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Tmf::SecureAgent>().IsConnected());

    resourceContext.Clear();
    node1.Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleInspectorResource, &resourceContext);

    message = node1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriDiagnosticGetQuery);
    VerifyOrQuit(message != nullptr);

    // Request many TLVs to potentially trigger multiple answer messages (segmentation)
    SuccessOrQuit(Tlv::Append<NetworkDiagnostic::TypeListTlv>(*message, kManylvTypes, sizeof(kManylvTypes)));

    responseContext.Clear();
    SuccessOrQuit(node1.Get<Tmf::SecureAgent>().SendMessage(*message, HandleInspectorResponse, &responseContext));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    SuccessOrQuit(responseContext.mError);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the received `Diagnostic Get Answer` by Inspector");

    VerifyOrQuit(resourceContext.mAnswerCount >= 1);

    Log("  Total answers received: %u", resourceContext.mAnswerCount);

    for (uint8_t tlvType : kManylvTypes)
    {
        bool found = false;

        for (const Message &answer : resourceContext.mAnswers)
        {
            if (tlvInfo.FindIn(answer, tlvType) == kErrorNone)
            {
                found = true;
                break;
            }
        }

        VerifyOrQuit(found);
    }

    Log("  All requested TLVs are present in %u answers", resourceContext.mAnswerCount);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check number of Child TLVs in all received answers");

    numTlvs = 0;

    for (Message &answer : resourceContext.mAnswers)
    {
        while (tlvInfo.FindIn(answer, NetworkDiagnostic::Tlv::kChild) == kErrorNone)
        {
            if (tlvInfo.GetLength() > 0)
            {
                NetworkDiagnostic::ChildTlvValue childTlvValue;

                SuccessOrQuit(tlvInfo.Read<NetworkDiagnostic::ChildTlv>(answer, childTlvValue));

                Log("  Child 0x%02x %s", childTlvValue.GetRloc16(),
                    childTlvValue.GetExtAddress().ToString().AsCString());
            }

            numTlvs++;
            answer.SetOffset(tlvInfo.GetTlvOffsetRange().GetEndOffset());
        }
    }

    Log("  Total Child TLVs in all answers: %u", numTlvs);

    VerifyOrQuit(numTlvs == kNumChildren + 1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAgentInspector();
    printf("All tests passed\n\r");
    return 0;
}
