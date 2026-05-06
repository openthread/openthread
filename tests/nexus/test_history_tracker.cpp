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

void TestHistoryTracker(void)
{
    static constexpr uint32_t kFormLeaderTimeMsec        = 13 * 1000;
    static constexpr uint32_t kStopLeaderTimeMsec        = 5 * 1000;
    static constexpr uint32_t kRestartLeaderTimeMsec     = 25 * 1000;
    static constexpr uint32_t kJoinChildTimeMsec         = 10 * 1000;
    static constexpr uint32_t kAgeVerificationWindowMsec = 10000u;

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Start leader and verify its netinfo history");

    leader.Form();
    nexus.AdvanceTime(kFormLeaderTimeMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    HistoryTracker::Iterator           iter;
    uint32_t                           age;
    const HistoryTracker::NetworkInfo *netInfo;

    iter.Init();
    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleLeader);
    VerifyOrQuit(netInfo->mMode.mRxOnWhenIdle);
    VerifyOrQuit(netInfo->mMode.mDeviceType);
    VerifyOrQuit(netInfo->mRloc16 == leader.Get<Mle::Mle>().GetRloc16());
    VerifyOrQuit(netInfo->mPartitionId == leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId());

    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleDetached);

    Log("---------------------------------------------------------------------------------------");
    Log("Stop leader and verify its netinfo history contains 'disabled'");

    leader.Get<ThreadNetif>().Down();
    leader.Get<Mle::Mle>().Stop();

    nexus.AdvanceTime(kStopLeaderTimeMsec);

    iter.Init();
    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleDisabled);

    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleLeader);

    Log("---------------------------------------------------------------------------------------");
    Log("Wait for 49 days and verify age calculations");

    nexus.AdvanceTime(Time::kOneDayInMsec);

    iter.Init();
    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(age >= Time::kOneDayInMsec && age < Time::kOneDayInMsec + kAgeVerificationWindowMsec);

    nexus.AdvanceTime(Time::kOneDayInMsec);
    iter.Init();
    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(age >= 2 * Time::kOneDayInMsec && age < 2 * Time::kOneDayInMsec + kAgeVerificationWindowMsec);

    nexus.AdvanceTime(47 * Time::kOneDayInMsec);
    iter.Init();
    netInfo = leader.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(age == HistoryTracker::kMaxAge);

    Log("---------------------------------------------------------------------------------------");
    Log("Restart Leader, join Child");

    leader.Form();
    nexus.AdvanceTime(kRestartLeaderTimeMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    child.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kJoinChildTimeMsec);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Verify child netinfo");

    iter.Init();
    netInfo = child.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleChild);
    VerifyOrQuit(netInfo->mMode.mRxOnWhenIdle);
    VerifyOrQuit(!netInfo->mMode.mDeviceType);
    VerifyOrQuit(netInfo->mRloc16 == child.Get<Mle::Mle>().GetRloc16());
    VerifyOrQuit(netInfo->mPartitionId == leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId());

    netInfo = child.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(MapEnum(netInfo->mRole) == Mle::kRoleDetached);

    Log("---------------------------------------------------------------------------------------");
    Log("Change child mode and verify netinfo");

    Mle::DeviceMode mode(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullThreadDevice |
                         Mle::DeviceMode::kModeFullNetworkData);

    SuccessOrQuit(child.Get<Mle::Mle>().SetDeviceMode(mode));
    SuccessOrQuit(child.Get<Mle::Mle>().SetRouterEligible(false));
    nexus.AdvanceTime(kJoinChildTimeMsec);

    iter.Init();
    netInfo = child.Get<HistoryTracker::Local>().IterateNetInfoHistory(iter, age);
    VerifyOrQuit(netInfo != nullptr);
    VerifyOrQuit(netInfo->mMode.mDeviceType);

    Log("---------------------------------------------------------------------------------------");
    Log("Ping between leader and child and verify TX and RX message histories");

    static constexpr uint16_t kPingSizes[] = {10, 100, 1000};
    for (uint16_t size : kPingSizes)
    {
        nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), size);
    }

    // Check the TX history of the leader for the 3 Echo Requests
    const HistoryTracker::MessageInfo *msgInfo;

    iter.Init();
    uint8_t txRequestsFound = 0;
    while ((msgInfo = leader.Get<HistoryTracker::Local>().IterateTxHistory(iter, age)) != nullptr)
    {
        if (msgInfo->mIcmp6Type == OT_ICMP6_TYPE_ECHO_REQUEST)
        {
            VerifyOrQuit(txRequestsFound < 3);
            uint16_t expectedSize = kPingSizes[2 - txRequestsFound];
            VerifyOrQuit(msgInfo->mIpProto == OT_IP6_PROTO_ICMP6);
            VerifyOrQuit(msgInfo->mPayloadLength == expectedSize + sizeof(Ip6::Icmp::Header));
            VerifyOrQuit(AsCoreType(&msgInfo->mSource.mAddress) == leader.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(AsCoreType(&msgInfo->mDestination.mAddress) == child.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(msgInfo->mLinkSecurity);
            VerifyOrQuit(msgInfo->mRadioIeee802154);
            VerifyOrQuit(msgInfo->mPriority == OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL);
            VerifyOrQuit(msgInfo->mNeighborRloc16 == child.Get<Mle::Mle>().GetRloc16());
            VerifyOrQuit(msgInfo->mChecksum != 0);
            VerifyOrQuit(msgInfo->mTxSuccess);
            txRequestsFound++;
        }
    }
    VerifyOrQuit(txRequestsFound == 3);

    // Now check the RX history of the child (should have received the 3 Echo Requests)
    iter.Init();
    uint8_t rxRequestsFound = 0;
    while ((msgInfo = child.Get<HistoryTracker::Local>().IterateRxHistory(iter, age)) != nullptr)
    {
        if (msgInfo->mIcmp6Type == OT_ICMP6_TYPE_ECHO_REQUEST)
        {
            VerifyOrQuit(rxRequestsFound < 3);
            uint16_t expectedSize = kPingSizes[2 - rxRequestsFound];
            VerifyOrQuit(msgInfo->mIpProto == OT_IP6_PROTO_ICMP6);
            VerifyOrQuit(msgInfo->mPayloadLength == expectedSize + sizeof(Ip6::Icmp::Header));
            VerifyOrQuit(AsCoreType(&msgInfo->mSource.mAddress) == leader.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(AsCoreType(&msgInfo->mDestination.mAddress) == child.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(msgInfo->mLinkSecurity);
            VerifyOrQuit(msgInfo->mRadioIeee802154);
            VerifyOrQuit(msgInfo->mPriority == OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL);
            VerifyOrQuit(msgInfo->mNeighborRloc16 == leader.Get<Mle::Mle>().GetRloc16());
            VerifyOrQuit(msgInfo->mChecksum != 0);
            rxRequestsFound++;
        }
    }
    VerifyOrQuit(rxRequestsFound == 3);

    // The child then replied, so let's check the RX history of the leader for the 3 Echo Replies
    iter.Init();
    uint8_t repliesFound = 0;
    while ((msgInfo = leader.Get<HistoryTracker::Local>().IterateRxHistory(iter, age)) != nullptr)
    {
        if (msgInfo->mIcmp6Type == OT_ICMP6_TYPE_ECHO_REPLY)
        {
            VerifyOrQuit(msgInfo->mIpProto == OT_IP6_PROTO_ICMP6);
            VerifyOrQuit(AsCoreType(&msgInfo->mSource.mAddress) == child.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(AsCoreType(&msgInfo->mDestination.mAddress) == leader.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(msgInfo->mLinkSecurity);
            VerifyOrQuit(msgInfo->mRadioIeee802154);
            VerifyOrQuit(msgInfo->mPriority == OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL);
            VerifyOrQuit(msgInfo->mNeighborRloc16 == child.Get<Mle::Mle>().GetRloc16());
            VerifyOrQuit(msgInfo->mChecksum != 0);
            repliesFound++;
        }
    }
    VerifyOrQuit(repliesFound == 3);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestHistoryTracker();
    printf("All tests passed\n");
    return 0;
}
