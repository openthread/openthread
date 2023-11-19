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
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "instance/instance.hpp"
#include "mac/mac_types.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetworkData {

RegisterLogModule("NetworkData");

Leader::Leader(Instance &aInstance)
    : MutableNetworkData(aInstance, mTlvBuffer, 0, sizeof(mTlvBuffer))
    , mMaxLength(0)
#if OPENTHREAD_FTD
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    , mIsClone(false)
#endif
    , mWaitingForNetDataSync(false)
    , mContextIds(aInstance)
    , mTimer(aInstance)
#endif
{
    Reset();
}

void Leader::Reset(void)
{
    mVersion       = Random::NonCrypto::GetUint8();
    mStableVersion = Random::NonCrypto::GetUint8();
    SetLength(0);
    SignalNetDataChanged();

#if OPENTHREAD_FTD
    mContextIds.Clear();
#endif
}

Error Leader::GetServiceId(uint32_t           aEnterpriseNumber,
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

Error Leader::GetPreferredNat64Prefix(ExternalRouteConfig &aConfig) const
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

const PrefixTlv *Leader::FindNextMatchingPrefixTlv(const Ip6::Address &aAddress, const PrefixTlv *aPrevTlv) const
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

Error Leader::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext) const
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

Error Leader::GetContext(uint8_t aContextId, Lowpan::Context &aContext) const
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

void Leader::GetContextForMeshLocalPrefix(Lowpan::Context &aContext) const
{
    aContext.mPrefix.Set(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    aContext.mContextId    = Mle::kMeshLocalPrefixContextId;
    aContext.mCompressFlag = true;
    aContext.mIsValid      = true;
}

bool Leader::IsOnMesh(const Ip6::Address &aAddress) const
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

Error Leader::RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint16_t &aRloc16) const
{
    Error            error     = kErrorNoRoute;
    const PrefixTlv *prefixTlv = nullptr;

    while ((prefixTlv = FindNextMatchingPrefixTlv(aSource, prefixTlv)) != nullptr)
    {
        if (prefixTlv->FindSubTlv<BorderRouterTlv>() == nullptr)
        {
            continue;
        }

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

template <typename EntryType> int Leader::CompareRouteEntries(const EntryType &aFirst, const EntryType &aSecond) const
{
    // `EntryType` can be `HasRouteEntry` or `BorderRouterEntry`.

    return CompareRouteEntries(aFirst.GetPreference(), aFirst.GetRloc(), aSecond.GetPreference(), aSecond.GetRloc());
}

int Leader::CompareRouteEntries(int8_t   aFirstPreference,
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
    VerifyOrExit(result == 0);

    // If all the same, prefer the BR acting as a router over an
    // end device.
    result = ThreeWayCompare(Mle::IsActiveRouter(aFirstRloc), Mle::IsActiveRouter(aSecondRloc));
#endif

exit:
    return result;
}

Error Leader::ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination, uint16_t &aRloc16) const
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

Error Leader::DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t &aRloc16) const
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

Error Leader::SetNetworkData(uint8_t        aVersion,
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

    SignalNetDataChanged();

exit:
    return error;
}

const CommissioningDataTlv *Leader::FindCommissioningData(void) const
{
    return NetworkDataTlv::Find<CommissioningDataTlv>(GetTlvsStart(), GetTlvsEnd());
}

const MeshCoP::Tlv *Leader::FindCommissioningDataSubTlv(uint8_t aType) const
{
    const MeshCoP::Tlv   *subTlv  = nullptr;
    const NetworkDataTlv *dataTlv = FindCommissioningData();

    VerifyOrExit(dataTlv != nullptr);
    subTlv = As<MeshCoP::Tlv>(Tlv::FindTlv(dataTlv->GetValue(), dataTlv->GetLength(), aType));

exit:
    return subTlv;
}

Error Leader::ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::Type aType, uint16_t &aValue) const
{
    Error               error  = kErrorNone;
    const MeshCoP::Tlv *subTlv = FindCommissioningDataSubTlv(aType);

    VerifyOrExit(subTlv != nullptr, error = kErrorNotFound);
    VerifyOrExit(subTlv->GetLength() >= sizeof(uint16_t), error = kErrorParse);
    aValue = BigEndian::ReadUint16(subTlv->GetValue());

exit:
    return error;
}

void Leader::GetCommissioningDataset(MeshCoP::CommissioningDataset &aDataset) const
{
    const CommissioningDataTlv *dataTlv = FindCommissioningData();
    const MeshCoP::Tlv         *subTlv;
    const MeshCoP::Tlv         *endTlv;

    aDataset.Clear();

    VerifyOrExit(dataTlv != nullptr);

    aDataset.mIsLocatorSet       = (FindBorderAgentRloc(aDataset.mLocator) == kErrorNone);
    aDataset.mIsSessionIdSet     = (FindCommissioningSessionId(aDataset.mSessionId) == kErrorNone);
    aDataset.mIsJoinerUdpPortSet = (FindJoinerUdpPort(aDataset.mJoinerUdpPort) == kErrorNone);
    aDataset.mIsSteeringDataSet  = (FindSteeringData(AsCoreType(&aDataset.mSteeringData)) == kErrorNone);

    // Determine if the Commissioning data has any extra unknown TLVs

    subTlv = reinterpret_cast<const MeshCoP::Tlv *>(dataTlv->GetValue());
    endTlv = reinterpret_cast<const MeshCoP::Tlv *>(dataTlv->GetValue() + dataTlv->GetLength());

    for (; subTlv < endTlv; subTlv = subTlv->GetNext())
    {
        switch (subTlv->GetType())
        {
        case MeshCoP::Tlv::kBorderAgentLocator:
        case MeshCoP::Tlv::kSteeringData:
        case MeshCoP::Tlv::kJoinerUdpPort:
        case MeshCoP::Tlv::kCommissionerSessionId:
            break;
        default:
            ExitNow(aDataset.mHasExtraTlv = true);
        }
    }

exit:
    return;
}

Error Leader::FindBorderAgentRloc(uint16_t &aRloc16) const
{
    return ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::kBorderAgentLocator, aRloc16);
}

Error Leader::FindCommissioningSessionId(uint16_t &aSessionId) const
{
    return ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::kCommissionerSessionId, aSessionId);
}

Error Leader::FindJoinerUdpPort(uint16_t &aPort) const
{
    return ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::kJoinerUdpPort, aPort);
}

Error Leader::FindSteeringData(MeshCoP::SteeringData &aSteeringData) const
{
    Error                           error           = kErrorNone;
    const MeshCoP::SteeringDataTlv *steeringDataTlv = FindInCommissioningData<MeshCoP::SteeringDataTlv>();

    VerifyOrExit(steeringDataTlv != nullptr, error = kErrorNotFound);
    steeringDataTlv->CopyTo(aSteeringData);

exit:
    return error;
}

bool Leader::IsJoiningAllowed(void) const
{
    bool                  isAllowed = false;
    MeshCoP::SteeringData steeringData;

    SuccessOrExit(FindSteeringData(steeringData));
    isAllowed = !steeringData.IsEmpty();

exit:
    return isAllowed;
}

Error Leader::SteeringDataCheck(const FilterIndexes &aFilterIndexes) const
{
    Error                 error = kErrorInvalidState;
    MeshCoP::SteeringData steeringData;

    SuccessOrExit(FindSteeringData(steeringData));
    error = steeringData.Contains(aFilterIndexes) ? kErrorNone : kErrorNotFound;

exit:
    return error;
}

Error Leader::SteeringDataCheckJoiner(const Mac::ExtAddress &aEui64) const
{
    FilterIndexes   filterIndexes;
    Mac::ExtAddress joinerId;

    MeshCoP::ComputeJoinerId(aEui64, joinerId);
    MeshCoP::SteeringData::CalculateHashBitIndexes(joinerId, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

Error Leader::SteeringDataCheckJoiner(const MeshCoP::JoinerDiscerner &aDiscerner) const
{
    FilterIndexes filterIndexes;

    MeshCoP::SteeringData::CalculateHashBitIndexes(aDiscerner, filterIndexes);

    return SteeringDataCheck(filterIndexes);
}

void Leader::SignalNetDataChanged(void)
{
    mMaxLength = Max(mMaxLength, GetLength());
    Get<ot::Notifier>().Signal(kEventThreadNetdataChanged);
}

} // namespace NetworkData
} // namespace ot
