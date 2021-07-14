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
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "mac/mac_types.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetworkData {

NetworkData::NetworkData(Instance &aInstance, Type aType)
    : InstanceLocator(aInstance)
    , mType(aType)
{
    mLength = 0;
}

Error NetworkData::GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength) const
{
    Error error = kErrorNone;

    OT_ASSERT(aData != nullptr);
    VerifyOrExit(aDataLength >= mLength, error = kErrorNoBufs);

    memcpy(aData, mTlvs, mLength);
    aDataLength = mLength;

    if (aStable)
    {
        RemoveTemporaryData(aData, aDataLength);
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
    // On input, the non-nullptr pointer members in `aConfig` specify the
    // Network Data entry types (`mOnMeshPrefix`, `mExternalRoute`,
    // `mService`) to iterate over. On successful exit, the `aConfig`
    // is updated such that only one member pointer is not nullptr
    // indicating the type of entry and the non-nullptr config is updated
    // with the entry info.

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
                subTlvs = static_cast<const PrefixTlv *>(cur)->GetSubTlvs();
            }
            break;
        case NetworkDataTlv::kTypeService:
            if (aConfig.mService != nullptr)
            {
                subTlvs = static_cast<const ServiceTlv *>(cur)->GetSubTlvs();
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
                const PrefixTlv *prefixTlv = static_cast<const PrefixTlv *>(cur);

                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeBorderRouter:
                {
                    const BorderRouterTlv *borderRouter = static_cast<const BorderRouterTlv *>(subCur);

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
                    const HasRouteTlv *hasRoute = static_cast<const HasRouteTlv *>(subCur);

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
                const ServiceTlv *service = static_cast<const ServiceTlv *>(cur);

                if (aConfig.mService == nullptr)
                {
                    continue;
                }

                if (subCur->GetType() == NetworkDataTlv::kTypeServer)
                {
                    const ServerTlv *server = static_cast<const ServerTlv *>(subCur);

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

Error NetworkData::GetNextServiceId(Iterator &aIterator, uint16_t aRloc16, uint8_t &aServiceId) const
{
    Error         error;
    ServiceConfig config;

    SuccessOrExit(error = GetNextService(aIterator, aRloc16, config));
    aServiceId = config.mServiceId;

exit:
    return error;
}

bool NetworkData::ContainsOnMeshPrefixes(const NetworkData &aCompare, uint16_t aRloc16) const
{
    Iterator           outerIterator = kIteratorInit;
    OnMeshPrefixConfig outerConfig;
    bool               rval = true;

    while (aCompare.GetNextOnMeshPrefix(outerIterator, aRloc16, outerConfig) == kErrorNone)
    {
        Iterator           innerIterator = kIteratorInit;
        OnMeshPrefixConfig innerConfig;
        Error              error;

        while ((error = GetNextOnMeshPrefix(innerIterator, aRloc16, innerConfig)) == kErrorNone)
        {
            if (outerConfig == innerConfig)
            {
                break;
            }
        }

        if (error != kErrorNone)
        {
            ExitNow(rval = false);
        }
    }

exit:
    return rval;
}

bool NetworkData::ContainsExternalRoutes(const NetworkData &aCompare, uint16_t aRloc16) const
{
    Iterator            outerIterator = kIteratorInit;
    ExternalRouteConfig outerConfig;
    bool                rval = true;

    while (aCompare.GetNextExternalRoute(outerIterator, aRloc16, outerConfig) == kErrorNone)
    {
        Iterator            innerIterator = kIteratorInit;
        ExternalRouteConfig innerConfig;
        Error               error;

        while ((error = GetNextExternalRoute(innerIterator, aRloc16, innerConfig)) == kErrorNone)
        {
            if (outerConfig == innerConfig)
            {
                break;
            }
        }

        if (error != kErrorNone)
        {
            ExitNow(rval = false);
        }
    }

exit:
    return rval;
}

bool NetworkData::ContainsServices(const NetworkData &aCompare, uint16_t aRloc16) const
{
    Iterator      outerIterator = kIteratorInit;
    ServiceConfig outerConfig;
    bool          rval = true;

    while (aCompare.GetNextService(outerIterator, aRloc16, outerConfig) == kErrorNone)
    {
        Iterator      innerIterator = kIteratorInit;
        ServiceConfig innerConfig;
        Error         error;

        while ((error = GetNextService(innerIterator, aRloc16, innerConfig)) == kErrorNone)
        {
            if (outerConfig == innerConfig)
            {
                break;
            }
        }

        if (error != kErrorNone)
        {
            ExitNow(rval = false);
        }
    }

exit:
    return rval;
}

bool NetworkData::ContainsService(uint8_t aServiceId, uint16_t aRloc16) const
{
    Iterator iterator = kIteratorInit;
    uint8_t  serviceId;
    bool     rval = false;

    while (GetNextServiceId(iterator, aRloc16, serviceId) == kErrorNone)
    {
        if (serviceId == aServiceId)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aData);

    while (cur < reinterpret_cast<NetworkDataTlv *>(aData + aDataLength))
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            PrefixTlv *prefix = static_cast<PrefixTlv *>(cur);

            RemoveTemporaryData(aData, aDataLength, *prefix);

            if (prefix->GetSubTlvsLength() == 0)
            {
                RemoveTlv(aData, aDataLength, cur);
                continue;
            }

            otDumpDebgNetData("remove prefix done", aData, aDataLength);
            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            ServiceTlv *service = static_cast<ServiceTlv *>(cur);
            RemoveTemporaryData(aData, aDataLength, *service);

            if (service->GetSubTlvsLength() == 0)
            {
                RemoveTlv(aData, aDataLength, cur);
                continue;
            }

            otDumpDebgNetData("remove service done", aData, aDataLength);
            break;
        }

        default:
            // remove temporary tlv
            if (!cur->IsStable())
            {
                RemoveTlv(aData, aDataLength, cur);
                continue;
            }

            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", aData, aDataLength);
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix)
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
                BorderRouterTlv *borderRouter = static_cast<BorderRouterTlv *>(cur);
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
                HasRouteTlv *hasRoute = static_cast<HasRouteTlv *>(cur);

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
            RemoveTlv(aData, aDataLength, cur);
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - subTlvSize);
        }
    }
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, ServiceTlv &aService)
{
    NetworkDataTlv *cur = aService.GetSubTlvs();

    while (cur < aService.GetNext())
    {
        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeServer:
            {
                ServerTlv *server = static_cast<ServerTlv *>(cur);
                server->SetServer16(Mle::Mle::ServiceAlocFromId(aService.GetServiceId()));
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
            RemoveTlv(aData, aDataLength, cur);
            aService.SetSubTlvsLength(aService.GetSubTlvsLength() - subTlvSize);
        }
    }
}

const PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const
{
    return FindPrefix(aPrefix, aPrefixLength, mTlvs, mLength);
}

const PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix,
                                         uint8_t        aPrefixLength,
                                         const uint8_t *aTlvs,
                                         uint8_t        aTlvsLength)
{
    TlvIterator      tlvIterator(aTlvs, aTlvsLength);
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

const ServiceTlv *NetworkData::FindService(uint32_t         aEnterpriseNumber,
                                           const uint8_t *  aServiceData,
                                           uint8_t          aServiceDataLength,
                                           ServiceMatchMode aServiceMatchMode) const
{
    return FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, aServiceMatchMode, mTlvs, mLength);
}

const ServiceTlv *NetworkData::FindService(uint32_t         aEnterpriseNumber,
                                           const uint8_t *  aServiceData,
                                           uint8_t          aServiceDataLength,
                                           ServiceMatchMode aServiceMatchMode,
                                           const uint8_t *  aTlvs,
                                           uint8_t          aTlvsLength)
{
    TlvIterator       tlvIterator(aTlvs, aTlvsLength);
    const ServiceTlv *serviceTlv;

    while ((serviceTlv = tlvIterator.Iterate<ServiceTlv>()) != nullptr)
    {
        if (MatchService(*serviceTlv, aEnterpriseNumber, aServiceData, aServiceDataLength, aServiceMatchMode))
        {
            break;
        }
    }

    return serviceTlv;
}

const ServiceTlv *NetworkData::FindNextService(const ServiceTlv *aPrevServiceTlv,
                                               uint32_t          aEnterpriseNumber,
                                               const uint8_t *   aServiceData,
                                               uint8_t           aServiceDataLength,
                                               ServiceMatchMode  aServiceMatchMode) const
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

    return FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, aServiceMatchMode, tlvs, length);
}

bool NetworkData::MatchService(const ServiceTlv &aServiceTlv,
                               uint32_t          aEnterpriseNumber,
                               const uint8_t *   aServiceData,
                               uint8_t           aServiceDataLength,
                               ServiceMatchMode  aServiceMatchMode)
{
    bool match = false;

    VerifyOrExit(aServiceTlv.GetEnterpriseNumber() == aEnterpriseNumber &&
                 aServiceTlv.GetServiceDataLength() >= aServiceDataLength);

    switch (aServiceMatchMode)
    {
    case kServiceExactMatch:
        VerifyOrExit(aServiceTlv.GetServiceDataLength() == aServiceDataLength);
        OT_FALL_THROUGH;

    case kServicePrefixMatch:
        VerifyOrExit(memcmp(aServiceTlv.GetServiceData(), aServiceData, aServiceDataLength) == 0);
        break;
    }

    match = true;

exit:
    return match;
}

NetworkDataTlv *NetworkData::AppendTlv(uint16_t aTlvSize)
{
    NetworkDataTlv *tlv;

    VerifyOrExit(CanInsert(aTlvSize), tlv = nullptr);

    tlv = GetTlvsEnd();
    mLength += static_cast<uint8_t>(aTlvSize);

exit:
    return tlv;
}

void NetworkData::Insert(void *aStart, uint8_t aLength)
{
    uint8_t *start = reinterpret_cast<uint8_t *>(aStart);

    OT_ASSERT(CanInsert(aLength) && mTlvs <= start && start <= mTlvs + mLength);
    memmove(start + aLength, start, mLength - static_cast<size_t>(start - mTlvs));
    mLength += aLength;
}

void NetworkData::Remove(uint8_t *aData, uint8_t &aDataLength, uint8_t *aRemoveStart, uint8_t aRemoveLength)
{
    uint8_t *dataEnd   = aData + aDataLength;
    uint8_t *removeEnd = aRemoveStart + aRemoveLength;

    OT_ASSERT((aRemoveLength <= aDataLength) && (aData <= aRemoveStart) && (removeEnd <= dataEnd));

    memmove(aRemoveStart, removeEnd, static_cast<uint8_t>(dataEnd - removeEnd));
    aDataLength -= aRemoveLength;
}

void NetworkData::RemoveTlv(uint8_t *aData, uint8_t &aDataLength, NetworkDataTlv *aTlv)
{
    Remove(aData, aDataLength, reinterpret_cast<uint8_t *>(aTlv), aTlv->GetSize());
}

void NetworkData::Remove(void *aRemoveStart, uint8_t aRemoveLength)
{
    NetworkData::Remove(mTlvs, mLength, reinterpret_cast<uint8_t *>(aRemoveStart), aRemoveLength);
}

void NetworkData::RemoveTlv(NetworkDataTlv *aTlv)
{
    NetworkData::RemoveTlv(mTlvs, mLength, aTlv);
}

Error NetworkData::SendServerDataNotification(uint16_t aRloc16, Coap::ResponseHandler aHandler, void *aContext)
{
    Error            error   = kErrorNone;
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Tmf::Agent>().NewMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->InitAsConfirmablePost(UriPath::kServerData));
    SuccessOrExit(error = message->SetPayloadMarker());

    if (mType == kTypeLocal)
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

    IgnoreError(Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, aHandler, aContext));

    otLogInfoNetData("Sent server data notification");

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

} // namespace NetworkData
} // namespace ot
