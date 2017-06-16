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
 *   This file implements the local Thread Network Data.
 */

#include <openthread/config.h>

#include "network_data_local.hpp"

#include "common/debug.hpp"
#include "common/logging.hpp"
#include "common/code_utils.hpp"
#include "mac/mac_frame.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_BORDER_ROUTER

namespace ot {
namespace NetworkData {

Local::Local(ThreadNetif &aThreadNetif):
    NetworkData(aThreadNetif, true),
    mOldRloc(Mac::kShortAddrInvalid)
{
}

otError Local::AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf,
                               uint8_t aFlags, bool aStable)
{
    otError error = OT_ERROR_NONE;
    PrefixTlv *prefixTlv;
    BorderRouterTlv *brTlv;

    VerifyOrExit(Ip6::Address::PrefixMatch(aPrefix, GetNetif().GetMle().GetMeshLocalPrefix(),
                                           (aPrefixLength + 7) / 8) < Ip6::Address::kMeshLocalPrefixLength,
                 error = OT_ERROR_INVALID_ARGS);

    RemoveOnMeshPrefix(aPrefix, aPrefixLength);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv),
           sizeof(PrefixTlv) + BitVectorBytes(aPrefixLength) + sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry));
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry));

    brTlv = static_cast<BorderRouterTlv *>(prefixTlv->GetSubTlvs());
    brTlv->Init();
    brTlv->SetLength(brTlv->GetLength() + sizeof(BorderRouterEntry));
    brTlv->GetEntry(0)->Init();
    brTlv->GetEntry(0)->SetPreference(aPrf);
    brTlv->GetEntry(0)->SetFlags(aFlags);

    if (aStable)
    {
        prefixTlv->SetStable();
        brTlv->SetStable();
    }

    ClearResubmitDelayTimer();

    otDumpDebgNetData(GetInstance(), "add prefix done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    otError error = OT_ERROR_NONE;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(FindBorderRouter(*tlv) != NULL, error = OT_ERROR_NOT_FOUND);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);
    return error;
}

otError Local::AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable)
{
    PrefixTlv *prefixTlv;
    HasRouteTlv *hasRouteTlv;

    RemoveHasRoutePrefix(aPrefix, aPrefixLength);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv),
           sizeof(PrefixTlv) + BitVectorBytes(aPrefixLength) + sizeof(HasRouteTlv) + sizeof(HasRouteEntry));
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(HasRouteTlv) + sizeof(HasRouteEntry));

    hasRouteTlv = static_cast<HasRouteTlv *>(prefixTlv->GetSubTlvs());
    hasRouteTlv->Init();
    hasRouteTlv->SetLength(hasRouteTlv->GetLength() + sizeof(HasRouteEntry));
    hasRouteTlv->GetEntry(0)->Init();
    hasRouteTlv->GetEntry(0)->SetPreference(aPrf);

    if (aStable)
    {
        prefixTlv->SetStable();
        hasRouteTlv->SetStable();
    }

    ClearResubmitDelayTimer();

    otDumpDebgNetData(GetInstance(), "add route done", mTlvs, mLength);
    return OT_ERROR_NONE;
}

otError Local::RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    otError error = OT_ERROR_NONE;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(FindHasRoute(*tlv) != NULL, error = OT_ERROR_NOT_FOUND);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);
    return error;
}

otError Local::UpdateRloc(void)
{
    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            UpdateRloc(*static_cast<PrefixTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }

    ClearResubmitDelayTimer();

    return OT_ERROR_NONE;
}

otError Local::UpdateRloc(PrefixTlv &aPrefix)
{
    for (NetworkDataTlv *cur = aPrefix.GetSubTlvs(); cur < aPrefix.GetNext(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            UpdateRloc(*static_cast<HasRouteTlv *>(cur));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            UpdateRloc(*static_cast<BorderRouterTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }

    return OT_ERROR_NONE;
}

otError Local::UpdateRloc(HasRouteTlv &aHasRoute)
{
    HasRouteEntry *entry = aHasRoute.GetEntry(0);
    entry->SetRloc(GetNetif().GetMle().GetRloc16());
    return OT_ERROR_NONE;
}

otError Local::UpdateRloc(BorderRouterTlv &aBorderRouter)
{
    BorderRouterEntry *entry = aBorderRouter.GetEntry(0);
    entry->SetRloc(GetNetif().GetMle().GetRloc16());
    return OT_ERROR_NONE;
}

bool Local::IsOnMeshPrefixConsistent(void)
{
    ThreadNetif &netif = GetNetif();

    return (netif.GetNetworkDataLeader().ContainsOnMeshPrefixes(*this, netif.GetMle().GetRloc16()) &&
            ContainsOnMeshPrefixes(netif.GetNetworkDataLeader(), netif.GetMle().GetRloc16()));
}

bool Local::IsExternalRouteConsistent(void)
{
    ThreadNetif &netif = GetNetif();

    return (netif.GetNetworkDataLeader().ContainsExternalRoutes(*this, netif.GetMle().GetRloc16()) &&
            ContainsExternalRoutes(netif.GetNetworkDataLeader(), netif.GetMle().GetRloc16()));
}

otError Local::SendServerDataNotification(void)
{
    ThreadNetif &netif = GetNetif();
    Mle::MleRouter &mle = netif.GetMle();
    otError error = OT_ERROR_NONE;
    uint16_t rloc = mle.GetRloc16();

#if OPENTHREAD_FTD

    // Don't send this Server Data Notification if the device is going to upgrade to Router
    if ((mle.GetDeviceMode() & Mle::ModeTlv::kModeFFD) != 0 &&
        (mle.IsRouterRoleEnabled()) &&
        (mle.GetRole() < OT_DEVICE_ROLE_ROUTER) &&
        (mle.GetActiveRouterCount() < mle.GetRouterUpgradeThreshold()))
    {
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

#endif

    UpdateRloc();

    VerifyOrExit(!IsOnMeshPrefixConsistent() || !IsExternalRouteConsistent(), ClearResubmitDelayTimer());

    if (mOldRloc == rloc)
    {
        mOldRloc = Mac::kShortAddrInvalid;
    }

    SuccessOrExit(error = NetworkData::SendServerDataNotification(mOldRloc));
    mOldRloc = rloc;

exit:
    return error;
}

}  // namespace NetworkData
}  // namespace ot

#endif // OPENTHREAD_ENABLE_BORDER_ROUTER
