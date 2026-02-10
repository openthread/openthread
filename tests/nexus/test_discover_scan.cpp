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

struct DiscoverContext
{
    static constexpr uint16_t kMaxResults = 16;

    void Clear(void)
    {
        mDiscoverDone = false;
        mScanResults.Clear();
    }

    bool                                                 mDiscoverDone;
    Array<Mle::DiscoverScanner::ScanResult, kMaxResults> mScanResults;
};

struct RequestCallbackContext : public Clearable<RequestCallbackContext>
{
    bool                           mInvoked;
    Mle::Mle::DiscoveryRequestInfo mInfo;
};

void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    DiscoverContext *context = static_cast<DiscoverContext *>(aContext);

    VerifyOrQuit(aContext != nullptr);

    Log("   HandleDiscoverResult() called%s", aResult == nullptr ? " (done)" : "");

    if (aResult == nullptr)
    {
        context->mDiscoverDone = true;
    }
    else
    {
        VerifyOrQuit(!context->mDiscoverDone);
        SuccessOrQuit(context->mScanResults.PushBack(*aResult));
    }
}

void HandleDiscoverRequest(const otThreadDiscoveryRequestInfo *aInfo, void *aContext)
{
    RequestCallbackContext *context = static_cast<RequestCallbackContext *>(aContext);

    VerifyOrQuit(aInfo != nullptr);
    VerifyOrQuit(aContext != nullptr);

    Log("   HandleDiscoverRequest() called");
    Log("      ExtAddress: %s", AsCoreType(&aInfo->mExtAddress).ToString().AsCString());
    Log("      Version: %u", aInfo->mVersion);
    Log("      IsJoiner: %u", aInfo->mIsJoiner);

    VerifyOrQuit(!context->mInvoked);

    context->mInvoked = true;
    context->mInfo    = *aInfo;
}

void TestDiscoverScanRequestCallback(void)
{
    Core                              nexus;
    Node                             &leader  = nexus.CreateNode();
    Node                             &scanner = nexus.CreateNode();
    DiscoverContext                   resultContext;
    RequestCallbackContext            requestContext;
    Mle::DiscoverScanner::ScanResult *result;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestDiscoverScanRequestCallback");

    nexus.AdvanceTime(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form the network");

    leader.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    scanner.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Register Discovery Request callback on `leader`");

    requestContext.Clear();
    leader.Get<Mle::Mle>().SetDiscoveryRequestCallback(HandleDiscoverRequest, &requestContext);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Perform discover scan from `scanner`");
    resultContext.Clear();

    SuccessOrQuit(scanner.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), 0xffff, /* aJoiner */ false,
                                                               /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                               HandleDiscoverResult, &resultContext));

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the Discovery Request callback is invoked correctly");

    VerifyOrQuit(requestContext.mInvoked);
    VerifyOrQuit(requestContext.mInfo.mVersion == kThreadVersion);
    VerifyOrQuit(AsCoreType(&requestContext.mInfo.mExtAddress) == scanner.Get<Mac::Mac>().GetExtAddress());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the Discovery Result");

    VerifyOrQuit(resultContext.mDiscoverDone);
    VerifyOrQuit(resultContext.mScanResults.GetLength() == 1);

    result = &resultContext.mScanResults[0];

    VerifyOrQuit(AsCoreType(&result->mExtAddress) == leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(AsCoreType(&result->mExtendedPanId) == leader.Get<MeshCoP::NetworkIdentity>().GetExtPanId());
    VerifyOrQuit(result->mPanId == leader.Get<Mac::Mac>().GetPanId());
    VerifyOrQuit(result->mChannel == leader.Get<Mac::Mac>().GetPanChannel());
    VerifyOrQuit(result->mDiscover);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDiscoverScanRequestCallback();
    printf("All tests passed\n");
    return 0;
}
