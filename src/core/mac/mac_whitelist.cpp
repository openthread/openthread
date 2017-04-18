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
 *   This file implements whitelist IEEE 802.15.4 frame filtering based on MAC address.
 */

#include <string.h>

#include <common/code_utils.hpp>
#include <mac/mac_whitelist.hpp>

#if OPENTHREAD_ENABLE_MAC_WHITELIST

namespace Thread {
namespace Mac {

Whitelist::Whitelist(void)
{
    mEnabled = false;

    for (int i = 0; i < kMaxEntries; i++)
    {
        mWhitelist[i].mValid = false;
    }
}

ThreadError Whitelist::GetEntry(uint8_t aIndex, Entry &aEntry) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIndex < kMaxEntries, error = kThreadError_InvalidArgs);

    memcpy(&aEntry.mExtAddress, &mWhitelist[aIndex].mExtAddress, sizeof(aEntry.mExtAddress));
    aEntry.mRssi = mWhitelist[aIndex].mRssi;
    aEntry.mValid = mWhitelist[aIndex].mValid;
    aEntry.mFixedRssi = mWhitelist[aIndex].mFixedRssi;

exit:
    return error;
}

Whitelist::Entry *Whitelist::Add(const ExtAddress &address)
{
    Entry *rval;

    VerifyOrExit((rval = Find(address)) == NULL);

    for (int i = 0; i < kMaxEntries; i++)
    {
        if (mWhitelist[i].mValid)
        {
            continue;
        }

        memcpy(&mWhitelist[i].mExtAddress, &address, sizeof(mWhitelist[i].mExtAddress));
        mWhitelist[i].mValid = true;
        mWhitelist[i].mFixedRssi = false;
        ExitNow(rval = &mWhitelist[i]);
    }

exit:
    return rval;
}

void Whitelist::Clear(void)
{
    for (int i = 0; i < kMaxEntries; i++)
    {
        mWhitelist[i].mValid = false;
    }
}

void Whitelist::Remove(const ExtAddress &address)
{
    Entry *entry;

    VerifyOrExit((entry = Find(address)) != NULL);
    memset(entry, 0, sizeof(*entry));

exit:
    return;
}

Whitelist::Entry *Whitelist::Find(const ExtAddress &address)
{
    Entry *rval = NULL;

    for (int i = 0; i < kMaxEntries; i++)
    {
        if (!mWhitelist[i].mValid)
        {
            continue;
        }

        if (memcmp(&mWhitelist[i].mExtAddress, &address, sizeof(mWhitelist[i].mExtAddress)) == 0)
        {
            ExitNow(rval = &mWhitelist[i]);
        }
    }

exit:
    return rval;
}

void Whitelist::ClearFixedRssi(Entry &aEntry)
{
    aEntry.mFixedRssi = false;
}

ThreadError Whitelist::GetFixedRssi(Entry &aEntry, int8_t &rssi) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry.mValid && aEntry.mFixedRssi, error = kThreadError_Error);
    rssi = aEntry.mRssi;

exit:
    return error;
}

void Whitelist::SetFixedRssi(Entry &aEntry, int8_t aRssi)
{
    aEntry.mFixedRssi = true;
    aEntry.mRssi = aRssi;
}

}  // namespace Mac
}  // namespace Thread

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST
