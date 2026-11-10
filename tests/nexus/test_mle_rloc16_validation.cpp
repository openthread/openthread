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

#include "coap/coap_message.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/mle.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/thread_tlvs.hpp"

namespace ot {

class UnitTester
{
public:
    static bool HasParentCandidate(Nexus::Node &aNode)
    {
        return aNode.Get<Mle::Mle>().mAttacher.mParentCandidate.IsStateParentResponse();
    }

    static bool IsInParentRequestState(Nexus::Node &aNode)
    {
        return aNode.Get<Mle::Mle>().mAttacher.mState == Mle::Mle::Attacher::kStateParentRequest;
    }

    static bool IsInChildIdRequestState(Nexus::Node &aNode)
    {
        return aNode.Get<Mle::Mle>().mAttacher.mState == Mle::Mle::Attacher::kStateChildIdRequest;
    }

    static uint8_t GetRouterId(Nexus::Node &aNode) { return aNode.Get<Mle::Mle>().mRouterId; }

    static void SendParentResponse(Nexus::Node &aSender,
                                   Nexus::Node &aChild,
                                   uint16_t     aSourceRloc16,
                                   uint8_t      aLeaderRouterId)
    {
        Mle::Mle            &senderMle = aSender.Get<Mle::Mle>();
        Mle::Mle::TxMessage *message   = senderMle.NewMleMessage(Mle::kCommandParentResponse);
        Mle::LeaderData      leaderData;
        Mle::RxChallenge     response;
        Mle::TxChallenge     challenge;
        Ip6::Address         destination;

        VerifyOrQuit(message != nullptr);
        message->SetDirectTransmission();

        leaderData = senderMle.GetLeaderData();
        leaderData.SetLeaderRouterId(aLeaderRouterId);

        response.InitFrom(aChild.Get<Mle::Mle>().mAttacher.mParentRequestChallenge);
        challenge.GenerateRandom();

        SuccessOrQuit(Tlv::Append<Mle::SourceAddressTlv>(*message, aSourceRloc16));
        SuccessOrQuit(Tlv::Append<Mle::LeaderDataTlv>(*message, Mle::LeaderDataTlvValue(leaderData)));
        SuccessOrQuit(message->AppendLinkFrameCounterTlv());
        SuccessOrQuit(message->AppendMleFrameCounterTlv());
        SuccessOrQuit(message->AppendResponseTlv(response));
        SuccessOrQuit(message->AppendChallengeTlv(challenge));
        SuccessOrQuit(message->AppendLinkMarginTlv(30));
        SuccessOrQuit(message->AppendConnectivityTlv());
        SuccessOrQuit(message->AppendVersionTlv());

        destination.InitAsLinkLocalAddress(aChild.Get<Mac::Mac>().GetExtAddress());
        SuccessOrQuit(message->SendTo(destination));
    }

    static void SendChildIdResponse(Nexus::Node &aSender,
                                    Nexus::Node &aChild,
                                    uint16_t     aSourceRloc16,
                                    uint16_t     aChildRloc16,
                                    uint8_t      aLeaderRouterId)
    {
        Mle::Mle            &senderMle = aSender.Get<Mle::Mle>();
        Mle::Mle::TxMessage *message   = senderMle.NewMleMessage(Mle::kCommandChildIdResponse);
        Mle::LeaderData      leaderData;
        Ip6::Address         destination;

        VerifyOrQuit(message != nullptr);
        message->SetDirectTransmission();

        leaderData = senderMle.GetLeaderData();
        leaderData.SetLeaderRouterId(aLeaderRouterId);

        SuccessOrQuit(Tlv::Append<Mle::SourceAddressTlv>(*message, aSourceRloc16));
        SuccessOrQuit(Tlv::Append<Mle::LeaderDataTlv>(*message, Mle::LeaderDataTlvValue(leaderData)));
        SuccessOrQuit(message->AppendActiveTimestampTlv());
        SuccessOrQuit(message->AppendAddress16Tlv(aChildRloc16));
        SuccessOrQuit(message->AppendNetworkDataTlv(NetworkData::kFullSet));

        destination.InitAsLinkLocalAddress(aChild.Get<Mac::Mac>().GetExtAddress());
        SuccessOrQuit(message->SendTo(destination));
    }

    static void SendChildUpdateResponse(Nexus::Node &aSender, Nexus::Node &aChild, uint16_t aSourceRloc16)
    {
        Mle::Mle            &senderMle = aSender.Get<Mle::Mle>();
        Mle::Mle::TxMessage *message   = senderMle.NewMleMessage(Mle::kCommandChildUpdateResponse);
        Ip6::Address         destination;

        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<Mle::SourceAddressTlv>(*message, aSourceRloc16));
        SuccessOrQuit(message->AppendModeTlv(aChild.Get<Mle::Mle>().GetDeviceMode()));
        SuccessOrQuit(message->AppendLeaderDataTlv());

        destination.InitAsLinkLocalAddress(aChild.Get<Mac::Mac>().GetExtAddress());
        SuccessOrQuit(message->SendTo(destination));
    }

    static void ProcessAddressSolicitResponse(Nexus::Node             &aReed,
                                              uint16_t                 aAssignedRloc16,
                                              const Mle::RouterIdMask &aRouterIdMask)
    {
        Mle::Mle        &mle     = aReed.Get<Mle::Mle>();
        Coap::Message   *message = aReed.Get<Tmf::Agent>().NewMessage();
        Ip6::MessageInfo messageInfo;

        VerifyOrQuit(message != nullptr);
        SuccessOrQuit(message->Init(Coap::kTypeAck, Coap::kCodeChanged));
        SuccessOrQuit(message->AppendPayloadMarker());

        SuccessOrQuit(Tlv::Append<ThreadStatusTlv>(*message, Mle::Mle::kAddrSolicitSuccess));
        SuccessOrQuit(Tlv::Append<ThreadRloc16Tlv>(*message, aAssignedRloc16));
        SuccessOrQuit(Tlv::Append<ThreadRouterMaskTlv>(*message, aRouterIdMask));

        mle.ComposeLeaderRloc(messageInfo.GetPeerAddr());
        messageInfo.SetSockAddr(mle.GetMeshLocalRloc());

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRejectIfNoPayloadWithPayloadMarker));
            mle.mAddressSolicitPending = true;
            mle.HandleAddressSolicitResponse(&msg, kErrorNone);
        }

        message->Free();
    }
};

namespace Nexus {

void TestMleRloc16Validation(void)
{
    static constexpr uint16_t kRloc16ReservedBitMask = 1 << (Mle::kRouterIdOffset - 1);

    struct InvalidParentResponseCase
    {
        uint16_t mSourceRloc16;
        bool     mUseInvalidLeaderId;
    };

    struct InvalidChildIdResponseCase
    {
        uint16_t mSourceRloc16;
        uint16_t mChildRloc16;
        bool     mUseInvalidLeaderId;
    };

    static const InvalidParentResponseCase kInvalidParentResponses[] = {
        {Mac::kShortAddrBroadcast, false},
        {Mle::kInvalidRloc16, false},
        {0xfc00, false},
        {0xfc01, false},
        {0xfe00, false},
        {0x0200, false},
        {0x0201, false},
        {0x0000, true},
    };

    static const InvalidChildIdResponseCase kInvalidChildIdResponses[] = {
        {0xffff, 0xfc00, false}, {0xfc00, 0xfc01, false}, {0xfe00, 0xfffe, false}, {0x0200, 0x0201, false},
        {0x0000, 0x0000, false}, {0x0001, 0x0002, false}, {0x0400, 0x0801, false}, {0x0000, 0x0001, true},
    };

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();

    uint16_t leaderRloc16;
    uint8_t  leaderRouterId;
    uint16_t sedRloc16;
    uint16_t reedChildRloc16;

    leader.SetName("LEADER");
    sed.SetName("SED");
    reed.SetName("REED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network on Leader");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    leaderRloc16   = leader.Get<Mle::Mle>().GetRloc16();
    leaderRouterId = leader.Get<Mle::Mle>().GetLeaderId();
    VerifyOrQuit(Mle::IsValidRloc16(leaderRloc16) && Mle::IsRouterRloc16(leaderRloc16));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Verify SED rejects Parent Responses with invalid RLOC16 or Leader Router ID");

    // Enable an empty MAC allowlist on Leader so Leader does not automatically reply while we
    // transmit explicit MLE Parent Response / Child ID Response frames to the attaching SED.
    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));
    sed.Join(leader, Node::kAsSed);

    // Advance until SED sends its first Parent Request.
    for (uint16_t step = 0; (step < 50) && !UnitTester::IsInParentRequestState(sed); step++)
    {
        nexus.AdvanceTime(50);
    }

    VerifyOrQuit(UnitTester::IsInParentRequestState(sed));
    VerifyOrQuit(!UnitTester::HasParentCandidate(sed));

    for (const InvalidParentResponseCase &testCase : kInvalidParentResponses)
    {
        uint16_t sourceRloc16 = testCase.mUseInvalidLeaderId ? leaderRloc16 : testCase.mSourceRloc16;
        uint8_t  leaderId     = testCase.mUseInvalidLeaderId ? Mle::kInvalidRouterId : leaderRouterId;

        UnitTester::SendParentResponse(leader, sed, sourceRloc16, leaderId);
        nexus.AdvanceTime(20);

        VerifyOrQuit(!UnitTester::HasParentCandidate(sed));
        VerifyOrQuit(sed.Get<Mle::Mle>().IsDetached());
        VerifyOrQuit(sed.Get<Mle::Mle>().GetRloc16() == Mle::kInvalidRloc16);
        VerifyOrQuit(sed.Get<Mac::Mac>().GetShortAddress() == Mac::kShortAddrInvalid);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Advance SED to Child ID Request state and verify invalid Child ID Responses");

    UnitTester::SendParentResponse(leader, sed, leaderRloc16, leaderRouterId);
    nexus.AdvanceTime(20);
    VerifyOrQuit(UnitTester::HasParentCandidate(sed));

    for (uint16_t step = 0; (step < 30) && !UnitTester::IsInChildIdRequestState(sed); step++)
    {
        nexus.AdvanceTime(50);
    }

    VerifyOrQuit(UnitTester::IsInChildIdRequestState(sed));

    // Allow the Child ID Request MAC transmission to finish so SED is listening in RX-idle.
    nexus.AdvanceTime(100);

    for (const InvalidChildIdResponseCase &testCase : kInvalidChildIdResponses)
    {
        uint16_t sourceRloc16 = testCase.mUseInvalidLeaderId ? leaderRloc16 : testCase.mSourceRloc16;
        uint16_t childRloc16 =
            testCase.mUseInvalidLeaderId ? static_cast<uint16_t>(leaderRloc16 | 1) : testCase.mChildRloc16;
        uint8_t leaderId = testCase.mUseInvalidLeaderId ? Mle::kInvalidRouterId : leaderRouterId;

        UnitTester::SendChildIdResponse(leader, sed, sourceRloc16, childRloc16, leaderId);
        nexus.AdvanceTime(150);

        VerifyOrQuit(sed.Get<Mle::Mle>().IsDetached());
        VerifyOrQuit(sed.Get<Mle::Mle>().GetRloc16() == Mle::kInvalidRloc16);
        VerifyOrQuit(sed.Get<Mac::Mac>().GetShortAddress() == Mac::kShortAddrInvalid);
        VerifyOrQuit(UnitTester::IsInChildIdRequestState(sed));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Re-enable normal attach to Leader and verify SED attaches and polls cleanly");

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);

    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    sedRloc16 = sed.Get<Mle::Mle>().GetRloc16();
    VerifyOrQuit(Mle::IsValidRloc16(sedRloc16) && Mle::IsChildRloc16(sedRloc16));
    VerifyOrQuit(Mle::IsValidRloc16(sed.Get<Mle::Mle>().GetParent().GetRloc16()) &&
                 Mle::IsRouterRloc16(sed.Get<Mle::Mle>().GetParent().GetRloc16()));
    VerifyOrQuit(sed.Get<Mle::Mle>().GetParent().GetRloc16() == leaderRloc16);

    nexus.SendAndVerifyEchoRequest(sed, leader.Get<Mle::Mle>().GetMeshLocalRloc(), 0, 64, 2000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify SED detaches on Child Update Response with invalid or mismatched Source Address TLV");

    {
        const uint8_t  otherRouterId        = (leaderRouterId == 0) ? 1 : 0;
        const uint16_t invalidParentRlocs[] = {
            sedRloc16,
            Mle::Rloc16FromRouterId(otherRouterId),
            static_cast<uint16_t>(leaderRloc16 | kRloc16ReservedBitMask),
            0xfc00,
            Mac::kShortAddrBroadcast,
        };

        for (uint16_t badSourceRloc16 : invalidParentRlocs)
        {
            uint16_t prevDetachedCount = sed.Get<Mle::Mle>().GetCounters().mDetachedRole;

            UnitTester::SendChildUpdateResponse(leader, sed, badSourceRloc16);

            for (uint16_t step = 0;
                 (step < 30) && (sed.Get<Mle::Mle>().GetCounters().mDetachedRole == prevDetachedCount); step++)
            {
                nexus.AdvanceTime(50);
            }

            VerifyOrQuit(sed.Get<Mle::Mle>().GetCounters().mDetachedRole > prevDetachedCount);

            nexus.AdvanceTime(10 * 1000);
            VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
        }

        nexus.SendAndVerifyEchoRequest(sed, leader.Get<Mle::Mle>().GetMeshLocalRloc(), 0, 64, 2000);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Verify REED ignores invalid Address Solicit Responses without changing role");

    leader.Get<Mle::Mle>().SetLeaderUpgradeThreshold(1);
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
    reed.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    reedChildRloc16 = reed.Get<Mle::Mle>().GetRloc16();
    VerifyOrQuit(Mle::IsValidRloc16(reedChildRloc16) && Mle::IsChildRloc16(reedChildRloc16));

    {
        const uint8_t     altRouterId = (leaderRouterId == 0) ? 1 : 0;
        uint8_t           unallocId   = 0;
        Mle::RouterIdMask validMask;

        while ((unallocId == leaderRouterId) || (unallocId == altRouterId))
        {
            unallocId++;
        }

        validMask.Clear();
        validMask.Add(leaderRouterId);
        validMask.Add(altRouterId);

        {
            const uint16_t invalidRloc16s[] = {
                0xfc00,
                0xfe00,
                0xffff,
                Mle::kInvalidRloc16,
                static_cast<uint16_t>(Mle::Rloc16FromRouterId(altRouterId) | kRloc16ReservedBitMask),
                static_cast<uint16_t>(Mle::Rloc16FromRouterId(altRouterId) | 1),
                Mle::Rloc16FromRouterId(unallocId), // Valid router RLOC16, not allocated in Router ID Mask
            };

            for (uint16_t badRloc16 : invalidRloc16s)
            {
                UnitTester::ProcessAddressSolicitResponse(reed, badRloc16, validMask);
                VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
                VerifyOrQuit(reed.Get<Mle::Mle>().GetRloc16() == reedChildRloc16);
                VerifyOrQuit(UnitTester::GetRouterId(reed) == Mle::kInvalidRouterId);
            }
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Allow REED to upgrade to Router and check missing-leader Router ID Mask on REED2");

    leader.Get<Mle::Mle>().SetLeaderUpgradeThreshold(16);
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(16);
    reed.Get<Mle::Mle>().SetRouterSelectionJitter(1);
    SuccessOrQuit(reed.Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters));
    nexus.AdvanceTime(10 * 1000);

    VerifyOrQuit(reed.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(Mle::IsValidRloc16(reed.Get<Mle::Mle>().GetRloc16()) &&
                 Mle::IsRouterRloc16(reed.Get<Mle::Mle>().GetRloc16()));
    nexus.SendAndVerifyEchoRequest(reed, leader.Get<Mle::Mle>().GetMeshLocalRloc());

    {
        Node             &reed2 = nexus.CreateNode();
        uint16_t          reed2ChildRloc16;
        uint8_t           parentRouterId = Mle::RouterIdFromRloc16(reed.Get<Mle::Mle>().GetRloc16());
        uint8_t           newRouterId    = 0;
        Mle::RouterIdMask maskWithoutLeader;

        while ((newRouterId == leaderRouterId) || (newRouterId == parentRouterId))
        {
            newRouterId++;
        }

        reed2.SetName("REED2");
        reed2.AllowList(reed);
        reed.AllowList(reed2);
        reed.AllowList(leader);
        reed2.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
        reed2.Join(reed, Node::kAsFtd);
        nexus.AdvanceTime(10 * 1000);
        VerifyOrQuit(reed2.Get<Mle::Mle>().IsChild());

        reed2ChildRloc16 = reed2.Get<Mle::Mle>().GetRloc16();
        VerifyOrQuit(reed2.Get<Mle::Mle>().GetParent().GetRloc16() == reed.Get<Mle::Mle>().GetRloc16());

        maskWithoutLeader.Clear();
        maskWithoutLeader.Add(parentRouterId);
        maskWithoutLeader.Add(newRouterId);

        UnitTester::ProcessAddressSolicitResponse(reed2, Mle::Rloc16FromRouterId(newRouterId), maskWithoutLeader);
        VerifyOrQuit(reed2.Get<Mle::Mle>().IsChild());
        VerifyOrQuit(reed2.Get<Mle::Mle>().GetRloc16() == reed2ChildRloc16);
        VerifyOrQuit(UnitTester::GetRouterId(reed2) == Mle::kInvalidRouterId);
    }

    nexus.SaveTestInfo("test_mle_rloc16_validation.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMleRloc16Validation();
    printf("All tests passed\n");
    return 0;
}
