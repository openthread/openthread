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

#include "topology.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"

namespace ot {

void Neighbor::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);
    mLinkInfo.Init(aInstance);
    SetState(kStateInvalid);
}

bool Neighbor::IsStateValidOrAttaching(void) const
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

bool Neighbor::MatchesFilter(StateFilter aFilter) const
{
    bool matches = false;

    switch (aFilter)
    {
    case kInStateValid:
        matches = IsStateValid();
        break;

    case kInStateValidOrRestoring:
        matches = IsStateValidOrRestoring();
        break;

    case kInStateChildIdRequest:
        matches = IsStateChildIdRequest();
        break;

    case kInStateValidOrAttaching:
        matches = IsStateValidOrAttaching();
        break;

    case kInStateAnyExceptInvalid:
        matches = !IsStateInvalid();
        break;

    case kInStateAnyExceptValidOrRestoring:
        matches = !IsStateValidOrRestoring();
        break;
    }

    return matches;
}

void Neighbor::GenerateChallenge(void)
{
    IgnoreError(
        Random::Crypto::FillBuffer(mValidPending.mPending.mChallenge, sizeof(mValidPending.mPending.mChallenge)));
}

const Ip6::Address *Child::AddressIterator::GetAddress(void) const
{
    // `mIndex` value of zero indicates mesh-local IPv6 address.
    // Non-zero value specifies the index into address array starting
    // from one for first element (i.e, `mIndex - 1` gives the array
    // index).

    return (mIndex == 0) ? &mMeshLocalAddress : ((mIndex < kMaxIndex) ? &mChild.mIp6Address[mIndex - 1] : nullptr);
}

void Child::AddressIterator::Update(void)
{
    const Ip6::Address *address;

    while (true)
    {
        if ((mIndex == 0) && (mChild.GetMeshLocalIp6Address(mMeshLocalAddress) != OT_ERROR_NONE))
        {
            mIndex++;
        }

        address = GetAddress();

        VerifyOrExit((address != nullptr) && !address->IsUnspecified(), mIndex = kMaxIndex);

        VerifyOrExit(!address->MatchesFilter(mFilter), OT_NOOP);
        mIndex++;
    }

exit:
    return;
}

void Child::Clear(void)
{
    Instance &instance = GetInstance();

    memset(reinterpret_cast<void *>(this), 0, sizeof(Child));
    Init(instance);
}

void Child::ClearIp6Addresses(void)
{
    mMeshLocalIid.Clear();
    memset(mIp6Address, 0, sizeof(mIp6Address));
}

otError Child::GetMeshLocalIp6Address(Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mMeshLocalIid.IsUnspecified(), error = OT_ERROR_NOT_FOUND);

    aAddress.SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    aAddress.SetIid(mMeshLocalIid);

exit:
    return error;
}

otError Child::AddIp6Address(const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        VerifyOrExit(mMeshLocalIid.IsUnspecified(), error = OT_ERROR_ALREADY);
        mMeshLocalIid = aAddress.GetIid();
        ExitNow();
    }

    for (Ip6::Address &ip6Address : mIp6Address)
    {
        if (ip6Address.IsUnspecified())
        {
            ip6Address = aAddress;
            ExitNow();
        }

        VerifyOrExit(ip6Address != aAddress, error = OT_ERROR_ALREADY);
    }

    error = OT_ERROR_NO_BUFS;

exit:
    return error;
}

otError Child::RemoveIp6Address(const Ip6::Address &aAddress)
{
    otError  error = OT_ERROR_NOT_FOUND;
    uint16_t index;

    VerifyOrExit(!aAddress.IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        if (aAddress.GetIid() == mMeshLocalIid)
        {
            mMeshLocalIid.Clear();
            error = OT_ERROR_NONE;
        }

        ExitNow();
    }

    for (index = 0; index < kNumIp6Addresses; index++)
    {
        VerifyOrExit(!mIp6Address[index].IsUnspecified(), OT_NOOP);

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

bool Child::HasIp6Address(const Ip6::Address &aAddress) const
{
    bool retval = false;

    VerifyOrExit(!aAddress.IsUnspecified(), OT_NOOP);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        retval = (aAddress.GetIid() == mMeshLocalIid);
        ExitNow();
    }

    for (const Ip6::Address &ip6Address : mIp6Address)
    {
        VerifyOrExit(!ip6Address.IsUnspecified(), OT_NOOP);

        if (ip6Address == aAddress)
        {
            ExitNow(retval = true);
        }
    }

exit:
    return retval;
}

void Child::GenerateChallenge(void)
{
    IgnoreError(Random::Crypto::FillBuffer(mAttachChallenge, sizeof(mAttachChallenge)));
}

void Router::Clear(void)
{
    Instance &instance = GetInstance();

    memset(reinterpret_cast<void *>(this), 0, sizeof(Router));
    Init(instance);
}

} // namespace ot
