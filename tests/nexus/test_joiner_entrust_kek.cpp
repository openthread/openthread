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
 * Regression test for Joiner Router KEK handling on overlapping Joiner Entrust
 * transactions:
 *
 * When Joiner A stops responding after its JOIN_ENT.ntf is transmitted (leaving
 * the first JOIN_ENT.ntf CoAP transaction pending on the Joiner Router) and
 * Joiner B commissions behind it, aborting Joiner A's pending transaction must
 * not clear the KEK installed for Joiner B before Joiner B's JOIN_ENT.ntf is
 * encrypted and sent.
 */

#include <stdio.h>
#include <string.h>

#include <openthread/commissioner.h>
#include <openthread/dataset.h>
#include <openthread/joiner.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;

static bool  sJoinerBCallbackInvoked;
static Error sJoinerBResult;

static void HandleJoinerAComplete(otError, void *) {}

static void HandleJoinerBComplete(otError aError, void *)
{
    sJoinerBCallbackInvoked = true;
    sJoinerBResult          = static_cast<Error>(aError);
}

void TestJoinerEntrustKek(void)
{
    Core  nexus;
    Node &router  = nexus.CreateNode();
    Node &joinerA = nexus.CreateNode();
    Node &joinerB = nexus.CreateNode();
    Kek   kekA;
    Kek   zeroKek;

    zeroKek.Clear();
    sJoinerBCallbackInvoked = false;
    sJoinerBResult          = kErrorFailed;

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    router.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(otCommissionerStart(&router.GetInstance(), nullptr, nullptr, nullptr));
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(otCommissionerGetState(&router.GetInstance()) == OT_COMMISSIONER_STATE_ACTIVE);

    SuccessOrQuit(otCommissionerAddJoiner(&router.GetInstance(), nullptr, "J01NME", 600));
    nexus.AdvanceTime(2 * 1000);

    // Start Joiner A and advance until the router sets the KEK for Joiner A's JOIN_ENT.ntf.
    SuccessOrQuit(otIp6SetEnabled(&joinerA.GetInstance(), true));
    SuccessOrQuit(otJoinerStart(&joinerA.GetInstance(), "J01NME", nullptr, "VendorX", "ModelY", "1.0", nullptr,
                                HandleJoinerAComplete, nullptr));

    for (int i = 0; (i < 900) && !router.Get<KeyManager>().IsKekSet(); i++)
    {
        nexus.AdvanceTime(50);
    }

    VerifyOrQuit(router.Get<KeyManager>().IsKekSet());
    router.Get<KeyManager>().ExtractKek(kekA);
    VerifyOrQuit(kekA != zeroKek);

    // Denylist Joiner A on the router so Joiner A's JOIN_ENT.rsp is dropped and
    // Joiner A's JOIN_ENT.ntf CoAP transaction remains pending on the router.
    router.Get<Mac::Filter>().SetMode(Mac::Filter::kModeDenylist);
    SuccessOrQuit(router.Get<Mac::Filter>().AddAddress(joinerA.Get<Mac::Mac>().GetExtAddress()));

    nexus.AdvanceTime(200);
    VerifyOrQuit(router.Get<KeyManager>().IsKekSet());

    // Commission Joiner B while Joiner A's JOIN_ENT.ntf transaction is still pending.
    SuccessOrQuit(otIp6SetEnabled(&joinerB.GetInstance(), true));
    SuccessOrQuit(otJoinerStart(&joinerB.GetInstance(), "J01NME", nullptr, "VendorX", "ModelZ", "1.0", nullptr,
                                HandleJoinerBComplete, nullptr));

    for (int i = 0; (i < 1200) && !sJoinerBCallbackInvoked; i++)
    {
        nexus.AdvanceTime(25);
    }

    VerifyOrQuit(sJoinerBCallbackInvoked);
    VerifyOrQuit(sJoinerBResult == kErrorNone);

    {
        NetworkKey routerNetworkKey;
        NetworkKey joinerBNetworkKey;

        router.Get<KeyManager>().GetNetworkKey(routerNetworkKey);
        joinerB.Get<KeyManager>().GetNetworkKey(joinerBNetworkKey);
        VerifyOrQuit(joinerBNetworkKey == routerNetworkKey);
    }

    // Once Joiner B's JOIN_ENT.rsp has been received, the router must have cleared its KEK.
    VerifyOrQuit(!router.Get<KeyManager>().IsKekSet());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestJoinerEntrustKek();
    printf("All tests passed\n");
    return 0;
}
