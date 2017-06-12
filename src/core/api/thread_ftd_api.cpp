/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the OpenThread Thread API (FTD only).
 */

#define WPP_NAME "thread_ftd_api.tmh"

#include <openthread/openthread_config.h>

#if OPENTHREAD_FTD

#include <openthread/thread_ftd.h>
#include "openthread-core-config.h"
#include "openthread-instance.h"

using namespace ot;

uint8_t otThreadGetMaxAllowedChildren(otInstance *aInstance)
{
    uint8_t aNumChildren;

    (void)aInstance->mThreadNetif.GetMle().GetChildren(&aNumChildren);

    return aNumChildren;
}

otError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren)
{
    return aInstance->mThreadNetif.GetMle().SetMaxAllowedChildren(aMaxChildren);
}

bool otThreadIsRouterRoleEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsRouterRoleEnabled();
}

void otThreadSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.GetMle().SetRouterRoleEnabled(aEnabled);
}

otError otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().SetPreferredRouterId(aRouterId);
}

uint8_t otThreadGetLocalLeaderWeight(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderWeight();
}

void otThreadSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight)
{
    aInstance->mThreadNetif.GetMle().SetLeaderWeight(aWeight);
}

uint32_t otThreadGetLocalLeaderPartitionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderPartitionId();
}

void otThreadSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId)
{
    return aInstance->mThreadNetif.GetMle().SetLeaderPartitionId(aPartitionId);
}

uint16_t otThreadGetJoinerUdpPort(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetJoinerRouter().GetJoinerUdpPort();
}

otError otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
{
    return aInstance->mThreadNetif.GetJoinerRouter().SetJoinerUdpPort(aJoinerUdpPort);
}

uint32_t otThreadGetContextIdReuseDelay(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay)
{
    aInstance->mThreadNetif.GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
}

uint8_t otThreadGetNetworkIdTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetNetworkIdTimeout();
}

void otThreadSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout)
{
    aInstance->mThreadNetif.GetMle().SetNetworkIdTimeout((uint8_t)aTimeout);
}

uint8_t otThreadGetRouterUpgradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterUpgradeThreshold();
}

void otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterUpgradeThreshold(aThreshold);
}

otError otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().ReleaseRouterId(aRouterId);
}

otError otThreadBecomeRouter(otInstance *aInstance)
{
    otError error = OT_ERROR_INVALID_STATE;

    switch (aInstance->mThreadNetif.GetMle().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        error = aInstance->mThreadNetif.GetMle().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        error = OT_ERROR_NONE;
        break;
    }

    return error;
}

otError otThreadBecomeLeader(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeLeader();
}

uint8_t otThreadGetRouterDowngradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterDowngradeThreshold();
}

void otThreadSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otThreadGetRouterSelectionJitter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterSelectionJitter();
}

void otThreadSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter)
{
    aInstance->mThreadNetif.GetMle().SetRouterSelectionJitter(aRouterJitter);
}

otError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aChildInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

otError otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aChildInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

uint8_t otThreadGetRouterIdSequence(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterIdSequence();
}

otError otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aRouterInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

otError otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    otError error;

    VerifyOrExit(aEntry != NULL, error = OT_ERROR_INVALID_ARGS);
    error = aInstance->mThreadNetif.GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

otError otThreadSetSteeringData(otInstance *aInstance, otExtAddress *aExtAddress)
{
    otError error;

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    error = aInstance->mThreadNetif.GetMle().SetSteeringData(aExtAddress);
#else
    (void)aInstance;
    (void)aExtAddress;

    error = OT_ERROR_DISABLED_FEATURE;
#endif  // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

    return error;
}

const uint8_t *otThreadGetPSKc(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetKeyManager().GetPSKc();
}

otError otThreadSetPSKc(otInstance *aInstance, const uint8_t *aPSKc)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    aInstance->mThreadNetif.GetKeyManager().SetPSKc(aPSKc);
    aInstance->mThreadNetif.GetActiveDataset().Clear(false);
    aInstance->mThreadNetif.GetPendingDataset().Clear(false);

exit:
    return error;
}

#endif // OPENTHREAD_FTD
