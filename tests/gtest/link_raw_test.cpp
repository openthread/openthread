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
#include <openthread/link_raw.h>
#include <openthread/thread.h>
#include <openthread/platform/provisional/radio.h>
#include <openthread/platform/time.h>
#include "gmock/gmock.h"

#include "fake_platform.hpp"

using namespace ot;

static constexpr uint8_t kNumSlotEntries = 2;

static uint64_t    sTimestamp  = 0;
static uint8_t     sNumEntries = 0;
static otSlotEntry sSlotEntries[kNumSlotEntries];

void LinkRawRadioAvailMapUpdated(otInstance        *aInstance,
                                 uint64_t           aTimestamp,
                                 const otSlotEntry *aSlotEntries,
                                 uint8_t            aNumEntries)
{
    sTimestamp  = aTimestamp;
    sNumEntries = aNumEntries;

    if (aNumEntries <= kNumSlotEntries)
    {
        memcpy(sSlotEntries, aSlotEntries, sizeof(otSlotEntry) * aNumEntries);
    }
}

void LinkRawReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

TEST(otLinkRawSetRadioAvailMapUpdated, shouldUpdateRadioAvailabilityMap)
{
    static constexpr uint64_t kTimestamp = 10000000;

    otError      error;
    FakePlatform fakePlatform;
    otSlotEntry  initSlotEntries[kNumSlotEntries] = {{OT_SLOT_TYPE_ALLOWED, 10}, {OT_SLOT_TYPE_NOT_ALLOWED, 20}};

    sTimestamp  = 0;
    sNumEntries = 0;

    otLinkRawSetRadioAvailMapUpdated(FakePlatform::CurrentInstance(), LinkRawRadioAvailMapUpdated);
    ASSERT_EQ(sTimestamp, 0);
    ASSERT_EQ(sNumEntries, 0);

    fakePlatform.UpdateRadioAvailMap(kTimestamp, initSlotEntries, kNumSlotEntries);
    ASSERT_EQ(sTimestamp, 0);
    ASSERT_EQ(sNumEntries, 0);

    // Enable the link raw module
    error = otLinkRawSetReceiveDone(FakePlatform::CurrentInstance(), LinkRawReceiveDone);
    ASSERT_EQ(error, OT_ERROR_NONE);

    fakePlatform.UpdateRadioAvailMap(kTimestamp, initSlotEntries, kNumSlotEntries);
    ASSERT_EQ(sTimestamp, kTimestamp);
    ASSERT_EQ(sNumEntries, kNumSlotEntries);
    ASSERT_EQ(memcmp(initSlotEntries, sSlotEntries, kNumSlotEntries * sizeof(otSlotEntry)), 0);
}
