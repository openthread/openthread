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

#if OPENTHREAD_CONFIG_MULTI_RADIO

static constexpr uint32_t kFormNetworkTime     = 35 * 1000;
static constexpr uint32_t kScanTime            = 10 * 1000;
static constexpr uint32_t kInfraIfIndex        = 1;
static constexpr uint16_t kDiscoveryMaxResults = 16;
static constexpr uint16_t kPanIdAny            = 0xffff;

struct DiscoverContext
{
    void Clear(void)
    {
        mDiscoverDone = false;
        mScanResults.Clear();
    }

    bool                                    mDiscoverDone;
    Array<ScanResult, kDiscoveryMaxResults> mScanResults;
};

void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    DiscoverContext *context = static_cast<DiscoverContext *>(aContext);
    ScanResult      *result  = AsCoreTypePtr(aResult);

    VerifyOrQuit(aContext != nullptr);

    if (result == nullptr)
    {
        context->mDiscoverDone = true;
    }
    else
    {
        VerifyOrQuit(!context->mDiscoverDone);
        SuccessOrQuit(context->mScanResults.PushBack(*result));
    }
}

void Test1_4_Trel_Tc_5(void)
{
    /**
     * 8.5. [1.4] [CERT] Discover Scan over multi-radio
     *
     * 8.5.1. Purpose
     * This test covers MLE discovery scan when nodes support different radio links.
     *
     * 8.5.2. Topology
     * - 1. Node_1 BR (DUT) - Support multi-radio (15.4 and TREL).
     * - 2. Node_2 - Reference device that supports multi-radio (15.4 and TREL).
     * - 3. Node_3 - Reference device that supports 15.4 radio only.
     * Devices using TREL MUST be connected to the same infrastructure link.
     * Note: the infrastructure link is not shown in the figure below.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Discovery Scan   | 4.7.2.1      | 4.5.2.1
     */

    Core  nexus;
    Node &node1 = nexus.CreateNode();
    Node &node2 = nexus.CreateNode();
    Node &node3 = nexus.CreateNode();

    DiscoverContext resultContext;

    node1.SetName("NODE_1_DUT");
    node2.SetName("NODE_2");
    node3.SetName("NODE_3");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1
     * - Device: Node_1 (DUT), Node_2, Node_3
     * - Description (TREL-8.5): Form a network on Node_1. Form a different network on Node_2 and different one on
     *   Node_3 (each node has its own network).
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: Form separate networks on Node_1, Node_2, and Node_3");

    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    /** Node_3 supports 15.4 radio only. Disable TREL interface on Node_3. */
    node3.Get<ot::Trel::Interface>().SetEnabled(false, ot::Trel::Interface::kRequesterUser);

    node1.Form();
    node2.Form();
    node3.Form();

    nexus.AdvanceTime(kFormNetworkTime);

    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node2.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node3.Get<Mle::Mle>().IsLeader());

    /**
     * Step 2
     * - Device: Node_2
     * - Description (TREL-8.5): Harness instructs device to perform MLE Discovery Scan.
     * - Pass Criteria:
     *   - Node_2 MUST see both the DUT and Node_3 in its scan result.
     */
    Log("Step 2: Node_2 performs MLE Discovery Scan");

    resultContext.Clear();
    SuccessOrQuit(node2.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(), kPanIdAny, /* aJoiner */ false,
                                                             /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                             HandleDiscoverResult, &resultContext));

    nexus.AdvanceTime(kScanTime);

    VerifyOrQuit(resultContext.mDiscoverDone);
    {
        bool foundNode1 = false;
        bool foundNode3 = false;

        for (const ScanResult &result : resultContext.mScanResults)
        {
            if (result.GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress())
            {
                foundNode1 = true;
            }
            if (result.GetExtAddress() == node3.Get<Mac::Mac>().GetExtAddress())
            {
                foundNode3 = true;
            }
        }

        VerifyOrQuit(foundNode1, "Node_2 did not see Node_1 (DUT) in scan result");
        VerifyOrQuit(foundNode3, "Node_2 did not see Node_3 in scan result");
    }

    /**
     * Step 3
     * - Device: Node_3
     * - Description (TREL-8.5): Harness instructs device to perform MLE Discovery Scan.
     * - Pass Criteria:
     *   - Node_3 MUST see both the DUT and Node_2 in its scan result.
     */
    Log("Step 3: Node_3 performs MLE Discovery Scan");

    resultContext.Clear();
    SuccessOrQuit(node3.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(), kPanIdAny, /* aJoiner */ false,
                                                             /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                             HandleDiscoverResult, &resultContext));

    nexus.AdvanceTime(kScanTime);

    VerifyOrQuit(resultContext.mDiscoverDone);
    {
        bool foundNode1 = false;
        bool foundNode2 = false;

        for (const ScanResult &result : resultContext.mScanResults)
        {
            if (result.GetExtAddress() == node1.Get<Mac::Mac>().GetExtAddress())
            {
                foundNode1 = true;
            }
            if (result.GetExtAddress() == node2.Get<Mac::Mac>().GetExtAddress())
            {
                foundNode2 = true;
            }
        }

        VerifyOrQuit(foundNode1, "Node_3 did not see Node_1 (DUT) in scan result");
        VerifyOrQuit(foundNode2, "Node_3 did not see Node_2 in scan result");
    }

    nexus.SaveTestInfo("test_1_4_TREL_TC_5.json");
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ot::Nexus::Test1_4_Trel_Tc_5();
    printf("All tests passed\n");
#else
    printf("Multi-radio is not enabled - test skipped\n");
#endif
    return 0;
}
