/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements IPv6 flow label table.
 */

#include "ip6_flow_labels.hpp"

#include "common/instance.hpp"
#include "common/owner-locator.hpp"

#if OPENTHREAD_ENABLE_IP6_FLOW_LABELS

namespace ot {
namespace Ip6 {

FlowLabels::FlowLabels(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance, &FlowLabels::HandleTimer, this)
{
    memset(mEntries, 0, sizeof(mEntries));
}

bool FlowLabels::ContainsFlowLabel(uint32_t aFlowLabel)
{
    return (FindFlowLabelEntry(aFlowLabel, true) != NULL) ? true : false;
}

otError FlowLabels::AddFlowLabel(uint32_t aFlowLabel)
{
    otError         error = OT_ERROR_NONE;
    FlowLabelEntry *entry;

    VerifyOrExit((aFlowLabel & static_cast<uint32_t>(~kFlowLabelMask)) == 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(FindFlowLabelEntry(aFlowLabel, true) == NULL, error = OT_ERROR_ALREADY);
    VerifyOrExit((entry = FindFlowLabelEntry(0, false)) != NULL, error = OT_ERROR_NO_BUFS);

    entry->mFlowLabel = aFlowLabel;
    entry->mValid     = true;
    entry->mDelay     = 0;

exit:
    return error;
}

otError FlowLabels::RemoveFlowLabel(uint32_t aFlowLabel, uint8_t aDelay)
{
    otError         error = OT_ERROR_NONE;
    FlowLabelEntry *entry;

    VerifyOrExit((entry = FindFlowLabelEntry(aFlowLabel, true)) != NULL, error = OT_ERROR_NOT_FOUND);

    entry->mDelay = aDelay;
    if (entry->mDelay == 0)
    {
        entry->mValid = false;
    }
    else if (!mTimer.IsRunning())
    {
        mTimer.Start(kUpdatePeriod);
    }

exit:
    return error;
}

otError FlowLabels::GetNextFlowLabel(otIp6FlowLabelIterator &aIterator, uint32_t &aFlowLabel)
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t index = *reinterpret_cast<uint8_t *>(&aIterator);

    while (index < OT_ARRAY_LENGTH(mEntries))
    {
        if (mEntries[index].mValid)
        {
            aFlowLabel = mEntries[index++].mFlowLabel;
            ExitNow(error = OT_ERROR_NONE);
        }

        index++;
    }

exit:
    aIterator = *reinterpret_cast<otIp6FlowLabelIterator *>(&index);

    return error;
}

void FlowLabels::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<FlowLabels>().HandleTimer();
}

void FlowLabels::HandleTimer(void)
{
    bool shouldRun = false;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if (mEntries[i].mValid && (mEntries[i].mDelay > 0))
        {
            mEntries[i].mDelay--;

            if (mEntries[i].mDelay == 0)
            {
                mEntries[i].mValid = false;
            }
            else
            {
                shouldRun = true;
            }
        }
    }

    if (shouldRun)
    {
        mTimer.Start(kUpdatePeriod);
    }
}

FlowLabels::FlowLabelEntry *FlowLabels::FindFlowLabelEntry(uint32_t aFlowLabel, bool aValid)
{
    FlowLabelEntry *entry = NULL;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if ((mEntries[i].mValid == aValid) && (!aValid || (mEntries[i].mFlowLabel == aFlowLabel)))
        {
            ExitNow(entry = &mEntries[i]);
        }
    }

exit:
    return entry;
}

} // namespace Ip6
} // namespace ot

#endif // OPENTHREAD_ENABLE_IP6_FLOW_LABELS
