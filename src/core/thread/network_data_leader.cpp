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
 *   This file implements the Thread Network Data managed by the Thread Leader.
 */

#define WPP_NAME "network_data_leader.tmh"

#include "network_data_leader.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace NetworkData {

LeaderBase::LeaderBase(Instance &aInstance)
    : NetworkData(aInstance, kTypeLeader)
{
    Reset();
}

void LeaderBase::Reset(void)
{
    mVersion       = static_cast<uint8_t>(otPlatRandomGet());
    mStableVersion = static_cast<uint8_t>(otPlatRandomGet());
    mLength        = 0;
    GetNotifier().Signal(OT_CHANGED_THREAD_NETDATA);
}

otError LeaderBase::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext)
{
    ThreadNetif &netif = GetNetif();
    PrefixTlv *  prefix;
    ContextTlv * contextTlv;

    aContext.mPrefixLength = 0;

    if (PrefixMatch(netif.GetMle().GetMeshLocalPrefix().m8, aAddress.mFields.m8, 64) >= 0)
    {
        aContext.mPrefix       = netif.GetMle().GetMeshLocalPrefix().m8;
        aContext.mPrefixLength = 64;
        aContext.mContextId    = 0;
        aContext.mCompressFlag = true;
    }

    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aAddress.mFields.m8, prefix->GetPrefixLength()) < 0)
        {
            continue;
        }

        contextTlv = FindContext(*prefix);

        if (contextTlv == NULL)
        {
            continue;
        }

        if (prefix->GetPrefixLength() > aContext.mPrefixLength)
        {
            aContext.mPrefix       = prefix->GetPrefix();
            aContext.mPrefixLength = prefix->GetPrefixLength();
            aContext.mContextId    = contextTlv->GetContextId();
            aContext.mCompressFlag = contextTlv->IsCompress();
        }
    }

    return (aContext.mPrefixLength > 0) ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

otError LeaderBase::GetContext(uint8_t aContextId, Lowpan::Context &aContext)
{
    otError     error = OT_ERROR_NOT_FOUND;
    PrefixTlv * prefix;
    ContextTlv *contextTlv;

    if (aContextId == 0)
    {
        aContext.mPrefix       = GetNetif().GetMle().GetMeshLocalPrefix().m8;
        aContext.mPrefixLength = 64;
        aContext.mContextId    = 0;
        aContext.mCompressFlag = true;
        ExitNow(error = OT_ERROR_NONE);
    }

    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix     = static_cast<PrefixTlv *>(cur);
        contextTlv = FindContext(*prefix);

        if (contextTlv == NULL)
        {
            continue;
        }

        if (contextTlv->GetContextId() != aContextId)
        {
            continue;
        }

        aContext.mPrefix       = prefix->GetPrefix();
        aContext.mPrefixLength = prefix->GetPrefixLength();
        aContext.mContextId    = contextTlv->GetContextId();
        aContext.mCompressFlag = contextTlv->IsCompress();
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
otError LeaderBase::GetRlocByContextId(uint8_t aContextId, uint16_t &aRloc16)
{
    otError         error = OT_ERROR_NOT_FOUND;
    Lowpan::Context lowpanContext;

    if ((GetContext(aContextId, lowpanContext)) == OT_ERROR_NONE)
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otBorderRouterConfig  config;

        while (GetNextOnMeshPrefix(&iterator, &config) == OT_ERROR_NONE)
        {
            if (otIp6PrefixMatch(&(config.mPrefix.mPrefix), reinterpret_cast<const otIp6Address *>(
                                                                lowpanContext.mPrefix)) >= config.mPrefix.mLength)
            {
                aRloc16 = config.mRloc16;
                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT

bool LeaderBase::IsOnMesh(const Ip6::Address &aAddress)
{
    PrefixTlv *prefix;
    bool       rval = false;

    if (memcmp(aAddress.mFields.m8, GetNetif().GetMle().GetMeshLocalPrefix().m8, sizeof(otMeshLocalPrefix)) == 0)
    {
        ExitNow(rval = true);
    }

    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aAddress.mFields.m8, prefix->GetPrefixLength()) < 0)
        {
            continue;
        }

        if (FindBorderRouter(*prefix) == NULL)
        {
            continue;
        }

        ExitNow(rval = true);
    }

exit:
    return rval;
}

otError LeaderBase::RouteLookup(const Ip6::Address &aSource,
                                const Ip6::Address &aDestination,
                                uint8_t *           aPrefixMatch,
                                uint16_t *          aRloc16)
{
    otError    error = OT_ERROR_NO_ROUTE;
    PrefixTlv *prefix;

    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aSource.mFields.m8, prefix->GetPrefixLength()) >= 0)
        {
            if (ExternalRouteLookup(prefix->GetDomainId(), aDestination, aPrefixMatch, aRloc16) == OT_ERROR_NONE)
            {
                ExitNow(error = OT_ERROR_NONE);
            }

            if (DefaultRouteLookup(*prefix, aRloc16) == OT_ERROR_NONE)
            {
                if (aPrefixMatch)
                {
                    *aPrefixMatch = 0;
                }

                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}

otError LeaderBase::ExternalRouteLookup(uint8_t             aDomainId,
                                        const Ip6::Address &aDestination,
                                        uint8_t *           aPrefixMatch,
                                        uint16_t *          aRloc16)
{
    ThreadNetif &   netif = GetNetif();
    otError         error = OT_ERROR_NO_ROUTE;
    PrefixTlv *     prefix;
    HasRouteTlv *   hasRoute;
    HasRouteEntry * entry;
    HasRouteEntry * rvalRoute = NULL;
    uint8_t         rval_plen = 0;
    int8_t          plen;
    NetworkDataTlv *cur;
    NetworkDataTlv *subCur;

    for (cur = reinterpret_cast<NetworkDataTlv *>(mTlvs); cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if (prefix->GetDomainId() != aDomainId)
        {
            continue;
        }

        plen = PrefixMatch(prefix->GetPrefix(), aDestination.mFields.m8, prefix->GetPrefixLength());

        if (plen > rval_plen)
        {
            // select border router
            for (subCur = prefix->GetSubTlvs(); subCur < prefix->GetNext(); subCur = subCur->GetNext())
            {
                if (subCur->GetType() != NetworkDataTlv::kTypeHasRoute)
                {
                    continue;
                }

                hasRoute = static_cast<HasRouteTlv *>(subCur);

                for (uint8_t i = 0; i < hasRoute->GetNumEntries(); i++)
                {
                    entry = hasRoute->GetEntry(i);

                    if (rvalRoute == NULL || entry->GetPreference() > rvalRoute->GetPreference() ||
                        (entry->GetPreference() == rvalRoute->GetPreference() &&
                         (entry->GetRloc() == netif.GetMle().GetRloc16() ||
                          (rvalRoute->GetRloc() != netif.GetMle().GetRloc16() &&
                           netif.GetMle().GetCost(entry->GetRloc()) < netif.GetMle().GetCost(rvalRoute->GetRloc())))))
                    {
                        rvalRoute = entry;
                        rval_plen = static_cast<uint8_t>(plen);
                    }
                }
            }
        }
    }

    if (rvalRoute != NULL)
    {
        if (aRloc16 != NULL)
        {
            *aRloc16 = rvalRoute->GetRloc();
        }

        if (aPrefixMatch != NULL)
        {
            *aPrefixMatch = rval_plen;
        }

        error = OT_ERROR_NONE;
    }

    return error;
}

otError LeaderBase::DefaultRouteLookup(PrefixTlv &aPrefix, uint16_t *aRloc16)
{
    ThreadNetif &      netif = GetNetif();
    otError            error = OT_ERROR_NO_ROUTE;
    BorderRouterTlv *  borderRouter;
    BorderRouterEntry *entry;
    BorderRouterEntry *route = NULL;

    for (NetworkDataTlv *cur = aPrefix.GetSubTlvs(); cur < aPrefix.GetNext(); cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypeBorderRouter)
        {
            continue;
        }

        borderRouter = static_cast<BorderRouterTlv *>(cur);

        for (uint8_t i = 0; i < borderRouter->GetNumEntries(); i++)
        {
            entry = borderRouter->GetEntry(i);

            if (entry->IsDefaultRoute() == false)
            {
                continue;
            }

            if (route == NULL || entry->GetPreference() > route->GetPreference() ||
                (entry->GetPreference() == route->GetPreference() &&
                 (entry->GetRloc() == netif.GetMle().GetRloc16() ||
                  (route->GetRloc() != netif.GetMle().GetRloc16() &&
                   netif.GetMle().GetCost(entry->GetRloc()) < netif.GetMle().GetCost(route->GetRloc())))))
            {
                route = entry;
            }
        }
    }

    if (route != NULL)
    {
        if (aRloc16 != NULL)
        {
            *aRloc16 = route->GetRloc();
        }

        error = OT_ERROR_NONE;
    }

    return error;
}

otError LeaderBase::SetNetworkData(uint8_t        aVersion,
                                   uint8_t        aStableVersion,
                                   bool           aStable,
                                   const Message &aMessage,
                                   uint16_t       aMessageOffset)
{
    otError  error = OT_ERROR_NONE;
    Mle::Tlv tlv;
    uint16_t length;

    length = aMessage.Read(aMessageOffset, sizeof(tlv), &tlv);
    VerifyOrExit(length == sizeof(tlv), error = OT_ERROR_PARSE);

    length = aMessage.Read(aMessageOffset + sizeof(tlv), tlv.GetLength(), mTlvs);
    VerifyOrExit(length == tlv.GetLength(), error = OT_ERROR_PARSE);

    mLength        = tlv.GetLength();
    mVersion       = aVersion;
    mStableVersion = aStableVersion;

    if (aStable)
    {
        RemoveTemporaryData(mTlvs, mLength);
    }

#if OPENTHREAD_FTD
    // Synchronize internal 6LoWPAN Context ID Set with recently obtained Network Data.
    if (GetNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        GetNetif().GetNetworkDataLeader().UpdateContextsAfterReset();
    }
#endif

    otDumpDebgNetData("set network data", mTlvs, mLength);

    GetNotifier().Signal(OT_CHANGED_THREAD_NETDATA);

exit:
    return error;
}

otError LeaderBase::SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength)
{
    otError               error     = OT_ERROR_NONE;
    uint8_t               remaining = kMaxSize - mLength;
    CommissioningDataTlv *commissioningDataTlv;

    VerifyOrExit(sizeof(NetworkDataTlv) + aValueLength < remaining, error = OT_ERROR_NO_BUFS);

    RemoveCommissioningData();

    if (aValueLength > 0)
    {
        commissioningDataTlv = reinterpret_cast<CommissioningDataTlv *>(mTlvs + mLength);
        Insert(reinterpret_cast<uint8_t *>(commissioningDataTlv), sizeof(CommissioningDataTlv) + aValueLength);
        commissioningDataTlv->Init();
        commissioningDataTlv->SetLength(aValueLength);
        memcpy(commissioningDataTlv->GetValue(), aValue, aValueLength);
    }

    mVersion++;
    GetNotifier().Signal(OT_CHANGED_THREAD_NETDATA);

exit:
    return error;
}

NetworkDataTlv *LeaderBase::GetCommissioningData(void)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext())
    {
        if (cur->GetType() == NetworkDataTlv::kTypeCommissioningData)
        {
            ExitNow();
        }
    }

    cur = NULL;

exit:
    return cur;
}

MeshCoP::Tlv *LeaderBase::GetCommissioningDataSubTlv(MeshCoP::Tlv::Type aType)
{
    MeshCoP::Tlv *  rval = NULL;
    NetworkDataTlv *commissioningDataTlv;
    MeshCoP::Tlv *  cur;
    MeshCoP::Tlv *  end;

    commissioningDataTlv = GetCommissioningData();
    VerifyOrExit(commissioningDataTlv != NULL);

    cur = reinterpret_cast<MeshCoP::Tlv *>(commissioningDataTlv->GetValue());
    end = reinterpret_cast<MeshCoP::Tlv *>(commissioningDataTlv->GetValue() + commissioningDataTlv->GetLength());

    for (; cur < end; cur = cur->GetNext())
    {
        if (cur->GetType() == aType)
        {
            ExitNow(rval = cur);
        }
    }

exit:
    return rval;
}

bool LeaderBase::IsJoiningEnabled(void)
{
    MeshCoP::Tlv *steeringData;
    bool          rval = false;

    VerifyOrExit(GetCommissioningDataSubTlv(MeshCoP::Tlv::kBorderAgentLocator) != NULL);

    steeringData = GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);
    VerifyOrExit(steeringData != NULL);

    for (int i = 0; i < steeringData->GetLength(); i++)
    {
        if (steeringData->GetValue()[i] != 0)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

otError LeaderBase::RemoveCommissioningData(void)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        if (cur->GetType() == NetworkDataTlv::kTypeCommissioningData)
        {
            Remove(reinterpret_cast<uint8_t *>(cur), sizeof(NetworkDataTlv) + cur->GetLength());
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

} // namespace NetworkData
} // namespace ot
