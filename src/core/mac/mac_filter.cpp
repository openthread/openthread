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
    : mAddressMode(OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
    , mDefaultRssIn(kFixedRssDisabled)
{
    for (FilterEntry *entry = &mFilterEntries[0]; entry < OT_ARRAY_END(mFilterEntries); entry++)
    {
        entry->mFiltered = false;
        entry->mRssIn    = kFixedRssDisabled;
    }
}

Filter::FilterEntry *Filter::FindEntry(const ExtAddress &aExtAddress)
{
    FilterEntry *entry;

    for (entry = &mFilterEntries[0]; entry < OT_ARRAY_END(mFilterEntries); entry++)
    {
        if (entry->IsInUse() && (aExtAddress == entry->mExtAddress))
        {
            ExitNow();
        }
    }

    entry = NULL;

exit:
    return entry;
}

Filter::FilterEntry *Filter::FindAvailableEntry(void)
{
    FilterEntry *entry;

    for (entry = &mFilterEntries[0]; entry < OT_ARRAY_END(mFilterEntries); entry++)
    {
        VerifyOrExit(entry->IsInUse());
    }

    entry = NULL;

exit:
    return entry;
}

otError Filter::SetAddressMode(otMacFilterAddressMode aMode)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aMode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED || aMode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST ||
                     aMode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST,
                 error = OT_ERROR_INVALID_ARGS);

    mAddressMode = aMode;

exit:
    return error;
}

otError Filter::AddAddress(const ExtAddress &aExtAddress)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailableEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        entry->mExtAddress = aExtAddress;
    }

    VerifyOrExit(!entry->mFiltered, error = OT_ERROR_ALREADY);
    entry->mFiltered = true;

exit:
    return error;
}

otError Filter::RemoveAddress(const ExtAddress &aExtAddress)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry = FindEntry(aExtAddress);

    if (entry == NULL || !entry->mFiltered)
    {
        ExitNow(error = OT_ERROR_NOT_FOUND);
    }

    entry->mFiltered = false;

exit:
    return error;
}

void Filter::ClearAddresses(void)
{
    for (FilterEntry *entry = &mFilterEntries[0]; entry < OT_ARRAY_END(mFilterEntries); entry++)
    {
        entry->mFiltered = false;
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

otError Filter::AddRssIn(const ExtAddress *aExtAddress, int8_t aRss)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry;

    // Set the default RssIn when aExtAddress is not given (NULL)
    VerifyOrExit(aExtAddress != NULL, mDefaultRssIn = aRss);

    entry = FindEntry(*aExtAddress);

    if (entry == NULL)
    {
        entry = FindAvailableEntry();
        VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);

        entry->mExtAddress = *aExtAddress;
    }

    entry->mRssIn = aRss;

exit:
    return error;
}

otError Filter::RemoveRssIn(const ExtAddress *aExtAddress)
{
    otError      error = OT_ERROR_NONE;
    FilterEntry *entry;

    // If no aExtAddress is given, remove default RssIn
    VerifyOrExit(aExtAddress != NULL, mDefaultRssIn = kFixedRssDisabled);

    entry = FindEntry(*aExtAddress);
    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);
    entry->mRssIn = kFixedRssDisabled;

exit:
    return error;
}

void Filter::ClearRssIn(void)
{
    for (FilterEntry *entry = &mFilterEntries[0]; entry < OT_ARRAY_END(mFilterEntries); entry++)
    {
        entry->mRssIn = kFixedRssDisabled;
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

    // Use the default RssIn setting for all receiving messages first.
    aRss = mDefaultRssIn;

    if (mAddressMode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
    {
        VerifyOrExit(entry != NULL && entry->mFiltered, error = OT_ERROR_ADDRESS_FILTERED);
    }
    else if (mAddressMode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
    {
        VerifyOrExit(entry == NULL || !entry->mFiltered, error = OT_ERROR_ADDRESS_FILTERED);
    }

    if ((entry != NULL) && (entry->mRssIn != kFixedRssDisabled))
    {
        aRss = entry->mRssIn;
    }

exit:
    return error;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
