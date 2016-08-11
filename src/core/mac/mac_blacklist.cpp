/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements blacklist IEEE 802.15.4 frame filtering based on MAC address.
 */

#include <string.h>

#include <common/code_utils.hpp>
#include <mac/mac_blacklist.hpp>

namespace Thread {
namespace Mac {

Blacklist::Blacklist(void)
{
    mEnabled = false;

    for (int i = 0; i < kMaxEntries; i++)
    {
        mBlacklist[i].mValid = false;
    }
}

void Blacklist::Enable(void)
{
    mEnabled = true;
}

void Blacklist::Disable(void)
{
    mEnabled = false;
}

bool Blacklist::IsEnabled(void) const
{
    return mEnabled;
}

int Blacklist::GetMaxEntries(void) const
{
    return kMaxEntries;
}

ThreadError Blacklist::GetEntry(uint8_t aIndex, Entry &aEntry) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIndex < kMaxEntries, error = kThreadError_InvalidArgs);

    memcpy(&aEntry.mExtAddress, &mBlacklist[aIndex].mExtAddress, sizeof(aEntry.mExtAddress));
    aEntry.mValid = mBlacklist[aIndex].mValid;

exit:
    return error;
}

Blacklist::Entry *Blacklist::Add(const ExtAddress &address)
{
    Entry *rval;

    VerifyOrExit((rval = Find(address)) == NULL, ;);

    for (int i = 0; i < kMaxEntries; i++)
    {
        if (mBlacklist[i].mValid)
        {
            continue;
        }

        memcpy(&mBlacklist[i].mExtAddress, &address, sizeof(mBlacklist[i].mExtAddress));
        mBlacklist[i].mValid = true;
        ExitNow(rval = &mBlacklist[i]);
    }

exit:
    return rval;
}

void Blacklist::Clear(void)
{
    for (int i = 0; i < kMaxEntries; i++)
    {
        mBlacklist[i].mValid = false;
    }
}

void Blacklist::Remove(const ExtAddress &address)
{
    Entry *entry;

    VerifyOrExit((entry = Find(address)) != NULL, ;);
    memset(entry, 0, sizeof(*entry));

exit:
    {}
}

Blacklist::Entry *Blacklist::Find(const ExtAddress &address)
{
    Entry *rval = NULL;

    for (int i = 0; i < kMaxEntries; i++)
    {
        if (!mBlacklist[i].mValid)
        {
            continue;
        }

        if (memcmp(&mBlacklist[i].mExtAddress, &address, sizeof(mBlacklist[i].mExtAddress)) == 0)
        {
            ExitNow(rval = &mBlacklist[i]);
        }
    }

exit:
    return rval;
}

}  // namespace Mac
}  // namespace Thread
