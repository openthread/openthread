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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

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

ThreadError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren)
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

ThreadError otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
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

ThreadError otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
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

ThreadError otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otThreadBecomeRouter(otInstance *aInstance)
{
    ThreadError error = kThreadError_InvalidState;

    switch (aInstance->mThreadNetif.GetMle().GetDeviceState())
    {
    case Mle::kDeviceStateDisabled:
    case Mle::kDeviceStateDetached:
        break;

    case Mle::kDeviceStateChild:
        error = aInstance->mThreadNetif.GetMle().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case Mle::kDeviceStateRouter:
    case Mle::kDeviceStateLeader:
        error = kThreadError_None;
        break;
    }

    return error;
}

ThreadError otThreadBecomeLeader(otInstance *aInstance)
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

ThreadError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

uint8_t otThreadGetRouterIdSequence(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterIdSequence();
}

ThreadError otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}


ThreadError otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aInstance->mThreadNetif.GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}


ThreadError otThreadSetSteeringData(otInstance *aInstance, otExtAddress *aExtAddress)
{
    ThreadError error;

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    error = aInstance->mThreadNetif.GetMle().SetSteeringData(aExtAddress);
#else
    (void)aInstance;
    (void)aExtAddress;

    error = kThreadError_DisabledFeature;
#endif  // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

    return error;
}

#endif // OPENTHREAD_FTD
