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
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "mac/mac_types.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace NetworkData {

NetworkData::NetworkData(Instance &aInstance, Type aType)
    : InstanceLocator(aInstance)
    , mType(aType)
{
    mLength = 0;
}

void NetworkData::Clear(void)
{
    mLength = 0;
}

const NetworkDataTlv *NetworkData::FindTlv(const NetworkDataTlv *aStart,
                                           const NetworkDataTlv *aEnd,
                                           NetworkDataTlv::Type  aType)
{
    const NetworkDataTlv *tlv;

    for (tlv = aStart; tlv < aEnd; tlv = tlv->GetNext())
    {
        VerifyOrExit((tlv + 1) <= aEnd && tlv->GetNext() <= aEnd, tlv = NULL);

        if (tlv->GetType() == aType)
        {
            ExitNow();
        }
    }

    tlv = NULL;

exit:
    return tlv;
}

const NetworkDataTlv *NetworkData::FindTlv(const NetworkDataTlv *aStart,
                                           const NetworkDataTlv *aEnd,
                                           NetworkDataTlv::Type  aType,
                                           bool                  aStable)
{
    const NetworkDataTlv *tlv;

    for (tlv = aStart; tlv < aEnd; tlv = tlv->GetNext())
    {
        VerifyOrExit((tlv + 1) <= aEnd && tlv->GetNext() <= aEnd, tlv = NULL);

        if ((tlv->GetType() == aType) && (tlv->IsStable() == aStable))
        {
            ExitNow();
        }
    }

    tlv = NULL;

exit:
    return tlv;
}

const NetworkDataTlv *NetworkData::FindTlv(NetworkDataIterator &aIterator, NetworkDataTlv::Type aTlvType) const
{
    const NetworkDataTlv *start = aIterator.GetTlv(mTlvs);
    const NetworkDataTlv *tlv;

    tlv = FindTlv(start, GetTlvsEnd(), aTlvType);
    VerifyOrExit(tlv != NULL);

    if (tlv != start)
    {
        aIterator.SaveTlvOffset(tlv, mTlvs);
        aIterator.SetSubTlvOffset(0);
        aIterator.SetEntryIndex(0);
    }

exit:
    return tlv;
}

void NetworkData::IterateToNextTlv(NetworkDataIterator &aIterator) const
{
    const NetworkDataTlv *tlv = aIterator.GetTlv(mTlvs);

    tlv = tlv->GetNext();

    aIterator.SaveTlvOffset(tlv, mTlvs);
    aIterator.SetSubTlvOffset(0);
    aIterator.SetEntryIndex(0);
}

const NetworkDataTlv *NetworkData::FindSubTlv(NetworkDataIterator & aIterator,
                                              NetworkDataTlv::Type  aSubTlvType,
                                              const NetworkDataTlv *aSubTlvs,
                                              const NetworkDataTlv *aSubTlvsEnd) const
{
    const NetworkDataTlv *subStart = aIterator.GetSubTlv(aSubTlvs);
    const NetworkDataTlv *subTlv;

    subTlv = FindTlv(subStart, aSubTlvsEnd, aSubTlvType);
    VerifyOrExit(subTlv != NULL);

    if (subTlv != subStart)
    {
        aIterator.SaveSubTlvOffset(subTlv, aSubTlvs);
        aIterator.SetEntryIndex(0);
    }

exit:
    return subTlv;
}

void NetworkData::IterateToNextSubTlv(NetworkDataIterator &aIterator, const NetworkDataTlv *aSubTlvs) const
{
    const NetworkDataTlv *subTlv = aIterator.GetSubTlv(aSubTlvs);

    subTlv = subTlv->GetNext();

    aIterator.SaveSubTlvOffset(subTlv, aSubTlvs);
    aIterator.SetEntryIndex(0);
}

otError NetworkData::GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength) const
{
    otError error = OT_ERROR_NONE;

    OT_ASSERT(aData != NULL);
    VerifyOrExit(aDataLength >= mLength, error = OT_ERROR_NO_BUFS);

    memcpy(aData, mTlvs, mLength);
    aDataLength = mLength;

    if (aStable)
    {
        RemoveTemporaryData(aData, aDataLength);
    }

exit:
    return error;
}

otError NetworkData::GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig) const
{
    return GetNextOnMeshPrefix(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig) const
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);

    for (const PrefixTlv *prefix; (prefix = FindTlv<PrefixTlv>(iterator)) != NULL; IterateToNextTlv(iterator))
    {
        for (const BorderRouterTlv *borderRouter;
             (borderRouter = FindSubTlv<BorderRouterTlv>(iterator, prefix->GetSubTlvs(), prefix->GetNext())) != NULL;
             IterateToNextSubTlv(iterator, prefix->GetSubTlvs()))
        {
            for (uint8_t index = iterator.GetEntryIndex(); index < borderRouter->GetNumEntries(); index++)
            {
                if (aRloc16 == Mac::kShortAddrBroadcast || borderRouter->GetEntry(index)->GetRloc() == aRloc16)
                {
                    const BorderRouterEntry *borderRouterEntry = borderRouter->GetEntry(index);

                    memset(&aConfig, 0, sizeof(aConfig));
                    memcpy(&aConfig.mPrefix.mPrefix, prefix->GetPrefix(), BitVectorBytes(prefix->GetPrefixLength()));
                    aConfig.mPrefix.mLength = prefix->GetPrefixLength();
                    aConfig.mPreference     = borderRouterEntry->GetPreference();
                    aConfig.mPreferred      = borderRouterEntry->IsPreferred();
                    aConfig.mSlaac          = borderRouterEntry->IsSlaac();
                    aConfig.mDhcp           = borderRouterEntry->IsDhcp();
                    aConfig.mConfigure      = borderRouterEntry->IsConfigure();
                    aConfig.mDefaultRoute   = borderRouterEntry->IsDefaultRoute();
                    aConfig.mOnMesh         = borderRouterEntry->IsOnMesh();
                    aConfig.mStable         = borderRouter->IsStable();
                    aConfig.mRloc16         = borderRouterEntry->GetRloc();
                    aConfig.mNdDns          = borderRouterEntry->IsNdDns();
                    aConfig.mDp             = borderRouterEntry->IsDp();

                    iterator.SetEntryIndex(index + 1);

                    ExitNow(error = OT_ERROR_NONE);
                }
            }
        }
    }

exit:
    return error;
}

otError NetworkData::GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig) const
{
    return GetNextExternalRoute(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig) const
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);

    for (const PrefixTlv *prefix; (prefix = FindTlv<PrefixTlv>(iterator)) != NULL; IterateToNextTlv(iterator))
    {
        for (const HasRouteTlv *hasRoute;
             (hasRoute = FindSubTlv<HasRouteTlv>(iterator, prefix->GetSubTlvs(), prefix->GetNext())) != NULL;
             IterateToNextSubTlv(iterator, prefix->GetSubTlvs()))
        {
            for (uint8_t index = iterator.GetEntryIndex(); index < hasRoute->GetNumEntries(); index++)
            {
                if (aRloc16 == Mac::kShortAddrBroadcast || hasRoute->GetEntry(index)->GetRloc() == aRloc16)
                {
                    const HasRouteEntry *hasRouteEntry = hasRoute->GetEntry(index);

                    memset(&aConfig, 0, sizeof(aConfig));
                    memcpy(&aConfig.mPrefix.mPrefix, prefix->GetPrefix(), BitVectorBytes(prefix->GetPrefixLength()));
                    aConfig.mPrefix.mLength      = prefix->GetPrefixLength();
                    aConfig.mPreference          = hasRouteEntry->GetPreference();
                    aConfig.mStable              = hasRoute->IsStable();
                    aConfig.mRloc16              = hasRouteEntry->GetRloc();
                    aConfig.mNextHopIsThisDevice = (hasRouteEntry->GetRloc() == Get<Mle::MleRouter>().GetRloc16());

                    iterator.SetEntryIndex(index + 1);

                    ExitNow(error = OT_ERROR_NONE);
                }
            }
        }
    }

exit:
    return error;
}

otError NetworkData::GetNextService(Iterator &aIterator, ServiceConfig &aConfig) const
{
    return GetNextService(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig) const
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);

    for (const ServiceTlv *service; (service = FindTlv<ServiceTlv>(iterator)) != NULL; IterateToNextTlv(iterator))
    {
        for (const ServerTlv *server;
             (server = FindSubTlv<ServerTlv>(iterator, service->GetSubTlvs(), service->GetNext())) != NULL;
             IterateToNextSubTlv(iterator, service->GetSubTlvs()))
        {
            if (!iterator.IsNewEntry())
            {
                continue;
            }

            if ((aRloc16 == Mac::kShortAddrBroadcast) || (server->GetServer16() == aRloc16))
            {
                memset(&aConfig, 0, sizeof(aConfig));

                aConfig.mServiceId         = service->GetServiceId();
                aConfig.mEnterpriseNumber  = service->GetEnterpriseNumber();
                aConfig.mServiceDataLength = service->GetServiceDataLength();

                memcpy(&aConfig.mServiceData, service->GetServiceData(), service->GetServiceDataLength());

                aConfig.mServerConfig.mStable           = server->IsStable();
                aConfig.mServerConfig.mServerDataLength = server->GetServerDataLength();
                memcpy(&aConfig.mServerConfig.mServerData, server->GetServerData(), server->GetServerDataLength());
                aConfig.mServerConfig.mRloc16 = server->GetServer16();

                iterator.MarkEntryAsNotNew();

                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}

otError NetworkData::GetNextServiceId(Iterator &aIterator, uint16_t aRloc16, uint8_t &aServiceId) const
{
    otError       error;
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

    while (aCompare.GetNextOnMeshPrefix(outerIterator, aRloc16, outerConfig) == OT_ERROR_NONE)
    {
        Iterator           innerIterator = kIteratorInit;
        OnMeshPrefixConfig innerConfig;
        otError            error;

        while ((error = GetNextOnMeshPrefix(innerIterator, aRloc16, innerConfig)) == OT_ERROR_NONE)
        {
            if (memcmp(&outerConfig, &innerConfig, sizeof(outerConfig)) == 0)
            {
                break;
            }
        }

        if (error != OT_ERROR_NONE)
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

    while (aCompare.GetNextExternalRoute(outerIterator, aRloc16, outerConfig) == OT_ERROR_NONE)
    {
        Iterator            innerIterator = kIteratorInit;
        ExternalRouteConfig innerConfig;
        otError             error;

        while ((error = GetNextExternalRoute(innerIterator, aRloc16, innerConfig)) == OT_ERROR_NONE)
        {
            if (memcmp(&outerConfig, &innerConfig, sizeof(outerConfig)) == 0)
            {
                break;
            }
        }

        if (error != OT_ERROR_NONE)
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

    while (aCompare.GetNextService(outerIterator, aRloc16, outerConfig) == OT_ERROR_NONE)
    {
        Iterator      innerIterator = kIteratorInit;
        ServiceConfig innerConfig;
        otError       error;

        while ((error = GetNextService(innerIterator, aRloc16, innerConfig)) == OT_ERROR_NONE)
        {
            if ((outerConfig.mEnterpriseNumber == innerConfig.mEnterpriseNumber) &&
                (outerConfig.mServiceDataLength == innerConfig.mServiceDataLength) &&
                (memcmp(outerConfig.mServiceData, innerConfig.mServiceData, outerConfig.mServiceDataLength) == 0) &&
                (outerConfig.mServerConfig.mStable == innerConfig.mServerConfig.mStable) &&
                (outerConfig.mServerConfig.mServerDataLength == innerConfig.mServerConfig.mServerDataLength) &&
                (memcmp(outerConfig.mServerConfig.mServerData, innerConfig.mServerConfig.mServerData,
                        outerConfig.mServerConfig.mServerDataLength) == 0))
            {
                break;
            }
        }

        if (error != OT_ERROR_NONE)
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

    while (GetNextServiceId(iterator, aRloc16, serviceId) == OT_ERROR_NONE)
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
                ContextTlv *     context      = FindContext(aPrefix);

                // Replace p_border_router_16
                for (BorderRouterEntry *entry = borderRouter->GetFirstEntry(); entry <= borderRouter->GetLastEntry();
                     entry                    = entry->GetNext())
                {
                    if ((entry->IsDhcp() || entry->IsConfigure()) && (context != NULL))
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

const BorderRouterTlv *NetworkData::FindBorderRouter(const PrefixTlv &aPrefix)
{
    return FindTlv<BorderRouterTlv>(aPrefix.GetSubTlvs(), aPrefix.GetNext());
}

const BorderRouterTlv *NetworkData::FindBorderRouter(const PrefixTlv &aPrefix, bool aStable)
{
    return FindTlv<BorderRouterTlv>(aPrefix.GetSubTlvs(), aPrefix.GetNext(), aStable);
}

const HasRouteTlv *NetworkData::FindHasRoute(const PrefixTlv &aPrefix)
{
    return FindTlv<HasRouteTlv>(aPrefix.GetSubTlvs(), aPrefix.GetNext());
}

const HasRouteTlv *NetworkData::FindHasRoute(const PrefixTlv &aPrefix, bool aStable)
{
    return FindTlv<HasRouteTlv>(aPrefix.GetSubTlvs(), aPrefix.GetNext(), aStable);
}

const ContextTlv *NetworkData::FindContext(const PrefixTlv &aPrefix)
{
    return FindTlv<ContextTlv>(aPrefix.GetSubTlvs(), aPrefix.GetNext());
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
    const NetworkDataTlv *start = reinterpret_cast<const NetworkDataTlv *>(aTlvs);
    const NetworkDataTlv *end   = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);
    const PrefixTlv *     prefixTlv;

    while (start < end)
    {
        prefixTlv = FindTlv<PrefixTlv>(start, end);

        VerifyOrExit(prefixTlv != NULL);

        if (prefixTlv->GetPrefixLength() == aPrefixLength &&
            PrefixMatch(prefixTlv->GetPrefix(), aPrefix, aPrefixLength) >= aPrefixLength)
        {
            ExitNow();
        }

        start = prefixTlv->GetNext();
    }

    prefixTlv = NULL;

exit:
    return prefixTlv;
}

int8_t NetworkData::PrefixMatch(const uint8_t *a, const uint8_t *b, uint8_t aLength)
{
    uint8_t matchedLength;

    // Note that he `Ip6::Address::PrefixMatch` expects the prefix
    // length to be in bytes unit.

    matchedLength = Ip6::Address::PrefixMatch(a, b, BitVectorBytes(aLength));

    return (matchedLength >= aLength) ? static_cast<int8_t>(matchedLength) : -1;
}

const ServiceTlv *NetworkData::FindService(uint32_t       aEnterpriseNumber,
                                           const uint8_t *aServiceData,
                                           uint8_t        aServiceDataLength) const
{
    return FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, mTlvs, mLength);
}

const ServiceTlv *NetworkData::FindService(uint32_t       aEnterpriseNumber,
                                           const uint8_t *aServiceData,
                                           uint8_t        aServiceDataLength,
                                           const uint8_t *aTlvs,
                                           uint8_t        aTlvsLength)
{
    const NetworkDataTlv *start = reinterpret_cast<const NetworkDataTlv *>(aTlvs);
    const NetworkDataTlv *end   = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);
    const ServiceTlv *    serviceTlv;

    while (start < end)
    {
        serviceTlv = FindTlv<ServiceTlv>(start, end);

        VerifyOrExit(serviceTlv != NULL);

        if ((serviceTlv->GetEnterpriseNumber() == aEnterpriseNumber) &&
            (serviceTlv->GetServiceDataLength() == aServiceDataLength) &&
            (memcmp(serviceTlv->GetServiceData(), aServiceData, aServiceDataLength) == 0))
        {
            ExitNow();
        }

        start = serviceTlv->GetNext();
    }

    serviceTlv = NULL;

exit:
    return serviceTlv;
}

NetworkDataTlv *NetworkData::AppendTlv(uint8_t aTlvSize)
{
    NetworkDataTlv *tlv;

    VerifyOrExit(mLength + aTlvSize <= kMaxSize, tlv = NULL);

    tlv = GetTlvsEnd();
    mLength += aTlvSize;

exit:
    return tlv;
}

void NetworkData::Insert(void *aStart, uint8_t aLength)
{
    uint8_t *start = reinterpret_cast<uint8_t *>(aStart);

    OT_ASSERT(aLength + mLength <= sizeof(mTlvs) && mTlvs <= start && start <= mTlvs + mLength);
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

otError NetworkData::SendServerDataNotification(uint16_t aRloc16, Coap::ResponseHandler aHandler, void *aContext)
{
    otError          error   = OT_ERROR_NONE;
    Coap::Message *  message = NULL;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_SERVER_DATA));
    SuccessOrExit(error = message->SetPayloadMarker());

    if (mType == kTypeLocal)
    {
        ThreadTlv tlv;
        tlv.SetType(ThreadTlv::kThreadNetworkData);
        tlv.SetLength(mLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(mTlvs, mLength));
    }

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, ThreadTlv::kRloc16, aRloc16));
    }

    Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo, aHandler, aContext));

    otLogInfoNetData("Sent server data notification");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError NetworkData::GetNextServer(Iterator &aIterator, uint16_t &aRloc16) const
{
    otError             error = OT_ERROR_NONE;
    NetworkDataIterator iterator(aIterator);

    while (true)
    {
        switch (iterator.GetType())
        {
        case NetworkDataIterator::kTypeOnMeshPrefix:
        {
            OnMeshPrefixConfig prefixConfig;

            if (GetNextOnMeshPrefix(aIterator, prefixConfig) == OT_ERROR_NONE)
            {
                aRloc16 = prefixConfig.mRloc16;
                ExitNow();
            }

            iterator.SetType(NetworkDataIterator::kTypeExternalRoute);
            break;
        }

        case NetworkDataIterator::kTypeExternalRoute:
        {
            ExternalRouteConfig routeConfig;

            if (GetNextExternalRoute(aIterator, routeConfig) == OT_ERROR_NONE)
            {
                aRloc16 = routeConfig.mRloc16;
                ExitNow();
            }

            iterator.SetType(NetworkDataIterator::kTypeService);
            break;
        }

        case NetworkDataIterator::kTypeService:
        {
            ServiceConfig serviceConfig;

            if (GetNextService(aIterator, serviceConfig) == OT_ERROR_NONE)
            {
                aRloc16 = serviceConfig.mServerConfig.mRloc16;
                ExitNow();
            }

            ExitNow(error = OT_ERROR_NOT_FOUND);
        }
        }

        // Reset the iterator for next type.
        iterator.SetTlvOffset(0);
        iterator.SetSubTlvOffset(0);
        iterator.SetEntryIndex(0);
    }

exit:
    return error;
}

} // namespace NetworkData
} // namespace ot
