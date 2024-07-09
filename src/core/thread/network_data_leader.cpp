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

#include "instance/instance.hpp"

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

bool Leader::IsNat64(const Ip6::Address &aAddress) const
{
    bool                isNat64  = false;
    Iterator            iterator = kIteratorInit;
    ExternalRouteConfig config;

    while (GetNextExternalRoute(iterator, config) == kErrorNone)
    {
        if (config.mNat64 && config.GetPrefix().IsValidNat64() && aAddress.MatchesPrefix(config.GetPrefix()))
        {
            isNat64 = true;
            break;
        }
    }

    return isNat64;
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

const PrefixTlv *Leader::FindPrefixTlvForContextId(uint8_t aContextId, const ContextTlv *&aContextTlv) const
{
    TlvIterator      tlvIterator(GetTlvsStart(), GetTlvsEnd());
    const PrefixTlv *prefixTlv;

    while ((prefixTlv = tlvIterator.Iterate<PrefixTlv>()) != nullptr)
    {
        const ContextTlv *contextTlv = prefixTlv->FindSubTlv<ContextTlv>();

        if ((contextTlv != nullptr) && (contextTlv->GetContextId() == aContextId))
        {
            aContextTlv = contextTlv;
            break;
        }
    }

    return prefixTlv;
}

Error Leader::GetContext(uint8_t aContextId, Lowpan::Context &aContext) const
{
    Error             error = kErrorNone;
    TlvIterator       tlvIterator(GetTlvsStart(), GetTlvsEnd());
    const PrefixTlv  *prefixTlv;
    const ContextTlv *contextTlv;

    if (aContextId == Mle::kMeshLocalPrefixContextId)
    {
        GetContextForMeshLocalPrefix(aContext);
        ExitNow();
    }

    prefixTlv = FindPrefixTlvForContextId(aContextId, contextTlv);
    VerifyOrExit(prefixTlv != nullptr, error = kErrorNotFound);

    prefixTlv->CopyPrefixTo(aContext.mPrefix);
    aContext.mContextId    = contextTlv->GetContextId();
    aContext.mCompressFlag = contextTlv->IsCompress();
    aContext.mIsValid      = true;

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

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
    {
        // The `Slaac` module keeps track of the associated Domain IDs
        // for deprecating SLAAC prefixes, even if the related
        // Prefix TLV has already been removed from the Network
        // Data.

        uint8_t domainId;

        if (Get<Utils::Slaac>().FindDomainIdFor(aSource, domainId) == kErrorNone)
        {
            error = ExternalRouteLookup(domainId, aDestination, aRloc16);
        }
    }
#endif

exit:
    return error;
}

int Leader::CompareRouteEntries(const BorderRouterEntry &aFirst, const BorderRouterEntry &aSecond) const
{
    return CompareRouteEntries(aFirst.GetPreference(), aFirst.GetRloc(), aSecond.GetPreference(), aSecond.GetRloc());
}

int Leader::CompareRouteEntries(const HasRouteEntry &aFirst, const HasRouteEntry &aSecond) const
{
    return CompareRouteEntries(aFirst.GetPreference(), aFirst.GetRloc(), aSecond.GetPreference(), aSecond.GetRloc());
}

int Leader::CompareRouteEntries(const ServerTlv &aFirst, const ServerTlv &aSecond) const
{
    return CompareRouteEntries(/* aFirstPreference */ 0, aFirst.GetServer16(), /* aSecondPreference */ 0,
                               aSecond.GetServer16());
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

    result = ThreeWayCompare((Get<Mle::Mle>().HasRloc16(aFirstRloc)), Get<Mle::Mle>().HasRloc16(aSecondRloc));
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
    result = ThreeWayCompare(Mle::IsRouterRloc16(aFirstRloc), Mle::IsRouterRloc16(aSecondRloc));
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

Error Leader::LookupRouteIn(const PrefixTlv &aPrefixTlv, EntryChecker aEntryChecker, uint16_t &aRloc16) const
{
    // Iterates over all `BorderRouterEntry` associated with
    // `aPrefixTlv` which also match `aEntryChecker` and determine the
    // best route. For example, this is used from `DefaultRouteLookup()`
    // to look up best default route.

    Error                    error = kErrorNoRoute;
    TlvIterator              subTlvIterator(aPrefixTlv);
    const BorderRouterTlv   *brTlv;
    const BorderRouterEntry *bestEntry = nullptr;

    while ((brTlv = subTlvIterator.Iterate<BorderRouterTlv>()) != nullptr)
    {
        for (const BorderRouterEntry *entry = brTlv->GetFirstEntry(); entry <= brTlv->GetLastEntry();
             entry                          = entry->GetNext())
        {
            if (!aEntryChecker(*entry))
            {
                continue;
            }

            if ((bestEntry == nullptr) || CompareRouteEntries(*entry, *bestEntry) > 0)
            {
                bestEntry = entry;
            }
        }
    }

    if (bestEntry != nullptr)
    {
        aRloc16 = bestEntry->GetRloc();
        error   = kErrorNone;
    }

    return error;
}

bool Leader::IsEntryDefaultRoute(const BorderRouterEntry &aEntry) { return aEntry.IsDefaultRoute(); }

Error Leader::DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t &aRloc16) const
{
    return LookupRouteIn(aPrefix, IsEntryDefaultRoute, aRloc16);
}

Error Leader::SetNetworkData(uint8_t            aVersion,
                             uint8_t            aStableVersion,
                             Type               aType,
                             const Message     &aMessage,
                             const OffsetRange &aOffsetRange)
{
    Error    error  = kErrorNone;
    uint16_t length = aOffsetRange.GetLength();

    VerifyOrExit(length <= kMaxSize, error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aOffsetRange.GetOffset(), GetBytes(), length));

    SetLength(static_cast<uint8_t>(length));
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

Coap::Message *Leader::ProcessCommissionerGetRequest(const Coap::Message &aMessage) const
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;
    OffsetRange    offsetRange;

    response = Get<Tmf::Agent>().NewPriorityResponseMessage(aMessage);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    if (Tlv::FindTlvValueOffsetRange(aMessage, MeshCoP::Tlv::kGet, offsetRange) == kErrorNone)
    {
        // Append the requested sub-TLV types given in Get TLV.

        while (!offsetRange.IsEmpty())
        {
            uint8_t             type;
            const MeshCoP::Tlv *subTlv;

            IgnoreError(aMessage.Read(offsetRange, type));
            offsetRange.AdvanceOffset(sizeof(type));

            subTlv = FindCommissioningDataSubTlv(type);

            if (subTlv != nullptr)
            {
                SuccessOrExit(error = subTlv->AppendTo(*response));
            }
        }
    }
    else
    {
        // Append all sub-TLVs in the Commissioning Data.

        const CommissioningDataTlv *dataTlv = FindCommissioningData();

        if (dataTlv != nullptr)
        {
            SuccessOrExit(error = response->AppendBytes(dataTlv->GetValue(), dataTlv->GetLength()));
        }
    }

exit:
    FreeAndNullMessageOnError(response, error);
    return response;
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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

bool Leader::ContainsOmrPrefix(const Ip6::Prefix &aPrefix) const
{
    bool                   contains = false;
    const PrefixTlv       *prefixTlv;
    const BorderRouterTlv *brSubTlv;

    VerifyOrExit(BorderRouter::RoutingManager::IsValidOmrPrefix(aPrefix));

    prefixTlv = FindPrefix(aPrefix);
    VerifyOrExit(prefixTlv != nullptr);

    brSubTlv = prefixTlv->FindSubTlv<BorderRouterTlv>(/* aStable */ true);

    VerifyOrExit(brSubTlv != nullptr);

    for (const BorderRouterEntry *entry = brSubTlv->GetFirstEntry(); entry <= brSubTlv->GetLastEntry(); entry++)
    {
        OnMeshPrefixConfig config;

        config.SetFrom(*prefixTlv, *brSubTlv, *entry);

        if (BorderRouter::RoutingManager::IsValidOmrPrefix(config))
        {
            ExitNow(contains = true);
        }
    }

exit:
    return contains;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

} // namespace NetworkData
} // namespace ot
