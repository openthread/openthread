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

/*
 * Regression test for JOIN_FIN.rsp state handling and Provisioning URL requirements:
 *
 * 1) A joiner whose JOIN_FIN.req the commissioner REJECTS (provisioning URL mismatch)
 *    must NOT complete joining: the commissioner must not send the Joiner Entrust
 *    (network credentials) and the joiner must finish with an error.
 * 2) A joiner that OMITS the Provisioning URL TLV while the commissioner HAS a
 *    provisioning URL configured must be rejected too (the URL check must not be
 *    joiner-opt-in).
 * 3) Positive control: a joiner presenting the matching provisioning URL joins
 *    successfully through the identical flow.
 * 4) Interop control: a joiner that omits the TLV still joins a commissioner with
 *    NO provisioning URL configured (joiners predating the TLV keep working).
 *
 * FAILS without the fix (a rejected joiner still receives the entrust and joins;
 * an omitted TLV is accepted despite a configured URL); PASSES with it (the joiner
 * half fails closed on any non-Accept state; the commissioner half only marks an
 * ACCEPTED finalize response as the entrust-triggering subtype and rejects an omitted
 * Provisioning URL TLV when one is configured).
 */

#include <stdio.h>
#include <string.h>

#include <openthread/commissioner.h>
#include <openthread/joiner.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "meshcop/joiner.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;

static bool  sJoinerCallbackInvoked;
static Error sJoinerResult;

static void HandleJoinerComplete(otError aError, void *)
{
    sJoinerCallbackInvoked = true;
    sJoinerResult          = static_cast<Error>(aError);
}

// aCommissionerUrl == nullptr => no provisioning URL configured on the commissioner.
// aJoinerUrl == nullptr       => the joiner omits the Provisioning URL TLV entirely.
static void RunJoin(const char *aCommissionerUrl, const char *aJoinerUrl, bool aExpectSuccess)
{
    Core  nexus;
    Node &commissioner = nexus.CreateNode();
    Node &joiner       = nexus.CreateNode();

    sJoinerCallbackInvoked = false;
    sJoinerResult          = kErrorNone;

    commissioner.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(otCommissionerStart(&commissioner.GetInstance(), nullptr, nullptr, nullptr));
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(otCommissionerGetState(&commissioner.GetInstance()) == OT_COMMISSIONER_STATE_ACTIVE);

    if (aCommissionerUrl != nullptr)
    {
        SuccessOrQuit(otCommissionerSetProvisioningUrl(&commissioner.GetInstance(), aCommissionerUrl));
    }

    SuccessOrQuit(otCommissionerAddJoiner(&commissioner.GetInstance(), nullptr, "J01NME", 300));
    nexus.AdvanceTime(2 * 1000);

    SuccessOrQuit(otIp6SetEnabled(&joiner.GetInstance(), true));
    SuccessOrQuit(otJoinerStart(&joiner.GetInstance(), "J01NME", aJoinerUrl, "VendorX", "ModelY", "1.0", nullptr,
                                HandleJoinerComplete, nullptr));

    bool sawEntrustState = false;

    for (int i = 0; (i < 600) && !sJoinerCallbackInvoked; i++)
    {
        nexus.AdvanceTime(100);

        if (joiner.Get<MeshCoP::Joiner>().GetState() == MeshCoP::Joiner::kStateEntrust)
        {
            sawEntrustState = true;
        }
    }

    Log("commissioner url=%s joiner url=%s: callback=%s result=%d sawEntrustState=%s",
        (aCommissionerUrl != nullptr) ? aCommissionerUrl : "(none)",
        (aJoinerUrl != nullptr) ? aJoinerUrl : "(TLV omitted)", sJoinerCallbackInvoked ? "invoked" : "NOT-invoked",
        static_cast<int>(sJoinerResult), sawEntrustState ? "YES" : "no");

    VerifyOrQuit(sJoinerCallbackInvoked);

    if (aExpectSuccess)
    {
        VerifyOrQuit(sJoinerResult == kErrorNone);
    }
    else
    {
        // A rejected joiner must not be entrusted and must not report success.
        VerifyOrQuit(sJoinerResult != kErrorNone);
        VerifyOrQuit(!sawEntrustState);
    }
}

void TestJoinerFinalizeReject(void)
{
    // printf (not nexus Log) for the banners: Log needs a live Core and each
    // case creates and destroys its own.
    printf("Case 1: provisioning URL mismatch - commissioner rejects - join must FAIL\n");
    RunJoin("good-url", "bad-url", /* aExpectSuccess */ false);

    printf("Case 2: ProvisioningUrl TLV omitted while commissioner has a URL configured - join must FAIL\n");
    RunJoin("good-url", nullptr, /* aExpectSuccess */ false);

    printf("Case 3 (positive control): matching provisioning URL - join must SUCCEED\n");
    RunJoin("good-url", "good-url", /* aExpectSuccess */ true);

    printf("Case 4 (interop control): no URL configured, joiner omits the TLV - join must SUCCEED\n");
    RunJoin(nullptr, nullptr, /* aExpectSuccess */ true);

    printf("TestJoinerFinalizeReject passed\n");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestJoinerFinalizeReject();
    printf("All tests passed\n");
    return 0;
}
