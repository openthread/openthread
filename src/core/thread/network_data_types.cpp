/*
 *  Copyright (c) 2016-21, The OpenThread Authors.
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
 *   This file implements config types for Thread Network Data.
 */

#include "network_data_types.hpp"

#include "common/instance.hpp"
#include "thread/network_data_tlvs.hpp"

namespace ot {
namespace NetworkData {

void OnMeshPrefixConfig::SetFrom(const PrefixTlv &        aPrefixTlv,
                                 const BorderRouterTlv &  aBorderRouterTlv,
                                 const BorderRouterEntry &aBorderRouterEntry)
{
    Clear();

    aPrefixTlv.CopyPrefixTo(GetPrefix());
    mPreference   = aBorderRouterEntry.GetPreference();
    mPreferred    = aBorderRouterEntry.IsPreferred();
    mSlaac        = aBorderRouterEntry.IsSlaac();
    mDhcp         = aBorderRouterEntry.IsDhcp();
    mConfigure    = aBorderRouterEntry.IsConfigure();
    mDefaultRoute = aBorderRouterEntry.IsDefaultRoute();
    mOnMesh       = aBorderRouterEntry.IsOnMesh();
    mStable       = aBorderRouterTlv.IsStable();
    mRloc16       = aBorderRouterEntry.GetRloc();
    mNdDns        = aBorderRouterEntry.IsNdDns();
    mDp           = aBorderRouterEntry.IsDp();
}

void ExternalRouteConfig::SetFrom(Instance &           aInstance,
                                  const PrefixTlv &    aPrefixTlv,
                                  const HasRouteTlv &  aHasRouteTlv,
                                  const HasRouteEntry &aHasRouteEntry)
{
    Clear();

    aPrefixTlv.CopyPrefixTo(GetPrefix());
    mPreference          = aHasRouteEntry.GetPreference();
    mNat64               = aHasRouteEntry.IsNat64();
    mStable              = aHasRouteTlv.IsStable();
    mRloc16              = aHasRouteEntry.GetRloc();
    mNextHopIsThisDevice = (aHasRouteEntry.GetRloc() == aInstance.Get<Mle::MleRouter>().GetRloc16());
}

bool ServiceConfig::ServerConfig::operator==(const ServerConfig &aOther) const
{
    return (mStable == aOther.mStable) && (mServerDataLength == aOther.mServerDataLength) &&
           (memcmp(mServerData, aOther.mServerData, mServerDataLength) == 0);
}

void ServiceConfig::ServerConfig::SetFrom(const ServerTlv &aServerTlv)
{
    mStable           = aServerTlv.IsStable();
    mRloc16           = aServerTlv.GetServer16();
    mServerDataLength = aServerTlv.GetServerDataLength();
    memcpy(&mServerData, aServerTlv.GetServerData(), mServerDataLength);
}

bool ServiceConfig::operator==(const ServiceConfig &aOther) const
{
    return (mEnterpriseNumber == aOther.mEnterpriseNumber) && (mServiceDataLength == aOther.mServiceDataLength) &&
           (memcmp(mServiceData, aOther.mServiceData, mServiceDataLength) == 0) &&
           (GetServerConfig() == aOther.GetServerConfig());
}

void ServiceConfig::SetFrom(const ServiceTlv &aServiceTlv, const ServerTlv &aServerTlv)
{
    Clear();

    mServiceId         = aServiceTlv.GetServiceId();
    mEnterpriseNumber  = aServiceTlv.GetEnterpriseNumber();
    mServiceDataLength = aServiceTlv.GetServiceDataLength();
    memcpy(&mServiceData, aServiceTlv.GetServiceData(), mServiceDataLength);
    GetServerConfig().SetFrom(aServerTlv);
}

} // namespace NetworkData
} // namespace ot
