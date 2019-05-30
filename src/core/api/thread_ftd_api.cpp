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

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include <openthread/thread_ftd.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "thread/mle_constants.hpp"
#include "thread/topology.hpp"

using namespace ot;

uint8_t otThreadGetMaxAllowedChildren(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ChildTable>().GetMaxChildrenAllowed();
}

otError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ChildTable>().SetMaxChildrenAllowed(aMaxChildren);
}

bool otThreadIsRouterRoleEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().IsRouterRoleEnabled();
}

void otThreadSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetRouterRoleEnabled(aEnabled);
}

otError otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetPreferredRouterId(aRouterId);
}

uint8_t otThreadGetLocalLeaderWeight(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderWeight();
}

void otThreadSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetLeaderWeight(aWeight);
}

uint32_t otThreadGetLocalLeaderPartitionId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderPartitionId();
}

void otThreadSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetLeaderPartitionId(aPartitionId);
}

uint16_t otThreadGetJoinerUdpPort(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort();
}

otError otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(aJoinerUdpPort);

    return OT_ERROR_NONE;
}

uint32_t otThreadGetContextIdReuseDelay(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkData::Leader>().GetContextIdReuseDelay();
}

void otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Leader>().SetContextIdReuseDelay(aDelay);
}

uint8_t otThreadGetNetworkIdTimeout(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetNetworkIdTimeout();
}

void otThreadSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetNetworkIdTimeout(aTimeout);
}

uint8_t otThreadGetRouterUpgradeThreshold(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetRouterUpgradeThreshold();
}

void otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetRouterUpgradeThreshold(aThreshold);
}

otError otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aRouterId <= Mle::kMaxRouterId, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<RouterTable>().Release(aRouterId);

exit:
    return error;
}

otError otThreadBecomeRouter(otInstance *aInstance)
{
    otError   error    = OT_ERROR_INVALID_STATE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    switch (instance.Get<Mle::MleRouter>().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        error = instance.Get<Mle::MleRouter>().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
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
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().BecomeLeader();
}

uint8_t otThreadGetRouterDowngradeThreshold(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetRouterDowngradeThreshold();
}

void otThreadSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otThreadGetRouterSelectionJitter(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetRouterSelectionJitter();
}

void otThreadSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetRouterSelectionJitter(aRouterJitter);
}

otError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aChildInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<Mle::MleRouter>().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

otError otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aChildInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<Mle::MleRouter>().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

otError otThreadGetChildNextIp6Address(otInstance *               aInstance,
                                       uint8_t                    aChildIndex,
                                       otChildIp6AddressIterator *aIterator,
                                       otIp6Address *             aAddress)
{
    otError                   error    = OT_ERROR_NONE;
    Instance &                instance = *static_cast<Instance *>(aInstance);
    Child::Ip6AddressIterator iterator;
    Ip6::Address *            address;

    VerifyOrExit(aIterator != NULL && aAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    address = static_cast<Ip6::Address *>(aAddress);
    iterator.Set(*aIterator);

    SuccessOrExit(error = instance.Get<Mle::MleRouter>().GetChildNextIp6Address(aChildIndex, iterator, *address));

    *aIterator = iterator.Get();

exit:
    return error;
}

uint8_t otThreadGetRouterIdSequence(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<RouterTable>().GetRouterIdSequence();
}

uint8_t otThreadGetMaxRouterId(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return Mle::kMaxRouterId;
}

otError otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aRouterInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<RouterTable>().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

otError otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aEntry != NULL, error = OT_ERROR_INVALID_ARGS);
    error = instance.Get<AddressResolver>().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

otError otThreadSetSteeringData(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError error;

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    Instance &instance = *static_cast<Instance *>(aInstance);

    error = instance.Get<Mle::MleRouter>().SetSteeringData(static_cast<const Mac::ExtAddress *>(aExtAddress));
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);

    error = OT_ERROR_DISABLED_FEATURE;
#endif

    return error;
}

const otPSKc *otThreadGetPSKc(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<KeyManager>().GetPSKc();
}

otError otThreadSetPSKc(otInstance *aInstance, const otPSKc *aPSKc)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<KeyManager>().SetPSKc(*aPSKc);
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

int8_t otThreadGetParentPriority(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetAssignParentPriority();
}

otError otThreadSetParentPriority(otInstance *aInstance, const int8_t aParentPriority)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetAssignParentPriority(aParentPriority);
}

void otThreadRegisterNeighborTableCallback(otInstance *aInstance, otNeighborTableCallback aCallback)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().RegisterNeighborTableChangedCallback(aCallback);
}

#endif // OPENTHREAD_FTD
