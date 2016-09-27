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
 *   This file implements MPL.
 */

#include <common/code_utils.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/ip6_mpl.hpp>

namespace Thread {
namespace Ip6 {

Mpl::Mpl(Ip6 &aIp6):
    mTimer(aIp6.mTimerScheduler, &Mpl::HandleTimer, this),
    mMatchingAddress(NULL)
{
    memset(mEntries, 0, sizeof(mEntries));
    mSequence = 0;
    mSeed = 0;
}

void Mpl::InitOption(OptionMpl &aOption, const Address &aAddress)
{
    aOption.Init();
    aOption.SetSequence(mSequence++);

    // Check if Seed can be elided.
    if (mMatchingAddress && aAddress == *mMatchingAddress)
    {
        aOption.SetSeedLength(OptionMpl::kSeedLength0);

        // Decrease default option length.
        aOption.SetLength(aOption.GetLength() - sizeof(mSeed));
    }
    else
    {
        aOption.SetSeedLength(OptionMpl::kSeedLength2);
        aOption.SetSeed(mSeed);
    }
}

ThreadError Mpl::ProcessOption(const Message &aMessage, const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    OptionMpl option;
    MplEntry *entry = NULL;
    int8_t diff;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(option), &option) >= OptionMpl::kMinLength &&
                 (option.GetSeedLength() == OptionMpl::kSeedLength0 ||
                  option.GetSeedLength() == OptionMpl::kSeedLength2),
                 error = kThreadError_Drop);

    if (option.GetSeedLength() == OptionMpl::kSeedLength0)
    {
        // Retrieve Seed from the IPv6 Source Address.
        option.SetSeed(HostSwap16(aAddress.mFields.m16[7]));
    }

    for (int i = 0; i < kNumEntries; i++)
    {
        if (mEntries[i].mLifetime == 0)
        {
            entry = &mEntries[i];
        }
        else if (mEntries[i].mSeed == option.GetSeed())
        {
            entry = &mEntries[i];
            diff = static_cast<int8_t>(option.GetSequence() - entry->mSequence);

            VerifyOrExit(diff > 0, error = kThreadError_Drop);

            break;
        }
    }

    VerifyOrExit(entry != NULL, error = kThreadError_Drop);

    entry->mSeed = option.GetSeed();
    entry->mSequence = option.GetSequence();
    entry->mLifetime = kLifetime;
    mTimer.Start(1000);

exit:
    return error;
}

void Mpl::HandleTimer(void *aContext)
{
    static_cast<Mpl *>(aContext)->HandleTimer();
}

void Mpl::HandleTimer()
{
    bool startTimer = false;

    for (int i = 0; i < kNumEntries; i++)
    {
        if (mEntries[i].mLifetime > 0)
        {
            mEntries[i].mLifetime--;
            startTimer = true;
        }
    }

    if (startTimer)
    {
        mTimer.Start(1000);
    }
}

}  // namespace Ip6
}  // namespace Thread
