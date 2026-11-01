/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "test_platform.h"
#include "test_util.h"

#include <openthread/config.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "mac/mac_filter.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

static Mac::ExtAddress MakeExtAddress(uint16_t aIndex)
{
    Mac::ExtAddress addr;

    ClearAllBytes(addr);
    BigEndian::WriteUint16(aIndex, &addr.m8[sizeof(Mac::ExtAddress) - sizeof(uint16_t)]);

    return addr;
}

static void TestFilterMode(void)
{
    Mac::Filter     filter;
    Mac::ExtAddress addr = MakeExtAddress(1);
    int8_t          rss;

    printf("TestFilterMode\n");

    // Verify initial mode and transitions between modes.

    VerifyOrQuit(filter.GetMode() == Mac::Filter::kModeRssInOnly);

    filter.SetMode(Mac::Filter::kModeAllowlist);
    VerifyOrQuit(filter.GetMode() == Mac::Filter::kModeAllowlist);

    filter.SetMode(Mac::Filter::kModeDenylist);
    VerifyOrQuit(filter.GetMode() == Mac::Filter::kModeDenylist);

    filter.SetMode(Mac::Filter::kModeRssInOnly);
    VerifyOrQuit(filter.GetMode() == Mac::Filter::kModeRssInOnly);

    // Verify Apply() behavior when filter list is empty.

    SuccessOrQuit(filter.Apply(addr, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    filter.SetMode(Mac::Filter::kModeAllowlist);
    VerifyOrQuit(filter.Apply(addr, rss) == kErrorAddressFiltered);

    filter.SetMode(Mac::Filter::kModeDenylist);
    SuccessOrQuit(filter.Apply(addr, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
}

static void TestAddressAllowlist(void)
{
    Mac::Filter     filter;
    Mac::ExtAddress addr1 = MakeExtAddress(1);
    Mac::ExtAddress addr2 = MakeExtAddress(2);
    Mac::ExtAddress addr3 = MakeExtAddress(3);
    int8_t          rss;

    printf("TestAddressAllowlist\n");

    filter.SetMode(Mac::Filter::kModeAllowlist);

    // Add addresses and verify allowlist filtering.

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddAddress(addr2));

    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
    SuccessOrQuit(filter.Apply(addr2, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
    VerifyOrQuit(filter.Apply(addr3, rss) == kErrorAddressFiltered);

    // Adding duplicate address should succeed and keep entry in allowlist.

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.Apply(addr1, rss));

    // Remove an address and verify it is filtered.

    filter.RemoveAddress(addr1);
    VerifyOrQuit(filter.Apply(addr1, rss) == kErrorAddressFiltered);
    SuccessOrQuit(filter.Apply(addr2, rss));

    // Removing non-existent address should not affect existing entries.

    filter.RemoveAddress(addr3);
    SuccessOrQuit(filter.Apply(addr2, rss));

    // Clear all addresses and verify all are filtered.

    filter.ClearAddresses();
    VerifyOrQuit(filter.Apply(addr2, rss) == kErrorAddressFiltered);
}

static void TestAddressDenylist(void)
{
    Mac::Filter     filter;
    Mac::ExtAddress addr1 = MakeExtAddress(1);
    Mac::ExtAddress addr2 = MakeExtAddress(2);
    Mac::ExtAddress addr3 = MakeExtAddress(3);
    int8_t          rss;

    printf("TestAddressDenylist\n");

    filter.SetMode(Mac::Filter::kModeDenylist);

    // Add addresses and verify denylist filtering.

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddAddress(addr2));

    VerifyOrQuit(filter.Apply(addr1, rss) == kErrorAddressFiltered);
    VerifyOrQuit(filter.Apply(addr2, rss) == kErrorAddressFiltered);
    SuccessOrQuit(filter.Apply(addr3, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    // Remove an address from denylist and verify it is allowed.

    filter.RemoveAddress(addr1);
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(filter.Apply(addr2, rss) == kErrorAddressFiltered);

    // Removing non-existent address should not affect existing entries.

    filter.RemoveAddress(addr3);
    VerifyOrQuit(filter.Apply(addr2, rss) == kErrorAddressFiltered);

    // Clear all addresses and verify all are allowed.

    filter.ClearAddresses();
    SuccessOrQuit(filter.Apply(addr2, rss));
}

static void TestAddressIteration(void)
{
    Mac::Filter            filter;
    Mac::Filter::Iterator  iter;
    Mac::Filter::EntryInfo info;
    Mac::ExtAddress        addr1 = MakeExtAddress(1);
    Mac::ExtAddress        addr2 = MakeExtAddress(2);
    Mac::ExtAddress        addr3 = MakeExtAddress(3);
    uint16_t               count;
    bool                   foundAddr1;
    bool                   foundAddr2;
    bool                   foundAddr3;

    printf("TestAddressIteration\n");

    // Empty filter should return kErrorNotFound.

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    // Add multiple addresses and iterate through all entries.

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddAddress(addr2));
    SuccessOrQuit(filter.AddAddress(addr3));

    iter       = Mac::Filter::kIteratorInit;
    count      = 0;
    foundAddr1 = false;
    foundAddr2 = false;
    foundAddr3 = false;

    while (filter.GetNextAddress(iter, info) == kErrorNone)
    {
        count++;
        VerifyOrQuit(info.mRssIn == Mac::Filter::kFixedRssDisabled);
        if (AsCoreType(&info.mExtAddress) == addr1)
        {
            foundAddr1 = true;
        }
        else if (AsCoreType(&info.mExtAddress) == addr2)
        {
            foundAddr2 = true;
        }
        else if (AsCoreType(&info.mExtAddress) == addr3)
        {
            foundAddr3 = true;
        }
    }

    VerifyOrQuit(count == 3);
    VerifyOrQuit(foundAddr1 && foundAddr2 && foundAddr3);

    // Remove middle entry and verify remaining entries iterate correctly.

    filter.RemoveAddress(addr2);

    iter       = Mac::Filter::kIteratorInit;
    count      = 0;
    foundAddr1 = false;
    foundAddr2 = false;
    foundAddr3 = false;

    while (filter.GetNextAddress(iter, info) == kErrorNone)
    {
        count++;
        if (AsCoreType(&info.mExtAddress) == addr1)
        {
            foundAddr1 = true;
        }
        else if (AsCoreType(&info.mExtAddress) == addr2)
        {
            foundAddr2 = true;
        }
        else if (AsCoreType(&info.mExtAddress) == addr3)
        {
            foundAddr3 = true;
        }
    }

    VerifyOrQuit(count == 2);
    VerifyOrQuit(foundAddr1 && !foundAddr2 && foundAddr3);

    // Clear all addresses and verify iteration returns kErrorNotFound.

    filter.ClearAddresses();
    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);
}

static void TestRssInFiltering(void)
{
    Mac::Filter     filter;
    Mac::ExtAddress addr1 = MakeExtAddress(1);
    Mac::ExtAddress addr2 = MakeExtAddress(2);
    int8_t          rss;

    printf("TestRssInFiltering\n");

    // Verify kFixedRssDisabled cannot be added as valid RSS.

    VerifyOrQuit(filter.AddRssIn(addr1, Mac::Filter::kFixedRssDisabled) == kErrorInvalidArgs);

    // Add fixed RSS and verify retrieval.

    SuccessOrQuit(filter.AddRssIn(addr1, -70));
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -70);

    SuccessOrQuit(filter.Apply(addr2, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    // Update fixed RSS for existing entry.

    SuccessOrQuit(filter.AddRssIn(addr1, -50));
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -50);

    // Remove fixed RSS and verify default is restored.

    filter.RemoveRssIn(addr1);
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    // Removing non-existent RSS entry should not cause issues.

    filter.RemoveRssIn(addr2);
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
}

static void TestDefaultRssIn(void)
{
    Mac::Filter     filter;
    Mac::ExtAddress addr1 = MakeExtAddress(1);
    Mac::ExtAddress addr2 = MakeExtAddress(2);
    int8_t          rss;

    printf("TestDefaultRssIn\n");

    // Set default RSS and verify it applies to all addresses.

    filter.SetDefaultRssIn(-65);
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -65);
    SuccessOrQuit(filter.Apply(addr2, rss));
    VerifyOrQuit(rss == -65);

    // Explicit RSS should override default RSS.

    SuccessOrQuit(filter.AddRssIn(addr1, -40));
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -40);
    SuccessOrQuit(filter.Apply(addr2, rss));
    VerifyOrQuit(rss == -65);

    // Removing explicit RSS falls back to default RSS.

    filter.RemoveRssIn(addr1);
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -65);

    // Clear default RSS and verify no fixed RSS is applied.

    filter.ClearDefaultRssIn();
    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    // ClearAllRssIn() clears both explicit and default RSS.

    filter.SetDefaultRssIn(-80);
    SuccessOrQuit(filter.AddRssIn(addr1, -30));
    SuccessOrQuit(filter.AddRssIn(addr2, -45));
    filter.ClearAllRssIn();

    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
    SuccessOrQuit(filter.Apply(addr2, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);
}

static void TestRssInIteration(void)
{
    Mac::Filter            filter;
    Mac::Filter::Iterator  iter;
    Mac::Filter::EntryInfo info;
    Mac::ExtAddress        addr1 = MakeExtAddress(1);
    Mac::ExtAddress        addr2 = MakeExtAddress(2);
    Mac::ExtAddress        defaultRssAddr;
    uint16_t               count;
    bool                   foundAddr1;
    bool                   foundAddr2;
    bool                   foundDefault;

    printf("TestRssInIteration\n");

    defaultRssAddr.Fill(0xff);

    // Empty filter with no default RSS yields nothing.

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // Empty filter with default RSS yields synthetic all-0xff entry.

    filter.SetDefaultRssIn(-75);
    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextRssIn(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == defaultRssAddr);
    VerifyOrQuit(info.mRssIn == -75);
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // Explicit entries are yielded along with default RSS.

    SuccessOrQuit(filter.AddRssIn(addr1, -60));
    SuccessOrQuit(filter.AddRssIn(addr2, -80));

    iter         = Mac::Filter::kIteratorInit;
    count        = 0;
    foundAddr1   = false;
    foundAddr2   = false;
    foundDefault = false;

    while (filter.GetNextRssIn(iter, info) == kErrorNone)
    {
        count++;
        if (AsCoreType(&info.mExtAddress) == addr1)
        {
            foundAddr1 = true;
            VerifyOrQuit(info.mRssIn == -60);
        }
        else if (AsCoreType(&info.mExtAddress) == addr2)
        {
            foundAddr2 = true;
            VerifyOrQuit(info.mRssIn == -80);
        }
        else if (AsCoreType(&info.mExtAddress) == defaultRssAddr)
        {
            foundDefault = true;
            VerifyOrQuit(info.mRssIn == -75);
        }
    }

    VerifyOrQuit(count == 3);
    VerifyOrQuit(foundAddr1 && foundAddr2 && foundDefault);

    // Clear default RSS and verify only explicit entries are yielded.

    filter.ClearDefaultRssIn();

    iter         = Mac::Filter::kIteratorInit;
    count        = 0;
    foundAddr1   = false;
    foundAddr2   = false;
    foundDefault = false;

    while (filter.GetNextRssIn(iter, info) == kErrorNone)
    {
        count++;
        if (AsCoreType(&info.mExtAddress) == addr1)
        {
            foundAddr1 = true;
            VerifyOrQuit(info.mRssIn == -60);
        }
        else if (AsCoreType(&info.mExtAddress) == addr2)
        {
            foundAddr2 = true;
            VerifyOrQuit(info.mRssIn == -80);
        }
        else if (AsCoreType(&info.mExtAddress) == defaultRssAddr)
        {
            foundDefault = true;
        }
    }

    VerifyOrQuit(count == 2);
    VerifyOrQuit(foundAddr1 && foundAddr2 && !foundDefault);
}

static void TestAddressAndRssCoexistence(void)
{
    Mac::Filter            filter;
    Mac::Filter::Iterator  iter;
    Mac::Filter::EntryInfo info;
    Mac::ExtAddress        addr1 = MakeExtAddress(1);
    Mac::ExtAddress        addr2 = MakeExtAddress(2);
    int8_t                 rss;

    printf("TestAddressAndRssCoexistence\n");

    filter.SetMode(Mac::Filter::kModeAllowlist);

    // Add both address and RSS-In to the same entry.

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddRssIn(addr1, -55));

    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == -55);

    // Verify entry is present in both address and RSS iterations.

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextAddress(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == -55);
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextRssIn(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == -55);
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // Remove address filter only; RSS-In entry must persist.

    filter.RemoveAddress(addr1);
    VerifyOrQuit(filter.Apply(addr1, rss) == kErrorAddressFiltered);

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextRssIn(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == -55);
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // Re-add address and remove RSS-In; address filter must persist.

    SuccessOrQuit(filter.AddAddress(addr1));
    filter.RemoveRssIn(addr1);

    SuccessOrQuit(filter.Apply(addr1, rss));
    VerifyOrQuit(rss == Mac::Filter::kFixedRssDisabled);

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextAddress(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == Mac::Filter::kFixedRssDisabled);
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // Remove address; entry should be completely removed.

    filter.RemoveAddress(addr1);
    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    // ClearAddresses() removes address filters while preserving RSS-In entries.

    filter.ClearAllRssIn();
    filter.ClearAddresses();

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddRssIn(addr1, -48));
    SuccessOrQuit(filter.AddAddress(addr2));

    filter.ClearAddresses();

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextRssIn(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == -48);
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    // ClearAllRssIn() removes RSS-In entries while preserving address filters.

    filter.ClearAllRssIn();
    filter.ClearAddresses();

    SuccessOrQuit(filter.AddAddress(addr1));
    SuccessOrQuit(filter.AddRssIn(addr1, -48));
    SuccessOrQuit(filter.AddRssIn(addr2, -62));

    filter.ClearAllRssIn();

    iter = Mac::Filter::kIteratorInit;
    VerifyOrQuit(filter.GetNextRssIn(iter, info) == kErrorNotFound);

    iter = Mac::Filter::kIteratorInit;
    SuccessOrQuit(filter.GetNextAddress(iter, info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == addr1);
    VerifyOrQuit(info.mRssIn == Mac::Filter::kFixedRssDisabled);
    VerifyOrQuit(filter.GetNextAddress(iter, info) == kErrorNotFound);
}

static void TestCapacityLimits(void)
{
    constexpr uint16_t kMaxEntries = OPENTHREAD_CONFIG_MAC_FILTER_SIZE;

    Mac::Filter            filter;
    Mac::Filter::Iterator  iter;
    Mac::Filter::EntryInfo info;
    Mac::ExtAddress        extraAddr = MakeExtAddress(kMaxEntries + 1);
    uint16_t               count;
    uint16_t               i;

    printf("TestCapacityLimits\n");

    // Fill filter to maximum capacity.

    for (i = 1; i <= kMaxEntries; i++)
    {
        SuccessOrQuit(filter.AddAddress(MakeExtAddress(i)));
    }

    // Adding beyond capacity fails with kErrorNoBufs.

    VerifyOrQuit(filter.AddAddress(extraAddr) == kErrorNoBufs);

    // Adding RSS-In to an existing address entry reuses slot.

    SuccessOrQuit(filter.AddRssIn(MakeExtAddress(1), -60));

    // Adding RSS-In for a new address fails when table is full.

    VerifyOrQuit(filter.AddRssIn(extraAddr, -60) == kErrorNoBufs);

    // Free slot by removing an address-only entry.

    filter.RemoveAddress(MakeExtAddress(2));

    SuccessOrQuit(filter.AddAddress(extraAddr));

    VerifyOrQuit(filter.AddAddress(MakeExtAddress(kMaxEntries + 2)) == kErrorNoBufs);

    // Removing address from entry with active RSS-In does not free slot.

    filter.RemoveAddress(MakeExtAddress(1));

    VerifyOrQuit(filter.AddAddress(MakeExtAddress(kMaxEntries + 2)) == kErrorNoBufs);

    // Removing RSS-In now frees the slot.

    filter.RemoveRssIn(MakeExtAddress(1));

    SuccessOrQuit(filter.AddAddress(MakeExtAddress(kMaxEntries + 2)));

    // Clear all entries and verify all slots are freed.

    filter.ClearAddresses();
    filter.ClearAllRssIn();

    count = 0;
    iter  = Mac::Filter::kIteratorInit;
    while (filter.GetNextAddress(iter, info) == kErrorNone)
    {
        count++;
    }
    VerifyOrQuit(count == 0);

    iter = Mac::Filter::kIteratorInit;
    while (filter.GetNextRssIn(iter, info) == kErrorNone)
    {
        count++;
    }
    VerifyOrQuit(count == 0);

    // Verify all slots can be populated again up to capacity.

    for (i = 1; i <= kMaxEntries; i++)
    {
        SuccessOrQuit(filter.AddAddress(MakeExtAddress(i)));
    }
    VerifyOrQuit(filter.AddAddress(extraAddr) == kErrorNoBufs);
}

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    ot::TestFilterMode();
    ot::TestAddressAllowlist();
    ot::TestAddressDenylist();
    ot::TestAddressIteration();
    ot::TestRssInFiltering();
    ot::TestDefaultRssIn();
    ot::TestRssInIteration();
    ot::TestAddressAndRssCoexistence();
    ot::TestCapacityLimits();
    printf("All tests passed\n");
#else
    printf("MAC filter is not enabled\n");
#endif

    return 0;
}
