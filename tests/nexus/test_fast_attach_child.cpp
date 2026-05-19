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

#if OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE

namespace ot {
namespace Nexus {

/**
 * Fast Attach: verify the Child-side early-select behavior.
 *
 * Two FED nodes attach (sequentially, to avoid radio interference) to the same
 * single-router partition. The first has Fast Attach enabled explicitly via
 * SetFastAttachEnabled(true) before Join; the second keeps the default
 * (disabled). The fast-attach child must complete attach noticeably sooner than
 * the legacy child because it sets the F flag, the Leader uses reduced jitter,
 * and the child early-selects on the first acceptable LQ3 Parent Response. The
 * legacy child sets no F flag and waits the full kParentRequestRouterTimeout
 * (750 ms) before selecting.
 *
 * Per-DUT attach time is measured by polling IsChild() after Join() in
 * kPollStepMs increments. Both DUTs experience the same first-attach random
 * startup delay model (uniform [0, 750] ms via GetStartDelay() when
 * mAttachCounter == 0), so the absolute difference between the two attach
 * times is dominated by the legacy 750 ms Parent-Request wait that the slow
 * DUT pays and the fast DUT skips.
 *
 * The test also verifies the one-shot semantic: after a successful attach the
 * fast-attach flag is auto-cleared, so a subsequent attach attempt would not
 * carry F unless explicitly re-armed.
 *
 * Topology:
 *   LEADER -- FAST_DUT  (Fast Attach explicitly enabled, joins first)
 *   LEADER -- SLOW_DUT  (Fast Attach default-disabled, joins after settle)
 */

static constexpr uint32_t kPollStepMs            = 50;
static constexpr uint32_t kPollMaxSteps          = 60; // up to 3 seconds
static constexpr uint32_t kSettleTimeMs          = 2 * Time::kOneSecondInMsec;
static constexpr uint32_t kMinExpectedSlowExcess = 250; // slow MUST exceed fast by at least this much

static uint32_t MeasureAttachTime(Core &aNexus, Node &aDut)
{
    for (uint32_t step = 1; step <= kPollMaxSteps; step++)
    {
        aNexus.AdvanceTime(kPollStepMs);

        if (aDut.Get<Mle::Mle>().IsChild())
        {
            return step * kPollStepMs;
        }
    }
    return 0;
}

void TestFastAttachChild(void)
{
    Core  nexus;
    Node &leader  = nexus.CreateNode();
    Node &fastDut = nexus.CreateNode();
    Node &slowDut = nexus.CreateNode();

    leader.SetName("LEADER");
    fastDut.SetName("FAST_DUT");
    slowDut.SetName("SLOW_DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, fastDut);
    AllowLinkBetween(leader, slowDut);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network with LEADER");

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Run 1: FAST_DUT attaches with Fast Attach explicitly enabled");

    // Default is disabled; verify and then explicitly enable.
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    SuccessOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    VerifyOrQuit(fastDut.Get<Mle::Mle>().IsFastAttachEnabled());

    fastDut.Join(leader, Node::kAsFed);

    uint32_t fastAttachMs = MeasureAttachTime(nexus, fastDut);
    VerifyOrQuit(fastAttachMs != 0);
    VerifyOrQuit(fastDut.Get<Mle::Mle>().IsChild());

    // One-shot: flag must be auto-cleared after successful attach.
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("One-shot auto-clear verified: IsFastAttachEnabled() is FALSE post-attach");

    // Re-enabling while attached must be rejected.
    VerifyOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(true) == kErrorInvalidState);
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("Setter rejection verified: SetFastAttachEnabled(true) returns kErrorInvalidState when attached");

    // Disabling is always allowed regardless of role.
    SuccessOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(false));
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());

    // Settle so the slow DUT's measurement starts cleanly.
    nexus.AdvanceTime(kSettleTimeMs);

    Log("---------------------------------------------------------------------------------------");
    Log("Run 2: SLOW_DUT attaches with Fast Attach default-disabled");

    VerifyOrQuit(!slowDut.Get<Mle::Mle>().IsFastAttachEnabled());

    slowDut.Join(leader, Node::kAsFed);

    uint32_t slowAttachMs = MeasureAttachTime(nexus, slowDut);
    VerifyOrQuit(slowAttachMs != 0);
    VerifyOrQuit(slowDut.Get<Mle::Mle>().IsChild());

    // The slow DUT pays the full kParentRequestRouterTimeout (750 ms) that the
    // fast DUT skips. After accounting for variance in the [0, 750 ms] random
    // startup delay shared by both DUTs, we conservatively require at least
    // kMinExpectedSlowExcess ms of additional time for the slow DUT.
    Log("FAST_DUT attached at %u ms, SLOW_DUT attached at %u ms (excess %u ms)", fastAttachMs, slowAttachMs,
        slowAttachMs - fastAttachMs);

    VerifyOrQuit(slowAttachMs >= fastAttachMs + kMinExpectedSlowExcess);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFastAttachChild();
    printf("All tests passed\n");
    return 0;
}

#else // OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE

int main(void)
{
    printf("Fast Attach is disabled at compile time; test skipped.\n");
    return 0;
}

#endif // OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE
