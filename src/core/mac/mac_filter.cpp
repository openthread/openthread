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

#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

namespace ot {
namespace Mac {

Filter::Filter(void)
    : mMode(kModeRssInOnly)
    , mDefaultRssIn(kFixedRssDisabled)
{
    for (FilterEntry &entry : mFilterEntries)
    {
        entry.mFiltered = false;
        entry.mRssIn    = kFixedRssDisabled;
    }
}

Filter::FilterEntry *Filter::FindEntry(const ExtAddress &aExtAddress)
{
    FilterEntry *rval = nullptr;

    for (FilterEntry &entry : mFilterEntries)
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

otError Filter::AddAddress(const ExtAddress &aExtAddress)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == nullptr)
    {
        VerifyOrExit((entry = FindAvailableEntry()) != nullptr, error = OT_ERROR_NO_BUFS);
        entry->mExtAddress = aExtAddress;
    }

    entry->mFiltered = true;

exit:
    return error;
}

void Filter::RemoveAddress(const ExtAddress &aExtAddress)
{
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry != nullptr)
    {
        entry->mFiltered = false;
    }
}

void Filter::ClearAddresses(void)
{
    for (FilterEntry &entry : mFilterEntries)
    {
        entry.mFiltered = false;
    }
}

otError Filter::GetNextAddress(Iterator &aIterator, Entry &aEntry) const
{
    otError error = OT_ERROR_NOT_FOUND;

    for (; aIterator < OT_ARRAY_LENGTH(mFilterEntries); aIterator++)
    {
        const FilterEntry &entry = mFilterEntries[aIterator];

        if (entry.mFiltered)
        {
            aEntry.mExtAddress = entry.mExtAddress;
            aEntry.mRssIn      = entry.mRssIn;
            error              = OT_ERROR_NONE;
            aIterator++;
            break;
        }
    }

    return error;
}

otError Filter::AddRssIn(const ExtAddress &aExtAddress, int8_t aRss)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == nullptr)
    {
        entry = FindAvailableEntry();
        VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

        entry->mExtAddress = aExtAddress;
    }

    entry->mRssIn = aRss;

exit:
    return error;
}

void Filter::RemoveRssIn(const ExtAddress &aExtAddress)
{
    FilterEntry *entry = FindEntry(aExtAddress);

    VerifyOrExit(entry != nullptr, OT_NOOP);

    entry->mRssIn = kFixedRssDisabled;

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
}

otError Filter::GetNextRssIn(Iterator &aIterator, Entry &aEntry)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (; aIterator < OT_ARRAY_LENGTH(mFilterEntries); aIterator++)
    {
        FilterEntry &entry = mFilterEntries[aIterator];

        if (entry.mRssIn != kFixedRssDisabled)
        {
            aEntry.mExtAddress = entry.mExtAddress;
            aEntry.mRssIn      = entry.mRssIn;
            error              = OT_ERROR_NONE;
            aIterator++;
            ExitNow();
        }
    }

    // Return the default RssIn at the end of list
    if ((aIterator == OT_ARRAY_LENGTH(mFilterEntries)) && (mDefaultRssIn != kFixedRssDisabled))
    {
        static_cast<ExtAddress &>(aEntry.mExtAddress).Fill(0xff);
        aEntry.mRssIn = mDefaultRssIn;
        error         = OT_ERROR_NONE;
        aIterator++;
    }

exit:
    return error;
}

otError Filter::Apply(const ExtAddress &aExtAddress, int8_t &aRss)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry = FindEntry(aExtAddress);
    bool         isInFilterList;

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
        VerifyOrExit(isInFilterList, error = OT_ERROR_ADDRESS_FILTERED);
        break;

    case kModeDenylist:
        VerifyOrExit(!isInFilterList, error = OT_ERROR_ADDRESS_FILTERED);
        break;
    }

    if ((entry != nullptr) && (entry->mRssIn != kFixedRssDisabled))
    {
        aRss = entry->mRssIn;
    }

exit:
    return error;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
