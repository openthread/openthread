/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

/**
 * @file
 *   This file implements the History Tracker public APIs.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#include <openthread/history_tracker.h>

#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "utils/history_tracker.hpp"

using namespace ot;

void otHistoryTrackerInitIterator(otHistoryTrackerIterator *aIterator)
{
    static_cast<Utils::HistoryTracker::Iterator *>(aIterator)->Init();
}

const otHistoryTrackerNetworkInfo *otHistoryTrackerIterateNetInfoHistory(otInstance *              aInstance,
                                                                         otHistoryTrackerIterator *aIterator,
                                                                         uint32_t *                aEntryAge)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Utils::HistoryTracker>().IterateNetInfoHistory(
        *static_cast<Utils::HistoryTracker::Iterator *>(aIterator), *aEntryAge);
}

const otHistoryTrackerMessageInfo *otHistoryTrackerIterateRxHistory(otInstance *              aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t *                aEntryAge)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Utils::HistoryTracker>().IterateRxHistory(
        *static_cast<Utils::HistoryTracker::Iterator *>(aIterator), *aEntryAge);
}

const otHistoryTrackerMessageInfo *otHistoryTrackerIterateTxHistory(otInstance *              aInstance,
                                                                    otHistoryTrackerIterator *aIterator,
                                                                    uint32_t *                aEntryAge)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Utils::HistoryTracker>().IterateTxHistory(
        *static_cast<Utils::HistoryTracker::Iterator *>(aIterator), *aEntryAge);
}

void otHistoryTrackerEntryAgeToString(uint32_t aEntryAge, char *aBuffer, uint16_t aSize)
{
    Utils::HistoryTracker::EntryAgeToString(aEntryAge, aBuffer, aSize);
}

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
