/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file includes definitions for maintaining Thread network topologies.
 */

#define WPP_NAME "topology.tmh"

#include "topology.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"

namespace ot {

void Neighbor::GenerateChallenge(void)
{
    for (uint8_t i = 0; i < sizeof(mValidPending.mPending.mChallenge); i++)
    {
        mValidPending.mPending.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }
}

void Child::ClearIp6Addresses(void)
{
    memset(mIp6Address, 0, sizeof(mIp6Address));
}

otError Child::GetNextIp6Address(Ip6AddressIterator &aIterator, Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIterator.Get() < kMaxIp6AddressPerChild, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(!mIp6Address[aIterator.Get()].IsUnspecified(), error = OT_ERROR_NOT_FOUND);
    aAddress = mIp6Address[aIterator.Get()];
    aIterator.Increment();

exit:
    return error;
}

otError Child::AddIp6Address(const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    for (uint16_t index = 0; index < kMaxIp6AddressPerChild; index++)
    {
        if (mIp6Address[index].IsUnspecified())
        {
            mIp6Address[index] = aAddress;
            ExitNow();
        }

        VerifyOrExit(mIp6Address[index] != aAddress, error = OT_ERROR_ALREADY);
    }

    error = OT_ERROR_NO_BUFS;

exit:
    return error;
}

otError Child::RemoveIp6Address(const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NOT_FOUND;
    uint16_t index;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    for (index = 0; index < kMaxIp6AddressPerChild; index++)
    {
        VerifyOrExit(!mIp6Address[index].IsUnspecified());

        if (mIp6Address[index] == aAddress)
        {
            error = OT_ERROR_NONE;
            break;
        }
    }

    SuccessOrExit(error);

    for (; index < kMaxIp6AddressPerChild - 1; index++)
    {
        mIp6Address[index] = mIp6Address[index + 1];
    }

    mIp6Address[kMaxIp6AddressPerChild - 1].Clear();

exit:
    return error;
}

bool Child::HasIp6Address(const Ip6::Address &aAddress) const
{
    bool retval = false;

    VerifyOrExit(!aAddress.IsUnspecified());

    for (uint16_t index = 0; index < kMaxIp6AddressPerChild; index++)
    {
        VerifyOrExit(!mIp6Address[index].IsUnspecified());

        if (mIp6Address[index] == aAddress)
        {
            ExitNow(retval = true);
        }
    }

exit:
    return retval;
}

void Child::GenerateChallenge(void)
{
    for (uint8_t i = 0; i < sizeof(mAttachChallenge); i++)
    {
        mAttachChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }
}

const Mac::Address &Child::GetMacAddress(Mac::Address &aMacAddress) const
{
    if (mUseShortAddress)
    {
        aMacAddress.mShortAddress = GetRloc16();
        aMacAddress.mLength = sizeof(aMacAddress.mShortAddress);
    }
    else
    {
        aMacAddress.mExtAddress = GetExtAddress();
        aMacAddress.mLength = sizeof(aMacAddress.mExtAddress);
    }

    return aMacAddress;
}

}  // namespace ot
