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

#define WPP_NAME "network_data.tmh"

#include "network_data.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "mac/mac_frame.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace NetworkData {

NetworkData::NetworkData(Instance &aInstance, Type aType)
    : InstanceLocator(aInstance)
    , mType(aType)
    , mLastAttemptWait(false)
    , mLastAttempt(0)
{
    mLength = 0;
}

void NetworkData::Clear(void)
{
    mLength = 0;
}

otError NetworkData::GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength)
{
    otError error = OT_ERROR_NONE;

    assert(aData != NULL);
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

otError NetworkData::GetNextOnMeshPrefix(otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig)
{
    return GetNextOnMeshPrefix(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextOnMeshPrefix(otNetworkDataIterator *aIterator,
                                         uint16_t               aRloc16,
                                         otBorderRouterConfig * aConfig)
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);
    NetworkDataTlv *    cur = reinterpret_cast<NetworkDataTlv *>(mTlvs + iterator.GetTlvOffset());
    NetworkDataTlv *    end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext(), iterator.SetSubTlvOffset(0), iterator.SetEntryIndex(0))
    {
        PrefixTlv *     prefix;
        NetworkDataTlv *subCur;
        NetworkDataTlv *subEnd;

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);
        subCur = reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(prefix->GetSubTlvs()) +
                                                    iterator.GetSubTlvOffset());
        subEnd = cur->GetNext();

        for (; subCur < subEnd; subCur = subCur->GetNext(), iterator.SetEntryIndex(0))
        {
            BorderRouterTlv *borderRouter;

            VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

            if (subCur->GetType() != NetworkDataTlv::kTypeBorderRouter)
            {
                continue;
            }

            borderRouter = static_cast<BorderRouterTlv *>(subCur);

            for (uint8_t index = iterator.GetEntryIndex(); index < borderRouter->GetNumEntries(); index++)
            {
                if (aRloc16 == Mac::kShortAddrBroadcast || borderRouter->GetEntry(index)->GetRloc() == aRloc16)
                {
                    BorderRouterEntry *borderRouterEntry = borderRouter->GetEntry(index);

                    memset(aConfig, 0, sizeof(*aConfig));
                    memcpy(&aConfig->mPrefix.mPrefix, prefix->GetPrefix(), BitVectorBytes(prefix->GetPrefixLength()));
                    aConfig->mPrefix.mLength = prefix->GetPrefixLength();
                    aConfig->mPreference     = borderRouterEntry->GetPreference();
                    aConfig->mPreferred      = borderRouterEntry->IsPreferred();
                    aConfig->mSlaac          = borderRouterEntry->IsSlaac();
                    aConfig->mDhcp           = borderRouterEntry->IsDhcp();
                    aConfig->mConfigure      = borderRouterEntry->IsConfigure();
                    aConfig->mDefaultRoute   = borderRouterEntry->IsDefaultRoute();
                    aConfig->mOnMesh         = borderRouterEntry->IsOnMesh();
                    aConfig->mStable         = borderRouter->IsStable();
                    aConfig->mRloc16         = borderRouterEntry->GetRloc();

                    iterator.SaveTlvOffset(cur, mTlvs);
                    iterator.SaveSubTlvOffset(subCur, prefix->GetSubTlvs());
                    iterator.SetEntryIndex(index + 1);

                    ExitNow(error = OT_ERROR_NONE);
                }
            }
        }
    }

exit:
    return error;
}

otError NetworkData::GetNextExternalRoute(otNetworkDataIterator *aIterator, otExternalRouteConfig *aConfig)
{
    return GetNextExternalRoute(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextExternalRoute(otNetworkDataIterator *aIterator,
                                          uint16_t               aRloc16,
                                          otExternalRouteConfig *aConfig)
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);
    NetworkDataTlv *    cur = reinterpret_cast<NetworkDataTlv *>(mTlvs + iterator.GetTlvOffset());
    NetworkDataTlv *    end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext(), iterator.SetSubTlvOffset(0), iterator.SetEntryIndex(0))
    {
        PrefixTlv *     prefix;
        NetworkDataTlv *subCur;
        NetworkDataTlv *subEnd;

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        subCur = reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(prefix->GetSubTlvs()) +
                                                    iterator.GetSubTlvOffset());
        subEnd = cur->GetNext();

        for (; subCur < subEnd; subCur = subCur->GetNext(), iterator.SetEntryIndex(0))
        {
            HasRouteTlv *hasRoute;

            VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

            if (subCur->GetType() != NetworkDataTlv::kTypeHasRoute)
            {
                continue;
            }

            hasRoute = static_cast<HasRouteTlv *>(subCur);

            for (uint8_t index = iterator.GetEntryIndex(); index < hasRoute->GetNumEntries(); index++)
            {
                if (aRloc16 == Mac::kShortAddrBroadcast || hasRoute->GetEntry(index)->GetRloc() == aRloc16)
                {
                    HasRouteEntry *hasRouteEntry = hasRoute->GetEntry(index);

                    memset(aConfig, 0, sizeof(*aConfig));
                    memcpy(&aConfig->mPrefix.mPrefix, prefix->GetPrefix(), BitVectorBytes(prefix->GetPrefixLength()));
                    aConfig->mPrefix.mLength      = prefix->GetPrefixLength();
                    aConfig->mPreference          = hasRouteEntry->GetPreference();
                    aConfig->mStable              = hasRoute->IsStable();
                    aConfig->mRloc16              = hasRouteEntry->GetRloc();
                    aConfig->mNextHopIsThisDevice = (hasRouteEntry->GetRloc() == GetNetif().GetMle().GetRloc16());

                    iterator.SaveTlvOffset(cur, mTlvs);
                    iterator.SaveSubTlvOffset(subCur, prefix->GetSubTlvs());
                    iterator.SetEntryIndex(index + 1);

                    ExitNow(error = OT_ERROR_NONE);
                }
            }
        }
    }

exit:
    return error;
}

#if OPENTHREAD_ENABLE_SERVICE
otError NetworkData::GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig)
{
    return GetNextService(aIterator, Mac::kShortAddrBroadcast, aConfig);
}

otError NetworkData::GetNextService(otNetworkDataIterator *aIterator, uint16_t aRloc16, otServiceConfig *aConfig)
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);
    NetworkDataTlv *    cur = reinterpret_cast<NetworkDataTlv *>(mTlvs + iterator.GetTlvOffset());
    NetworkDataTlv *    end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext(), iterator.SetSubTlvOffset(0))
    {
        ServiceTlv *    service;
        NetworkDataTlv *subCur;
        NetworkDataTlv *subEnd;

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        if (cur->GetType() != NetworkDataTlv::kTypeService)
        {
            continue;
        }

        service = static_cast<ServiceTlv *>(cur);

        subCur = reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(service->GetSubTlvs()) +
                                                    iterator.GetSubTlvOffset());
        subEnd = cur->GetNext();

        for (; subCur < subEnd; subCur = subCur->GetNext())
        {
            ServerTlv *server;

            VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

            if (subCur->GetType() != NetworkDataTlv::kTypeServer)
            {
                continue;
            }

            server = static_cast<ServerTlv *>(subCur);

            if ((aRloc16 == Mac::kShortAddrBroadcast) || (server->GetServer16() == aRloc16))
            {
                memset(aConfig, 0, sizeof(*aConfig));

                aConfig->mServiceID         = service->GetServiceID();
                aConfig->mEnterpriseNumber  = service->GetEnterpriseNumber();
                aConfig->mServiceDataLength = service->GetServiceDataLength();

                memcpy(&aConfig->mServiceData, service->GetServiceData(), service->GetServiceDataLength());

                aConfig->mServerConfig.mStable           = server->IsStable();
                aConfig->mServerConfig.mServerDataLength = server->GetServerDataLength();
                memcpy(&aConfig->mServerConfig.mServerData, server->GetServerData(), server->GetServerDataLength());
                aConfig->mServerConfig.mRloc16 = server->GetServer16();

                if (subCur->GetNext() >= cur->GetNext())
                {
                    iterator.SaveTlvOffset(cur->GetNext(), mTlvs);
                    iterator.SetSubTlvOffset(0);
                }
                else
                {
                    iterator.SaveTlvOffset(cur, mTlvs);
                    iterator.SaveSubTlvOffset(subCur->GetNext(), service->GetSubTlvs());
                }

                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}

otError NetworkData::GetNextServiceId(otNetworkDataIterator *aIterator, uint16_t aRloc16, uint8_t *aServiceId)
{
    otError             error = OT_ERROR_NOT_FOUND;
    NetworkDataIterator iterator(aIterator);
    NetworkDataTlv *    cur = reinterpret_cast<NetworkDataTlv *>(mTlvs + iterator.GetTlvOffset());
    NetworkDataTlv *    end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext(), iterator.SetSubTlvOffset(0))
    {
        ServiceTlv *    service;
        NetworkDataTlv *subCur;
        NetworkDataTlv *subEnd;

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        if (cur->GetType() != NetworkDataTlv::kTypeService)
        {
            continue;
        }

        service = static_cast<ServiceTlv *>(cur);

        subCur = reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(service->GetSubTlvs()) +
                                                    iterator.GetSubTlvOffset());
        subEnd = cur->GetNext();

        for (; subCur < subEnd; subCur = subCur->GetNext())
        {
            ServerTlv *server;

            VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

            if (subCur->GetType() != NetworkDataTlv::kTypeServer)
            {
                continue;
            }

            server = static_cast<ServerTlv *>(subCur);

            if ((aRloc16 == Mac::kShortAddrBroadcast) || (server->GetServer16() == aRloc16))
            {
                *aServiceId = service->GetServiceID();

                if (subCur->GetNext() >= cur->GetNext())
                {
                    iterator.SaveTlvOffset(cur->GetNext(), mTlvs);
                    iterator.SetSubTlvOffset(0);
                }
                else
                {
                    iterator.SaveTlvOffset(cur, mTlvs);
                    iterator.SaveSubTlvOffset(subCur->GetNext(), service->GetSubTlvs());
                }

                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:
    return error;
}
#endif

bool NetworkData::ContainsOnMeshPrefixes(NetworkData &aCompare, uint16_t aRloc16)
{
    otNetworkDataIterator outerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  outerConfig;
    bool                  rval = true;

    while (aCompare.GetNextOnMeshPrefix(&outerIterator, aRloc16, &outerConfig) == OT_ERROR_NONE)
    {
        otNetworkDataIterator innerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otBorderRouterConfig  innerConfig;
        otError               error;

        while ((error = GetNextOnMeshPrefix(&innerIterator, aRloc16, &innerConfig)) == OT_ERROR_NONE)
        {
            if (memcmp(&outerConfig, &innerConfig, (sizeof(outerConfig) - sizeof(outerConfig.mRloc16))) == 0)
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

bool NetworkData::ContainsExternalRoutes(NetworkData &aCompare, uint16_t aRloc16)
{
    otNetworkDataIterator outerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig outerConfig;
    bool                  rval = true;

    while (aCompare.GetNextExternalRoute(&outerIterator, aRloc16, &outerConfig) == OT_ERROR_NONE)
    {
        otNetworkDataIterator innerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otExternalRouteConfig innerConfig;
        otError               error;

        while ((error = GetNextExternalRoute(&innerIterator, aRloc16, &innerConfig)) == OT_ERROR_NONE)
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

#if OPENTHREAD_ENABLE_SERVICE
bool NetworkData::ContainsServices(NetworkData &aCompare, uint16_t aRloc16)
{
    otNetworkDataIterator outerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otServiceConfig       outerConfig;
    bool                  rval = true;

    while (aCompare.GetNextService(&outerIterator, aRloc16, &outerConfig) == OT_ERROR_NONE)
    {
        otNetworkDataIterator innerIterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otServiceConfig       innerConfig;
        otError               error;

        while ((error = GetNextService(&innerIterator, aRloc16, &innerConfig)) == OT_ERROR_NONE)
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

bool NetworkData::ContainsService(uint8_t aServiceId, uint16_t aRloc16)
{
    bool            rval = false;
    NetworkDataTlv *cur  = reinterpret_cast<NetworkDataTlv *>(mTlvs);
    NetworkDataTlv *end  = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext())
    {
        ServiceTlv *    service;
        NetworkDataTlv *subCur;
        NetworkDataTlv *subEnd;

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() != NetworkDataTlv::kTypeService)
        {
            continue;
        }

        service = static_cast<ServiceTlv *>(cur);

        if (service->GetServiceID() == aServiceId)
        {
            subCur = reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(service->GetSubTlvs()));
            subEnd = cur->GetNext();

            for (; subCur < subEnd; subCur = subCur->GetNext())
            {
                ServerTlv *server;

                VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd);

                if (subCur->GetType() != NetworkDataTlv::kTypeServer)
                {
                    continue;
                }

                server = static_cast<ServerTlv *>(subCur);

                if (server->GetServer16() == aRloc16)
                {
                    ExitNow(rval = true);
                }
            }
        }
    }

exit:
    return rval;
}
#endif

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aData);
    NetworkDataTlv *end;
    PrefixTlv *     prefix;
#if OPENTHREAD_ENABLE_SERVICE
    ServiceTlv *service;
#endif
    uint8_t  length;
    uint8_t *dst;
    uint8_t *src;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(aData + aDataLength);

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            RemoveTemporaryData(aData, aDataLength, *prefix);

            if (prefix->GetSubTlvsLength() == 0)
            {
                length = sizeof(NetworkDataTlv) + cur->GetLength();
                dst    = reinterpret_cast<uint8_t *>(cur);
                src    = reinterpret_cast<uint8_t *>(cur->GetNext());
                memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
                aDataLength -= length;
                continue;
            }

            otDumpDebgNetData("remove prefix done", mTlvs, mLength);
            break;
        }

#if OPENTHREAD_ENABLE_SERVICE

        case NetworkDataTlv::kTypeService:
        {
            service = reinterpret_cast<ServiceTlv *>(cur);
            RemoveTemporaryData(aData, aDataLength, *service);

            if (service->GetSubTlvsLength() == 0)
            {
                length = sizeof(NetworkDataTlv) + cur->GetLength();
                dst    = reinterpret_cast<uint8_t *>(cur);
                src    = reinterpret_cast<uint8_t *>(cur->GetNext());
                memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
                aDataLength -= length;
                continue;
            }

            otDumpDebgNetData("remove service done", mTlvs, mLength);
            break;
        }

#endif

        default:
        {
            // remove temporary tlv
            if (!cur->IsStable())
            {
                length = sizeof(NetworkDataTlv) + cur->GetLength();
                dst    = reinterpret_cast<uint8_t *>(cur);
                src    = reinterpret_cast<uint8_t *>(cur->GetNext());
                memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
                aDataLength -= length;
                continue;
            }

            break;
        }
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", aData, aDataLength);
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix)
{
    NetworkDataTlv *   cur = aPrefix.GetSubTlvs();
    NetworkDataTlv *   end;
    BorderRouterTlv *  borderRouter;
    HasRouteTlv *      hasRoute;
    ContextTlv *       context;
    BorderRouterEntry *borderRouterEntry;
    HasRouteEntry *    hasRouteEntry;
    uint8_t            length;
    uint8_t            contextId;
    uint8_t *          dst;
    uint8_t *          src;

    while (1)
    {
        end = aPrefix.GetNext();

        if (cur >= end)
        {
            break;
        }

        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeBorderRouter:
            {
                borderRouter = FindBorderRouter(aPrefix);

                if ((context = FindContext(aPrefix)) == NULL)
                {
                    break;
                }

                contextId = context->GetContextId();

                // replace p_border_router_16
                for (uint8_t i = 0; i < borderRouter->GetNumEntries(); i++)
                {
                    borderRouterEntry = borderRouter->GetEntry(i);

                    if (borderRouterEntry->IsDhcp() || borderRouterEntry->IsConfigure())
                    {
                        borderRouterEntry->SetRloc(0xfc00 | contextId);
                    }
                    else
                    {
                        borderRouterEntry->SetRloc(0xfffe);
                    }
                }

                break;
            }

            case NetworkDataTlv::kTypeHasRoute:
            {
                hasRoute = FindHasRoute(aPrefix);

                // replace r_border_router_16
                for (uint8_t j = 0; j < hasRoute->GetNumEntries(); j++)
                {
                    hasRouteEntry = hasRoute->GetEntry(j);
                    hasRouteEntry->SetRloc(0xfffe);
                }

                break;
            }

            default:
            {
                break;
            }
            }

            // keep stable tlv
            cur = cur->GetNext();
        }
        else
        {
            // remove temporary tlv
            length = sizeof(NetworkDataTlv) + cur->GetLength();
            dst    = reinterpret_cast<uint8_t *>(cur);
            src    = reinterpret_cast<uint8_t *>(cur->GetNext());
            memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - length);
            aDataLength -= length;
        }
    }
}

#if OPENTHREAD_ENABLE_SERVICE
void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, ServiceTlv &aService)
{
    NetworkDataTlv *cur = aService.GetSubTlvs();
    NetworkDataTlv *end;
    ServerTlv *     server;
    uint8_t         length;
    uint8_t *       dst;
    uint8_t *       src;

    while (1)
    {
        end = aService.GetNext();

        if (cur >= end)
        {
            break;
        }

        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeServer:
            {
                server = reinterpret_cast<ServerTlv *>(cur);
                server->SetServer16(Mle::Mle::GetServiceAlocFromId(aService.GetServiceID()));
                break;
            }

            default:
            {
                break;
            }
            }

            // keep stable tlv
            cur = cur->GetNext();
        }
        else
        {
            // remove temporary tlv
            length = sizeof(NetworkDataTlv) + cur->GetLength();
            dst    = reinterpret_cast<uint8_t *>(cur);
            src    = reinterpret_cast<uint8_t *>(cur->GetNext());
            memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
            aService.SetSubTlvsLength(aService.GetSubTlvsLength() - length);
            aDataLength -= length;
        }
    }
}
#endif

BorderRouterTlv *NetworkData::FindBorderRouter(PrefixTlv &aPrefix)
{
    BorderRouterTlv *rval = NULL;
    NetworkDataTlv * cur  = aPrefix.GetSubTlvs();
    NetworkDataTlv * end  = aPrefix.GetNext();

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeBorderRouter)
        {
            ExitNow(rval = reinterpret_cast<BorderRouterTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

BorderRouterTlv *NetworkData::FindBorderRouter(PrefixTlv &aPrefix, bool aStable)
{
    BorderRouterTlv *rval = NULL;
    NetworkDataTlv * cur  = aPrefix.GetSubTlvs();
    NetworkDataTlv * end  = aPrefix.GetNext();

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeBorderRouter && cur->IsStable() == aStable)
        {
            ExitNow(rval = reinterpret_cast<BorderRouterTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

HasRouteTlv *NetworkData::FindHasRoute(PrefixTlv &aPrefix)
{
    HasRouteTlv *   rval = NULL;
    NetworkDataTlv *cur  = aPrefix.GetSubTlvs();
    NetworkDataTlv *end  = aPrefix.GetNext();

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeHasRoute)
        {
            ExitNow(rval = reinterpret_cast<HasRouteTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

HasRouteTlv *NetworkData::FindHasRoute(PrefixTlv &aPrefix, bool aStable)
{
    HasRouteTlv *   rval = NULL;
    NetworkDataTlv *cur  = aPrefix.GetSubTlvs();
    NetworkDataTlv *end  = aPrefix.GetNext();

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeHasRoute && cur->IsStable() == aStable)
        {
            ExitNow(rval = reinterpret_cast<HasRouteTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

ContextTlv *NetworkData::FindContext(PrefixTlv &aPrefix)
{
    ContextTlv *    rval = NULL;
    NetworkDataTlv *cur  = aPrefix.GetSubTlvs();
    NetworkDataTlv *end  = aPrefix.GetNext();

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeContext)
        {
            ExitNow(rval = reinterpret_cast<ContextTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    return FindPrefix(aPrefix, aPrefixLength, mTlvs, mLength);
}

PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    NetworkDataTlv *cur     = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end     = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);
    PrefixTlv *     compare = NULL;

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypePrefix)
        {
            compare = reinterpret_cast<PrefixTlv *>(cur);

            if (compare->GetPrefixLength() == aPrefixLength &&
                PrefixMatch(compare->GetPrefix(), aPrefix, aPrefixLength) >= aPrefixLength)
            {
                ExitNow();
            }
        }

        cur = cur->GetNext();
    }

    compare = NULL;

exit:
    return compare;
}

int8_t NetworkData::PrefixMatch(const uint8_t *a, const uint8_t *b, uint8_t aLength)
{
    int8_t  rval  = 0;
    uint8_t bytes = BitVectorBytes(aLength);
    uint8_t diff;

    for (uint8_t i = 0; i < bytes; i++)
    {
        diff = a[i] ^ b[i];

        if (diff == 0)
        {
            rval += 8;
        }
        else
        {
            while ((diff & 0x80) == 0)
            {
                rval++;
                diff <<= 1;
            }

            break;
        }
    }

    return (rval >= aLength) ? rval : -1;
}

#if OPENTHREAD_ENABLE_SERVICE
ServiceTlv *NetworkData::FindService(uint32_t       aEnterpriseNumber,
                                     const uint8_t *aServiceData,
                                     uint8_t        aServiceDataLength)
{
    return FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, mTlvs, mLength);
}

ServiceTlv *NetworkData::FindService(uint32_t       aEnterpriseNumber,
                                     const uint8_t *aServiceData,
                                     uint8_t        aServiceDataLength,
                                     uint8_t *      aTlvs,
                                     uint8_t        aTlvsLength)
{
    NetworkDataTlv *cur     = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end     = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);
    ServiceTlv *    compare = NULL;

    while (cur < end)
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        if (cur->GetType() == NetworkDataTlv::kTypeService)
        {
            compare = reinterpret_cast<ServiceTlv *>(cur);

            if ((compare->GetEnterpriseNumber() == aEnterpriseNumber) &&
                (compare->GetServiceDataLength() == aServiceDataLength) &&
                (memcmp(compare->GetServiceData(), aServiceData, aServiceDataLength) == 0))
            {
                ExitNow();
            }
        }

        cur = cur->GetNext();
    }

    compare = NULL;

exit:
    return compare;
}
#endif

otError NetworkData::Insert(uint8_t *aStart, uint8_t aLength)
{
    assert(aLength + mLength <= sizeof(mTlvs) && mTlvs <= aStart && aStart <= mTlvs + mLength);
    memmove(aStart + aLength, aStart, mLength - static_cast<size_t>(aStart - mTlvs));
    mLength += aLength;
    return OT_ERROR_NONE;
}

otError NetworkData::Remove(uint8_t *aStart, uint8_t aLength)
{
    assert(aLength <= mLength && mTlvs <= aStart && (aStart - mTlvs) + aLength <= mLength);
    memmove(aStart, aStart + aLength, mLength - (static_cast<size_t>(aStart - mTlvs) + aLength));
    mLength -= aLength;
    return OT_ERROR_NONE;
}

otError NetworkData::SendServerDataNotification(uint16_t aRloc16)
{
    ThreadNetif &    netif   = GetNetif();
    otError          error   = OT_ERROR_NONE;
    Coap::Message *  message = NULL;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(!mLastAttemptWait || static_cast<int32_t>(TimerMilli::GetNow() - mLastAttempt) < kDataResubmitDelay,
                 error = OT_ERROR_ALREADY);

    VerifyOrExit((message = netif.GetCoap().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    message->SetToken(Coap::Message::kDefaultTokenLength);
    message->AppendUriPathOptions(OT_URI_PATH_SERVER_DATA);
    message->SetPayloadMarker();

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
        ThreadRloc16Tlv rloc16Tlv;
        rloc16Tlv.Init();
        rloc16Tlv.SetRloc16(aRloc16);
        SuccessOrExit(error = message->Append(&rloc16Tlv, sizeof(rloc16Tlv)));
    }

    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    if (mType == kTypeLocal)
    {
        mLastAttempt     = TimerMilli::GetNow();
        mLastAttemptWait = true;
    }

    otLogInfoNetData("Sent server data notification");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkData::ClearResubmitDelayTimer(void)
{
    mLastAttemptWait = false;
}

} // namespace NetworkData
} // namespace ot
