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

#include "network_data_leader.hpp"

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetworkData {

LeaderBase::LeaderBase(Instance &aInstance)
    : NetworkData(aInstance, kTypeLeader)
{
    Reset();
}

void LeaderBase::Reset(void)
{
    mVersion       = Random::NonCrypto::GetUint8();
    mStableVersion = Random::NonCrypto::GetUint8();
    mLength        = 0;
    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);
}

otError LeaderBase::GetServiceId(uint32_t       aEnterpriseNumber,
                                 const uint8_t *aServiceData,
                                 uint8_t        aServiceDataLength,
                                 bool           aServerStable,
                                 uint8_t &      aServiceId) const
{
    otError       error    = OT_ERROR_NOT_FOUND;
    Iterator      iterator = kIteratorInit;
    ServiceConfig serviceConfig;

    while (GetNextService(iterator, serviceConfig) == OT_ERROR_NONE)
    {
        if (aEnterpriseNumber == serviceConfig.mEnterpriseNumber &&
            aServiceDataLength == serviceConfig.mServiceDataLength &&
            memcmp(aServiceData, serviceConfig.mServiceData, aServiceDataLength) == 0 &&
            aServerStable == serviceConfig.mServerConfig.mStable)
        {
            aServiceId = serviceConfig.mServiceId;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
otError LeaderBase::GetBackboneRouterPrimary(BackboneRouter::BackboneRouterConfig &aConfig) const
{
    otError                         error          = OT_ERROR_NOT_FOUND;
    uint8_t                         serviceData    = ServiceTlv::kServiceDataBackboneRouter;
    const ServerTlv *               rvalServerTlv  = nullptr;
    const BackboneRouterServerData *rvalServerData = nullptr;
    const ServiceTlv *              serviceTlv;
    const ServerTlv *               serverTlv;

    serviceTlv = Get<Leader>().FindService(ServiceTlv::kThreadEnterpriseNumber, &serviceData, sizeof(serviceData));

    VerifyOrExit(serviceTlv != nullptr, aConfig.mServer16 = Mac::kShortAddrInvalid);

    for (const NetworkDataTlv *start                                                      = serviceTlv->GetSubTlvs();
         (serverTlv = FindTlv<ServerTlv>(start, serviceTlv->GetNext())) != nullptr; start = serverTlv->GetNext())
    {
        const BackboneRouterServerData *serverData =
            reinterpret_cast<const BackboneRouterServerData *>(serverTlv->GetServerData());

        if (rvalServerTlv == nullptr ||
            (serverTlv->GetServer16() == Mle::Mle::Rloc16FromRouterId(Get<Mle::MleRouter>().GetLeaderId())) ||
            serverData->GetSequenceNumber() > rvalServerData->GetSequenceNumber() ||
            (serverData->GetSequenceNumber() == rvalServerData->GetSequenceNumber() &&
             serverTlv->GetServer16() > rvalServerTlv->GetServer16()))
        {
            rvalServerTlv  = serverTlv;
            rvalServerData = serverData;
        }
    }

    VerifyOrExit(rvalServerTlv != nullptr, OT_NOOP);

    aConfig.mServer16            = rvalServerTlv->GetServer16();
    aConfig.mSequenceNumber      = rvalServerData->GetSequenceNumber();
    aConfig.mReregistrationDelay = rvalServerData->GetReregistrationDelay();
    aConfig.mMlrTimeout          = rvalServerData->GetMlrTimeout();

    error = OT_ERROR_NONE;

exit:
    return error;
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

const PrefixTlv *LeaderBase::FindNextMatchingPrefix(const Ip6::Address &aAddress, const PrefixTlv *aPrevTlv) const
{
    const PrefixTlv *prefixTlv;

    for (const NetworkDataTlv *start = (aPrevTlv == nullptr) ? GetTlvsStart() : aPrevTlv->GetNext();
         (prefixTlv = FindTlv<PrefixTlv>(start, GetTlvsEnd())) != nullptr; start = prefixTlv->GetNext())
    {
        if (aAddress.MatchesPrefix(prefixTlv->GetPrefix(), prefixTlv->GetPrefixLength()))
        {
            ExitNow();
        }
    }

exit:
    return prefixTlv;
}

otError LeaderBase::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext) const
{
    const PrefixTlv * prefix = nullptr;
    const ContextTlv *contextTlv;

    aContext.mPrefix.SetLength(0);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        aContext.mPrefix.Set(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        aContext.mContextId    = Mle::kMeshLocalPrefixContextId;
        aContext.mCompressFlag = true;
    }

    while ((prefix = FindNextMatchingPrefix(aAddress, prefix)) != nullptr)
    {
        contextTlv = FindContext(*prefix);

        if (contextTlv == nullptr)
        {
            continue;
        }

        if (prefix->GetPrefixLength() > aContext.mPrefix.GetLength())
        {
            aContext.mPrefix.Set(prefix->GetPrefix(), prefix->GetPrefixLength());
            aContext.mContextId    = contextTlv->GetContextId();
            aContext.mCompressFlag = contextTlv->IsCompress();
        }
    }

    return (aContext.mPrefix.GetLength() > 0) ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

otError LeaderBase::GetContext(uint8_t aContextId, Lowpan::Context &aContext) const
{
    otError          error = OT_ERROR_NOT_FOUND;
    const PrefixTlv *prefix;

    if (aContextId == Mle::kMeshLocalPrefixContextId)
    {
        aContext.mPrefix.Set(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        aContext.mContextId    = Mle::kMeshLocalPrefixContextId;
        aContext.mCompressFlag = true;
        ExitNow(error = OT_ERROR_NONE);
    }

    for (const NetworkDataTlv *start = GetTlvsStart(); (prefix = FindTlv<PrefixTlv>(start, GetTlvsEnd())) != nullptr;
         start                       = prefix->GetNext())
    {
        const ContextTlv *contextTlv = FindContext(*prefix);

        if ((contextTlv == nullptr) || (contextTlv->GetContextId() != aContextId))
        {
            continue;
        }

        aContext.mPrefix.Set(prefix->GetPrefix(), prefix->GetPrefixLength());
        aContext.mContextId    = contextTlv->GetContextId();
        aContext.mCompressFlag = contextTlv->IsCompress();
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError LeaderBase::GetRlocByContextId(uint8_t aContextId, uint16_t &aRloc16) const
{
    otError         error = OT_ERROR_NOT_FOUND;
    Lowpan::Context lowpanContext;

    if ((GetContext(aContextId, lowpanContext)) == OT_ERROR_NONE)
    {
        Iterator           iterator = kIteratorInit;
        OnMeshPrefixConfig config;

        while (GetNextOnMeshPrefix(iterator, config) == OT_ERROR_NONE)
        {
            if (lowpanContext.mPrefix.ContainsPrefix(config.GetPrefix()))
            {
                aRloc16 = config.mRloc16;
                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}

bool LeaderBase::IsOnMesh(const Ip6::Address &aAddress) const
{
    const PrefixTlv *prefix = nullptr;
    bool             rval   = false;

    VerifyOrExit(!Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress), rval = true);

    while ((prefix = FindNextMatchingPrefix(aAddress, prefix)) != nullptr)
    {
        // check both stable and temporary Border Router TLVs
        for (int i = 0; i < 2; i++)
        {
            const BorderRouterTlv *borderRouter = FindBorderRouter(*prefix, /* aStable */ (i == 0));

            if (borderRouter == nullptr)
            {
                continue;
            }

            for (const BorderRouterEntry *entry = borderRouter->GetFirstEntry(); entry <= borderRouter->GetLastEntry();
                 entry                          = entry->GetNext())
            {
                if (entry->IsOnMesh())
                {
                    ExitNow(rval = true);
                }
            }
        }
    }

exit:
    return rval;
}

otError LeaderBase::RouteLookup(const Ip6::Address &aSource,
                                const Ip6::Address &aDestination,
                                uint8_t *           aPrefixMatchLength,
                                uint16_t *          aRloc16) const
{
    otError          error  = OT_ERROR_NO_ROUTE;
    const PrefixTlv *prefix = nullptr;

    while ((prefix = FindNextMatchingPrefix(aSource, prefix)) != nullptr)
    {
        if (ExternalRouteLookup(prefix->GetDomainId(), aDestination, aPrefixMatchLength, aRloc16) == OT_ERROR_NONE)
        {
            ExitNow(error = OT_ERROR_NONE);
        }

        if (DefaultRouteLookup(*prefix, aRloc16) == OT_ERROR_NONE)
        {
            if (aPrefixMatchLength)
            {
                *aPrefixMatchLength = 0;
            }

            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

otError LeaderBase::ExternalRouteLookup(uint8_t             aDomainId,
                                        const Ip6::Address &aDestination,
                                        uint8_t *           aPrefixMatchLength,
                                        uint16_t *          aRloc16) const
{
    otError              error = OT_ERROR_NO_ROUTE;
    const PrefixTlv *    prefixTlv;
    const HasRouteEntry *bestRouteEntry  = nullptr;
    uint8_t              bestMatchLength = 0;

    for (const NetworkDataTlv *start = GetTlvsStart(); (prefixTlv = FindTlv<PrefixTlv>(start, GetTlvsEnd())) != nullptr;
         start                       = prefixTlv->GetNext())
    {
        const HasRouteTlv *hasRoute;
        uint8_t            prefixLength = prefixTlv->GetPrefixLength();

        if (prefixTlv->GetDomainId() != aDomainId)
        {
            continue;
        }

        if (!aDestination.MatchesPrefix(prefixTlv->GetPrefix(), prefixLength))
        {
            continue;
        }

        if (prefixLength <= bestMatchLength)
        {
            continue;
        }

        for (const NetworkDataTlv *subStart = prefixTlv->GetSubTlvs();
             (hasRoute = FindTlv<HasRouteTlv>(subStart, prefixTlv->GetNext())) != nullptr;
             subStart = hasRoute->GetNext())
        {
            for (const HasRouteEntry *entry = hasRoute->GetFirstEntry(); entry <= hasRoute->GetLastEntry();
                 entry                      = entry->GetNext())
            {
                if (bestRouteEntry == nullptr || entry->GetPreference() > bestRouteEntry->GetPreference() ||
                    (entry->GetPreference() == bestRouteEntry->GetPreference() &&
                     (entry->GetRloc() == Get<Mle::MleRouter>().GetRloc16() ||
                      (bestRouteEntry->GetRloc() != Get<Mle::MleRouter>().GetRloc16() &&
                       Get<Mle::MleRouter>().GetCost(entry->GetRloc()) <
                           Get<Mle::MleRouter>().GetCost(bestRouteEntry->GetRloc())))))
                {
                    bestRouteEntry  = entry;
                    bestMatchLength = prefixLength;
                }
            }
        }
    }

    if (bestRouteEntry != nullptr)
    {
        if (aRloc16 != nullptr)
        {
            *aRloc16 = bestRouteEntry->GetRloc();
        }

        if (aPrefixMatchLength != nullptr)
        {
            *aPrefixMatchLength = bestMatchLength;
        }

        error = OT_ERROR_NONE;
    }

    return error;
}

otError LeaderBase::DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t *aRloc16) const
{
    otError                  error = OT_ERROR_NO_ROUTE;
    const BorderRouterTlv *  borderRouter;
    const BorderRouterEntry *route = nullptr;

    for (const NetworkDataTlv *start = aPrefix.GetSubTlvs();
         (borderRouter = FindTlv<BorderRouterTlv>(start, aPrefix.GetNext())) != nullptr;
         start = borderRouter->GetNext())
    {
        for (const BorderRouterEntry *entry = borderRouter->GetFirstEntry(); entry <= borderRouter->GetLastEntry();
             entry                          = entry->GetNext())
        {
            if (!entry->IsDefaultRoute())
            {
                continue;
            }

            if (route == nullptr || entry->GetPreference() > route->GetPreference() ||
                (entry->GetPreference() == route->GetPreference() &&
                 (entry->GetRloc() == Get<Mle::MleRouter>().GetRloc16() ||
                  (route->GetRloc() != Get<Mle::MleRouter>().GetRloc16() &&
                   Get<Mle::MleRouter>().GetCost(entry->GetRloc()) < Get<Mle::MleRouter>().GetCost(route->GetRloc())))))
            {
                route = entry;
            }
        }
    }

    if (route != nullptr)
    {
        if (aRloc16 != nullptr)
        {
            *aRloc16 = route->GetRloc();
        }

        error = OT_ERROR_NONE;
    }

    return error;
}

otError LeaderBase::SetNetworkData(uint8_t        aVersion,
                                   uint8_t        aStableVersion,
                                   bool           aStableOnly,
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

    if (aStableOnly)
    {
        RemoveTemporaryData(mTlvs, mLength);
    }

#if OPENTHREAD_FTD
    // Synchronize internal 6LoWPAN Context ID Set with recently obtained Network Data.
    if (Get<Mle::MleRouter>().IsLeader())
    {
        Get<Leader>().UpdateContextsAfterReset();
    }
#endif

    otDumpDebgNetData("set network data", mTlvs, mLength);

    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);

exit:
    return error;
}

otError LeaderBase::SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength)
{
    otError               error = OT_ERROR_NONE;
    CommissioningDataTlv *commissioningDataTlv;

    RemoveCommissioningData();

    if (aValueLength > 0)
    {
        VerifyOrExit(aValueLength <= kMaxSize - sizeof(CommissioningDataTlv), error = OT_ERROR_NO_BUFS);
        commissioningDataTlv =
            static_cast<CommissioningDataTlv *>(AppendTlv(sizeof(CommissioningDataTlv) + aValueLength));
        VerifyOrExit(commissioningDataTlv != nullptr, error = OT_ERROR_NO_BUFS);

        commissioningDataTlv->Init();
        commissioningDataTlv->SetLength(aValueLength);
        memcpy(commissioningDataTlv->GetValue(), aValue, aValueLength);
    }

    mVersion++;
    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);

exit:
    return error;
}

const CommissioningDataTlv *LeaderBase::GetCommissioningData(void) const
{
    return FindTlv<CommissioningDataTlv>(GetTlvsStart(), GetTlvsEnd());
}

const MeshCoP::Tlv *LeaderBase::GetCommissioningDataSubTlv(MeshCoP::Tlv::Type aType) const
{
    const MeshCoP::Tlv *  rval = nullptr;
    const NetworkDataTlv *commissioningDataTlv;

    commissioningDataTlv = GetCommissioningData();
    VerifyOrExit(commissioningDataTlv != nullptr, OT_NOOP);

    rval = MeshCoP::Tlv::FindTlv(commissioningDataTlv->GetValue(), commissioningDataTlv->GetLength(), aType);

exit:
    return rval;
}

bool LeaderBase::IsJoiningEnabled(void) const
{
    const MeshCoP::Tlv *steeringData;
    bool                rval = false;

    VerifyOrExit(GetCommissioningDataSubTlv(MeshCoP::Tlv::kBorderAgentLocator) != nullptr, OT_NOOP);

    steeringData = GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);
    VerifyOrExit(steeringData != nullptr, OT_NOOP);

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

void LeaderBase::RemoveCommissioningData(void)
{
    CommissioningDataTlv *tlv = GetCommissioningData();

    VerifyOrExit(tlv != nullptr, OT_NOOP);
    RemoveTlv(tlv);

exit:
    return;
}

otError LeaderBase::SteeringDataCheck(const FilterIndexes &aFilterIndexes) const
{
    otError               error = OT_ERROR_NONE;
    const MeshCoP::Tlv *  steeringDataTlv;
    MeshCoP::SteeringData steeringData;

    steeringDataTlv = GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);
    VerifyOrExit(steeringDataTlv != nullptr, error = OT_ERROR_INVALID_STATE);

    static_cast<const MeshCoP::SteeringDataTlv *>(steeringDataTlv)->CopyTo(steeringData);

    VerifyOrExit(steeringData.Contains(aFilterIndexes), error = OT_ERROR_NOT_FOUND);

exit:
    return error;
}

otError LeaderBase::SteeringDataCheckJoiner(const Mac::ExtAddress &aEui64) const
{
    FilterIndexes   filterIndexes;
    Mac::ExtAddress joinerId;

    MeshCoP::ComputeJoinerId(aEui64, joinerId);
    MeshCoP::SteeringData::CalculateHashBitIndexes(joinerId, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

otError LeaderBase::SteeringDataCheckJoiner(const MeshCoP::JoinerDiscerner &aDiscerner) const
{
    FilterIndexes filterIndexes;

    MeshCoP::SteeringData::CalculateHashBitIndexes(aDiscerner, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

} // namespace NetworkData
} // namespace ot
