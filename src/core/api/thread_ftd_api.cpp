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

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include <openthread/thread_ftd.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "thread/mle_types.hpp"
#include "thread/topology.hpp"

using namespace ot;

uint16_t otThreadGetMaxAllowedChildren(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ChildTable>().GetMaxChildrenAllowed();
}

otError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint16_t aMaxChildren)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<ChildTable>().SetMaxChildrenAllowed(aMaxChildren);
}

uint8_t otThreadGetMaxChildIpAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetMaxChildIpAddresses();
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError otThreadSetMaxChildIpAddresses(otInstance *aInstance, uint8_t aMaxIpAddresses)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetMaxChildIpAddresses(aMaxIpAddresses);
}
#endif

bool otThreadIsRouterEligible(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().IsRouterEligible();
}

otError otThreadSetRouterEligible(otInstance *aInstance, bool aEligible)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetRouterEligible(aEligible);
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
    case Mle::kRoleDisabled:
    case Mle::kRoleDetached:
        break;

    case Mle::kRoleChild:
        error = instance.Get<Mle::MleRouter>().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case Mle::kRoleRouter:
    case Mle::kRoleLeader:
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

    IgnoreError(instance.Get<Mle::MleRouter>().SetRouterSelectionJitter(aRouterJitter));
}

otError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aChildInfo != nullptr);

    return instance.Get<ChildTable>().GetChildInfoById(aChildId, *static_cast<Child::Info *>(aChildInfo));
}

otError otThreadGetChildInfoByIndex(otInstance *aInstance, uint16_t aChildIndex, otChildInfo *aChildInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aChildInfo != nullptr);

    return instance.Get<ChildTable>().GetChildInfoByIndex(aChildIndex, *static_cast<Child::Info *>(aChildInfo));
}

otError otThreadGetChildNextIp6Address(otInstance *               aInstance,
                                       uint16_t                   aChildIndex,
                                       otChildIp6AddressIterator *aIterIndex,
                                       otIp6Address *             aAddress)
{
    otError      error    = OT_ERROR_NONE;
    Instance &   instance = *static_cast<Instance *>(aInstance);
    const Child *child;

    OT_ASSERT(aIterIndex != nullptr && aAddress != nullptr);

    child = instance.Get<ChildTable>().GetChildAtIndex(aChildIndex);
    VerifyOrExit(child != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(child->IsStateValidOrRestoring(), error = OT_ERROR_INVALID_ARGS);

    {
        Child::AddressIterator iter(*child, *aIterIndex);

        VerifyOrExit(!iter.IsDone(), error = OT_ERROR_NOT_FOUND);
        *aAddress = *iter.GetAddress();

        iter++;
        *aIterIndex = iter.GetAsIndex();
    }

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
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aRouterInfo != nullptr);

    return instance.Get<RouterTable>().GetRouterInfo(aRouterId, *static_cast<Router::Info *>(aRouterInfo));
}

otError otThreadGetNextCacheEntry(otInstance *aInstance, otCacheEntryInfo *aEntryInfo, otCacheEntryIterator *aIterator)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT((aIterator != nullptr) && (aEntryInfo != nullptr));

    return instance.Get<AddressResolver>().GetNextCacheEntry(*aEntryInfo, *aIterator);
}

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
void otThreadSetSteeringData(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetSteeringData(static_cast<const Mac::ExtAddress *>(aExtAddress));
}
#endif

const otPskc *otThreadGetPskc(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<KeyManager>().GetPskc();
}

otError otThreadSetPskc(otInstance *aInstance, const otPskc *aPskc)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    instance.Get<KeyManager>().SetPskc(*static_cast<const Pskc *>(aPskc));
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

otError otThreadSetParentPriority(otInstance *aInstance, int8_t aParentPriority)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetAssignParentPriority(aParentPriority);
}

void otThreadRegisterNeighborTableCallback(otInstance *aInstance, otNeighborTableCallback aCallback)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NeighborTable>().RegisterCallback(aCallback);
}

void otThreadSetDiscoveryRequestCallback(otInstance *                     aInstance,
                                         otThreadDiscoveryRequestCallback aCallback,
                                         void *                           aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetDiscoveryRequestCallback(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void otThreadSendAddressNotification(otInstance *              aInstance,
                                     otIp6Address *            aDestination,
                                     otIp6Address *            aTarget,
                                     otIp6InterfaceIdentifier *aMlIid)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<AddressResolver>().SendAddressQueryResponse(static_cast<Ip6::Address &>(*aTarget),
                                                             static_cast<Ip6::InterfaceIdentifier &>(*aMlIid), nullptr,
                                                             static_cast<Ip6::Address &>(*aDestination));
}
#endif

#endif // OPENTHREAD_FTD
