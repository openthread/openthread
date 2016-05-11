/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements whitelist IEEE 802.15.4 frame filtering based on MAC address.
 */

#include <string.h>

#include <common/code_utils.hpp>
#include <mac/mac_whitelist.hpp>

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

void Whitelist::Enable(void)
{
    mEnabled = true;
}

void Whitelist::Disable(void)
{
    mEnabled = false;
}

bool Whitelist::IsEnabled(void) const
{
    return mEnabled;
}

int Whitelist::GetMaxEntries(void) const
{
    return kMaxEntries;
}

const Whitelist::Entry *Whitelist::GetEntries(void) const
{
    return mWhitelist;
}

Whitelist::Entry *Whitelist::Add(const ExtAddress &address)
{
    Entry *rval;

    VerifyOrExit((rval = Find(address)) == NULL, ;);

    for (int i = 0; i < kMaxEntries; i++)
    {
        if (mWhitelist[i].mValid)
        {
            continue;
        }

        memcpy(&mWhitelist[i], &address, sizeof(mWhitelist[i]));
        mWhitelist[i].mValid = true;
        mWhitelist[i].mConstantRssi = false;
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

    VerifyOrExit((entry = Find(address)) != NULL, ;);
    memset(entry, 0, sizeof(*entry));

exit:
    {}
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

void Whitelist::ClearConstantRssi(Entry &aEntry)
{
    aEntry.mConstantRssi = false;
}

ThreadError Whitelist::GetConstantRssi(Entry &aEntry, int8_t &rssi) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry.mValid && aEntry.mConstantRssi, error = kThreadError_Error);
    rssi = aEntry.mRssi;

exit:
    return error;
}

void Whitelist::SetConstantRssi(Entry &aEntry, int8_t aRssi)
{
    aEntry.mConstantRssi = true;
    aEntry.mRssi = aRssi;
}

}  // namespace Mac
}  // namespace Thread
