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

static constexpr uint32_t kFormNetworkTime     = 13 * 1000;
static constexpr uint32_t kAttachAsSsedTime    = 20 * 1000;
static constexpr uint32_t kCslPeriodUs         = 288000;
static constexpr uint32_t kCslPeriod           = kCslPeriodUs / OT_US_PER_TEN_SYMBOLS;
static constexpr uint32_t kCslSyncTime         = 10 * 1000;
static constexpr uint16_t kEchoPayloadSize     = 10;
static constexpr uint32_t kEchoResponseTimeout = 3 * 1000;
static constexpr uint32_t kWaitTimeAfterPing   = 1 * 1000;

void TestCsl(void)
{
    Core nexus;

    Node &parent = nexus.CreateNode();
    Node &ssed   = nexus.CreateNode();

    parent.SetName("PARENT");
    ssed.SetName("SSED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelDebg));

    AllowLinkBetween(parent, ssed);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network with Parent Leader and attach SSED child");

    parent.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(parent.Get<Mle::Mle>().IsLeader());

    ssed.Join(parent, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed.Get<Mle::Mle>().IsAttached());

    ssed.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);
    VerifyOrQuit(ssed.Get<Mac::Mac>().IsCslEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SSED pings Parent");

    nexus.SendAndVerifyEchoRequest(ssed, parent.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize,
                                  Ip6::kDefaultHopLimit, kEchoResponseTimeout);
    nexus.AdvanceTime(kWaitTimeAfterPing);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Parent pings SSED");

    nexus.SendAndVerifyEchoRequest(parent, ssed.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize,
                                  Ip6::kDefaultHopLimit, kEchoResponseTimeout);
    nexus.AdvanceTime(kWaitTimeAfterPing);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCsl();
    printf("All tests passed\n");
    return 0;
}
