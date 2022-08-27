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

#include "coap/coap_message.hpp"
#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "mac/mac_types.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetworkData {

RegisterLogModule("NetworkData");

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

    return Iterate(aIterator, aRloc16, config);
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
            if ((aConfig.mOnMeshPrefix != nullptr) || (aConfig.mExternalRoute != nullptr))
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
             iterator.AdvaceSubTlv(subTlvs))
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

                            aConfig.mOnMeshPrefix = nullptr;
                            aConfig.mService      = nullptr;
                            aConfig.mExternalRoute->SetFrom(GetInstance(), *prefixTlv, *hasRoute, *hasRouteEntry);

                            ExitNow(error = kErrorNone);
                        }
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

void MutableNetworkData::RemoveTemporaryData(void)
{
    NetworkDataTlv *cur = GetTlvsStart();

    while (cur < GetTlvsEnd())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            PrefixTlv *prefix = As<PrefixTlv>(cur);

            RemoveTemporaryDataIn(*prefix);

            if (prefix->GetSubTlvsLength() == 0)
            {
                RemoveTlv(cur);
                continue;
            }

            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            ServiceTlv *service = As<ServiceTlv>(cur);

            RemoveTemporaryDataIn(*service);

            if (service->GetSubTlvsLength() == 0)
            {
                RemoveTlv(cur);
                continue;
            }

            break;
        }

        default:
            // remove temporary tlv
            if (!cur->IsStable())
            {
                RemoveTlv(cur);
                continue;
            }

            break;
        }

        cur = cur->GetNext();
    }
}

void MutableNetworkData::RemoveTemporaryDataIn(PrefixTlv &aPrefix)
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
                ContextTlv *     context      = aPrefix.FindSubTlv<ContextTlv>();

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
}

void MutableNetworkData::RemoveTemporaryDataIn(ServiceTlv &aService)
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

const ServiceTlv *NetworkData::FindNextService(const ServiceTlv * aPrevServiceTlv,
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

const ServiceTlv *NetworkData::FindNextThreadService(const ServiceTlv * aPrevServiceTlv,
                                                     const ServiceData &aServiceData,
                                                     ServiceMatchMode   aServiceMatchMode) const
{
    return FindNextService(aPrevServiceTlv, ServiceTlv::kThreadEnterpriseNumber, aServiceData, aServiceMatchMode);
}

bool NetworkData::MatchService(const ServiceTlv & aServiceTlv,
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

void MutableNetworkData::RemoveTlv(NetworkDataTlv *aTlv)
{
    Remove(aTlv, aTlv->GetSize());
}

Error NetworkData::SendServerDataNotification(uint16_t              aRloc16,
                                              bool                  aAppendNetDataTlv,
                                              Coap::ResponseHandler aHandler,
                                              void *                aContext) const
{
    Error            error = kErrorNone;
    Coap::Message *  message;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kServerData);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aAppendNetDataTlv)
    {
        ThreadTlv tlv;
        tlv.SetType(ThreadTlv::kThreadNetworkData);
        tlv.SetLength(mLength);
        SuccessOrExit(error = message->Append(tlv));
        SuccessOrExit(error = message->AppendBytes(mTlvs, mLength));
    }

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, aRloc16));
    }

    IgnoreError(messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, aHandler, aContext));

    LogInfo("Sent server data notification");

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error NetworkData::GetNextServer(Iterator &aIterator, uint16_t &aRloc16) const
{
    Error               error;
    OnMeshPrefixConfig  prefixConfig;
    ExternalRouteConfig routeConfig;
    ServiceConfig       serviceConfig;
    Config              config;

    config.mOnMeshPrefix  = &prefixConfig;
    config.mExternalRoute = &routeConfig;
    config.mService       = &serviceConfig;

    SuccessOrExit(error = Iterate(aIterator, Mac::kShortAddrBroadcast, config));

    if (config.mOnMeshPrefix != nullptr)
    {
        aRloc16 = config.mOnMeshPrefix->mRloc16;
    }
    else if (config.mExternalRoute != nullptr)
    {
        aRloc16 = config.mExternalRoute->mRloc16;
    }
    else if (config.mService != nullptr)
    {
        aRloc16 = config.mService->mServerConfig.mRloc16;
    }
    else
    {
        OT_ASSERT(false);
    }

exit:
    return error;
}

Error NetworkData::FindBorderRouters(RoleFilter aRoleFilter, uint16_t aRlocs[], uint8_t &aRlocsLength) const
{
    class Rlocs // Wrapper over an array of RLOC16s.
    {
    public:
        Rlocs(RoleFilter aRoleFilter, uint16_t *aRlocs, uint8_t aRlocsMaxLength)
            : mRoleFilter(aRoleFilter)
            , mRlocs(aRlocs)
            , mLength(0)
            , mMaxLength(aRlocsMaxLength)
        {
        }

        uint8_t GetLength(void) const { return mLength; }

        Error AddRloc16(uint16_t aRloc16)
        {
            // Add `aRloc16` into the array if it matches `RoleFilter` and
            // it is not in the array already. If we need to add the `aRloc16`
            // but there is no more room in the array, return `kErrorNoBufs`.

            Error   error = kErrorNone;
            uint8_t index;

            switch (mRoleFilter)
            {
            case kAnyRole:
                break;

            case kRouterRoleOnly:
                VerifyOrExit(Mle::IsActiveRouter(aRloc16));
                break;

            case kChildRoleOnly:
                VerifyOrExit(!Mle::IsActiveRouter(aRloc16));
                break;
            }

            for (index = 0; index < mLength; index++)
            {
                if (mRlocs[index] == aRloc16)
                {
                    break;
                }
            }

            if (index == mLength)
            {
                VerifyOrExit(mLength < mMaxLength, error = kErrorNoBufs);
                mRlocs[mLength++] = aRloc16;
            }

        exit:
            return error;
        }

    private:
        RoleFilter mRoleFilter;
        uint16_t * mRlocs;
        uint8_t    mLength;
        uint8_t    mMaxLength;
    };

    Error               error = kErrorNone;
    Rlocs               rlocs(aRoleFilter, aRlocs, aRlocsLength);
    Iterator            iterator = kIteratorInit;
    ExternalRouteConfig route;
    OnMeshPrefixConfig  prefix;

    while (GetNextExternalRoute(iterator, route) == kErrorNone)
    {
        SuccessOrExit(error = rlocs.AddRloc16(route.mRloc16));
    }

    iterator = kIteratorInit;

    while (GetNextOnMeshPrefix(iterator, prefix) == kErrorNone)
    {
        if (!prefix.mDefaultRoute || !prefix.mOnMesh)
        {
            continue;
        }

        SuccessOrExit(error = rlocs.AddRloc16(prefix.mRloc16));
    }

exit:
    aRlocsLength = rlocs.GetLength();
    return error;
}

uint8_t NetworkData::CountBorderRouters(RoleFilter aRoleFilter) const
{
    // We use an over-estimate of max number of border routers in the
    // Network Data using the facts that network data is limited to 254
    // bytes and that an external route entry uses at minimum 3 bytes
    // for RLOC16 and flag, so `ceil(254/3) = 85`.

    static constexpr uint16_t kMaxRlocs = 85;

    uint16_t rlocs[kMaxRlocs];
    uint8_t  rlocsLength = kMaxRlocs;

    SuccessOrAssert(FindBorderRouters(aRoleFilter, rlocs, rlocsLength));

    return rlocsLength;
}

bool NetworkData::ContainsBorderRouterWithRloc(uint16_t aRloc16) const
{
    bool                contains = false;
    Iterator            iterator = kIteratorInit;
    ExternalRouteConfig route;
    OnMeshPrefixConfig  prefix;

    while (GetNextExternalRoute(iterator, route) == kErrorNone)
    {
        if (route.mRloc16 == aRloc16)
        {
            ExitNow(contains = true);
        }
    }

    iterator = kIteratorInit;

    while (GetNextOnMeshPrefix(iterator, prefix) == kErrorNone)
    {
        if ((prefix.mRloc16 == aRloc16) && prefix.mOnMesh && (prefix.mDefaultRoute || prefix.mDp))
        {
            ExitNow(contains = true);
        }
    }

exit:
    return contains;
}

} // namespace NetworkData
} // namespace ot
