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
 *   This file implements the OpenThread Network Data API.
 */

#include "openthread/netdata.h"

#include "openthread-instance.h"

using namespace Thread;

#ifdef __cplusplus
extern "C" {
#endif

ThreadError otNetDataGetLeader(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetNetworkDataLeader().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otNetDataGetLocal(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otNetDataAddPrefixInfo(otInstance *aInstance, const otBorderRouterConfig *aConfig)
{
    uint8_t flags = 0;

    if (aConfig->mPreferred)
    {
        flags |= NetworkData::BorderRouterEntry::kPreferredFlag;
    }

    if (aConfig->mSlaac)
    {
        flags |= NetworkData::BorderRouterEntry::kSlaacFlag;
    }

    if (aConfig->mDhcp)
    {
        flags |= NetworkData::BorderRouterEntry::kDhcpFlag;
    }

    if (aConfig->mConfigure)
    {
        flags |= NetworkData::BorderRouterEntry::kConfigureFlag;
    }

    if (aConfig->mDefaultRoute)
    {
        flags |= NetworkData::BorderRouterEntry::kDefaultRouteFlag;
    }

    if (aConfig->mOnMesh)
    {
        flags |= NetworkData::BorderRouterEntry::kOnMeshFlag;
    }

    return aInstance->mThreadNetif.GetNetworkDataLocal().AddOnMeshPrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                         aConfig->mPrefix.mLength,
                                                                         aConfig->mPreference, flags, aConfig->mStable);
}

ThreadError otNetDataRemovePrefixInfo(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otNetDataGetNextPrefixInfo(otInstance *aInstance, bool aLocal, otNetworkDataIterator *aIterator,
                                       otBorderRouterConfig *aConfig)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIterator && aConfig, error = kThreadError_InvalidArgs);

    if (aLocal)
    {
        error = aInstance->mThreadNetif.GetNetworkDataLocal().GetNextOnMeshPrefix(aIterator, aConfig);
    }
    else
    {
        error = aInstance->mThreadNetif.GetNetworkDataLeader().GetNextOnMeshPrefix(aIterator, aConfig);
    }

exit:
    return error;
}

ThreadError otNetDataAddRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().AddHasRoutePrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                           aConfig->mPrefix.mLength,
                                                                           aConfig->mPreference, aConfig->mStable);
}

ThreadError otNetDataRemoveRoute(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8,
                                                                              aPrefix->mLength);
}

ThreadError otNetDataRegister(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().SendServerDataNotification();
}

uint8_t otNetDataGetVersion(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetDataVersion();
}

uint8_t otNetDataGetStableVersion(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetStableDataVersion();
}

#ifdef __cplusplus
}  // extern "C"
#endif
