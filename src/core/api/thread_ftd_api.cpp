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

#include "instance/instance.hpp"

using namespace ot;

uint16_t otThreadGetMaxAllowedChildren(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<ChildTable>().GetMaxChildrenAllowed();
}

otError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint16_t aMaxChildren)
{
    return AsCoreType(aInstance).Get<ChildTable>().SetMaxChildrenAllowed(aMaxChildren);
}

uint8_t otThreadGetMaxChildIpAddresses(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetMaxChildIpAddresses();
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError otThreadSetMaxChildIpAddresses(otInstance *aInstance, uint8_t aMaxIpAddresses)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetMaxChildIpAddresses(aMaxIpAddresses);
}
#endif

bool otThreadIsRouterEligible(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().IsRouterEligible();
}

otError otThreadSetRouterEligible(otInstance *aInstance, bool aEligible)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetRouterEligible(aEligible);
}

otError otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetPreferredRouterId(aRouterId);
}

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
const otDeviceProperties *otThreadGetDeviceProperties(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetDeviceProperties();
}

void otThreadSetDeviceProperties(otInstance *aInstance, const otDeviceProperties *aDeviceProperties)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetDeviceProperties(AsCoreType(aDeviceProperties));
}
#endif

uint8_t otThreadGetLocalLeaderWeight(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetLeaderWeight();
}

void otThreadSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetLeaderWeight(aWeight);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
uint32_t otThreadGetPreferredLeaderPartitionId(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetPreferredLeaderPartitionId();
}

void otThreadSetPreferredLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetPreferredLeaderPartitionId(aPartitionId);
}
#endif

uint16_t otThreadGetJoinerUdpPort(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort();
}

otError otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
{
    AsCoreType(aInstance).Get<MeshCoP::JoinerRouter>().SetJoinerUdpPort(aJoinerUdpPort);

    return kErrorNone;
}

uint32_t otThreadGetContextIdReuseDelay(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<NetworkData::Leader>().GetContextIdReuseDelay();
}

void otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay)
{
    AsCoreType(aInstance).Get<NetworkData::Leader>().SetContextIdReuseDelay(aDelay);
}

uint8_t otThreadGetNetworkIdTimeout(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetNetworkIdTimeout();
}

void otThreadSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetNetworkIdTimeout(aTimeout);
}

uint8_t otThreadGetRouterUpgradeThreshold(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetRouterUpgradeThreshold();
}

void otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetRouterUpgradeThreshold(aThreshold);
}

uint8_t otThreadGetChildRouterLinks(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetChildRouterLinks();
}

otError otThreadSetChildRouterLinks(otInstance *aInstance, uint8_t aChildRouterLinks)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetChildRouterLinks(aChildRouterLinks);
}

otError otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    Error error = kErrorNone;

    VerifyOrExit(aRouterId <= Mle::kMaxRouterId, error = kErrorInvalidArgs);

    error = AsCoreType(aInstance).Get<RouterTable>().Release(aRouterId);

exit:
    return error;
}

otError otThreadBecomeRouter(otInstance *aInstance)
{
    Error error = kErrorInvalidState;

    switch (AsCoreType(aInstance).Get<Mle::MleRouter>().GetRole())
    {
    case Mle::kRoleDisabled:
    case Mle::kRoleDetached:
        break;

    case Mle::kRoleChild:
        error = AsCoreType(aInstance).Get<Mle::MleRouter>().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case Mle::kRoleRouter:
    case Mle::kRoleLeader:
        error = kErrorNone;
        break;
    }

    return error;
}

otError otThreadBecomeLeader(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().BecomeLeader(/* aCheckWeight */ true);
}

uint8_t otThreadGetRouterDowngradeThreshold(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetRouterDowngradeThreshold();
}

void otThreadSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otThreadGetRouterSelectionJitter(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetRouterSelectionJitter();
}

void otThreadSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetRouterSelectionJitter(aRouterJitter);
}

otError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    return AsCoreType(aInstance).Get<ChildTable>().GetChildInfoById(aChildId, AsCoreType(aChildInfo));
}

otError otThreadGetChildInfoByIndex(otInstance *aInstance, uint16_t aChildIndex, otChildInfo *aChildInfo)
{
    return AsCoreType(aInstance).Get<ChildTable>().GetChildInfoByIndex(aChildIndex, AsCoreType(aChildInfo));
}

otError otThreadGetChildNextIp6Address(otInstance                *aInstance,
                                       uint16_t                   aChildIndex,
                                       otChildIp6AddressIterator *aIterator,
                                       otIp6Address              *aAddress)
{
    Error        error = kErrorNone;
    const Child *child;

    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aAddress);

    child = AsCoreType(aInstance).Get<ChildTable>().GetChildAtIndex(aChildIndex);
    VerifyOrExit(child != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(child->IsStateValidOrRestoring(), error = kErrorInvalidArgs);

    error = child->GetNextIp6Address(*aIterator, AsCoreType(aAddress));

exit:
    return error;
}

uint8_t otThreadGetRouterIdSequence(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<RouterTable>().GetRouterIdSequence();
}

uint8_t otThreadGetMaxRouterId(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return Mle::kMaxRouterId;
}

otError otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    return AsCoreType(aInstance).Get<RouterTable>().GetRouterInfo(aRouterId, AsCoreType(aRouterInfo));
}

otError otThreadGetNextCacheEntry(otInstance *aInstance, otCacheEntryInfo *aEntryInfo, otCacheEntryIterator *aIterator)
{
    return AsCoreType(aInstance).Get<AddressResolver>().GetNextCacheEntry(AsCoreType(aEntryInfo),
                                                                          AsCoreType(aIterator));
}

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
void otThreadSetSteeringData(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetSteeringData(AsCoreTypePtr(aExtAddress));
}
#endif

void otThreadGetPskc(otInstance *aInstance, otPskc *aPskc)
{
    AsCoreType(aInstance).Get<KeyManager>().GetPskc(AsCoreType(aPskc));
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
otPskcRef otThreadGetPskcRef(otInstance *aInstance) { return AsCoreType(aInstance).Get<KeyManager>().GetPskcRef(); }
#endif

otError otThreadSetPskc(otInstance *aInstance, const otPskc *aPskc)
{
    Error error = kErrorNone;

    VerifyOrExit(AsCoreType(aInstance).Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    AsCoreType(aInstance).Get<KeyManager>().SetPskc(AsCoreType(aPskc));
    AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Clear();
    AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
otError otThreadSetPskcRef(otInstance *aInstance, otPskcRef aKeyRef)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(aKeyRef != 0, error = kErrorInvalidArgs);
    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    instance.Get<KeyManager>().SetPskcRef(aKeyRef);
    instance.Get<MeshCoP::ActiveDatasetManager>().Clear();
    instance.Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}
#endif

int8_t otThreadGetParentPriority(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetAssignParentPriority();
}

otError otThreadSetParentPriority(otInstance *aInstance, int8_t aParentPriority)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetAssignParentPriority(aParentPriority);
}

void otThreadRegisterNeighborTableCallback(otInstance *aInstance, otNeighborTableCallback aCallback)
{
    AsCoreType(aInstance).Get<NeighborTable>().RegisterCallback(aCallback);
}

void otThreadSetDiscoveryRequestCallback(otInstance                      *aInstance,
                                         otThreadDiscoveryRequestCallback aCallback,
                                         void                            *aContext)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetDiscoveryRequestCallback(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

void otThreadSendAddressNotification(otInstance               *aInstance,
                                     otIp6Address             *aDestination,
                                     otIp6Address             *aTarget,
                                     otIp6InterfaceIdentifier *aMlIid)
{
    AsCoreType(aInstance).Get<AddressResolver>().SendAddressQueryResponse(AsCoreType(aTarget), AsCoreType(aMlIid),
                                                                          nullptr, AsCoreType(aDestination));
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
otError otThreadSendProactiveBackboneNotification(otInstance               *aInstance,
                                                  otIp6Address             *aTarget,
                                                  otIp6InterfaceIdentifier *aMlIid,
                                                  uint32_t                  aTimeSinceLastTransaction)
{
    return AsCoreType(aInstance).Get<BackboneRouter::Manager>().SendProactiveBackboneNotification(
        AsCoreType(aTarget), AsCoreType(aMlIid), aTimeSinceLastTransaction);
}
#endif

void otThreadSetCcmEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetCcmEnabled(aEnabled);
}

void otThreadSetThreadVersionCheckEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetThreadVersionCheckEnabled(aEnabled);
}

void otThreadSetTmfOriginFilterEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<Ip6::Ip6>().SetTmfOriginFilterEnabled(aEnabled);
}

bool otThreadIsTmfOriginFilterEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Ip6::Ip6>().IsTmfOriginFilterEnabled();
}

void otThreadGetRouterIdRange(otInstance *aInstance, uint8_t *aMinRouterId, uint8_t *aMaxRouterId)
{
    AssertPointerIsNotNull(aMinRouterId);
    AssertPointerIsNotNull(aMaxRouterId);

    AsCoreType(aInstance).Get<RouterTable>().GetRouterIdRange(*aMinRouterId, *aMaxRouterId);
}

otError otThreadSetRouterIdRange(otInstance *aInstance, uint8_t aMinRouterId, uint8_t aMaxRouterId)
{
    return AsCoreType(aInstance).Get<RouterTable>().SetRouterIdRange(aMinRouterId, aMaxRouterId);
}

uint32_t otThreadGetAdvertisementTrickleIntervalMax(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetAdvertisementTrickleIntervalMax();
}

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

bool otThreadIsRouterIdAllocated(otInstance *aInstance, uint8_t aRouterId)
{
    return AsCoreType(aInstance).Get<RouterTable>().IsAllocated(aRouterId);
}

void otThreadGetNextHopAndPathCost(otInstance *aInstance,
                                   uint16_t    aDestRloc16,
                                   uint16_t   *aNextHopRloc16,
                                   uint8_t    *aPathCost)
{
    uint8_t  pathcost;
    uint16_t nextHopRloc16;

    AsCoreType(aInstance).Get<RouterTable>().GetNextHopAndPathCost(
        aDestRloc16, (aNextHopRloc16 != nullptr) ? *aNextHopRloc16 : nextHopRloc16,
        (aPathCost != nullptr) ? *aPathCost : pathcost);
}

#endif // OPENTHREAD_FTD
