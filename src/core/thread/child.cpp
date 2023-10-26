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

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

#if OPENTHREAD_FTD

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

    if ((mIndex == 0) && (mChild.GetMeshLocalIp6Address(mMeshLocalAddress) != kErrorNone))
    {
        mIndex++;
    }

    while (true)
    {
        address = GetAddress();

        VerifyOrExit((address != nullptr) && !address->IsUnspecified(), mIndex = kMaxIndex);

        VerifyOrExit(!address->MatchesFilter(mFilter));
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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    mMlrToRegisterMask.Clear();
    mMlrRegisteredMask.Clear();
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

    for (Ip6::Address &ip6Address : mIp6Address)
    {
        if (ip6Address.IsUnspecified())
        {
            ip6Address = aAddress;
            ExitNow();
        }

        VerifyOrExit(ip6Address != aAddress, error = kErrorAlready);
    }

    error = kErrorNoBufs;

exit:
    return error;
}

Error Child::RemoveIp6Address(const Ip6::Address &aAddress)
{
    Error    error = kErrorNotFound;
    uint16_t index;

    VerifyOrExit(!aAddress.IsUnspecified(), error = kErrorInvalidArgs);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        if (aAddress.GetIid() == mMeshLocalIid)
        {
            mMeshLocalIid.Clear();
            error = kErrorNone;
        }

        ExitNow();
    }

    for (index = 0; index < kNumIp6Addresses; index++)
    {
        VerifyOrExit(!mIp6Address[index].IsUnspecified());

        if (mIp6Address[index] == aAddress)
        {
            error = kErrorNone;
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

    VerifyOrExit(!aAddress.IsUnspecified());

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        retval = (aAddress.GetIid() == mMeshLocalIid);
        ExitNow();
    }

    for (const Ip6::Address &ip6Address : mIp6Address)
    {
        VerifyOrExit(!ip6Address.IsUnspecified());

        if (ip6Address == aAddress)
        {
            ExitNow(retval = true);
        }
    }

exit:
    return retval;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
const Ip6::Address *Child::GetDomainUnicastAddress(void) const
{
    const Ip6::Address *addr = nullptr;

    for (const Ip6::Address &ip6Address : mIp6Address)
    {
        VerifyOrExit(!ip6Address.IsUnspecified());

        if (Get<BackboneRouter::Leader>().IsDomainUnicast(ip6Address))
        {
            ExitNow(addr = &ip6Address);
        }
    }

exit:
    return addr;
}
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
bool Child::HasMlrRegisteredAddress(const Ip6::Address &aAddress) const
{
    bool has = false;

    VerifyOrExit(mMlrRegisteredMask.HasAny());

    for (const Ip6::Address &address : IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (GetAddressMlrState(address) == kMlrStateRegistered && address == aAddress)
        {
            ExitNow(has = true);
        }
    }

exit:
    return has;
}

MlrState Child::GetAddressMlrState(const Ip6::Address &aAddress) const
{
    uint16_t addressIndex;

    OT_ASSERT(&mIp6Address[0] <= &aAddress && &aAddress < GetArrayEnd(mIp6Address));

    addressIndex = static_cast<uint16_t>(&aAddress - mIp6Address);

    return mMlrToRegisterMask.Get(addressIndex)
               ? kMlrStateToRegister
               : (mMlrRegisteredMask.Get(addressIndex) ? kMlrStateRegistered : kMlrStateRegistering);
}

void Child::SetAddressMlrState(const Ip6::Address &aAddress, MlrState aState)
{
    uint16_t addressIndex;

    OT_ASSERT(&mIp6Address[0] <= &aAddress && &aAddress < GetArrayEnd(mIp6Address));

    addressIndex = static_cast<uint16_t>(&aAddress - mIp6Address);

    mMlrToRegisterMask.Set(addressIndex, aState == kMlrStateToRegister);
    mMlrRegisteredMask.Set(addressIndex, aState == kMlrStateRegistered);
}
#endif // OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

#endif // OPENTHREAD_FTD

} // namespace ot
