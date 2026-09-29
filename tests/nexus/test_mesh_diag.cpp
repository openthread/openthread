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

using MeshDiag = ot::Utils::MeshDiag;

static constexpr uint16_t kMaxEntries = 8;

struct DiscoveredRouter
{
    bool                                    mValid;
    uint16_t                                mRloc16;
    bool                                    mIsThisDevice;
    bool                                    mIsLeader;
    uint16_t                                mVersion;
    Array<Ip6::Address, kMaxEntries>        mIp6Addrs;
    Array<MeshDiag::ChildInfo, kMaxEntries> mChildren;
    Array<uint8_t, kMaxEntries>             mExtraTlvTypes;
};

static Error                                sLastCallbackError;
static Array<DiscoveredRouter, kMaxEntries> sDiscoveredRouters;

static void ResetTest(void)
{
    sLastCallbackError = kErrorNone;
    sDiscoveredRouters.Clear();
}

static void HandleDiscoverCallback(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext)
{
    DiscoveredRouter *router;

    VerifyOrQuit(aContext == nullptr);

    sLastCallbackError = aError;

    VerifyOrExit(aRouterInfo != nullptr);

    router = sDiscoveredRouters.PushBack();

    VerifyOrQuit(router != nullptr, "sDiscoveredRouters is full");

    router->mValid        = true;
    router->mRloc16       = aRouterInfo->mRloc16;
    router->mIsThisDevice = aRouterInfo->mIsThisDevice;
    router->mIsLeader     = aRouterInfo->mIsLeader;
    router->mVersion      = aRouterInfo->mVersion;
    router->mIp6Addrs.Clear();
    router->mChildren.Clear();
    router->mExtraTlvTypes.Clear();

    if (aRouterInfo->mIp6AddrIterator != nullptr)
    {
        MeshDiag::Ip6AddrIterator &ip6Iterator = AsCoreType(aRouterInfo->mIp6AddrIterator);
        Ip6::Address               ip6Addr;

        while (ip6Iterator.GetNextAddress(ip6Addr) == kErrorNone)
        {
            SuccessOrQuit(router->mIp6Addrs.PushBack(ip6Addr));
        }
    }

    if (aRouterInfo->mChildIterator != nullptr)
    {
        MeshDiag::ChildIterator &childIterator = AsCoreType(aRouterInfo->mChildIterator);
        MeshDiag::ChildInfo      childInfo;

        while (childIterator.GetNextChildInfo(childInfo) == kErrorNone)
        {
            SuccessOrQuit(router->mChildren.PushBack(childInfo));
        }
    }

    if (aRouterInfo->mTlvIterator != nullptr)
    {
        MeshDiag::TlvIterator &tlvIterator = AsCoreType(aRouterInfo->mTlvIterator);
        MeshDiag::DiagTlvInfo  tlvInfo;

        while (tlvIterator.GetNextTlvInfo(tlvInfo) == kErrorNone)
        {
            SuccessOrQuit(router->mExtraTlvTypes.PushBack(tlvInfo.mType));
        }
    }

exit:
    return;
}

void TestMeshDiag(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &child1  = nexus.CreateNode();
    Node &child2  = nexus.CreateNode();

    MeshDiag::DiscoverConfig config;
    Array<uint8_t, 3>        extraTlvTypes;
    Array<uint8_t, 1>        forbiddenTlvTypes;
    bool                     foundLeader;
    bool                     foundRouter1;

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, router1);
    AllowLinkBetween(router1, child1);
    AllowLinkBetween(router1, child2);

    leader.Form();
    nexus.AdvanceTime(15 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(5 * Time::kOneMinuteInMsec);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    child1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);
    child2.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(child1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(child2.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Test Scenario 1: Base Discovery (both config flags false, no extra TLVs)");

    ResetTest();

    ClearAllBytes(config);

    SuccessOrQuit(leader.Get<Utils::MeshDiag>().DiscoverTopology(config, HandleDiscoverCallback, nullptr));

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    SuccessOrQuit(sLastCallbackError);
    VerifyOrQuit(sDiscoveredRouters.GetLength() == 2);

    foundLeader  = false;
    foundRouter1 = false;

    for (DiscoveredRouter &router : sDiscoveredRouters)
    {
        VerifyOrQuit(router.mValid);

        if (router.mRloc16 == leader.Get<Mle::Mle>().GetRloc16())
        {
            foundLeader = true;
            VerifyOrQuit(router.mIsLeader);
            VerifyOrQuit(router.mIsThisDevice);
        }
        else if (router.mRloc16 == router1.Get<Mle::Mle>().GetRloc16())
        {
            foundRouter1 = true;
            VerifyOrQuit(!router.mIsLeader);
            VerifyOrQuit(!router.mIsThisDevice);
        }
        else
        {
            VerifyOrQuit(false, "Discovered unexpected router RLOC16");
        }

        VerifyOrQuit(router.mIp6Addrs.IsEmpty());
        VerifyOrQuit(router.mChildren.IsEmpty());
        VerifyOrQuit(router.mExtraTlvTypes.IsEmpty());
    }

    VerifyOrQuit(foundLeader);
    VerifyOrQuit(foundRouter1);

    Log("---------------------------------------------------------------------------------------");
    Log("Test Scenario 2: Discover IPv6 and Child Tables (both config flags true)");

    ResetTest();

    ClearAllBytes(config);
    config.mDiscoverIp6Addresses = true;
    config.mDiscoverChildTable   = true;

    SuccessOrQuit(leader.Get<Utils::MeshDiag>().DiscoverTopology(config, HandleDiscoverCallback, nullptr));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    SuccessOrQuit(sLastCallbackError);
    VerifyOrQuit(sDiscoveredRouters.GetLength() == 2);

    foundLeader  = false;
    foundRouter1 = false;

    for (DiscoveredRouter &router : sDiscoveredRouters)
    {
        VerifyOrQuit(router.mValid);
        if (router.mRloc16 == leader.Get<Mle::Mle>().GetRloc16())
        {
            foundLeader = true;
            VerifyOrQuit(router.mIsLeader);
            VerifyOrQuit(router.mIsThisDevice);

            // Leader has no children in this topology (both children joined Router 1)
            VerifyOrQuit(router.mChildren.IsEmpty());
        }
        else if (router.mRloc16 == router1.Get<Mle::Mle>().GetRloc16())
        {
            foundRouter1 = true;
            VerifyOrQuit(!router.mIsLeader);
            VerifyOrQuit(!router.mIsThisDevice);

            // Router 1 should have 2 children (Child 1 and Child 2)
            VerifyOrQuit(router.mChildren.GetLength() == 2);
            VerifyOrQuit(router.mChildren[0].mRloc16 == child1.Get<Mle::Mle>().GetRloc16() ||
                         router.mChildren[0].mRloc16 == child2.Get<Mle::Mle>().GetRloc16());
            VerifyOrQuit(router.mChildren[1].mRloc16 == child1.Get<Mle::Mle>().GetRloc16() ||
                         router.mChildren[1].mRloc16 == child2.Get<Mle::Mle>().GetRloc16());
        }
        else
        {
            VerifyOrQuit(false, "Discovered unexpected router RLOC16");
        }

        // Both routers should have returned IPv6 addresses
        VerifyOrQuit(!router.mIp6Addrs.IsEmpty());
    }

    VerifyOrQuit(foundLeader);
    VerifyOrQuit(foundRouter1);

    Log("---------------------------------------------------------------------------------------");
    Log("Test Scenario 3: Discover Extra Custom TLVs");

    ResetTest();

    extraTlvTypes.Clear();
    SuccessOrQuit(extraTlvTypes.PushBack(NetDiag::Tlv::kVendorName));
    SuccessOrQuit(extraTlvTypes.PushBack(NetDiag::Tlv::kVendorModel));
    SuccessOrQuit(extraTlvTypes.PushBack(NetDiag::Tlv::kVendorSwVersion));

    ClearAllBytes(config);
    config.mDiscoverIp6Addresses = false;
    config.mDiscoverChildTable   = false;
    config.mExtraTlvTypes        = extraTlvTypes.GetArrayBuffer();
    config.mExtraTlvTypesLength  = extraTlvTypes.GetLength();

    SuccessOrQuit(leader.Get<Utils::MeshDiag>().DiscoverTopology(config, HandleDiscoverCallback, nullptr));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    SuccessOrQuit(sLastCallbackError);
    VerifyOrQuit(sDiscoveredRouters.GetLength() == 2);

    for (DiscoveredRouter &router : sDiscoveredRouters)
    {
        VerifyOrQuit(router.mValid);
        VerifyOrQuit(router.mIp6Addrs.IsEmpty());
        VerifyOrQuit(router.mChildren.IsEmpty());

        // Expect extra TLVs to match the length of requested extra TLVs
        VerifyOrQuit(router.mExtraTlvTypes.GetLength() == extraTlvTypes.GetLength());

        for (uint8_t tlvType : extraTlvTypes)
        {
            VerifyOrQuit(router.mExtraTlvTypes.Contains(tlvType));
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test Scenario 4: Error/Validation checks for extra TLV types");

    forbiddenTlvTypes.Clear();
    SuccessOrQuit(forbiddenTlvTypes.PushBack(NetDiag::Tlv::kAddress16));

    ClearAllBytes(config);
    config.mDiscoverIp6Addresses = false;
    config.mDiscoverChildTable   = false;
    config.mExtraTlvTypes        = forbiddenTlvTypes.GetArrayBuffer();
    config.mExtraTlvTypesLength  = forbiddenTlvTypes.GetLength();

    VerifyOrQuit(leader.Get<Utils::MeshDiag>().DiscoverTopology(config, HandleDiscoverCallback, nullptr) ==
                 kErrorInvalidArgs);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMeshDiag();
    printf("All tests passed\n");
    return 0;
}
