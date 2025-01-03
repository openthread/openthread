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

#include <openthread/platform/alarm-milli.h>

#include "nexus_node.hpp"
#include "nexus_settings.hpp"

namespace ot {
namespace Nexus {

//---------------------------------------------------------------------------------------------------------------------
// otPlatSettings APIs

extern "C" {

void otPlatSettingsInit(otInstance *, const uint16_t *, uint16_t) {}
void otPlatSettingsDeinit(otInstance *) {}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    return AsNode(aInstance).mSettings.Get(aKey, aIndex, aValue, aValueLength);
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return AsNode(aInstance).mSettings.SetOrAdd(Settings::kSet, aKey, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return AsNode(aInstance).mSettings.SetOrAdd(Settings::kAdd, aKey, aValue, aValueLength);
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    return AsNode(aInstance).mSettings.Delete(aKey, aIndex);
}

void otPlatSettingsWipe(otInstance *aInstance) { AsNode(aInstance).mSettings.Wipe(); }

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Settings

Error Settings::Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
{
    Error               error = kErrorNone;
    const Entry        *entry;
    const Entry::Value *value;
    IndexMatcher        IndexMatcher(aIndex);

    entry = mEntries.FindMatching(aKey);
    VerifyOrExit(entry != nullptr, error = kErrorNotFound);

    value = entry->mValues.FindMatching(IndexMatcher);
    VerifyOrExit(value != nullptr, error = kErrorNotFound);

    if (aValueLength != nullptr)
    {
        uint16_t size   = *aValueLength;
        uint16_t length = value->mData.GetLength();

        *aValueLength = length;

        if (aValue != nullptr)
        {
            memcpy(aValue, value->mData.GetBytes(), Min(size, length));
        }
    }

exit:
    return error;
}

Error Settings::SetOrAdd(SetAddMode aMode, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    Entry        *entry;
    Entry::Value *value;

    entry = mEntries.FindMatching(aKey);

    if (entry == nullptr)
    {
        entry = Settings::Entry::Allocate();
        VerifyOrQuit(entry != nullptr);
        entry->mKey = aKey;
        mEntries.Push(*entry);
    }

    value = Entry::Value::Allocate();
    VerifyOrQuit(value != nullptr);
    SuccessOrQuit(value->mData.SetFrom(aValue, aValueLength));

    if (aMode == kSet)
    {
        entry->mValues.Clear();
    }

    entry->mValues.Push(*value);

    return kErrorNone;
}

Error Settings::Delete(uint16_t aKey, int aIndex)
{
    Error         error = kErrorNone;
    Entry        *entry;
    Entry::Value *preValue;

    entry = mEntries.FindMatching(aKey);
    VerifyOrExit(entry != nullptr, error = kErrorNotFound);

    if (aIndex < 0)
    {
        mEntries.RemoveMatching(aKey);
    }
    else
    {
        IndexMatcher           indexMatcher(aIndex);
        OwnedPtr<Entry::Value> valuePtr;

        valuePtr = entry->mValues.RemoveMatching(indexMatcher);
        VerifyOrExit(valuePtr != nullptr, error = kErrorNotFound);
    }

exit:
    return error;
}

void Settings::Wipe(void) { mEntries.Clear(); }

} // namespace Nexus
} // namespace ot
