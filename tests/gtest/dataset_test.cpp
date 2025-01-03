/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <openthread/border_agent.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include <openthread/platform/time.h>
#include "gmock/gmock.h"

#include "fake_platform.hpp"

using namespace ot;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::MockFunction;
using ::testing::Truly;

template <typename R, typename... A> class MockCallback : public testing::MockFunction<R(A...)>
{
public:
    static R CallWithContext(A... aArgs, void *aContext)
    {
        return static_cast<MockCallback *>(aContext)->Call(aArgs...);
    };
};

TEST(otDatasetSetActiveTlvs, shouldTriggerStateCallbackOnSuccess)
{
    FakePlatform fakePlatform;

    typedef MockCallback<void, otChangedFlags> MockStateCallback;

    MockStateCallback mockStateCallback;

    EXPECT_CALL(mockStateCallback, Call).Times(AnyNumber());

    EXPECT_CALL(mockStateCallback, Call(Truly([](otChangedFlags aChangedFlags) -> bool {
                    return (aChangedFlags & OT_CHANGED_ACTIVE_DATASET);
                })))
        .Times(AtLeast(1));

    otError error = otSetStateChangedCallback(FakePlatform::CurrentInstance(), MockStateCallback::CallWithContext,
                                              &mockStateCallback);
    assert(error == OT_ERROR_NONE);

    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    error = otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset);
    assert(error == OT_ERROR_NONE);

    otDatasetConvertToTlvs(&dataset, &datasetTlvs);
    error = otDatasetSetActiveTlvs(FakePlatform::CurrentInstance(), &datasetTlvs);
    assert(error == OT_ERROR_NONE);

    fakePlatform.GoInMs(10000);
}
