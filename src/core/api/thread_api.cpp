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
 *   This file implements the OpenThread Thread API (for both FTD and MTD).
 */

#include "openthread-core-config.h"

#include <openthread/thread.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/settings.hpp"

using namespace ot;

#if OPENTHREAD_FTD || OPENTHREAD_MTD
uint32_t otThreadGetChildTimeout(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetTimeout();
}

void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().SetTimeout(aTimeout);
}

const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mac::Mac>().GetExtendedPanId();
}

otError otThreadSetExtendedPanId(otInstance *aInstance, const otExtendedPanId *aExtendedPanId)
{
    otError                   error    = OT_ERROR_NONE;
    Instance &                instance = *static_cast<Instance *>(aInstance);
    const Mac::ExtendedPanId &extPanId = *static_cast<const Mac::ExtendedPanId *>(aExtendedPanId);
    Mle::MeshLocalPrefix      prefix;

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetExtendedPanId(extPanId);

    prefix.SetFromExtendedPanId(extPanId);
    instance.Get<Mle::MleRouter>().SetMeshLocalPrefix(prefix);

    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aLeaderRloc != nullptr);

    return instance.Get<Mle::MleRouter>().GetLeaderAddress(*static_cast<Ip6::Address *>(aLeaderRloc));
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
    otLinkModeConfig config;
    Instance &       instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().GetDeviceMode().Get(config);

    return config;
}

otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().SetDeviceMode(Mle::DeviceMode(aConfig));
}

const otMasterKey *otThreadGetMasterKey(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<KeyManager>().GetMasterKey();
}

otError otThreadSetMasterKey(otInstance *aInstance, const otMasterKey *aKey)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aKey != nullptr);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    error = instance.Get<KeyManager>().SetMasterKey(*static_cast<const MasterKey *>(aKey));
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

const otIp6Address *otThreadGetRloc(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mle::MleRouter>().GetMeshLocal16();
}

const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mle::MleRouter>().GetMeshLocal64();
}

const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mle::MleRouter>().GetMeshLocalPrefix();
}

otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const otMeshLocalPrefix *aMeshLocalPrefix)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    instance.Get<Mle::MleRouter>().SetMeshLocalPrefix(*static_cast<const Mle::MeshLocalPrefix *>(aMeshLocalPrefix));
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mle::MleRouter>().GetLinkLocalAddress();
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetNetworkName().GetAsCString();
}

otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    error = instance.Get<Mac::Mac>().SetNetworkName(aNetworkName);
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
const char *otThreadGetDomainName(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetDomainName().GetAsCString();
}

otError otThreadSetDomainName(otInstance *aInstance, const char *aDomainName)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    error = instance.Get<Mac::Mac>().SetDomainName(aDomainName);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
otError otThreadSetFixedDuaInterfaceIdentifier(otInstance *aInstance, const otIp6InterfaceIdentifier *aIid)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    otError   error    = OT_ERROR_NONE;

    if (aIid)
    {
        error = instance.Get<DuaManager>().SetFixedDuaInterfaceIdentifier(
            *static_cast<const Ip6::InterfaceIdentifier *>(aIid));
    }
    else
    {
        instance.Get<DuaManager>().ClearFixedDuaInterfaceIdentifier();
    }

    return error;
}

const otIp6InterfaceIdentifier *otThreadGetFixedDuaInterfaceIdentifier(otInstance *aInstance)
{
    Instance &                      instance = *static_cast<Instance *>(aInstance);
    const otIp6InterfaceIdentifier *iid      = nullptr;

    if (instance.Get<DuaManager>().IsFixedDuaInterfaceIdentifierSet())
    {
        iid = &instance.Get<DuaManager>().GetFixedDuaInterfaceIdentifier();
    }

    return iid;
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<KeyManager>().GetCurrentKeySequence();
}

void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<KeyManager>().SetCurrentKeySequence(aKeySequenceCounter);
}

uint32_t otThreadGetKeySwitchGuardTime(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<KeyManager>().GetKeySwitchGuardTime();
}

void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<KeyManager>().SetKeySwitchGuardTime(aKeySwitchGuardTime);
}

otError otThreadBecomeDetached(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().BecomeDetached();
}

otError otThreadBecomeChild(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().BecomeChild(Mle::kAttachAny);
}

otError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT((aInfo != nullptr) && (aIterator != nullptr));

    return instance.Get<NeighborTable>().GetNextNeighborInfo(*aIterator, *static_cast<Neighbor::Info *>(aInfo));
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return static_cast<otDeviceRole>(instance.Get<Mle::MleRouter>().GetRole());
}

otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    otError   error    = OT_ERROR_NONE;

    OT_ASSERT(aLeaderData != nullptr);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_DETACHED);
    *aLeaderData = instance.Get<Mle::MleRouter>().GetLeaderData();

exit:
    return error;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderId();
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderData().GetWeighting();
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderData().GetPartitionId();
}

uint16_t otThreadGetRloc16(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetRloc16();
}

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    otError   error    = OT_ERROR_NONE;
    Router *  parent;

    OT_ASSERT(aParentInfo != nullptr);

    // Reference device needs get the original parent's info even after the node state changed.
#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(instance.Get<Mle::MleRouter>().IsChild(), error = OT_ERROR_INVALID_STATE);
#endif

    parent = &instance.Get<Mle::MleRouter>().GetParent();

    aParentInfo->mExtAddress     = parent->GetExtAddress();
    aParentInfo->mRloc16         = parent->GetRloc16();
    aParentInfo->mRouterId       = Mle::Mle::RouterIdFromRloc16(parent->GetRloc16());
    aParentInfo->mNextHop        = parent->GetNextHop();
    aParentInfo->mPathCost       = parent->GetCost();
    aParentInfo->mLinkQualityIn  = parent->GetLinkInfo().GetLinkQuality();
    aParentInfo->mLinkQualityOut = parent->GetLinkQualityOut();
    aParentInfo->mAge            = static_cast<uint8_t>(Time::MsecToSec(TimerMilli::GetNow() - parent->GetLastHeard()));
    aParentInfo->mAllocated      = true;
    aParentInfo->mLinkEstablished = parent->IsStateValid();

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
exit:
#endif
    return error;
}

otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aParentRssi != nullptr);

    *aParentRssi = instance.Get<Mle::MleRouter>().GetParent().GetLinkInfo().GetAverageRss();

    VerifyOrExit(*aParentRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aLastRssi != nullptr);

    *aLastRssi = instance.Get<Mle::MleRouter>().GetParent().GetLinkInfo().GetLastRss();

    VerifyOrExit(*aLastRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aEnabled)
    {
        error = instance.Get<Mle::MleRouter>().Start(/* aAnnounceAttach */ false);
    }
    else
    {
        instance.Get<Mle::MleRouter>().Stop(true);
    }

    return error;
}

uint16_t otThreadGetVersion(void)
{
    return OPENTHREAD_CONFIG_THREAD_VERSION;
}

bool otThreadIsSingleton(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().IsSingleton();
}

otError otThreadDiscover(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aPanId,
                         bool                     aJoiner,
                         bool                     aEnableEui64Filtering,
                         otHandleActiveScanResult aCallback,
                         void *                   aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::DiscoverScanner>().Discover(
        static_cast<Mac::ChannelMask>(aScanChannels), aPanId, aJoiner, aEnableEui64Filtering,
        /* aFilterIndexes (use hash of factory EUI64) */ nullptr, aCallback, aCallbackContext);
}

otError otThreadSetJoinerAdvertisement(otInstance *   aInstance,
                                       uint32_t       aOui,
                                       const uint8_t *aAdvData,
                                       uint8_t        aAdvDataLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::DiscoverScanner>().SetJoinerAdvertisement(aOui, aAdvData, aAdvDataLength);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::DiscoverScanner>().IsInProgress();
}

const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<MeshForwarder>().GetCounters();
}

void otThreadResetIp6Counters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<MeshForwarder>().ResetCounters();
}

const otMleCounters *otThreadGetMleCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mle::MleRouter>().GetCounters();
}

void otThreadResetMleCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().ResetCounters();
}

void otThreadRegisterParentResponseCallback(otInstance *                   aInstance,
                                            otThreadParentResponseCallback aCallback,
                                            void *                         aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().RegisterParentResponseStatsCallback(aCallback, aContext);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
