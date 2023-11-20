/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include "test_platform.h"

#include <openthread/config.h>

#include "test_util.h"
#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "mac/mac_types.hpp"
#include "thread/child_table.hpp"

namespace ot {

static Instance *sInstance;

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE

using namespace Utils;

static uint32_t sNow = 10000;

extern "C" {

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

} // extern "C"

struct TestChild
{
    Child::State mState;
    otExtAddress mExtAddress;
    uint16_t     mThreadVersion;
};

class UnitTester
{
public:
    static void TestLinkMetricsManager(void);

private:
    static void SetTestLinkMetricsValues(otLinkMetricsValues &aLinkMetricsValues,
                                         uint8_t              aLinkMarginValue,
                                         int8_t               aRssiValue);

    static const TestChild mTestChildList[];
};

const TestChild UnitTester::mTestChildList[] = {
    {
        Child::kStateValid,
        {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x16}},
        kThreadVersion1p2,
    },
    {
        Child::kStateValid,
        {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x17}},
        kThreadVersion1p2,
    },
    {
        Child::kStateParentRequest,
        {{0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x18}},
        kThreadVersion1p2,
    },
};

void UnitTester::SetTestLinkMetricsValues(otLinkMetricsValues &aLinkMetricsValues,
                                          uint8_t              aLinkMarginValue,
                                          int8_t               aRssiValue)
{
    aLinkMetricsValues.mMetrics.mLinkMargin = true;
    aLinkMetricsValues.mMetrics.mRssi       = true;

    aLinkMetricsValues.mLinkMarginValue = aLinkMarginValue;
    aLinkMetricsValues.mRssiValue       = aRssiValue;
}

void UnitTester::TestLinkMetricsManager(void)
{
    ChildTable                  *childTable;
    const uint16_t               testListLength = GetArrayLength(mTestChildList);
    Ip6::Address                 linkLocalAddr;
    LinkMetricsManager::Subject *subject1       = nullptr;
    LinkMetricsManager::Subject *subject2       = nullptr;
    LinkMetricsManager          *linkMetricsMgr = nullptr;
    otLinkMetricsValues          linkMetricsValues;
    otShortAddress               anyShortAddress = 0x1234;

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != nullptr);

    childTable = &sInstance->Get<ChildTable>();

    sInstance->Get<Mle::Mle>().SetRole(Mle::kRoleRouter);
    // Add the child entries from test list
    for (uint16_t i = 0; i < testListLength; i++)
    {
        Child *child;

        child = childTable->GetNewChild();
        VerifyOrQuit(child != nullptr, "GetNewChild() failed");

        child->SetState(mTestChildList[i].mState);
        child->SetExtAddress((AsCoreType(&mTestChildList[i].mExtAddress)));
        child->SetVersion(mTestChildList[i].mThreadVersion);
    }

    linkMetricsMgr = &sInstance->Get<LinkMetricsManager>();
    linkMetricsMgr->SetEnabled(true);
    // Update the subjects for the first time.
    linkMetricsMgr->UpdateSubjects();

    // Expect there are 2 subjects in NotConfigured state.
    {
        uint16_t index = 0;
        for (LinkMetricsManager::Subject &subject : linkMetricsMgr->mSubjectList)
        {
            VerifyOrQuit(subject.mExtAddress == AsCoreType(&mTestChildList[1 - index].mExtAddress));
            VerifyOrQuit(subject.mState == LinkMetricsManager::SubjectState::kNotConfigured);
            index++;
        }
    }
    subject1 = linkMetricsMgr->mSubjectList.FindMatching(AsCoreType(&mTestChildList[0].mExtAddress));
    subject2 = linkMetricsMgr->mSubjectList.FindMatching(AsCoreType(&mTestChildList[1].mExtAddress));
    VerifyOrQuit(subject1 != nullptr);
    VerifyOrQuit(subject2 != nullptr);

    // Update the state of the subjects
    linkMetricsMgr->UpdateLinkMetricsStates();
    // Expect the 2 subjects are in Configuring state.
    for (LinkMetricsManager::Subject &subject : linkMetricsMgr->mSubjectList)
    {
        VerifyOrQuit(subject.mState == LinkMetricsManager::SubjectState::kConfiguring);
    }

    // subject1 received a response with a success status code
    linkLocalAddr.SetToLinkLocalAddress(AsCoreType(&mTestChildList[0].mExtAddress));
    linkMetricsMgr->HandleMgmtResponse(&linkLocalAddr, MapEnum(LinkMetrics::Status::kStatusSuccess));
    VerifyOrQuit(subject1->mState == LinkMetricsManager::SubjectState::kActive);

    // subject1 received Enhanced ACK IE and updated the link metrics data
    {
        constexpr uint8_t kTestLinkMargin = 100;
        constexpr int8_t  kTestRssi       = -30;
        SetTestLinkMetricsValues(linkMetricsValues, kTestLinkMargin, kTestRssi);
        linkMetricsMgr->HandleEnhAckIe(anyShortAddress, &mTestChildList[0].mExtAddress, &linkMetricsValues);
        VerifyOrQuit(subject1->mData.mLinkMargin == kTestLinkMargin);
        VerifyOrQuit(subject1->mData.mRssi == kTestRssi);
    }

    // subject2 didn't receive any responses after a few attempts and marked as UnSupported.
    for (uint8_t i = 0; i < LinkMetricsManager::kConfigureLinkMetricsMaxAttempts; i++)
    {
        linkMetricsMgr->Update();
    }
    VerifyOrQuit(subject2->mState == LinkMetricsManager::SubjectState::kNotSupported);

    // neighbor2 is removed and subject2 should also be removed.
    Child *child2 = childTable->FindChild(subject2->mExtAddress, Child::kInStateValid);
    child2->SetState(Child::kStateInvalid);
    linkMetricsMgr->Update();
    subject2 = linkMetricsMgr->mSubjectList.FindMatching(AsCoreType(&mTestChildList[1].mExtAddress));
    VerifyOrQuit(subject2 == nullptr);

    // subject1 still existed
    subject1 = linkMetricsMgr->mSubjectList.FindMatching(AsCoreType(&mTestChildList[0].mExtAddress));
    VerifyOrQuit(subject1 != nullptr);

    // Make subject1 renew
    sNow += LinkMetricsManager::kStateUpdateIntervalMilliSec + 1;
    linkMetricsMgr->Update();
    VerifyOrQuit(subject1->mState == LinkMetricsManager::SubjectState::kRenewing);

    // subject1 got Enh-ACK IE when in kRenewing state
    sNow += 1;
    linkMetricsMgr->HandleEnhAckIe(anyShortAddress, &mTestChildList[0].mExtAddress, &linkMetricsValues);
    VerifyOrQuit(subject1->mLastUpdateTime == TimeMilli(sNow));

    // subject1 got response and become active again
    linkMetricsMgr->HandleMgmtResponse(&linkLocalAddr, MapEnum(LinkMetrics::Status::kStatusSuccess));
    VerifyOrQuit(subject1->mState == LinkMetricsManager::SubjectState::kActive);
}

#endif // OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
    ot::UnitTester::TestLinkMetricsManager();
#endif
    printf("\nAll tests passed.\n");
    return 0;
}
