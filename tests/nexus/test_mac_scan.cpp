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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

struct MacScanContext
{
    static constexpr uint16_t kMaxResults = 16;

    void Clear(void)
    {
        mScanDone = false;
        mScanResults.Clear();
    }

    bool                           mScanDone;
    Array<ScanResult, kMaxResults> mScanResults;
};

void HandleMacScanResult(otActiveScanResult *aResult, void *aContext)
{
    MacScanContext *context = static_cast<MacScanContext *>(aContext);
    ScanResult     *result  = AsCoreTypePtr(aResult);

    VerifyOrQuit(aContext != nullptr);

    Log("   HandleMacScanResult() called%s", result == nullptr ? " (done)" : "");

    if (result == nullptr)
    {
        context->mScanDone = true;
    }
    else
    {
        VerifyOrQuit(!context->mScanDone);
        SuccessOrQuit(context->mScanResults.PushBack(*result));
    }
}

void TestMacScan(void)
{
    Core           nexus;
    Node          &leader = nexus.CreateNode();
    Node          &router = nexus.CreateNode();
    MacScanContext resultContext;
    ScanResult    *result;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestMacScan");

    nexus.AdvanceTime(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form the network");

    leader.Form();
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    leader.AllowList(router);
    router.AllowList(leader);

    router.Join(leader);
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsAttached());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Perform MAC scan from leader to find router");

    resultContext.Clear();

    SuccessOrQuit(otLinkActiveScan(&leader.GetInstance(), 0, 0, HandleMacScanResult, &resultContext));

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the Scan Result");

    VerifyOrQuit(resultContext.mScanDone);
    VerifyOrQuit(resultContext.mScanResults.GetLength() == 1);

    bool foundRouter = false;
    for (uint16_t i = 0; i < resultContext.mScanResults.GetLength(); i++)
    {
        result = &resultContext.mScanResults[i];
        Log("   Found result: extaddr:%s channel:%u", result->GetExtAddress().ToString().AsCString(),
            result->GetChannel());

        if (result->GetExtAddress() == router.Get<Mac::Mac>().GetExtAddress())
        {
            foundRouter = true;
            VerifyOrQuit(result->GetChannel() == leader.Get<Mac::Mac>().GetPanChannel());
        }
    }

    VerifyOrQuit(foundRouter, "Leader did not find Router in MAC scan");
    Log("Leader found Router in MAC scan successfully");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMacScan();
    printf("All tests passed\n");
    return 0;
}
