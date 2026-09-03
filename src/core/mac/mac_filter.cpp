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

//---------------------------------------------------------------------------------------------------------------------
// Filter

Filter::Filter(void)
    : mMode(kModeRssInOnly)
    , mDefaultRssIn(kFixedRssDisabled)
{
}

Error Filter::AddOrUpdateEntry(Type aType, const ExtAddress &aExtAddress, int8_t aRss)
{
    Error  error = kErrorNone;
    Entry *entry = mEntries.FindMatching(aExtAddress);

    if (entry == nullptr)
    {
        entry = mEntries.PushBack();
        VerifyOrExit(entry != nullptr, error = kErrorNoBufs);
        entry->Init(aExtAddress);
    }

    if (aType == kAddrFilter)
    {
        entry->SetInAddrFilter(true);
    }
    else
    {
        entry->SetRssIn(aRss);
    }

exit:
    return error;
}

void Filter::RemoveEntry(Type aType, const ExtAddress &aExtAddress)
{
    Entry *entry = mEntries.FindMatching(aExtAddress);

    VerifyOrExit(entry != nullptr);

    if (aType == kAddrFilter)
    {
        entry->SetInAddrFilter(false);
    }
    else
    {
        entry->ClearRssIn();
    }

    if (entry->Matches(Entry::kNotInUse))
    {
        mEntries.Remove(*entry);
    }

exit:
    return;
}

Error Filter::GetNext(Type aType, Iterator &aIterator, EntryInfo &aInfo) const
{
    Error error = kErrorNotFound;

    for (; aIterator < mEntries.GetLength(); aIterator++)
    {
        const Entry &entry = mEntries[aIterator];

        if ((aType == kAddrFilter) ? entry.IsInAddrFilter() : entry.IsInRssFilter())
        {
            aInfo.mExtAddress = entry.GetExtAddress();
            aInfo.mRssIn      = entry.GetRssIn();
            error             = kErrorNone;
            aIterator++;
            ExitNow();
        }
    }

    VerifyOrExit(aType == kRssFilter);

    // Return the default RssIn at the end of list

    VerifyOrExit(aIterator == mEntries.GetLength());

    if (mDefaultRssIn != kFixedRssDisabled)
    {
        AsCoreType(&aInfo.mExtAddress).Fill(0xff);
        aInfo.mRssIn = mDefaultRssIn;
        error        = kErrorNone;
        aIterator++;
    }

exit:
    return error;
}

void Filter::ClearAll(Type aType)
{
    for (Entry &entry : mEntries)
    {
        if (aType == kAddrFilter)
        {
            entry.SetInAddrFilter(false);
        }
        else
        {
            entry.ClearRssIn();
        }
    }

    if (aType == kRssFilter)
    {
        mDefaultRssIn = kFixedRssDisabled;
    }

    mEntries.RemoveAllMatching(Entry::kNotInUse);
}

Error Filter::AddRssIn(const ExtAddress &aExtAddress, int8_t aRss)
{
    Error error = kErrorNone;

    VerifyOrExit(aRss != kFixedRssDisabled, error = kErrorInvalidArgs);
    error = AddOrUpdateEntry(kRssFilter, aExtAddress, aRss);

exit:
    return error;
}

Error Filter::Apply(const ExtAddress &aExtAddress, int8_t &aRss) const
{
    Error        error = kErrorNone;
    const Entry *entry = mEntries.FindMatching(aExtAddress);
    bool         isInAddrFilterList;

    // Use the default RssIn setting for all receiving messages first.
    aRss = mDefaultRssIn;

    // In allowlist mode, entry must be present in the list, in
    // denylist mode it must not be present.

    isInAddrFilterList = (entry != nullptr) && entry->IsInAddrFilter();

    switch (mMode)
    {
    case kModeRssInOnly:
        break;

    case kModeAllowlist:
        VerifyOrExit(isInAddrFilterList, error = kErrorAddressFiltered);
        break;

    case kModeDenylist:
        VerifyOrExit(!isInAddrFilterList, error = kErrorAddressFiltered);
        break;
    }

    if ((entry != nullptr) && entry->IsInRssFilter())
    {
        aRss = entry->GetRssIn();
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

//---------------------------------------------------------------------------------------------------------------------
// Filter::Entry

void Filter::Entry::Init(const ExtAddress &aExtAddress)
{
    mExtAddress   = aExtAddress;
    mInAddrFilter = false;
    mRssIn        = kFixedRssDisabled;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
