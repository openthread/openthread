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
#include "mock_callback.hpp"

using namespace ot;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::Truly;

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

TEST(otDatasetIsValid, shouldReturnTrueForValidActiveDataset)
{
    FakePlatform             fakePlatform;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    EXPECT_EQ(otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset), OT_ERROR_NONE);
    otDatasetConvertToTlvs(&dataset, &datasetTlvs);

    EXPECT_TRUE(otDatasetIsValid(&datasetTlvs, true));
}

TEST(otDatasetIsValid, shouldReturnFalseForIncompleteActiveDataset)
{
    FakePlatform             fakePlatform;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    EXPECT_EQ(otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset), OT_ERROR_NONE);

    // Remove a required field, e.g., Network Key
    dataset.mComponents.mIsNetworkKeyPresent = false;

    otDatasetConvertToTlvs(&dataset, &datasetTlvs);

    EXPECT_FALSE(otDatasetIsValid(&datasetTlvs, true));
}

TEST(otDatasetIsValid, shouldReturnFalseForDuplicatedTlvs)
{
    FakePlatform             fakePlatform;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    EXPECT_EQ(otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset), OT_ERROR_NONE);
    otDatasetConvertToTlvs(&dataset, &datasetTlvs);

    // Duplicate a TLV (e.g. Network Name TLV)
    // TLV format: Type (1 byte), Length (1 byte), Value (n bytes)
    datasetTlvs.mTlvs[datasetTlvs.mLength]     = OT_MESHCOP_TLV_NETWORKNAME; // Type: Network Name
    datasetTlvs.mTlvs[datasetTlvs.mLength + 1] = 1;                          // Length: 1
    datasetTlvs.mTlvs[datasetTlvs.mLength + 2] = 'A';
    datasetTlvs.mLength += 3;

    EXPECT_FALSE(otDatasetIsValid(&datasetTlvs, true));
}

TEST(otDatasetIsValid, shouldReturnTrueForValidPendingDataset)
{
    FakePlatform             fakePlatform;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    EXPECT_EQ(otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset), OT_ERROR_NONE);

    // Pending Dataset MUST contain Pending Timestamp and Delay Timer.
    dataset.mPendingTimestamp.mSeconds             = 100;
    dataset.mPendingTimestamp.mTicks               = 0;
    dataset.mComponents.mIsPendingTimestampPresent = true;
    dataset.mDelay                                 = 30000;
    dataset.mComponents.mIsDelayPresent            = true;

    otDatasetConvertToTlvs(&dataset, &datasetTlvs);

    EXPECT_TRUE(otDatasetIsValid(&datasetTlvs, false));
}

TEST(otDatasetIsValid, shouldReturnFalseForIncompletePendingDataset)
{
    FakePlatform             fakePlatform;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    EXPECT_EQ(otDatasetCreateNewNetwork(FakePlatform::CurrentInstance(), &dataset), OT_ERROR_NONE);

    // Provide Pending Timestamp but omit Delay Timer.
    dataset.mPendingTimestamp.mSeconds             = 100;
    dataset.mPendingTimestamp.mTicks               = 0;
    dataset.mComponents.mIsPendingTimestampPresent = true;

    otDatasetConvertToTlvs(&dataset, &datasetTlvs);

    EXPECT_FALSE(otDatasetIsValid(&datasetTlvs, false));
}
