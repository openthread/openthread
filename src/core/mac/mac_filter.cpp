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

#include <openthread/config.h>


#include "utils/wrap_string.h"
#include "openthread/types.h"
#include "common/code_utils.hpp"

#if OPENTHREAD_ENABLE_MAC_FILTER
#include "mac_filter.hpp"

namespace ot {
namespace Mac {

Filter::Filter(void) :
    mAddressFilterState(OT_MAC_ADDRESSFILTER_DISABLED),
    mRssiIn(OT_RSSI_OVERRIDE_DISABLED)
{
    for (int i = 0; i < kMaxEntries; i++)
    {
        mFilterEntries[i].mFiltered = false;
        mFilterEntries[i].mFixedRssi = false;
    }
}

Filter::Entry *Filter::AddressFilterFindEntry(const ExtAddress *aAddress)
{
    Entry *entry = NULL;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if (mFilterEntries[i].mFiltered &&
            memcmp(aAddress, &mFilterEntries[i].mExtAddress, OT_EXT_ADDRESS_SIZE) == 0)
        {
            ExitNow(entry = &mFilterEntries[i]);
        }
    }

exit:
    return entry;
}

Filter::Entry *Filter::RssiInFilterFindEntry(const ExtAddress *aAddress)
{
    Entry *entry = NULL;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if (mFilterEntries[i].mFixedRssi &&
            memcmp(aAddress, &mFilterEntries[i].mExtAddress, OT_EXT_ADDRESS_SIZE) == 0)
        {
            ExitNow(entry = &mFilterEntries[i]);
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
        if (!mFilterEntries[i].mFiltered && !mFilterEntries[i].mFixedRssi)
        {
            ExitNow(entry = &mFilterEntries[i]);
        }
    }

exit:
    return entry;
}

otError Filter::AddEntry(const ExtAddress *aAddress, int8_t aRssi)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    VerifyOrExit(mAddressFilterState != OT_MAC_ADDRESSFILTER_BLACKLIST, error = OT_ERROR_INVALID_STATE);

    if ((entry = AddressFilterFindEntry(aAddress)) == NULL)
    {
        entry = RssiInFilterFindEntry(aAddress);
    }

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        memcpy(&entry->mExtAddress, aAddress, OT_EXT_ADDRESS_SIZE);
    }

    if (aRssi != OT_RSSI_OVERRIDE_DISABLED)
    {
        entry->mFixedRssi = true;
        entry->mRssi = aRssi;
    }

    entry->mFiltered = true;

exit:
    return error;
}

otError Filter::AddressFilterSetState(uint8_t aState)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mAddressFilterState == OT_MAC_ADDRESSFILTER_DISABLED ||
                 aState == OT_MAC_ADDRESSFILTER_DISABLED ||
                 aState == mAddressFilterState,
                 error = OT_ERROR_INVALID_STATE);

    mAddressFilterState = aState;
exit:
    return error;
}

otError Filter::AddressFilterAddEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    VerifyOrExit((entry = AddressFilterFindEntry(aAddress)) == NULL, error = OT_ERROR_ALREADY);

    if (mAddressFilterState != OT_MAC_ADDRESSFILTER_BLACKLIST)
    {
        entry = RssiInFilterFindEntry(aAddress);
    }

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        memcpy(&entry->mExtAddress, aAddress, OT_EXT_ADDRESS_SIZE);
    }

    entry->mFiltered = true;

exit:
    return error;
}

otError Filter::AddressFilterRemoveEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    VerifyOrExit((entry = AddressFilterFindEntry(aAddress)) != NULL, error = OT_ERROR_NOT_FOUND);
    entry->mFiltered = false;

exit:
    return error;
}

otError Filter::GetNextAddressFilterEntry(otMacFilterIterator *aIterator, Entry *aEntry) const
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t index = *reinterpret_cast<uint8_t *>(aIterator);

    for (; index < GetMaxEntries(); index++)
    {
        if (mFilterEntries[index].mFiltered)
        {
            memcpy(&aEntry->mExtAddress, &mFilterEntries[index].mExtAddress, OT_EXT_ADDRESS_SIZE);
            aEntry->mFiltered = mFilterEntries[index].mFiltered;
            aEntry->mFixedRssi = mFilterEntries[index].mFixedRssi;
            aEntry->mRssi = mFilterEntries[index].mRssi;

            *aIterator = *reinterpret_cast<otMacFilterIterator *>(&(++index));
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

void Filter::AddressFilterClearEntries(void)
{
    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if (mFilterEntries[i].mFiltered)
        {
            mFilterEntries[i].mFiltered = false;
        }
    }
}

void Filter::RssiInFilterSet(int8_t aRssi)
{
    mRssiIn = aRssi;
}

int8_t Filter::RssiInFilterGet(void)
{
    return mRssiIn;
}

otError Filter::RssiInFilterAddEntry(const ExtAddress *aAddress, int8_t aRssi)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = RssiInFilterFindEntry(aAddress);

    if (entry == NULL && (mAddressFilterState != OT_MAC_ADDRESSFILTER_BLACKLIST))
    {
        // reuse current address filter entry if any
        entry = AddressFilterFindEntry(aAddress);
    }

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        memcpy(&entry->mExtAddress, aAddress, OT_EXT_ADDRESS_SIZE);
    }

    entry->mFixedRssi = true;
    entry->mRssi = aRssi;

exit:
    return error;
}

otError Filter::RssiInFilterRemoveEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    VerifyOrExit((entry = RssiInFilterFindEntry(aAddress)) != NULL, error = OT_ERROR_NOT_FOUND);
    entry->mFixedRssi = false;

exit:
    return error;
}

otError Filter::GetNextRssiInFilterEntry(otMacFilterIterator *aIterator, Entry *aEntry) const
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t index = *reinterpret_cast<uint8_t *>(aIterator);

    for (; index < GetMaxEntries(); index++)
    {
        if (mFilterEntries[index].mFixedRssi)
        {
            memcpy(&aEntry->mExtAddress, &mFilterEntries[index].mExtAddress, OT_EXT_ADDRESS_SIZE);
            aEntry->mFiltered = mFilterEntries[index].mFiltered;
            aEntry->mFixedRssi = mFilterEntries[index].mFixedRssi;
            aEntry->mRssi = mFilterEntries[index].mRssi;

            *aIterator = *reinterpret_cast<otMacFilterIterator *>(&(++index));
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

void Filter::RssiInFilterClearEntries(void)
{
    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mFilterEntries[i].mFixedRssi = false;
    }
}

otError Filter::Apply(const ExtAddress *aAddress, int8_t &aRssi)
{
    otError error = OT_ERROR_NONE;
    otMacFilterEntry *entry = AddressFilterFindEntry(aAddress);

    // assign the default RssiIn setting for all receiving messages first
    aRssi = mRssiIn;

    // check AddressFilter
    if (mAddressFilterState == OT_MAC_ADDRESSFILTER_WHITELIST)
    {
        VerifyOrExit(entry != NULL, error = OT_ERROR_WHITELIST_FILTERED);
    }
    else if (mAddressFilterState == OT_MAC_ADDRESSFILTER_BLACKLIST)
    {
        VerifyOrExit(entry == NULL, error = OT_ERROR_BLACKLIST_FILTERED);
    }

    // check RssiInFilter
    if ((entry = RssiInFilterFindEntry(aAddress)) != NULL)
    {
        aRssi = entry->mRssi;
    }

exit:
    return error;
}

}  // namespace Mac
}  // namespace ot

#endif  // OPENTHREAD_ENABLE_MAC_FILTER
