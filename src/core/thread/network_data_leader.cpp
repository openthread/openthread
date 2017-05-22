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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "network_data_leader.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "thread/mle_router.hpp"
#include "thread/lowpan.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace NetworkData {

LeaderBase::LeaderBase(ThreadNetif &aThreadNetif):
    NetworkData(aThreadNetif, false)
{
    Reset();
}

void LeaderBase::Reset(void)
{
    mVersion = static_cast<uint8_t>(otPlatRandomGet());
    mStableVersion = static_cast<uint8_t>(otPlatRandomGet());
    mLength = 0;
    mNetif.SetStateChangedFlags(OT_THREAD_NETDATA_UPDATED);
}

ThreadError LeaderBase::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext)
{
    PrefixTlv *prefix;
    ContextTlv *contextTlv;

    aContext.mPrefixLength = 0;

    if (PrefixMatch(mNetif.GetMle().GetMeshLocalPrefix(), aAddress.mFields.m8, 64) >= 0)
    {
        aContext.mPrefix = mNetif.GetMle().GetMeshLocalPrefix();
        aContext.mPrefixLength = 64;
        aContext.mContextId = 0;
        aContext.mCompressFlag = true;
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
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
            aContext.mPrefix = prefix->GetPrefix();
            aContext.mPrefixLength = prefix->GetPrefixLength();
            aContext.mContextId = contextTlv->GetContextId();
            aContext.mCompressFlag = contextTlv->IsCompress();
        }
    }

    return (aContext.mPrefixLength > 0) ? kThreadError_None : kThreadError_Error;
}

ThreadError LeaderBase::GetContext(uint8_t aContextId, Lowpan::Context &aContext)
{
    ThreadError error = kThreadError_Error;
    PrefixTlv *prefix;
    ContextTlv *contextTlv;

    if (aContextId == 0)
    {
        aContext.mPrefix = mNetif.GetMle().GetMeshLocalPrefix();
        aContext.mPrefixLength = 64;
        aContext.mContextId = 0;
        aContext.mCompressFlag = true;
        ExitNow(error = kThreadError_None);
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);
        contextTlv = FindContext(*prefix);

        if (contextTlv == NULL)
        {
            continue;
        }

        if (contextTlv->GetContextId() != aContextId)
        {
            continue;
        }

        aContext.mPrefix = prefix->GetPrefix();
        aContext.mPrefixLength = prefix->GetPrefixLength();
        aContext.mContextId = contextTlv->GetContextId();
        aContext.mCompressFlag = contextTlv->IsCompress();
        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
ThreadError LeaderBase::GetRlocByContextId(uint8_t aContextId, uint16_t &aRloc16)
{
    ThreadError error = kThreadError_NotFound;
    Lowpan::Context lowpanContext;

    if ((GetContext(aContextId, lowpanContext)) == kThreadError_None)
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otBorderRouterConfig config;

        while (GetNextOnMeshPrefix(&iterator, &config) == kThreadError_None)
        {
            if (otIp6PrefixMatch(&(config.mPrefix.mPrefix),
                                 reinterpret_cast<const otIp6Address *>(lowpanContext.mPrefix)) >= config.mPrefix.mLength)
            {
                aRloc16 = config.mRloc16;
                ExitNow(error = kThreadError_None);
            }
        }
    }

exit:
    return error;
}
#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT

bool LeaderBase::IsOnMesh(const Ip6::Address &aAddress)
{
    PrefixTlv *prefix;
    bool rval = false;

    if (memcmp(aAddress.mFields.m8, mNetif.GetMle().GetMeshLocalPrefix(), 8) == 0)
    {
        ExitNow(rval = true);
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
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

ThreadError LeaderBase::RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination,
                                    uint8_t *aPrefixMatch, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    PrefixTlv *prefix;

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aSource.mFields.m8, prefix->GetPrefixLength()) >= 0)
        {
            if (ExternalRouteLookup(prefix->GetDomainId(), aDestination, aPrefixMatch, aRloc16) == kThreadError_None)
            {
                ExitNow(error = kThreadError_None);
            }

            if (DefaultRouteLookup(*prefix, aRloc16) == kThreadError_None)
            {
                if (aPrefixMatch)
                {
                    *aPrefixMatch = 0;
                }

                ExitNow(error = kThreadError_None);
            }
        }
    }

exit:
    return error;
}

ThreadError LeaderBase::ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination,
                                            uint8_t *aPrefixMatch, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    PrefixTlv *prefix;
    HasRouteTlv *hasRoute;
    HasRouteEntry *entry;
    HasRouteEntry *rvalRoute = NULL;
    uint8_t rval_plen = 0;
    int8_t plen;
    NetworkDataTlv *cur;
    NetworkDataTlv *subCur;

    for (cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
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

                    if (rvalRoute == NULL ||
                        entry->GetPreference() > rvalRoute->GetPreference() ||
                        (entry->GetPreference() == rvalRoute->GetPreference() &&
                         mNetif.GetMle().GetCost(entry->GetRloc()) < mNetif.GetMle().GetCost(rvalRoute->GetRloc())))
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

        error = kThreadError_None;
    }

    return error;
}

ThreadError LeaderBase::DefaultRouteLookup(PrefixTlv &aPrefix, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    BorderRouterTlv *borderRouter;
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

            if (route == NULL ||
                entry->GetPreference() > route->GetPreference() ||
                (entry->GetPreference() == route->GetPreference() &&
                 (entry->GetRloc() == mNetif.GetMle().GetRloc16() ||
                  (route->GetRloc() != mNetif.GetMle().GetRloc16() &&
                   mNetif.GetMle().GetCost(entry->GetRloc()) < mNetif.GetMle().GetCost(route->GetRloc())))))
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

        error = kThreadError_None;
    }

    return error;
}

void LeaderBase::SetNetworkData(uint8_t aVersion, uint8_t aStableVersion, bool aStable,
                                const uint8_t *aData, uint8_t aDataLength)
{
    mVersion = aVersion;
    mStableVersion = aStableVersion;
    memcpy(mTlvs, aData, aDataLength);
    mLength = aDataLength;

    if (aStable)
    {
        RemoveTemporaryData(mTlvs, mLength);
    }

    otDumpDebgNetData(GetInstance(), "set network data", mTlvs, mLength);

    mNetif.SetStateChangedFlags(OT_THREAD_NETDATA_UPDATED);
}

ThreadError LeaderBase::SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength)
{
    ThreadError error = kThreadError_None;
    uint8_t remaining = kMaxSize - mLength;
    CommissioningDataTlv *commissioningDataTlv;

    VerifyOrExit(sizeof(NetworkDataTlv) + aValueLength < remaining, error = kThreadError_NoBufs);

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
    mNetif.SetStateChangedFlags(OT_THREAD_NETDATA_UPDATED);

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
    MeshCoP::Tlv *rval = NULL;
    NetworkDataTlv *commissioningDataTlv;
    MeshCoP::Tlv *cur;
    MeshCoP::Tlv *end;

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
    bool rval = false;

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

ThreadError LeaderBase::RemoveCommissioningData(void)
{
    ThreadError error = kThreadError_NotFound;

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() == NetworkDataTlv::kTypeCommissioningData)
        {
            Remove(reinterpret_cast<uint8_t *>(cur), sizeof(NetworkDataTlv) + cur->GetLength());
            ExitNow(error = kThreadError_None);
        }
    }

exit:
    return error;
}

}  // namespace NetworkData
}  // namespace ot
