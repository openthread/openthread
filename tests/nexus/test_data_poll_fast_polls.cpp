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

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime   = 13 * 1000;
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;
static constexpr uint32_t kSettleTime        = 5 * 1000;
static constexpr uint32_t kIdleCheckWindow   = 3 * 1000;
static constexpr uint32_t kFirstFastPollWait = 100; // Between first poll (10 ms) and second poll (198 ms)
static constexpr uint32_t kFastPollPeriod    = 188;

static uint32_t GetTxDataPollCount(Node &aNode) { return aNode.Get<Mac::Mac>().GetCounters().mTxDataPoll; }

void TestDataPollFastPolls(void)
{
    Core            nexus;
    Node           &leader = nexus.CreateNode();
    Node           &sed    = nexus.CreateNode();
    DataPollSender &sender = sed.Get<DataPollSender>();
    uint32_t        pollsBefore;

    leader.SetName("LEADER");
    sed.SetName("SED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    AllowLinkBetween(leader, sed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    // Let any initial attach fast polls complete and verify no polls occur during a short idle window
    // (normal keep-alive period is ~236 s).
    nexus.AdvanceTime(kSettleTime);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 1: Zero-count RequestFastPolls() and inactive CancelFastPolls() are no-ops");

    pollsBefore = GetTxDataPollCount(sed);
    sender.RequestFastPolls(DataPollSender::kRequesterUser, 0);
    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 2: Repeated RequestFastPolls() from same requester stopped by single CancelFastPolls()");

    pollsBefore = GetTxDataPollCount(sed);

    sender.RequestFastPolls(DataPollSender::kRequesterSrpClient, 4);
    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 1);

    // Retransmission-style repeat calls from the same requester (both smaller and larger counts).
    sender.RequestFastPolls(DataPollSender::kRequesterSrpClient, 2);
    sender.RequestFastPolls(DataPollSender::kRequesterSrpClient, 10);

    nexus.AdvanceTime(kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 2);

    // A single `CancelFastPolls()` clears `kRequesterSrpClient` and stops fast polling immediately.
    sender.CancelFastPolls(DataPollSender::kRequesterSrpClient);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 2);

    //-----------------------------------------------------------------------------------------------------------------

    Log("Case 3: Concurrent requesters cancelling independently");

    pollsBefore = GetTxDataPollCount(sed);

    sender.RequestFastPolls(DataPollSender::kRequesterMle, 6);
    sender.RequestFastPolls(DataPollSender::kRequesterMlrManager, 10);

    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 1);

    // Cancelling an inactive requester (`kRequesterUser`) and one of the two active requesters (`kRequesterMle`)
    // must keep fast polling active for `kRequesterMlrManager`.
    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    sender.CancelFastPolls(DataPollSender::kRequesterMle);

    nexus.AdvanceTime(2 * kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 3);

    // Repeating `CancelFastPolls()` for an already-cancelled requester has no effect.
    sender.CancelFastPolls(DataPollSender::kRequesterMle);
    nexus.AdvanceTime(kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 4);

    // Cancelling the remaining active requester (`kRequesterMlrManager`) stops fast polling immediately.
    sender.CancelFastPolls(DataPollSender::kRequesterMlrManager);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 4);

    //-----------------------------------------------------------------------------------------------------------------
    // `mRemainingFastPolls` expiry preserves `Max()` count, stops fast polling, and clears active requesters.

    Log("Case 4: Remaining fast polls expiry clears active requesters");

    pollsBefore = GetTxDataPollCount(sed);

    sender.RequestFastPolls(DataPollSender::kRequesterMle, 5);
    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 1);

    // A smaller concurrent request (`2 < 4` remaining) must not shrink `mRemainingFastPolls`, and marks a second
    // active requester without calling `CancelFastPolls()` on either requester.
    sender.RequestFastPolls(DataPollSender::kRequesterSrpClient, 2);

    // Let all remaining fast polls expire naturally and verify exact total count (5 polls).
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 5);

    // Verify `kRequesterMle` and `kRequesterSrpClient` were cleared on expiry: a new request from `kRequesterUser`
    // followed by `CancelFastPolls(kRequesterUser)` alone must stop fast polling immediately.
    sender.RequestFastPolls(DataPollSender::kRequesterUser, 8);
    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 6);

    sender.CancelFastPolls(DataPollSender::kRequesterUser);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 6);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDataPollFastPolls();

    printf("All tests passed\n");
    return 0;
}
