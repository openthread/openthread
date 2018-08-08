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

#include "utils/wrap_string.h"

#include "common/code_utils.hpp"

#if OPENTHREAD_ENABLE_MAC_FILTER

namespace ot {
namespace Mac {

Filter::Filter(void)
    : mAddressMode(OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
    , mRssIn(OT_MAC_FILTER_FIXED_RSS_DISABLED)
{
    for (int i = 0; i < GetMaxEntries(); i++)
    {
        memset(&mEntries[i], 0, sizeof(Entry));

        mEntries[i].mFiltered = false;
        mEntries[i].mRssIn    = OT_MAC_FILTER_FIXED_RSS_DISABLED;
    }
}

Filter::Entry *Filter::FindEntry(const ExtAddress &aExtAddress)
{
    Entry *entry = NULL;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if ((mEntries[i].mFiltered || mEntries[i].mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED) &&
            (aExtAddress == static_cast<const ExtAddress &>(mEntries[i].mExtAddress)))
        {
            ExitNow(entry = &mEntries[i]);
        }
    }

exit:
    return entry;
}

Filter::Entry *Filter::FindAvailEntry(void)
{
    Entry *entry = NULL;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if (!mEntries[i].mFiltered && mEntries[i].mRssIn == OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            ExitNow(entry = &mEntries[i]);
        }
    }

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
    otError error = OT_ERROR_NONE;
    Entry * entry = FindEntry(aExtAddress);

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        entry->mExtAddress = aExtAddress;
    }

    if (entry->mFiltered)
    {
        ExitNow(error = OT_ERROR_ALREADY);
    }

    entry->mFiltered = true;

exit:
    return error;
}

otError Filter::RemoveAddress(const ExtAddress &aExtAddress)
{
    otError error = OT_ERROR_NONE;
    Entry * entry = FindEntry(aExtAddress);

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
    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mEntries[i].mFiltered = false;
    }
}

otError Filter::GetNextAddress(otMacFilterIterator &aIterator, Entry &aEntry)
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t i     = *reinterpret_cast<uint8_t *>(&aIterator);

    for (; i < GetMaxEntries(); i++)
    {
        if (mEntries[i].mFiltered)
        {
            aEntry    = mEntries[i];
            aIterator = *reinterpret_cast<otMacFilterIterator *>(&(++i));
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

otError Filter::AddRssIn(const ExtAddress *aExtAddress, int8_t aRss)
{
    otError error = OT_ERROR_NONE;

    // set the default RssIn for all received messages.
    if (aExtAddress == NULL)
    {
        mRssIn = aRss;
        ExitNow();
    }
    else
    {
        Entry *entry = FindEntry(*aExtAddress);

        if (entry == NULL)
        {
            VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
            entry->mExtAddress = static_cast<const otExtAddress &>(*aExtAddress);
        }

        entry->mRssIn = aRss;
    }

exit:
    return error;
}

otError Filter::RemoveRssIn(const ExtAddress *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    if (aExtAddress == NULL)
    {
        mRssIn = OT_MAC_FILTER_FIXED_RSS_DISABLED;
    }
    else
    {
        Entry *entry = FindEntry(*aExtAddress);
        VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);
        entry->mRssIn = OT_MAC_FILTER_FIXED_RSS_DISABLED;
    }

exit:
    return error;
}

void Filter::ClearRssIn(void)
{
    mRssIn = OT_MAC_FILTER_FIXED_RSS_DISABLED;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mEntries[i].mRssIn = OT_MAC_FILTER_FIXED_RSS_DISABLED;
    }
}

otError Filter::GetNextRssIn(otMacFilterIterator &aIterator, Entry &aEntry)
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t i     = *reinterpret_cast<uint8_t *>(&aIterator);

    for (; i < GetMaxEntries(); i++)
    {
        if (mEntries[i].mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            aEntry    = mEntries[i];
            aIterator = *reinterpret_cast<otMacFilterIterator *>(&(++i));
            ExitNow(error = OT_ERROR_NONE);
        }
    }

    // return default rssin setting if no more rssin filter entry.
    if (i == GetMaxEntries() && mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        memset(&aEntry.mExtAddress, 0xff, OT_EXT_ADDRESS_SIZE);
        aEntry.mRssIn = mRssIn;
        aIterator     = *reinterpret_cast<otMacFilterIterator *>(&(++i));
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError Filter::Apply(const ExtAddress &aExtAddress, int8_t &aRss)
{
    otError error = OT_ERROR_NONE;

    otMacFilterEntry *entry = FindEntry(aExtAddress);

    // assign the default RssIn setting for all receiving messages first.
    aRss = mRssIn;

    // check AddressFilter.
    if (mAddressMode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
    {
        VerifyOrExit(entry != NULL && entry->mFiltered, error = OT_ERROR_ADDRESS_FILTERED);
    }
    else if (mAddressMode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
    {
        VerifyOrExit(entry == NULL || !entry->mFiltered, error = OT_ERROR_ADDRESS_FILTERED);
    }

    // not override the default RssIn setting if no specific RssIn on the Extended Address.
    if (entry != NULL && entry->mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        aRss = entry->mRssIn;
    }

exit:
    return error;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_ENABLE_MAC_FILTER
