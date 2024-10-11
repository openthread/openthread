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
 *   This file includes definitions for a Thread `Child`.
 */

#include "child.hpp"

#include "instance/instance.hpp"

namespace ot {

#if OPENTHREAD_FTD

//---------------------------------------------------------------------------------------------------------------------
// Child::Info

void Child::Info::SetFrom(const Child &aChild)
{
    Clear();
    mExtAddress          = aChild.GetExtAddress();
    mTimeout             = aChild.GetTimeout();
    mRloc16              = aChild.GetRloc16();
    mChildId             = Mle::ChildIdFromRloc16(aChild.GetRloc16());
    mNetworkDataVersion  = aChild.GetNetworkDataVersion();
    mAge                 = Time::MsecToSec(TimerMilli::GetNow() - aChild.GetLastHeard());
    mLinkQualityIn       = aChild.GetLinkQualityIn();
    mAverageRssi         = aChild.GetLinkInfo().GetAverageRss();
    mLastRssi            = aChild.GetLinkInfo().GetLastRss();
    mFrameErrorRate      = aChild.GetLinkInfo().GetFrameErrorRate();
    mMessageErrorRate    = aChild.GetLinkInfo().GetMessageErrorRate();
    mQueuedMessageCnt    = aChild.GetIndirectMessageCount();
    mVersion             = ClampToUint8(aChild.GetVersion());
    mRxOnWhenIdle        = aChild.IsRxOnWhenIdle();
    mFullThreadDevice    = aChild.IsFullThreadDevice();
    mFullNetworkData     = (aChild.GetNetworkDataType() == NetworkData::kFullSet);
    mIsStateRestoring    = aChild.IsStateRestoring();
    mSupervisionInterval = aChild.GetSupervisionInterval();
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mIsCslSynced = aChild.IsCslSynchronized();
#else
    mIsCslSynced = false;
#endif
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    mConnectionTime = aChild.GetConnectionTime();
#endif
}

//---------------------------------------------------------------------------------------------------------------------
// Child::Ip6AddrEntry

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

MlrState Child::Ip6AddrEntry::GetMlrState(const Child &aChild) const
{
    MlrState                   state = kMlrStateRegistering;
    Ip6AddressArray::IndexType index;

    OT_ASSERT(aChild.mIp6Addresses.IsInArrayBuffer(this));

    index = aChild.mIp6Addresses.IndexOf(*this);

    if (aChild.mMlrToRegisterSet.Has(index))
    {
        state = kMlrStateToRegister;
    }
    else if (aChild.mMlrRegisteredSet.Has(index))
    {
        state = kMlrStateRegistered;
    }

    return state;
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Child::Ip6AddrEntry::SetMlrState(MlrState aState, Child &aChild)
{
    Ip6AddressArray::IndexType index;

    OT_ASSERT(aChild.mIp6Addresses.IsInArrayBuffer(this));

    index = aChild.mIp6Addresses.IndexOf(*this);

    aChild.mMlrToRegisterSet.Update(index, aState == kMlrStateToRegister);
    aChild.mMlrRegisteredSet.Update(index, aState == kMlrStateRegistered);
}

#endif // OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Child

void Child::Clear(void)
{
    Instance &instance = GetInstance();

    ClearAllBytes(*this);
    Init(instance);
}

void Child::ClearIp6Addresses(void)
{
    mMeshLocalIid.Clear();
    mIp6Addresses.Clear();
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    mMlrToRegisterSet.Clear();
    mMlrRegisteredSet.Clear();
#endif
}

void Child::SetDeviceMode(Mle::DeviceMode aMode)
{
    VerifyOrExit(aMode != GetDeviceMode());

    Neighbor::SetDeviceMode(aMode);

    VerifyOrExit(IsStateValid());
    Get<NeighborTable>().Signal(NeighborTable::kChildModeChanged, *this);

exit:
    return;
}

Error Child::GetMeshLocalIp6Address(Ip6::Address &aAddress) const
{
    Error error = kErrorNone;

    VerifyOrExit(!mMeshLocalIid.IsUnspecified(), error = kErrorNotFound);

    aAddress.SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    aAddress.SetIid(mMeshLocalIid);

exit:
    return error;
}

Error Child::GetNextIp6Address(AddressIterator &aIterator, Ip6::Address &aAddress) const
{
    Error error = kErrorNone;

    if (aIterator == 0)
    {
        aIterator++;

        if (GetMeshLocalIp6Address(aAddress) == kErrorNone)
        {
            ExitNow();
        }
    }

    VerifyOrExit(aIterator - 1 < mIp6Addresses.GetLength(), error = kErrorNotFound);

    aAddress = mIp6Addresses[static_cast<Ip6AddressArray::IndexType>(aIterator - 1)];
    aIterator++;

exit:
    return error;
}

Error Child::AddIp6Address(const Ip6::Address &aAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(!aAddress.IsUnspecified(), error = kErrorInvalidArgs);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        VerifyOrExit(mMeshLocalIid.IsUnspecified(), error = kErrorAlready);
        mMeshLocalIid = aAddress.GetIid();
        ExitNow();
    }

    VerifyOrExit(!mIp6Addresses.ContainsMatching(aAddress), error = kErrorAlready);
    error = mIp6Addresses.PushBack(static_cast<const Ip6AddrEntry &>(aAddress));

exit:
    return error;
}

Error Child::RemoveIp6Address(const Ip6::Address &aAddress)
{
    Error         error = kErrorNotFound;
    Ip6AddrEntry *entry;

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        if (aAddress.GetIid() == mMeshLocalIid)
        {
            mMeshLocalIid.Clear();
            error = kErrorNone;
        }

        ExitNow();
    }

    entry = mIp6Addresses.FindMatching(aAddress);
    VerifyOrExit(entry != nullptr);

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    {
        // `Array::Remove()` will replace the removed entry with the
        // last one in the array. We also update the MLR bit vectors
        // to reflect this change.

        uint16_t entryIndex = mIp6Addresses.IndexOf(*entry);
        uint16_t lastIndex  = mIp6Addresses.GetLength() - 1;

        mMlrToRegisterSet.Update(entryIndex, mMlrToRegisterSet.Has(lastIndex));
        mMlrToRegisterSet.Remove(lastIndex);

        mMlrRegisteredSet.Update(entryIndex, mMlrRegisteredSet.Has(lastIndex));
        mMlrRegisteredSet.Remove(lastIndex);
    }
#endif

    mIp6Addresses.Remove(*entry);
    error = kErrorNone;

exit:
    return error;
}

bool Child::HasIp6Address(const Ip6::Address &aAddress) const
{
    bool hasAddress = false;

    VerifyOrExit(!aAddress.IsUnspecified());

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        hasAddress = (aAddress.GetIid() == mMeshLocalIid);
        ExitNow();
    }

    hasAddress = mIp6Addresses.ContainsMatching(aAddress);

exit:
    return hasAddress;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
Error Child::GetDomainUnicastAddress(Ip6::Address &aAddress) const
{
    Error error = kErrorNotFound;

    for (const Ip6::Address &ip6Address : mIp6Addresses)
    {
        if (Get<BackboneRouter::Leader>().IsDomainUnicast(ip6Address))
        {
            aAddress = ip6Address;
            error    = kErrorNone;
            ExitNow();
        }
    }

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

bool Child::HasMlrRegisteredAddress(const Ip6::Address &aAddress) const
{
    bool                hasAddress = false;
    const Ip6AddrEntry *entry;

    entry = mIp6Addresses.FindMatching(aAddress);
    VerifyOrExit(entry != nullptr);
    hasAddress = entry->GetMlrState(*this) == kMlrStateRegistered;

exit:
    return hasAddress;
}

#endif

#endif // OPENTHREAD_FTD

} // namespace ot
