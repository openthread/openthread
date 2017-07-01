/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "mac_filter.hpp"

#include "utils/wrap_string.h"
#include "openthread/types.h"
#include "common/code_utils.hpp"


namespace ot {
namespace Mac {

Filter::Filter(void) :
    mAddressFilterState(OT_MAC_ADDRESSFILTER_DISABLED),
    mLinkQualityIn(kInvalidLinkQualityIn)
{
    for (int i = 0; i < kMaxEntries; i++)
    {
        mFilterEntries[i].mFiltered = false;
        mFilterEntries[i].mLinkQualityInFixed = false;
    }
}

Filter::Entry *Filter::FindEntry(const ExtAddress *aAddress)
{
    Entry *entry = NULL;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if ((mFilterEntries[i].mFiltered || mFilterEntries[i].mLinkQualityInFixed) &&
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
        if (!mFilterEntries[i].mFiltered && !mFilterEntries[i].mLinkQualityInFixed)
        {
            ExitNow(entry = &mFilterEntries[i]);
        }
    }

exit:
    return entry;
}

otError Filter::AddressFilterSetState(uint8_t aState)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mAddressFilterState != OT_MAC_ADDRESSFILTER_DISABLED ||
                 aState != OT_MAC_ADDRESSFILTER_DISABLED, error = OT_ERROR_INVALID_STATE);

    mAddressFilterState = aState;
exit:
    return error;
}

otError Filter::AddressFilterAddEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    entry = FindEntry(aAddress);

    if (entry)
    {
        VerifyOrExit(!entry->mFiltered, error = OT_ERROR_ALREADY);
        entry->mFiltered = true;
    }
    else
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);

        memcpy(&entry->mExtAddress, aAddress, OT_EXT_ADDRESS_SIZE);
        entry->mFiltered = true;
    }

exit:
    return error;
}

otError Filter::AddressFilterRemoveEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    entry = FindEntry(aAddress);

    VerifyOrExit(entry != NULL && entry->mFiltered, error = OT_ERROR_NOT_FOUND);

    entry->mFiltered = false;

exit:
    return error;
}

otError Filter::AddressFilterClearEntries(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mAddressFilterState != OT_MAC_ADDRESSFILTER_DISABLED, error = OT_ERROR_INVALID_STATE);

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        if (mFilterEntries[i].mFiltered)
        {
            mFilterEntries[i].mFiltered = false;
        }
    }

exit:
    return error;
}

void Filter::AddressFilterReset(void)
{
    mAddressFilterState = OT_MAC_ADDRESSFILTER_DISABLED;

    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mFilterEntries[i].mFiltered = false;
    }
}

otError Filter::LinkQualityInFilterSet(uint8_t aLinkQualityIn)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aLinkQualityIn <= 3, error = OT_ERROR_INVALID_ARGS);

    mLinkQualityIn = aLinkQualityIn;

exit:
    return error;
}

otError Filter::LinkQualityInFilterGet(uint8_t *aLinkQualityIn)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mLinkQualityIn != kInvalidLinkQualityIn, error = OT_ERROR_NOT_FOUND);

    *aLinkQualityIn = mLinkQualityIn;

exit:
    return error;
}

void Filter::LinkQualityInFilterUnset(void)
{
    mLinkQualityIn = kInvalidLinkQualityIn;
}

otError Filter::LinkQualityInFilterAddEntry(const ExtAddress *aAddress, uint8_t aLinkQualityIn)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = NULL;

    VerifyOrExit(aLinkQualityIn <= 3, error = OT_ERROR_INVALID_ARGS);

    entry = FindEntry(aAddress);

    if (entry == NULL)
    {
        VerifyOrExit((entry = FindAvailEntry()) != NULL, error = OT_ERROR_NO_BUFS);
        memcpy(&entry->mExtAddress, aAddress, OT_EXT_ADDRESS_SIZE);
    }

    entry->mLinkQualityInFixed = true;
    entry->mLinkQualityIn = aLinkQualityIn;

exit:
    return error;
}

otError Filter::LinkQualityInFilterRemoveEntry(const ExtAddress *aAddress)
{
    otError error = OT_ERROR_NONE;
    Entry *entry = FindEntry(aAddress);

    VerifyOrExit(entry != NULL && entry->mLinkQualityInFixed, error = OT_ERROR_NOT_FOUND);

    entry->mLinkQualityInFixed = false;

exit:
    return error;
}

void Filter::LinkQualityInFilterClearEntries(void)
{
    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mFilterEntries[i].mLinkQualityInFixed = false;
    }
}

void Filter::LinkQualityInFilterReset(void)
{
    for (uint8_t i = 0; i < GetMaxEntries(); i++)
    {
        mFilterEntries[i].mLinkQualityInFixed = false;
    }

    mLinkQualityIn = kInvalidLinkQualityIn;
}

otError Filter::GetNextEntry(otMacFilterIterator *aIterator, Entry *aEntry) const
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t index = *reinterpret_cast<uint8_t *>(aIterator);

    for (; index < GetMaxEntries(); index++)
    {
        if (mFilterEntries[index].mFiltered || mFilterEntries[index].mLinkQualityInFixed)
        {
            memcpy(&aEntry->mExtAddress, &mFilterEntries[index].mExtAddress, OT_EXT_ADDRESS_SIZE);
            aEntry->mLinkQualityIn = mFilterEntries[index].mLinkQualityIn;
            aEntry->mFiltered = mFilterEntries[index].mFiltered;
            aEntry->mLinkQualityInFixed = mFilterEntries[index].mLinkQualityInFixed;

            *aIterator = *reinterpret_cast<otMacFilterIterator *>(&(++index));
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

int8_t Filter::ConvertLinkQualityToRss(int8_t aNoiseFloor, uint8_t aLinkQuality)
{
    int8_t rss;
    uint8_t linkMargin;

    switch (aLinkQuality)
    {
    case 3:
        linkMargin = 30;
        break;

    case 2:
        linkMargin = 15;
        break;

    case 1:
        linkMargin = 5;
        break;

    default:
        linkMargin = 0;
        break;
    }

    rss = linkMargin + aNoiseFloor;

    return rss;
}


}  // namespace Mac
}  // namespace ot

