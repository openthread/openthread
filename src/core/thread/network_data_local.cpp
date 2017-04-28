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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/code_utils.hpp>
#include <mac/mac_frame.hpp>
#include <thread/network_data_local.hpp>
#include <thread/thread_netif.hpp>

#if OPENTHREAD_FTD

namespace ot {
namespace NetworkData {

Local::Local(ThreadNetif &aThreadNetif):
    NetworkData(aThreadNetif, true),
    mOldRloc(Mac::kShortAddrInvalid)
{
}

ThreadError Local::AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf,
                                   uint8_t aFlags, bool aStable)
{
    PrefixTlv *prefixTlv;
    BorderRouterTlv *brTlv;

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
    return kThreadError_None;
}

ThreadError Local::RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    ThreadError error = kThreadError_None;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = kThreadError_Error);
    VerifyOrExit(FindBorderRouter(*tlv) != NULL, error = kThreadError_Error);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);
    return error;
}

ThreadError Local::AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable)
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
    return kThreadError_None;
}

ThreadError Local::RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    ThreadError error = kThreadError_None;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = kThreadError_Error);
    VerifyOrExit(FindHasRoute(*tlv) != NULL, error = kThreadError_Error);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);
    return error;
}

ThreadError Local::UpdateRloc(void)
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

    return kThreadError_None;
}

ThreadError Local::UpdateRloc(PrefixTlv &aPrefix)
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

    return kThreadError_None;
}

ThreadError Local::UpdateRloc(HasRouteTlv &aHasRoute)
{
    HasRouteEntry *entry = aHasRoute.GetEntry(0);
    entry->SetRloc(mNetif.GetMle().GetRloc16());
    return kThreadError_None;
}

ThreadError Local::UpdateRloc(BorderRouterTlv &aBorderRouter)
{
    BorderRouterEntry *entry = aBorderRouter.GetEntry(0);
    entry->SetRloc(mNetif.GetMle().GetRloc16());
    return kThreadError_None;
}

bool Local::IsOnMeshPrefixConsistent(void)
{
    return (mNetif.GetNetworkDataLeader().ContainsOnMeshPrefixes(*this, mNetif.GetMle().GetRloc16()) &&
            ContainsOnMeshPrefixes(mNetif.GetNetworkDataLeader(), mNetif.GetMle().GetRloc16()));
}

bool Local::IsExternalRouteConsistent(void)
{
    return (mNetif.GetNetworkDataLeader().ContainsExternalRoutes(*this, mNetif.GetMle().GetRloc16()) &&
            ContainsExternalRoutes(mNetif.GetNetworkDataLeader(), mNetif.GetMle().GetRloc16()));
}

ThreadError Local::SendServerDataNotification(void)
{
    ThreadError error = kThreadError_None;
    uint16_t rloc = mNetif.GetMle().GetRloc16();

    if ((mNetif.GetMle().GetDeviceMode() & Mle::ModeTlv::kModeFFD) != 0 &&
        (mNetif.GetMle().IsRouterRoleEnabled()) &&
        (mNetif.GetMle().GetDeviceState() < Mle::kDeviceStateRouter) &&
        (mNetif.GetMle().GetActiveRouterCount() < mNetif.GetMle().GetRouterUpgradeThreshold()))
    {
        ExitNow(error = kThreadError_InvalidState);
    }

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

#endif // OPENTHREAD_FTD
