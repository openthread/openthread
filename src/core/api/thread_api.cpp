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

#define WPP_NAME "thread_api.tmh"

#include "openthread-core-config.h"

#include <openthread/thread.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"

using namespace ot;

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
    otError           error    = OT_ERROR_NONE;
    Instance &        instance = *static_cast<Instance *>(aInstance);
    otMeshLocalPrefix prefix;

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetExtendedPanId(*aExtendedPanId);

    prefix.m8[0] = 0xfd;
    memcpy(&prefix.m8[1], aExtendedPanId->m8, 5);
    prefix.m8[6] = 0x00;
    prefix.m8[7] = 0x00;
    instance.Get<Mle::MleRouter>().SetMeshLocalPrefix(prefix);

    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aLeaderRloc != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<Mle::MleRouter>().GetLeaderAddress(*static_cast<Ip6::Address *>(aLeaderRloc));

exit:
    return error;
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
    otLinkModeConfig config;
    Instance &       instance = *static_cast<Instance *>(aInstance);
    uint8_t          mode     = instance.Get<Mle::MleRouter>().GetDeviceMode();

    memset(&config, 0, sizeof(otLinkModeConfig));

    if (mode & Mle::ModeTlv::kModeRxOnWhenIdle)
    {
        config.mRxOnWhenIdle = 1;
    }

    if (mode & Mle::ModeTlv::kModeSecureDataRequest)
    {
        config.mSecureDataRequests = 1;
    }

    if (mode & Mle::ModeTlv::kModeFullThreadDevice)
    {
        config.mDeviceType = 1;
    }

    if (mode & Mle::ModeTlv::kModeFullNetworkData)
    {
        config.mNetworkData = 1;
    }

    return config;
}

otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
{
    uint8_t   mode     = 0;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aConfig.mRxOnWhenIdle)
    {
        mode |= Mle::ModeTlv::kModeRxOnWhenIdle;
    }

    if (aConfig.mSecureDataRequests)
    {
        mode |= Mle::ModeTlv::kModeSecureDataRequest;
    }

    if (aConfig.mDeviceType)
    {
        mode |= Mle::ModeTlv::kModeFullThreadDevice;
    }

    if (aConfig.mNetworkData)
    {
        mode |= Mle::ModeTlv::kModeFullNetworkData;
    }

    return instance.Get<Mle::MleRouter>().SetDeviceMode(mode);
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

    VerifyOrExit(aKey != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    error = instance.Get<KeyManager>().SetMasterKey(*aKey);
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
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

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<Mle::MleRouter>().SetMeshLocalPrefix(*aMeshLocalPrefix);
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

    return instance.Get<Mac::Mac>().GetNetworkName();
}

otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    error = instance.Get<Mac::Mac>().SetNetworkName(aNetworkName);
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

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
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<Mle::MleRouter>().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return static_cast<otDeviceRole>(instance.Get<Mle::MleRouter>().GetRole());
}

otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aLeaderData != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<Mle::MleRouter>().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderDataTlv().GetWeighting();
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otThreadGetRloc16(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().GetRloc16();
}

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aParentInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    parent                       = instance.Get<Mle::MleRouter>().GetParent();
    aParentInfo->mExtAddress     = parent->GetExtAddress();
    aParentInfo->mRloc16         = parent->GetRloc16();
    aParentInfo->mRouterId       = Mle::Mle::GetRouterId(parent->GetRloc16());
    aParentInfo->mNextHop        = parent->GetNextHop();
    aParentInfo->mPathCost       = parent->GetCost();
    aParentInfo->mLinkQualityIn  = parent->GetLinkInfo().GetLinkQuality();
    aParentInfo->mLinkQualityOut = parent->GetLinkQualityOut();
    aParentInfo->mAge = static_cast<uint8_t>(TimerMilli::MsecToSec(TimerMilli::GetNow() - parent->GetLastHeard()));
    aParentInfo->mAllocated       = true;
    aParentInfo->mLinkEstablished = parent->GetState() == Neighbor::kStateValid;

exit:
    return error;
}

otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aParentRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent       = instance.Get<Mle::MleRouter>().GetParent();
    *aParentRssi = parent->GetLinkInfo().GetAverageRss();

    VerifyOrExit(*aParentRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aLastRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent     = instance.Get<Mle::MleRouter>().GetParent();
    *aLastRssi = parent->GetLinkInfo().GetLastRss();

    VerifyOrExit(*aLastRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
void otThreadSetReceiveDiagnosticGetCallback(otInstance *                   aInstance,
                                             otReceiveDiagnosticGetCallback aCallback,
                                             void *                         aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkDiagnostic::NetworkDiagnostic>().SetReceiveDiagnosticGetCallback(aCallback, aCallbackContext);
}

otError otThreadSendDiagnosticGet(otInstance *        aInstance,
                                  const otIp6Address *aDestination,
                                  const uint8_t       aTlvTypes[],
                                  uint8_t             aCount)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkDiagnostic::NetworkDiagnostic>().SendDiagnosticGet(
        *static_cast<const Ip6::Address *>(aDestination), aTlvTypes, aCount);
}

otError otThreadSendDiagnosticReset(otInstance *        aInstance,
                                    const otIp6Address *aDestination,
                                    const uint8_t       aTlvTypes[],
                                    uint8_t             aCount)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<NetworkDiagnostic::NetworkDiagnostic>().SendDiagnosticReset(
        *static_cast<const Ip6::Address *>(aDestination), aTlvTypes, aCount);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aEnabled)
    {
        VerifyOrExit(instance.Get<Mac::Mac>().GetPanId() != Mac::kPanIdBroadcast, error = OT_ERROR_INVALID_STATE);
        error = instance.Get<Mle::MleRouter>().Start(/* aAnnounceAttach */ false);
    }
    else
    {
        instance.Get<Mle::MleRouter>().Stop(true);
    }

exit:
    return error;
}

bool otThreadGetAutoStart(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t   autoStart = 0;
    Instance &instance  = *static_cast<Instance *>(aInstance);

    if (instance.Get<Settings>().ReadThreadAutoStart(autoStart) != OT_ERROR_NONE)
    {
        autoStart = 0;
    }

    return autoStart != 0;
#else
    OT_UNUSED_VARIABLE(aInstance);
    return false;
#endif
}

otError otThreadSetAutoStart(otInstance *aInstance, bool aStartAutomatically)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t   autoStart = aStartAutomatically ? 1 : 0;
    Instance &instance  = *static_cast<Instance *>(aInstance);

    return instance.Get<Settings>().SaveThreadAutoStart(autoStart);
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartAutomatically);
    return OT_ERROR_NOT_IMPLEMENTED;
#endif
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

    return instance.Get<Mle::MleRouter>().Discover(static_cast<Mac::ChannelMask>(aScanChannels), aPanId, aJoiner,
                                                   aEnableEui64Filtering, aCallback, aCallbackContext);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mle::MleRouter>().IsDiscoverInProgress();
}

const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<MeshForwarder>().GetCounters();
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

otError otThreadRegisterParentResponseCallback(otInstance *                   aInstance,
                                               otThreadParentResponseCallback aCallback,
                                               void *                         aContext)
{
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mle::MleRouter>().RegisterParentResponseStatsCallback(aCallback, aContext);

    return OT_ERROR_NONE;
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aContext);

    return OT_ERROR_DISABLED_FEATURE;
#endif
}
