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
#include "common/instance.hpp"
#include "common/logging.hpp"

namespace ot {

void Neighbor::GenerateChallenge(void)
{
    Random::FillBuffer(mValidPending.mPending.mChallenge, sizeof(mValidPending.mPending.mChallenge));
}

bool Child::IsStateValidOrAttaching(void) const
{
    bool rval = false;

    switch (GetState())
    {
    case kStateInvalid:
    case kStateParentRequest:
    case kStateParentResponse:
        break;

    case kStateRestored:
    case kStateChildIdRequest:
    case kStateLinkRequest:
    case kStateChildUpdateRequest:
    case kStateValid:
        rval = true;
        break;
    }

    return rval;
}

void Child::ClearIp6Addresses(void)
{
    memset(mMeshLocalIid, 0, sizeof(mMeshLocalIid));
    memset(mIp6Address, 0, sizeof(mIp6Address));
}

/**
 * Determines if all elements in an array are zero.
 *
 * @param[in]  aArray   A pointer to an array of bytes.
 * @param[in]  aLength  Array length (number of bytes).
 *
 * @returns TRUE if all bytes in the array are zero, FALSE otherwise.
 *
 */
static bool IsAllZero(const uint8_t *aArray, uint8_t aLength)
{
    bool retval = true;

    for (; aLength != 0; aArray++, aLength--)
    {
        VerifyOrExit(*aArray == 0, retval = false);
    }

exit:
    return retval;
}

otError Child::GetMeshLocalIp6Address(Instance &aInstance, Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsAllZero(mMeshLocalIid, sizeof(mMeshLocalIid)), error = OT_ERROR_NOT_FOUND);

    memcpy(aAddress.mFields.m8, aInstance.GetThreadNetif().GetMle().GetMeshLocalPrefix().m8,
           Ip6::Address::kMeshLocalPrefixSize);

    aAddress.SetIid(mMeshLocalIid);

exit:
    return error;
}

otError Child::GetNextIp6Address(Instance &aInstance, Ip6AddressIterator &aIterator, Ip6::Address &aAddress) const
{
    otError                   error = OT_ERROR_NONE;
    otChildIp6AddressIterator index;

    // Index zero corresponds to the Mesh Local IPv6 address (if any).

    if (aIterator.Get() == 0)
    {
        aIterator.Increment();
        VerifyOrExit(GetMeshLocalIp6Address(aInstance, aAddress) == OT_ERROR_NOT_FOUND);
    }

    index = aIterator.Get() - 1;

    VerifyOrExit(index < kNumIp6Addresses, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(!mIp6Address[index].IsUnspecified(), error = OT_ERROR_NOT_FOUND);
    aAddress = mIp6Address[index];
    aIterator.Increment();

exit:
    return error;
}

otError Child::AddIp6Address(Instance &aInstance, const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    if (aInstance.GetThreadNetif().GetMle().IsMeshLocalAddress(aAddress))
    {
        VerifyOrExit(IsAllZero(mMeshLocalIid, sizeof(mMeshLocalIid)), error = OT_ERROR_ALREADY);
        memcpy(mMeshLocalIid, aAddress.GetIid(), Ip6::Address::kInterfaceIdentifierSize);
        ExitNow();
    }

    for (uint16_t index = 0; index < kNumIp6Addresses; index++)
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

otError Child::RemoveIp6Address(Instance &aInstance, const Ip6::Address &aAddress)
{
    otError  error = OT_ERROR_NOT_FOUND;
    uint16_t index;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    if (aInstance.GetThreadNetif().GetMle().IsMeshLocalAddress(aAddress))
    {
        if (memcmp(aAddress.GetIid(), mMeshLocalIid, Ip6::Address::kInterfaceIdentifierSize) == 0)
        {
            memset(mMeshLocalIid, 0, sizeof(mMeshLocalIid));
            error = OT_ERROR_NONE;
        }

        ExitNow();
    }

    for (index = 0; index < kNumIp6Addresses; index++)
    {
        VerifyOrExit(!mIp6Address[index].IsUnspecified());

        if (mIp6Address[index] == aAddress)
        {
            error = OT_ERROR_NONE;
            break;
        }
    }

    SuccessOrExit(error);

    for (; index < kNumIp6Addresses - 1; index++)
    {
        mIp6Address[index] = mIp6Address[index + 1];
    }

    mIp6Address[kNumIp6Addresses - 1].Clear();

exit:
    return error;
}

bool Child::HasIp6Address(Instance &aInstance, const Ip6::Address &aAddress) const
{
    bool retval = false;

    VerifyOrExit(!aAddress.IsUnspecified());

    if (aInstance.GetThreadNetif().GetMle().IsMeshLocalAddress(aAddress))
    {
        retval = (memcmp(aAddress.GetIid(), mMeshLocalIid, Ip6::Address::kInterfaceIdentifierSize) == 0);
        ExitNow();
    }

    for (uint16_t index = 0; index < kNumIp6Addresses; index++)
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
    Random::FillBuffer(mAttachChallenge, sizeof(mAttachChallenge));
}

const Mac::Address &Child::GetMacAddress(Mac::Address &aMacAddress) const
{
    if (mUseShortAddress)
    {
        aMacAddress.SetShort(GetRloc16());
    }
    else
    {
        aMacAddress.SetExtended(GetExtAddress());
    }

    return aMacAddress;
}

} // namespace ot
