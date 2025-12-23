/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements Filter IEEE 802.15.4 frame filtering based on MAC address.
 */

#include "mac_filter.hpp"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

Filter::Filter(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMode(kModeRssInOnly)
    , mDefaultRssIn(kFixedRssDisabled)
{
    for (FilterEntry &entry : mFilterEntries)
    {
        entry.mFiltered = false;
        entry.mRssIn    = kFixedRssDisabled;
    }
}

void Filter::Restore(void)
{
    uint16_t iterator = 0;
    uint16_t index    = 0;

    SettingsBase::MacFilterEntry  macFilterEntry;
    SettingsBase::MacFilterConfig macFilterConfig;

    while (Get<Settings>().ReadMacFilterEntry(iterator, macFilterEntry) == kErrorNone)
    {
        mFilterEntries[index].mFiltered = macFilterEntry.mFiltered;
        mFilterEntries[index].mRssIn    = macFilterEntry.mRssIn;
        mFilterEntries[index].mExtAddress.Set(macFilterEntry.mExtAddress.m8);

        index++;
        if (index >= kMaxEntries)
        {
            break;
        }
    }

    SuccessOrExit(Get<Settings>().ReadMacFilterConfig(macFilterConfig));
    mMode         = static_cast<Mode>(macFilterConfig.mMode);
    mDefaultRssIn = macFilterConfig.mDefaultRssIn;

exit:
    return;
}

void Filter::Refresh(void)
{
    Error                         error;
    SettingsBase::MacFilterEntry  macFilterEntry;
    SettingsBase::MacFilterConfig macFilterConfig;

    Get<Settings>().DeleteAllMacFilterEntry();

    for (const FilterEntry &entry : mFilterEntries)
    {
        if (!entry.IsInUse())
        {
            continue;
        }

        macFilterEntry.mFiltered = entry.mFiltered;
        macFilterEntry.mRssIn    = entry.mRssIn;
        macFilterEntry.mExtAddress.Set(entry.mExtAddress.m8);

        SuccessOrExit(error = Get<Settings>().AddMacFilterEntry(macFilterEntry));
    }

    Get<Settings>().DeleteMacFilterConfig();

    macFilterConfig.mMode         = mMode;
    macFilterConfig.mDefaultRssIn = mDefaultRssIn;
    error                         = Get<Settings>().WriteMacFilterConfig(macFilterConfig);

exit:
    if (error != kErrorNone)
    {
        Get<Settings>().DeleteAllMacFilterEntry();
        Get<Settings>().DeleteMacFilterConfig();
    }

    return;
}

const Filter::FilterEntry *Filter::FindEntry(const ExtAddress &aExtAddress) const
{
    const FilterEntry *rval = nullptr;

    for (const FilterEntry &entry : mFilterEntries)
    {
        if (entry.IsInUse() && (aExtAddress == entry.mExtAddress))
        {
            ExitNow(rval = &entry);
        }
    }

exit:
    return rval;
}

Filter::FilterEntry *Filter::FindAvailableEntry(void)
{
    FilterEntry *rval = nullptr;

    for (FilterEntry &entry : mFilterEntries)
    {
        if (!entry.IsInUse())
        {
            ExitNow(rval = &entry);
        }
    }

exit:
    return rval;
}

Error Filter::AddAddress(const ExtAddress &aExtAddress)
{
    Error        error = kErrorNone;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == nullptr)
    {
        VerifyOrExit((entry = FindAvailableEntry()) != nullptr, error = kErrorNoBufs);
        entry->mExtAddress = aExtAddress;
    }

    entry->mFiltered = true;
    Refresh();

exit:
    return error;
}

void Filter::RemoveAddress(const ExtAddress &aExtAddress)
{
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry != nullptr)
    {
        entry->mFiltered = false;
        Refresh();
    }
}

void Filter::ClearAddresses(void)
{
    for (FilterEntry &entry : mFilterEntries)
    {
        entry.mFiltered = false;
    }

    Refresh();
}

Error Filter::GetNextAddress(Iterator &aIterator, Entry &aEntry) const
{
    Error error = kErrorNotFound;

    for (; aIterator < GetArrayLength(mFilterEntries); aIterator++)
    {
        const FilterEntry &entry = mFilterEntries[aIterator];

        if (entry.mFiltered)
        {
            aEntry.mExtAddress = entry.mExtAddress;
            aEntry.mRssIn      = entry.mRssIn;
            error              = kErrorNone;
            aIterator++;
            break;
        }
    }

    return error;
}

Error Filter::AddRssIn(const ExtAddress &aExtAddress, int8_t aRss)
{
    Error        error = kErrorNone;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == nullptr)
    {
        entry = FindAvailableEntry();
        VerifyOrExit(entry != nullptr, error = kErrorNoBufs);

        entry->mExtAddress = aExtAddress;
    }

    entry->mRssIn = aRss;

    Refresh();

exit:
    return error;
}

void Filter::RemoveRssIn(const ExtAddress &aExtAddress)
{
    FilterEntry *entry = FindEntry(aExtAddress);

    VerifyOrExit(entry != nullptr);

    entry->mRssIn = kFixedRssDisabled;
    Refresh();

exit:
    return;
}

void Filter::ClearAllRssIn(void)
{
    for (FilterEntry &entry : mFilterEntries)
    {
        entry.mRssIn = kFixedRssDisabled;
    }

    mDefaultRssIn = kFixedRssDisabled;
    Refresh();
}

Error Filter::GetNextRssIn(Iterator &aIterator, Entry &aEntry) const
{
    Error error = kErrorNotFound;

    for (; aIterator < GetArrayLength(mFilterEntries); aIterator++)
    {
        const FilterEntry &entry = mFilterEntries[aIterator];

        if (entry.mRssIn != kFixedRssDisabled)
        {
            aEntry.mExtAddress = entry.mExtAddress;
            aEntry.mRssIn      = entry.mRssIn;
            error              = kErrorNone;
            aIterator++;
            ExitNow();
        }
    }

    // Return the default RssIn at the end of list
    if ((aIterator == GetArrayLength(mFilterEntries)) && (mDefaultRssIn != kFixedRssDisabled))
    {
        AsCoreType(&aEntry.mExtAddress).Fill(0xff);
        aEntry.mRssIn = mDefaultRssIn;
        error         = kErrorNone;
        aIterator++;
    }

exit:
    return error;
}

Error Filter::Apply(const ExtAddress &aExtAddress, int8_t &aRss) const
{
    Error              error = kErrorNone;
    const FilterEntry *entry = FindEntry(aExtAddress);
    bool               isInFilterList;

    // Use the default RssIn setting for all receiving messages first.
    aRss = mDefaultRssIn;

    // In allowlist mode, entry must be present in the list, in
    // denylist mode it must not be present.

    isInFilterList = (entry != nullptr) && entry->mFiltered;

    switch (mMode)
    {
    case kModeRssInOnly:
        break;

    case kModeAllowlist:
        VerifyOrExit(isInFilterList, error = kErrorAddressFiltered);
        break;

    case kModeDenylist:
        VerifyOrExit(!isInFilterList, error = kErrorAddressFiltered);
        break;
    }

    if ((entry != nullptr) && (entry->mRssIn != kFixedRssDisabled))
    {
        aRss = entry->mRssIn;
    }

exit:
    return error;
}

Error Filter::ApplyToRxFrame(RxFrame &aRxFrame, const ExtAddress &aExtAddress, Neighbor *aNeighbor) const
{
    Error  error;
    int8_t fixedRss;

    SuccessOrExit(error = Apply(aExtAddress, fixedRss));

    VerifyOrExit(fixedRss != kFixedRssDisabled);

    aRxFrame.SetRssi(fixedRss);

    if (aNeighbor != nullptr)
    {
        // Clear the previous RSS average to ensure the fixed RSS
        // value takes effect quickly.
        aNeighbor->GetLinkInfo().ClearAverageRss();
    }

exit:
    return error;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
