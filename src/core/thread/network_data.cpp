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
 *   This file implements common methods for manipulating Thread Network Data.
 */

#include "network_data.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace NetworkData {

RegisterLogModule("NetworkData");

//---------------------------------------------------------------------------------------------------------------------
// NetworkData

Error NetworkData::CopyNetworkData(Type aType, uint8_t *aData, uint8_t &aDataLength) const
{
    Error              error;
    MutableNetworkData netDataCopy(GetInstance(), aData, 0, aDataLength);

    SuccessOrExit(error = CopyNetworkData(aType, netDataCopy));
    aDataLength = netDataCopy.GetLength();

exit:
    return error;
}

Error NetworkData::CopyNetworkData(Type aType, MutableNetworkData &aNetworkData) const
{
    Error error = kErrorNone;

    VerifyOrExit(aNetworkData.GetSize() >= mLength, error = kErrorNoBufs);

    memcpy(aNetworkData.GetBytes(), mTlvs, mLength);
    aNetworkData.SetLength(mLength);

    if (aType == kStableSubset)
    {
        aNetworkData.RemoveTemporaryData();
    }

exit:
    return error;
}

Error NetworkData::GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig) const
{
    return GetNextOnMeshPrefix(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

Error NetworkData::GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig) const
{
    Config config;

    config.mOnMeshPrefix  = &aConfig;
    config.mExternalRoute = nullptr;
    config.mService       = nullptr;
    config.mLowpanContext = nullptr;

    return Iterate(aIterator, aRloc16, config);
}

Error NetworkData::GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig) const
{
    return GetNextExternalRoute(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

Error NetworkData::GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig) const
{
    Config config;

    config.mOnMeshPrefix  = nullptr;
    config.mExternalRoute = &aConfig;
    config.mService       = nullptr;
    config.mLowpanContext = nullptr;

    return Iterate(aIterator, aRloc16, config);
}

Error NetworkData::GetNextService(Iterator &aIterator, ServiceConfig &aConfig) const
{
    return GetNextService(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

Error NetworkData::GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig) const
{
    Config config;

    config.mOnMeshPrefix  = nullptr;
    config.mExternalRoute = nullptr;
    config.mService       = &aConfig;
    config.mLowpanContext = nullptr;

    return Iterate(aIterator, aRloc16, config);
}

Error NetworkData::GetNextLowpanContextInfo(Iterator &aIterator, LowpanContextInfo &aContextInfo) const
{
    Config config;

    config.mOnMeshPrefix  = nullptr;
    config.mExternalRoute = nullptr;
    config.mService       = nullptr;
    config.mLowpanContext = &aContextInfo;

    return Iterate(aIterator, Mac::kShortAddrBroadcast, config);
}

Error NetworkData::Iterate(Iterator &aIterator, uint16_t aRloc16, Config &aConfig) const
{
    // Iterate to the next entry in Network Data matching `aRloc16`
    // (can be set to `Mac::kShortAddrBroadcast` to allow any RLOC).
    // The `aIterator` is used to track and save the current position.
    // On input, the non-`nullptr` pointer members in `aConfig` specify
    // the Network Data entry types (`mOnMeshPrefix`, `mExternalRoute`,
    // `mService`) to iterate over. On successful exit, the `aConfig`
    // is updated such that only one member pointer is not `nullptr`
    // indicating the type of entry and the non-`nullptr` config is
    // updated with the entry info.

    Error               error = kErrorNotFound;
    NetworkDataIterator iterator(aIterator);

    for (const NetworkDataTlv *cur;
         cur = iterator.GetTlv(mTlvs), (cur + 1 <= GetTlvsEnd()) && (cur->GetNext() <= GetTlvsEnd());
         iterator.AdvanceTlv(mTlvs))
    {
        const NetworkDataTlv *subTlvs = nullptr;

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            if ((aConfig.mOnMeshPrefix != nullptr) || (aConfig.mExternalRoute != nullptr) ||
                (aConfig.mLowpanContext != nullptr))
            {
                subTlvs = As<PrefixTlv>(cur)->GetSubTlvs();
            }
            break;
        case NetworkDataTlv::kTypeService:
            if (aConfig.mService != nullptr)
            {
                subTlvs = As<ServiceTlv>(cur)->GetSubTlvs();
            }
            break;
        default:
            break;
        }

        if (subTlvs == nullptr)
        {
            continue;
        }

        for (const NetworkDataTlv *subCur; subCur = iterator.GetSubTlv(subTlvs),
                                           (subCur + 1 <= cur->GetNext()) && (subCur->GetNext() <= cur->GetNext());
             iterator.AdvanceSubTlv(subTlvs))
        {
            if (cur->GetType() == NetworkDataTlv::kTypePrefix)
            {
                const PrefixTlv *prefixTlv = As<PrefixTlv>(cur);

                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeBorderRouter:
                {
                    const BorderRouterTlv *borderRouter = As<BorderRouterTlv>(subCur);

                    if (aConfig.mOnMeshPrefix == nullptr)
                    {
                        continue;
                    }

                    for (uint8_t index; (index = iterator.GetAndAdvanceIndex()) < borderRouter->GetNumEntries();)
                    {
                        if (aRloc16 == Mac::kShortAddrBroadcast || borderRouter->GetEntry(index)->GetRloc() == aRloc16)
                        {
                            const BorderRouterEntry *borderRouterEntry = borderRouter->GetEntry(index);

                            aConfig.mExternalRoute = nullptr;
                            aConfig.mService       = nullptr;
                            aConfig.mLowpanContext = nullptr;
                            aConfig.mOnMeshPrefix->SetFrom(*prefixTlv, *borderRouter, *borderRouterEntry);

                            ExitNow(error = kErrorNone);
                        }
                    }

                    break;
                }

                case NetworkDataTlv::kTypeHasRoute:
                {
                    const HasRouteTlv *hasRoute = As<HasRouteTlv>(subCur);

                    if (aConfig.mExternalRoute == nullptr)
                    {
                        continue;
                    }

                    for (uint8_t index; (index = iterator.GetAndAdvanceIndex()) < hasRoute->GetNumEntries();)
                    {
                        if (aRloc16 == Mac::kShortAddrBroadcast || hasRoute->GetEntry(index)->GetRloc() == aRloc16)
                        {
                            const HasRouteEntry *hasRouteEntry = hasRoute->GetEntry(index);

                            aConfig.mOnMeshPrefix  = nullptr;
                            aConfig.mService       = nullptr;
                            aConfig.mLowpanContext = nullptr;
                            aConfig.mExternalRoute->SetFrom(GetInstance(), *prefixTlv, *hasRoute, *hasRouteEntry);

                            ExitNow(error = kErrorNone);
                        }
                    }

                    break;
                }

                case NetworkDataTlv::kTypeContext:
                {
                    const ContextTlv *contextTlv = As<ContextTlv>(subCur);

                    if (aConfig.mLowpanContext == nullptr)
                    {
                        continue;
                    }

                    if (iterator.IsNewEntry())
                    {
                        aConfig.mOnMeshPrefix  = nullptr;
                        aConfig.mExternalRoute = nullptr;
                        aConfig.mService       = nullptr;
                        aConfig.mLowpanContext->SetFrom(*prefixTlv, *contextTlv);

                        iterator.MarkEntryAsNotNew();
                        ExitNow(error = kErrorNone);
                    }

                    break;
                }

                default:
                    break;
                }
            }
            else // cur is `ServiceTLv`
            {
                const ServiceTlv *service = As<ServiceTlv>(cur);

                if (aConfig.mService == nullptr)
                {
                    continue;
                }

                if (subCur->GetType() == NetworkDataTlv::kTypeServer)
                {
                    const ServerTlv *server = As<ServerTlv>(subCur);

                    if (!iterator.IsNewEntry())
                    {
                        continue;
                    }

                    if ((aRloc16 == Mac::kShortAddrBroadcast) || (server->GetServer16() == aRloc16))
                    {
                        aConfig.mOnMeshPrefix  = nullptr;
                        aConfig.mExternalRoute = nullptr;
                        aConfig.mLowpanContext = nullptr;
                        aConfig.mService->SetFrom(*service, *server);

                        iterator.MarkEntryAsNotNew();

                        ExitNow(error = kErrorNone);
                    }
                }
            }
        }
    }

exit:
    return error;
}

bool NetworkData::ContainsOnMeshPrefix(const OnMeshPrefixConfig &aPrefix) const
{
    bool               contains = false;
    Iterator           iterator = kIteratorInit;
    OnMeshPrefixConfig prefix;

    while (GetNextOnMeshPrefix(iterator, aPrefix.mRloc16, prefix) == kErrorNone)
    {
        if (prefix == aPrefix)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool NetworkData::ContainsExternalRoute(const ExternalRouteConfig &aRoute) const
{
    bool                contains = false;
    Iterator            iterator = kIteratorInit;
    ExternalRouteConfig route;

    while (GetNextExternalRoute(iterator, aRoute.mRloc16, route) == kErrorNone)
    {
        if (route == aRoute)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool NetworkData::ContainsService(const ServiceConfig &aService) const
{
    bool          contains = false;
    Iterator      iterator = kIteratorInit;
    ServiceConfig service;

    while (GetNextService(iterator, aService.GetServerConfig().mRloc16, service) == kErrorNone)
    {
        if (service == aService)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool NetworkData::ContainsEntriesFrom(const NetworkData &aCompare, uint16_t aRloc16) const
{
    bool     contains = true;
    Iterator iterator = kIteratorInit;

    while (true)
    {
        Config              config;
        OnMeshPrefixConfig  prefix;
        ExternalRouteConfig route;
        ServiceConfig       service;

        config.mOnMeshPrefix  = &prefix;
        config.mExternalRoute = &route;
        config.mService       = &service;
        config.mLowpanContext = nullptr;

        SuccessOrExit(aCompare.Iterate(iterator, aRloc16, config));

        if (((config.mOnMeshPrefix != nullptr) && !ContainsOnMeshPrefix(*config.mOnMeshPrefix)) ||
            ((config.mExternalRoute != nullptr) && !ContainsExternalRoute(*config.mExternalRoute)) ||
            ((config.mService != nullptr) && !ContainsService(*config.mService)))
        {
            ExitNow(contains = false);
        }
    }

exit:
    return contains;
}

const PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const
{
    TlvIterator      tlvIterator(mTlvs, mLength);
    const PrefixTlv *prefixTlv;

    while ((prefixTlv = tlvIterator.Iterate<PrefixTlv>()) != nullptr)
    {
        if (prefixTlv->IsEqual(aPrefix, aPrefixLength))
        {
            break;
        }
    }

    return prefixTlv;
}

const ServiceTlv *NetworkData::FindService(uint32_t           aEnterpriseNumber,
                                           const ServiceData &aServiceData,
                                           ServiceMatchMode   aServiceMatchMode) const
{
    TlvIterator       tlvIterator(mTlvs, mLength);
    const ServiceTlv *serviceTlv;

    while ((serviceTlv = tlvIterator.Iterate<ServiceTlv>()) != nullptr)
    {
        if (MatchService(*serviceTlv, aEnterpriseNumber, aServiceData, aServiceMatchMode))
        {
            break;
        }
    }

    return serviceTlv;
}

const ServiceTlv *NetworkData::FindNextService(const ServiceTlv  *aPrevServiceTlv,
                                               uint32_t           aEnterpriseNumber,
                                               const ServiceData &aServiceData,
                                               ServiceMatchMode   aServiceMatchMode) const
{
    const uint8_t *tlvs;
    uint8_t        length;

    if (aPrevServiceTlv == nullptr)
    {
        tlvs   = mTlvs;
        length = mLength;
    }
    else
    {
        tlvs   = reinterpret_cast<const uint8_t *>(aPrevServiceTlv->GetNext());
        length = static_cast<uint8_t>((mTlvs + mLength) - tlvs);
    }

    return NetworkData(GetInstance(), tlvs, length).FindService(aEnterpriseNumber, aServiceData, aServiceMatchMode);
}

const ServiceTlv *NetworkData::FindNextThreadService(const ServiceTlv  *aPrevServiceTlv,
                                                     const ServiceData &aServiceData,
                                                     ServiceMatchMode   aServiceMatchMode) const
{
    return FindNextService(aPrevServiceTlv, ServiceTlv::kThreadEnterpriseNumber, aServiceData, aServiceMatchMode);
}

bool NetworkData::MatchService(const ServiceTlv  &aServiceTlv,
                               uint32_t           aEnterpriseNumber,
                               const ServiceData &aServiceData,
                               ServiceMatchMode   aServiceMatchMode)
{
    bool        match = false;
    ServiceData serviceData;

    VerifyOrExit(aServiceTlv.GetEnterpriseNumber() == aEnterpriseNumber);

    aServiceTlv.GetServiceData(serviceData);

    switch (aServiceMatchMode)
    {
    case kServiceExactMatch:
        match = (serviceData == aServiceData);
        break;

    case kServicePrefixMatch:
        match = serviceData.StartsWith(aServiceData);
        break;
    }

exit:
    return match;
}

void NetworkData::FindRlocs(BorderRouterFilter aBrFilter, RoleFilter aRoleFilter, Rlocs &aRlocs) const
{
    Iterator            iterator = kIteratorInit;
    OnMeshPrefixConfig  prefix;
    ExternalRouteConfig route;
    ServiceConfig       service;
    Config              config;

    aRlocs.Clear();

    while (true)
    {
        config.mOnMeshPrefix  = &prefix;
        config.mExternalRoute = &route;
        config.mService       = &service;
        config.mLowpanContext = nullptr;

        SuccessOrExit(Iterate(iterator, Mac::kShortAddrBroadcast, config));

        if (config.mOnMeshPrefix != nullptr)
        {
            bool matches = true;

            switch (aBrFilter)
            {
            case kAnyBrOrServer:
                break;
            case kBrProvidingExternalIpConn:
                matches = prefix.mOnMesh && (prefix.mDefaultRoute || prefix.mDp);
                break;
            }

            if (matches)
            {
                AddRloc16ToRlocs(prefix.mRloc16, aRlocs, aRoleFilter);
            }
        }
        else if (config.mExternalRoute != nullptr)
        {
            AddRloc16ToRlocs(route.mRloc16, aRlocs, aRoleFilter);
        }
        else if (config.mService != nullptr)
        {
            switch (aBrFilter)
            {
            case kAnyBrOrServer:
                AddRloc16ToRlocs(service.mServerConfig.mRloc16, aRlocs, aRoleFilter);
                break;
            case kBrProvidingExternalIpConn:
                break;
            }
        }
    }

exit:
    return;
}

uint8_t NetworkData::CountBorderRouters(RoleFilter aRoleFilter) const
{
    Rlocs rlocs;

    FindRlocs(kBrProvidingExternalIpConn, aRoleFilter, rlocs);

    return rlocs.GetLength();
}

bool NetworkData::ContainsBorderRouterWithRloc(uint16_t aRloc16) const
{
    Rlocs rlocs;

    FindRlocs(kBrProvidingExternalIpConn, kAnyRole, rlocs);

    return rlocs.Contains(aRloc16);
}

void NetworkData::AddRloc16ToRlocs(uint16_t aRloc16, Rlocs &aRlocs, RoleFilter aRoleFilter)
{
    switch (aRoleFilter)
    {
    case kAnyRole:
        break;

    case kRouterRoleOnly:
        VerifyOrExit(Mle::IsRouterRloc16(aRloc16));
        break;

    case kChildRoleOnly:
        VerifyOrExit(Mle::IsChildRloc16(aRloc16));
        break;
    }

    VerifyOrExit(!aRlocs.Contains(aRloc16));
    IgnoreError(aRlocs.PushBack(aRloc16));

exit:
    return;
}

Error NetworkData::FindDomainIdFor(const Ip6::Prefix &aPrefix, uint8_t &aDomainId) const
{
    Error            error     = kErrorNone;
    const PrefixTlv *prefixTlv = FindPrefix(aPrefix);

    VerifyOrExit(prefixTlv != nullptr, error = kErrorNotFound);
    aDomainId = prefixTlv->GetDomainId();

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MutableNetworkData

void MutableNetworkData::RemoveTemporaryData(void)
{
    NetworkDataTlv *cur = GetTlvsStart();

    while (cur < GetTlvsEnd())
    {
        bool shouldRemove = false;

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            shouldRemove = RemoveTemporaryDataIn(*As<PrefixTlv>(cur));
            break;

        case NetworkDataTlv::kTypeService:
            shouldRemove = RemoveTemporaryDataIn(*As<ServiceTlv>(cur));
            break;

        default:
            shouldRemove = !cur->IsStable();
            break;
        }

        if (shouldRemove)
        {
            RemoveTlv(cur);
            continue;
        }

        cur = cur->GetNext();
    }
}

bool MutableNetworkData::RemoveTemporaryDataIn(PrefixTlv &aPrefix)
{
    NetworkDataTlv *cur = aPrefix.GetSubTlvs();

    while (cur < aPrefix.GetNext())
    {
        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeBorderRouter:
            {
                BorderRouterTlv *borderRouter = As<BorderRouterTlv>(cur);
                ContextTlv      *context      = aPrefix.FindSubTlv<ContextTlv>();

                // Replace p_border_router_16
                for (BorderRouterEntry *entry = borderRouter->GetFirstEntry(); entry <= borderRouter->GetLastEntry();
                     entry                    = entry->GetNext())
                {
                    if ((entry->IsDhcp() || entry->IsConfigure()) && (context != nullptr))
                    {
                        entry->SetRloc(0xfc00 | context->GetContextId());
                    }
                    else
                    {
                        entry->SetRloc(0xfffe);
                    }
                }

                break;
            }

            case NetworkDataTlv::kTypeHasRoute:
            {
                HasRouteTlv *hasRoute = As<HasRouteTlv>(cur);

                // Replace r_border_router_16
                for (HasRouteEntry *entry = hasRoute->GetFirstEntry(); entry <= hasRoute->GetLastEntry();
                     entry                = entry->GetNext())
                {
                    entry->SetRloc(0xfffe);
                }

                break;
            }

            default:
                break;
            }

            // keep stable tlv
            cur = cur->GetNext();
        }
        else
        {
            // remove temporary tlv
            uint8_t subTlvSize = cur->GetSize();
            RemoveTlv(cur);
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - subTlvSize);
        }
    }

    return (aPrefix.GetSubTlvsLength() == 0);
}

bool MutableNetworkData::RemoveTemporaryDataIn(ServiceTlv &aService)
{
    NetworkDataTlv *cur = aService.GetSubTlvs();

    while (cur < aService.GetNext())
    {
        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeServer:
                As<ServerTlv>(cur)->SetServer16(Mle::ServiceAlocFromId(aService.GetServiceId()));
                break;

            default:
                break;
            }

            // keep stable tlv
            cur = cur->GetNext();
        }
        else
        {
            // remove temporary tlv
            uint8_t subTlvSize = cur->GetSize();
            RemoveTlv(cur);
            aService.SetSubTlvsLength(aService.GetSubTlvsLength() - subTlvSize);
        }
    }

    return (aService.GetSubTlvsLength() == 0);
}

NetworkDataTlv *MutableNetworkData::AppendTlv(uint16_t aTlvSize)
{
    NetworkDataTlv *tlv;

    VerifyOrExit(CanInsert(aTlvSize), tlv = nullptr);

    tlv = GetTlvsEnd();
    mLength += static_cast<uint8_t>(aTlvSize);

exit:
    return tlv;
}

void MutableNetworkData::Insert(void *aStart, uint8_t aLength)
{
    uint8_t *start = reinterpret_cast<uint8_t *>(aStart);

    OT_ASSERT(CanInsert(aLength) && mTlvs <= start && start <= mTlvs + mLength);
    memmove(start + aLength, start, mLength - static_cast<size_t>(start - mTlvs));
    mLength += aLength;
}

void MutableNetworkData::Remove(void *aRemoveStart, uint8_t aRemoveLength)
{
    uint8_t *end         = GetBytes() + mLength;
    uint8_t *removeStart = reinterpret_cast<uint8_t *>(aRemoveStart);
    uint8_t *removeEnd   = removeStart + aRemoveLength;

    OT_ASSERT((aRemoveLength <= mLength) && (GetBytes() <= removeStart) && (removeEnd <= end));

    memmove(removeStart, removeEnd, static_cast<uint8_t>(end - removeEnd));
    mLength -= aRemoveLength;
}

void MutableNetworkData::RemoveTlv(NetworkDataTlv *aTlv) { Remove(aTlv, aTlv->GetSize()); }

} // namespace NetworkData
} // namespace ot
