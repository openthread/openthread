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

#include  "openthread/openthread_enable_defines.h"

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

otError Child::FindIp6Address(const Ip6::Address &aAddress, uint8_t *aIndex) const
{
    otError error = OT_ERROR_NOT_FOUND;

    for (uint8_t index = 0; index < kMaxIp6AddressPerChild; index++)
    {
        if (mIp6Address[index] == aAddress)
        {
            if (aIndex != NULL)
            {
                *aIndex = index;
            }

            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

void Child::RemoveIp6Address(uint8_t aIndex)
{
    VerifyOrExit(aIndex < kMaxIp6AddressPerChild);

    for (uint8_t i = aIndex; i < kMaxIp6AddressPerChild - 1; i++)
    {
        mIp6Address[i] = mIp6Address[i + 1];
    }

    memset(&mIp6Address[kMaxIp6AddressPerChild - 1], 0, sizeof(Ip6::Address));

exit:
    return;
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
