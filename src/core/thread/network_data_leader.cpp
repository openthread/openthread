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
#include "common/locator_getters.hpp"
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

RegisterLogModule("NetworkData");

void LeaderBase::Reset(void)
{
    mVersion       = Random::NonCrypto::GetUint8();
    mStableVersion = Random::NonCrypto::GetUint8();
    SetLength(0);
    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);
}

Error LeaderBase::GetServiceId(uint32_t           aEnterpriseNumber,
                               const ServiceData &aServiceData,
                               bool               aServerStable,
                               uint8_t           &aServiceId) const
{
    Error         error    = kErrorNotFound;
    Iterator      iterator = kIteratorInit;
    ServiceConfig serviceConfig;
    ServiceData   serviceData;

    while (GetNextService(iterator, serviceConfig) == kErrorNone)
    {
        serviceConfig.GetServiceData(serviceData);

        if (aEnterpriseNumber == serviceConfig.mEnterpriseNumber && aServiceData == serviceData &&
            aServerStable == serviceConfig.mServerConfig.mStable)
        {
            aServiceId = serviceConfig.mServiceId;
            ExitNow(error = kErrorNone);
        }
    }

exit:
    return error;
}

Error LeaderBase::GetPreferredNat64Prefix(ExternalRouteConfig &aConfig) const
{
    Error               error    = kErrorNotFound;
    Iterator            iterator = kIteratorInit;
    ExternalRouteConfig config;

    while (GetNextExternalRoute(iterator, config) == kErrorNone)
    {
        if (!config.mNat64 || !config.GetPrefix().IsValidNat64())
        {
            continue;
        }

        if ((error == kErrorNotFound) || (config.mPreference > aConfig.mPreference) ||
            (config.mPreference == aConfig.mPreference && config.GetPrefix() < aConfig.GetPrefix()))
        {
            aConfig = config;
            error   = kErrorNone;
        }
    }

    return error;
}

const PrefixTlv *LeaderBase::FindNextMatchingPrefixTlv(const Ip6::Address &aAddress, const PrefixTlv *aPrevTlv) const
{
    // This method iterates over Prefix TLVs which match a given IPv6
    // `aAddress`. If `aPrevTlv` is `nullptr` we start from the
    // beginning. Otherwise, we search for a match after `aPrevTlv`.
    // This method returns a pointer to the next matching Prefix TLV
    // when found, or `nullptr` if no match is found.

    const PrefixTlv *prefixTlv;
    TlvIterator      tlvIterator((aPrevTlv == nullptr) ? GetTlvsStart() : aPrevTlv->GetNext(), GetTlvsEnd());

    while ((prefixTlv = tlvIterator.Iterate<PrefixTlv>()) != nullptr)
    {
        if (aAddress.MatchesPrefix(prefixTlv->GetPrefix(), prefixTlv->GetPrefixLength()))
        {
            break;
        }
    }

    return prefixTlv;
}

Error LeaderBase::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext) const
{
    const PrefixTlv  *prefixTlv = nullptr;
    const ContextTlv *contextTlv;

    aContext.mPrefix.SetLength(0);

    if (Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress))
    {
        GetContextForMeshLocalPrefix(aContext);
    }

    while ((prefixTlv = FindNextMatchingPrefixTlv(aAddress, prefixTlv)) != nullptr)
    {
        contextTlv = prefixTlv->FindSubTlv<ContextTlv>();

        if (contextTlv == nullptr)
        {
            continue;
        }

        if (prefixTlv->GetPrefixLength() > aContext.mPrefix.GetLength())
        {
            prefixTlv->CopyPrefixTo(aContext.mPrefix);
            aContext.mContextId    = contextTlv->GetContextId();
            aContext.mCompressFlag = contextTlv->IsCompress();
            aContext.mIsValid      = true;
        }
    }

    return (aContext.mPrefix.GetLength() > 0) ? kErrorNone : kErrorNotFound;
}

Error LeaderBase::GetContext(uint8_t aContextId, Lowpan::Context &aContext) const
{
    Error            error = kErrorNotFound;
    TlvIterator      tlvIterator(GetTlvsStart(), GetTlvsEnd());
    const PrefixTlv *prefixTlv;

    if (aContextId == Mle::kMeshLocalPrefixContextId)
    {
        GetContextForMeshLocalPrefix(aContext);
        ExitNow(error = kErrorNone);
    }

    while ((prefixTlv = tlvIterator.Iterate<PrefixTlv>()) != nullptr)
    {
        const ContextTlv *contextTlv = prefixTlv->FindSubTlv<ContextTlv>();

        if ((contextTlv == nullptr) || (contextTlv->GetContextId() != aContextId))
        {
            continue;
        }

        prefixTlv->CopyPrefixTo(aContext.mPrefix);
        aContext.mContextId    = contextTlv->GetContextId();
        aContext.mCompressFlag = contextTlv->IsCompress();
        aContext.mIsValid      = true;
        ExitNow(error = kErrorNone);
    }

exit:
    return error;
}

void LeaderBase::GetContextForMeshLocalPrefix(Lowpan::Context &aContext) const
{
    aContext.mPrefix.Set(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    aContext.mContextId    = Mle::kMeshLocalPrefixContextId;
    aContext.mCompressFlag = true;
    aContext.mIsValid      = true;
}

bool LeaderBase::IsOnMesh(const Ip6::Address &aAddress) const
{
    const PrefixTlv *prefixTlv = nullptr;
    bool             isOnMesh  = false;

    VerifyOrExit(!Get<Mle::MleRouter>().IsMeshLocalAddress(aAddress), isOnMesh = true);

    while ((prefixTlv = FindNextMatchingPrefixTlv(aAddress, prefixTlv)) != nullptr)
    {
        TlvIterator            subTlvIterator(*prefixTlv);
        const BorderRouterTlv *brTlv;

        while ((brTlv = subTlvIterator.Iterate<BorderRouterTlv>()) != nullptr)
        {
            for (const BorderRouterEntry *entry = brTlv->GetFirstEntry(); entry <= brTlv->GetLastEntry();
                 entry                          = entry->GetNext())
            {
                if (entry->IsOnMesh())
                {
                    ExitNow(isOnMesh = true);
                }
            }
        }
    }

exit:
    return isOnMesh;
}

Error LeaderBase::RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint16_t &aRloc16) const
{
    Error            error     = kErrorNoRoute;
    const PrefixTlv *prefixTlv = nullptr;

    while ((prefixTlv = FindNextMatchingPrefixTlv(aSource, prefixTlv)) != nullptr)
    {
        if (ExternalRouteLookup(prefixTlv->GetDomainId(), aDestination, aRloc16) == kErrorNone)
        {
            ExitNow(error = kErrorNone);
        }

        if (DefaultRouteLookup(*prefixTlv, aRloc16) == kErrorNone)
        {
            ExitNow(error = kErrorNone);
        }
    }

exit:
    return error;
}

template <typename EntryType>
int LeaderBase::CompareRouteEntries(const EntryType &aFirst, const EntryType &aSecond) const
{
    // `EntryType` can be `HasRouteEntry` or `BorderRouterEntry`.

    return CompareRouteEntries(aFirst.GetPreference(), aFirst.GetRloc(), aSecond.GetPreference(), aSecond.GetRloc());
}

int LeaderBase::CompareRouteEntries(int8_t   aFirstPreference,
                                    uint16_t aFirstRloc,
                                    int8_t   aSecondPreference,
                                    uint16_t aSecondRloc) const
{
    // Performs three-way comparison between two BR entries.

    int result;

    // Prefer the entry with higher preference.

    result = ThreeWayCompare(aFirstPreference, aSecondPreference);
    VerifyOrExit(result == 0);

#if OPENTHREAD_MTD
    // On MTD, prefer the BR that is this device itself. This handles
    // the uncommon case where an MTD itself may be acting as BR.

    result = ThreeWayCompare((aFirstRloc == Get<Mle::Mle>().GetRloc16()), (aSecondRloc == Get<Mle::Mle>().GetRloc16()));
#endif

#if OPENTHREAD_FTD
    // If all the same, prefer the one with lower mesh path cost.
    // Lower cost is preferred so we pass the second entry's cost as
    // the first argument in the call to `ThreeWayCompare()`, i.e.,
    // if the second entry's cost is larger, we return 1 indicating
    // that the first entry is preferred over the second one.

    result = ThreeWayCompare(Get<RouterTable>().GetPathCost(aSecondRloc), Get<RouterTable>().GetPathCost(aFirstRloc));
#endif

exit:
    return result;
}

Error LeaderBase::ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination, uint16_t &aRloc16) const
{
    Error                error           = kErrorNoRoute;
    const PrefixTlv     *prefixTlv       = nullptr;
    const HasRouteEntry *bestRouteEntry  = nullptr;
    uint8_t              bestMatchLength = 0;

    while ((prefixTlv = FindNextMatchingPrefixTlv(aDestination, prefixTlv)) != nullptr)
    {
        const HasRouteTlv *hasRoute;
        uint8_t            prefixLength = prefixTlv->GetPrefixLength();
        TlvIterator        subTlvIterator(*prefixTlv);

        if (prefixTlv->GetDomainId() != aDomainId)
        {
            continue;
        }

        if ((bestRouteEntry != nullptr) && (prefixLength <= bestMatchLength))
        {
            continue;
        }

        while ((hasRoute = subTlvIterator.Iterate<HasRouteTlv>()) != nullptr)
        {
            for (const HasRouteEntry *entry = hasRoute->GetFirstEntry(); entry <= hasRoute->GetLastEntry();
                 entry                      = entry->GetNext())
            {
                if ((bestRouteEntry == nullptr) || (prefixLength > bestMatchLength) ||
                    CompareRouteEntries(*entry, *bestRouteEntry) > 0)
                {
                    bestRouteEntry  = entry;
                    bestMatchLength = prefixLength;
                }
            }
        }
    }

    if (bestRouteEntry != nullptr)
    {
        aRloc16 = bestRouteEntry->GetRloc();
        error   = kErrorNone;
    }

    return error;
}

Error LeaderBase::DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t &aRloc16) const
{
    Error                    error = kErrorNoRoute;
    TlvIterator              subTlvIterator(aPrefix);
    const BorderRouterTlv   *brTlv;
    const BorderRouterEntry *route = nullptr;

    while ((brTlv = subTlvIterator.Iterate<BorderRouterTlv>()) != nullptr)
    {
        for (const BorderRouterEntry *entry = brTlv->GetFirstEntry(); entry <= brTlv->GetLastEntry();
             entry                          = entry->GetNext())
        {
            if (!entry->IsDefaultRoute())
            {
                continue;
            }

            if (route == nullptr || CompareRouteEntries(*entry, *route) > 0)
            {
                route = entry;
            }
        }
    }

    if (route != nullptr)
    {
        aRloc16 = route->GetRloc();
        error   = kErrorNone;
    }

    return error;
}

Error LeaderBase::SetNetworkData(uint8_t        aVersion,
                                 uint8_t        aStableVersion,
                                 Type           aType,
                                 const Message &aMessage,
                                 uint16_t       aOffset,
                                 uint16_t       aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aLength <= kMaxSize, error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aOffset, GetBytes(), aLength));

    SetLength(static_cast<uint8_t>(aLength));
    mVersion       = aVersion;
    mStableVersion = aStableVersion;

    if (aType == kStableSubset)
    {
        RemoveTemporaryData();
    }

#if OPENTHREAD_FTD
    if (Get<Mle::MleRouter>().IsLeader())
    {
        Get<Leader>().HandleNetworkDataRestoredAfterReset();
    }
#endif

    DumpDebg("SetNetworkData", GetBytes(), GetLength());

    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);

exit:
    return error;
}

Error LeaderBase::SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength)
{
    Error                 error = kErrorNone;
    CommissioningDataTlv *commissioningDataTlv;

    RemoveCommissioningData();

    if (aValueLength > 0)
    {
        VerifyOrExit(aValueLength <= kMaxSize - sizeof(CommissioningDataTlv), error = kErrorNoBufs);
        commissioningDataTlv = As<CommissioningDataTlv>(AppendTlv(sizeof(CommissioningDataTlv) + aValueLength));
        VerifyOrExit(commissioningDataTlv != nullptr, error = kErrorNoBufs);

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
    return NetworkDataTlv::Find<CommissioningDataTlv>(GetTlvsStart(), GetTlvsEnd());
}

const MeshCoP::Tlv *LeaderBase::GetCommissioningDataSubTlv(MeshCoP::Tlv::Type aType) const
{
    const MeshCoP::Tlv   *rval = nullptr;
    const NetworkDataTlv *commissioningDataTlv;

    commissioningDataTlv = GetCommissioningData();
    VerifyOrExit(commissioningDataTlv != nullptr);

    rval = MeshCoP::Tlv::FindTlv(commissioningDataTlv->GetValue(), commissioningDataTlv->GetLength(), aType);

exit:
    return rval;
}

bool LeaderBase::IsJoiningEnabled(void) const
{
    const MeshCoP::Tlv *steeringData;
    bool                rval = false;

    VerifyOrExit(GetCommissioningDataSubTlv(MeshCoP::Tlv::kBorderAgentLocator) != nullptr);

    steeringData = GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);
    VerifyOrExit(steeringData != nullptr);

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

    VerifyOrExit(tlv != nullptr);
    RemoveTlv(tlv);

exit:
    return;
}

Error LeaderBase::SteeringDataCheck(const FilterIndexes &aFilterIndexes) const
{
    Error                 error = kErrorNone;
    const MeshCoP::Tlv   *steeringDataTlv;
    MeshCoP::SteeringData steeringData;

    steeringDataTlv = GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);
    VerifyOrExit(steeringDataTlv != nullptr, error = kErrorInvalidState);

    As<MeshCoP::SteeringDataTlv>(steeringDataTlv)->CopyTo(steeringData);

    VerifyOrExit(steeringData.Contains(aFilterIndexes), error = kErrorNotFound);

exit:
    return error;
}

Error LeaderBase::SteeringDataCheckJoiner(const Mac::ExtAddress &aEui64) const
{
    FilterIndexes   filterIndexes;
    Mac::ExtAddress joinerId;

    MeshCoP::ComputeJoinerId(aEui64, joinerId);
    MeshCoP::SteeringData::CalculateHashBitIndexes(joinerId, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

Error LeaderBase::SteeringDataCheckJoiner(const MeshCoP::JoinerDiscerner &aDiscerner) const
{
    FilterIndexes filterIndexes;

    MeshCoP::SteeringData::CalculateHashBitIndexes(aDiscerner, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

} // namespace NetworkData
} // namespace ot
